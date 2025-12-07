// UltraWeb/runtime/EventSystem.cpp
// Event Binding System Implementation
// Version: 1.0.0

#include "EventSystem.h"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <iomanip>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

std::string EventTypeToString(EventType type) {
    switch (type) {
        case EventType::Click: return "click";
        case EventType::DblClick: return "dblclick";
        case EventType::MouseDown: return "mousedown";
        case EventType::MouseUp: return "mouseup";
        case EventType::MouseMove: return "mousemove";
        case EventType::MouseEnter: return "mouseenter";
        case EventType::MouseLeave: return "mouseleave";
        case EventType::MouseOver: return "mouseover";
        case EventType::MouseOut: return "mouseout";
        case EventType::ContextMenu: return "contextmenu";
        case EventType::Wheel: return "wheel";
        case EventType::KeyDown: return "keydown";
        case EventType::KeyUp: return "keyup";
        case EventType::KeyPress: return "keypress";
        case EventType::Focus: return "focus";
        case EventType::Blur: return "blur";
        case EventType::FocusIn: return "focusin";
        case EventType::FocusOut: return "focusout";
        case EventType::Input: return "input";
        case EventType::Change: return "change";
        case EventType::Submit: return "submit";
        case EventType::Reset: return "reset";
        case EventType::TouchStart: return "touchstart";
        case EventType::TouchMove: return "touchmove";
        case EventType::TouchEnd: return "touchend";
        case EventType::TouchCancel: return "touchcancel";
        case EventType::DragStart: return "dragstart";
        case EventType::DragEnd: return "dragend";
        case EventType::Drag: return "drag";
        case EventType::DragEnter: return "dragenter";
        case EventType::DragLeave: return "dragleave";
        case EventType::DragOver: return "dragover";
        case EventType::Drop: return "drop";
        case EventType::Scroll: return "scroll";
        case EventType::Resize: return "resize";
        case EventType::Custom: return "custom";
        default: return "unknown";
    }
}

EventType StringToEventType(const std::string& str) {
    static const std::unordered_map<std::string, EventType> map = {
        {"click", EventType::Click},
        {"dblclick", EventType::DblClick},
        {"mousedown", EventType::MouseDown},
        {"mouseup", EventType::MouseUp},
        {"mousemove", EventType::MouseMove},
        {"mouseenter", EventType::MouseEnter},
        {"mouseleave", EventType::MouseLeave},
        {"mouseover", EventType::MouseOver},
        {"mouseout", EventType::MouseOut},
        {"contextmenu", EventType::ContextMenu},
        {"wheel", EventType::Wheel},
        {"keydown", EventType::KeyDown},
        {"keyup", EventType::KeyUp},
        {"keypress", EventType::KeyPress},
        {"focus", EventType::Focus},
        {"blur", EventType::Blur},
        {"focusin", EventType::FocusIn},
        {"focusout", EventType::FocusOut},
        {"input", EventType::Input},
        {"change", EventType::Change},
        {"submit", EventType::Submit},
        {"reset", EventType::Reset},
        {"touchstart", EventType::TouchStart},
        {"touchmove", EventType::TouchMove},
        {"touchend", EventType::TouchEnd},
        {"touchcancel", EventType::TouchCancel},
        {"dragstart", EventType::DragStart},
        {"dragend", EventType::DragEnd},
        {"drag", EventType::Drag},
        {"dragenter", EventType::DragEnter},
        {"dragleave", EventType::DragLeave},
        {"dragover", EventType::DragOver},
        {"drop", EventType::Drop},
        {"scroll", EventType::Scroll},
        {"resize", EventType::Resize},
    };
    
    auto it = map.find(str);
    return (it != map.end()) ? it->second : EventType::Custom;
}

static double GetTimestamp() {
    auto now = std::chrono::high_resolution_clock::now();
    auto epoch = now.time_since_epoch();
    return std::chrono::duration<double, std::milli>(epoch).count();
}

