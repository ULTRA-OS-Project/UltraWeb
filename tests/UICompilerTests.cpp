// UltraWeb/tests/UICompilerTests.cpp
// Unit tests for UCML Parser and UCB Compiler
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "../include/UltraWebUIParser.h"
#include "../include/UltraWebUICompiler.h"
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace UltraWeb;

// ============================================================================
// TEST HELPERS
// ============================================================================

void PrintTestResult(const std::string& testName, bool passed) {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << testName << std::endl;
}

void PrintSeparator() {
    std::cout << std::string(60, '=') << std::endl;
}

// ============================================================================
// TOKENIZER TESTS
// ============================================================================

bool TestTokenizer_SimpleTag() {
    UCMLTokenizer tokenizer("<div></div>");
    auto tokens = tokenizer.Tokenize();
    
    // Expected: TagOpen, Identifier(div), TagClose, TagEnd, Identifier(div), TagClose, EOF
    if (tokens.size() < 6) return false;
    if (tokens[0].type != UCMLTokenType::TagOpen) return false;
    if (tokens[1].type != UCMLTokenType::Identifier || tokens[1].value != "div") return false;
    if (tokens[2].type != UCMLTokenType::TagClose) return false;
    
    return true;
}

bool TestTokenizer_SelfClosing() {
    UCMLTokenizer tokenizer("<input />");
    auto tokens = tokenizer.Tokenize();
    
    // Expected: TagOpen, Identifier(input), TagSelfClose, EOF
    if (tokens.size() < 3) return false;
    if (tokens[0].type != UCMLTokenType::TagOpen) return false;
    if (tokens[1].type != UCMLTokenType::Identifier || tokens[1].value != "input") return false;
    if (tokens[2].type != UCMLTokenType::TagSelfClose) return false;
    
    return true;
}

bool TestTokenizer_Attributes() {
    UCMLTokenizer tokenizer("<button id=\"btn1\" class=\"primary\">Click</button>");
    auto tokens = tokenizer.Tokenize();
    
    // Check for attribute tokens
    bool hasId = false;
    bool hasClass = false;
    bool hasText = false;
    
    for (const auto& token : tokens) {
        if (token.type == UCMLTokenType::Identifier && token.value == "id") hasId = true;
        if (token.type == UCMLTokenType::Identifier && token.value == "class") hasClass = true;
        if (token.type == UCMLTokenType::Text && token.value == "Click") hasText = true;
    }
    
    return hasId && hasClass && hasText;
}

bool TestTokenizer_Comment() {
    UCMLTokenizer tokenizer("<!-- This is a comment --><div></div>");
    auto tokens = tokenizer.Tokenize();
    
    bool hasComment = false;
    for (const auto& token : tokens) {
        if (token.type == UCMLTokenType::Comment) {
            hasComment = true;
            break;
        }
    }
    
    return hasComment;
}

// ============================================================================
// PARSER TESTS
// ============================================================================

bool TestParser_SimpleElement() {
    UCMLParser parser;
    std::string ucml = "<div></div>";
    
    UCMLDocument doc = parser.Parse(ucml);
    
    if (doc.HasErrors()) return false;
    if (!doc.root) return false;
    if (doc.root->tagName != "div") return false;
    if (doc.root->elementType != UCBElementType::Container) return false;
    
    return true;
}

bool TestParser_NestedElements() {
    UCMLParser parser;
    std::string ucml = R"(
        <div>
            <span>Hello</span>
            <button>Click</button>
        </div>
    )";
    
    UCMLDocument doc = parser.Parse(ucml);
    
    if (doc.HasErrors()) return false;
    if (!doc.root) return false;
    if (doc.root->children.size() != 2) return false;
    if (doc.root->children[0]->tagName != "span") return false;
    if (doc.root->children[1]->tagName != "button") return false;
    
    return true;
}

