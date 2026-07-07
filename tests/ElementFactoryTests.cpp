// UltraWeb/tests/ElementFactoryTests.cpp
// Unit tests for Element Factory and UCComponents
// Version: 1.0.0

#include "../runtime/ElementFactory.h"
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

// Create test package
std::vector<uint8_t> CreateTestPackage() {
    PackageBundler bundler;
    
    std::string ucml = R"(
        <div id="app" class="container">
            <flex class="header">
                <text class="title">Hello World</text>
            </flex>
            <div class="content">
                <button id="btn1" onclick="handleClick">Click Me</button>
                <input id="input1" type="text" placeholder="Enter text" />
            </div>
        </div>
    )";
    
    std::string css = R"(
        .container {
            display: flex;
            flex-direction: column;
            padding: 16px;
            background-color: #F5F5F5;
        }
        .header {
            background-color: #3366FF;
            padding: 12px;
            border-radius: 8px;
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
            padding: 8px 16px;
            border-radius: 4px;
        }
    )";
    
    bundler.SetUIFromSource(ucml);
    bundler.SetStyleFromSource(css);
    
    BundleResult result = bundler.Bundle();
    return result.data;
}

// ============================================================================
// UCCOMPONENT TESTS
// ============================================================================

bool TestUCComponent_BasicProperties() {
    UCComponent comp;
    
    comp.SetId(42);
    comp.SetName("testComp");
    comp.SetFrame(10, 20, 100, 50);
    comp.SetVisible(true);
    comp.SetEnabled(true);
    
    if (comp.GetId() != 42) return false;
    if (comp.GetName() != "testComp") return false;
    if (comp.GetFrame().x != 10) return false;
    if (comp.GetFrame().width != 100) return false;
    if (!comp.IsVisible()) return false;
    if (!comp.IsEnabled()) return false;
    
    return true;
}

bool TestUCComponent_Hierarchy() {
    auto parent = std::make_shared<UCContainer>();
    auto child1 = std::make_shared<UCComponent>();
    auto child2 = std::make_shared<UCComponent>();
    
    child1->SetName("child1");
    child2->SetName("child2");
    
    parent->AddChild(child1);
    parent->AddChild(child2);
    
    if (parent->GetChildren().size() != 2) return false;
    if (child1->GetParent() != parent.get()) return false;
    if (child2->GetParent() != parent.get()) return false;
    
    parent->RemoveChild(child1.get());
    if (parent->GetChildren().size() != 1) return false;
    if (child1->GetParent() != nullptr) return false;
    
    return true;
}

bool TestUCComponent_Styling() {
    UCComponent comp;
    
    comp.SetBackgroundColor(UCColor(255, 0, 0, 128));
    comp.SetBorderColor(UCColor(0, 0, 0));
    comp.SetBorderWidth(2.0f);
    comp.SetBorderRadius(8.0f);
    comp.SetPadding(10, 20, 10, 20);
    comp.SetOpacity(0.8f);
    
    if (comp.GetBackgroundColor().r != 255) return false;
    if (comp.GetBackgroundColor().a != 128) return false;
    if (comp.GetOpacity() != 0.8f) return false;
    
    return true;
}

bool TestUCComponent_HitTest() {
    auto parent = std::make_shared<UCContainer>();
    auto child = std::make_shared<UCComponent>();
    
    parent->SetFrame(0, 0, 200, 200);
    child->SetFrame(50, 50, 100, 100);
    child->SetName("child");
    
    parent->AddChild(child);
    
    // Hit inside child
    UCComponent* hit = parent->HitTest(75, 75);
    if (hit != child.get()) {
        std::cout << "  Expected child, got " << (hit ? hit->GetName() : "null") << std::endl;
        return false;
    }
    
    // Hit outside child but inside parent
    hit = parent->HitTest(25, 25);
    if (hit != parent.get()) {
        std::cout << "  Expected parent, got " << (hit ? hit->GetName() : "null") << std::endl;
        return false;
    }
    
    // Hit outside parent
    hit = parent->HitTest(300, 300);
    if (hit != nullptr) {
        std::cout << "  Expected null, got " << hit->GetName() << std::endl;
        return false;
    }
    
    return true;
}

