// UltraWeb/tests/RuntimeTests.cpp
// Unit tests for Runtime Loader
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "../runtime/UltraWebRuntime.h"
#include "../include/UltraWebBundler.h"
#include "../include/UltraWebUICompiler.h"
#include "../include/UltraWebCSSCompiler.h"
#include <iostream>
#include <cassert>

using namespace UltraWeb;
using namespace UltraWeb::Runtime;

// ============================================================================
// TEST HELPERS
// ============================================================================

void PrintTestResult(const std::string& testName, bool passed) {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << testName << std::endl;
}

void PrintSeparator() {
    std::cout << std::string(60, '=') << std::endl;
}

// Create a test package with UI and styles
std::vector<uint8_t> CreateTestPackage() {
    PackageBundler bundler;
    
    std::string ucml = R"(
        <div id="app" class="container">
            <flex class="header">
                <text class="title">Hello World</text>
            </flex>
            <div class="content">
                <button id="btn1" onclick="handleClick">Click Me</button>
                <input type="text" placeholder="Enter text" />
            </div>
        </div>
    )";
    
    std::string css = R"(
        .container {
            display: flex;
            flex-direction: column;
            padding: 16px;
        }
        .header {
            background-color: #3366FF;
            padding: 12px;
        }
        .title {
            color: #FFFFFF;
            font-size: 24px;
        }
        .content {
            padding: 8px;
        }
        button {
            background-color: #4CAF50;
            color: #FFFFFF;
            padding: 8px;
            border-radius: 4px;
        }
    )";
    
    bundler.SetUIFromSource(ucml);
    bundler.SetStyleFromSource(css);
    
    BundleResult result = bundler.Bundle();
    return result.data;
}

// ============================================================================
// UCB LOADER TESTS
// ============================================================================

bool TestUCBLoader_LoadSimple() {
    UICompiler compiler;
    std::string ucml = "<div id=\"root\"><text>Hello</text></div>";
    
    auto result = compiler.Compile(ucml);
    if (!result.success) return false;
    
    UCBLoader loader;
    auto loadResult = loader.Load(result.data);
    
    if (!loadResult.success) {
        std::cout << "  Load error: " << loadResult.error << std::endl;
        return false;
    }
    
    if (loader.GetElements().size() != 2) {
        std::cout << "  Element count: " << loader.GetElements().size() << " (expected 2)" << std::endl;
        return false;
    }
    
    return true;
}

bool TestUCBLoader_GetById() {
    UICompiler compiler;
    std::string ucml = "<div id=\"app\"><button id=\"btn\">Click</button></div>";
    
    auto result = compiler.Compile(ucml);
    if (!result.success) return false;
    
    UCBLoader loader;
    loader.Load(result.data);
    
    const ElementNode* btn = loader.GetElementById("btn");
    if (!btn) {
        std::cout << "  Button not found by ID" << std::endl;
        return false;
    }
    
    if (btn->type != UCBElementType::Button) {
        std::cout << "  Wrong element type" << std::endl;
        return false;
    }
    
    return true;
}

bool TestUCBLoader_TreeTraversal() {
    UICompiler compiler;
    std::string ucml = R"(
        <div id="root">
            <div id="child1">
                <text>Text1</text>
            </div>
            <div id="child2">
                <text>Text2</text>
            </div>
        </div>
    )";
    
    auto result = compiler.Compile(ucml);
    if (!result.success) return false;
    
    UCBLoader loader;
    loader.Load(result.data);
    
    int count = 0;
    int maxDepth = 0;
    loader.TraverseDepthFirst([&count, &maxDepth](const ElementNode& node, int depth) {
        count++;
        if (depth > maxDepth) maxDepth = depth;
    });
    
    if (count != 5) {
        std::cout << "  Traverse count: " << count << " (expected 5)" << std::endl;
        return false;
    }
    
    if (maxDepth != 2) {
        std::cout << "  Max depth: " << maxDepth << " (expected 2)" << std::endl;
        return false;
    }
    
    return true;
}

