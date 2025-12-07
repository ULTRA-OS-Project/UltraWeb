// UltraWeb/runtime/UCSLoader.cpp
// UCS Binary Style Format Loader Implementation
// Version: 1.1.0 - Fixed to match CSS compiler output format

#include "UCSLoader.h"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// STYLE RULE IMPLEMENTATION
// ============================================================================

bool StyleRule::HasProperty(CSSPropertyId id) const {
    for (const auto& prop : properties) {
        if (prop.id == id) return true;
    }
    return false;
}

const StyleValue* StyleRule::GetProperty(CSSPropertyId id) const {
    for (const auto& prop : properties) {
        if (prop.id == id) return &prop.value;
    }
    return nullptr;
}

// ============================================================================
// UCS LOADER IMPLEMENTATION
// ============================================================================

UCSLoader::UCSLoader() : isLoaded(false) {
    header = UCSHeader();
}

UCSLoader::~UCSLoader() {
    Clear();
}

UCSLoadResult UCSLoader::Load(const std::vector<uint8_t>& data) {
    return Load(data.data(), data.size());
}

UCSLoadResult UCSLoader::Load(const uint8_t* data, size_t size) {
    UCSLoadResult result;
    Clear();
    
    if (size < sizeof(UCSHeader)) {
        result.error = "Data too small for UCS header";
        return result;
    }
    
    if (!ParseHeader(data, size)) {
        result.error = "Invalid UCS header";
        return result;
    }
    
    result.version = header.version;
    result.ruleCount = header.ruleCount;
    
    // Parse selector table first to get selector count
    if (!ParseSelectorTable(data, size)) {
        result.error = "Failed to parse selector table";
        return result;
    }
    
    // Now parse rules and discover string table location
    if (!ParseRulesAndStringTable(data, size)) {
        result.error = "Failed to parse rules";
        return result;
    }
    
    BuildIndices();
    
    isLoaded = true;
    result.success = true;
    return result;
}

bool UCSLoader::ParseHeader(const uint8_t* data, size_t size) {
    if (size < sizeof(UCSHeader)) return false;
    std::memcpy(&header, data, sizeof(UCSHeader));
    return (header.magic == UCS_MAGIC);
}

size_t UCSLoader::GetValueSize(CSSValueType type, const uint8_t* ptr, const uint8_t* end) {
    switch (type) {
        case CSSValueType::Color:
            return 4;  // RGBA
        case CSSValueType::Length:
            return 3;  // int16 value + uint8 unit
        case CSSValueType::Percentage:
        case CSSValueType::Float:
            return 4;  // float32
        case CSSValueType::Integer:
            return 4;  // int32
        case CSSValueType::Enum:
            return 1;  // uint8
        case CSSValueType::StringRef:
        case CSSValueType::VarRef:
            return 2;  // uint16 string ID
        case CSSValueType::MultiLength:
            if (ptr < end) {
                uint8_t count = *ptr;
                return 1 + count * 3;  // count + (int16 value + uint8 unit) per length
            }
            return 1;
        case CSSValueType::Shadow:
            return 14;  // offsetX(2) + offsetY(2) + blur(2) + spread(2) + color(4) + inset(1) + reserved(1)
        case CSSValueType::Auto:
        case CSSValueType::None:
        case CSSValueType::Inherit:
            return 0;
        default:
            return 0;
    }
}

bool UCSLoader::ParseSelectorTable(const uint8_t* data, size_t size) {
    if (header.selectorTableOffset == 0 || header.selectorTableOffset >= size) {
        return true;  // No selector table
    }
    
    const uint8_t* ptr = data + header.selectorTableOffset;
    const uint8_t* end = data + size;
    
    // Don't read past rule table
    if (header.propertyTableOffset > 0 && header.propertyTableOffset < size) {
        end = data + header.propertyTableOffset;
    }
    
    if (ptr + 2 > end) return false;
    uint16_t selectorCount = *reinterpret_cast<const uint16_t*>(ptr);
    ptr += 2;
    
    selectors.reserve(selectorCount);
    
    for (uint16_t i = 0; i < selectorCount && ptr + 10 <= end; i++) {
        SelectorEntry entry;
        entry.hash = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += 4;
        entry.stringOffset = *reinterpret_cast<const uint16_t*>(ptr);
        ptr += 2;
        entry.stringLength = *reinterpret_cast<const uint16_t*>(ptr);
        ptr += 2;
        entry.pseudoClass = static_cast<CSSPseudoClass>(*ptr++);
        ptr++;  // reserved
        
        selectors.push_back(entry);
    }
    
    return true;
}

