// UltraWeb/tests/CSSCompilerTests.cpp
// Unit tests for CSS Tokenizer, Parser, and Compiler
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "../include/UltraWebCSSParser.h"
#include "../include/UltraWebCSSCompiler.h"
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

bool TestTokenizer_BasicTokens() {
    CSSTokenizer tokenizer(".btn { color: red; }");
    auto tokens = tokenizer.Tokenize();
    
    // Expected: Delim('.'), Identifier('btn'), OpenBrace, Identifier('color'), 
    //           Colon, Identifier('red'), Semicolon, CloseBrace, EOF
    
    if (tokens.size() < 7) return false;
    if (tokens[1].type != CSSTokenType::Identifier || tokens[1].value != "btn") return false;
    if (tokens[2].type != CSSTokenType::OpenBrace) return false;
    if (tokens[3].type != CSSTokenType::Identifier || tokens[3].value != "color") return false;
    if (tokens[4].type != CSSTokenType::Colon) return false;
    
    return true;
}

bool TestTokenizer_HexColor() {
    CSSTokenizer tokenizer("#FF5733");
    auto tokens = tokenizer.Tokenize();
    
    if (tokens.empty()) return false;
    if (tokens[0].type != CSSTokenType::Hash) return false;
    if (tokens[0].value != "FF5733") return false;
    
    return true;
}

bool TestTokenizer_Dimensions() {
    CSSTokenizer tokenizer("10px 2em 100% 50vh");
    auto tokens = tokenizer.Tokenize();
    
    // Should have 4 dimension/percentage tokens
    int dimCount = 0;
    for (const auto& token : tokens) {
        if (token.type == CSSTokenType::Dimension || 
            token.type == CSSTokenType::Percentage) {
            dimCount++;
        }
    }
    
    return dimCount == 4;
}

bool TestTokenizer_Function() {
    CSSTokenizer tokenizer("rgb(255, 128, 64)");
    auto tokens = tokenizer.Tokenize();
    
    if (tokens.empty()) return false;
    if (tokens[0].type != CSSTokenType::Function) return false;
    if (tokens[0].value != "rgb") return false;
    
    return true;
}

// ============================================================================
// PARSER TESTS
// ============================================================================

bool TestParser_SimpleRule() {
    CSSParser parser;
    std::string css = ".button { background-color: blue; }";
    
    CSSStylesheet stylesheet = parser.Parse(css);
    
    if (stylesheet.HasErrors()) return false;
    if (stylesheet.rules.size() != 1) return false;
    if (stylesheet.rules[0].selectors.empty()) return false;
    if (stylesheet.rules[0].selectors[0].text != ".button") return false;
    if (stylesheet.rules[0].declarations.size() != 1) return false;
    if (stylesheet.rules[0].declarations[0].property != "background-color") return false;
    
    return true;
}

bool TestParser_MultipleSelectors() {
    CSSParser parser;
    std::string css = ".btn, .button, #submit { color: white; }";
    
    CSSStylesheet stylesheet = parser.Parse(css);
    
    if (stylesheet.HasErrors()) return false;
    if (stylesheet.rules.size() != 1) return false;
    if (stylesheet.rules[0].selectors.size() != 3) return false;
    
    return true;
}

bool TestParser_HexColor() {
    CSSParser parser;
    std::string css = "div { color: #FF5733; }";
    
    CSSStylesheet stylesheet = parser.Parse(css);
    
    if (stylesheet.HasErrors()) return false;
    if (stylesheet.rules.size() != 1) return false;
    
    const auto& decl = stylesheet.rules[0].declarations[0];
    if (decl.value.type != CSSValueType::Color) return false;
    
    auto* color = decl.value.AsColor();
    if (!color) return false;
    if (color->r != 0xFF || color->g != 0x57 || color->b != 0x33) return false;
    
    return true;
}

