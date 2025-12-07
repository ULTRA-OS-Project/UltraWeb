// UltraWeb/tests/EventSystemTests.cpp
// Unit tests for Event Binding System
// Version: 1.0.0

#include "../runtime/EventSystem.h"
#include "../runtime/UCComponents.h"
#include <iostream>
#include <cassert>
#include <cmath>

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

// ============================================================================
// EVENT CREATION TESTS
// ============================================================================

bool TestEvent_BasicProperties() {
    Event event(EventType::Click);
    
    if (event.GetType() != EventType::Click) return false;
    if (event.GetTypeString() != "click") return false;
    if (!event.Bubbles()) return false;
    if (!event.IsCancelable()) return false;
    if (!event.IsTrusted()) return false;
    if (event.IsDefaultPrevented()) return false;
    if (event.IsPropagationStopped()) return false;
    if (event.GetTimeStamp() <= 0) return false;
    
    return true;
}

bool TestEvent_PropagationControl() {
    Event event(EventType::Click);
    
    if (event.IsPropagationStopped()) return false;
    
    event.StopPropagation();
    if (!event.IsPropagationStopped()) return false;
    if (event.IsImmediatePropagationStopped()) return false;
    
    Event event2(EventType::Click);
    event2.StopImmediatePropagation();
    if (!event2.IsPropagationStopped()) return false;
    if (!event2.IsImmediatePropagationStopped()) return false;
    
    return true;
}

bool TestEvent_PreventDefault() {
    Event event(EventType::Click);
    
    if (event.IsDefaultPrevented()) return false;
    
    event.PreventDefault();
    if (!event.IsDefaultPrevented()) return false;
    
    return true;
}

bool TestEvent_NonBubblingEvents() {
    // Focus/blur and mouseenter/leave don't bubble
    FocusEvent focusEvent(EventType::Focus);
    if (focusEvent.Bubbles()) return false;
    
    FocusEvent blurEvent(EventType::Blur);
    if (blurEvent.Bubbles()) return false;
    
    MouseEvent enterEvent(EventType::MouseEnter);
    if (enterEvent.Bubbles()) return false;
    
    MouseEvent leaveEvent(EventType::MouseLeave);
    if (leaveEvent.Bubbles()) return false;
    
    // Click should bubble
    MouseEvent clickEvent(EventType::Click);
    if (!clickEvent.Bubbles()) return false;
    
    return true;
}