bool UCSLoader::ParseRulesAndStringTable(const uint8_t* data, size_t size) {
    if (header.propertyTableOffset == 0 || header.propertyTableOffset >= size) {
        return true;  // No rules
    }
    
    const uint8_t* ptr = data + header.propertyTableOffset;
    const uint8_t* end = data + size;
    
    // Store rule data positions for second pass after loading string table
    std::vector<std::tuple<uint16_t, uint16_t, uint16_t, const uint8_t*>> rulePositions;
    
    // First pass: skip through rules to find string table
    for (size_t i = 0; i < selectors.size() && ptr + 6 <= end; i++) {
        uint16_t selectorId = *reinterpret_cast<const uint16_t*>(ptr);
        ptr += 2;
        uint16_t specificity = *reinterpret_cast<const uint16_t*>(ptr);
        ptr += 2;
        uint16_t propCount = *reinterpret_cast<const uint16_t*>(ptr);
        ptr += 2;
        
        const uint8_t* propStart = ptr;
        
        // Skip properties
        for (uint16_t p = 0; p < propCount && ptr + 2 <= end; p++) {
            ptr++;  // propertyId
            CSSValueType valueType = static_cast<CSSValueType>(*ptr++);
            size_t valueSize = GetValueSize(valueType, ptr, end);
            if (ptr + valueSize > end) return false;
            ptr += valueSize;
        }
        
        rulePositions.emplace_back(selectorId, specificity, propCount, propStart);
    }
    
    // Now ptr should be at the string table
    if (ptr + 2 <= end) {
        uint16_t stringCount = *reinterpret_cast<const uint16_t*>(ptr);
        ptr += 2;
        
        stringTable.reserve(stringCount);
        
        for (uint16_t i = 0; i < stringCount && ptr + 2 <= end; i++) {
            uint16_t len = *reinterpret_cast<const uint16_t*>(ptr);
            ptr += 2;
            if (ptr + len > end) break;
            stringTable.emplace_back(reinterpret_cast<const char*>(ptr), len);
            ptr += len;
        }
    }
    
    // Update selector names from string table
    for (auto& sel : selectors) {
        if (sel.stringOffset < stringTable.size()) {
            sel.name = stringTable[sel.stringOffset];
        }
        
        // Determine selector type from name
        if (!sel.name.empty()) {
            if (sel.name[0] == '.') {
                sel.type = CSSSelectorType::Class;
                sel.name = sel.name.substr(1);  // Remove dot
            } else if (sel.name[0] == '#') {
                sel.type = CSSSelectorType::Id;
                sel.name = sel.name.substr(1);  // Remove hash
            } else if (sel.name[0] == ':') {
                sel.type = CSSSelectorType::PseudoClass;
            } else {
                sel.type = CSSSelectorType::Element;
            }
        }
    }
    
    // Second pass: actually parse rules
    for (const auto& [selectorId, specificity, propCount, propPtr] : rulePositions) {
        StyleRule rule;
        
        // Get selector info
        if (selectorId < selectors.size()) {
            rule.selector.type = selectors[selectorId].type;
            rule.selector.hash = selectors[selectorId].hash;
            rule.selector.name = selectors[selectorId].name;
            rule.selector.specificity = specificity;
        }
        
        // Parse properties
        const uint8_t* ptr2 = propPtr;
        rule.properties.reserve(propCount);
        
        for (uint16_t p = 0; p < propCount && ptr2 + 2 <= end; p++) {
            StyleProperty prop;
            prop.id = static_cast<CSSPropertyId>(*ptr2++);
            CSSValueType valueType = static_cast<CSSValueType>(*ptr2++);
            
            prop.value = ParseValue(valueType, ptr2, end);
            size_t valueSize = GetValueSize(valueType, ptr2, end);
            ptr2 += valueSize;
            
            rule.properties.push_back(std::move(prop));
        }
        
        rules.push_back(std::move(rule));
    }
    
    return true;
}

