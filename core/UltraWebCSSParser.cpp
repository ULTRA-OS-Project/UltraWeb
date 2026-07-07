// UltraWeb/core/UltraWebCSSParser.cpp
// CSS Tokenizer and Parser Implementation
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "../include/UltraWebCSSParser.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

namespace UltraWeb {

// ============================================================================
// CSS TOKENIZER IMPLEMENTATION
// ============================================================================

CSSTokenizer::CSSTokenizer(const std::string& input)
    : input(input)
    , position(0)
    , line(1)
    , column(1)
{
}

std::vector<CSSToken> CSSTokenizer::Tokenize() {
    std::vector<CSSToken> tokens;
    
    while (!IsAtEnd()) {
        CSSToken token = NextToken();
        if (token.type != CSSTokenType::Comment && 
            token.type != CSSTokenType::Whitespace) {
            tokens.push_back(token);
        }
        if (token.type == CSSTokenType::EndOfFile) {
            break;
        }
    }
    
    return tokens;
}

CSSToken CSSTokenizer::NextToken() {
    if (IsAtEnd()) {
        return CSSToken(CSSTokenType::EndOfFile, "", line, column);
    }
    
    char c = Current();
    
    // Whitespace
    if (IsWhitespace(c)) {
        size_t startLine = line;
        size_t startCol = column;
        while (!IsAtEnd() && IsWhitespace(Current())) {
            Advance();
        }
        return CSSToken(CSSTokenType::Whitespace, " ", startLine, startCol);
    }
    
    // Comments
    if (c == '/' && Peek() == '*') {
        size_t startLine = line;
        size_t startCol = column;
        SkipComment();
        return CSSToken(CSSTokenType::Comment, "", startLine, startCol);
    }
    
    // Strings
    if (c == '"' || c == '\'') {
        return ReadString(c);
    }
    
    // Hash (ID selector or hex color)
    if (c == '#') {
        return ReadHash();
    }
    
    // At-keyword
    if (c == '@') {
        return ReadAtKeyword();
    }
    
    // Numbers
    if (IsDigit(c) || (c == '.' && IsDigit(Peek())) || 
        (c == '-' && (IsDigit(Peek()) || Peek() == '.'))) {
        return ReadNumber();
    }
    
    // Identifiers and functions
    if (IsIdentStart(c)) {
        return ReadIdentifier();
    }
    
    // Single character tokens
    size_t startLine = line;
    size_t startCol = column;
    Advance();
    
    switch (c) {
        case ':': return CSSToken(CSSTokenType::Colon, ":", startLine, startCol);
        case ';': return CSSToken(CSSTokenType::Semicolon, ";", startLine, startCol);
        case ',': return CSSToken(CSSTokenType::Comma, ",", startLine, startCol);
        case '{': return CSSToken(CSSTokenType::OpenBrace, "{", startLine, startCol);
        case '}': return CSSToken(CSSTokenType::CloseBrace, "}", startLine, startCol);
        case '(': return CSSToken(CSSTokenType::OpenParen, "(", startLine, startCol);
        case ')': return CSSToken(CSSTokenType::CloseParen, ")", startLine, startCol);
        case '[': return CSSToken(CSSTokenType::OpenBracket, "[", startLine, startCol);
        case ']': return CSSToken(CSSTokenType::CloseBracket, "]", startLine, startCol);
        default: {
            CSSToken token(CSSTokenType::Delim, std::string(1, c), startLine, startCol);
            return token;
        }
    }
}

CSSToken CSSTokenizer::PeekToken() {
    size_t savedPos = position;
    size_t savedLine = line;
    size_t savedCol = column;
    
    CSSToken token = NextToken();
    
    position = savedPos;
    line = savedLine;
    column = savedCol;
    
    return token;
}

bool CSSTokenizer::IsAtEnd() const {
    return position >= input.length();
}

char CSSTokenizer::Current() const {
    if (IsAtEnd()) return '\0';
    return input[position];
}

char CSSTokenizer::Peek(size_t offset) const {
    if (position + offset >= input.length()) return '\0';
    return input[position + offset];
}

char CSSTokenizer::Advance() {
    char c = Current();
    position++;
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

void CSSTokenizer::SkipWhitespace() {
    while (!IsAtEnd() && IsWhitespace(Current())) {
        Advance();
    }
}

void CSSTokenizer::SkipComment() {
    // Consume /*
    Advance();
    Advance();
    
    while (!IsAtEnd()) {
        if (Current() == '*' && Peek() == '/') {
            Advance();
            Advance();
            return;
        }
        Advance();
    }
}

CSSToken CSSTokenizer::ReadIdentifier() {
    size_t startLine = line;
    size_t startCol = column;
    std::string value;
    
    while (!IsAtEnd() && IsIdentChar(Current())) {
        value += Advance();
    }
    
    // Check if it's a function (followed by '(')
    if (!IsAtEnd() && Current() == '(') {
        Advance();  // Consume '('
        return CSSToken(CSSTokenType::Function, value, startLine, startCol);
    }
    
    // Check for url() function
    if (value == "url" && !IsAtEnd() && Current() == '(') {
        return ReadUrl();
    }
    
    return CSSToken(CSSTokenType::Identifier, value, startLine, startCol);
}

CSSToken CSSTokenizer::ReadNumber() {
    size_t startLine = line;
    size_t startCol = column;
    std::string value;
    
    // Handle negative sign
    if (Current() == '-') {
        value += Advance();
    }
    
    // Integer part
    while (!IsAtEnd() && IsDigit(Current())) {
        value += Advance();
    }
    
    // Decimal part
    if (!IsAtEnd() && Current() == '.' && IsDigit(Peek())) {
        value += Advance();  // '.'
        while (!IsAtEnd() && IsDigit(Current())) {
            value += Advance();
        }
    }
    
    // Check for unit or percentage
    if (!IsAtEnd()) {
        if (Current() == '%') {
            Advance();
            CSSToken token(CSSTokenType::Percentage, value, startLine, startCol);
            token.numericValue = std::stod(value);
            return token;
        }
        
        if (IsIdentStart(Current())) {
            std::string unit;
            while (!IsAtEnd() && IsIdentChar(Current())) {
                unit += Advance();
            }
            CSSToken token(CSSTokenType::Dimension, value, startLine, startCol);
            token.numericValue = std::stod(value);
            token.unit = unit;
            return token;
        }
    }
    
    CSSToken token(CSSTokenType::Number, value, startLine, startCol);
    token.numericValue = std::stod(value);
    return token;
}

CSSToken CSSTokenizer::ReadString(char quote) {
    size_t startLine = line;
    size_t startCol = column;
    std::string value;
    
    Advance();  // Consume opening quote
    
    while (!IsAtEnd() && Current() != quote) {
        if (Current() == '\\') {
            Advance();  // Skip backslash
            if (!IsAtEnd()) {
                value += Advance();  // Add escaped character
            }
        } else {
            value += Advance();
        }
    }
    
    if (!IsAtEnd()) {
        Advance();  // Consume closing quote
    }
    
    return CSSToken(CSSTokenType::String, value, startLine, startCol);
}

CSSToken CSSTokenizer::ReadHash() {
    size_t startLine = line;
    size_t startCol = column;
    
    Advance();  // Consume '#'
    
    std::string value;
    while (!IsAtEnd() && (IsIdentChar(Current()) || IsHexDigit(Current()))) {
        value += Advance();
    }
    
    return CSSToken(CSSTokenType::Hash, value, startLine, startCol);
}

CSSToken CSSTokenizer::ReadAtKeyword() {
    size_t startLine = line;
    size_t startCol = column;
    
    Advance();  // Consume '@'
    
    std::string value;
    while (!IsAtEnd() && IsIdentChar(Current())) {
        value += Advance();
    }
    
    return CSSToken(CSSTokenType::AtKeyword, value, startLine, startCol);
}

CSSToken CSSTokenizer::ReadUrl() {
    size_t startLine = line;
    size_t startCol = column;
    
    // Already consumed 'url', now consume '('
    Advance();
    
    SkipWhitespace();
    
    std::string value;
    
    // Check for quoted URL
    if (!IsAtEnd() && (Current() == '"' || Current() == '\'')) {
        char quote = Advance();
        while (!IsAtEnd() && Current() != quote) {
            if (Current() == '\\') {
                Advance();
                if (!IsAtEnd()) value += Advance();
            } else {
                value += Advance();
            }
        }
        if (!IsAtEnd()) Advance();  // Closing quote
    } else {
        // Unquoted URL
        while (!IsAtEnd() && Current() != ')' && !IsWhitespace(Current())) {
            value += Advance();
        }
    }
    
    SkipWhitespace();
    if (!IsAtEnd() && Current() == ')') {
        Advance();  // Consume ')'
    }
    
    return CSSToken(CSSTokenType::Url, value, startLine, startCol);
}

bool CSSTokenizer::IsDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool CSSTokenizer::IsHexDigit(char c) const {
    return IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool CSSTokenizer::IsLetter(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool CSSTokenizer::IsIdentStart(char c) const {
    return IsLetter(c) || c == '_' || c == '-';
}

bool CSSTokenizer::IsIdentChar(char c) const {
    return IsIdentStart(c) || IsDigit(c);
}

bool CSSTokenizer::IsWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

// ============================================================================
// CSS PARSER IMPLEMENTATION
// ============================================================================

CSSParser::CSSParser()
    : position(0)
{
}

CSSStylesheet CSSParser::Parse(const std::string& css) {
    CSSStylesheet stylesheet;
    
    // Tokenize
    CSSTokenizer tokenizer(css);
    tokens = tokenizer.Tokenize();
    position = 0;
    
    // Parse
    ParseStylesheet(stylesheet);
    
    return stylesheet;
}

std::optional<CSSDeclaration> CSSParser::ParseDeclaration(const std::string& declarationStr) {
    CSSTokenizer tokenizer(declarationStr);
    tokens = tokenizer.Tokenize();
    position = 0;
    
    return ParseDeclarationInternal();
}

CSSValue CSSParser::ParseValue(const std::string& valueStr, CSSPropertyId propertyId) {
    CSSTokenizer tokenizer(valueStr);
    tokens = tokenizer.Tokenize();
    position = 0;
    
    return ParseValueInternal(propertyId);
}

const CSSToken& CSSParser::Current() const {
    static CSSToken eof(CSSTokenType::EndOfFile, "", 0, 0);
    if (position >= tokens.size()) return eof;
    return tokens[position];
}

const CSSToken& CSSParser::Peek(size_t offset) const {
    static CSSToken eof(CSSTokenType::EndOfFile, "", 0, 0);
    if (position + offset >= tokens.size()) return eof;
    return tokens[position + offset];
}

bool CSSParser::Check(CSSTokenType type) const {
    return Current().type == type;
}

bool CSSParser::Match(CSSTokenType type) {
    if (Check(type)) {
        Advance();
        return true;
    }
    return false;
}

void CSSParser::Advance() {
    if (!IsAtEnd()) position++;
}

bool CSSParser::IsAtEnd() const {
    return position >= tokens.size() || Current().type == CSSTokenType::EndOfFile;
}

void CSSParser::SkipWhitespace() {
    while (Check(CSSTokenType::Whitespace)) {
        Advance();
    }
}

void CSSParser::Error(const std::string& message) {
    std::ostringstream oss;
    oss << "Line " << Current().line << ", Col " << Current().column << ": " << message;
    lastError = oss.str();
}

void CSSParser::Synchronize() {
    // Skip to next rule or end
    while (!IsAtEnd()) {
        if (Check(CSSTokenType::CloseBrace)) {
            Advance();
            return;
        }
        if (Check(CSSTokenType::Semicolon)) {
            Advance();
            return;
        }
        Advance();
    }
}

void CSSParser::ParseStylesheet(CSSStylesheet& stylesheet) {
    while (!IsAtEnd()) {
        SkipWhitespace();
        
        if (IsAtEnd()) break;
        
        // Try to parse a rule
        auto rule = ParseRule();
        if (rule) {
            // Check if this is a :root rule for variables
            bool isRoot = false;
            for (const auto& sel : rule->selectors) {
                if (sel.text == ":root") {
                    isRoot = true;
                    break;
                }
            }
            
            if (isRoot) {
                ParseRootVariables(stylesheet, rule->declarations);
            } else {
                stylesheet.rules.push_back(std::move(*rule));
            }
        } else {
            stylesheet.errors.push_back(lastError);
            Synchronize();
        }
    }
}

std::optional<CSSRule> CSSParser::ParseRule() {
    CSSRule rule;
    rule.line = Current().line;
    
    // Parse selectors
    rule.selectors = ParseSelectors();
    if (rule.selectors.empty()) {
        Error("Expected selector");
        return std::nullopt;
    }
    
    // Expect '{'
    SkipWhitespace();
    if (!Match(CSSTokenType::OpenBrace)) {
        Error("Expected '{'");
        return std::nullopt;
    }
    
    // Parse declarations
    rule.declarations = ParseDeclarationBlock();
    
    // Expect '}'
    SkipWhitespace();
    if (!Match(CSSTokenType::CloseBrace)) {
        Error("Expected '}'");
        return std::nullopt;
    }
    
    return rule;
}

std::vector<CSSSelector> CSSParser::ParseSelectors() {
    std::vector<CSSSelector> selectors;
    
    while (!IsAtEnd() && !Check(CSSTokenType::OpenBrace)) {
        CSSSelector selector = ParseSelector();
        if (!selector.text.empty()) {
            selectors.push_back(selector);
        }
        
        SkipWhitespace();
        
        // Check for comma (multiple selectors)
        if (Match(CSSTokenType::Comma)) {
            SkipWhitespace();
            continue;
        }
        
        break;
    }
    
    return selectors;
}

CSSSelector CSSParser::ParseSelector() {
    CSSSelector selector;
    std::string text;
    
    while (!IsAtEnd() && !Check(CSSTokenType::OpenBrace) && !Check(CSSTokenType::Comma)) {
        const CSSToken& token = Current();
        
        if (token.type == CSSTokenType::Identifier) {
            text += token.value;
        } else if (token.type == CSSTokenType::Hash) {
            text += "#" + token.value;
        } else if (token.type == CSSTokenType::Delim && token.value == ".") {
            text += ".";
        } else if (token.type == CSSTokenType::Colon) {
            text += ":";
            Advance();
            // Check for pseudo-class
            if (Check(CSSTokenType::Identifier)) {
                std::string pseudo = Current().value;
                text += pseudo;
                Advance();  // Move past the pseudo-class identifier
                
                // Set pseudo-class flag
                if (pseudo == "hover") {
                    selector.pseudoClass = selector.pseudoClass | CSSPseudoClass::Hover;
                } else if (pseudo == "active") {
                    selector.pseudoClass = selector.pseudoClass | CSSPseudoClass::Active;
                } else if (pseudo == "focus") {
                    selector.pseudoClass = selector.pseudoClass | CSSPseudoClass::Focus;
                } else if (pseudo == "disabled") {
                    selector.pseudoClass = selector.pseudoClass | CSSPseudoClass::Disabled;
                } else if (pseudo == "first-child") {
                    selector.pseudoClass = selector.pseudoClass | CSSPseudoClass::FirstChild;
                } else if (pseudo == "last-child") {
                    selector.pseudoClass = selector.pseudoClass | CSSPseudoClass::LastChild;
                }
            }
            continue;
        } else if (token.type == CSSTokenType::OpenBracket) {
            // Attribute selector
            text += "[";
            Advance();
            while (!IsAtEnd() && !Check(CSSTokenType::CloseBracket)) {
                text += Current().value;
                Advance();
            }
            if (Check(CSSTokenType::CloseBracket)) {
                text += "]";
            }
        } else if (token.type == CSSTokenType::Whitespace) {
            text += " ";
        } else if (token.type == CSSTokenType::Delim) {
            text += token.value;
        } else {
            break;
        }
        
        Advance();
    }
    
    // Trim whitespace
    while (!text.empty() && std::isspace(text.back())) {
        text.pop_back();
    }
    while (!text.empty() && std::isspace(text.front())) {
        text.erase(0, 1);
    }
    
    selector.text = text;
    selector.hash = HashSelector(text);
    selector.specificity = CalculateSelectorSpecificity(text);
    
    return selector;
}

std::vector<CSSDeclaration> CSSParser::ParseDeclarationBlock() {
    std::vector<CSSDeclaration> declarations;
    
    while (!IsAtEnd() && !Check(CSSTokenType::CloseBrace)) {
        SkipWhitespace();
        
        if (Check(CSSTokenType::CloseBrace)) break;
        
        auto decl = ParseDeclarationInternal();
        if (decl) {
            declarations.push_back(std::move(*decl));
        }
        
        // Skip semicolon
        SkipWhitespace();
        Match(CSSTokenType::Semicolon);
    }
    
    return declarations;
}

std::optional<CSSDeclaration> CSSParser::ParseDeclarationInternal() {
    SkipWhitespace();
    
    if (!Check(CSSTokenType::Identifier)) {
        Error("Expected property name");
        return std::nullopt;
    }
    
    CSSDeclaration decl;
    decl.line = Current().line;
    decl.property = Current().value;
    decl.propertyId = ResolvePropertyId(decl.property);
    Advance();
    
    SkipWhitespace();
    
    if (!Match(CSSTokenType::Colon)) {
        Error("Expected ':'");
        return std::nullopt;
    }
    
    SkipWhitespace();
    
    // Parse value
    decl.value = ParseValueInternal(decl.propertyId);
    
    // Check for !important
    SkipWhitespace();
    if (Check(CSSTokenType::Delim) && Current().value == "!") {
        Advance();
        SkipWhitespace();
        if (Check(CSSTokenType::Identifier) && Current().value == "important") {
            decl.important = true;
            Advance();
        }
    }
    
    return decl;
}

CSSValue CSSParser::ParseValueInternal(CSSPropertyId propertyId) {
    CSSValue value;
    std::string rawValue;
    
    SkipWhitespace();
    
    const CSSToken& token = Current();
    
    // Check for keywords
    if (token.type == CSSTokenType::Identifier) {
        std::string ident = token.value;
        std::transform(ident.begin(), ident.end(), ident.begin(), ::tolower);
        
        if (ident == "auto") {
            value.type = CSSValueType::Auto;
            Advance();
            return value;
        }
        if (ident == "none") {
            value.type = CSSValueType::None;
            Advance();
            return value;
        }
        if (ident == "inherit") {
            value.type = CSSValueType::Inherit;
            Advance();
            return value;
        }
        
        // Check for named color
        auto namedColor = ParseNamedColor(ident);
        if (namedColor) {
            value.type = CSSValueType::Color;
            value.data = *namedColor;
            Advance();
            return value;
        }
        
        // Check for enum values based on property
        // This is a simplified version - real implementation would have complete mappings
        value.type = CSSValueType::Enum;
        value.data = ident;
        Advance();
        return value;
    }
    
    // Hash (hex color)
    if (token.type == CSSTokenType::Hash) {
        value.type = CSSValueType::Color;
        value.data = ParseHexColor(token.value);
        Advance();
        return value;
    }
    
    // Function (rgb, rgba, var, linear-gradient, etc.)
    if (token.type == CSSTokenType::Function) {
        std::string funcName = token.value;
        std::transform(funcName.begin(), funcName.end(), funcName.begin(), ::tolower);
        
        Advance();  // Move past function name
        
        if (funcName == "rgb") {
            value.type = CSSValueType::Color;
            value.data = ParseRgbFunction();
            return value;
        }
        if (funcName == "rgba") {
            value.type = CSSValueType::Color;
            value.data = ParseRgbaFunction();
            return value;
        }
        if (funcName == "var") {
            // Variable reference
            SkipWhitespace();
            if (Check(CSSTokenType::Identifier)) {
                std::string varName = Current().value;
                if (varName.substr(0, 2) == "--") {
                    varName = varName.substr(2);
                }
                value.type = CSSValueType::VarRef;
                value.data = varName;
                Advance();
            }
            // Skip to closing paren
            while (!IsAtEnd() && !Check(CSSTokenType::CloseParen)) {
                Advance();
            }
            Match(CSSTokenType::CloseParen);
            return value;
        }
        if (funcName == "linear-gradient" || funcName == "radial-gradient") {
            // TODO: Full gradient parsing
            value.type = CSSValueType::Gradient;
            // Skip gradient contents for now
            int depth = 1;
            while (!IsAtEnd() && depth > 0) {
                if (Check(CSSTokenType::OpenParen)) depth++;
                if (Check(CSSTokenType::CloseParen)) depth--;
                Advance();
            }
            return value;
        }
        
        // Unknown function - skip contents
        int depth = 1;
        while (!IsAtEnd() && depth > 0) {
            if (Check(CSSTokenType::OpenParen)) depth++;
            if (Check(CSSTokenType::CloseParen)) depth--;
            Advance();
        }
        return value;
    }
    
    // Dimension (10px, 2em, etc.)
    if (token.type == CSSTokenType::Dimension) {
        value.type = CSSValueType::Length;
        CSSLengthValue length;
        length.value = token.numericValue;
        length.unit = ParseLengthUnit(token.unit);
        value.data = length;
        Advance();
        
        // Check for multi-length values (padding, margin, etc.)
        if (propertyId == CSSPropertyId::Padding || 
            propertyId == CSSPropertyId::Margin ||
            propertyId == CSSPropertyId::BorderRadius) {
            return ParseMultiLengthValue(4);
        }
        
        return value;
    }
    
    // Percentage
    if (token.type == CSSTokenType::Percentage) {
        value.type = CSSValueType::Percentage;
        value.data = token.numericValue;
        Advance();
        return value;
    }
    
    // Number
    if (token.type == CSSTokenType::Number) {
        // Could be length with implicit px, or just a number
        value.type = CSSValueType::Float;
        value.data = token.numericValue;
        Advance();
        return value;
    }
    
    // String
    if (token.type == CSSTokenType::String) {
        value.type = CSSValueType::StringRef;
        value.data = token.value;
        Advance();
        return value;
    }
    
    value.type = CSSValueType::Invalid;
    return value;
}

CSSValue CSSParser::ParseMultiLengthValue(int maxValues) {
    CSSValue value;
    std::vector<CSSLengthValue> lengths;
    
    // First value already parsed - need to backtrack
    position--;
    
    for (int i = 0; i < maxValues; i++) {
        SkipWhitespace();
        
        const CSSToken& token = Current();
        
        if (token.type == CSSTokenType::Dimension) {
            CSSLengthValue length;
            length.value = token.numericValue;
            length.unit = ParseLengthUnit(token.unit);
            lengths.push_back(length);
            Advance();
        } else if (token.type == CSSTokenType::Number && token.numericValue == 0) {
            lengths.push_back(CSSLengthValue(0, CSSLengthUnit::Px));
            Advance();
        } else if (token.type == CSSTokenType::Percentage) {
            CSSLengthValue length;
            length.value = token.numericValue;
            length.unit = CSSLengthUnit::Percent;
            lengths.push_back(length);
            Advance();
        } else {
            break;
        }
    }
    
    if (lengths.size() == 1) {
        value.type = CSSValueType::Length;
        value.data = lengths[0];
    } else {
        value.type = CSSValueType::MultiLength;
        value.data = lengths;
    }
    
    return value;
}

CSSColorValue CSSParser::ParseHexColor(const std::string& hex) {
    CSSColorValue color;
    
    if (hex.length() == 3) {
        // #RGB
        color.r = static_cast<uint8_t>(std::stoi(std::string(2, hex[0]), nullptr, 16));
        color.g = static_cast<uint8_t>(std::stoi(std::string(2, hex[1]), nullptr, 16));
        color.b = static_cast<uint8_t>(std::stoi(std::string(2, hex[2]), nullptr, 16));
        color.a = 255;
    } else if (hex.length() == 4) {
        // #RGBA
        color.r = static_cast<uint8_t>(std::stoi(std::string(2, hex[0]), nullptr, 16));
        color.g = static_cast<uint8_t>(std::stoi(std::string(2, hex[1]), nullptr, 16));
        color.b = static_cast<uint8_t>(std::stoi(std::string(2, hex[2]), nullptr, 16));
        color.a = static_cast<uint8_t>(std::stoi(std::string(2, hex[3]), nullptr, 16));
    } else if (hex.length() == 6) {
        // #RRGGBB
        color.r = static_cast<uint8_t>(std::stoi(hex.substr(0, 2), nullptr, 16));
        color.g = static_cast<uint8_t>(std::stoi(hex.substr(2, 2), nullptr, 16));
        color.b = static_cast<uint8_t>(std::stoi(hex.substr(4, 2), nullptr, 16));
        color.a = 255;
    } else if (hex.length() == 8) {
        // #RRGGBBAA
        color.r = static_cast<uint8_t>(std::stoi(hex.substr(0, 2), nullptr, 16));
        color.g = static_cast<uint8_t>(std::stoi(hex.substr(2, 2), nullptr, 16));
        color.b = static_cast<uint8_t>(std::stoi(hex.substr(4, 2), nullptr, 16));
        color.a = static_cast<uint8_t>(std::stoi(hex.substr(6, 2), nullptr, 16));
    }
    
    return color;
}

CSSColorValue CSSParser::ParseRgbFunction() {
    CSSColorValue color;
    
    SkipWhitespace();
    
    // Parse R
    if (Check(CSSTokenType::Number) || Check(CSSTokenType::Percentage)) {
        color.r = static_cast<uint8_t>(std::clamp(Current().numericValue, 0.0, 255.0));
        Advance();
    }
    
    SkipWhitespace();
    Match(CSSTokenType::Comma);
    SkipWhitespace();
    
    // Parse G
    if (Check(CSSTokenType::Number) || Check(CSSTokenType::Percentage)) {
        color.g = static_cast<uint8_t>(std::clamp(Current().numericValue, 0.0, 255.0));
        Advance();
    }
    
    SkipWhitespace();
    Match(CSSTokenType::Comma);
    SkipWhitespace();
    
    // Parse B
    if (Check(CSSTokenType::Number) || Check(CSSTokenType::Percentage)) {
        color.b = static_cast<uint8_t>(std::clamp(Current().numericValue, 0.0, 255.0));
        Advance();
    }
    
    color.a = 255;
    
    SkipWhitespace();
    Match(CSSTokenType::CloseParen);
    
    return color;
}

CSSColorValue CSSParser::ParseRgbaFunction() {
    CSSColorValue color = ParseRgbFunction();
    
    // Go back and parse alpha
    // This is a simplified version - proper implementation would parse all 4 values
    
    return color;
}

std::optional<CSSColorValue> CSSParser::ParseNamedColor(const std::string& name) {
    return GetNamedColor(name);
}

CSSLengthUnit CSSParser::ParseLengthUnit(const std::string& unit) {
    std::string lowerUnit = unit;
    std::transform(lowerUnit.begin(), lowerUnit.end(), lowerUnit.begin(), ::tolower);
    
    if (lowerUnit == "px") return CSSLengthUnit::Px;
    if (lowerUnit == "em") return CSSLengthUnit::Em;
    if (lowerUnit == "rem") return CSSLengthUnit::Rem;
    if (lowerUnit == "%") return CSSLengthUnit::Percent;
    if (lowerUnit == "vw") return CSSLengthUnit::Vw;
    if (lowerUnit == "vh") return CSSLengthUnit::Vh;
    if (lowerUnit == "vmin") return CSSLengthUnit::Vmin;
    if (lowerUnit == "vmax") return CSSLengthUnit::Vmax;
    if (lowerUnit == "pt") return CSSLengthUnit::Pt;
    if (lowerUnit == "ch") return CSSLengthUnit::Ch;
    if (lowerUnit == "ex") return CSSLengthUnit::Ex;
    
    return CSSLengthUnit::Px;  // Default to pixels
}

uint16_t CSSParser::CalculateSelectorSpecificity(const std::string& selector) {
    uint8_t ids = 0;
    uint8_t classes = 0;
    uint8_t elements = 0;
    
    bool inId = false;
    bool inClass = false;
    bool inElement = true;
    
    for (size_t i = 0; i < selector.length(); i++) {
        char c = selector[i];
        
        if (c == '#') {
            inId = true;
            inClass = false;
            inElement = false;
        } else if (c == '.') {
            inId = false;
            inClass = true;
            inElement = false;
        } else if (c == ' ' || c == '>' || c == '+' || c == '~') {
            if (inId) ids++;
            else if (inClass) classes++;
            else if (inElement) elements++;
            
            inId = false;
            inClass = false;
            inElement = true;
        } else if (c == ':') {
            // Pseudo-class counts as class
            if (inId) ids++;
            else if (inClass) classes++;
            else if (inElement) elements++;
            
            inClass = true;
            inId = false;
            inElement = false;
        } else if (c == '[') {
            // Attribute selector counts as class
            if (inId) ids++;
            else if (inClass) classes++;
            else if (inElement) elements++;
            
            inClass = true;
            inId = false;
            inElement = false;
            
            // Skip to ]
            while (i < selector.length() && selector[i] != ']') i++;
        }
    }
    
    // Count final segment
    if (inId) ids++;
    else if (inClass) classes++;
    else if (inElement && !selector.empty()) elements++;
    
    return CalculateSpecificity(ids, classes, elements);
}

CSSPropertyId CSSParser::ResolvePropertyId(const std::string& name) {
    return GetPropertyId(name);
}

void CSSParser::ParseRootVariables(CSSStylesheet& stylesheet, const std::vector<CSSDeclaration>& declarations) {
    for (const auto& decl : declarations) {
        if (decl.property.substr(0, 2) == "--") {
            CSSVariable var;
            var.name = decl.property.substr(2);
            var.value = decl.value;
            var.line = decl.line;
            stylesheet.variables.push_back(var);
        }
    }
}

// ============================================================================
// PROPERTY NAME MAPPING
// ============================================================================

CSSPropertyId GetPropertyId(const std::string& name) {
    static const std::unordered_map<std::string, CSSPropertyId> propertyMap = {
        // Layout
        {"display", CSSPropertyId::Display},
        {"position", CSSPropertyId::Position},
        {"flex-direction", CSSPropertyId::FlexDirection},
        {"justify-content", CSSPropertyId::JustifyContent},
        {"align-items", CSSPropertyId::AlignItems},
        {"align-content", CSSPropertyId::AlignContent},
        {"flex-wrap", CSSPropertyId::FlexWrap},
        {"flex-grow", CSSPropertyId::FlexGrow},
        {"flex-shrink", CSSPropertyId::FlexShrink},
        {"flex-basis", CSSPropertyId::FlexBasis},
        {"order", CSSPropertyId::Order},
        {"gap", CSSPropertyId::Gap},
        {"row-gap", CSSPropertyId::RowGap},
        {"column-gap", CSSPropertyId::ColumnGap},
        {"align-self", CSSPropertyId::AlignSelf},
        
        // Box Model
        {"width", CSSPropertyId::Width},
        {"height", CSSPropertyId::Height},
        {"padding", CSSPropertyId::Padding},
        {"padding-top", CSSPropertyId::PaddingTop},
        {"padding-right", CSSPropertyId::PaddingRight},
        {"padding-bottom", CSSPropertyId::PaddingBottom},
        {"padding-left", CSSPropertyId::PaddingLeft},
        {"margin", CSSPropertyId::Margin},
        {"margin-top", CSSPropertyId::MarginTop},
        {"margin-right", CSSPropertyId::MarginRight},
        {"margin-bottom", CSSPropertyId::MarginBottom},
        {"margin-left", CSSPropertyId::MarginLeft},
        {"min-width", CSSPropertyId::MinWidth},
        {"max-width", CSSPropertyId::MaxWidth},
        {"min-height", CSSPropertyId::MinHeight},
        {"max-height", CSSPropertyId::MaxHeight},
        
        // Typography
        {"font-family", CSSPropertyId::FontFamily},
        {"font-size", CSSPropertyId::FontSize},
        {"font-weight", CSSPropertyId::FontWeight},
        {"font-style", CSSPropertyId::FontStyle},
        {"color", CSSPropertyId::Color},
        {"text-align", CSSPropertyId::TextAlign},
        {"text-decoration", CSSPropertyId::TextDecoration},
        {"text-transform", CSSPropertyId::TextTransform},
        {"line-height", CSSPropertyId::LineHeight},
        {"letter-spacing", CSSPropertyId::LetterSpacing},
        {"word-spacing", CSSPropertyId::WordSpacing},
        {"white-space", CSSPropertyId::WhiteSpace},
        {"text-overflow", CSSPropertyId::TextOverflow},
        
        // Visual
        {"background", CSSPropertyId::Background},
        {"background-color", CSSPropertyId::BackgroundColor},
        {"background-image", CSSPropertyId::BackgroundImage},
        {"border-radius", CSSPropertyId::BorderRadius},
        {"border-top-left-radius", CSSPropertyId::BorderTopLeftRadius},
        {"border-top-right-radius", CSSPropertyId::BorderTopRightRadius},
        {"border-bottom-right-radius", CSSPropertyId::BorderBottomRightRadius},
        {"border-bottom-left-radius", CSSPropertyId::BorderBottomLeftRadius},
        {"border", CSSPropertyId::Border},
        {"border-width", CSSPropertyId::BorderWidth},
        {"border-style", CSSPropertyId::BorderStyle},
        {"border-color", CSSPropertyId::BorderColor},
        {"box-shadow", CSSPropertyId::BoxShadow},
        {"opacity", CSSPropertyId::Opacity},
        
        // Visibility & Interaction
        {"visibility", CSSPropertyId::Visibility},
        {"overflow", CSSPropertyId::Overflow},
        {"overflow-x", CSSPropertyId::OverflowX},
        {"overflow-y", CSSPropertyId::OverflowY},
        {"pointer-events", CSSPropertyId::PointerEvents},
        {"cursor", CSSPropertyId::Cursor},
        {"z-index", CSSPropertyId::ZIndex},
        
        // Positioning
        {"top", CSSPropertyId::Top},
        {"right", CSSPropertyId::Right},
        {"bottom", CSSPropertyId::Bottom},
        {"left", CSSPropertyId::Left},
        
        // Transitions
        {"transition", CSSPropertyId::Transition},
        {"transition-property", CSSPropertyId::TransitionProperty},
        {"transition-duration", CSSPropertyId::TransitionDuration},
        {"transition-timing-function", CSSPropertyId::TransitionTimingFunction}
    };
    
    auto it = propertyMap.find(name);
    if (it != propertyMap.end()) {
        return it->second;
    }
    return CSSPropertyId::Invalid;
}

const char* GetPropertyName(CSSPropertyId id) {
    switch (id) {
        case CSSPropertyId::Display: return "display";
        case CSSPropertyId::Position: return "position";
        case CSSPropertyId::FlexDirection: return "flex-direction";
        case CSSPropertyId::JustifyContent: return "justify-content";
        case CSSPropertyId::AlignItems: return "align-items";
        case CSSPropertyId::Width: return "width";
        case CSSPropertyId::Height: return "height";
        case CSSPropertyId::Padding: return "padding";
        case CSSPropertyId::Margin: return "margin";
        case CSSPropertyId::Color: return "color";
        case CSSPropertyId::BackgroundColor: return "background-color";
        case CSSPropertyId::FontSize: return "font-size";
        case CSSPropertyId::FontFamily: return "font-family";
        case CSSPropertyId::BorderRadius: return "border-radius";
        case CSSPropertyId::Opacity: return "opacity";
        // ... add more as needed
        default: return "unknown";
    }
}

bool IsShorthandProperty(CSSPropertyId id) {
    switch (id) {
        case CSSPropertyId::Padding:
        case CSSPropertyId::Margin:
        case CSSPropertyId::Border:
        case CSSPropertyId::BorderRadius:
        case CSSPropertyId::Background:
        case CSSPropertyId::Transition:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// NAMED COLOR MAPPING
// ============================================================================

std::optional<CSSColorValue> GetNamedColor(const std::string& name) {
    static const std::unordered_map<std::string, CSSColorValue> namedColors = {
        // Basic colors
        {"transparent", CSSColorValue(0, 0, 0, 0)},
        {"black", CSSColorValue(0, 0, 0)},
        {"white", CSSColorValue(255, 255, 255)},
        {"red", CSSColorValue(255, 0, 0)},
        {"green", CSSColorValue(0, 128, 0)},
        {"blue", CSSColorValue(0, 0, 255)},
        {"yellow", CSSColorValue(255, 255, 0)},
        {"cyan", CSSColorValue(0, 255, 255)},
        {"magenta", CSSColorValue(255, 0, 255)},
        
        // Extended colors
        {"gray", CSSColorValue(128, 128, 128)},
        {"grey", CSSColorValue(128, 128, 128)},
        {"silver", CSSColorValue(192, 192, 192)},
        {"maroon", CSSColorValue(128, 0, 0)},
        {"olive", CSSColorValue(128, 128, 0)},
        {"lime", CSSColorValue(0, 255, 0)},
        {"aqua", CSSColorValue(0, 255, 255)},
        {"teal", CSSColorValue(0, 128, 128)},
        {"navy", CSSColorValue(0, 0, 128)},
        {"fuchsia", CSSColorValue(255, 0, 255)},
        {"purple", CSSColorValue(128, 0, 128)},
        {"orange", CSSColorValue(255, 165, 0)},
        {"pink", CSSColorValue(255, 192, 203)},
        {"brown", CSSColorValue(165, 42, 42)},
        
        // Web colors (subset)
        {"coral", CSSColorValue(255, 127, 80)},
        {"crimson", CSSColorValue(220, 20, 60)},
        {"darkblue", CSSColorValue(0, 0, 139)},
        {"darkgray", CSSColorValue(169, 169, 169)},
        {"darkgreen", CSSColorValue(0, 100, 0)},
        {"darkred", CSSColorValue(139, 0, 0)},
        {"gold", CSSColorValue(255, 215, 0)},
        {"indigo", CSSColorValue(75, 0, 130)},
        {"ivory", CSSColorValue(255, 255, 240)},
        {"khaki", CSSColorValue(240, 230, 140)},
        {"lavender", CSSColorValue(230, 230, 250)},
        {"lightblue", CSSColorValue(173, 216, 230)},
        {"lightgray", CSSColorValue(211, 211, 211)},
        {"lightgreen", CSSColorValue(144, 238, 144)},
        {"linen", CSSColorValue(250, 240, 230)},
        {"mintcream", CSSColorValue(245, 255, 250)},
        {"mistyrose", CSSColorValue(255, 228, 225)},
        {"moccasin", CSSColorValue(255, 228, 181)},
        {"oldlace", CSSColorValue(253, 245, 230)},
        {"orangered", CSSColorValue(255, 69, 0)},
        {"orchid", CSSColorValue(218, 112, 214)},
        {"salmon", CSSColorValue(250, 128, 114)},
        {"seagreen", CSSColorValue(46, 139, 87)},
        {"sienna", CSSColorValue(160, 82, 45)},
        {"skyblue", CSSColorValue(135, 206, 235)},
        {"slategray", CSSColorValue(112, 128, 144)},
        {"snow", CSSColorValue(255, 250, 250)},
        {"steelblue", CSSColorValue(70, 130, 180)},
        {"tan", CSSColorValue(210, 180, 140)},
        {"thistle", CSSColorValue(216, 191, 216)},
        {"tomato", CSSColorValue(255, 99, 71)},
        {"turquoise", CSSColorValue(64, 224, 208)},
        {"violet", CSSColorValue(238, 130, 238)},
        {"wheat", CSSColorValue(245, 222, 179)},
        {"whitesmoke", CSSColorValue(245, 245, 245)},
        {"yellowgreen", CSSColorValue(154, 205, 50)}
    };
    
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    auto it = namedColors.find(lowerName);
    if (it != namedColors.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace UltraWeb