bool TestEvent_ToJSON() {
    Event event(EventType::Click);
    std::string json = event.ToJSON();
    
    // Check JSON contains expected fields
    if (json.find("\"type\":\"click\"") == std::string::npos) {
        std::cout << "  Missing type field: " << json << std::endl;
        return false;
    }
    if (json.find("\"bubbles\":true") == std::string::npos) {
        std::cout << "  Missing bubbles field: " << json << std::endl;
        return false;
    }
    if (json.find("\"timeStamp\":") == std::string::npos) {
        std::cout << "  Missing timeStamp field: " << json << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// MOUSE EVENT TESTS
// ============================================================================

bool TestMouseEvent_Position() {
    MouseEvent event(EventType::Click);
    
    event.SetClientPos(100.5f, 200.5f);
    event.SetScreenPos(110.0f, 210.0f);
    event.SetOffsetPos(10.0f, 20.0f);
    event.SetPagePos(100.5f, 200.5f);
    
    if (std::abs(event.GetClientX() - 100.5f) > 0.001f) return false;
    if (std::abs(event.GetClientY() - 200.5f) > 0.001f) return false;
    if (std::abs(event.GetScreenX() - 110.0f) > 0.001f) return false;
    if (std::abs(event.GetOffsetX() - 10.0f) > 0.001f) return false;
    
    return true;
}

bool TestMouseEvent_Button() {
    MouseEvent event(EventType::MouseDown);
    
    event.SetButton(2);  // Right button
    event.SetButtons(4); // Right button mask
    
    if (event.GetButton() != 2) return false;
    if (event.GetButtons() != 4) return false;
    
    return true;
}

bool TestMouseEvent_Modifiers() {
    MouseEvent event(EventType::Click);
    
    event.SetModifiers(true, false, true, false);
    
    if (!event.GetAltKey()) return false;
    if (event.GetCtrlKey()) return false;
    if (!event.GetShiftKey()) return false;
    if (event.GetMetaKey()) return false;
    
    return true;
}

bool TestMouseEvent_ToJSON() {
    MouseEvent event(EventType::Click);
    event.SetClientPos(150.0f, 250.0f);
    event.SetButton(0);
    event.SetModifiers(false, true, false, false);
    
    std::string json = event.ToJSON();
    
    if (json.find("\"clientX\":150") == std::string::npos) {
        std::cout << "  Missing clientX: " << json << std::endl;
        return false;
    }
    if (json.find("\"ctrlKey\":true") == std::string::npos) {
        std::cout << "  Missing ctrlKey: " << json << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// KEYBOARD EVENT TESTS
// ============================================================================

bool TestKeyboardEvent_KeyInfo() {
    KeyboardEvent event(EventType::KeyDown);
    
    event.SetKey("Enter");
    event.SetCode("Enter");
    event.SetKeyCode(13);
    event.SetCharCode(13);
    
    if (event.GetKey() != "Enter") return false;
    if (event.GetCode() != "Enter") return false;
    if (event.GetKeyCode() != 13) return false;
    if (event.GetCharCode() != 13) return false;
    
    return true;
}

bool TestKeyboardEvent_Location() {
    KeyboardEvent event(EventType::KeyDown);
    
    event.SetLocation(KeyboardEvent::Location::Numpad);
    
    if (event.GetLocation() != KeyboardEvent::Location::Numpad) return false;
    
    return true;
}

bool TestKeyboardEvent_RepeatAndComposing() {
    KeyboardEvent event(EventType::KeyDown);
    
    event.SetRepeat(true);
    event.SetComposing(true);
    
    if (!event.IsRepeat()) return false;
    if (!event.IsComposing()) return false;
    
    return true;
}

bool TestKeyboardEvent_ToJSON() {
    KeyboardEvent event(EventType::KeyDown);
    event.SetKey("a");
    event.SetKeyCode(65);
    event.SetModifiers(false, false, true, false);  // Shift
    
    std::string json = event.ToJSON();
    
    if (json.find("\"key\":\"a\"") == std::string::npos) {
        std::cout << "  Missing key: " << json << std::endl;
        return false;
    }
    if (json.find("\"keyCode\":65") == std::string::npos) {
        std::cout << "  Missing keyCode: " << json << std::endl;
        return false;
    }
    if (json.find("\"shiftKey\":true") == std::string::npos) {
        std::cout << "  Missing shiftKey: " << json << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// INPUT EVENT TESTS
// ============================================================================

bool TestInputEvent_Data() {
    InputEvent event(EventType::Input);
    
    event.SetData("Hello");
    event.SetInputType("insertText");
    
    if (event.GetData() != "Hello") return false;
    if (event.GetInputType() != "insertText") return false;
    
    return true;
}

bool TestInputEvent_ToJSON() {
    InputEvent event(EventType::Input);
    event.SetData("test input");
    
    std::string json = event.ToJSON();
    
    if (json.find("\"data\":\"test input\"") == std::string::npos) {
        std::cout << "  Missing data: " << json << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// FOCUS EVENT TESTS
// ============================================================================

bool TestFocusEvent_RelatedTarget() {
    auto comp1 = std::make_shared<UCComponent>();
    auto comp2 = std::make_shared<UCComponent>();
    
    FocusEvent event(EventType::Focus);
    event.SetTarget(comp1.get());
    event.SetRelatedTarget(comp2.get());
    
    if (event.GetTarget() != comp1.get()) return false;
    if (event.GetRelatedTarget() != comp2.get()) return false;
    
    return true;
}

// ============================================================================
// WHEEL EVENT TESTS
// ============================================================================

bool TestWheelEvent_Delta() {
    WheelEvent event;
    
    event.SetDelta(10.0f, -100.0f, 0.0f);
    event.SetDeltaMode(WheelEvent::DeltaMode::Line);
    
    if (std::abs(event.GetDeltaX() - 10.0f) > 0.001f) return false;
    if (std::abs(event.GetDeltaY() - (-100.0f)) > 0.001f) return false;
    if (event.GetDeltaMode() != WheelEvent::DeltaMode::Line) return false;
    
    return true;
}

// ============================================================================
// CUSTOM EVENT TESTS
// ============================================================================

bool TestCustomEvent_Detail() {
    CustomEvent event("myCustomEvent");
    
    if (event.GetTypeString() != "myCustomEvent") return false;
    
    // Test different detail types
    event.SetDetail(42);
    if (!std::holds_alternative<int>(event.GetDetail())) return false;
    if (std::get<int>(event.GetDetail()) != 42) return false;
    
    event.SetDetail(std::string("hello"));
    if (!std::holds_alternative<std::string>(event.GetDetail())) return false;
    
    event.SetDetail(true);
    if (!std::holds_alternative<bool>(event.GetDetail())) return false;
    
    return true;
}

bool TestCustomEvent_ToJSON() {
    CustomEvent event("testEvent");
    event.SetDetail(std::string("custom data"));
    
    std::string json = event.ToJSON();
    
    if (json.find("\"type\":\"testEvent\"") == std::string::npos) {
        std::cout << "  Missing type: " << json << std::endl;
        return false;
    }
    if (json.find("\"detail\":\"custom data\"") == std::string::npos) {
        std::cout << "  Missing detail: " << json << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// EVENT TYPE CONVERSION TESTS
// ============================================================================

bool TestEventType_Conversion() {
    // String to type
    if (StringToEventType("click") != EventType::Click) return false;
    if (StringToEventType("keydown") != EventType::KeyDown) return false;
    if (StringToEventType("mouseenter") != EventType::MouseEnter) return false;
    if (StringToEventType("unknown") != EventType::Custom) return false;
    
    // Type to string
    if (EventTypeToString(EventType::Click) != "click") return false;
    if (EventTypeToString(EventType::KeyDown) != "keydown") return false;
    if (EventTypeToString(EventType::MouseEnter) != "mouseenter") return false;
    
    return true;
}

// ============================================================================
// EVENT DISPATCHER TESTS
// ============================================================================

bool TestDispatcher_BuildPath() {
    auto root = std::make_shared<UCContainer>();
    auto child1 = std::make_shared<UCContainer>();
    auto child2 = std::make_shared<UCComponent>();
    
    root->SetName("root");
    child1->SetName("child1");
    child2->SetName("child2");
    
    root->AddChild(child1);
    child1->AddChild(child2);
    
    EventDispatcher dispatcher;
    auto path = dispatcher.BuildPath(child2.get());
    
    // Path should be [root, child1] (not including target)
    if (path.size() != 2) {
        std::cout << "  Path size: " << path.size() << " (expected 2)" << std::endl;
        return false;
    }
    
    if (path[0] != root.get()) {
        std::cout << "  Path[0] should be root" << std::endl;
        return false;
    }
    
    if (path[1] != child1.get()) {
        std::cout << "  Path[1] should be child1" << std::endl;
        return false;
    }
    
    return true;
}

bool TestDispatcher_Dispatch() {
    auto root = std::make_shared<UCContainer>();
    auto button = std::make_shared<UCButton>("Test");
    
    root->SetId(1);
    button->SetId(2);
    root->AddChild(button);
    
    EventDispatcher dispatcher;
    
    MouseEvent event(EventType::Click);
    bool result = dispatcher.Dispatch(event, button.get());
    
    if (!result) {
        std::cout << "  Dispatch returned false" << std::endl;
        return false;
    }
    
    if (event.GetTarget() != button.get()) {
        std::cout << "  Target not set correctly" << std::endl;
        return false;
    }
    
    auto stats = dispatcher.GetStats();
    if (stats.totalDispatched != 1) {
        std::cout << "  Total dispatched: " << stats.totalDispatched << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// JS EVENT BRIDGE TESTS
// ============================================================================

bool TestJSBridge_RegisterHandler() {
    JSEventBridge bridge;
    
    bridge.RegisterHandler(1, EventType::Click, "handleClick");
    bridge.RegisterHandler(1, EventType::Input, "handleInput");
    bridge.RegisterHandler(2, EventType::Click, "handleBtn2Click");
    
    // Test event handling
    std::vector<JSEventData> receivedEvents;
    bridge.SetEventHandler([&receivedEvents](const JSEventData& data) {
        receivedEvents.push_back(data);
    });
    
    MouseEvent clickEvent(EventType::Click);
    bridge.OnEvent(1, clickEvent);
    
    if (receivedEvents.size() != 1) {
        std::cout << "  Expected 1 event, got " << receivedEvents.size() << std::endl;
        return false;
    }
    
    if (receivedEvents[0].handlerName != "handleClick") {
        std::cout << "  Handler name: " << receivedEvents[0].handlerName << std::endl;
        return false;
    }
    
    if (receivedEvents[0].elementId != 1) {
        std::cout << "  Element ID: " << receivedEvents[0].elementId << std::endl;
        return false;
    }
    
    return true;
}

bool TestJSBridge_QueueEvents() {
    JSEventBridge bridge;
    
    bridge.RegisterHandler(1, EventType::Click, "onClick");
    
    // Don't set handler - events should queue
    MouseEvent event1(EventType::Click);
    bridge.OnEvent(1, event1);
    
    MouseEvent event2(EventType::Click);
    bridge.OnEvent(1, event2);
    
    if (!bridge.HasPendingEvents()) {
        std::cout << "  Should have pending events" << std::endl;
        return false;
    }
    
    auto queued = bridge.FlushQueue();
    
    if (queued.size() != 2) {
        std::cout << "  Queued: " << queued.size() << " (expected 2)" << std::endl;
        return false;
    }
    
    if (bridge.HasPendingEvents()) {
        std::cout << "  Should not have pending events after flush" << std::endl;
        return false;
    }
    
    return true;
}

bool TestJSBridge_CreateEvents() {
    JSEventBridge bridge;
    
    // Create mouse event
    auto mouseEvent = bridge.CreateMouseEvent(EventType::Click, 100.0f, 200.0f);
    if (mouseEvent->GetType() != EventType::Click) return false;
    if (std::abs(mouseEvent->GetClientX() - 100.0f) > 0.001f) return false;
    if (mouseEvent->IsTrusted()) return false;  // Script-created events are not trusted
    
    // Create keyboard event
    auto keyEvent = bridge.CreateKeyboardEvent(EventType::KeyDown, "Enter", 13);
    if (keyEvent->GetKey() != "Enter") return false;
    if (keyEvent->GetKeyCode() != 13) return false;
    
    // Create input event
    auto inputEvent = bridge.CreateInputEvent("test");
    if (inputEvent->GetData() != "test") return false;
    
    // Create custom event
    auto customEvent = bridge.CreateCustomEvent("myEvent");
    if (customEvent->GetTypeString() != "myEvent") return false;
    
    return true;
}

// ============================================================================
// EVENT SYSTEM TESTS
// ============================================================================

bool TestEventSystem_FireClick() {
    EventSystem& system = EventSystem::Instance();
    system.GetDispatcher().ResetStats();
    
    auto button = std::make_shared<UCButton>("Test");
    button->SetId(1);
    button->SetFrame(0, 0, 100, 40);
    
    bool clicked = false;
    button->SetEventHandler(UCEventType::Click, [&clicked](const UCEvent& e) {
        (void)e;
        clicked = true;
    });
    
    system.FireClick(button.get(), 50.0f, 20.0f);
    
    // Note: The click event goes through EventSystem dispatch, not directly to UCComponent
    // So we check dispatch stats instead
    auto stats = system.GetDispatcher().GetStats();
    if (stats.totalDispatched == 0) {
        std::cout << "  No events dispatched" << std::endl;
        return false;
    }
    
    return true;
}

bool TestEventSystem_FocusManagement() {
    EventSystem& system = EventSystem::Instance();
    
    auto input1 = std::make_shared<UCInput>();
    auto input2 = std::make_shared<UCInput>();
    
    input1->SetId(1);
    input1->SetName("input1");
    input2->SetId(2);
    input2->SetName("input2");
    
    // Initial state
    if (system.GetFocus() != nullptr) {
        // Reset focus from previous tests
        system.SetFocus(nullptr);
    }
    
    // Focus first input
    system.SetFocus(input1.get());
    
    if (system.GetFocus() != input1.get()) {
        std::cout << "  Focus not set to input1" << std::endl;
        return false;
    }
    
    if (!input1->IsFocused()) {
        std::cout << "  input1 not marked as focused" << std::endl;
        return false;
    }
    
    // Focus second input
    system.SetFocus(input2.get());
    
    if (system.GetFocus() != input2.get()) {
        std::cout << "  Focus not moved to input2" << std::endl;
        return false;
    }
    
    if (input1->IsFocused()) {
        std::cout << "  input1 should not be focused" << std::endl;
        return false;
    }
    
    if (!input2->IsFocused()) {
        std::cout << "  input2 should be focused" << std::endl;
        return false;
    }
    
    // Clear focus
    system.SetFocus(nullptr);
    
    if (system.GetFocus() != nullptr) {
        std::cout << "  Focus should be null" << std::endl;
        return false;
    }
    
    return true;
}

bool TestEventSystem_HoverTracking() {
    EventSystem& system = EventSystem::Instance();
    
    auto button1 = std::make_shared<UCButton>("Button 1");
    auto button2 = std::make_shared<UCButton>("Button 2");
    
    button1->SetId(1);
    button2->SetId(2);
    
    // Update hover to button1
    system.UpdateHover(button1.get());
    
    if (system.GetHovered() != button1.get()) {
        std::cout << "  Hover not set to button1" << std::endl;
        return false;
    }
    
    if (!button1->IsHovered()) {
        std::cout << "  button1 should be hovered" << std::endl;
        return false;
    }
    
    // Move hover to button2
    system.UpdateHover(button2.get());
    
    if (button1->IsHovered()) {
        std::cout << "  button1 should not be hovered" << std::endl;
        return false;
    }
    
    if (!button2->IsHovered()) {
        std::cout << "  button2 should be hovered" << std::endl;
        return false;
    }
    
    // Clear hover
    system.UpdateHover(nullptr);
    
    if (button2->IsHovered()) {
        std::cout << "  button2 should not be hovered" << std::endl;
        return false;
    }
    
    return true;
}

bool TestEventSystem_PressedState() {
    EventSystem& system = EventSystem::Instance();
    
    auto button = std::make_shared<UCButton>("Test");
    button->SetId(1);
    
    system.SetPressed(button.get());
    
    if (system.GetPressed() != button.get()) return false;
    if (!button->IsPressed()) return false;
    
    system.ReleasePressed();
    
    if (system.GetPressed() != nullptr) return false;
    if (button->IsPressed()) return false;
    
    return true;
}

bool TestEventSystem_ModifierState() {
    EventSystem& system = EventSystem::Instance();
    
    system.SetModifierState(true, false, true, false);
    
    if (!system.IsAltDown()) return false;
    if (system.IsCtrlDown()) return false;
    if (!system.IsShiftDown()) return false;
    if (system.IsMetaDown()) return false;
    
    system.SetModifierState(false, false, false, false);
    
    return true;
}

bool TestEventSystem_KeyState() {
    EventSystem& system = EventSystem::Instance();
    
    system.SetKeyState(65, true);  // 'A' key
    system.SetKeyState(16, true);  // Shift
    
    if (!system.IsKeyDown(65)) return false;
    if (!system.IsKeyDown(16)) return false;
    if (system.IsKeyDown(17)) return false;  // Ctrl not pressed
    
    system.SetKeyState(65, false);
    
    if (system.IsKeyDown(65)) return false;
    
    system.SetKeyState(16, false);
    
    return true;
}

bool TestEventSystem_JSBridgeIntegration() {
    EventSystem& system = EventSystem::Instance();
    JSEventBridge& bridge = system.GetJSBridge();
    
    auto button = std::make_shared<UCButton>("Test");
    button->SetId(42);
    
    bridge.RegisterHandler(42, EventType::Click, "onButtonClick");
    
    std::vector<JSEventData> events;
    bridge.SetEventHandler([&events](const JSEventData& data) {
        events.push_back(data);
    });
    
    system.FireClick(button.get(), 50.0f, 20.0f);
    
    if (events.empty()) {
        std::cout << "  No events received by JS bridge" << std::endl;
        return false;
    }
    
    if (events[0].handlerName != "onButtonClick") {
        std::cout << "  Handler: " << events[0].handlerName << std::endl;
        return false;
    }
    
    if (events[0].elementId != 42) {
        std::cout << "  Element ID: " << events[0].elementId << std::endl;
        return false;
    }
    
    // Clear for next test
    bridge.SetEventHandler(nullptr);
    
    return true;
}

// ============================================================================
// EVENT CLONE TESTS
// ============================================================================

bool TestEvent_Clone() {
    MouseEvent original(EventType::Click);
    original.SetClientPos(100.0f, 200.0f);
    original.SetButton(1);
    original.SetModifiers(true, false, false, false);
    
    auto clone = original.Clone();
    auto* mouseClone = dynamic_cast<MouseEvent*>(clone.get());
    
    if (!mouseClone) {
        std::cout << "  Clone is not MouseEvent" << std::endl;
        return false;
    }
    
    if (mouseClone->GetType() != EventType::Click) return false;
    if (std::abs(mouseClone->GetClientX() - 100.0f) > 0.001f) return false;
    if (mouseClone->GetButton() != 1) return false;
    if (!mouseClone->GetAltKey()) return false;
    
    return true;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "UltraWeb Event System Test Suite\n";
    PrintSeparator();
    
    int passed = 0;
    int failed = 0;
    
    // Event Creation Tests
    std::cout << "\n[Event Creation Tests]\n";
    
    if (TestEvent_BasicProperties()) { passed++; PrintTestResult("Basic Properties", true); }
    else { failed++; PrintTestResult("Basic Properties", false); }
    
    if (TestEvent_PropagationControl()) { passed++; PrintTestResult("Propagation Control", true); }
    else { failed++; PrintTestResult("Propagation Control", false); }
    
    if (TestEvent_PreventDefault()) { passed++; PrintTestResult("Prevent Default", true); }
    else { failed++; PrintTestResult("Prevent Default", false); }
    
    if (TestEvent_NonBubblingEvents()) { passed++; PrintTestResult("Non-Bubbling Events", true); }
    else { failed++; PrintTestResult("Non-Bubbling Events", false); }
    
    if (TestEvent_ToJSON()) { passed++; PrintTestResult("To JSON", true); }
    else { failed++; PrintTestResult("To JSON", false); }
    
    // Mouse Event Tests
    std::cout << "\n[Mouse Event Tests]\n";
    
    if (TestMouseEvent_Position()) { passed++; PrintTestResult("Position", true); }
    else { failed++; PrintTestResult("Position", false); }
    
    if (TestMouseEvent_Button()) { passed++; PrintTestResult("Button", true); }
    else { failed++; PrintTestResult("Button", false); }
    
    if (TestMouseEvent_Modifiers()) { passed++; PrintTestResult("Modifiers", true); }
    else { failed++; PrintTestResult("Modifiers", false); }
    
    if (TestMouseEvent_ToJSON()) { passed++; PrintTestResult("To JSON", true); }
    else { failed++; PrintTestResult("To JSON", false); }
    
    // Keyboard Event Tests
    std::cout << "\n[Keyboard Event Tests]\n";
    
    if (TestKeyboardEvent_KeyInfo()) { passed++; PrintTestResult("Key Info", true); }
    else { failed++; PrintTestResult("Key Info", false); }
    
    if (TestKeyboardEvent_Location()) { passed++; PrintTestResult("Location", true); }
    else { failed++; PrintTestResult("Location", false); }
    
    if (TestKeyboardEvent_RepeatAndComposing()) { passed++; PrintTestResult("Repeat and Composing", true); }
    else { failed++; PrintTestResult("Repeat and Composing", false); }
    
    if (TestKeyboardEvent_ToJSON()) { passed++; PrintTestResult("To JSON", true); }
    else { failed++; PrintTestResult("To JSON", false); }
    
    // Input Event Tests
    std::cout << "\n[Input Event Tests]\n";
    
    if (TestInputEvent_Data()) { passed++; PrintTestResult("Data", true); }
    else { failed++; PrintTestResult("Data", false); }
    
    if (TestInputEvent_ToJSON()) { passed++; PrintTestResult("To JSON", true); }
    else { failed++; PrintTestResult("To JSON", false); }
    
    // Focus Event Tests
    std::cout << "\n[Focus Event Tests]\n";
    
    if (TestFocusEvent_RelatedTarget()) { passed++; PrintTestResult("Related Target", true); }
    else { failed++; PrintTestResult("Related Target", false); }
    
    // Wheel Event Tests
    std::cout << "\n[Wheel Event Tests]\n";
    
    if (TestWheelEvent_Delta()) { passed++; PrintTestResult("Delta", true); }
    else { failed++; PrintTestResult("Delta", false); }
    
    // Custom Event Tests
    std::cout << "\n[Custom Event Tests]\n";
    
    if (TestCustomEvent_Detail()) { passed++; PrintTestResult("Detail", true); }
    else { failed++; PrintTestResult("Detail", false); }
    
    if (TestCustomEvent_ToJSON()) { passed++; PrintTestResult("To JSON", true); }
    else { failed++; PrintTestResult("To JSON", false); }
    
    // Type Conversion Tests
    std::cout << "\n[Type Conversion Tests]\n";
    
    if (TestEventType_Conversion()) { passed++; PrintTestResult("Event Type Conversion", true); }
    else { failed++; PrintTestResult("Event Type Conversion", false); }
    
    // Dispatcher Tests
    std::cout << "\n[Event Dispatcher Tests]\n";
    
    if (TestDispatcher_BuildPath()) { passed++; PrintTestResult("Build Path", true); }
    else { failed++; PrintTestResult("Build Path", false); }
    
    if (TestDispatcher_Dispatch()) { passed++; PrintTestResult("Dispatch", true); }
    else { failed++; PrintTestResult("Dispatch", false); }
    
    // JS Bridge Tests
    std::cout << "\n[JS Event Bridge Tests]\n";
    
    if (TestJSBridge_RegisterHandler()) { passed++; PrintTestResult("Register Handler", true); }
    else { failed++; PrintTestResult("Register Handler", false); }
    
    if (TestJSBridge_QueueEvents()) { passed++; PrintTestResult("Queue Events", true); }
    else { failed++; PrintTestResult("Queue Events", false); }
    
    if (TestJSBridge_CreateEvents()) { passed++; PrintTestResult("Create Events", true); }
    else { failed++; PrintTestResult("Create Events", false); }
    
    // Event System Tests
    std::cout << "\n[Event System Tests]\n";
    
    if (TestEventSystem_FireClick()) { passed++; PrintTestResult("Fire Click", true); }
    else { failed++; PrintTestResult("Fire Click", false); }
    
    if (TestEventSystem_FocusManagement()) { passed++; PrintTestResult("Focus Management", true); }
    else { failed++; PrintTestResult("Focus Management", false); }
    
    if (TestEventSystem_HoverTracking()) { passed++; PrintTestResult("Hover Tracking", true); }
    else { failed++; PrintTestResult("Hover Tracking", false); }
    
    if (TestEventSystem_PressedState()) { passed++; PrintTestResult("Pressed State", true); }
    else { failed++; PrintTestResult("Pressed State", false); }
    
    if (TestEventSystem_ModifierState()) { passed++; PrintTestResult("Modifier State", true); }
    else { failed++; PrintTestResult("Modifier State", false); }
    
    if (TestEventSystem_KeyState()) { passed++; PrintTestResult("Key State", true); }
    else { failed++; PrintTestResult("Key State", false); }
    
    if (TestEventSystem_JSBridgeIntegration()) { passed++; PrintTestResult("JS Bridge Integration", true); }
    else { failed++; PrintTestResult("JS Bridge Integration", false); }
    
    // Clone Tests
    std::cout << "\n[Event Clone Tests]\n";
    
    if (TestEvent_Clone()) { passed++; PrintTestResult("Clone", true); }
    else { failed++; PrintTestResult("Clone", false); }
    
    // Summary
    std::cout << "\n";
    PrintSeparator();
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    PrintSeparator();
    std::cout << "\n";
    
    return failed > 0 ? 1 : 0;
}
