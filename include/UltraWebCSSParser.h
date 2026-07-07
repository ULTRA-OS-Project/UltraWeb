// UltraWeb/include/UltraWebCSSParser.h
// CSS Tokenizer and Parser - Converts CSS text to Abstract Syntax Tree
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework
#pragma once

#include "UltraWebFormats.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <variant>

namespace UltraWeb {

// ============================================================================
// CSS TOKEN TYPES
// ============================================================================

enum class CSSTokenType {
    // Basic tokens
    Identifier,     // property-name, class-name, etc.
    String,         // "quoted string" or 'quoted string'
    Number,         // 123, 45.67
    Percentage,     // 50%
    Dimension,      // 10px, 2em, 100vh
    Hash,           // #fff, #id
    AtKeyword,      // @media, @keyframes
    Function,       // rgb(, var(, calc(
    Url,            // url(...)
    
    // Delimiters
    Colon,          // :
    Semicolon,      // ;
    Comma,          // ,
    OpenBrace,      // {
    CloseBrace,     // }
    OpenParen,      // (
    CloseParen,     // )
    OpenBracket,    // [
    CloseBracket,   // ]
    
    // Combinators
    Whitespace,
    Delim,          // Other single characters
    
    // Special
    Comment,        // /* ... */
    EndOfFile,
    Invalid
};

// ============================================================================
// CSS TOKEN
// ============================================================================

struct CSSToken {
    CSSTokenType type;
    std::string value;
    double numericValue;
    std::string unit;           // For dimensions (px, em, etc.)
    size_t line;
    size_t column;
    
    CSSToken()
        : type(CSSTokenType::Invalid)
        , numericValue(0.0)
        , line(0)
        , column(0) {}
    
    CSSToken(CSSTokenType t, const std::string& v, size_t l, size_t c)
        : type(t), value(v), numericValue(0.0), line(l), column(c) {}
        
    bool IsIdentifier() const { return type == CSSTokenType::Identifier; }
    bool IsNumber() const { return type == CSSTokenType::Number; }
    bool IsDimension() const { return type == CSSTokenType::Dimension; }
    bool IsPercentage() const { return type == CSSTokenType::Percentage; }
    bool IsString() const { return type == CSSTokenType::String; }
    bool IsHash() const { return type == CSSTokenType::Hash; }
    bool IsFunction() const { return type == CSSTokenType::Function; }
};

// ============================================================================
// CSS TOKENIZER
// ============================================================================

class CSSTokenizer {
public:
    explicit CSSTokenizer(const std::string& input);
    
    // Tokenize entire input
    std::vector<CSSToken> Tokenize();
    
    // Get next token (for streaming)
    CSSToken NextToken();
    
    // Peek at next token without consuming
    CSSToken PeekToken();
    
    // Check if at end
    bool IsAtEnd() const;
    
    // Get current position info
    size_t GetLine() const { return line; }
    size_t GetColumn() const { return column; }
    
private:
    std::string input;
    size_t position;
    size_t line;
    size_t column;
    
    // Character helpers
    char Current() const;
    char Peek(size_t offset = 1) const;
    char Advance();
    void SkipWhitespace();
    void SkipComment();
    
    // Token parsers
    CSSToken ReadIdentifier();
    CSSToken ReadNumber();
    CSSToken ReadString(char quote);
    CSSToken ReadHash();
    CSSToken ReadAtKeyword();
    CSSToken ReadUrl();
    
    // Character classification
    bool IsDigit(char c) const;
    bool IsHexDigit(char c) const;
    bool IsLetter(char c) const;
    bool IsIdentStart(char c) const;
    bool IsIdentChar(char c) const;
    bool IsWhitespace(char c) const;
};

// ============================================================================
// CSS AST - VALUE NODES
// ============================================================================

// Forward declarations
struct CSSValue;
struct CSSDeclaration;
struct CSSRule;
struct CSSStylesheet;

// Color value (parsed)
struct CSSColorValue {
    uint8_t r, g, b, a;
    
