// UltraWeb/tests/JSRuntimeTests.cpp
// Unit tests for the JavaScript runtime layer (Phase 3)
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Engine-dependent tests run on the QuickJS backend (ULTRAWEB_USE_QUICKJS);
// without an engine backend they are skipped. HermesCompiler tests run the
// real hermesc when it is discoverable (ULTRAWEB_HERMESC or PATH) and are
// skipped otherwise.

#include "../include/UltraWebBundler.h"
#include "../runtime/UltraWebRuntime.h"
#include "../runtime/JSONUtil.h"
#include "../server/HermesCompiler.h"

#ifdef ULTRAWEB_USE_QUICKJS
#include "../runtime/QuickJSEngine.h"
#endif

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace UltraWeb;
using namespace UltraWeb::Runtime;

namespace {

void PrintTestResult(const std::string& testName, bool passed) {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << testName << std::endl;
}

void PrintSeparator() {
    std::cout << std::string(60, '=') << std::endl;
}

// ============================================================================
// JSON UTILITY TESTS
// ============================================================================

bool TestJSON_RoundTrip() {
    bool ok = false;
    JSONValue v = JSONValue::Parse(
        R"([1,"two",true,null,{"a":[3.5,false],"b":"c\"d"}])", ok);
    if (!ok || !v.IsArray() || v.Size() != 5) return false;
    if (v.At(0).GetInt() != 1) return false;
    if (v.At(1).GetString() != "two") return false;
    if (!v.At(2).GetBool()) return false;
    if (!v.At(3).IsNull()) return false;
    if (v.At(4).Get("a").At(0).GetNumber() != 3.5) return false;
    if (v.At(4).Get("b").GetString() != "c\"d") return false;

    // Serialize -> parse -> serialize is stable
    std::string s1 = v.Serialize();
    JSONValue v2 = JSONValue::Parse(s1, ok);
    return ok && v2.Serialize() == s1;
}

bool TestJSON_Malformed() {
    bool ok = true;
    JSONValue::Parse("{broken", ok);
    if (ok) return false;
    JSONValue::Parse("[1,2", ok);
    if (ok) return false;
    JSONValue::Parse("", ok);
    return !ok;
}

// ============================================================================
// HERMES COMPILER TESTS
// ============================================================================

bool TestHermes_MagicDetection() {
    std::vector<uint8_t> hbc = {0xC6, 0x1F, 0xBC, 0x03, 0xC1, 0x03, 0x19, 0x1F,
                                0x59, 0x00, 0x00, 0x00};
    if (!Server::HermesCompiler::IsHermesBytecode(hbc)) return false;
    if (Server::HermesCompiler::GetBytecodeVersion(hbc) != 0x59) return false;

    std::vector<uint8_t> js = {'v', 'a', 'r', ' ', 'x', ';'};
    return !Server::HermesCompiler::IsHermesBytecode(js);
}

bool TestHermes_CompileSource(bool& skipped) {
    Server::HermesCompiler compiler;
    if (!compiler.IsAvailable()) {
        skipped = true;
        std::cout << "  (hermesc not found - skipped)" << std::endl;
        return true;
    }
    auto result = compiler.CompileSource("function f(){return 6*7;} f();");
    if (!result.success) {
        std::cout << "  " << result.error << std::endl;
        return false;
    }
    if (!Server::HermesCompiler::IsHermesBytecode(result.bytecode)) return false;
    std::cout << "  compiled " << result.bytecode.size()
              << " bytes, HBC version "
              << Server::HermesCompiler::GetBytecodeVersion(result.bytecode)
              << std::endl;
    return true;
}

bool TestHermes_CompileError(bool& skipped) {
    Server::HermesCompiler compiler;
    if (!compiler.IsAvailable()) {
        skipped = true;
        return true;
    }
    auto result = compiler.CompileSource("function ( { syntax error");
    return !result.success && !result.error.empty();
}

// ============================================================================
// ENGINE-BACKED RUNTIME TESTS (QuickJS)
// ============================================================================

#ifdef ULTRAWEB_USE_QUICKJS

// Builds a small app package and loads it into a runtime with an engine
struct JSFixture {
    UltraWebRuntime runtime;
    std::vector<std::string> consoleLines;
    bool ready = false;
    std::string error;

