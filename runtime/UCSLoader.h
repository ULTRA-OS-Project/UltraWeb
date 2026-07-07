// UltraWeb/runtime/UCSLoader.h
// UCS Binary Style Format Loader - Client-side decoder
// Version: 1.0.0
#pragma once

#include "../include/UltraWebFormats.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <algorithm>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// RUNTIME STYLE TYPES
// ============================================================================

struct StyleColor {
    uint8_t r, g, b, a;
    StyleColor() : r(0), g(0), b(0), a(255) {}
    StyleColor(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) 
        : r(r_), g(g_), b(b_), a(a_) {}
};

struct StyleDimension {
    float value;
    uint8_t unitCode;  // Maps to CSSUnit enum value
    
    StyleDimension() : value(0), unitCode(0) {}
    StyleDimension(float v, uint8_t u = 0) : value(v), unitCode(u) {}
    
    float ToPixels() const { return value; }  // Simplified
};

using StyleValue = std::variant<
    std::monostate,
    StyleColor,
    StyleDimension,
    float,
    int32_t,
    std::string
>;

struct StyleProperty {
    CSSPropertyId id;
    StyleValue value;
    StyleProperty() : id(CSSPropertyId::Display) {}
};

struct StyleSelector {
    UltraWeb::CSSSelectorType type;
    uint32_t hash;
    std::string name;
    uint16_t specificity;
    StyleSelector() : type(UltraWeb::CSSSelectorType::Class), hash(0), specificity(0) {}
};

struct StyleRule {
    StyleSelector selector;
    std::vector<StyleProperty> properties;
    
    bool HasProperty(CSSPropertyId id) const;
    const StyleValue* GetProperty(CSSPropertyId id) const;
};

// Internal selector entry from binary format
struct SelectorEntry {
    uint32_t hash;
    uint16_t stringOffset;
    uint16_t stringLength;
    CSSPseudoClass pseudoClass;
    CSSSelectorType type;
    std::string name;
    
    SelectorEntry() : hash(0), stringOffset(0), stringLength(0), 
                      pseudoClass(CSSPseudoClass::None), type(CSSSelectorType::Class) {}
};

// ============================================================================
// UCS LOADER
// ============================================================================

struct UCSLoadResult {
    bool success;
    std::string error;
    uint16_t version;
    uint32_t ruleCount;
    UCSLoadResult() : success(false), version(0), ruleCount(0) {}
};

class UCSLoader {
public:
    UCSLoader();
    ~UCSLoader();
    
    UCSLoadResult Load(const std::vector<uint8_t>& data);
    UCSLoadResult Load(const uint8_t* data, size_t size);
    
    const std::vector<StyleRule>& GetRules() const { return rules; }
    size_t GetRuleCount() const { return rules.size(); }
    
    std::vector<const StyleRule*> FindRulesByClass(const std::string& className) const;
    std::vector<const StyleRule*> FindRulesById(const std::string& id) const;
    std::vector<const StyleRule*> FindRulesByTag(const std::string& tag) const;
    
    const std::string& GetString(uint16_t index) const;
    std::string DumpRules() const;
    void Clear();
    
private:
    std::vector<StyleRule> rules;
    std::vector<SelectorEntry> selectors;
    std::vector<std::string> stringTable;
    std::unordered_map<std::string, std::vector<size_t>> classToRules;
    std::unordered_map<std::string, std::vector<size_t>> idToRules;
    std::unordered_map<std::string, std::vector<size_t>> tagToRules;
    
    UCSHeader header;
    bool isLoaded;
    
    bool ParseHeader(const uint8_t* data, size_t size);
    bool ParseSelectorTable(const uint8_t* data, size_t size);
    bool ParseRulesAndStringTable(const uint8_t* data, size_t size);
    StyleValue ParseValue(CSSValueType type, const uint8_t* data, const uint8_t* end);
    size_t GetValueSize(CSSValueType type, const uint8_t* ptr, const uint8_t* end);
    void BuildIndices();
};

const char* GetCSSPropertyName(UltraWeb::CSSPropertyId id);
const char* GetSelectorTypeName(UltraWeb::CSSSelectorType type);

} // namespace Runtime
} // namespace UltraWeb
