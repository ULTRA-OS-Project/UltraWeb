// UltraWeb/runtime/EventSystem.h
// Event Binding System with Propagation and JS Integration
// Version: 1.0.0

#pragma once

#include "UCComponents.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <queue>
#include <variant>
#include <unordered_set>

namespace UltraWeb {
namespace Runtime {

// Forward declarations
class UCComponent;
class EventSystem;
class EventDispatcher;

// ============================================================================
// EVENT DATA STRUCTURES
// ============================================================================

// Extended event types
enum class EventType {
    // Mouse events
    Click,
    DblClick,
    MouseDown,
    MouseUp,
    MouseMove,
    MouseEnter,
    MouseLeave,
    MouseOver,
    MouseOut,
    ContextMenu,
    Wheel,
    
    // Keyboard events
    KeyDown,
    KeyUp,
    KeyPress,
    
    // Focus events
    Focus,
    Blur,
    FocusIn,
    FocusOut,
    
    // Form events
    Input,
    Change,
    Submit,
    Reset,
    
    // Touch events
    TouchStart,
    TouchMove,
    TouchEnd,
    TouchCancel,
    
    // Drag events
    DragStart,
    DragEnd,
    Drag,
    DragEnter,
    DragLeave,
    DragOver,
    Drop,
    
    // UI events
    Scroll,
    Resize,
    
    // Custom events
    Custom
};

// Event phase (for propagation)
enum class EventPhase {
    None = 0,
    Capturing = 1,
    AtTarget = 2,
    Bubbling = 3
};

// Convert EventType to string
std::string EventTypeToString(EventType type);
EventType StringToEventType(const std::string& str);

// ============================================================================
// EVENT OBJECT
// ============================================================================

class Event {
public:
    Event(EventType type);
    Event(EventType type, const std::string& customType);
    virtual ~Event() = default;
    
    // Type
    EventType GetType() const { return type; }
    const std::string& GetTypeString() const { return typeString; }
    
    // Target and propagation
    UCComponent* GetTarget() const { return target; }
    UCComponent* GetCurrentTarget() const { return currentTarget; }
    EventPhase GetPhase() const { return phase; }
    
    void SetTarget(UCComponent* t) { target = t; }
    void SetCurrentTarget(UCComponent* t) { currentTarget = t; }
    void SetPhase(EventPhase p) { phase = p; }
    
    // Propagation control
    void StopPropagation() { propagationStopped = true; }
    void StopImmediatePropagation() { immediatePropagationStopped = true; propagationStopped = true; }
    bool IsPropagationStopped() const { return propagationStopped; }
    bool IsImmediatePropagationStopped() const { return immediatePropagationStopped; }
    
    // Default action
    void PreventDefault() { defaultPrevented = true; }
    bool IsDefaultPrevented() const { return defaultPrevented; }
    
    // Bubbling
    bool Bubbles() const { return bubbles; }
    void SetBubbles(bool b) { bubbles = b; }
    
    // Cancelable
    bool IsCancelable() const { return cancelable; }
    void SetCancelable(bool c) { cancelable = c; }
    
    // Timestamp
    double GetTimeStamp() const { return timeStamp; }
    
    // Trusted (system-generated vs script-generated)
    bool IsTrusted() const { return trusted; }
    void SetTrusted(bool t) { trusted = t; }
    
    // Clone for re-dispatch
    virtual std::shared_ptr<Event> Clone() const;
    
    // Serialize to JSON for JS consumption
    virtual std::string ToJSON() const;
    
protected:
    EventType type;
    std::string typeString;
    UCComponent* target;
    UCComponent* currentTarget;
    EventPhase phase;
    
    bool propagationStopped;
    bool immediatePropagationStopped;
    bool defaultPrevented;
    bool bubbles;
    bool cancelable;
    bool trusted;
    double timeStamp;
};

// ============================================================================
// MOUSE EVENT
// ============================================================================

class MouseEvent : public Event {
public:
    MouseEvent(EventType type);
    
    // Position
    float GetClientX() const { return clientX; }
    float GetClientY() const { return clientY; }
    float GetScreenX() const { return screenX; }
    float GetScreenY() const { return screenY; }
    float GetOffsetX() const { return offsetX; }
    float GetOffsetY() const { return offsetY; }
    float GetPageX() const { return pageX; }
    float GetPageY() const { return pageY; }
    