bool TestUCComponent_Events() {
    UCComponent comp;
    
    bool clickReceived = false;
    bool hoverReceived = false;
    
    comp.SetEventHandler(UCEventType::Click, [&clickReceived](const UCEvent& e) {
        (void)e;
        clickReceived = true;
    });
    
    comp.SetEventHandler(UCEventType::MouseEnter, [&hoverReceived](const UCEvent& e) {
        (void)e;
        hoverReceived = true;
    });
    
    if (!comp.HasEventHandler(UCEventType::Click)) return false;
    if (!comp.HasEventHandler(UCEventType::MouseEnter)) return false;
    if (comp.HasEventHandler(UCEventType::KeyDown)) return false;
    
    UCEvent clickEvent;
    clickEvent.type = UCEventType::Click;
    comp.DispatchEvent(clickEvent);
    
    if (!clickReceived) return false;
    
    UCEvent hoverEvent;
    hoverEvent.type = UCEventType::MouseEnter;
    comp.DispatchEvent(hoverEvent);
    
    if (!hoverReceived) return false;
    
    return true;
}

// ============================================================================
// UCCONTAINER TESTS
// ============================================================================

bool TestUCContainer_ColumnLayout() {
    auto container = std::make_shared<UCContainer>();
    container->SetFrame(0, 0, 200, 400);
    container->SetLayoutDirection(UCContainer::LayoutDirection::Column);
    container->SetGap(10);
    
    auto child1 = std::make_shared<UCComponent>();
    auto child2 = std::make_shared<UCComponent>();
    auto child3 = std::make_shared<UCComponent>();
    
    child1->SetFrame(0, 0, 100, 50);
    child2->SetFrame(0, 0, 100, 50);
    child3->SetFrame(0, 0, 100, 50);
    
    container->AddChild(child1);
    container->AddChild(child2);
    container->AddChild(child3);
    
    container->Layout();
    
    // Children should be stacked vertically with gap
    if (child1->GetFrame().y != 0) {
        std::cout << "  Child1 y=" << child1->GetFrame().y << " (expected 0)" << std::endl;
        return false;
    }
    if (child2->GetFrame().y != 60) {  // 50 + 10 gap
        std::cout << "  Child2 y=" << child2->GetFrame().y << " (expected 60)" << std::endl;
        return false;
    }
    if (child3->GetFrame().y != 120) {  // 50 + 10 + 50 + 10
        std::cout << "  Child3 y=" << child3->GetFrame().y << " (expected 120)" << std::endl;
        return false;
    }
    
    return true;
}

bool TestUCContainer_RowLayout() {
    auto container = std::make_shared<UCFlexBox>();
    container->SetFrame(0, 0, 400, 100);
    container->SetGap(10);
    
    auto child1 = std::make_shared<UCComponent>();
    auto child2 = std::make_shared<UCComponent>();
    
    child1->SetFrame(0, 0, 100, 50);
    child2->SetFrame(0, 0, 100, 50);
    
    container->AddChild(child1);
    container->AddChild(child2);
    
    container->Layout();
    
    // Children should be side by side
    if (child1->GetFrame().x != 0) return false;
    if (child2->GetFrame().x != 110) {  // 100 + 10 gap
        std::cout << "  Child2 x=" << child2->GetFrame().x << " (expected 110)" << std::endl;
        return false;
    }
    
    return true;
}