bool TestUCBLoader_Properties() {
    UICompiler compiler;
    std::string ucml = "<input type=\"email\" placeholder=\"Enter email\" required />";
    
    auto result = compiler.Compile(ucml);
    if (!result.success) return false;
    
    UCBLoader loader;
    loader.Load(result.data);
    
    const ElementNode* input = loader.GetRootElement();
    if (!input) return false;
    
    std::string type = input->GetStringProperty(UCBPropertyId::Type);
    if (type != "email") {
        std::cout << "  Type: \"" << type << "\" (expected \"email\")" << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// UCS LOADER TESTS
// ============================================================================

bool TestUCSLoader_LoadSimple() {
    CSSCompiler compiler;
    std::string css = ".button { color: #FF0000; padding: 8px; }";
    
    auto result = compiler.Compile(css);
    if (!result.success) return false;
    
    UCSLoader loader;
    auto loadResult = loader.Load(result.data);
    
    if (!loadResult.success) {
        std::cout << "  Load error: " << loadResult.error << std::endl;
        return false;
    }
    
    if (loader.GetRuleCount() != 1) {
        std::cout << "  Rule count: " << loader.GetRuleCount() << " (expected 1)" << std::endl;
        return false;
    }
    
    return true;
}

bool TestUCSLoader_FindByClass() {
    CSSCompiler compiler;
    std::string css = R"(
        .header { color: blue; }
        .content { padding: 16px; }
        .header { font-size: 24px; }
    )";
    
    auto result = compiler.Compile(css);
    if (!result.success) return false;
    
    UCSLoader loader;
    loader.Load(result.data);
    
    auto headerRules = loader.FindRulesByClass("header");
    if (headerRules.size() != 2) {
        std::cout << "  Header rules: " << headerRules.size() << " (expected 2)" << std::endl;
        return false;
    }
    
    return true;
}

bool TestUCSLoader_ColorProperty() {
    CSSCompiler compiler;
    std::string css = ".red { color: #FF0000; background-color: #00FF00; }";
    
    auto result = compiler.Compile(css);
    if (!result.success) return false;
    
    UCSLoader loader;
    loader.Load(result.data);
    
    auto rules = loader.FindRulesByClass("red");
    if (rules.empty()) return false;
    
    const StyleValue* colorVal = rules[0]->GetProperty(CSSPropertyId::Color);
    if (!colorVal || !std::holds_alternative<StyleColor>(*colorVal)) {
        std::cout << "  Color property not found or wrong type" << std::endl;
        return false;
    }
    
    StyleColor color = std::get<StyleColor>(*colorVal);
    if (color.r != 255 || color.g != 0 || color.b != 0) {
        std::cout << "  Color: rgb(" << (int)color.r << "," << (int)color.g << "," << (int)color.b << ")" << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// RUNTIME TESTS
// ============================================================================

bool TestRuntime_LoadPackage() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    auto result = runtime.LoadPackage(packageData);
    
    if (!result.success) {
        std::cout << "  Load error: " << result.error << std::endl;
        return false;
    }
    
    if (result.elementCount == 0) {
        std::cout << "  No elements loaded" << std::endl;
        return false;
    }
    
    std::cout << "  Elements: " << result.elementCount 
              << " | Rules: " << result.styleRuleCount << std::endl;
    
    return true;
}

bool TestRuntime_GetElements() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    
    RuntimeElement* app = runtime.GetElementById("app");
    if (!app) {
        std::cout << "  #app not found" << std::endl;
        return false;
    }
    
    RuntimeElement* btn = runtime.GetElementById("btn1");
    if (!btn) {
        std::cout << "  #btn1 not found" << std::endl;
        return false;
    }
    
    if (btn->type != UCBElementType::Button) {
        std::cout << "  Wrong button type" << std::endl;
        return false;
    }
    
    return true;
}

bool TestRuntime_StyleComputation() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    
    RuntimeElement* app = runtime.GetElementById("app");
    if (!app) return false;
    
    // Check computed style
    const auto& style = app->computedStyle;
    
    if (style.paddingTop != 16.0f) {
        std::cout << "  Padding: " << style.paddingTop << " (expected 16)" << std::endl;
        return false;
    }
    
    return true;
}