    JSFixture() {
        PackageBundler bundler;
        bundler.SetUIFromSource(R"(
            <div id="app" class="container">
                <text id="title" class="heading">Hello</text>
                <text id="counter" class="value">0</text>
                <button id="btn" class="btn">Click me</button>
            </div>
        )");
        bundler.SetStyleFromSource(
            ".container { padding: 16px; } .active { color: #FF0000; }");
        BundleResult bundle = bundler.Bundle();
        if (!bundle.success) {
            error = "bundle failed";
            return;
        }

        auto load = runtime.LoadPackage(bundle.data);
        if (!load.success) {
            error = "load failed: " + load.error;
            return;
        }

        if (!runtime.AttachJSEngine(std::make_shared<QuickJSEngine>(), error)) {
            return;
        }
        runtime.GetJSApi()->SetConsoleSink(
            [this](const std::string& line) { consoleLines.push_back(line); });
        ready = true;
    }

    bool Run(const std::string& js) {
        std::string err;
        if (!runtime.GetJSEngine()->EvaluateSource(js, "<test>", err)) {
            std::cout << "  JS error: " << err << std::endl;
            return false;
        }
        return true;
    }
};

bool TestJS_PreludeLoads() {
    JSFixture fx;
    if (!fx.ready) {
        std::cout << "  " << fx.error << std::endl;
        return false;
    }
    return fx.Run("if (typeof UC !== 'object') throw new Error('no UC');");
}