    CSSColorValue() : r(0), g(0), b(0), a(255) {}
    CSSColorValue(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
};

// Length value (parsed)
struct CSSLengthValue {
    double value;
    CSSLengthUnit unit;
    
    CSSLengthValue() : value(0), unit(CSSLengthUnit::Px) {}
    CSSLengthValue(double v, CSSLengthUnit u) : value(v), unit(u) {}
};

// Gradient stop
struct CSSGradientStopValue {
    CSSColorValue color;
    std::optional<double> position;  // 0.0 - 1.0, nullopt for auto
};

// Gradient value
struct CSSGradientValue {
    CSSGradientType type;
    double angle;                   // For linear gradients (degrees)
    std::vector<CSSGradientStopValue> stops;
    
    CSSGradientValue() : type(CSSGradientType::Linear), angle(180.0) {}
};

// Box shadow value
struct CSSBoxShadowValue {
    double offsetX;
    double offsetY;
    double blurRadius;
    double spreadRadius;
    CSSColorValue color;
    bool inset;
    
    CSSBoxShadowValue()
        : offsetX(0), offsetY(0), blurRadius(0), spreadRadius(0)
        , color(), inset(false) {}
};

// CSS Value (variant type)
using CSSValueData = std::variant<
    std::monostate,             // None/Auto/Inherit
    std::string,                // Identifier, string, function name
    double,                     // Number, percentage
    CSSLengthValue,             // Length with unit
    CSSColorValue,              // Color
    CSSGradientValue,           // Gradient
    CSSBoxShadowValue,          // Box shadow
    std::vector<CSSLengthValue> // Multiple lengths (margin, padding)
>;

struct CSSValue {
    CSSValueType type;
    CSSValueData data;
    std::string raw;            // Original string representation
    
    CSSValue() : type(CSSValueType::Invalid) {}
    
    bool IsValid() const { return type != CSSValueType::Invalid; }
    bool IsAuto() const { return type == CSSValueType::Auto; }
    bool IsNone() const { return type == CSSValueType::None; }
    bool IsInherit() const { return type == CSSValueType::Inherit; }
    
    // Type-safe getters
    const std::string* AsString() const { 
        return std::get_if<std::string>(&data); 
    }
    const double* AsNumber() const { 
        return std::get_if<double>(&data); 
    }
    const CSSLengthValue* AsLength() const { 
        return std::get_if<CSSLengthValue>(&data); 
    }
    const CSSColorValue* AsColor() const { 
        return std::get_if<CSSColorValue>(&data); 
    }
    const CSSGradientValue* AsGradient() const { 
        return std::get_if<CSSGradientValue>(&data); 
    }
    const CSSBoxShadowValue* AsShadow() const { 
        return std::get_if<CSSBoxShadowValue>(&data); 
    }
    const std::vector<CSSLengthValue>* AsMultiLength() const { 
        return std::get_if<std::vector<CSSLengthValue>>(&data); 
    }
};

// ============================================================================
// CSS AST - DECLARATION & RULE
// ============================================================================

// CSS Declaration (property: value)
struct CSSDeclaration {
    std::string property;       // Property name (e.g., "background-color")
    CSSPropertyId propertyId;   // Resolved property ID
    CSSValue value;             // Parsed value
    bool important;             // !important flag
    size_t line;                // Source line for error reporting
    
    CSSDeclaration() 
        : propertyId(CSSPropertyId::Invalid)
        , important(false)
        , line(0) {}
};

// CSS Selector
struct CSSSelector {
    std::string text;           // Full selector text
    uint32_t hash;              // Pre-computed hash
    uint16_t specificity;       // Pre-computed specificity
    CSSPseudoClass pseudoClass; // Pseudo-class flags
    