bool TestParser_Attributes() {
    UCMLParser parser;
    std::string ucml = R"(<button id="btn1" class="primary large" disabled>Submit</button>)";
    
    UCMLDocument doc = parser.Parse(ucml);
    
    if (doc.HasErrors()) return false;
    if (!doc.root) return false;
    
    // Check attributes
    const auto* idAttr = doc.root->GetAttribute("id");
    if (!idAttr) return false;
    const auto* idValue = idAttr->AsString();
    if (!idValue || *idValue != "btn1") return false;
    
    const auto* classAttr = doc.root->GetAttribute("class");
    if (!classAttr) return false;
    
    const auto* disabledAttr = doc.root->GetAttribute("disabled");
    if (!disabledAttr) return false;
    
    return true;
}

bool TestParser_TextContent() {
    UCMLParser parser;
    std::string ucml = "<p>Hello, World!</p>";
    
    UCMLDocument doc = parser.Parse(ucml);
    
    if (doc.HasErrors()) return false;
    if (!doc.root) return false;
    if (doc.root->textContent != "Hello, World!") return false;
    
    return true;
}

bool TestParser_SelfClosingTags() {
    UCMLParser parser;
    std::string ucml = R"(<input type="text" placeholder="Enter name" />)";
    
    UCMLDocument doc = parser.Parse(ucml);
    
    if (doc.HasErrors()) return false;
    if (!doc.root) return false;
    if (!doc.root->selfClosing) return false;
    if (doc.root->elementType != UCBElementType::Input) return false;
    
    return true;
}

bool TestParser_ElementTypes() {
    UCMLParser parser;
    
    // Test various element types
    struct TestCase {
        std::string ucml;
        UCBElementType expected;
    };
    
    std::vector<TestCase> testCases = {
        {"<div></div>", UCBElementType::Container},
        {"<flex></flex>", UCBElementType::FlexBox},
        {"<grid></grid>", UCBElementType::Grid},
        {"<button></button>", UCBElementType::Button},
        {"<input />", UCBElementType::Input},
        {"<img />", UCBElementType::Image},
        {"<text></text>", UCBElementType::Text},
        {"<list></list>", UCBElementType::List},
        {"<table></table>", UCBElementType::Table},
        {"<modal></modal>", UCBElementType::Modal}
    };
    
    for (const auto& tc : testCases) {
        UCMLDocument doc = parser.Parse(tc.ucml);
        if (doc.HasErrors() || !doc.root) return false;
        if (doc.root->elementType != tc.expected) return false;
    }
    
    return true;
}

bool TestParser_EventHandlers() {
    UCMLParser parser;
    std::string ucml = R"(<button onclick="handleClick" onmouseenter="handleHover">Test</button>)";
    
    UCMLDocument doc = parser.Parse(ucml);
    
    if (doc.HasErrors()) return false;
    
    const auto* onclick = doc.root->GetAttribute("onclick");
    if (!onclick) return false;
    
    const auto* onmouseenter = doc.root->GetAttribute("onmouseenter");
    if (!onmouseenter) return false;
    
    return true;
}

bool TestParser_ComplexDocument() {
    UCMLParser parser;
    std::string ucml = R"(
        <div id="app" class="container">
            <flex class="header">
                <text class="title">My App</text>
                <button onclick="openMenu">Menu</button>
            </flex>
            <div class="content">
                <input type="text" placeholder="Search..." />
                <list id="items">
                    <div class="item">Item 1</div>
                    <div class="item">Item 2</div>
                    <div class="item">Item 3</div>
                </list>
            </div>
            <flex class="footer">
                <text>© 2025</text>
            </flex>
        </div>
    )";
    
    UCMLDocument doc = parser.Parse(ucml);
    
    if (doc.HasErrors()) {
        for (const auto& err : doc.errors) {
            std::cout << "  Error: " << err << std::endl;
        }
        return false;
    }
    
    // Count elements
    size_t count = doc.CountElements();
    if (count != 12) {
        std::cout << "  Element count: " << count << " (expected 12)" << std::endl;
        return false;
    }
    
    // Find by ID
    auto app = doc.FindById("app");
    if (!app) return false;
    
    auto items = doc.FindById("items");
    if (!items) return false;
    
    return true;
}