static std::string EscapeJSON(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (c < 0x20) {
                    oss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << (int)c;
                } else {
                    oss << c;
                }
        }
    }
    return oss.str();
}

// ============================================================================
// EVENT IMPLEMENTATION
// ============================================================================

Event::Event(EventType t)
    : type(t)
    , typeString(EventTypeToString(t))
    , target(nullptr)
    , currentTarget(nullptr)
    , phase(EventPhase::None)
    , propagationStopped(false)
    , immediatePropagationStopped(false)
    , defaultPrevented(false)
    , bubbles(true)
    , cancelable(true)
    , trusted(true)
    , timeStamp(GetTimestamp()) {
    
    // Some events don't bubble
    switch (type) {
        case EventType::Focus:
        case EventType::Blur:
        case EventType::MouseEnter:
        case EventType::MouseLeave:
            bubbles = false;
            break;
        default:
            break;
    }
}

Event::Event(EventType t, const std::string& customType)
    : Event(t) {
    typeString = customType;
}

std::shared_ptr<Event> Event::Clone() const {
    auto clone = std::make_shared<Event>(type, typeString);
    clone->bubbles = bubbles;
    clone->cancelable = cancelable;
    return clone;
}

std::string Event::ToJSON() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"" << EscapeJSON(typeString) << "\"";
    oss << ",\"bubbles\":" << (bubbles ? "true" : "false");
    oss << ",\"cancelable\":" << (cancelable ? "true" : "false");
    oss << ",\"defaultPrevented\":" << (defaultPrevented ? "true" : "false");
    oss << ",\"eventPhase\":" << static_cast<int>(phase);
    oss << ",\"timeStamp\":" << std::fixed << std::setprecision(3) << timeStamp;
    oss << ",\"isTrusted\":" << (trusted ? "true" : "false");
    if (target) {
        oss << ",\"targetId\":" << target->GetId();
    }
    oss << "}";
    return oss.str();
}

// ============================================================================
// MOUSE EVENT IMPLEMENTATION
// ============================================================================

MouseEvent::MouseEvent(EventType type)
    : Event(type)
    , clientX(0), clientY(0)
    , screenX(0), screenY(0)
    , offsetX(0), offsetY(0)
    , pageX(0), pageY(0)
    , button(0), buttons(0)
    , altKey(false), ctrlKey(false), shiftKey(false), metaKey(false)
    , relatedTarget(nullptr) {
}

std::shared_ptr<Event> MouseEvent::Clone() const {
    auto clone = std::make_shared<MouseEvent>(type);
    clone->SetClientPos(clientX, clientY);
    clone->SetScreenPos(screenX, screenY);
    clone->SetOffsetPos(offsetX, offsetY);
    clone->SetPagePos(pageX, pageY);
    clone->SetButton(button);
    clone->SetButtons(buttons);
    clone->SetModifiers(altKey, ctrlKey, shiftKey, metaKey);
    clone->SetRelatedTarget(relatedTarget);
    clone->SetBubbles(bubbles);
    clone->SetCancelable(cancelable);
    return clone;
}

std::string MouseEvent::ToJSON() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"" << EscapeJSON(typeString) << "\"";
    oss << ",\"clientX\":" << clientX;
    oss << ",\"clientY\":" << clientY;
    oss << ",\"screenX\":" << screenX;
    oss << ",\"screenY\":" << screenY;
    oss << ",\"offsetX\":" << offsetX;
    oss << ",\"offsetY\":" << offsetY;
    oss << ",\"pageX\":" << pageX;
    oss << ",\"pageY\":" << pageY;
    oss << ",\"button\":" << button;
    oss << ",\"buttons\":" << buttons;
    oss << ",\"altKey\":" << (altKey ? "true" : "false");
    oss << ",\"ctrlKey\":" << (ctrlKey ? "true" : "false");
    oss << ",\"shiftKey\":" << (shiftKey ? "true" : "false");
    oss << ",\"metaKey\":" << (metaKey ? "true" : "false");
    oss << ",\"bubbles\":" << (bubbles ? "true" : "false");
    oss << ",\"cancelable\":" << (cancelable ? "true" : "false");
    oss << ",\"defaultPrevented\":" << (defaultPrevented ? "true" : "false");
    oss << ",\"timeStamp\":" << std::fixed << std::setprecision(3) << timeStamp;
    if (target) {
        oss << ",\"targetId\":" << target->GetId();
    }
    if (relatedTarget) {
        oss << ",\"relatedTargetId\":" << relatedTarget->GetId();
    }
    oss << "}";
    return oss.str();
}