bool TestParser_NamedColor() {
    CSSParser parser;
    std::string css = "div { background: coral; }";
    
    CSSStylesheet stylesheet = parser.Parse(css);
    
    if (stylesheet.HasErrors()) return false;
    
    const auto& decl = stylesheet.rules[0].declarations[0];
    if (decl.value.type != CSSValueType::Color) return false;
    
    auto* color = decl.value.AsColor();
    if (!color) return false;
    // coral = rgb(255, 127, 80)
    if (color->r != 255 || color->g != 127 || color->b != 80) return false;
    
    return true;
}

bool TestParser_Dimensions() {
    CSSParser parser;
    std::string css = "div { width: 100px; height: 50%; }";
    
    CSSStylesheet stylesheet = parser.Parse(css);
    
    if (stylesheet.HasErrors()) return false;
    if (stylesheet.rules[0].declarations.size() != 2) return false;
    
    const auto& widthDecl = stylesheet.rules[0].declarations[0];
    if (widthDecl.value.type != CSSValueType::Length) return false;
    
    auto* length = widthDecl.value.AsLength();
    if (!length) return false;
    if (length->value != 100 || length->unit != CSSLengthUnit::Px) return false;
    
    return true;
}

bool TestParser_PseudoClass() {
    CSSParser parser;
    std::string css = ".btn:hover { background: red; }";
    
    CSSStylesheet stylesheet = parser.Parse(css);
    
    if (stylesheet.HasErrors()) return false;
    if (stylesheet.rules.empty()) return false;
    
    const auto& selector = stylesheet.rules[0].selectors[0];
    if ((selector.pseudoClass & CSSPseudoClass::Hover) != CSSPseudoClass::Hover) return false;
    
    return true;
}

bool TestParser_Variables() {
    CSSParser parser;
    std::string css = ":root { --primary: #3399FF; --spacing: 16px; }";
    
    CSSStylesheet stylesheet = parser.Parse(css);
    
    if (stylesheet.HasErrors()) return false;
    if (stylesheet.variables.size() != 2) return false;
    if (stylesheet.variables[0].name != "primary") return false;
    if (stylesheet.variables[1].name != "spacing") return false;
    
    return true;
}

// ============================================================================
// COMPILER TESTS
// ============================================================================

bool TestCompiler_BasicCompilation() {
    CSSCompiler compiler;
    std::string css = ".btn { display: flex; }";
    
    CompilationResult result = compiler.Compile(css);
    
    if (!result.success) {
        std::cout << "  Errors: ";
        for (const auto& err : result.errors) {
            std::cout << err << " ";
        }
        std::cout << std::endl;
        return false;
    }
    
    if (result.data.empty()) return false;
    if (result.ruleCount != 1) return false;
    
    return true;
}

bool TestCompiler_MagicNumber() {
    CSSCompiler compiler;
    std::string css = ".test { color: red; }";
    
    CompilationResult result = compiler.Compile(css);
    
    if (!result.success) return false;
    if (result.data.size() < 4) return false;
    
    // Check magic number
    uint32_t magic = *reinterpret_cast<const uint32_t*>(result.data.data());
    if (magic != UCS_MAGIC) return false;
    
    return true;
}

bool TestCompiler_CompressionRatio() {
    CSSCompiler compiler;
    std::string css = R"(
        .container {
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            padding: 16px;
            margin: 8px;
            background-color: #FFFFFF;
            border-radius: 8px;
        }
        
        .button {
            display: inline-flex;
            padding: 12px 24px;
            background-color: #3399FF;
            color: white;
            border-radius: 4px;
            cursor: pointer;
        }
        
        .button:hover {
            background-color: #2277DD;
        }
    )";
    
    CompilationResult result = compiler.Compile(css);
    
    if (!result.success) return false;
    
    std::cout << "  Original size: " << result.originalSize << " bytes" << std::endl;
    std::cout << "  Compiled size: " << result.compiledSize << " bytes" << std::endl;
    std::cout << "  Compression ratio: " << std::fixed << std::setprecision(2) 
              << result.compressionRatio << "x" << std::endl;
    
    // Binary should be smaller than original
    if (result.compiledSize >= result.originalSize) return false;
    
    return true;
}