// ============================================================================
// COMPILER TESTS
// ============================================================================

bool TestCompiler_BasicCompilation() {
    UICompiler compiler;
    std::string ucml = "<div></div>";
    
    UICompilationResult result = compiler.Compile(ucml);
    
    if (!result.success) {
        std::cout << "  Errors: ";
        for (const auto& err : result.errors) {
            std::cout << err << " ";
        }
        std::cout << std::endl;
        return false;
    }
    
    if (result.data.empty()) return false;
    if (result.elementCount != 1) return false;
    
    return true;
}

bool TestCompiler_MagicNumber() {
    UICompiler compiler;
    std::string ucml = "<div></div>";
    
    UICompilationResult result = compiler.Compile(ucml);
    
    if (!result.success) return false;
    if (result.data.size() < 4) return false;
    
    // Check magic number
    uint32_t magic = *reinterpret_cast<const uint32_t*>(result.data.data());
    if (magic != UCB_MAGIC) {
        std::cout << "  Magic: 0x" << std::hex << magic << std::dec 
                  << " (expected 0x" << std::hex << UCB_MAGIC << std::dec << ")" << std::endl;
        return false;
    }
    
    return true;
}

bool TestCompiler_CompressionRatio() {
    UICompiler compiler;
    std::string ucml = R"(
        <div id="app" class="container main-app">
            <flex class="header navigation">
                <text class="title brand">Application Title</text>
                <button id="menuBtn" class="btn primary" onclick="toggleMenu">Menu</button>
            </flex>
            <div class="content main-content">
                <input id="searchInput" type="text" placeholder="Search..." class="search-input" />
                <list id="itemList" class="item-list scrollable">
                    <div class="item selectable">First Item</div>
                    <div class="item selectable">Second Item</div>
                    <div class="item selectable">Third Item</div>
                </list>
            </div>
            <flex class="footer">
                <text class="copyright">© 2025 MyApp</text>
            </flex>
        </div>
    )";
    
    UICompilationResult result = compiler.Compile(ucml);
    
    if (!result.success) return false;
    
    std::cout << "  Original size: " << result.originalSize << " bytes" << std::endl;
    std::cout << "  Compiled size: " << result.compiledSize << " bytes" << std::endl;
    std::cout << "  Elements: " << result.elementCount << std::endl;
    std::cout << "  Attributes: " << result.attributeCount << std::endl;
    std::cout << "  Compression ratio: " << std::fixed << std::setprecision(2) 
              << result.compressionRatio << "x" << std::endl;
    
    // Binary should be smaller than original
    if (result.compiledSize >= result.originalSize) return false;
    
    return true;
}

bool TestCompiler_WriteToFile() {
    UICompiler compiler;
    std::string ucml = R"(
        <div id="test">
            <button onclick="click">Click Me</button>
        </div>
    )";
    std::string outputPath = "/tmp/test_ui_output.ucb";
    
    bool written = compiler.CompileToFile(ucml, outputPath);
    if (!written) return false;
    
    // Verify file info
    UCBFileInfo info = GetUCBFileInfo(outputPath);
    if (!info.valid) return false;
    if (info.elementCount != 2) return false;
    
    // Cleanup
    std::remove(outputPath.c_str());
    
    return true;
}