    CSSSelector() 
        : hash(0)
        , specificity(0)
        , pseudoClass(CSSPseudoClass::None) {}
};

// CSS Rule (selector { declarations })
struct CSSRule {
    std::vector<CSSSelector> selectors;  // Multiple selectors (comma-separated)
    std::vector<CSSDeclaration> declarations;
    size_t line;                // Source line
    
    CSSRule() : line(0) {}
};

// CSS Variable
struct CSSVariable {
    std::string name;           // Variable name without --
    CSSValue value;             // Parsed value
    size_t line;
    
    CSSVariable() : line(0) {}
};

// ============================================================================
// CSS AST - STYLESHEET
// ============================================================================

struct CSSStylesheet {
    std::vector<CSSRule> rules;
    std::vector<CSSVariable> variables;  // :root variables
    std::vector<std::string> errors;     // Parse errors
    
    bool HasErrors() const { return !errors.empty(); }
    size_t GetRuleCount() const { return rules.size(); }
    size_t GetVariableCount() const { return variables.size(); }
};

// ============================================================================
// CSS PARSER
// ============================================================================

class CSSParser {
public:
    CSSParser();
    
    // Parse CSS string to stylesheet AST
    CSSStylesheet Parse(const std::string& css);
    
    // Parse a single declaration
    std::optional<CSSDeclaration> ParseDeclaration(const std::string& declarationStr);
    
    // Parse a single value
    CSSValue ParseValue(const std::string& valueStr, CSSPropertyId propertyId);
    
    // Get last error
    const std::string& GetLastError() const { return lastError; }
    
private:
    std::vector<CSSToken> tokens;
    size_t position;
    std::string lastError;
    
    // Token helpers
    const CSSToken& Current() const;
    const CSSToken& Peek(size_t offset = 1) const;
    bool Check(CSSTokenType type) const;
    bool Match(CSSTokenType type);
    void Advance();
    bool IsAtEnd() const;
    void SkipWhitespace();
    
    // Error handling
    void Error(const std::string& message);
    void Synchronize();
    
    // Parsing methods
    void ParseStylesheet(CSSStylesheet& stylesheet);
    std::optional<CSSRule> ParseRule();
    std::vector<CSSSelector> ParseSelectors();
    CSSSelector ParseSelector();
    std::vector<CSSDeclaration> ParseDeclarationBlock();
    std::optional<CSSDeclaration> ParseDeclarationInternal();
    
    // Value parsing
    CSSValue ParseValueInternal(CSSPropertyId propertyId);
    CSSValue ParseColorValue();
    CSSValue ParseLengthValue();
    CSSValue ParseGradientValue();
    CSSValue ParseShadowValue();
    CSSValue ParseMultiLengthValue(int maxValues);
    
    // Property resolution
    CSSPropertyId ResolvePropertyId(const std::string& name);
    
    // Color parsing helpers
    CSSColorValue ParseHexColor(const std::string& hex);
    CSSColorValue ParseRgbFunction();
    CSSColorValue ParseRgbaFunction();
    std::optional<CSSColorValue> ParseNamedColor(const std::string& name);
    
    // Length parsing helper
    CSSLengthUnit ParseLengthUnit(const std::string& unit);
    
    // Specificity calculation
    uint16_t CalculateSelectorSpecificity(const std::string& selector);
    
    // Variable parsing
    void ParseRootVariables(CSSStylesheet& stylesheet, const std::vector<CSSDeclaration>& declarations);
};

// ============================================================================
// PROPERTY NAME MAPPING
// ============================================================================

// Get property ID from name
CSSPropertyId GetPropertyId(const std::string& name);

// Get property name from ID
const char* GetPropertyName(CSSPropertyId id);

// Check if property is a shorthand
bool IsShorthandProperty(CSSPropertyId id);

// ============================================================================
// COLOR NAME MAPPING
// ============================================================================

// Get color from CSS named color
std::optional<CSSColorValue> GetNamedColor(const std::string& name);

} // namespace UltraWeb
