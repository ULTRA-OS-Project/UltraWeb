// UltraWeb/runtime/JSEngine.h
// JavaScript engine abstraction - Hermes in production, QuickJS for dev/test
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// UltraWeb code never talks to a JS engine directly; it programs against
// this interface. Backends:
//   HermesEngine  (HermesRuntime.h, ULTRAWEB_USE_HERMES)  - executes .hbc
//     bytecode, the production engine per the UltraWeb spec
//   QuickJSEngine (QuickJSEngine.h, ULTRAWEB_USE_QUICKJS) - executes plain
//     JS source, used for development and CI where the Hermes SDK is not
//     available (the spec's risk table anticipates an engine fallback)
//
// Marshalling model: everything crossing the C++/JS boundary is JSON text.
// JS reaches native code through a single global host function
//   __uc_native(functionName, argsJson) -> resultJson
// which the engine routes to the NativeDispatcher (implemented by UCApi).
// C++ reaches JS by calling a global function with a JSON argument array.
// This keeps each engine binding tiny and identical in behavior.

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace UltraWeb {
namespace Runtime {

class JSEngine {
public:
    virtual ~JSEngine() = default;

    // Engine identification ("Hermes", "QuickJS")
    virtual std::string GetName() const = 0;

    // Receives every __uc_native(functionName, argsJson) call made from JS.
    // Returns the result as JSON text ("null" when there is nothing to say).
    using NativeDispatcher =
        std::function<std::string(const std::string& functionName,
                                  const std::string& argsJson)>;
    virtual void SetNativeDispatcher(NativeDispatcher dispatcher) = 0;

    // Evaluates JavaScript source. sourceName appears in stack traces.
    virtual bool EvaluateSource(const std::string& source,
                                const std::string& sourceName,
                                std::string& error) = 0;

    // Evaluates engine-specific bytecode (.hbc for Hermes). Backends that
    // cannot execute bytecode return false with a clear error.
    virtual bool EvaluateBytecode(const std::vector<uint8_t>& bytecode,
                                  std::string& error) = 0;
    virtual bool SupportsBytecode() const = 0;

    // Calls a global JS function with a JSON array of arguments; the
    // function's return value comes back JSON-encoded in resultJson.
    virtual bool CallGlobalFunction(const std::string& name,
                                    const std::string& argsJson,
                                    std::string& resultJson,
                                    std::string& error) = 0;
};

} // namespace Runtime
} // namespace UltraWeb