// ============================================================================
// KEYBOARD EVENT IMPLEMENTATION
// ============================================================================

KeyboardEvent::KeyboardEvent(EventType type)
    : Event(type)
    , keyCode(0), charCode(0)
    , location(Location::Standard)
    , altKey(false), ctrlKey(false), shiftKey(false), metaKey(false)
    , repeat(false), isComposing(false) {
}

std::shared_ptr<Event> KeyboardEvent::Clone() const {
    auto clone = std::make_shared<KeyboardEvent>(type);
    clone->SetKey(key);
    clone->SetCode(code);
    clone->SetKeyCode(keyCode);
    clone->SetCharCode(charCode);
    clone->SetLocation(location);
    clone->SetModifiers(altKey, ctrlKey, shiftKey, metaKey);
    clone->SetRepeat(repeat);
    clone->SetComposing(isComposing);
    clone->SetBubbles(bubbles);
    clone->SetCancelable(cancelable);
    return clone;
}

std::string KeyboardEvent::ToJSON() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"" << EscapeJSON(typeString) << "\"";
    oss << ",\"key\":\"" << EscapeJSON(key) << "\"";
    oss << ",\"code\":\"" << EscapeJSON(code) << "\"";
    oss << ",\"keyCode\":" << keyCode;
    oss << ",\"charCode\":" << charCode;
    oss << ",\"location\":" << static_cast<int>(location);
    oss << ",\"altKey\":" << (altKey ? "true" : "false");
    oss << ",\"ctrlKey\":" << (ctrlKey ? "true" : "false");
    oss << ",\"shiftKey\":" << (shiftKey ? "true" : "false");
    oss << ",\"metaKey\":" << (metaKey ? "true" : "false");
    oss << ",\"repeat\":" << (repeat ? "true" : "false");
    oss << ",\"isComposing\":" << (isComposing ? "true" : "false");
    oss << ",\"bubbles\":" << (bubbles ? "true" : "false");
    oss << ",\"cancelable\":" << (cancelable ? "true" : "false");
    oss << ",\"defaultPrevented\":" << (defaultPrevented ? "true" : "false");
    oss << ",\"timeStamp\":" << std::fixed << std::setprecision(3) << timeStamp;
    if (target) {
        oss << ",\"targetId\":" << target->GetId();
    }
    oss << "}";
    return oss.str();
}

// ============================================================================
// INPUT EVENT IMPLEMENTATION
// ============================================================================

InputEvent::InputEvent(EventType type)
    : Event(type)
    , inputType("insertText")
    , isComposing(false) {
}

std::shared_ptr<Event> InputEvent::Clone() const {
    auto clone = std::make_shared<InputEvent>(type);
    clone->SetData(data);
    clone->SetInputType(inputType);
    clone->SetComposing(isComposing);
    clone->SetBubbles(bubbles);
    clone->SetCancelable(cancelable);
    return clone;
}

std::string InputEvent::ToJSON() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"" << EscapeJSON(typeString) << "\"";
    oss << ",\"data\":\"" << EscapeJSON(data) << "\"";
    oss << ",\"inputType\":\"" << EscapeJSON(inputType) << "\"";
    oss << ",\"isComposing\":" << (isComposing ? "true" : "false");
    oss << ",\"bubbles\":" << (bubbles ? "true" : "false");
    oss << ",\"cancelable\":" << (cancelable ? "true" : "false");
    oss << ",\"timeStamp\":" << std::fixed << std::setprecision(3) << timeStamp;
    if (target) {
        oss << ",\"targetId\":" << target->GetId();
    }
    oss << "}";
    return oss.str();
}

