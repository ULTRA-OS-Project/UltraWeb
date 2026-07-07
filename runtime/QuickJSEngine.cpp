// UltraWeb/runtime/QuickJSEngine.cpp
// QuickJS backend implementation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#ifdef ULTRAWEB_USE_QUICKJS

#include "QuickJSEngine.h"

#include <quickjs/quickjs.h>

#include <cstring>

namespace UltraWeb {
namespace Runtime {

struct QuickJSEngine::Impl {
    JSRuntime* rt = nullptr;
    JSContext* ctx = nullptr;
    NativeDispatcher dispatcher;

    static std::string GetExceptionText(JSContext* ctx) {
        JSValue exc = JS_GetException(ctx);
        const char* str = JS_ToCString(ctx, exc);
        std::string out = str ? str : "unknown JS exception";
        if (str) JS_FreeCString(ctx, str);
        // Append stack when present
        JSValue stack = JS_GetPropertyStr(ctx, exc, "stack");
        if (!JS_IsUndefined(stack) && !JS_IsException(stack)) {
            const char* s = JS_ToCString(ctx, stack);
            if (s) {
                out += "\n";
                out += s;
                JS_FreeCString(ctx, s);
            }
        }
        JS_FreeValue(ctx, stack);
        JS_FreeValue(ctx, exc);
        return out;
    }

    // JSON-stringify an engine value (used to return results to C++)
    static std::string ToJSONText(JSContext* ctx, JSValueConst v) {
        if (JS_IsUndefined(v)) return "null";
        JSValue jsonStr = JS_JSONStringify(ctx, v, JS_UNDEFINED, JS_UNDEFINED);
        if (JS_IsException(jsonStr) || JS_IsUndefined(jsonStr)) {
            JS_FreeValue(ctx, jsonStr);
            return "null";
        }
        const char* s = JS_ToCString(ctx, jsonStr);
        std::string out = s ? s : "null";
        if (s) JS_FreeCString(ctx, s);
        JS_FreeValue(ctx, jsonStr);
        return out;
    }
};

namespace {

// global.__uc_native(functionName, argsJson) -> resultJson (parsed object)
JSValue UcNative(JSContext* ctx, JSValueConst /*thisVal*/,
                 int argc, JSValueConst* argv) {
    auto* impl = static_cast<QuickJSEngine::Impl*>(
        JS_GetContextOpaque(ctx));
    if (!impl || !impl->dispatcher) {
        return JS_ThrowInternalError(ctx, "__uc_native: no dispatcher attached");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "__uc_native: function name required");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_EXCEPTION;
    std::string functionName = name;
    JS_FreeCString(ctx, name);

    std::string argsJson = "[]";
    if (argc >= 2 && !JS_IsUndefined(argv[1])) {
        const char* args = JS_ToCString(ctx, argv[1]);
        if (args) {
            argsJson = args;
            JS_FreeCString(ctx, args);
        }
    }

    std::string result = impl->dispatcher(functionName, argsJson);
    if (result.empty()) result = "null";

    // Hand the result back as a parsed value so the prelude does not need
    // a second JSON.parse round-trip
    JSValue parsed = JS_ParseJSON(ctx, result.c_str(), result.size(),
                                  "<uc_native_result>");
    if (JS_IsException(parsed)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        return JS_NULL;
    }
    return parsed;
}

} // namespace

QuickJSEngine::QuickJSEngine() : impl(new Impl()) {
    impl->rt = JS_NewRuntime();
    impl->ctx = JS_NewContext(impl->rt);
    JS_SetContextOpaque(impl->ctx, impl.get());

    JSValue global = JS_GetGlobalObject(impl->ctx);
    JS_SetPropertyStr(impl->ctx, global, "__uc_native",
                      JS_NewCFunction(impl->ctx, UcNative, "__uc_native", 2));
    JS_FreeValue(impl->ctx, global);
}

QuickJSEngine::~QuickJSEngine() {
    if (impl->ctx) JS_FreeContext(impl->ctx);
    if (impl->rt) JS_FreeRuntime(impl->rt);
}

void QuickJSEngine::SetNativeDispatcher(NativeDispatcher dispatcher) {
    impl->dispatcher = std::move(dispatcher);
}

bool QuickJSEngine::EvaluateSource(const std::string& source,
                                   const std::string& sourceName,
                                   std::string& error) {
    JSValue result = JS_Eval(impl->ctx, source.c_str(), source.size(),
                             sourceName.c_str(), JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) {
        error = Impl::GetExceptionText(impl->ctx);
        JS_FreeValue(impl->ctx, result);
        return false;
    }
    JS_FreeValue(impl->ctx, result);
    return true;
}

bool QuickJSEngine::EvaluateBytecode(const std::vector<uint8_t>&,
                                     std::string& error) {
    error = "QuickJS backend executes JavaScript source only; "
            "Hermes bytecode (.hbc) requires the Hermes engine "
            "(build with ULTRAWEB_USE_HERMES)";
    return false;
}

bool QuickJSEngine::CallGlobalFunction(const std::string& name,
                                       const std::string& argsJson,
                                       std::string& resultJson,
                                       std::string& error) {
    resultJson = "null";

    JSValue global = JS_GetGlobalObject(impl->ctx);
    JSValue fn = JS_GetPropertyStr(impl->ctx, global, name.c_str());
    if (!JS_IsFunction(impl->ctx, fn)) {
        JS_FreeValue(impl->ctx, fn);
        JS_FreeValue(impl->ctx, global);
        error = "global function not found: " + name;
        return false;
    }

    // Parse the JSON argument array into individual arguments
    JSValue argsVal = JS_ParseJSON(impl->ctx, argsJson.c_str(), argsJson.size(),
                                   "<uc_call_args>");
    if (JS_IsException(argsVal)) {
        JS_FreeValue(impl->ctx, fn);
        JS_FreeValue(impl->ctx, global);
        error = "invalid argument JSON for " + name;
        return false;
    }

    std::vector<JSValue> args;
    JSValue lenVal = JS_GetPropertyStr(impl->ctx, argsVal, "length");
    uint32_t len = 0;
    JS_ToUint32(impl->ctx, &len, lenVal);
    JS_FreeValue(impl->ctx, lenVal);
    for (uint32_t i = 0; i < len; i++) {
        args.push_back(JS_GetPropertyUint32(impl->ctx, argsVal, i));
    }

    JSValue result = JS_Call(impl->ctx, fn, global,
                             static_cast<int>(args.size()), args.data());

    for (JSValue& a : args) JS_FreeValue(impl->ctx, a);
    JS_FreeValue(impl->ctx, argsVal);
    JS_FreeValue(impl->ctx, fn);
    JS_FreeValue(impl->ctx, global);

    if (JS_IsException(result)) {
        error = Impl::GetExceptionText(impl->ctx);
        JS_FreeValue(impl->ctx, result);
        return false;
    }

    resultJson = Impl::ToJSONText(impl->ctx, result);
    JS_FreeValue(impl->ctx, result);
    return true;
}

} // namespace Runtime
} // namespace UltraWeb

#endif // ULTRAWEB_USE_QUICKJS