StyleValue UCSLoader::ParseValue(CSSValueType type, const uint8_t* data, const uint8_t* end) {
    switch (type) {
        case CSSValueType::Color:
            if (data + 4 <= end) {
                return StyleColor(data[0], data[1], data[2], data[3]);
            }
            break;
            
        case CSSValueType::Length:
            if (data + 3 <= end) {
                int16_t value = *reinterpret_cast<const int16_t*>(data);
                uint8_t unit = data[2];
                return StyleDimension(static_cast<float>(value), unit);
            }
            break;
            
        case CSSValueType::Percentage:
        case CSSValueType::Float:
            if (data + 4 <= end) {
                float value;
                std::memcpy(&value, data, sizeof(float));
                return value;
            }
            break;
            
        case CSSValueType::Integer:
            if (data + 4 <= end) {
                return *reinterpret_cast<const int32_t*>(data);
            }
            break;
            
        case CSSValueType::Enum:
            if (data + 1 <= end) {
                return static_cast<int32_t>(data[0]);
            }
            break;
            
        case CSSValueType::StringRef:
        case CSSValueType::VarRef:
            if (data + 2 <= end) {
                uint16_t ref = *reinterpret_cast<const uint16_t*>(data);
                if (ref < stringTable.size()) {
                    return stringTable[ref];
                }
            }
            break;
            
        case CSSValueType::MultiLength:
            if (data + 1 <= end) {
                uint8_t count = *data++;
                // For now, just return the first value if present
                if (count > 0 && data + 3 <= end) {
                    int16_t value = *reinterpret_cast<const int16_t*>(data);
                    uint8_t unit = data[2];
                    return StyleDimension(static_cast<float>(value), unit);
                }
            }
            break;
            
        default:
            break;
    }
    
    return std::monostate{};
}

void UCSLoader::BuildIndices() {
    classToRules.clear();
    idToRules.clear();
    tagToRules.clear();
    
    for (size_t i = 0; i < rules.size(); i++) {
        const auto& rule = rules[i];
        switch (rule.selector.type) {
            case CSSSelectorType::Class:
                classToRules[rule.selector.name].push_back(i);
                break;
            case CSSSelectorType::Id:
                idToRules[rule.selector.name].push_back(i);
                break;
            case CSSSelectorType::Element:
                tagToRules[rule.selector.name].push_back(i);
                break;
            default:
                break;
        }
    }
}

std::vector<const StyleRule*> UCSLoader::FindRulesByClass(const std::string& className) const {
    std::vector<const StyleRule*> result;
    auto it = classToRules.find(className);
    if (it != classToRules.end()) {
        for (size_t idx : it->second) {
            result.push_back(&rules[idx]);
        }
    }
    return result;
}

std::vector<const StyleRule*> UCSLoader::FindRulesById(const std::string& id) const {
    std::vector<const StyleRule*> result;
    auto it = idToRules.find(id);
    if (it != idToRules.end()) {
        for (size_t idx : it->second) {
            result.push_back(&rules[idx]);
        }
    }
    return result;
}

std::vector<const StyleRule*> UCSLoader::FindRulesByTag(const std::string& tag) const {
    std::vector<const StyleRule*> result;
    auto it = tagToRules.find(tag);
    if (it != tagToRules.end()) {
        for (size_t idx : it->second) {
            result.push_back(&rules[idx]);
        }
    }
    return result;
}

const std::string& UCSLoader::GetString(uint16_t index) const {
    static const std::string empty;
    return (index < stringTable.size()) ? stringTable[index] : empty;
}

