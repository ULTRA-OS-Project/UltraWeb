// UltraWeb/core/UltraWebCSSCompiler.cpp
// CSS to UCS Binary Compiler Implementation
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "../include/UltraWebCSSCompiler.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace UltraWeb {

// ============================================================================
// BINARY WRITER IMPLEMENTATION
// ============================================================================

BinaryWriter::BinaryWriter() {
    buffer.reserve(4096);  // Pre-allocate reasonable size
}

void BinaryWriter::WriteUInt8(uint8_t value) {
    buffer.push_back(value);
}

void BinaryWriter::WriteUInt16(uint16_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void BinaryWriter::WriteUInt32(uint32_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void BinaryWriter::WriteInt16(int16_t value) {
    WriteUInt16(static_cast<uint16_t>(value));
}

void BinaryWriter::WriteInt32(int32_t value) {
    WriteUInt32(static_cast<uint32_t>(value));
}

void BinaryWriter::WriteFloat(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    WriteUInt32(bits);
}

void BinaryWriter::WriteBytes(const uint8_t* data, size_t length) {
    buffer.insert(buffer.end(), data, data + length);
}

void BinaryWriter::WriteString(const std::string& str) {
    // Length-prefixed string (2 bytes length + data)
    WriteUInt16(static_cast<uint16_t>(str.length()));
    buffer.insert(buffer.end(), str.begin(), str.end());
}

void BinaryWriter::WriteLength(const UCSLength& length) {
    WriteInt16(length.value);
    WriteUInt8(static_cast<uint8_t>(length.unit));
}

void BinaryWriter::WriteColor(const UCSColor& color) {
    WriteUInt8(color.r);
    WriteUInt8(color.g);
    WriteUInt8(color.b);
    WriteUInt8(color.a);
}

void BinaryWriter::WriteBoxShadow(const UCSBoxShadow& shadow) {
    WriteInt16(shadow.offsetX);
    WriteInt16(shadow.offsetY);
    WriteUInt16(shadow.blurRadius);
    WriteUInt16(shadow.spreadRadius);
    WriteColor(shadow.color);
    WriteUInt8(shadow.inset);
    WriteUInt8(shadow.reserved);
}

void BinaryWriter::WriteGradientStop(const UCSGradientStop& stop) {
    WriteColor(stop.color);
    WriteUInt8(stop.position);
}

void BinaryWriter::PatchUInt32(size_t position, uint32_t value) {
    if (position + 4 <= buffer.size()) {
        buffer[position] = static_cast<uint8_t>(value & 0xFF);
        buffer[position + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        buffer[position + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        buffer[position + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    }
}

void BinaryWriter::PatchUInt16(size_t position, uint16_t value) {
    if (position + 2 <= buffer.size()) {
        buffer[position] = static_cast<uint8_t>(value & 0xFF);
        buffer[position + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    }
}

// ============================================================================
// STRING TABLE IMPLEMENTATION
// ============================================================================

StringTable::StringTable() {
    // Add empty string as first entry
    AddString("");
}

uint16_t StringTable::AddString(const std::string& str) {
    auto it = stringToId.find(str);
    if (it != stringToId.end()) {
        return it->second;
    }
    
    uint16_t id = static_cast<uint16_t>(strings.size());
    strings.push_back(str);
    stringToId[str] = id;
    return id;
}

const std::string& StringTable::GetString(uint16_t id) const {
    static const std::string empty;
    if (id >= strings.size()) return empty;
    return strings[id];
}

uint16_t StringTable::FindString(const std::string& str) const {
    auto it = stringToId.find(str);
    if (it != stringToId.end()) {
        return it->second;
    }
    return 0xFFFF;
}

void StringTable::WriteTo(BinaryWriter& writer) const {
    // Write count
    writer.WriteUInt16(static_cast<uint16_t>(strings.size()));
    
    // Write each string (length-prefixed)
    for (const auto& str : strings) {
        writer.WriteString(str);
    }
}

size_t StringTable::GetByteSize() const {
    size_t size = 2;  // String count
    for (const auto& str : strings) {
        size += 2 + str.length();  // Length prefix + string data
    }
    return size;
}

// ============================================================================
// CSS COMPILER IMPLEMENTATION
// ============================================================================

CSSCompiler::CSSCompiler()
    : optimizeOutput(true)
    , includeSourceMap(false)
{
}

void CSSCompiler::Reset() {
    writer.Clear();
    stringTable = StringTable();
}

CompilationResult CSSCompiler::Compile(const std::string& css) {
    CSSParser parser;
    CSSStylesheet stylesheet = parser.Parse(css);
    
    CompilationResult result = Compile(stylesheet);
    result.originalSize = css.length();
    
    // Recalculate compression ratio now that we have original size
    if (result.originalSize > 0 && result.compiledSize > 0) {
        result.compressionRatio = static_cast<float>(result.originalSize) / 
                                  static_cast<float>(result.compiledSize);
    }
    
    if (!stylesheet.errors.empty()) {
        for (const auto& error : stylesheet.errors) {
            result.AddError("Parse error: " + error);
        }
    }
    
    return result;
}

CompilationResult CSSCompiler::Compile(const CSSStylesheet& stylesheet) {
    CompilationResult result;
    Reset();
    
    try {
        // Build sections
        std::vector<uint8_t> variableSection = BuildVariableTable(stylesheet.variables);
        std::vector<uint8_t> selectorSection = BuildSelectorTable(stylesheet.rules);
        std::vector<uint8_t> ruleSection = BuildRuleTable(stylesheet.rules);
        std::vector<uint8_t> stringSection = BuildStringTable();
        
        // Calculate offsets
        size_t headerSize = sizeof(UCSHeader);
        size_t variableOffset = headerSize;
        size_t selectorOffset = variableOffset + variableSection.size();
        size_t ruleOffset = selectorOffset + selectorSection.size();
        // String table follows rule table
        
        // Write header
        UCSHeader header;
        header.magic = UCS_MAGIC;
        header.version = UCS_VERSION;
        header.ruleCount = static_cast<uint16_t>(stylesheet.rules.size());
        header.variableTableOffset = static_cast<uint32_t>(variableOffset);
        header.selectorTableOffset = static_cast<uint32_t>(selectorOffset);
        header.propertyTableOffset = static_cast<uint32_t>(ruleOffset);
        
        writer.WriteUInt32(header.magic);
        writer.WriteUInt16(header.version);
        writer.WriteUInt16(header.ruleCount);
        writer.WriteUInt32(header.selectorTableOffset);
        writer.WriteUInt32(header.propertyTableOffset);
        writer.WriteUInt32(header.variableTableOffset);
        
        // Write sections
        writer.WriteBytes(variableSection.data(), variableSection.size());
        writer.WriteBytes(selectorSection.data(), selectorSection.size());
        writer.WriteBytes(ruleSection.data(), ruleSection.size());
        writer.WriteBytes(stringSection.data(), stringSection.size());
        
        // Set result
        result.success = true;
        result.data = writer.TakeData();
        result.ruleCount = stylesheet.rules.size();
        result.variableCount = stylesheet.variables.size();
        result.compiledSize = result.data.size();
        
        // Count selectors and properties
        for (const auto& rule : stylesheet.rules) {
            result.selectorCount += rule.selectors.size();
            result.propertyCount += rule.declarations.size();
        }
        
        if (result.originalSize > 0) {
            result.compressionRatio = static_cast<float>(result.originalSize) / 
                                      static_cast<float>(result.compiledSize);
        }
        
    } catch (const std::exception& e) {
        result.AddError(std::string("Compilation error: ") + e.what());
    }
    
    return result;
}

bool CSSCompiler::CompileToFile(const std::string& css, const std::string& outputPath) {
    CompilationResult result = Compile(css);
    
    if (!result.success) {
        return false;
    }
    
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(result.data.data()), result.data.size());
    file.close();
    
    return true;
}

bool CSSCompiler::CompileToFile(const CSSStylesheet& stylesheet, const std::string& outputPath) {
    CompilationResult result = Compile(stylesheet);
    
    if (!result.success) {
        return false;
    }
    
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(result.data.data()), result.data.size());
    file.close();
    
    return true;
}

std::vector<uint8_t> CSSCompiler::BuildVariableTable(const std::vector<CSSVariable>& variables) {
    BinaryWriter varWriter;
    
    // Write variable count
    varWriter.WriteUInt16(static_cast<uint16_t>(variables.size()));
    
    for (const auto& var : variables) {
        // Add variable name to string table
        uint16_t nameId = stringTable.AddString(var.name);
        
        UCSVariableHeader header;
        header.nameId = nameId;
        header.valueType = var.value.type;
        
        // Calculate value size
        size_t valueSize = GetValueSize(var.value.type);
        header.valueSize = static_cast<uint8_t>(valueSize);
        
        varWriter.WriteUInt16(header.nameId);
        varWriter.WriteUInt8(static_cast<uint8_t>(header.valueType));
        varWriter.WriteUInt8(header.valueSize);
        
        // Write value based on type
        if (auto* color = var.value.AsColor()) {
            varWriter.WriteUInt8(color->r);
            varWriter.WriteUInt8(color->g);
            varWriter.WriteUInt8(color->b);
            varWriter.WriteUInt8(color->a);
        } else if (auto* length = var.value.AsLength()) {
            varWriter.WriteInt16(static_cast<int16_t>(length->value));
            varWriter.WriteUInt8(static_cast<uint8_t>(length->unit));
        } else if (auto* str = var.value.AsString()) {
            uint16_t strId = stringTable.AddString(*str);
            varWriter.WriteUInt16(strId);
        } else if (auto* num = var.value.AsNumber()) {
            varWriter.WriteFloat(static_cast<float>(*num));
        }
    }
    
    return varWriter.TakeData();
}

std::vector<uint8_t> CSSCompiler::BuildSelectorTable(const std::vector<CSSRule>& rules) {
    BinaryWriter selWriter;
    
    // Count total selectors
    uint16_t totalSelectors = 0;
    for (const auto& rule : rules) {
        totalSelectors += static_cast<uint16_t>(rule.selectors.size());
    }
    
    selWriter.WriteUInt16(totalSelectors);
    
    for (const auto& rule : rules) {
        for (const auto& selector : rule.selectors) {
            // Add selector text to string table
            uint16_t stringId = stringTable.AddString(selector.text);
            
            UCSSelectorHeader header;
            header.hash = selector.hash;
            header.stringOffset = stringId;  // Using as string table reference
            header.stringLength = static_cast<uint16_t>(selector.text.length());
            header.pseudoClass = selector.pseudoClass;
            header.reserved = 0;
            
            selWriter.WriteUInt32(header.hash);
            selWriter.WriteUInt16(header.stringOffset);
            selWriter.WriteUInt16(header.stringLength);
            selWriter.WriteUInt8(static_cast<uint8_t>(header.pseudoClass));
            selWriter.WriteUInt8(header.reserved);
        }
    }
    
    return selWriter.TakeData();
}

std::vector<uint8_t> CSSCompiler::BuildRuleTable(const std::vector<CSSRule>& rules) {
    BinaryWriter ruleWriter;
    
    uint16_t selectorIndex = 0;
    
    for (const auto& rule : rules) {
        // For each selector in the rule, create a rule entry
        for (size_t i = 0; i < rule.selectors.size(); i++) {
            const auto& selector = rule.selectors[i];
            
            UCSRuleHeader header;
            header.selectorId = selectorIndex;
            header.specificity = selector.specificity;
            header.propertyCount = static_cast<uint16_t>(rule.declarations.size());
            
            ruleWriter.WriteUInt16(header.selectorId);
            ruleWriter.WriteUInt16(header.specificity);
            ruleWriter.WriteUInt16(header.propertyCount);
            
            // Write properties
            for (const auto& decl : rule.declarations) {
                CompileDeclarationToWriter(decl, ruleWriter);
            }
            
            selectorIndex++;
        }
    }
    
    return ruleWriter.TakeData();
}

void CSSCompiler::CompileDeclarationToWriter(const CSSDeclaration& decl, BinaryWriter& ruleWriter) {
    // Write property header
    ruleWriter.WriteUInt8(static_cast<uint8_t>(decl.propertyId));
    ruleWriter.WriteUInt8(static_cast<uint8_t>(decl.value.type));
    
    // Write value based on type
    switch (decl.value.type) {
        case CSSValueType::Color:
            if (auto* color = decl.value.AsColor()) {
                ruleWriter.WriteUInt8(color->r);
                ruleWriter.WriteUInt8(color->g);
                ruleWriter.WriteUInt8(color->b);
                ruleWriter.WriteUInt8(color->a);
            }
            break;
            
        case CSSValueType::Length:
            if (auto* length = decl.value.AsLength()) {
                ruleWriter.WriteInt16(static_cast<int16_t>(length->value));
                ruleWriter.WriteUInt8(static_cast<uint8_t>(length->unit));
            }
            break;
            
        case CSSValueType::MultiLength:
            if (auto* lengths = decl.value.AsMultiLength()) {
                ruleWriter.WriteUInt8(static_cast<uint8_t>(lengths->size()));
                for (const auto& len : *lengths) {
                    ruleWriter.WriteInt16(static_cast<int16_t>(len.value));
                    ruleWriter.WriteUInt8(static_cast<uint8_t>(len.unit));
                }
            }
            break;
            
        case CSSValueType::Enum:
            if (auto* str = decl.value.AsString()) {
                uint8_t enumValue = ResolveEnumValue(*str, decl.propertyId);
                ruleWriter.WriteUInt8(enumValue);
            }
            break;
            
        case CSSValueType::Float:
        case CSSValueType::Percentage:
            if (auto* num = decl.value.AsNumber()) {
                ruleWriter.WriteFloat(static_cast<float>(*num));
            }
            break;
            
        case CSSValueType::StringRef:
            if (auto* str = decl.value.AsString()) {
                uint16_t strId = stringTable.AddString(*str);
                ruleWriter.WriteUInt16(strId);
            }
            break;
            
        case CSSValueType::Shadow:
            if (auto* shadow = decl.value.AsShadow()) {
                ruleWriter.WriteInt16(static_cast<int16_t>(shadow->offsetX));
                ruleWriter.WriteInt16(static_cast<int16_t>(shadow->offsetY));
                ruleWriter.WriteUInt16(static_cast<uint16_t>(shadow->blurRadius));
                ruleWriter.WriteUInt16(static_cast<uint16_t>(shadow->spreadRadius));
                ruleWriter.WriteUInt8(shadow->color.r);
                ruleWriter.WriteUInt8(shadow->color.g);
                ruleWriter.WriteUInt8(shadow->color.b);
                ruleWriter.WriteUInt8(shadow->color.a);
                ruleWriter.WriteUInt8(shadow->inset ? 1 : 0);
                ruleWriter.WriteUInt8(0);  // Reserved
            }
            break;
            
        case CSSValueType::VarRef:
            if (auto* str = decl.value.AsString()) {
                uint16_t varId = stringTable.FindString(*str);
                if (varId == 0xFFFF) {
                    varId = stringTable.AddString(*str);
                }
                ruleWriter.WriteUInt16(varId);
            }
            break;
            
        case CSSValueType::Auto:
        case CSSValueType::None:
        case CSSValueType::Inherit:
            // No additional data needed
            break;
            
        default:
            // Unknown type - write nothing
            break;
    }
}

uint8_t CSSCompiler::ResolveEnumValue(const std::string& value, CSSPropertyId propertyId) {
    std::string lowerValue = value;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
    
    switch (propertyId) {
        case CSSPropertyId::Display:
            return ResolveDisplayValue(lowerValue);
        case CSSPropertyId::Position:
            return ResolvePositionValue(lowerValue);
        case CSSPropertyId::FlexDirection:
            return ResolveFlexDirectionValue(lowerValue);
        case CSSPropertyId::JustifyContent:
            return ResolveJustifyContentValue(lowerValue);
        case CSSPropertyId::AlignItems:
        case CSSPropertyId::AlignContent:
        case CSSPropertyId::AlignSelf:
            return ResolveAlignItemsValue(lowerValue);
        case CSSPropertyId::FlexWrap:
            return ResolveFlexWrapValue(lowerValue);
        case CSSPropertyId::TextAlign:
            return ResolveTextAlignValue(lowerValue);
        case CSSPropertyId::FontStyle:
            return ResolveFontStyleValue(lowerValue);
        case CSSPropertyId::TextDecoration:
            return ResolveTextDecorationValue(lowerValue);
        case CSSPropertyId::Visibility:
            return ResolveVisibilityValue(lowerValue);
        case CSSPropertyId::Overflow:
        case CSSPropertyId::OverflowX:
        case CSSPropertyId::OverflowY:
            return ResolveOverflowValue(lowerValue);
        case CSSPropertyId::Cursor:
            return ResolveCursorValue(lowerValue);
        case CSSPropertyId::BorderStyle:
            return ResolveBorderStyleValue(lowerValue);
        default:
            return 0;
    }
}

uint8_t CSSCompiler::ResolveDisplayValue(const std::string& value) {
    if (value == "block") return static_cast<uint8_t>(CSSDisplay::Block);
    if (value == "flex") return static_cast<uint8_t>(CSSDisplay::Flex);
    if (value == "grid") return static_cast<uint8_t>(CSSDisplay::Grid);
    if (value == "inline-block") return static_cast<uint8_t>(CSSDisplay::InlineBlock);
    if (value == "inline") return static_cast<uint8_t>(CSSDisplay::Inline);
    if (value == "inline-flex") return static_cast<uint8_t>(CSSDisplay::InlineFlex);
    if (value == "none") return static_cast<uint8_t>(CSSDisplay::None);
    return 0;
}

uint8_t CSSCompiler::ResolvePositionValue(const std::string& value) {
    if (value == "static") return static_cast<uint8_t>(CSSPosition::Static);
    if (value == "relative") return static_cast<uint8_t>(CSSPosition::Relative);
    if (value == "absolute") return static_cast<uint8_t>(CSSPosition::Absolute);
    if (value == "fixed") return static_cast<uint8_t>(CSSPosition::Fixed);
    if (value == "sticky") return static_cast<uint8_t>(CSSPosition::Sticky);
    return 0;
}

uint8_t CSSCompiler::ResolveFlexDirectionValue(const std::string& value) {
    if (value == "row") return static_cast<uint8_t>(CSSFlexDirection::Row);
    if (value == "row-reverse") return static_cast<uint8_t>(CSSFlexDirection::RowReverse);
    if (value == "column") return static_cast<uint8_t>(CSSFlexDirection::Column);
    if (value == "column-reverse") return static_cast<uint8_t>(CSSFlexDirection::ColumnReverse);
    return 0;
}

uint8_t CSSCompiler::ResolveJustifyContentValue(const std::string& value) {
    if (value == "flex-start" || value == "start") return static_cast<uint8_t>(CSSJustifyContent::FlexStart);
    if (value == "flex-end" || value == "end") return static_cast<uint8_t>(CSSJustifyContent::FlexEnd);
    if (value == "center") return static_cast<uint8_t>(CSSJustifyContent::Center);
    if (value == "space-between") return static_cast<uint8_t>(CSSJustifyContent::SpaceBetween);
    if (value == "space-around") return static_cast<uint8_t>(CSSJustifyContent::SpaceAround);
    if (value == "space-evenly") return static_cast<uint8_t>(CSSJustifyContent::SpaceEvenly);
    return 0;
}

uint8_t CSSCompiler::ResolveAlignItemsValue(const std::string& value) {
    if (value == "flex-start" || value == "start") return static_cast<uint8_t>(CSSAlignItems::FlexStart);
    if (value == "flex-end" || value == "end") return static_cast<uint8_t>(CSSAlignItems::FlexEnd);
    if (value == "center") return static_cast<uint8_t>(CSSAlignItems::Center);
    if (value == "stretch") return static_cast<uint8_t>(CSSAlignItems::Stretch);
    if (value == "baseline") return static_cast<uint8_t>(CSSAlignItems::Baseline);
    return 0;
}

uint8_t CSSCompiler::ResolveFlexWrapValue(const std::string& value) {
    if (value == "nowrap") return static_cast<uint8_t>(CSSFlexWrap::NoWrap);
    if (value == "wrap") return static_cast<uint8_t>(CSSFlexWrap::Wrap);
    if (value == "wrap-reverse") return static_cast<uint8_t>(CSSFlexWrap::WrapReverse);
    return 0;
}

uint8_t CSSCompiler::ResolveTextAlignValue(const std::string& value) {
    if (value == "left") return static_cast<uint8_t>(CSSTextAlign::Left);
    if (value == "right") return static_cast<uint8_t>(CSSTextAlign::Right);
    if (value == "center") return static_cast<uint8_t>(CSSTextAlign::Center);
    if (value == "justify") return static_cast<uint8_t>(CSSTextAlign::Justify);
    return 0;
}

uint8_t CSSCompiler::ResolveFontStyleValue(const std::string& value) {
    if (value == "normal") return static_cast<uint8_t>(CSSFontStyle::Normal);
    if (value == "italic") return static_cast<uint8_t>(CSSFontStyle::Italic);
    if (value == "oblique") return static_cast<uint8_t>(CSSFontStyle::Oblique);
    return 0;
}

uint8_t CSSCompiler::ResolveTextDecorationValue(const std::string& value) {
    if (value == "none") return static_cast<uint8_t>(CSSTextDecoration::None);
    if (value == "underline") return static_cast<uint8_t>(CSSTextDecoration::Underline);
    if (value == "line-through") return static_cast<uint8_t>(CSSTextDecoration::LineThrough);
    if (value == "overline") return static_cast<uint8_t>(CSSTextDecoration::Overline);
    return 0;
}

uint8_t CSSCompiler::ResolveVisibilityValue(const std::string& value) {
    if (value == "visible") return static_cast<uint8_t>(CSSVisibility::Visible);
    if (value == "hidden") return static_cast<uint8_t>(CSSVisibility::Hidden);
    return 0;
}

uint8_t CSSCompiler::ResolveOverflowValue(const std::string& value) {
    if (value == "visible") return static_cast<uint8_t>(CSSOverflow::Visible);
    if (value == "hidden") return static_cast<uint8_t>(CSSOverflow::Hidden);
    if (value == "scroll") return static_cast<uint8_t>(CSSOverflow::Scroll);
    if (value == "auto") return static_cast<uint8_t>(CSSOverflow::Auto);
    return 0;
}

uint8_t CSSCompiler::ResolveCursorValue(const std::string& value) {
    if (value == "auto") return static_cast<uint8_t>(CSSCursor::Auto);
    if (value == "pointer") return static_cast<uint8_t>(CSSCursor::Pointer);
    if (value == "text") return static_cast<uint8_t>(CSSCursor::Text);
    if (value == "move") return static_cast<uint8_t>(CSSCursor::Move);
    if (value == "not-allowed") return static_cast<uint8_t>(CSSCursor::NotAllowed);
    if (value == "grab") return static_cast<uint8_t>(CSSCursor::Grab);
    if (value == "grabbing") return static_cast<uint8_t>(CSSCursor::Grabbing);
    if (value == "default") return static_cast<uint8_t>(CSSCursor::Default);
    if (value == "crosshair") return static_cast<uint8_t>(CSSCursor::Crosshair);
    if (value == "help") return static_cast<uint8_t>(CSSCursor::Help);
    if (value == "wait") return static_cast<uint8_t>(CSSCursor::Wait);
    if (value == "progress") return static_cast<uint8_t>(CSSCursor::Progress);
    return 0;
}

uint8_t CSSCompiler::ResolveBorderStyleValue(const std::string& value) {
    if (value == "none") return static_cast<uint8_t>(CSSBorderStyle::None);
    if (value == "solid") return static_cast<uint8_t>(CSSBorderStyle::Solid);
    if (value == "dashed") return static_cast<uint8_t>(CSSBorderStyle::Dashed);
    if (value == "dotted") return static_cast<uint8_t>(CSSBorderStyle::Dotted);
    if (value == "double") return static_cast<uint8_t>(CSSBorderStyle::Double);
    return 0;
}

uint16_t CSSCompiler::ResolveFontWeightValue(const std::string& value) {
    if (value == "normal") return static_cast<uint16_t>(CSSFontWeight::Normal);
    if (value == "bold") return static_cast<uint16_t>(CSSFontWeight::Bold);
    if (value == "lighter" || value == "light") return static_cast<uint16_t>(CSSFontWeight::Light);
    if (value == "bolder") return static_cast<uint16_t>(CSSFontWeight::ExtraBold);
    
    // Try to parse as number
    try {
        int weight = std::stoi(value);
        return static_cast<uint16_t>(weight);
    } catch (...) {
        return static_cast<uint16_t>(CSSFontWeight::Normal);
    }
}

std::vector<uint8_t> CSSCompiler::BuildStringTable() {
    BinaryWriter strWriter;
    stringTable.WriteTo(strWriter);
    return strWriter.TakeData();
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

UCSFileInfo GetUCSFileInfo(const std::string& filePath) {
    UCSFileInfo info;
    info.valid = false;
    
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        info.error = "Cannot open file";
        return info;
    }
    
    info.fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    if (info.fileSize < sizeof(UCSHeader)) {
        info.error = "File too small to contain valid header";
        return info;
    }
    
    UCSHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    file.close();
    
    if (header.magic != UCS_MAGIC) {
        info.error = "Invalid magic number";
        return info;
    }
    
    info.valid = true;
    info.version = header.version;
    info.ruleCount = header.ruleCount;
    
    return info;
}

bool ValidateUCSFile(const std::string& filePath, std::string& error) {
    UCSFileInfo info = GetUCSFileInfo(filePath);
    if (!info.valid) {
        error = info.error;
        return false;
    }
    
    // Additional validation could be done here
    // - Check section offsets are within file bounds
    // - Verify section integrity
    // - etc.
    
    return true;
}

std::string DumpUCSFile(const std::string& filePath) {
    std::ostringstream oss;
    
    UCSFileInfo info = GetUCSFileInfo(filePath);
    if (!info.valid) {
        oss << "Invalid UCS file: " << info.error << "\n";
        return oss.str();
    }
    
    oss << "=== UCS File Dump ===\n";
    oss << "File: " << filePath << "\n";
    oss << "Size: " << info.fileSize << " bytes\n";
    oss << "Version: " << std::hex << info.version << std::dec << "\n";
    oss << "Rule Count: " << info.ruleCount << "\n";
    oss << "=====================\n";
    
    // Read full file for detailed dump
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        oss << "Cannot open file for detailed dump\n";
        return oss.str();
    }
    
    std::vector<uint8_t> data(info.fileSize);
    file.read(reinterpret_cast<char*>(data.data()), info.fileSize);
    file.close();
    
    // Dump header
    oss << "\n[Header]\n";
    oss << "  Magic: 0x" << std::hex << std::setfill('0') << std::setw(8) 
        << *reinterpret_cast<uint32_t*>(data.data()) << std::dec << "\n";
    oss << "  Version: 0x" << std::hex << std::setw(4) 
        << *reinterpret_cast<uint16_t*>(data.data() + 4) << std::dec << "\n";
    oss << "  Rule Count: " << *reinterpret_cast<uint16_t*>(data.data() + 6) << "\n";
    
    uint32_t selectorOffset = *reinterpret_cast<uint32_t*>(data.data() + 8);
    uint32_t propertyOffset = *reinterpret_cast<uint32_t*>(data.data() + 12);
    uint32_t variableOffset = *reinterpret_cast<uint32_t*>(data.data() + 16);
    
    oss << "  Selector Table Offset: " << selectorOffset << "\n";
    oss << "  Property Table Offset: " << propertyOffset << "\n";
    oss << "  Variable Table Offset: " << variableOffset << "\n";
    
    return oss.str();
}

} // namespace UltraWeb
