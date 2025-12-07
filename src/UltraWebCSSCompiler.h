// UltraWeb/include/UltraWebCSSCompiler.h
// CSS to UCS Binary Compiler - Converts parsed CSS AST to binary format
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework
#pragma once

#include "UltraWebFormats.h"
#include "UltraWebCSSParser.h"
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

namespace UltraWeb {

// ============================================================================
// COMPILATION RESULT
// ============================================================================

struct CompilationResult {
    bool success;
    std::vector<uint8_t> data;      // Binary output
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    // Statistics
    size_t ruleCount;
    size_t variableCount;
    size_t selectorCount;
    size_t propertyCount;
    size_t originalSize;            // Original CSS size
    size_t compiledSize;            // Compiled binary size
    float compressionRatio;         // originalSize / compiledSize
    
    CompilationResult()
        : success(false)
        , ruleCount(0)
        , variableCount(0)
        , selectorCount(0)
        , propertyCount(0)
        , originalSize(0)
        , compiledSize(0)
        , compressionRatio(0.0f) {}
    
    void AddError(const std::string& error) {
        errors.push_back(error);
        success = false;
    }
    
    void AddWarning(const std::string& warning) {
        warnings.push_back(warning);
    }
};

// ============================================================================
// BINARY WRITER HELPER
// ============================================================================

class BinaryWriter {
public:
    BinaryWriter();
    
    // Write primitives
    void WriteUInt8(uint8_t value);
    void WriteUInt16(uint16_t value);
    void WriteUInt32(uint32_t value);
    void WriteInt16(int16_t value);
    void WriteInt32(int32_t value);
    void WriteFloat(float value);
    void WriteBytes(const uint8_t* data, size_t length);
    void WriteString(const std::string& str);
    
    // Write UCS types
    void WriteLength(const UCSLength& length);
    void WriteColor(const UCSColor& color);
    void WriteBoxShadow(const UCSBoxShadow& shadow);
    void WriteGradientStop(const UCSGradientStop& stop);
    
    // Position management
    size_t GetPosition() const { return buffer.size(); }
    void SetPosition(size_t pos);
    void PatchUInt32(size_t position, uint32_t value);
    void PatchUInt16(size_t position, uint16_t value);
    
    // Get result
    const std::vector<uint8_t>& GetData() const { return buffer; }
    std::vector<uint8_t> TakeData() { return std::move(buffer); }
    void Clear() { buffer.clear(); }
    
private:
    std::vector<uint8_t> buffer;
};

// ============================================================================
// STRING TABLE
// ============================================================================

class StringTable {
public:
    StringTable();
    
    // Add string and get its ID
    uint16_t AddString(const std::string& str);
    
    // Get string by ID
    const std::string& GetString(uint16_t id) const;
    
    // Get ID of existing string (returns 0xFFFF if not found)
    uint16_t FindString(const std::string& str) const;
    
    // Write table to binary
    void WriteTo(BinaryWriter& writer) const;
    
    // Get total size in bytes
    size_t GetByteSize() const;
    
    // Get string count
    size_t GetCount() const { return strings.size(); }
    
private:
    std::vector<std::string> strings;
    std::unordered_map<std::string, uint16_t> stringToId;
};

// ============================================================================
// CSS COMPILER
// ============================================================================

class CSSCompiler {
public:
    CSSCompiler();
    
    // Compile CSS string to UCS binary
    CompilationResult Compile(const std::string& css);
    
    // Compile parsed stylesheet to UCS binary
    CompilationResult Compile(const CSSStylesheet& stylesheet);
    
    // Compile and write to file
    bool CompileToFile(const std::string& css, const std::string& outputPath);
    bool CompileToFile(const CSSStylesheet& stylesheet, const std::string& outputPath);
    
    // Configuration
    void SetOptimizeOutput(bool optimize) { optimizeOutput = optimize; }
    void SetIncludeSourceMap(bool include) { includeSourceMap = include; }
    
private:
    bool optimizeOutput;
    bool includeSourceMap;
    StringTable stringTable;
    BinaryWriter writer;
    
    // Compilation stages
    void Reset();
    void CompileVariables(const std::vector<CSSVariable>& variables);
    void CompileRules(const std::vector<CSSRule>& rules);
    void CompileRule(const CSSRule& rule);
    void CompileSelector(const CSSSelector& selector);
    void CompileDeclaration(const CSSDeclaration& decl);
    void CompileDeclarationToWriter(const CSSDeclaration& decl, BinaryWriter& ruleWriter);
    void CompileValue(const CSSValue& value, CSSPropertyId propertyId);
    
    // Value compilation
    void CompileColorValue(const CSSColorValue& color);
    void CompileLengthValue(const CSSLengthValue& length);
    void CompileMultiLengthValue(const std::vector<CSSLengthValue>& lengths);
    void CompileShadowValue(const CSSBoxShadowValue& shadow);
    void CompileGradientValue(const CSSGradientValue& gradient);
    void CompileEnumValue(const std::string& value, CSSPropertyId propertyId);
    
    // Enum value resolution
    uint8_t ResolveEnumValue(const std::string& value, CSSPropertyId propertyId);
    uint8_t ResolveDisplayValue(const std::string& value);
    uint8_t ResolvePositionValue(const std::string& value);
    uint8_t ResolveFlexDirectionValue(const std::string& value);
    uint8_t ResolveJustifyContentValue(const std::string& value);
    uint8_t ResolveAlignItemsValue(const std::string& value);
    uint8_t ResolveFlexWrapValue(const std::string& value);
    uint8_t ResolveTextAlignValue(const std::string& value);
    uint8_t ResolveFontStyleValue(const std::string& value);
    uint8_t ResolveTextDecorationValue(const std::string& value);
    uint8_t ResolveVisibilityValue(const std::string& value);
    uint8_t ResolveOverflowValue(const std::string& value);
    uint8_t ResolveCursorValue(const std::string& value);
    uint8_t ResolveBorderStyleValue(const std::string& value);
    uint16_t ResolveFontWeightValue(const std::string& value);
    
    // Section builders
    std::vector<uint8_t> BuildSelectorTable(const std::vector<CSSRule>& rules);
    std::vector<uint8_t> BuildVariableTable(const std::vector<CSSVariable>& variables);
    std::vector<uint8_t> BuildRuleTable(const std::vector<CSSRule>& rules);
    std::vector<uint8_t> BuildStringTable();
    
    // Finalization
    void WriteHeader(size_t ruleCount, 
                     size_t selectorTableOffset, 
                     size_t propertyTableOffset,
                     size_t variableTableOffset);
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Load UCS file and get basic info
struct UCSFileInfo {
    bool valid;
    uint16_t version;
    uint16_t ruleCount;
    size_t fileSize;
    std::string error;
};

UCSFileInfo GetUCSFileInfo(const std::string& filePath);

// Validate UCS file
bool ValidateUCSFile(const std::string& filePath, std::string& error);

// Dump UCS file to human-readable format (for debugging)
std::string DumpUCSFile(const std::string& filePath);

} // namespace UltraWeb
