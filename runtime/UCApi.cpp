// UltraWeb/runtime/UCApi.cpp
// UC.* JavaScript API implementation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "UCApi.h"
#include "UltraWebRuntime.h"

#include <algorithm>
#include <cstdio>

namespace UltraWeb {
namespace Runtime {

UCApi::UCApi(UltraWebRuntime& runtime, JSEngine& engine)
    : runtime(runtime), engine(engine) {

    consoleSink = [](const std::string& line) {
        std::printf("[JS] %s\n", line.c_str());
    };

    // State changes run JS effects (subscribers live in the prelude)
    stateManager.SetChangeListener(
        [this](uint32_t stateId, const std::string& valueJson) {
            std::string result, error;
            std::string args = "[" + std::to_string(stateId) + "," + valueJson + "]";
            this->engine.CallGlobalFunction("__uc_onStateChanged", args,
                                            result, error);
        });

    // Bindings write through to runtime elements
    stateManager.SetBindingApplier(
        [this](uint16_t elementId, const std::string& property,
               const std::string& valueJson) {
            bool ok = false;
            JSONValue v = JSONValue::Parse(valueJson, ok);
            std::string text = v.IsString()
                                   ? v.GetString()
                                   : (ok ? v.Serialize() : valueJson);
            if (property == "text") {
                this->runtime.SetElementText(elementId, text);
            } else if (property == "value") {
                this->runtime.SetElementValue(elementId, text);
            } else if (property == "visible") {
                this->runtime.SetElementVisible(elementId, v.GetBool(true));
            }
        });
}

uint64_t UCApi::HandlerKey(uint16_t elementId, const std::string& eventType) {
    uint64_t h = 1469598103934665603ULL; // FNV-1a over the event type
    for (char c : eventType) {
        h ^= static_cast<uint8_t>(c);
        h *= 1099511628211ULL;
    }
    return (static_cast<uint64_t>(elementId) << 48) ^ (h & 0xFFFFFFFFFFFFULL);
}

std::string UCApi::Dispatch(const std::string& fn, const std::string& argsJson) {
    bool ok = false;
    JSONValue args = JSONValue::Parse(argsJson, ok);
    if (!ok || !args.IsArray()) {
        JSONValue err = JSONValue::MakeObject();
        err.Set("error", JSONValue::MakeString("malformed argument JSON"));
        return err.Serialize();
    }

    JSONValue result;
    if      (fn == "element.getById")            result = OnGetElementById(args);
    else if (fn == "element.getByClassName")     result = OnGetElementsByClassName(args);
    else if (fn == "element.getText")            result = OnGetText(args);
    else if (fn == "element.setText")            result = OnSetText(args);
    else if (fn == "element.getValue")           result = OnGetValue(args);
    else if (fn == "element.setValue")           result = OnSetValue(args);
    else if (fn == "element.addClass")           result = OnClassOp("add", args);
    else if (fn == "element.removeClass")        result = OnClassOp("remove", args);
    else if (fn == "element.toggleClass")        result = OnClassOp("toggle", args);
    else if (fn == "element.hasClass")           result = OnClassOp("has", args);
    else if (fn == "element.setVisible")         result = OnSetVisible(args);
    else if (fn == "element.isVisible")          result = OnIsVisible(args);
    else if (fn == "element.setEnabled")         result = OnSetEnabled(args);
    else if (fn == "element.getBounds")          result = OnGetBounds(args);
    else if (fn == "event.on")                   result = OnEventOn(args);
    else if (fn == "event.off")                  result = OnEventOff(args);
    else if (fn == "state.create")               result = OnStateCreate(args);
    else if (fn == "state.get")                  result = OnStateGet(args);
    else if (fn == "state.set")                  result = OnStateSet(args);
    else if (fn == "state.bind")                 result = OnStateBind(args);
    else if (fn == "console.log")                result = OnConsoleLog(args);
    else {
        JSONValue err = JSONValue::MakeObject();
        err.Set("error", JSONValue::MakeString("unknown native function: " + fn));
        return err.Serialize();
    }
    return result.Serialize();
}

// ============================================================================
// ELEMENT ACCESS
// ============================================================================

JSONValue UCApi::OnGetElementById(const JSONValue& args) {
    RuntimeElement* el = runtime.GetElementById(args.At(0).GetString());
    return el ? JSONValue::MakeNumber(el->elementId) : JSONValue::MakeNull();
}

JSONValue UCApi::OnGetElementsByClassName(const JSONValue& args) {
    const std::string& cls = args.At(0).GetString();
    JSONValue out = JSONValue::MakeArray();
    for (const auto& el : runtime.GetElements()) {
        if (std::find(el.classNames.begin(), el.classNames.end(), cls) !=
            el.classNames.end()) {
            out.Push(JSONValue::MakeNumber(el.elementId));
        }
    }
    return out;
}

JSONValue UCApi::OnGetText(const JSONValue& args) {
    RuntimeElement* el = runtime.GetElement(
        static_cast<uint16_t>(args.At(0).GetInt()));
    return el ? JSONValue::MakeString(el->textContent) : JSONValue::MakeNull();
}

JSONValue UCApi::OnSetText(const JSONValue& args) {
    runtime.SetElementText(static_cast<uint16_t>(args.At(0).GetInt()),
                           args.At(1).GetString());
    return JSONValue::MakeBool(true);
}

JSONValue UCApi::OnGetValue(const JSONValue& args) {
    RuntimeElement* el = runtime.GetElement(
        static_cast<uint16_t>(args.At(0).GetInt()));
    if (!el) return JSONValue::MakeNull();
    return JSONValue::MakeString(
        el->GetStringProp(UCBPropertyId::Value, el->textContent));
}

JSONValue UCApi::OnSetValue(const JSONValue& args) {
    runtime.SetElementValue(static_cast<uint16_t>(args.At(0).GetInt()),
                            args.At(1).GetString());
    return JSONValue::MakeBool(true);
}

// ============================================================================
// STYLING
// ============================================================================

JSONValue UCApi::OnClassOp(const std::string& op, const JSONValue& args) {
    uint16_t elementId = static_cast<uint16_t>(args.At(0).GetInt());
    const std::string& cls = args.At(1).GetString();

    RuntimeElement* el = runtime.GetElement(elementId);
    if (!el || cls.empty()) return JSONValue::MakeBool(false);

    auto it = std::find(el->classNames.begin(), el->classNames.end(), cls);
    bool has = it != el->classNames.end();

    if (op == "has") return JSONValue::MakeBool(has);

    bool changed = false;
    if (op == "add" && !has) {
        el->classNames.push_back(cls);
        changed = true;
    } else if (op == "remove" && has) {
        el->classNames.erase(it);
        changed = true;
    } else if (op == "toggle") {
        if (has) el->classNames.erase(it);
        else el->classNames.push_back(cls);
        changed = true;
        has = !has;
    }

    if (changed) runtime.RecomputeStyle(elementId);
    return JSONValue::MakeBool(op == "toggle" ? has : changed);
}

// ============================================================================
// VISIBILITY / STATE / LAYOUT
// ============================================================================

JSONValue UCApi::OnSetVisible(const JSONValue& args) {
    runtime.SetElementVisible(static_cast<uint16_t>(args.At(0).GetInt()),
                              args.At(1).GetBool(true));
    return JSONValue::MakeBool(true);
}

JSONValue UCApi::OnIsVisible(const JSONValue& args) {
    RuntimeElement* el = runtime.GetElement(
        static_cast<uint16_t>(args.At(0).GetInt()));
    return JSONValue::MakeBool(el ? el->visible : false);
}

JSONValue UCApi::OnSetEnabled(const JSONValue& args) {
    runtime.SetElementEnabled(static_cast<uint16_t>(args.At(0).GetInt()),
                              args.At(1).GetBool(true));
    return JSONValue::MakeBool(true);
}

JSONValue UCApi::OnGetBounds(const JSONValue& args) {
    RuntimeElement* el = runtime.GetElement(
        static_cast<uint16_t>(args.At(0).GetInt()));
    if (!el) return JSONValue::MakeNull();
    JSONValue out = JSONValue::MakeObject();
    out.Set("x", JSONValue::MakeNumber(el->x));
    out.Set("y", JSONValue::MakeNumber(el->y));
    out.Set("width", JSONValue::MakeNumber(el->layoutWidth));
    out.Set("height", JSONValue::MakeNumber(el->layoutHeight));
    return out;
}

// ============================================================================
// EVENTS
// ============================================================================

JSONValue UCApi::OnEventOn(const JSONValue& args) {
    uint16_t elementId = static_cast<uint16_t>(args.At(0).GetInt());
    const std::string& type = args.At(1).GetString();
    if (!runtime.GetElement(elementId) || type.empty()) {
        return JSONValue::MakeBool(false);
    }
    handlerCounts[HandlerKey(elementId, type)]++;
    return JSONValue::MakeBool(true);
}

JSONValue UCApi::OnEventOff(const JSONValue& args) {
    uint16_t elementId = static_cast<uint16_t>(args.At(0).GetInt());
    const std::string& type = args.At(1).GetString();
    auto it = handlerCounts.find(HandlerKey(elementId, type));
    if (it != handlerCounts.end() && it->second > 0 && --it->second == 0) {
        handlerCounts.erase(it);
    }
    return JSONValue::MakeBool(true);
}

bool UCApi::HasHandler(uint16_t elementId, const std::string& eventType) const {
    auto it = handlerCounts.find(HandlerKey(elementId, eventType));
    return it != handlerCounts.end() && it->second > 0;
}

bool UCApi::FireEvent(uint16_t elementId, const std::string& eventType,
                      const std::string& eventJSON) {
    if (!HasHandler(elementId, eventType)) return false;
    std::string result, error;
    std::string args = "[" + std::to_string(elementId) + "," +
                       JSONValue::Quote(eventType) + "," +
                       (eventJSON.empty() ? "{}" : eventJSON) + "]";
    if (!engine.CallGlobalFunction("__uc_dispatchEvent", args, result, error)) {
        consoleSink("event dispatch error: " + error);
        return false;
    }
    return result == "true";
}

// ============================================================================
// STATE
// ============================================================================

JSONValue UCApi::OnStateCreate(const JSONValue& args) {
    uint32_t id = stateManager.CreateState(args.At(0).Serialize());
    return JSONValue::MakeNumber(id);
}

JSONValue UCApi::OnStateGet(const JSONValue& args) {
    bool ok = false;
    return JSONValue::Parse(
        stateManager.GetState(static_cast<uint32_t>(args.At(0).GetInt())), ok);
}

JSONValue UCApi::OnStateSet(const JSONValue& args) {
    bool set = stateManager.SetState(
        static_cast<uint32_t>(args.At(0).GetInt()), args.At(1).Serialize());
    return JSONValue::MakeBool(set);
}

JSONValue UCApi::OnStateBind(const JSONValue& args) {
    bool bound = stateManager.Bind(
        static_cast<uint32_t>(args.At(0).GetInt()),
        static_cast<uint16_t>(args.At(1).GetInt()),
        args.At(2).GetString());
    return JSONValue::MakeBool(bound);
}

// ============================================================================
// CONSOLE
// ============================================================================

JSONValue UCApi::OnConsoleLog(const JSONValue& args) {
    std::string line;
    for (size_t i = 0; i < args.Size(); i++) {
        if (i) line += ' ';
        const JSONValue& v = args.At(i);
        line += v.IsString() ? v.GetString() : v.Serialize();
    }
    if (consoleSink) consoleSink(line);
    return JSONValue::MakeNull();
}

// ============================================================================
// JS PRELUDE - the UC global object
// ============================================================================

std::string UCApi::GetPreludeSource() {
    // Plain ES5-ish JavaScript so it runs identically on Hermes and QuickJS.
    return R"UCJS(
(function (global) {
    'use strict';

    function native(fn, args) {
        return __uc_native(fn, JSON.stringify(args || []));
    }

    // ----- element wrapper -----
    function UCElement(elementId) { this.elementId = elementId; }

    UCElement.prototype.getText = function () { return native('element.getText', [this.elementId]); };
    UCElement.prototype.setText = function (t) { native('element.setText', [this.elementId, String(t)]); return this; };
    UCElement.prototype.getValue = function () { return native('element.getValue', [this.elementId]); };
    UCElement.prototype.setValue = function (v) { native('element.setValue', [this.elementId, String(v)]); return this; };
    UCElement.prototype.addClass = function (c) { native('element.addClass', [this.elementId, c]); return this; };
    UCElement.prototype.removeClass = function (c) { native('element.removeClass', [this.elementId, c]); return this; };
    UCElement.prototype.toggleClass = function (c) { return native('element.toggleClass', [this.elementId, c]); };
    UCElement.prototype.hasClass = function (c) { return native('element.hasClass', [this.elementId, c]); };
    UCElement.prototype.show = function () { native('element.setVisible', [this.elementId, true]); return this; };
    UCElement.prototype.hide = function () { native('element.setVisible', [this.elementId, false]); return this; };
    UCElement.prototype.isVisible = function () { return native('element.isVisible', [this.elementId]); };
    UCElement.prototype.setEnabled = function (v) { native('element.setEnabled', [this.elementId, !!v]); return this; };
    UCElement.prototype.getBounds = function () { return native('element.getBounds', [this.elementId]); };

    // ----- events (callbacks stay in JS; native only routes) -----
    var eventHandlers = {}; // elementId -> type -> [fn]

    UCElement.prototype.on = function (type, fn) {
        var byType = eventHandlers[this.elementId] || (eventHandlers[this.elementId] = {});
        (byType[type] || (byType[type] = [])).push(fn);
        native('event.on', [this.elementId, type]);
        return this;
    };
    UCElement.prototype.off = function (type, fn) {
        var byType = eventHandlers[this.elementId];
        var list = byType && byType[type];
        if (list) {
            var i = list.indexOf(fn);
            if (i >= 0) { list.splice(i, 1); native('event.off', [this.elementId, type]); }
        }
        return this;
    };
    UCElement.prototype.once = function (type, fn) {
        var self = this;
        function wrapper(e) { self.off(type, wrapper); fn(e); }
        return this.on(type, wrapper);
    };
    UCElement.prototype.bind = function (property, stateOrFn) {
        if (stateOrFn && typeof stateOrFn.__ucStateId === 'number') {
            native('state.bind', [stateOrFn.__ucStateId, this.elementId, property]);
        } else {
            throw new Error('bind() expects a UC state accessor');
        }
        return this;
    };

    // Called from C++ (UCApi::FireEvent); returns true if a handler ran
    global.__uc_dispatchEvent = function (elementId, type, event) {
        var byType = eventHandlers[elementId];
        var list = byType && byType[type];
        if (!list || !list.length) return false;
        // Copy: handlers may add/remove listeners while running
        list.slice().forEach(function (fn) { fn(event); });
        return true;
    };

    // ----- reactive state -----
    var effects = {};   // stateId -> [fn]

    // Called from C++ (StateManager change listener)
    global.__uc_onStateChanged = function (stateId, value) {
        (effects[stateId] || []).forEach(function (fn) { fn(value); });
    };

    function useState(initial) {
        var stateId = native('state.create', [initial === undefined ? null : initial]);
        function get() { return native('state.get', [stateId]); }
        function set(v) { native('state.set', [stateId, v === undefined ? null : v]); }
        get.__ucStateId = stateId;
        set.__ucStateId = stateId;
        return [get, set];
    }

    function effect(fn, states) {
        (states || []).forEach(function (accessor) {
            var id = accessor && accessor.__ucStateId;
            if (typeof id === 'number') (effects[id] || (effects[id] = [])).push(fn);
        });
        fn(); // run once immediately, like the spec's UC.effect
    }

    function computed(fn, states) {
        var pair = useState(fn());
        effect(function () { pair[1](fn()); }, states || []);
        return pair[0];
    }

    // ----- console -----
    if (typeof global.console === 'undefined') global.console = {};
    global.console.log = function () {
        native('console.log', Array.prototype.slice.call(arguments));
    };
    global.console.error = global.console.log;
    global.console.warn = global.console.log;

    // ----- UC global -----
    var UC = {
        getElementById: function (id) {
            var elementId = native('element.getById', [id]);
            return elementId === null ? null : new UCElement(elementId);
        },
        getElementsByClassName: function (cls) {
            return native('element.getByClassName', [cls]).map(function (id) {
                return new UCElement(id);
            });
        },
        useState: useState,
        computed: computed,
        effect: effect,

        // Phase 4 (server framework) / Phase 5 surfaces - explicit stubs
        fetch: function () { throw new Error('UC.fetch arrives with the UltraWeb server framework (Phase 4)'); },
        websocket: function () { throw new Error('UC.websocket arrives with the UltraWeb server framework (Phase 4)'); },
        router: { navigate: function () { throw new Error('UC.router arrives in Phase 5'); } },
        storage: { get: function () { throw new Error('UC.storage arrives in Phase 5'); },
                   set: function () { throw new Error('UC.storage arrives in Phase 5'); } }
    };

    global.UC = UC;
})(typeof globalThis !== 'undefined' ? globalThis : this);
)UCJS";
}

} // namespace Runtime
} // namespace UltraWeb
