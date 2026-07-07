// UltraWeb/runtime/UCApi.h
// UC.* JavaScript API - native dispatcher and JS prelude
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Implements the UltraWeb JavaScript API (spec: "JavaScript API" section)
// on top of the JSEngine abstraction. All JS calls funnel through
// __uc_native(functionName, argsJson); UCApi::Dispatch routes them to
// UltraWebRuntime element operations and the StateManager. The UC global
// object itself (element wrappers, useState/computed/effect, event
// helpers) is plain JavaScript - GetPreludeSource() - evaluated into the
// engine when the runtime attaches it.
//
// Phase 3 scope: element access/manipulation, styling classes, events,
// reactive state. UC.fetch / UC.websocket arrive with the server framework
// (Phase 4); UC.router / UC.storage with Phase 5 - their prelude stubs
// throw descriptive errors.

#pragma once

#include "JSEngine.h"
#include "JSONUtil.h"
#include "StateManager.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace UltraWeb {
namespace Runtime {

class UltraWebRuntime;

class UCApi {
public:
    UCApi(UltraWebRuntime& runtime, JSEngine& engine);

    // The dispatcher entry point: routes __uc_native calls. Returns JSON.
    std::string Dispatch(const std::string& functionName,
                         const std::string& argsJson);

    // JS source of the UC global object (evaluated before application code)
    static std::string GetPreludeSource();

    // Fires a DOM-style event registered from JS (element.on(...)).
    // eventJSON is the event payload ({"x":..,"y":..} etc.); returns true
    // if at least one JS handler ran.
    bool FireEvent(uint16_t elementId, const std::string& eventType,
                   const std::string& eventJSON);

    // True if any JS handler is registered for (element, eventType)
    bool HasHandler(uint16_t elementId, const std::string& eventType) const;

    StateManager& GetStateManager() { return stateManager; }

    // Console output sink (defaults to stdout); tests can capture it
    using ConsoleSink = std::function<void(const std::string& line)>;
    void SetConsoleSink(ConsoleSink sink) { consoleSink = std::move(sink); }

private:
    UltraWebRuntime& runtime;
    JSEngine& engine;
    StateManager stateManager;
    ConsoleSink consoleSink;

    // (elementId, eventType) -> registered (JS keeps the callbacks; we only
    // track that a handler exists so events are forwarded)
    std::unordered_map<uint64_t, uint32_t> handlerCounts;

    static uint64_t HandlerKey(uint16_t elementId, const std::string& eventType);

    // Dispatch handlers (args is the parsed JSON argument array)
    JSONValue OnGetElementById(const JSONValue& args);
    JSONValue OnGetElementsByClassName(const JSONValue& args);
    JSONValue OnGetText(const JSONValue& args);
    JSONValue OnSetText(const JSONValue& args);
    JSONValue OnGetValue(const JSONValue& args);
    JSONValue OnSetValue(const JSONValue& args);
    JSONValue OnClassOp(const std::string& op, const JSONValue& args);
    JSONValue OnSetVisible(const JSONValue& args);
    JSONValue OnIsVisible(const JSONValue& args);
    JSONValue OnSetEnabled(const JSONValue& args);
    JSONValue OnGetBounds(const JSONValue& args);
    JSONValue OnEventOn(const JSONValue& args);
    JSONValue OnEventOff(const JSONValue& args);
    JSONValue OnStateCreate(const JSONValue& args);
    JSONValue OnStateGet(const JSONValue& args);
    JSONValue OnStateSet(const JSONValue& args);
    JSONValue OnStateBind(const JSONValue& args);
    JSONValue OnConsoleLog(const JSONValue& args);
};

} // namespace Runtime
} // namespace UltraWeb
