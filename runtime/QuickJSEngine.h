// UltraWeb/runtime/QuickJSEngine.h
// QuickJS backend for the JSEngine abstraction (development / CI engine)
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Executes plain JavaScript source. Does NOT execute Hermes .hbc bytecode -
// production deployments use HermesEngine (HermesRuntime.h). Compiled only
// with ULTRAWEB_USE_QUICKJS.

#pragma once

#ifdef ULTRAWEB_USE_QUICKJS

#include "JSEngine.h"

#include <memory>

namespace UltraWeb {
namespace Runtime {

class QuickJSEngine : public JSEngine {
public:
    QuickJSEngine();
    ~QuickJSEngine() override;

    QuickJSEngine(const QuickJSEngine&) = delete;
    QuickJSEngine& operator=(const QuickJSEngine&) = delete;

    std::string GetName() const override { return "QuickJS"; }

    void SetNativeDispatcher(NativeDispatcher dispatcher) override;

    bool EvaluateSource(const std::string& source,
                        const std::string& sourceName,
                        std::string& error) override;

    bool EvaluateBytecode(const std::vector<uint8_t>& bytecode,
                          std::string& error) override;
    bool SupportsBytecode() const override { return false; }

    bool CallGlobalFunction(const std::string& name,
                            const std::string& argsJson,
                            std::string& resultJson,
                            std::string& error) override;

    // Public so the QuickJS C callback (a free function) can reach it
    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace Runtime
} // namespace UltraWeb

#endif // ULTRAWEB_USE_QUICKJS