bool TestCompiler_MultipleRules() {
    CSSCompiler compiler;
    std::string css = R"(
        .a { color: red; }
        .b { color: green; }
        .c { color: blue; }
    )";
    
    CompilationResult result = compiler.Compile(css);
    
    if (!result.success) return false;
    if (result.ruleCount != 3) return false;
    
    return true;
}

bool TestCompiler_WriteToFile() {
    CSSCompiler compiler;
    std::string css = ".test { display: flex; color: #FF0000; }";
    std::string outputPath = "/tmp/test_output.ucs";
    
    bool written = compiler.CompileToFile(css, outputPath);
    if (!written) return false;
    
    // Verify file info
    UCSFileInfo info = GetUCSFileInfo(outputPath);
    if (!info.valid) return false;
    if (info.ruleCount != 1) return false;
    
    // Cleanup
    std::remove(outputPath.c_str());
    
    return true;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "UltraWeb CSS Compiler Test Suite\n";
    PrintSeparator();
    
    int passed = 0;
    int failed = 0;
    
    // Tokenizer Tests
    std::cout << "\n[Tokenizer Tests]\n";
    
    if (TestTokenizer_BasicTokens()) { passed++; PrintTestResult("Basic Tokens", true); }
    else { failed++; PrintTestResult("Basic Tokens", false); }
    
    if (TestTokenizer_HexColor()) { passed++; PrintTestResult("Hex Color", true); }
    else { failed++; PrintTestResult("Hex Color", false); }
    
    if (TestTokenizer_Dimensions()) { passed++; PrintTestResult("Dimensions", true); }
    else { failed++; PrintTestResult("Dimensions", false); }
    
    if (TestTokenizer_Function()) { passed++; PrintTestResult("Function", true); }
    else { failed++; PrintTestResult("Function", false); }
    
    // Parser Tests
    std::cout << "\n[Parser Tests]\n";
    
    if (TestParser_SimpleRule()) { passed++; PrintTestResult("Simple Rule", true); }
    else { failed++; PrintTestResult("Simple Rule", false); }
    
    if (TestParser_MultipleSelectors()) { passed++; PrintTestResult("Multiple Selectors", true); }
    else { failed++; PrintTestResult("Multiple Selectors", false); }
    
    if (TestParser_HexColor()) { passed++; PrintTestResult("Hex Color Parsing", true); }
    else { failed++; PrintTestResult("Hex Color Parsing", false); }
    
    if (TestParser_NamedColor()) { passed++; PrintTestResult("Named Color", true); }
    else { failed++; PrintTestResult("Named Color", false); }
    
    if (TestParser_Dimensions()) { passed++; PrintTestResult("Dimensions", true); }
    else { failed++; PrintTestResult("Dimensions", false); }
    
    if (TestParser_PseudoClass()) { passed++; PrintTestResult("Pseudo Class", true); }
    else { failed++; PrintTestResult("Pseudo Class", false); }
    
    if (TestParser_Variables()) { passed++; PrintTestResult("CSS Variables", true); }
    else { failed++; PrintTestResult("CSS Variables", false); }
    
    // Compiler Tests
    std::cout << "\n[Compiler Tests]\n";
    
    if (TestCompiler_BasicCompilation()) { passed++; PrintTestResult("Basic Compilation", true); }
    else { failed++; PrintTestResult("Basic Compilation", false); }
    
    if (TestCompiler_MagicNumber()) { passed++; PrintTestResult("Magic Number", true); }
    else { failed++; PrintTestResult("Magic Number", false); }
    
    if (TestCompiler_CompressionRatio()) { passed++; PrintTestResult("Compression Ratio", true); }
    else { failed++; PrintTestResult("Compression Ratio", false); }
    
    if (TestCompiler_MultipleRules()) { passed++; PrintTestResult("Multiple Rules", true); }
    else { failed++; PrintTestResult("Multiple Rules", false); }
    
    if (TestCompiler_WriteToFile()) { passed++; PrintTestResult("Write To File", true); }
    else { failed++; PrintTestResult("Write To File", false); }
    
    // Summary
    std::cout << "\n";
    PrintSeparator();
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    PrintSeparator();
    std::cout << "\n";
    
    return failed > 0 ? 1 : 0;
}