// ============================================================================
// FOCUS EVENT IMPLEMENTATION
// ============================================================================

FocusEvent::FocusEvent(EventType type)
    : Event(type)
    , relatedTarget(nullptr) {
}

std::shared_ptr<Event> FocusEvent::Clone() const {
    auto clone = std::make_shared<FocusEvent>(type);
    clone->SetRelatedTarget(relatedTarget);
    clone->SetBubbles(bubbles);
    clone->SetCancelable(cancelable);
    return clone;
}

std::string FocusEvent::ToJSON() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"" << EscapeJSON(typeString) << "\"";
    oss << ",\"bubbles\":" << (bubbles ? "true" : "false");
    oss << ",\"cancelable\":" << (cancelable ? "true" : "false");
    oss << ",\"timeStamp\":" << std::fixed << std::setprecision(3) << timeStamp;
    if (target) {
        oss << ",\"targetId\":" << target->GetId();
    }
    if (relatedTarget) {
        oss << ",\"relatedTargetId\":" << relatedTarget->GetId();
    }
    oss << "}";
    return oss.str();
}

// ============================================================================
// WHEEL EVENT IMPLEMENTATION
// ============================================================================

WheelEvent::WheelEvent()
    : MouseEvent(EventType::Wheel)
    , deltaX(0), deltaY(0), deltaZ(0)
    , deltaMode(DeltaMode::Pixel) {
}

std::shared_ptr<Event> WheelEvent::Clone() const {
    auto clone = std::make_shared<WheelEvent>();
    clone->SetClientPos(GetClientX(), GetClientY());
    clone->SetDelta(deltaX, deltaY, deltaZ);
    clone->SetDeltaMode(deltaMode);
    clone->SetModifiers(GetAltKey(), GetCtrlKey(), GetShiftKey(), GetMetaKey());
    clone->SetBubbles(bubbles);
    clone->SetCancelable(cancelable);
    return clone;
}

std::string WheelEvent::ToJSON() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"wheel\"";
    oss << ",\"deltaX\":" << deltaX;
    oss << ",\"deltaY\":" << deltaY;
    oss << ",\"deltaZ\":" << deltaZ;
    oss << ",\"deltaMode\":" << static_cast<int>(deltaMode);
    oss << ",\"clientX\":" << GetClientX();
    oss << ",\"clientY\":" << GetClientY();
    oss << ",\"altKey\":" << (GetAltKey() ? "true" : "false");
    oss << ",\"ctrlKey\":" << (GetCtrlKey() ? "true" : "false");
    oss << ",\"shiftKey\":" << (GetShiftKey() ? "true" : "false");
    oss << ",\"metaKey\":" << (GetMetaKey() ? "true" : "false");
    oss << ",\"timeStamp\":" << std::fixed << std::setprecision(3) << timeStamp;
    if (target) {
        oss << ",\"targetId\":" << target->GetId();
    }
    oss << "}";
    return oss.str();
}

// ============================================================================
// CUSTOM EVENT IMPLEMENTATION
// ============================================================================

CustomEvent::CustomEvent(const std::string& eventType)
    : Event(EventType::Custom, eventType)
    , detail(nullptr) {
}

std::shared_ptr<Event> CustomEvent::Clone() const {
    auto clone = std::make_shared<CustomEvent>(typeString);
    clone->SetDetail(detail);
    clone->SetBubbles(bubbles);
    clone->SetCancelable(cancelable);
    return clone;
}

