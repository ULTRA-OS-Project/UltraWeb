// UltraWeb/core/UltraWebUIParser.cpp
// UCML Parser Implementation
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "../include/UltraWebUIParser.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stack>

namespace UltraWeb {

// ============================================================================
// UCML TOKENIZER IMPLEMENTATION
// ============================================================================

UCMLTokenizer::UCMLTokenizer(const std::string& input)
    : input(input)
    , position(0)
    , line(1)
    , column(1)
    , inTag(false)
{
}

std::vector<UCMLToken> UCMLTokenizer::Tokenize() {
    std::vector<UCMLToken> tokens;
    
    while (!IsAtEnd()) {
        UCMLToken token = NextToken();
        if (token.type != UCMLTokenType::Whitespace) {
            tokens.push_back(token);
        }
        if (token.type == UCMLTokenType::EndOfFile) {
            break;
        }
    }
    
    return tokens;
}

UCMLToken UCMLTokenizer::NextToken() {
    if (IsAtEnd()) {
        return UCMLToken(UCMLTokenType::EndOfFile, "", line, column);
    }
    
    char c = Current();
    
    // Handle whitespace inside tags
    if (inTag && IsWhitespace(c)) {
        size_t startLine = line;
        size_t startCol = column;
        while (!IsAtEnd() && IsWhitespace(Current())) {
            Advance();
        }
        return UCMLToken(UCMLTokenType::Whitespace, " ", startLine, startCol);
    }
    
    // Comment: <!-- ... -->
    if (c == '<' && Peek(1) == '!' && Peek(2) == '-' && Peek(3) == '-') {
        return ReadComment();
    }
    
    // End tag: </
    if (c == '<' && Peek() == '/') {
        size_t startLine = line;
        size_t startCol = column;
        Advance(); // <
        Advance(); // /
        inTag = true;
        return UCMLToken(UCMLTokenType::TagEnd, "</", startLine, startCol);
    }
    
    // Tag open: <
    if (c == '<') {
        size_t startLine = line;
        size_t startCol = column;
        Advance();
        inTag = true;
        return UCMLToken(UCMLTokenType::TagOpen, "<", startLine, startCol);
    }
    
    // Self-closing: />
    if (c == '/' && Peek() == '>') {
        size_t startLine = line;
        size_t startCol = column;
        Advance(); // /
        Advance(); // >
        inTag = false;
        return UCMLToken(UCMLTokenType::TagSelfClose, "/>", startLine, startCol);
    }
    
    // Tag close: >
    if (c == '>') {
        size_t startLine = line;
        size_t startCol = column;
        Advance();
        inTag = false;
        return UCMLToken(UCMLTokenType::TagClose, ">", startLine, startCol);
    }
    
    // Inside tag
    if (inTag) {
        // Equals
        if (c == '=') {
            size_t startLine = line;
            size_t startCol = column;
            Advance();
            return UCMLToken(UCMLTokenType::Equals, "=", startLine, startCol);
        }
        
        // String value
        if (c == '"' || c == '\'') {
            return ReadString();
        }
        
        // Identifier
        if (IsIdentStart(c)) {
            return ReadIdentifier();
        }
    }
    
    // Text content between tags
    if (!inTag) {
        return ReadText();
    }
    
    // Skip unknown character
    size_t startLine = line;
    size_t startCol = column;
    Advance();
    return UCMLToken(UCMLTokenType::Invalid, std::string(1, c), startLine, startCol);
}

bool UCMLTokenizer::IsAtEnd() const {
    return position >= input.length();
}

char UCMLTokenizer::Current() const {
    if (IsAtEnd()) return '\0';
    return input[position];
}

char UCMLTokenizer::Peek(size_t offset) const {
    if (position + offset >= input.length()) return '\0';
    return input[position + offset];
}

char UCMLTokenizer::Advance() {
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

void UCMLTokenizer::SkipWhitespace() {
    while (!IsAtEnd() && IsWhitespace(Current())) {
        Advance();
    }
}

UCMLToken UCMLTokenizer::ReadIdentifier() {
    size_t startLine = line;
    size_t startCol = column;
    std::string value;
    
    while (!IsAtEnd() && IsIdentChar(Current())) {
        value += Advance();
    }
    
    return UCMLToken(UCMLTokenType::Identifier, value, startLine, startCol);
}

UCMLToken UCMLTokenizer::ReadString() {
    size_t startLine = line;
    size_t startCol = column;
    char quote = Advance();  // Consume opening quote
    std::string value;
    
    while (!IsAtEnd() && Current() != quote) {
        if (Current() == '\\') {
            Advance();  // Skip backslash
            if (!IsAtEnd()) {
                char escaped = Advance();
                switch (escaped) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case 'r': value += '\r'; break;
                    case '\\': value += '\\'; break;
                    case '"': value += '"'; break;
                    case '\'': value += '\''; break;
                    default: value += escaped; break;
                }
            }
        } else {
            value += Advance();
        }
    }
    
    if (!IsAtEnd()) {
        Advance();  // Consume closing quote
    }
    
    return UCMLToken(UCMLTokenType::String, value, startLine, startCol);
}

UCMLToken UCMLTokenizer::ReadText() {
    size_t startLine = line;
    size_t startCol = column;
    std::string value;
    
    while (!IsAtEnd() && Current() != '<') {
        value += Advance();
    }
    
    // Trim whitespace
    size_t start = value.find_first_not_of(" \t\n\r");
    size_t end = value.find_last_not_of(" \t\n\r");
    
    if (start == std::string::npos) {
        return UCMLToken(UCMLTokenType::Whitespace, "", startLine, startCol);
    }
    
    value = value.substr(start, end - start + 1);
    
    if (value.empty()) {
        return UCMLToken(UCMLTokenType::Whitespace, "", startLine, startCol);
    }
    
    return UCMLToken(UCMLTokenType::Text, value, startLine, startCol);
}

UCMLToken UCMLTokenizer::ReadComment() {
    size_t startLine = line;
    size_t startCol = column;
    
    // Skip <!--
    Advance(); Advance(); Advance(); Advance();
    
    std::string value;
    while (!IsAtEnd()) {
        if (Current() == '-' && Peek(1) == '-' && Peek(2) == '>') {
            Advance(); Advance(); Advance();  // Skip -->
            break;
        }
        value += Advance();
    }
    
    return UCMLToken(UCMLTokenType::Comment, value, startLine, startCol);
}

bool UCMLTokenizer::IsLetter(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool UCMLTokenizer::IsDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool UCMLTokenizer::IsIdentStart(char c) const {
    return IsLetter(c) || c == '_' || c == ':';
}

bool UCMLTokenizer::IsIdentChar(char c) const {
    return IsIdentStart(c) || IsDigit(c) || c == '-' || c == '.';
}

bool UCMLTokenizer::IsWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// ============================================================================
// UCML ELEMENT HELPERS
// ============================================================================

std::vector<std::string> UCMLElement::GetClasses() const {
    std::vector<std::string> result;
    const auto* classAttr = GetAttribute("class");
    if (!classAttr) return result;
    
    const auto* classStr = classAttr->AsString();
    if (!classStr) return result;
    
    // Split by whitespace
    std::istringstream iss(*classStr);
    std::string className;
    while (iss >> className) {
        result.push_back(className);
    }
    
    return result;
}

// ============================================================================
// UCML DOCUMENT HELPERS
// ============================================================================

size_t UCMLDocument::CountElements() const {
    if (!root) return 0;
    
    size_t count = 0;
    std::stack<std::shared_ptr<UCMLElement>> stack;
    stack.push(root);
    
    while (!stack.empty()) {
        auto elem = stack.top();
        stack.pop();
        count++;
        
        for (auto it = elem->children.rbegin(); it != elem->children.rend(); ++it) {
            stack.push(*it);
        }
    }
    
    return count;
}

std::shared_ptr<UCMLElement> UCMLDocument::FindById(const std::string& id) const {
    if (!root) return nullptr;
    
    std::stack<std::shared_ptr<UCMLElement>> stack;
    stack.push(root);
    
    while (!stack.empty()) {
        auto elem = stack.top();
        stack.pop();
        
        const auto* idAttr = elem->GetAttribute("id");
        if (idAttr) {
            const auto* idStr = idAttr->AsString();
            if (idStr && *idStr == id) {
                return elem;
            }
        }
        
        for (auto it = elem->children.rbegin(); it != elem->children.rend(); ++it) {
            stack.push(*it);
        }
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<UCMLElement>> UCMLDocument::FindByTagName(const std::string& tagName) const {
    std::vector<std::shared_ptr<UCMLElement>> result;
    if (!root) return result;
    
    std::stack<std::shared_ptr<UCMLElement>> stack;
    stack.push(root);
    
    while (!stack.empty()) {
        auto elem = stack.top();
        stack.pop();
        
        if (elem->tagName == tagName) {
            result.push_back(elem);
        }
        
        for (auto it = elem->children.rbegin(); it != elem->children.rend(); ++it) {
            stack.push(*it);
        }
    }
    
    return result;
}

// ============================================================================
// UCML PARSER IMPLEMENTATION
// ============================================================================

UCMLParser::UCMLParser()
    : position(0)
    , nextElementId(1)
{
}

UCMLDocument UCMLParser::Parse(const std::string& ucml) {
    UCMLDocument document;
    
    // Tokenize
    UCMLTokenizer tokenizer(ucml);
    tokens = tokenizer.Tokenize();
    position = 0;
    nextElementId = 1;
    
    // Skip any leading whitespace/text
    SkipWhitespace();
    
    // Parse root element
    if (!IsAtEnd() && Check(UCMLTokenType::TagOpen)) {
        document.root = ParseElementNode(0);
    } else {
        document.errors.push_back("Expected root element");
    }
    
    return document;
}

std::shared_ptr<UCMLElement> UCMLParser::ParseElement(const std::string& ucml) {
    UCMLDocument doc = Parse(ucml);
    return doc.root;
}

const UCMLToken& UCMLParser::Current() const {
    static UCMLToken eof(UCMLTokenType::EndOfFile, "", 0, 0);
    if (position >= tokens.size()) return eof;
    return tokens[position];
}

const UCMLToken& UCMLParser::Peek(size_t offset) const {
    static UCMLToken eof(UCMLTokenType::EndOfFile, "", 0, 0);
    if (position + offset >= tokens.size()) return eof;
    return tokens[position + offset];
}

bool UCMLParser::Check(UCMLTokenType type) const {
    return Current().type == type;
}

bool UCMLParser::Match(UCMLTokenType type) {
    if (Check(type)) {
        Advance();
        return true;
    }
    return false;
}

void UCMLParser::Advance() {
    if (!IsAtEnd()) position++;
}

bool UCMLParser::IsAtEnd() const {
    return position >= tokens.size() || Current().type == UCMLTokenType::EndOfFile;
}

void UCMLParser::SkipWhitespace() {
    while (Check(UCMLTokenType::Whitespace) || Check(UCMLTokenType::Comment)) {
        Advance();
    }
}

void UCMLParser::Error(const std::string& message) {
    std::ostringstream oss;
    oss << "Line " << Current().line << ", Col " << Current().column << ": " << message;
    lastError = oss.str();
}

uint16_t UCMLParser::AssignElementId() {
    return nextElementId++;
}

std::shared_ptr<UCMLElement> UCMLParser::ParseElementNode(uint16_t parentId) {
    auto element = std::make_shared<UCMLElement>();
    element->parentId = parentId;
    element->line = Current().line;
    
    // Expect <
    if (!Match(UCMLTokenType::TagOpen)) {
        Error("Expected '<'");
        return nullptr;
    }
    
    // Get tag name
    if (!Check(UCMLTokenType::Identifier)) {
        Error("Expected tag name");
        return nullptr;
    }
    
    element->tagName = Current().value;
    element->elementType = ResolveElementType(element->tagName);
    element->elementId = AssignElementId();
    Advance();
    
    // Parse attributes
    element->attributes = ParseAttributes();
    
    // Check for self-closing
    if (Match(UCMLTokenType::TagSelfClose)) {
        element->selfClosing = true;
        return element;
    }
    
    // Expect >
    if (!Match(UCMLTokenType::TagClose)) {
        Error("Expected '>' or '/>'");
        return nullptr;
    }
    
    // Check if this tag is self-closing by default (like <input>)
    if (IsSelfClosingTag(element->tagName)) {
        element->selfClosing = true;
        return element;
    }
    
    // Parse children and text content
    while (!IsAtEnd()) {
        SkipWhitespace();
        
        // Check for end tag
        if (Check(UCMLTokenType::TagEnd)) {
            break;
        }
        
        // Check for text content
        if (Check(UCMLTokenType::Text)) {
            element->textContent += Current().value;
            Advance();
            continue;
        }
        
        // Check for child element
        if (Check(UCMLTokenType::TagOpen)) {
            auto child = ParseElementNode(element->elementId);
            if (child) {
                element->children.push_back(child);
            }
            continue;
        }
        
        break;
    }
    
    // Expect </tagName>
    if (!Match(UCMLTokenType::TagEnd)) {
        Error("Expected closing tag '</" + element->tagName + ">'");
        return element;
    }
    
    // Verify tag name matches
    if (!Check(UCMLTokenType::Identifier) || Current().value != element->tagName) {
        Error("Mismatched closing tag, expected '</" + element->tagName + ">'");
    } else {
        Advance();
    }
    
    // Expect >
    Match(UCMLTokenType::TagClose);
    
    return element;
}

std::vector<UCMLAttribute> UCMLParser::ParseAttributes() {
    std::vector<UCMLAttribute> attributes;
    
    while (!IsAtEnd() && !Check(UCMLTokenType::TagClose) && !Check(UCMLTokenType::TagSelfClose)) {
        if (Check(UCMLTokenType::Identifier)) {
            attributes.push_back(ParseAttribute());
        } else {
            break;
        }
    }
    
    return attributes;
}

UCMLAttribute UCMLParser::ParseAttribute() {
    UCMLAttribute attr;
    attr.line = Current().line;
    attr.name = Current().value;
    attr.propertyId = ResolvePropertyId(attr.name);
    Advance();
    
    // Check for value
    if (Match(UCMLTokenType::Equals)) {
        if (Check(UCMLTokenType::String)) {
            std::string strValue = Current().value;
            Advance();
            
            // Try to parse as different types
            if (strValue == "true") {
                attr.value = true;
            } else if (strValue == "false") {
                attr.value = false;
            } else {
                // Try integer
                try {
                    size_t pos;
                    int intVal = std::stoi(strValue, &pos);
                    if (pos == strValue.length()) {
                        attr.value = static_cast<int32_t>(intVal);
                    } else {
                        // Try float
                        double floatVal = std::stod(strValue, &pos);
                        if (pos == strValue.length()) {
                            attr.value = floatVal;
                        } else {
                            attr.value = strValue;
                        }
                    }
                } catch (...) {
                    attr.value = strValue;
                }
            }
        } else if (Check(UCMLTokenType::Identifier)) {
            // Unquoted value
            attr.value = Current().value;
            Advance();
        }
    } else {
        // Boolean attribute (no value)
        attr.value = true;
    }
    
    return attr;
}

UCBElementType UCMLParser::ResolveElementType(const std::string& tagName) {
    return GetElementType(tagName);
}

UCBPropertyId UCMLParser::ResolvePropertyId(const std::string& attrName) {
    return GetUCBPropertyId(attrName);
}

// ============================================================================
// TAG NAME MAPPING
// ============================================================================

UCBElementType GetElementType(const std::string& tagName) {
    static const std::unordered_map<std::string, UCBElementType> tagMap = {
        // Layout containers
        {"div", UCBElementType::Container},
        {"container", UCBElementType::Container},
        {"flex", UCBElementType::FlexBox},
        {"flexbox", UCBElementType::FlexBox},
        {"grid", UCBElementType::Grid},
        {"scroll", UCBElementType::ScrollView},
        {"scrollview", UCBElementType::ScrollView},
        
        // Form elements
        {"span", UCBElementType::Text},
        {"text", UCBElementType::Text},
        {"p", UCBElementType::Text},
        {"h1", UCBElementType::Text},
        {"h2", UCBElementType::Text},
        {"h3", UCBElementType::Text},
        {"h4", UCBElementType::Text},
        {"h5", UCBElementType::Text},
        {"h6", UCBElementType::Text},
        {"label", UCBElementType::Text},
        {"button", UCBElementType::Button},
        {"btn", UCBElementType::Button},
        {"input", UCBElementType::Input},
        {"textarea", UCBElementType::TextArea},
        {"checkbox", UCBElementType::Checkbox},
        {"radio", UCBElementType::Radio},
        {"select", UCBElementType::Select},
        {"dropdown", UCBElementType::Select},
        {"slider", UCBElementType::Slider},
        {"range", UCBElementType::Slider},
        
        // Media elements
        {"img", UCBElementType::Image},
        {"image", UCBElementType::Image},
        {"svg", UCBElementType::SVG},
        {"canvas", UCBElementType::Canvas},
        {"video", UCBElementType::Video},
        {"audio", UCBElementType::Audio},
        
        // Data display
        {"list", UCBElementType::List},
        {"ul", UCBElementType::List},
        {"ol", UCBElementType::List},
        {"table", UCBElementType::Table},
        {"tree", UCBElementType::Tree},
        
        // Complex components
        {"tabs", UCBElementType::Tabs},
        {"modal", UCBElementType::Modal},
        {"dialog", UCBElementType::Modal},
        {"menu", UCBElementType::Menu},
        {"tooltip", UCBElementType::Tooltip},
        {"popover", UCBElementType::Popover}
    };
    
    std::string lowerName = tagName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    auto it = tagMap.find(lowerName);
    if (it != tagMap.end()) {
        return it->second;
    }
    
    return UCBElementType::Custom;
}

const char* GetTagName(UCBElementType type) {
    switch (type) {
        case UCBElementType::Container: return "div";
        case UCBElementType::FlexBox: return "flex";
        case UCBElementType::Grid: return "grid";
        case UCBElementType::ScrollView: return "scroll";
        case UCBElementType::Text: return "text";
        case UCBElementType::Button: return "button";
        case UCBElementType::Input: return "input";
        case UCBElementType::TextArea: return "textarea";
        case UCBElementType::Checkbox: return "checkbox";
        case UCBElementType::Radio: return "radio";
        case UCBElementType::Select: return "select";
        case UCBElementType::Slider: return "slider";
        case UCBElementType::Image: return "img";
        case UCBElementType::SVG: return "svg";
        case UCBElementType::Canvas: return "canvas";
        case UCBElementType::Video: return "video";
        case UCBElementType::Audio: return "audio";
        case UCBElementType::List: return "list";
        case UCBElementType::Table: return "table";
        case UCBElementType::Tree: return "tree";
        case UCBElementType::Tabs: return "tabs";
        case UCBElementType::Modal: return "modal";
        case UCBElementType::Menu: return "menu";
        case UCBElementType::Tooltip: return "tooltip";
        case UCBElementType::Popover: return "popover";
        default: return "custom";
    }
}

bool IsSelfClosingTag(const std::string& tagName) {
    static const std::unordered_map<std::string, bool> selfClosing = {
        {"input", true},
        {"img", true},
        {"image", true},
        {"br", true},
        {"hr", true},
        {"checkbox", true},
        {"radio", true},
        {"slider", true},
        {"range", true}
    };
    
    std::string lowerName = tagName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return selfClosing.count(lowerName) > 0;
}

// ============================================================================
// ATTRIBUTE NAME MAPPING
// ============================================================================

UCBPropertyId GetUCBPropertyId(const std::string& attrName) {
    static const std::unordered_map<std::string, UCBPropertyId> attrMap = {
        // Core properties
        {"id", UCBPropertyId::Id},
        {"class", UCBPropertyId::Class},
        {"name", UCBPropertyId::Name},
        {"visible", UCBPropertyId::Visible},
        {"enabled", UCBPropertyId::Enabled},
        
        // Content properties
        {"text", UCBPropertyId::Text},
        {"value", UCBPropertyId::Value},
        {"placeholder", UCBPropertyId::Placeholder},
        {"src", UCBPropertyId::Src},
        {"alt", UCBPropertyId::Alt},
        {"href", UCBPropertyId::Href},
        
        // Layout hints
        {"width", UCBPropertyId::Width},
        {"height", UCBPropertyId::Height},
        {"min-width", UCBPropertyId::MinWidth},
        {"max-width", UCBPropertyId::MaxWidth},
        {"min-height", UCBPropertyId::MinHeight},
        {"max-height", UCBPropertyId::MaxHeight},
        
        // Form properties
        {"type", UCBPropertyId::Type},
        {"required", UCBPropertyId::Required},
        {"readonly", UCBPropertyId::ReadOnly},
        {"disabled", UCBPropertyId::Disabled},
        {"checked", UCBPropertyId::Checked},
        {"selected", UCBPropertyId::Selected},
        {"min", UCBPropertyId::Min},
        {"max", UCBPropertyId::Max},
        {"step", UCBPropertyId::Step},
        
        // Event handlers
        {"onclick", UCBPropertyId::OnClick},
        {"onchange", UCBPropertyId::OnChange},
        {"oninput", UCBPropertyId::OnInput},
        {"onfocus", UCBPropertyId::OnFocus},
        {"onblur", UCBPropertyId::OnBlur},
        {"onsubmit", UCBPropertyId::OnSubmit},
        {"onkeydown", UCBPropertyId::OnKeyDown},
        {"onkeyup", UCBPropertyId::OnKeyUp},
        {"onmouseenter", UCBPropertyId::OnMouseEnter},
        {"onmouseleave", UCBPropertyId::OnMouseLeave},
        {"onscroll", UCBPropertyId::OnScroll},
        
        // Data binding
        {"bind", UCBPropertyId::Bind},
        {"model", UCBPropertyId::Model},
        {"for", UCBPropertyId::For},
        {"if", UCBPropertyId::If},
        
        // Custom
        {"data", UCBPropertyId::Data}
    };
    
    std::string lowerName = attrName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    auto it = attrMap.find(lowerName);
    if (it != attrMap.end()) {
        return it->second;
    }
    
    return UCBPropertyId::Custom;
}

const char* GetUCBAttributeName(UCBPropertyId id) {
    switch (id) {
        case UCBPropertyId::Id: return "id";
        case UCBPropertyId::Class: return "class";
        case UCBPropertyId::Name: return "name";
        case UCBPropertyId::Visible: return "visible";
        case UCBPropertyId::Enabled: return "enabled";
        case UCBPropertyId::Text: return "text";
        case UCBPropertyId::Value: return "value";
        case UCBPropertyId::Placeholder: return "placeholder";
        case UCBPropertyId::Src: return "src";
        case UCBPropertyId::Alt: return "alt";
        case UCBPropertyId::Href: return "href";
        case UCBPropertyId::OnClick: return "onclick";
        case UCBPropertyId::OnChange: return "onchange";
        default: return "custom";
    }
}

bool IsEventHandler(const std::string& attrName) {
    std::string lowerName = attrName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    return lowerName.substr(0, 2) == "on";
}

bool IsDataBinding(const std::string& attrName) {
    std::string lowerName = attrName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    return lowerName == "bind" || lowerName == "model" || 
           lowerName == "for" || lowerName == "if";
}

} // namespace UltraWeb
