// UltraWeb/include/UltraWebUIParser.h
// UCML (UltraCanvas Markup Language) Parser - Converts UI markup to AST
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework
#pragma once

#include "UltraWebFormats.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>
#include <variant>

namespace UltraWeb {

// ============================================================================
// UCML TOKEN TYPES
// ============================================================================

enum class UCMLTokenType {
    // Tags
    TagOpen,            // <
    TagClose,           // >
    TagSelfClose,       // />
    TagEnd,             // </
    
    // Content
    Identifier,         // tag names, attribute names
    String,             // "value" or 'value'
    Text,               // Text content between tags
    
    // Operators
    Equals,             // =
    
    // Special
    Comment,            // <!-- ... -->
    Whitespace,
    EndOfFile,
    Invalid
};

// ============================================================================
// UCML TOKEN
// ============================================================================

struct UCMLToken {
    UCMLTokenType type;
    std::string value;
    size_t line;
    size_t column;
    
    UCMLToken()
        : type(UCMLTokenType::Invalid)
        , line(0)
        , column(0) {}
    
    UCMLToken(UCMLTokenType t, const std::string& v, size_t l, size_t c)
        : type(t), value(v), line(l), column(c) {}
};

// ============================================================================
// UCML TOKENIZER
// ============================================================================

class UCMLTokenizer {
public:
    explicit UCMLTokenizer(const std::string& input);
    
    std::vector<UCMLToken> Tokenize();
    UCMLToken NextToken();
    bool IsAtEnd() const;
    
private:
    std::string input;
    size_t position;
    size_t line;
    size_t column;
    bool inTag;  // Track if we're inside a tag
    
    char Current() const;
    char Peek(size_t offset = 1) const;
    char Advance();
    void SkipWhitespace();
    
    UCMLToken ReadIdentifier();
    UCMLToken ReadString();
    UCMLToken ReadText();
    UCMLToken ReadComment();
    
    bool IsLetter(char c) const;
    bool IsDigit(char c) const;
    bool IsIdentStart(char c) const;
    bool IsIdentChar(char c) const;
    bool IsWhitespace(char c) const;
};

// ============================================================================
// UCML AST - ATTRIBUTE
// ============================================================================

using UCMLAttributeValue = std::variant<
    std::monostate,     // No value (boolean attribute)
    bool,               // Boolean value
    int32_t,            // Integer value
    double,             // Float value
    std::string         // String value
>;

struct UCMLAttribute {
    std::string name;
    UCMLAttributeValue value;
    UCBPropertyId propertyId;
    size_t line;
    
    UCMLAttribute() 
        : propertyId(UCBPropertyId::Custom)
        , line(0) {}
    
    bool HasValue() const { 
        return !std::holds_alternative<std::monostate>(value); 
    }
    
    bool IsString() const { 
        return std::holds_alternative<std::string>(value); 
    }
    
    const std::string* AsString() const {
        return std::get_if<std::string>(&value);
    }
    
    const bool* AsBool() const {
        return std::get_if<bool>(&value);
    }
    
    const int32_t* AsInt() const {
        return std::get_if<int32_t>(&value);
    }
    
    const double* AsFloat() const {
        return std::get_if<double>(&value);
    }
};

// ============================================================================
// UCML AST - ELEMENT
// ============================================================================

struct UCMLElement {
    std::string tagName;
    UCBElementType elementType;
    uint16_t elementId;
    uint16_t parentId;
    std::vector<UCMLAttribute> attributes;
    std::vector<std::shared_ptr<UCMLElement>> children;
    std::string textContent;
    bool selfClosing;
    size_t line;
    
    UCMLElement()
        : elementType(UCBElementType::Invalid)
        , elementId(0)
        , parentId(0)
        , selfClosing(false)
        , line(0) {}
    
    // Get attribute by name
    const UCMLAttribute* GetAttribute(const std::string& name) const {
        for (const auto& attr : attributes) {
            if (attr.name == name) {
                return &attr;
            }
        }
        return nullptr;
    }
    
    // Check if element has class
    bool HasClass(const std::string& className) const {
        const auto* classAttr = GetAttribute("class");
        if (!classAttr) return false;
        const auto* classes = classAttr->AsString();
        if (!classes) return false;
        return classes->find(className) != std::string::npos;
    }
    
    // Get all style classes
    std::vector<std::string> GetClasses() const;
};

// ============================================================================
// UCML DOCUMENT
// ============================================================================

struct UCMLDocument {
    std::shared_ptr<UCMLElement> root;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    bool HasErrors() const { return !errors.empty(); }
    
    // Count total elements
    size_t CountElements() const;
    
    // Find element by ID
    std::shared_ptr<UCMLElement> FindById(const std::string& id) const;
    
    // Find all elements by tag name
    std::vector<std::shared_ptr<UCMLElement>> FindByTagName(const std::string& tagName) const;
};

// ============================================================================
// UCML PARSER
// ============================================================================

class UCMLParser {
public:
    UCMLParser();
    
    // Parse UCML string to document
    UCMLDocument Parse(const std::string& ucml);
    
    // Parse single element (for testing)
    std::shared_ptr<UCMLElement> ParseElement(const std::string& ucml);
    
    // Get last error
    const std::string& GetLastError() const { return lastError; }
    
private:
    std::vector<UCMLToken> tokens;
    size_t position;
    std::string lastError;
    uint16_t nextElementId;
    
    // Token helpers
    const UCMLToken& Current() const;
    const UCMLToken& Peek(size_t offset = 1) const;
    bool Check(UCMLTokenType type) const;
    bool Match(UCMLTokenType type);
    void Advance();
    bool IsAtEnd() const;
    void SkipWhitespace();
    
    // Error handling
    void Error(const std::string& message);
    
    // Parsing methods
    std::shared_ptr<UCMLElement> ParseElementNode(uint16_t parentId);
    std::vector<UCMLAttribute> ParseAttributes();
    UCMLAttribute ParseAttribute();
    std::string ParseTextContent();
    
    // Element type resolution
    UCBElementType ResolveElementType(const std::string& tagName);
    UCBPropertyId ResolvePropertyId(const std::string& attrName);
    
    // ID assignment
    uint16_t AssignElementId();
};

// ============================================================================
// TAG NAME MAPPING
// ============================================================================

// Get element type from tag name
UCBElementType GetElementType(const std::string& tagName);

// Get tag name from element type
const char* GetTagName(UCBElementType type);

// Check if tag is self-closing by default
bool IsSelfClosingTag(const std::string& tagName);

// ============================================================================
// ATTRIBUTE NAME MAPPING
// ============================================================================

// Get property ID from attribute name
UCBPropertyId GetUCBPropertyId(const std::string& attrName);

// Get attribute name from property ID
const char* GetUCBAttributeName(UCBPropertyId id);

// Check if attribute is an event handler
bool IsEventHandler(const std::string& attrName);

// Check if attribute is a data binding
bool IsDataBinding(const std::string& attrName);

} // namespace UltraWeb