    void SetClientPos(float x, float y) { clientX = x; clientY = y; }
    void SetScreenPos(float x, float y) { screenX = x; screenY = y; }
    void SetOffsetPos(float x, float y) { offsetX = x; offsetY = y; }
    void SetPagePos(float x, float y) { pageX = x; pageY = y; }
    
    // Button
    int GetButton() const { return button; }
    int GetButtons() const { return buttons; }
    void SetButton(int b) { button = b; }
    void SetButtons(int b) { buttons = b; }
    
    // Modifiers
    bool GetAltKey() const { return altKey; }
    bool GetCtrlKey() const { return ctrlKey; }
    bool GetShiftKey() const { return shiftKey; }
    bool GetMetaKey() const { return metaKey; }
    
    void SetModifiers(bool alt, bool ctrl, bool shift, bool meta) {
        altKey = alt; ctrlKey = ctrl; shiftKey = shift; metaKey = meta;
    }
    
    // Related target (for enter/leave events)
    UCComponent* GetRelatedTarget() const { return relatedTarget; }
    void SetRelatedTarget(UCComponent* t) { relatedTarget = t; }
    
    std::shared_ptr<Event> Clone() const override;
    std::string ToJSON() const override;
    
private:
    float clientX, clientY;
    float screenX, screenY;
    float offsetX, offsetY;
    float pageX, pageY;
    int button;
    int buttons;
    bool altKey, ctrlKey, shiftKey, metaKey;
    UCComponent* relatedTarget;
};

// ============================================================================
// KEYBOARD EVENT
// ============================================================================

class KeyboardEvent : public Event {
public:
    KeyboardEvent(EventType type);
    
    // Key info
    const std::string& GetKey() const { return key; }
    const std::string& GetCode() const { return code; }
    int GetKeyCode() const { return keyCode; }
    int GetCharCode() const { return charCode; }
    
    void SetKey(const std::string& k) { key = k; }
    void SetCode(const std::string& c) { code = c; }
    void SetKeyCode(int kc) { keyCode = kc; }
    void SetCharCode(int cc) { charCode = cc; }
    
    // Location
    enum class Location { Standard = 0, Left = 1, Right = 2, Numpad = 3 };
    Location GetLocation() const { return location; }
    void SetLocation(Location loc) { location = loc; }
    
    // Modifiers
    bool GetAltKey() const { return altKey; }
    bool GetCtrlKey() const { return ctrlKey; }
    bool GetShiftKey() const { return shiftKey; }
    bool GetMetaKey() const { return metaKey; }
    
    void SetModifiers(bool alt, bool ctrl, bool shift, bool meta) {
        altKey = alt; ctrlKey = ctrl; shiftKey = shift; metaKey = meta;
    }
    
    // Repeat
    bool IsRepeat() const { return repeat; }
    void SetRepeat(bool r) { repeat = r; }
    
    // Composing (for IME)
    bool IsComposing() const { return isComposing; }
    void SetComposing(bool c) { isComposing = c; }
    
    std::shared_ptr<Event> Clone() const override;
    std::string ToJSON() const override;
    
private:
    std::string key;
    std::string code;
    int keyCode;
    int charCode;
    Location location;
    bool altKey, ctrlKey, shiftKey, metaKey;
    bool repeat;
    bool isComposing;
};

// ============================================================================
// INPUT EVENT
// ============================================================================

class InputEvent : public Event {
public:
    InputEvent(EventType type);
    
    // Input data
    const std::string& GetData() const { return data; }
    void SetData(const std::string& d) { data = d; }
    
    // Input type
    const std::string& GetInputType() const { return inputType; }
    void SetInputType(const std::string& t) { inputType = t; }
    
    // Is composing
    bool IsComposing() const { return isComposing; }
    void SetComposing(bool c) { isComposing = c; }
    
    std::shared_ptr<Event> Clone() const override;
    std::string ToJSON() const override;
    
private:
    std::string data;
    std::string inputType;  // insertText, deleteContentBackward, etc.
    bool isComposing;
};

// ============================================================================
// FOCUS EVENT
// ============================================================================

class FocusEvent : public Event {
public:
    FocusEvent(EventType type);
    
    UCComponent* GetRelatedTarget() const { return relatedTarget; }
    void SetRelatedTarget(UCComponent* t) { relatedTarget = t; }
    