std::string CustomEvent::ToJSON() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"" << EscapeJSON(typeString) << "\"";
    
    // Serialize detail based on variant type
    oss << ",\"detail\":";
    if (std::holds_alternative<std::nullptr_t>(detail)) {
        oss << "null";
    } else if (std::holds_alternative<bool>(detail)) {
        oss << (std::get<bool>(detail) ? "true" : "false");
    } else if (std::holds_alternative<int>(detail)) {
        oss << std::get<int>(detail);
    } else if (std::holds_alternative<double>(detail)) {
        oss << std::get<double>(detail);
    } else if (std::holds_alternative<std::string>(detail)) {
        oss << "\"" << EscapeJSON(std::get<std::string>(detail)) << "\"";
    } else if (std::holds_alternative<std::vector<std::string>>(detail)) {
        const auto& vec = std::get<std::vector<std::string>>(detail);
        oss << "[";
        for (size_t i = 0; i < vec.size(); i++) {
            if (i > 0) oss << ",";
            oss << "\"" << EscapeJSON(vec[i]) << "\"";
        }
        oss << "]";
    }
    
    oss << ",\"bubbles\":" << (bubbles ? "true" : "false");
    oss << ",\"cancelable\":" << (cancelable ? "true" : "false");
    oss << ",\"timeStamp\":" << std::fixed << std::setprecision(3) << timeStamp;
    if (target) {
        oss << ",\"targetId\":" << target->GetId();
    }
    oss << "}";
    return oss.str();
}

// ============================================================================
// EVENT TARGET IMPLEMENTATION
// ============================================================================

EventTarget::EventTarget()
    : nextListenerId(1) {
}

uint32_t EventTarget::AddEventListener(EventType type, EventCallback callback,
                                        const EventListenerOptions& options) {
    uint32_t id = nextListenerId++;
    listeners[type].push_back(EventListener(callback, options, id));
    return id;
}

uint32_t EventTarget::AddEventListener(const std::string& type, EventCallback callback,
                                        const EventListenerOptions& options) {
    return AddEventListener(StringToEventType(type), callback, options);
}

void EventTarget::RemoveEventListener(EventType type, uint32_t listenerId) {
    auto it = listeners.find(type);
    if (it != listeners.end()) {
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [listenerId](const EventListener& l) { return l.id == listenerId; }),
            vec.end());
    }
}

void EventTarget::RemoveEventListener(const std::string& type, uint32_t listenerId) {
    RemoveEventListener(StringToEventType(type), listenerId);
}

void EventTarget::RemoveAllEventListeners(EventType type) {
    listeners.erase(type);
}

void EventTarget::RemoveAllEventListeners() {
    listeners.clear();
}

void EventTarget::BindHandler(EventType type, const std::string& handlerName,
                               const EventListenerOptions& options) {
    uint32_t id = nextListenerId++;
    EventListener listener;
    listener.options = options;
    listener.handlerName = handlerName;
    listener.id = id;
    // Callback will be set by the event system when JS bridge is connected
    listeners[type].push_back(listener);
}

bool EventTarget::DispatchEvent(Event& event) {
    InvokeListeners(event);
    return !event.IsDefaultPrevented();
}

const std::vector<EventListener>& EventTarget::GetListeners(EventType type) const {
    static const std::vector<EventListener> empty;
    auto it = listeners.find(type);
    return (it != listeners.end()) ? it->second : empty;
}

bool EventTarget::HasListeners(EventType type) const {
    auto it = listeners.find(type);
    return it != listeners.end() && !it->second.empty();
}

void EventTarget::InvokeListeners(Event& event) {
    auto it = listeners.find(event.GetType());
    if (it == listeners.end()) return;
    
    std::vector<uint32_t> toRemove;
    
    for (auto& listener : it->second) {
        if (event.IsImmediatePropagationStopped()) break;
        
        // Check capture/bubble phase match
        bool inCapture = (event.GetPhase() == EventPhase::Capturing);
        bool atTarget = (event.GetPhase() == EventPhase::AtTarget);
        
        if (!atTarget) {
            if (inCapture && !listener.options.capture) continue;
            if (!inCapture && listener.options.capture) continue;
        }
        
        // Invoke callback
        if (listener.callback) {
            listener.callback(event);
        }
        
        // Mark for removal if once
        if (listener.options.once) {
            toRemove.push_back(listener.id);
        }
    }
    
    // Remove once listeners
    for (uint32_t id : toRemove) {
        RemoveEventListener(event.GetType(), id);
    }
}