bool TestCompiler_AllElementTypes() {
    UICompiler compiler;
    std::string ucml = R"(
        <div>
            <flex></flex>
            <grid></grid>
            <scroll></scroll>
            <text>Hello</text>
            <button>Click</button>
            <input type="text" />
            <textarea></textarea>
            <checkbox />
            <radio />
            <select></select>
            <slider />
            <img src="test.png" />
            <list></list>
            <table></table>
            <tabs></tabs>
            <modal></modal>
            <menu></menu>
        </div>
    )";
    
    UICompilationResult result = compiler.Compile(ucml);
    
    if (!result.success) {
        for (const auto& err : result.errors) {
            std::cout << "  Error: " << err << std::endl;
        }
        return false;
    }
    
    // Should have 18 elements (1 root + 17 children)
    if (result.elementCount != 18) {
        std::cout << "  Element count: " << result.elementCount << " (expected 18)" << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "UltraWeb UI Compiler Test Suite\n";
    PrintSeparator();
    
    int passed = 0;
    int failed = 0;
    
    // Tokenizer Tests
    std::cout << "\n[Tokenizer Tests]\n";
    
    if (TestTokenizer_SimpleTag()) { passed++; PrintTestResult("Simple Tag", true); }
    else { failed++; PrintTestResult("Simple Tag", false); }
    
    if (TestTokenizer_SelfClosing()) { passed++; PrintTestResult("Self Closing", true); }
    else { failed++; PrintTestResult("Self Closing", false); }
    
    if (TestTokenizer_Attributes()) { passed++; PrintTestResult("Attributes", true); }
    else { failed++; PrintTestResult("Attributes", false); }
    
    if (TestTokenizer_Comment()) { passed++; PrintTestResult("Comment", true); }
    else { failed++; PrintTestResult("Comment", false); }
    
    // Parser Tests
    std::cout << "\n[Parser Tests]\n";
    
    if (TestParser_SimpleElement()) { passed++; PrintTestResult("Simple Element", true); }
    else { failed++; PrintTestResult("Simple Element", false); }
    
    if (TestParser_NestedElements()) { passed++; PrintTestResult("Nested Elements", true); }
    else { failed++; PrintTestResult("Nested Elements", false); }
    
    if (TestParser_Attributes()) { passed++; PrintTestResult("Attributes", true); }
    else { failed++; PrintTestResult("Attributes", false); }
    
    if (TestParser_TextContent()) { passed++; PrintTestResult("Text Content", true); }
    else { failed++; PrintTestResult("Text Content", false); }
    
    if (TestParser_SelfClosingTags()) { passed++; PrintTestResult("Self-Closing Tags", true); }
    else { failed++; PrintTestResult("Self-Closing Tags", false); }
    
    if (TestParser_ElementTypes()) { passed++; PrintTestResult("Element Types", true); }
    else { failed++; PrintTestResult("Element Types", false); }
    
    if (TestParser_EventHandlers()) { passed++; PrintTestResult("Event Handlers", true); }
    else { failed++; PrintTestResult("Event Handlers", false); }
    
    if (TestParser_ComplexDocument()) { passed++; PrintTestResult("Complex Document", true); }
    else { failed++; PrintTestResult("Complex Document", false); }
    
    // Compiler Tests
    std::cout << "\n[Compiler Tests]\n";
    
    if (TestCompiler_BasicCompilation()) { passed++; PrintTestResult("Basic Compilation", true); }
    else { failed++; PrintTestResult("Basic Compilation", false); }
    
    if (TestCompiler_MagicNumber()) { passed++; PrintTestResult("Magic Number", true); }
    else { failed++; PrintTestResult("Magic Number", false); }
    
    if (TestCompiler_CompressionRatio()) { passed++; PrintTestResult("Compression Ratio", true); }
    else { failed++; PrintTestResult("Compression Ratio", false); }
    
    if (TestCompiler_WriteToFile()) { passed++; PrintTestResult("Write To File", true); }
    else { failed++; PrintTestResult("Write To File", false); }
    
    if (TestCompiler_AllElementTypes()) { passed++; PrintTestResult("All Element Types", true); }
    else { failed++; PrintTestResult("All Element Types", false); }
    
    // Summary
    std::cout << "\n";
    PrintSeparator();
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    PrintSeparator();
    std::cout << "\n";
    
    return failed > 0 ? 1 : 0;
}