    std::shared_ptr<Event> Clone() const override;
    std::string ToJSON() const override;
    
private:
    UCComponent* relatedTarget;
};

// ============================================================================
// WHEEL EVENT
// ============================================================================

class WheelEvent : public MouseEvent {
public:
    WheelEvent();
    
    float GetDeltaX() const { return deltaX; }
    float GetDeltaY() const { return deltaY; }
    float GetDeltaZ() const { return deltaZ; }
    
    void SetDelta(float x, float y, float z = 0) {
        deltaX = x; deltaY = y; deltaZ = z;
    }
    
    enum class DeltaMode { Pixel = 0, Line = 1, Page = 2 };
    DeltaMode GetDeltaMode() const { return deltaMode; }
    void SetDeltaMode(DeltaMode mode) { deltaMode = mode; }
    
    std::shared_ptr<Event> Clone() const override;
    std::string ToJSON() const override;
    
private:
    float deltaX, deltaY, deltaZ;
    DeltaMode deltaMode;
};

// ============================================================================
// CUSTOM EVENT
// ============================================================================

class CustomEvent : public Event {
public:
    CustomEvent(const std::string& eventType);
    
    // Detail can hold arbitrary data
    using DetailValue = std::variant<
        std::nullptr_t,
        bool,
        int,
        double,
        std::string,
        std::vector<std::string>
    >;
    
    const DetailValue& GetDetail() const { return detail; }
    void SetDetail(const DetailValue& d) { detail = d; }
    
    std::shared_ptr<Event> Clone() const override;
    std::string ToJSON() const override;
    
private:
    DetailValue detail;
};

// ============================================================================
// EVENT LISTENER
// ============================================================================

struct EventListenerOptions {
    bool capture = false;      // Use capturing phase
    bool once = false;         // Remove after first invocation
    bool passive = false;      // Won't call preventDefault()
    
    EventListenerOptions() = default;
    EventListenerOptions(bool cap) : capture(cap) {}
};

using EventCallback = std::function<void(Event&)>;

struct EventListener {
    EventCallback callback;
    EventListenerOptions options;
    std::string handlerName;   // For JS binding
    uint32_t id;               // Unique listener ID
    
    EventListener() : id(0) {}
    EventListener(EventCallback cb, const EventListenerOptions& opts, uint32_t listenerId)
        : callback(cb), options(opts), id(listenerId) {}
};

// ============================================================================
// EVENT TARGET (Mixin for components)
// ============================================================================

class EventTarget {
public:
    EventTarget();
    virtual ~EventTarget() = default;
    
    // Add/remove listeners
    uint32_t AddEventListener(EventType type, EventCallback callback, 
                              const EventListenerOptions& options = {});
    uint32_t AddEventListener(const std::string& type, EventCallback callback,
                              const EventListenerOptions& options = {});
    
    void RemoveEventListener(EventType type, uint32_t listenerId);
    void RemoveEventListener(const std::string& type, uint32_t listenerId);
    void RemoveAllEventListeners(EventType type);
    void RemoveAllEventListeners();
    
    // Bind to named handler (for UCML onclick="handlerName" etc.)
    void BindHandler(EventType type, const std::string& handlerName,
                     const EventListenerOptions& options = {});
    
    // Dispatch event
    bool DispatchEvent(Event& event);
    
    // Get listeners
    const std::vector<EventListener>& GetListeners(EventType type) const;
    bool HasListeners(EventType type) const;
    
protected:
    virtual void InvokeListeners(Event& event);
    
    std::unordered_map<EventType, std::vector<EventListener>> listeners;
    uint32_t nextListenerId;
};

// ============================================================================
// EVENT DISPATCHER
// ============================================================================

class EventDispatcher {
public:
    EventDispatcher();
    
    // Main dispatch function - handles propagation
    bool Dispatch(Event& event, UCComponent* target);
    
    // Build propagation path
    std::vector<UCComponent*> BuildPath(UCComponent* target);
    
    // Statistics
    struct DispatchStats {
        uint64_t totalDispatched;
        uint64_t propagationsStopped;
        uint64_t defaultsPrevented;
    };
    DispatchStats GetStats() const { return stats; }
    void ResetStats() { stats = {0, 0, 0}; }
    
private:
    void DispatchAtPhase(Event& event, const std::vector<UCComponent*>& path, 
                         EventPhase phase);
    
