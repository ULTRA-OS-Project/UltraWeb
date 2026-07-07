// UltraWeb/runtime/StateManager.cpp
// Reactive state store implementation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "StateManager.h"

#include <algorithm>

namespace UltraWeb {
namespace Runtime {

uint32_t StateManager::CreateState(const std::string& initialJson) {
    uint32_t id = nextId++;
    states[id] = initialJson.empty() ? "null" : initialJson;
    return id;
}

std::string StateManager::GetState(uint32_t stateId) const {
    auto it = states.find(stateId);
    return it != states.end() ? it->second : "null";
}

bool StateManager::SetState(uint32_t stateId, const std::string& valueJson) {
    auto it = states.find(stateId);
    if (it == states.end()) return false;

    it->second = valueJson.empty() ? "null" : valueJson;
    ApplyBindings(stateId, it->second);
    if (listener) listener(stateId, it->second);
    return true;
}

bool StateManager::Bind(uint32_t stateId, uint16_t elementId,
                        const std::string& property) {
    auto it = states.find(stateId);
    if (it == states.end()) return false;

    auto& list = bindings[stateId];
    for (const auto& b : list) {
        if (b.elementId == elementId && b.property == property) return true;
    }
    list.push_back({elementId, property});

    // Apply current value immediately
    if (applier) applier(elementId, property, it->second);
    return true;
}

void StateManager::Unbind(uint32_t stateId, uint16_t elementId,
                          const std::string& property) {
    auto it = bindings.find(stateId);
    if (it == bindings.end()) return;
    auto& list = it->second;
    list.erase(std::remove_if(list.begin(), list.end(),
                              [&](const Binding& b) {
                                  return b.elementId == elementId &&
                                         b.property == property;
                              }),
               list.end());
}

void StateManager::Clear() {
    states.clear();
    bindings.clear();
    nextId = 1;
}

void StateManager::ApplyBindings(uint32_t stateId, const std::string& valueJson) {
    if (!applier) return;
    auto it = bindings.find(stateId);
    if (it == bindings.end()) return;
    for (const auto& b : it->second) {
        applier(b.elementId, b.property, valueJson);
    }
}

} // namespace Runtime
} // namespace UltraWeb