// ============================================================================
// EVENT DISPATCHER IMPLEMENTATION
// ============================================================================

EventDispatcher::EventDispatcher()
    : stats{0, 0, 0} {
}

bool EventDispatcher::Dispatch(Event& event, UCComponent* target) {
    if (!target) return false;
    
    stats.totalDispatched++;
    
    event.SetTarget(target);
    
    // Build propagation path
    std::vector<UCComponent*> path = BuildPath(target);
    
    // Capturing phase (root to target-1)
    if (!path.empty()) {
        DispatchAtPhase(event, path, EventPhase::Capturing);
    }
    
    if (!event.IsPropagationStopped()) {
        // At target
        event.SetPhase(EventPhase::AtTarget);
        event.SetCurrentTarget(target);
        
        // Find EventTarget interface - for now, treat UCComponent as EventTarget
        // In a full implementation, UCComponent would inherit from EventTarget
        // Here we dispatch using the UCComponent's own event system
        UCEvent ucEvent;
        ucEvent.type = static_cast<UCEventType>(event.GetType());
        ucEvent.target = target;
        target->DispatchEvent(ucEvent);
    }
    
    // Bubbling phase (target+1 to root)
    if (event.Bubbles() && !event.IsPropagationStopped()) {
        DispatchAtPhase(event, path, EventPhase::Bubbling);
    }
    
    if (event.IsPropagationStopped()) stats.propagationsStopped++;
    if (event.IsDefaultPrevented()) stats.defaultsPrevented++;
    
    return !event.IsDefaultPrevented();
}

std::vector<UCComponent*> EventDispatcher::BuildPath(UCComponent* target) {
    std::vector<UCComponent*> path;
    UCComponent* current = target->GetParent();
    
    while (current) {
        path.push_back(current);
        current = current->GetParent();
    }
    
    // Reverse so it goes from root to parent-of-target
    std::reverse(path.begin(), path.end());
    
    return path;
}

void EventDispatcher::DispatchAtPhase(Event& event, 
                                       const std::vector<UCComponent*>& path,
                                       EventPhase phase) {
    event.SetPhase(phase);
    
    if (phase == EventPhase::Capturing) {
        // Root to target
        for (auto* comp : path) {
            if (event.IsPropagationStopped()) break;
            event.SetCurrentTarget(comp);
            
            UCEvent ucEvent;
            ucEvent.type = static_cast<UCEventType>(event.GetType());
            ucEvent.target = comp;
            comp->DispatchEvent(ucEvent);
        }
    } else if (phase == EventPhase::Bubbling) {
        // Parent to root (reverse iteration)
        for (auto it = path.rbegin(); it != path.rend(); ++it) {
            if (event.IsPropagationStopped()) break;
            event.SetCurrentTarget(*it);
            
            UCEvent ucEvent;
            ucEvent.type = static_cast<UCEventType>(event.GetType());
            ucEvent.target = *it;
            (*it)->DispatchEvent(ucEvent);
        }
    }
}

// ============================================================================
// JS EVENT BRIDGE IMPLEMENTATION
// ============================================================================

JSEventBridge::JSEventBridge() {
}

void JSEventBridge::SetEventHandler(JSEventHandler handler) {
    jsHandler = handler;
}

uint64_t JSEventBridge::MakeKey(uint16_t elementId, EventType type) const {
    return (static_cast<uint64_t>(elementId) << 32) | static_cast<uint32_t>(type);
}

void JSEventBridge::RegisterHandler(uint16_t elementId, EventType type,
                                     const std::string& handlerName) {
    uint64_t key = MakeKey(elementId, type);
    handlerMap[key] = handlerName;
}

void JSEventBridge::UnregisterHandler(uint16_t elementId, EventType type) {
    uint64_t key = MakeKey(elementId, type);
    handlerMap.erase(key);
}