    DispatchStats stats;
};

// ============================================================================
// JS BINDING INTERFACE
// ============================================================================

// Represents a pending event to be sent to JS
struct JSEventData {
    uint16_t elementId;
    std::string eventType;
    std::string handlerName;
    std::string eventJSON;
    double timestamp;
};

// Callback type for JS event handling
using JSEventHandler = std::function<void(const JSEventData&)>;

// JS Event Bridge
class JSEventBridge {
public:
    JSEventBridge();
    
    // Set handler for all events going to JS
    void SetEventHandler(JSEventHandler handler);
    
    // Register element-handler mapping
    void RegisterHandler(uint16_t elementId, EventType type, 
                        const std::string& handlerName);
    void UnregisterHandler(uint16_t elementId, EventType type);
    
    // Called when event fires on element
    void OnEvent(uint16_t elementId, Event& event);
    
    // Queue management (for batched updates)
    void QueueEvent(const JSEventData& data);
    std::vector<JSEventData> FlushQueue();
    bool HasPendingEvents() const { return !eventQueue.empty(); }
    
    // Synthetic event creation (for JS to fire events)
    std::shared_ptr<MouseEvent> CreateMouseEvent(EventType type, float x, float y);
    std::shared_ptr<KeyboardEvent> CreateKeyboardEvent(EventType type, 
                                                        const std::string& key, int keyCode);
    std::shared_ptr<InputEvent> CreateInputEvent(const std::string& data);
    std::shared_ptr<CustomEvent> CreateCustomEvent(const std::string& type);
    
private:
    JSEventHandler jsHandler;
    std::unordered_map<uint64_t, std::string> handlerMap;  // (elementId << 32 | type) -> handlerName
    std::queue<JSEventData> eventQueue;
    
    uint64_t MakeKey(uint16_t elementId, EventType type) const;
};

// ============================================================================
// EVENT SYSTEM (Main coordinator)
// ============================================================================

class EventSystem {
public:
    EventSystem();
    ~EventSystem();
    
    // Get singleton (optional pattern)
    static EventSystem& Instance();
    
    // Dispatcher access
    EventDispatcher& GetDispatcher() { return dispatcher; }
    
    // JS Bridge access
    JSEventBridge& GetJSBridge() { return jsBridge; }
    
    // High-level event firing
    bool FireEvent(UCComponent* target, std::shared_ptr<Event> event);
    bool FireClick(UCComponent* target, float x, float y);
    bool FireMouseEvent(UCComponent* target, EventType type, float x, float y);
    bool FireKeyEvent(UCComponent* target, EventType type, 
                     const std::string& key, int keyCode);
    bool FireInputEvent(UCComponent* target, const std::string& data);
    bool FireFocusEvent(UCComponent* target, EventType type, 
                       UCComponent* relatedTarget = nullptr);
    bool FireCustomEvent(UCComponent* target, const std::string& eventType);
    
    // Focus management
    void SetFocus(UCComponent* component);
    UCComponent* GetFocus() const { return focusedComponent; }
    
    // Hover tracking
    void UpdateHover(UCComponent* component);
    UCComponent* GetHovered() const { return hoveredComponent; }
    
    // Pressed tracking
    void SetPressed(UCComponent* component);
    UCComponent* GetPressed() const { return pressedComponent; }
    void ReleasePressed();
    
    // Input state
    bool IsKeyDown(int keyCode) const;
    bool IsMouseButtonDown(int button) const;
    void SetKeyState(int keyCode, bool down);
    void SetMouseButtonState(int button, bool down);
    
    // Modifier state
    bool IsAltDown() const { return altDown; }
    bool IsCtrlDown() const { return ctrlDown; }
    bool IsShiftDown() const { return shiftDown; }
    bool IsMetaDown() const { return metaDown; }
    void SetModifierState(bool alt, bool ctrl, bool shift, bool meta);
    
private:
    EventDispatcher dispatcher;
    JSEventBridge jsBridge;
    
    UCComponent* focusedComponent;
    UCComponent* hoveredComponent;
    UCComponent* pressedComponent;
    
    std::unordered_set<int> keysDown;
    std::unordered_set<int> mouseButtonsDown;
    bool altDown, ctrlDown, shiftDown, metaDown;
};

} // namespace Runtime
} // namespace UltraWeb