bool TestRuntime_Layout() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    
    runtime.PerformLayout(800, 600);
    
    RuntimeElement* root = runtime.GetRootElement();
    if (!root) return false;
    
    if (root->layoutWidth != 800) {
        std::cout << "  Root width: " << root->layoutWidth << " (expected 800)" << std::endl;
        return false;
    }
    
    return true;
}

bool TestRuntime_EventHandlers() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    
    RuntimeElement* btn = runtime.GetElementById("btn1");
    if (!btn) return false;
    
    if (!btn->HasEventHandler(UCBPropertyId::OnClick)) {
        std::cout << "  Button has no onclick handler" << std::endl;
        return false;
    }
    
    std::string handler = btn->GetEventHandler(UCBPropertyId::OnClick);
    if (handler != "handleClick") {
        std::cout << "  Handler: \"" << handler << "\" (expected \"handleClick\")" << std::endl;
        return false;
    }
    
    return true;
}

bool TestRuntime_EventDispatch() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    
    bool eventReceived = false;
    std::string receivedHandler;
    
    runtime.SetEventCallback([&](uint16_t elementId, const std::string& handler) {
        eventReceived = true;
        receivedHandler = handler;
    });
    
    RuntimeElement* btn = runtime.GetElementById("btn1");
    if (!btn) return false;
    
    runtime.DispatchEvent(btn->elementId, UCBPropertyId::OnClick);
    
    if (!eventReceived) {
        std::cout << "  Event not received" << std::endl;
        return false;
    }
    
    if (receivedHandler != "handleClick") {
        std::cout << "  Received handler: \"" << receivedHandler << "\"" << std::endl;
        return false;
    }
    
    return true;
}

bool TestRuntime_HitTest() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    runtime.PerformLayout(800, 600);
    
    // Hit test inside root
    RuntimeElement* hit = runtime.HitTest(100, 100);
    if (!hit) {
        std::cout << "  No element hit at (100, 100)" << std::endl;
        return false;
    }
    
    // Hit test outside
    hit = runtime.HitTest(-10, -10);
    if (hit) {
        std::cout << "  Unexpected hit at (-10, -10)" << std::endl;
        return false;
    }
    
    return true;
}

bool TestRuntime_StateManagement() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    
    RuntimeElement* btn = runtime.GetElementById("btn1");
    if (!btn) return false;
    
    // Test state changes
    runtime.SetElementHovered(btn->elementId, true);
    if (!btn->hovered) {
        std::cout << "  Hovered state not set" << std::endl;
        return false;
    }
    
    runtime.SetElementPressed(btn->elementId, true);
    if (!btn->pressed) {
        std::cout << "  Pressed state not set" << std::endl;
        return false;
    }
    
    runtime.SetElementFocused(btn->elementId, true);
    if (!btn->focused) {
        std::cout << "  Focused state not set" << std::endl;
        return false;
    }
    
    return true;
}

bool TestRuntime_Render() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    runtime.PerformLayout(800, 600);
    
    int renderCount = 0;
    runtime.Render([&renderCount](const RuntimeElement& elem) {
        renderCount++;
    });
    
    if (renderCount == 0) {
        std::cout << "  No elements rendered" << std::endl;
        return false;
    }
    
    std::cout << "  Rendered " << renderCount << " elements" << std::endl;
    
    return true;
}

