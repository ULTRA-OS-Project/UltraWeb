// UltraWeb/runtime/HermesRuntime.h
// Hermes VM wrapper - the production JSEngine backend
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Executes Hermes bytecode (.hbc) and plain JS source via the JSI API.
// Compiled only with ULTRAWEB_USE_HERMES, which requires the Hermes SDK
// (headers <hermes/hermes.h>, <jsi/jsi.h> and libhermes). Set
// ULTRAWEB_HERMES_SDK_DIR in CMake to the SDK install prefix.

#pragma once

#ifdef ULTRAWEB_USE_HERMES

#include "JSEngine.h"

#include <memory>

namespace UltraWeb {
namespace Runtime {

class HermesEngine : public JSEngine {
public:
    HermesEngine();
    ~HermesEngine() override;

    HermesEngine(const HermesEngine&) = delete;
    HermesEngine& operator=(const HermesEngine&) = delete;

    std::string GetName() const override { return "Hermes"; }

    void SetNativeDispatcher(NativeDispatcher dispatcher) override;

    bool EvaluateSource(const std::string& source,
                        const std::string& sourceName,
                        std::string& error) override;

    bool EvaluateBytecode(const std::vector<uint8_t>& bytecode,
                          std::string& error) override;
    bool SupportsBytecode() const override { return true; }

    bool CallGlobalFunction(const std::string& name,
                            const std::string& argsJson,
                            std::string& resultJson,
                            std::string& error) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Runtime
} // namespace UltraWeb

#endif // ULTRAWEB_USE_HERMES