bool TestJS_GetSetText() {
    JSFixture fx;
    if (!fx.ready) return false;
    if (!fx.Run(R"(
        var title = UC.getElementById('title');
        if (!title) throw new Error('title not found');
        if (title.getText() !== 'Hello') throw new Error('got ' + title.getText());
        title.setText('Changed from JS');
        if (UC.getElementById('missing') !== null) throw new Error('phantom element');
    )")) return false;
    RuntimeElement* el = fx.runtime.GetElementById("title");
    return el && el->textContent == "Changed from JS";
}

bool TestJS_ClassOps() {
    JSFixture fx;
    if (!fx.ready) return false;
    if (!fx.Run(R"(
        var btn = UC.getElementById('btn');
        if (btn.hasClass('active')) throw new Error('unexpected class');
        btn.addClass('active');
        if (!btn.hasClass('active')) throw new Error('addClass failed');
        if (btn.toggleClass('active') !== false) throw new Error('toggle off failed');
        if (btn.hasClass('active')) throw new Error('class still present');
    )")) return false;
    return true;
}

bool TestJS_StateBinding() {
    JSFixture fx;
    if (!fx.ready) return false;
    if (!fx.Run(R"(
        var pair = UC.useState(0);
        var getCount = pair[0], setCount = pair[1];
        var counter = UC.getElementById('counter');
        counter.bind('text', getCount);
        setCount(41);
        setCount(getCount() + 1);
        if (getCount() !== 42) throw new Error('state is ' + getCount());
    )")) return false;
    RuntimeElement* el = fx.runtime.GetElementById("counter");
    return el && el->textContent == "42";
}

bool TestJS_Effects() {
    JSFixture fx;
    if (!fx.ready) return false;
    return fx.Run(R"(
        var pair = UC.useState('a');
        var log = [];
        UC.effect(function () { log.push(pair[0]()); }, [pair[0]]);
        pair[1]('b');
        pair[1]('c');
        if (log.join(',') !== 'a,b,c') throw new Error('effect log: ' + log.join(','));

        var doubled = UC.computed(function () {
            var v = pair[0]();
            return v + v;
        }, [pair[0]]);
        pair[1]('x');
        if (doubled() !== 'xx') throw new Error('computed: ' + doubled());
    )");
}

bool TestJS_EventDispatch() {
    JSFixture fx;
    if (!fx.ready) return false;
    if (!fx.Run(R"(
        var clicks = UC.useState(0);
        var btn = UC.getElementById('btn');
        UC.getElementById('counter').bind('text', clicks[0]);
        btn.on('click', function (e) {
            clicks[1](clicks[0]() + 1);
            console.log('clicked at', e.x, e.y);
        });
    )")) return false;

    RuntimeElement* btn = fx.runtime.GetElementById("btn");
    if (!btn) return false;

    // Native side fires the event twice
    if (!fx.runtime.FireDomEvent(btn->elementId, "click", R"({"x":10,"y":20})"))
        return false;
    if (!fx.runtime.FireDomEvent(btn->elementId, "click", R"({"x":11,"y":21})"))
        return false;

    // Unregistered events report no handler
    if (fx.runtime.FireDomEvent(btn->elementId, "keydown", "{}")) return false;

    RuntimeElement* counter = fx.runtime.GetElementById("counter");
    if (!counter || counter->textContent != "2") {
        std::cout << "  counter text: "
                  << (counter ? counter->textContent : "<missing>") << std::endl;
        return false;
    }
    return fx.consoleLines.size() == 2 &&
           fx.consoleLines[0] == "clicked at 10 20";
}

bool TestJS_OffAndOnce() {
    JSFixture fx;
    if (!fx.ready) return false;
    if (!fx.Run(R"(
        var count = 0;
        var btn = UC.getElementById('btn');
        btn.once('click', function () { count++; });
        globalThis.__getCount = function () { return count; };
    )")) return false;

    RuntimeElement* btn = fx.runtime.GetElementById("btn");
    fx.runtime.FireDomEvent(btn->elementId, "click");
    // 'once' removed itself; second fire finds no handler
    if (fx.runtime.FireDomEvent(btn->elementId, "click")) return false;

    std::string result, err;
    fx.runtime.GetJSEngine()->CallGlobalFunction("__getCount", "[]", result, err);
    return result == "1";
}

bool TestJS_CodeSectionSource() {
    JSFixture fx;
    if (!fx.ready) return false;

    // Development mode: plain JS source as the code section
    std::string app = "UC.getElementById('title').setText('from code section');";
    fx.runtime.LoadCode(std::vector<uint8_t>(app.begin(), app.end()));

    std::string err;
    if (!fx.runtime.ExecuteCodeSection(err)) {
        std::cout << "  " << err << std::endl;
        return false;
    }
    RuntimeElement* el = fx.runtime.GetElementById("title");
    return el && el->textContent == "from code section";
}

bool TestJS_BytecodeNeedsHermes() {
    JSFixture fx;
    if (!fx.ready) return false;

    // Real HBC header: QuickJS must refuse with a clear message
    std::vector<uint8_t> hbc = {0xC6, 0x1F, 0xBC, 0x03, 0xC1, 0x03, 0x19, 0x1F,
                                0x59, 0x00, 0x00, 0x00};
    fx.runtime.LoadCode(hbc);

    std::string err;
    if (fx.runtime.ExecuteCodeSection(err)) return false;
    return err.find("Hermes") != std::string::npos;
}

bool TestJS_Phase4StubsThrow() {
    JSFixture fx;
    if (!fx.ready) return false;
    return fx.Run(R"(
        var threw = 0;
        try { UC.fetch('/api'); } catch (e) { threw++; }
        try { UC.websocket('/ws'); } catch (e) { threw++; }
        try { UC.router.navigate('/'); } catch (e) { threw++; }
        try { UC.storage.set('k', 'v'); } catch (e) { threw++; }
        if (threw !== 4) throw new Error('stubs threw ' + threw + '/4');
    )");
}

#endif // ULTRAWEB_USE_QUICKJS

} // namespace

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "UltraWeb JavaScript Runtime Test Suite (Phase 3)\n";
    PrintSeparator();

    int passed = 0;
    int failed = 0;
    auto run = [&](const char* name, bool ok) {
        if (ok) passed++; else failed++;
        PrintTestResult(name, ok);
    };

    std::cout << "\n[JSON Utility Tests]\n";
    run("Round Trip", TestJSON_RoundTrip());
    run("Malformed Input", TestJSON_Malformed());

    std::cout << "\n[Hermes Compiler Tests]\n";
    run("Magic Detection", TestHermes_MagicDetection());
    {
        bool skipped = false;
        run("Compile Source", TestHermes_CompileSource(skipped));
        run("Compile Error Reporting", TestHermes_CompileError(skipped));
    }

#ifdef ULTRAWEB_USE_QUICKJS
    std::cout << "\n[JS Engine Tests (QuickJS backend)]\n";
    run("Prelude Loads", TestJS_PreludeLoads());
    run("Get/Set Text", TestJS_GetSetText());
    run("Class Operations", TestJS_ClassOps());
    run("State Binding", TestJS_StateBinding());
    run("Effects & Computed", TestJS_Effects());
    run("Event Dispatch", TestJS_EventDispatch());
    run("Off & Once", TestJS_OffAndOnce());
    run("Code Section (source)", TestJS_CodeSectionSource());
    run("Bytecode Needs Hermes", TestJS_BytecodeNeedsHermes());
    run("Phase 4/5 Stubs Throw", TestJS_Phase4StubsThrow());
#else
    std::cout << "\n[JS Engine Tests] skipped - no engine backend built "
                 "(enable ULTRAWEB_USE_QUICKJS or ULTRAWEB_USE_HERMES)\n";
#endif

    std::cout << "\n";
    PrintSeparator();
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    PrintSeparator();
    std::cout << "\n";

    return failed > 0 ? 1 : 0;
}