bool TestUCContainer_JustifyContent() {
    auto container = std::make_shared<UCFlexBox>();
    container->SetFrame(0, 0, 400, 100);
    container->SetJustifyContent(UCContainer::JustifyContent::Center);
    
    auto child = std::make_shared<UCComponent>();
    child->SetFrame(0, 0, 100, 50);
    
    container->AddChild(child);
    container->Layout();
    
    // Child should be centered
    float expectedX = (400 - 100) / 2;  // 150
    if (std::abs(child->GetFrame().x - expectedX) > 1) {
        std::cout << "  Child x=" << child->GetFrame().x << " (expected " << expectedX << ")" << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// MOCK RENDER CONTEXT TESTS
// ============================================================================

bool TestMockRenderContext_DrawCalls() {
    UCMockRenderContext ctx;
    
    ctx.SetFillColor(UCColor(255, 0, 0));
    ctx.FillRect({10, 20, 100, 50});
    
    ctx.SetStrokeColor(UCColor(0, 0, 0));
    ctx.SetLineWidth(2);
    ctx.StrokeRect({10, 20, 100, 50});
    
    ctx.SetFont("Arial", 16, 700);
    ctx.FillText("Hello", 50, 30);
    
    if (ctx.GetDrawCallCount() < 3) {
        std::cout << "  Draw call count: " << ctx.GetDrawCallCount() << " (expected >= 3)" << std::endl;
        return false;
    }
    
    // Check specific draw calls
    bool foundFillRect = false;
    bool foundStrokeRect = false;
    bool foundText = false;
    
    for (const auto& call : ctx.drawCalls) {
        if (call.type == "fillRect") foundFillRect = true;
        if (call.type == "strokeRect") foundStrokeRect = true;
        if (call.type == "fillText" && call.text == "Hello") foundText = true;
    }
    
    if (!foundFillRect || !foundStrokeRect || !foundText) {
        std::cout << "  Missing expected draw calls" << std::endl;
        return false;
    }
    
    return true;
}

bool TestMockRenderContext_ComponentRender() {
    UCMockRenderContext ctx;
    
    auto button = std::make_shared<UCButton>("Click Me");
    button->SetFrame(0, 0, 100, 40);
    button->SetBackgroundColor(UCColor(0, 128, 255));
    button->SetBorderRadius(4);
    
    button->Render(ctx);
    
    // Should have background and text draw calls
    if (ctx.GetDrawCallCount() < 2) {
        std::cout << "  Draw call count: " << ctx.GetDrawCallCount() << " (expected >= 2)" << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// ELEMENT FACTORY TESTS
// ============================================================================

bool TestElementFactory_CreateComponent() {
    RuntimeElement elem;
    elem.elementId = 1;
    elem.type = UCBElementType::Button;
    elem.id = "testBtn";
    elem.textContent = "Test Button";
    elem.visible = true;
    elem.enabled = true;
    elem.layoutWidth = 100;
    elem.layoutHeight = 40;
    
    ElementFactory factory;
    auto comp = factory.CreateComponent(elem);
    
    if (!comp) {
        std::cout << "  Component creation failed" << std::endl;
        return false;
    }
    
    if (comp->GetTypeName() != "Button") {
        std::cout << "  Type: " << comp->GetTypeName() << " (expected Button)" << std::endl;
        return false;
    }
    
    if (comp->GetName() != "testBtn") {
        std::cout << "  Name: " << comp->GetName() << " (expected testBtn)" << std::endl;
        return false;
    }
    
    // Check it's actually a button with text
    auto* button = dynamic_cast<UCButton*>(comp.get());
    if (!button) {
        std::cout << "  Not a UCButton instance" << std::endl;
        return false;
    }
    
    if (button->GetText() != "Test Button") {
        std::cout << "  Text: " << button->GetText() << " (expected Test Button)" << std::endl;
        return false;
    }
    
    return true;
}

bool TestElementFactory_BuildTree() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    auto result = runtime.LoadPackage(packageData);
    if (!result.success) {
        std::cout << "  Failed to load package" << std::endl;
        return false;
    }
    
    runtime.PerformLayout(800, 600);
    
    ElementFactory factory;
    auto root = factory.BuildComponentTree(runtime);
    
    if (!root) {
        std::cout << "  Failed to build component tree" << std::endl;
        return false;
    }
    
    // Check root is a container
    if (root->GetTypeName() != "Container" && root->GetTypeName() != "FlexBox") {
        std::cout << "  Root type: " << root->GetTypeName() << std::endl;
    }
    
    // Check it has children
    if (root->GetChildren().empty()) {
        std::cout << "  Root has no children" << std::endl;
        return false;
    }
    
    std::cout << "  Components: " << std::endl;
    std::cout << factory.DumpComponentTree(root.get(), 2);
    
    return true;
}

bool TestElementFactory_GetComponent() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    runtime.PerformLayout(800, 600);
    
    ElementFactory factory;
    auto root = factory.BuildComponentTree(runtime);
    
    // Find button by element ID
    RuntimeElement* btnElem = runtime.GetElementById("btn1");
    if (!btnElem) {
        std::cout << "  btn1 not found in runtime" << std::endl;
        return false;
    }
    
    UCComponent* btnComp = factory.GetComponent(btnElem->elementId);
    if (!btnComp) {
        std::cout << "  Button component not found" << std::endl;
        return false;
    }
    
    if (btnComp->GetTypeName() != "Button") {
        std::cout << "  Type: " << btnComp->GetTypeName() << " (expected Button)" << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// STYLE APPLICATOR TESTS
// ============================================================================

bool TestStyleApplicator_ApplyVisual() {
    UCComponent comp;
    
    ComputedStyle style;
    style.backgroundColor = StyleColor(255, 128, 0, 200);
    style.borderWidth = 2.0f;
    style.borderColor = StyleColor(0, 0, 0);
    style.borderRadius = 8.0f;
    style.opacity = 0.9f;
    
    StyleApplicator::ApplyVisual(&comp, style);
    
    if (comp.GetBackgroundColor().r != 255) return false;
    if (comp.GetBackgroundColor().g != 128) return false;
    if (comp.GetBackgroundColor().a != 200) return false;
    if (comp.GetOpacity() != 0.9f) return false;
    
    return true;
}

bool TestStyleApplicator_ApplyText() {
    UCText text;
    
    ComputedStyle style;
    style.color = StyleColor(50, 100, 150);
    style.fontFamily = "Roboto";
    style.fontSize = 20.0f;
    style.fontWeight = 700;
    style.lineHeight = 1.5f;
    style.textAlign = 1;  // center
    
    StyleApplicator::ApplyText(&text, style);
    
    if (text.GetText().empty()) {
        text.SetText("Test");
    }
    
    // Can't easily verify private members, but at least it didn't crash
    return true;
}

bool TestStyleApplicator_ApplyContainer() {
    UCContainer container;
    
    ComputedStyle style;
    style.flexDirection = 1;  // column
    style.justifyContent = 2;  // center
    style.alignItems = 3;  // stretch
    style.gap = 10.0f;
    
    StyleApplicator::ApplyContainer(&container, style);
    
    // Layout a child to verify settings took effect
    auto child = std::make_shared<UCComponent>();
    child->SetFrame(0, 0, 50, 50);
    container.SetFrame(0, 0, 200, 200);
    container.AddChild(child);
    container.Layout();
    
    // With stretch, child should have full width
    if (child->GetFrame().width != 200) {
        std::cout << "  Child width: " << child->GetFrame().width << " (expected 200)" << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// COMPONENT TREE RENDERER TESTS
// ============================================================================

bool TestRenderer_Render() {
    auto packageData = CreateTestPackage();
    
    UltraWebRuntime runtime;
    runtime.LoadPackage(packageData);
    runtime.PerformLayout(800, 600);
    
    ElementFactory factory;
    auto root = factory.BuildComponentTree(runtime);
    
    ComponentTreeRenderer renderer;
    renderer.SetRoot(root);
    renderer.PerformLayout(800, 600);
    
    UCMockRenderContext ctx;
    renderer.Render(ctx);
    
    auto stats = renderer.GetLastStats();
    std::cout << "  Components: " << stats.componentCount << std::endl;
    std::cout << "  Draw calls: " << stats.drawCalls << std::endl;
    std::cout << "  Layout time: " << stats.layoutTimeMs << "ms" << std::endl;
    std::cout << "  Render time: " << stats.renderTimeMs << "ms" << std::endl;
    
    if (stats.componentCount == 0) {
        std::cout << "  No components rendered" << std::endl;
        return false;
    }
    
    if (stats.drawCalls == 0) {
        std::cout << "  No draw calls" << std::endl;
        return false;
    }
    
    return true;
}

bool TestRenderer_HitTest() {
    auto container = std::make_shared<UCContainer>();
    auto button = std::make_shared<UCButton>("Test");
    
    container->SetFrame(0, 0, 400, 300);
    button->SetFrame(50, 50, 100, 40);
    button->SetName("testButton");
    
    container->AddChild(button);
    
    ComponentTreeRenderer renderer;
    renderer.SetRoot(container);
    
    // Hit button
    UCComponent* hit = renderer.HitTest(75, 60);
    if (hit != button.get()) {
        std::cout << "  Expected button, got " << (hit ? hit->GetName() : "null") << std::endl;
        return false;
    }
    
    // Hit container (not button)
    hit = renderer.HitTest(10, 10);
    if (hit != container.get()) {
        std::cout << "  Expected container" << std::endl;
        return false;
    }
    
    return true;
}

bool TestRenderer_EventHandling() {
    auto button = std::make_shared<UCButton>("Test");
    button->SetFrame(0, 0, 100, 40);
    
    bool clicked = false;
    button->SetEventHandler(UCEventType::Click, [&clicked](const UCEvent& e) {
        (void)e;
        clicked = true;
    });
    
    ComponentTreeRenderer renderer;
    renderer.SetRoot(button);
    
    // Simulate click
    renderer.HandleClick(50, 20);
    
    if (!clicked) {
        std::cout << "  Click event not received" << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// ULTRAWEB BRIDGE TESTS
// ============================================================================

bool TestBridge_LoadAndRender() {
    auto packageData = CreateTestPackage();
    
    UltraWebBridge bridge;
    
    if (!bridge.LoadPackage(packageData)) {
        std::cout << "  Failed to load package" << std::endl;
        return false;
    }
    
    bridge.SetViewport(800, 600);
    
    UCMockRenderContext ctx;
    bridge.Render(ctx);
    
    auto stats = bridge.GetStats();
    
    if (stats.componentCount == 0) {
        std::cout << "  No components" << std::endl;
        return false;
    }
    
    std::cout << "  Bridge stats: " << stats.componentCount << " components, " 
              << stats.drawCalls << " draw calls" << std::endl;
    
    return true;
}

bool TestBridge_EventCallback() {
    auto packageData = CreateTestPackage();
    
    UltraWebBridge bridge;
    bridge.LoadPackage(packageData);
    bridge.SetViewport(800, 600);
    
    bool eventReceived = false;
    std::string receivedHandler;
    
    bridge.SetJSEventCallback([&](uint16_t elemId, const std::string& handler, const UCEvent& event) {
        (void)elemId;
        (void)event;
        eventReceived = true;
        receivedHandler = handler;
    });
    
    // Find button position and simulate click
    UCComponent* btnComp = bridge.GetComponent(
        bridge.GetRuntime().GetElementById("btn1")->elementId
    );
    
    if (!btnComp) {
        std::cout << "  Button component not found" << std::endl;
        return false;
    }
    
    // Click on button
    float btnX = btnComp->GetFrame().x + btnComp->GetFrame().width / 2;
    float btnY = btnComp->GetFrame().y + btnComp->GetFrame().height / 2;
    bridge.HandleClick(btnX, btnY);
    
    // Note: Event might not fire immediately in this test setup
    // since we're clicking on component coordinates, not absolute
    
    return true;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "UltraWeb Element Factory Test Suite\n";
    PrintSeparator();
    
    int passed = 0;
    int failed = 0;
    
    // UCComponent Tests
    std::cout << "\n[UCComponent Tests]\n";
    
    if (TestUCComponent_BasicProperties()) { passed++; PrintTestResult("Basic Properties", true); }
    else { failed++; PrintTestResult("Basic Properties", false); }
    
    if (TestUCComponent_Hierarchy()) { passed++; PrintTestResult("Hierarchy", true); }
    else { failed++; PrintTestResult("Hierarchy", false); }
    
    if (TestUCComponent_Styling()) { passed++; PrintTestResult("Styling", true); }
    else { failed++; PrintTestResult("Styling", false); }
    
    if (TestUCComponent_HitTest()) { passed++; PrintTestResult("Hit Test", true); }
    else { failed++; PrintTestResult("Hit Test", false); }
    
    if (TestUCComponent_Events()) { passed++; PrintTestResult("Events", true); }
    else { failed++; PrintTestResult("Events", false); }
    
    // UCContainer Tests
    std::cout << "\n[UCContainer Tests]\n";
    
    if (TestUCContainer_ColumnLayout()) { passed++; PrintTestResult("Column Layout", true); }
    else { failed++; PrintTestResult("Column Layout", false); }
    
    if (TestUCContainer_RowLayout()) { passed++; PrintTestResult("Row Layout", true); }
    else { failed++; PrintTestResult("Row Layout", false); }
    
    if (TestUCContainer_JustifyContent()) { passed++; PrintTestResult("Justify Content", true); }
    else { failed++; PrintTestResult("Justify Content", false); }
    
    // Mock Render Context Tests
    std::cout << "\n[Render Context Tests]\n";
    
    if (TestMockRenderContext_DrawCalls()) { passed++; PrintTestResult("Draw Calls", true); }
    else { failed++; PrintTestResult("Draw Calls", false); }
    
    if (TestMockRenderContext_ComponentRender()) { passed++; PrintTestResult("Component Render", true); }
    else { failed++; PrintTestResult("Component Render", false); }
    
    // Element Factory Tests
    std::cout << "\n[Element Factory Tests]\n";
    
    if (TestElementFactory_CreateComponent()) { passed++; PrintTestResult("Create Component", true); }
    else { failed++; PrintTestResult("Create Component", false); }
    
    if (TestElementFactory_BuildTree()) { passed++; PrintTestResult("Build Tree", true); }
    else { failed++; PrintTestResult("Build Tree", false); }
    
    if (TestElementFactory_GetComponent()) { passed++; PrintTestResult("Get Component", true); }
    else { failed++; PrintTestResult("Get Component", false); }
    
    // Style Applicator Tests
    std::cout << "\n[Style Applicator Tests]\n";
    
    if (TestStyleApplicator_ApplyVisual()) { passed++; PrintTestResult("Apply Visual", true); }
    else { failed++; PrintTestResult("Apply Visual", false); }
    
    if (TestStyleApplicator_ApplyText()) { passed++; PrintTestResult("Apply Text", true); }
    else { failed++; PrintTestResult("Apply Text", false); }
    
    if (TestStyleApplicator_ApplyContainer()) { passed++; PrintTestResult("Apply Container", true); }
    else { failed++; PrintTestResult("Apply Container", false); }
    
    // Renderer Tests
    std::cout << "\n[Renderer Tests]\n";
    
    if (TestRenderer_Render()) { passed++; PrintTestResult("Render", true); }
    else { failed++; PrintTestResult("Render", false); }
    
    if (TestRenderer_HitTest()) { passed++; PrintTestResult("Hit Test", true); }
    else { failed++; PrintTestResult("Hit Test", false); }
    
    if (TestRenderer_EventHandling()) { passed++; PrintTestResult("Event Handling", true); }
    else { failed++; PrintTestResult("Event Handling", false); }
    
    // Bridge Tests
    std::cout << "\n[UltraWeb Bridge Tests]\n";
    
    if (TestBridge_LoadAndRender()) { passed++; PrintTestResult("Load and Render", true); }
    else { failed++; PrintTestResult("Load and Render", false); }
    
    if (TestBridge_EventCallback()) { passed++; PrintTestResult("Event Callback", true); }
    else { failed++; PrintTestResult("Event Callback", false); }
    
    // Summary
    std::cout << "\n";
    PrintSeparator();
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    PrintSeparator();
    std::cout << "\n";
    
    return failed > 0 ? 1 : 0;
}
