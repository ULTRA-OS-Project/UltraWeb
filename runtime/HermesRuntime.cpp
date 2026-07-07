// UltraWeb/runtime/HermesRuntime.cpp
// Hermes VM wrapper implementation (JSI)
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#ifdef ULTRAWEB_USE_HERMES

#include "HermesRuntime.h"

#include <hermes/hermes.h>
#include <jsi/jsi.h>

#include <cstring>

namespace UltraWeb {
namespace Runtime {

namespace jsi = facebook::jsi;

namespace {

// jsi::Buffer view over a byte vector (copied, so lifetime is owned here)
class BytesBuffer : public jsi::Buffer {
public:
    explicit BytesBuffer(std::vector<uint8_t> bytes) : bytes_(std::move(bytes)) {}
    explicit BytesBuffer(const std::string& s) : bytes_(s.begin(), s.end()) {}
    size_t size() const override { return bytes_.size(); }
    const uint8_t* data() const override { return bytes_.data(); }
private:
    std::vector<uint8_t> bytes_;
};

// JSON-stringify a jsi::Value using the engine's own JSON object
std::string ToJSONText(jsi::Runtime& rt, const jsi::Value& v) {
    if (v.isUndefined()) return "null";
    jsi::Object json = rt.global().getPropertyAsObject(rt, "JSON");
    jsi::Function stringify = json.getPropertyAsFunction(rt, "stringify");
    jsi::Value out = stringify.call(rt, v);
    if (!out.isString()) return "null";
    return out.getString(rt).utf8(rt);
}

// Parse JSON text into a jsi::Value
jsi::Value FromJSONText(jsi::Runtime& rt, const std::string& text) {
    jsi::Object json = rt.global().getPropertyAsObject(rt, "JSON");
    jsi::Function parse = json.getPropertyAsFunction(rt, "parse");
    return parse.call(rt, jsi::String::createFromUtf8(rt, text));
}

} // namespace

struct HermesEngine::Impl {
    std::unique_ptr<facebook::hermes::HermesRuntime> runtime;
    NativeDispatcher dispatcher;
};

HermesEngine::HermesEngine() : impl(new Impl()) {
    impl->runtime = facebook::hermes::makeHermesRuntime();
    jsi::Runtime& rt = *impl->runtime;

    // global.__uc_native(functionName, argsJson) -> result (parsed value)
    Impl* implPtr = impl.get();
    auto ucNative = jsi::Function::createFromHostFunction(
        rt, jsi::PropNameID::forAscii(rt, "__uc_native"), 2,
        [implPtr](jsi::Runtime& rt, const jsi::Value&,
                  const jsi::Value* args, size_t count) -> jsi::Value {
            if (!implPtr->dispatcher) {
                throw jsi::JSError(rt, "__uc_native: no dispatcher attached");
            }
            if (count < 1 || !args[0].isString()) {
                throw jsi::JSError(rt, "__uc_native: function name required");
            }
            std::string functionName = args[0].getString(rt).utf8(rt);
            std::string argsJson = "[]";
            if (count >= 2 && args[1].isString()) {
                argsJson = args[1].getString(rt).utf8(rt);
            }
            std::string result = implPtr->dispatcher(functionName, argsJson);
            if (result.empty()) result = "null";
            return FromJSONText(rt, result);
        });
    rt.global().setProperty(rt, "__uc_native", ucNative);
}

HermesEngine::~HermesEngine() = default;

void HermesEngine::SetNativeDispatcher(NativeDispatcher dispatcher) {
    impl->dispatcher = std::move(dispatcher);
}

bool HermesEngine::EvaluateSource(const std::string& source,
                                  const std::string& sourceName,
                                  std::string& error) {
    try {
        impl->runtime->evaluateJavaScript(
            std::make_shared<BytesBuffer>(source), sourceName);
        return true;
    } catch (const jsi::JSError& e) {
        error = e.getMessage() + "\n" + e.getStack();
        return false;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

bool HermesEngine::EvaluateBytecode(const std::vector<uint8_t>& bytecode,
                                    std::string& error) {
    if (!facebook::hermes::HermesRuntime::isHermesBytecode(
            bytecode.data(), bytecode.size())) {
        error = "buffer is not Hermes bytecode (bad magic)";
        return false;
    }
    try {
        impl->runtime->evaluateJavaScript(
            std::make_shared<BytesBuffer>(bytecode), "<bytecode>");
        return true;
    } catch (const jsi::JSError& e) {
        error = e.getMessage() + "\n" + e.getStack();
        return false;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

bool HermesEngine::CallGlobalFunction(const std::string& name,
                                      const std::string& argsJson,
                                      std::string& resultJson,
                                      std::string& error) {
    resultJson = "null";
    jsi::Runtime& rt = *impl->runtime;
    try {
        jsi::Value fnVal = rt.global().getProperty(rt, name.c_str());
        if (!fnVal.isObject() || !fnVal.getObject(rt).isFunction(rt)) {
            error = "global function not found: " + name;
            return false;
        }
        jsi::Function fn = fnVal.getObject(rt).getFunction(rt);

        jsi::Value argsVal = FromJSONText(rt, argsJson);
        if (!argsVal.isObject() || !argsVal.getObject(rt).isArray(rt)) {
            error = "invalid argument JSON for " + name;
            return false;
        }
        jsi::Array argsArr = argsVal.getObject(rt).getArray(rt);
        size_t len = argsArr.size(rt);

        std::vector<jsi::Value> args;
        args.reserve(len);
        for (size_t i = 0; i < len; i++) {
            args.push_back(argsArr.getValueAtIndex(rt, i));
        }

        jsi::Value result = fn.call(
            rt, static_cast<const jsi::Value*>(args.data()), args.size());
        resultJson = ToJSONText(rt, result);
        return true;
    } catch (const jsi::JSError& e) {
        error = e.getMessage() + "\n" + e.getStack();
        return false;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

} // namespace Runtime
} // namespace UltraWeb

#endif // ULTRAWEB_USE_HERMES