void JSEventBridge::OnEvent(uint16_t elementId, Event& event) {
    uint64_t key = MakeKey(elementId, event.GetType());
    auto it = handlerMap.find(key);
    
    if (it != handlerMap.end()) {
        JSEventData data;
        data.elementId = elementId;
        data.eventType = EventTypeToString(event.GetType());
        data.handlerName = it->second;
        data.eventJSON = event.ToJSON();
        data.timestamp = event.GetTimeStamp();
        
        if (jsHandler) {
            jsHandler(data);
        } else {
            QueueEvent(data);
        }
    }
}

void JSEventBridge::QueueEvent(const JSEventData& data) {
    eventQueue.push(data);
}

std::vector<JSEventData> JSEventBridge::FlushQueue() {
    std::vector<JSEventData> result;
    while (!eventQueue.empty()) {
        result.push_back(eventQueue.front());
        eventQueue.pop();
    }
    return result;
}

std::shared_ptr<MouseEvent> JSEventBridge::CreateMouseEvent(EventType type, 
                                                             float x, float y) {
    auto event = std::make_shared<MouseEvent>(type);
    event->SetClientPos(x, y);
    event->SetPagePos(x, y);
    event->SetTrusted(false);
    return event;
}

std::shared_ptr<KeyboardEvent> JSEventBridge::CreateKeyboardEvent(EventType type,
                                                                   const std::string& key,
                                                                   int keyCode) {
    auto event = std::make_shared<KeyboardEvent>(type);
    event->SetKey(key);
    event->SetKeyCode(keyCode);
    event->SetTrusted(false);
    return event;
}

std::shared_ptr<InputEvent> JSEventBridge::CreateInputEvent(const std::string& data) {
    auto event = std::make_shared<InputEvent>(EventType::Input);
    event->SetData(data);
    event->SetTrusted(false);
    return event;
}

std::shared_ptr<CustomEvent> JSEventBridge::CreateCustomEvent(const std::string& type) {
    auto event = std::make_shared<CustomEvent>(type);
    event->SetTrusted(false);
    return event;
}

// ============================================================================
// EVENT SYSTEM IMPLEMENTATION
// ============================================================================

EventSystem::EventSystem()
    : focusedComponent(nullptr)
    , hoveredComponent(nullptr)
    , pressedComponent(nullptr)
    , altDown(false), ctrlDown(false), shiftDown(false), metaDown(false) {
}

EventSystem::~EventSystem() {
}

EventSystem& EventSystem::Instance() {
    static EventSystem instance;
    return instance;
}

bool EventSystem::FireEvent(UCComponent* target, std::shared_ptr<Event> event) {
    if (!target || !event) return false;
    return dispatcher.Dispatch(*event, target);
}

bool EventSystem::FireClick(UCComponent* target, float x, float y) {
    return FireMouseEvent(target, EventType::Click, x, y);
}

bool EventSystem::FireMouseEvent(UCComponent* target, EventType type, float x, float y) {
    auto event = std::make_shared<MouseEvent>(type);
    event->SetClientPos(x, y);
    event->SetPagePos(x, y);
    event->SetModifiers(altDown, ctrlDown, shiftDown, metaDown);
    
    // Calculate offset relative to target
    if (target) {
        const UCRect& frame = target->GetFrame();
        event->SetOffsetPos(x - frame.x, y - frame.y);
    }
    
    bool result = dispatcher.Dispatch(*event, target);
    
    // Notify JS bridge
    if (target) {
        jsBridge.OnEvent(target->GetId(), *event);
    }
    
    return result;
}

bool EventSystem::FireKeyEvent(UCComponent* target, EventType type,
                                const std::string& key, int keyCode) {
    auto event = std::make_shared<KeyboardEvent>(type);
    event->SetKey(key);
    event->SetKeyCode(keyCode);
    event->SetModifiers(altDown, ctrlDown, shiftDown, metaDown);
    
    // If no target, use focused component
    UCComponent* actualTarget = target ? target : focusedComponent;
    if (!actualTarget) return false;
    
    bool result = dispatcher.Dispatch(*event, actualTarget);
    
    jsBridge.OnEvent(actualTarget->GetId(), *event);
    
    return result;
}