std::string UCSLoader::DumpRules() const {
    std::ostringstream oss;
    oss << "=== UCS Style Rules ===\n";
    oss << "Version: 0x" << std::hex << header.version << std::dec << "\n";
    oss << "Rules: " << rules.size() << "\n";
    oss << "Selectors: " << selectors.size() << "\n";
    oss << "Strings: " << stringTable.size() << "\n\n";
    
    for (const auto& rule : rules) {
        switch (rule.selector.type) {
            case CSSSelectorType::Class: oss << "."; break;
            case CSSSelectorType::Id: oss << "#"; break;
            case CSSSelectorType::PseudoClass: oss << ":"; break;
            case CSSSelectorType::PseudoElement: oss << "::"; break;
            default: break;
        }
        oss << rule.selector.name << " {\n";
        
        for (const auto& prop : rule.properties) {
            oss << "  " << GetCSSPropertyName(prop.id) << ": ";
            if (std::holds_alternative<StyleColor>(prop.value)) {
                const auto& c = std::get<StyleColor>(prop.value);
                oss << "rgba(" << (int)c.r << "," << (int)c.g << "," << (int)c.b << "," << (int)c.a << ")";
            } else if (std::holds_alternative<StyleDimension>(prop.value)) {
                oss << std::get<StyleDimension>(prop.value).value << "px";
            } else if (std::holds_alternative<float>(prop.value)) {
                oss << std::get<float>(prop.value);
            } else if (std::holds_alternative<int32_t>(prop.value)) {
                oss << std::get<int32_t>(prop.value);
            } else if (std::holds_alternative<std::string>(prop.value)) {
                oss << "\"" << std::get<std::string>(prop.value) << "\"";
            }
            oss << ";\n";
        }
        oss << "}\n\n";
    }
    
    return oss.str();
}

void UCSLoader::Clear() {
    rules.clear();
    selectors.clear();
    stringTable.clear();
    classToRules.clear();
    idToRules.clear();
    tagToRules.clear();
    header = UCSHeader();
    isLoaded = false;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char* GetCSSPropertyName(CSSPropertyId id) {
    switch (id) {
        case CSSPropertyId::Display: return "display";
        case CSSPropertyId::Position: return "position";
        case CSSPropertyId::FlexDirection: return "flex-direction";
        case CSSPropertyId::JustifyContent: return "justify-content";
        case CSSPropertyId::AlignItems: return "align-items";
        case CSSPropertyId::FlexWrap: return "flex-wrap";
        case CSSPropertyId::Gap: return "gap";
        case CSSPropertyId::Width: return "width";
        case CSSPropertyId::Height: return "height";
        case CSSPropertyId::Padding: return "padding";
        case CSSPropertyId::PaddingTop: return "padding-top";
        case CSSPropertyId::PaddingRight: return "padding-right";
        case CSSPropertyId::PaddingBottom: return "padding-bottom";
        case CSSPropertyId::PaddingLeft: return "padding-left";
        case CSSPropertyId::Margin: return "margin";
        case CSSPropertyId::MarginTop: return "margin-top";
        case CSSPropertyId::MarginRight: return "margin-right";
        case CSSPropertyId::MarginBottom: return "margin-bottom";
        case CSSPropertyId::MarginLeft: return "margin-left";
        case CSSPropertyId::Color: return "color";
        case CSSPropertyId::BackgroundColor: return "background-color";
        case CSSPropertyId::BorderWidth: return "border-width";
        case CSSPropertyId::BorderColor: return "border-color";
        case CSSPropertyId::BorderRadius: return "border-radius";
        case CSSPropertyId::FontFamily: return "font-family";
        case CSSPropertyId::FontSize: return "font-size";
        case CSSPropertyId::FontWeight: return "font-weight";
        case CSSPropertyId::Opacity: return "opacity";
        default: return "unknown";
    }
}

const char* GetSelectorTypeName(CSSSelectorType type) {
    switch (type) {
        case CSSSelectorType::Element: return "element";
        case CSSSelectorType::Class: return "class";
        case CSSSelectorType::Id: return "id";
        case CSSSelectorType::Universal: return "universal";
        case CSSSelectorType::Attribute: return "attribute";
        case CSSSelectorType::PseudoClass: return "pseudo-class";
        case CSSSelectorType::PseudoElement: return "pseudo-element";
        case CSSSelectorType::Combinator: return "combinator";
        default: return "unknown";
    }
}

} // namespace Runtime
} // namespace UltraWeb
