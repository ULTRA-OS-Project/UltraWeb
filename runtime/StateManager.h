// UltraWeb/runtime/StateManager.h
// Reactive state store backing UC.useState / UC.computed / UC.effect
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Native side of the reactive state system. State values are stored as JSON
// text (the marshalling currency of the JS bridge). JS-side effects and
// computed values live in the UC prelude; this store keeps the values,
// applies element property bindings, and notifies a change listener so
// UCApi can run JS subscribers. Keeping values native means the server
// delta protocol (Phase 4) can diff state without entering the JS engine.

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace UltraWeb {
namespace Runtime {

class StateManager {
public:
    // Applies a bound state value to an element property ("text", "value",
    // "visible"). Wired to UltraWebRuntime by UCApi.
    using BindingApplier = std::function<void(uint16_t elementId,
                                              const std::string& property,
                                              const std::string& valueJson)>;

    // Notified after every SetState (UCApi runs JS effects from here)
    using ChangeListener = std::function<void(uint32_t stateId,
                                              const std::string& valueJson)>;

    // Creates a state slot; returns its id
    uint32_t CreateState(const std::string& initialJson);

    // Returns the JSON value ("null" for unknown ids)
    std::string GetState(uint32_t stateId) const;

    // Replaces the value, applies bindings, fires the change listener.
    // Returns false for unknown ids.
    bool SetState(uint32_t stateId, const std::string& valueJson);

    // Binds a state slot to an element property; the current value is
    // applied immediately and on every subsequent SetState
    bool Bind(uint32_t stateId, uint16_t elementId, const std::string& property);
    void Unbind(uint32_t stateId, uint16_t elementId, const std::string& property);

    void SetBindingApplier(BindingApplier applier) { this->applier = std::move(applier); }
    void SetChangeListener(ChangeListener listener) { this->listener = std::move(listener); }

    size_t GetStateCount() const { return states.size(); }
    void Clear();

private:
    struct Binding {
        uint16_t elementId;
        std::string property;
    };

    std::unordered_map<uint32_t, std::string> states;
    std::unordered_map<uint32_t, std::vector<Binding>> bindings;
    uint32_t nextId = 1;

    BindingApplier applier;
    ChangeListener listener;

    void ApplyBindings(uint32_t stateId, const std::string& valueJson);
};

} // namespace Runtime
} // namespace UltraWeb