bool EventSystem::FireInputEvent(UCComponent* target, const std::string& data) {
    auto event = std::make_shared<InputEvent>(EventType::Input);
    event->SetData(data);
    
    UCComponent* actualTarget = target ? target : focusedComponent;
    if (!actualTarget) return false;
    
    bool result = dispatcher.Dispatch(*event, actualTarget);
    
    jsBridge.OnEvent(actualTarget->GetId(), *event);
    
    return result;
}

bool EventSystem::FireFocusEvent(UCComponent* target, EventType type,
                                  UCComponent* relatedTarget) {
    auto event = std::make_shared<FocusEvent>(type);
    event->SetRelatedTarget(relatedTarget);
    
    if (!target) return false;
    
    bool result = dispatcher.Dispatch(*event, target);
    
    jsBridge.OnEvent(target->GetId(), *event);
    
    return result;
}

bool EventSystem::FireCustomEvent(UCComponent* target, const std::string& eventType) {
    auto event = std::make_shared<CustomEvent>(eventType);
    
    if (!target) return false;
    
    bool result = dispatcher.Dispatch(*event, target);
    
    jsBridge.OnEvent(target->GetId(), *event);
    
    return result;
}

void EventSystem::SetFocus(UCComponent* component) {
    if (focusedComponent == component) return;
    
    UCComponent* oldFocus = focusedComponent;
    
    // Blur old
    if (oldFocus) {
        oldFocus->SetFocused(false);
        FireFocusEvent(oldFocus, EventType::Blur, component);
        FireFocusEvent(oldFocus, EventType::FocusOut, component);
    }
    
    focusedComponent = component;
    
    // Focus new
    if (component) {
        component->SetFocused(true);
        FireFocusEvent(component, EventType::Focus, oldFocus);
        FireFocusEvent(component, EventType::FocusIn, oldFocus);
    }
}

void EventSystem::UpdateHover(UCComponent* component) {
    if (hoveredComponent == component) return;
    
    UCComponent* oldHover = hoveredComponent;
    
    // Mouse leave old
    if (oldHover) {
        oldHover->SetHovered(false);
        
        // Find common ancestor to know where to stop events
        // For simplicity, fire on old component only
        FireMouseEvent(oldHover, EventType::MouseLeave, 0, 0);
        FireMouseEvent(oldHover, EventType::MouseOut, 0, 0);
    }
    
    hoveredComponent = component;
    
    // Mouse enter new
    if (component) {
        component->SetHovered(true);
        FireMouseEvent(component, EventType::MouseEnter, 0, 0);
        FireMouseEvent(component, EventType::MouseOver, 0, 0);
    }
}

void EventSystem::SetPressed(UCComponent* component) {
    pressedComponent = component;
    if (component) {
        component->SetPressed(true);
    }
}

void EventSystem::ReleasePressed() {
    if (pressedComponent) {
        pressedComponent->SetPressed(false);
        pressedComponent = nullptr;
    }
}

bool EventSystem::IsKeyDown(int keyCode) const {
    return keysDown.find(keyCode) != keysDown.end();
}

bool EventSystem::IsMouseButtonDown(int button) const {
    return mouseButtonsDown.find(button) != mouseButtonsDown.end();
}

void EventSystem::SetKeyState(int keyCode, bool down) {
    if (down) {
        keysDown.insert(keyCode);
    } else {
        keysDown.erase(keyCode);
    }
}

void EventSystem::SetMouseButtonState(int button, bool down) {
    if (down) {
        mouseButtonsDown.insert(button);
    } else {
        mouseButtonsDown.erase(button);
    }
}

void EventSystem::SetModifierState(bool alt, bool ctrl, bool shift, bool meta) {
    altDown = alt;
    ctrlDown = ctrl;
    shiftDown = shift;
    metaDown = meta;
}

} // namespace Runtime
} // namespace UltraWeb