bool TestRuntime_DumpTree() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    runtime.PerformLayout(800, 600);
    
    std::string dump = runtime.DumpElementTree();
    
    if (dump.empty()) {
        std::cout << "  Empty tree dump" << std::endl;
        return false;
    }
    
    // Check for expected content
    if (dump.find("#app") == std::string::npos) {
        std::cout << "  #app not in dump" << std::endl;
        return false;
    }
    
    std::cout << "\n" << dump.substr(0, 500) << "...\n" << std::endl;
    
    return true;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "UltraWeb Runtime Loader Test Suite\n";
    PrintSeparator();
    
    int passed = 0;
    int failed = 0;
    
    // UCB Loader Tests
    std::cout << "\n[UCB Loader Tests]\n";
    
    if (TestUCBLoader_LoadSimple()) { passed++; PrintTestResult("Load Simple", true); }
    else { failed++; PrintTestResult("Load Simple", false); }
    
    if (TestUCBLoader_GetById()) { passed++; PrintTestResult("Get By ID", true); }
    else { failed++; PrintTestResult("Get By ID", false); }
    
    if (TestUCBLoader_TreeTraversal()) { passed++; PrintTestResult("Tree Traversal", true); }
    else { failed++; PrintTestResult("Tree Traversal", false); }
    
    if (TestUCBLoader_Properties()) { passed++; PrintTestResult("Properties", true); }
    else { failed++; PrintTestResult("Properties", false); }
    
    // UCS Loader Tests
    std::cout << "\n[UCS Loader Tests]\n";
    
    if (TestUCSLoader_LoadSimple()) { passed++; PrintTestResult("Load Simple", true); }
    else { failed++; PrintTestResult("Load Simple", false); }
    
    if (TestUCSLoader_FindByClass()) { passed++; PrintTestResult("Find By Class", true); }
    else { failed++; PrintTestResult("Find By Class", false); }
    
    if (TestUCSLoader_ColorProperty()) { passed++; PrintTestResult("Color Property", true); }
    else { failed++; PrintTestResult("Color Property", false); }
    
    // Runtime Tests
    std::cout << "\n[Runtime Tests]\n";
    
    if (TestRuntime_LoadPackage()) { passed++; PrintTestResult("Load Package", true); }
    else { failed++; PrintTestResult("Load Package", false); }
    
    if (TestRuntime_GetElements()) { passed++; PrintTestResult("Get Elements", true); }
    else { failed++; PrintTestResult("Get Elements", false); }
    
    if (TestRuntime_StyleComputation()) { passed++; PrintTestResult("Style Computation", true); }
    else { failed++; PrintTestResult("Style Computation", false); }
    
    if (TestRuntime_Layout()) { passed++; PrintTestResult("Layout", true); }
    else { failed++; PrintTestResult("Layout", false); }
    
    if (TestRuntime_EventHandlers()) { passed++; PrintTestResult("Event Handlers", true); }
    else { failed++; PrintTestResult("Event Handlers", false); }
    
    if (TestRuntime_EventDispatch()) { passed++; PrintTestResult("Event Dispatch", true); }
    else { failed++; PrintTestResult("Event Dispatch", false); }
    
    if (TestRuntime_HitTest()) { passed++; PrintTestResult("Hit Test", true); }
    else { failed++; PrintTestResult("Hit Test", false); }
    
    if (TestRuntime_StateManagement()) { passed++; PrintTestResult("State Management", true); }
    else { failed++; PrintTestResult("State Management", false); }
    
    if (TestRuntime_Render()) { passed++; PrintTestResult("Render", true); }
    else { failed++; PrintTestResult("Render", false); }
    
    if (TestRuntime_DumpTree()) { passed++; PrintTestResult("Dump Tree", true); }
    else { failed++; PrintTestResult("Dump Tree", false); }
    
    // Summary
    std::cout << "\n";
    PrintSeparator();
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    PrintSeparator();
    std::cout << "\n";
    
    return failed > 0 ? 1 : 0;
}
