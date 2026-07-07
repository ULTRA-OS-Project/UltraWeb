// UltraWeb/tests/ServerTests.cpp
// Unit tests for the server framework (Phase 4)
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "../include/UltraWebBundler.h"
#include "../include/UltraWebDelta.h"
#include "../runtime/UltraWebRuntime.h"
#include "../server/DeltaGenerator.h"
#include "../server/DevServer.h"
#include "../server/FileWatcher.h"
#include "../server/UltraWebServer.h"
#include "../server/WebSocketHandler.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace UltraWeb;
using namespace UltraWeb::Server;

namespace {

void PrintTestResult(const std::string& testName, bool passed) {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << testName << std::endl;
}

void PrintSeparator() {
    std::cout << std::string(60, '=') << std::endl;
}

std::vector<uint8_t> BuildTestPackage(const std::string& titleText,
                                      const std::string& extraClass = "",
                                      const std::string& css =
                                          ".heading { font-size: 24px; }") {
    PackageBundler bundler;
    std::string cls = "heading";
    if (!extraClass.empty()) cls += " " + extraClass;
    bundler.SetUIFromSource(
        "<div id=\"app\"><text id=\"title\" class=\"" + cls + "\">" +
        titleText + "</text><button id=\"btn\">Go</button></div>");
    bundler.SetStyleFromSource(css);
    BundleResult result = bundler.Bundle();
    return result.success ? result.data : std::vector<uint8_t>{};
}

// Minimal blocking TCP client for the live-server tests
struct TcpClient {
    int fd = -1;

    bool Connect(uint16_t port) {
        fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return false;
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            ::close(fd);
            fd = -1;
            return false;
        }
        return true;
    }

    bool Send(const std::string& data) {
        return fd >= 0 &&
               ::send(fd, data.data(), data.size(), MSG_NOSIGNAL) ==
                   static_cast<ssize_t>(data.size());
    }

    // Reads until the connection closes or maxMs elapses
    std::string ReadAll(int maxMs = 2000) {
        std::string out;
        char buf[4096];
        auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(maxMs);
        timeval tv{0, 100 * 1000};
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        while (std::chrono::steady_clock::now() < deadline) {
            ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
            if (n > 0) out.append(buf, static_cast<size_t>(n));
            else if (n == 0) break;
        }
        return out;
    }

    // Reads until `bytes` are available or timeout
    std::vector<uint8_t> ReadExact(size_t bytes, int maxMs = 2000) {
        std::vector<uint8_t> out;
        char buf[4096];
        auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(maxMs);
        timeval tv{0, 100 * 1000};
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        while (out.size() < bytes &&
               std::chrono::steady_clock::now() < deadline) {
            ssize_t n = ::recv(fd, buf,
                               std::min(sizeof(buf), bytes - out.size()), 0);
            if (n > 0) out.insert(out.end(), buf, buf + n);
            else if (n == 0) break;
        }
        return out;
    }

    ~TcpClient() {
        if (fd >= 0) ::close(fd);
    }
};

// ============================================================================
// WEBSOCKET PROTOCOL TESTS
// ============================================================================

bool TestWS_SHA1KnownVector() {
    // FIPS 180-1: SHA1("abc") = a9993e364706816aba3e25717850c26c9cd0d89d
    auto digest = SHA1(reinterpret_cast<const uint8_t*>("abc"), 3);
    static const uint8_t expected[20] = {
        0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
        0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d};
    return digest.size() == 20 &&
           std::memcmp(digest.data(), expected, 20) == 0;
}

bool TestWS_AcceptKeyRFCVector() {
    // RFC 6455 section 1.3 example
    return WebSocketHandler::ComputeAcceptKey("dGhlIHNhbXBsZSBub25jZQ==") ==
           "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
}

bool TestWS_UpgradeParsing() {
    std::string request =
        "GET /chat HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";
    std::string key;
    if (!WebSocketHandler::ParseUpgradeRequest(request, key)) return false;
    if (key != "dGhlIHNhbXBsZSBub25jZQ==") return false;

    // Plain GET must not parse as an upgrade
    std::string plain = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
    return !WebSocketHandler::ParseUpgradeRequest(plain, key);
}

bool TestWS_FrameRoundTrip() {
    // Server-encoded (unmasked) frame decodes back
    auto encoded = WebSocketHandler::EncodeText("hello UltraWeb");
    WebSocketHandler::Frame frame;
    std::string error;
    size_t consumed = WebSocketHandler::DecodeFrame(
        encoded.data(), encoded.size(), frame, error);
    if (consumed != encoded.size()) return false;
    if (frame.opcode != WebSocketHandler::OpCode::Text || !frame.fin) return false;
    if (std::string(frame.payload.begin(), frame.payload.end()) !=
        "hello UltraWeb") return false;

    // Masked client frame (RFC 6455 5.7: masked "Hello")
    const uint8_t masked[] = {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d,
                              0x7f, 0x9f, 0x4d, 0x51, 0x58};
    consumed = WebSocketHandler::DecodeFrame(masked, sizeof(masked), frame, error);
    if (consumed != sizeof(masked)) return false;
    if (std::string(frame.payload.begin(), frame.payload.end()) != "Hello")
        return false;

    // Truncated input asks for more bytes
    consumed = WebSocketHandler::DecodeFrame(masked, 4, frame, error);
    if (consumed != 0) return false;

    // 16-bit length path
    std::string big(500, 'x');
    auto bigFrame = WebSocketHandler::EncodeText(big);
    consumed = WebSocketHandler::DecodeFrame(bigFrame.data(), bigFrame.size(),
                                             frame, error);
    return consumed == bigFrame.size() && frame.payload.size() == 500;
}

// ============================================================================
// DELTA TESTS
// ============================================================================

bool TestDelta_TextChange() {
    auto oldPkg = BuildTestPackage("Hello");
    auto newPkg = BuildTestPackage("Hello again");
    if (oldPkg.empty() || newPkg.empty()) return false;

    DeltaGenerator generator;
    DeltaResult delta = generator.Generate(oldPkg, newPkg);
    if (!delta.success || delta.identical || delta.uiStructureChanged)
        return false;
    if (delta.opCount != 1) {
        std::cout << "  expected 1 op, got " << delta.opCount << std::endl;
        return false;
    }
    std::cout << "  delta: " << delta.delta.size() << " bytes vs package "
              << newPkg.size() << " bytes" << std::endl;

    // Apply on a client runtime
    Runtime::UltraWebRuntime runtime;
    if (!runtime.LoadPackage(oldPkg).success) return false;
    std::string error;
    if (!runtime.ApplyDelta(delta.delta, error)) {
        std::cout << "  apply: " << error << std::endl;
        return false;
    }
    auto* el = runtime.GetElementById("title");
    return el && el->textContent == "Hello again" &&
           delta.delta.size() < newPkg.size();
}

bool TestDelta_ClassChange() {
    auto oldPkg = BuildTestPackage("Same");
    auto newPkg = BuildTestPackage("Same", "highlighted");
    if (oldPkg.empty() || newPkg.empty()) return false;

    DeltaGenerator generator;
    DeltaResult delta = generator.Generate(oldPkg, newPkg);
    if (!delta.success || delta.opCount < 1) return false;

    Runtime::UltraWebRuntime runtime;
    if (!runtime.LoadPackage(oldPkg).success) return false;
    std::string error;
    if (!runtime.ApplyDelta(delta.delta, error)) return false;

    auto* el = runtime.GetElementById("title");
    if (!el) return false;
    return std::find(el->classNames.begin(), el->classNames.end(),
                     "highlighted") != el->classNames.end();
}

bool TestDelta_StyleSectionReplace() {
    auto oldPkg = BuildTestPackage("Same");
    auto newPkg = BuildTestPackage("Same", "",
                                   ".heading { font-size: 32px; }");
    DeltaGenerator generator;
    DeltaResult delta = generator.Generate(oldPkg, newPkg);
    if (!delta.success || delta.opCount != 1) return false;

    Runtime::UltraWebRuntime runtime;
    if (!runtime.LoadPackage(oldPkg).success) return false;
    std::string error;
    return runtime.ApplyDelta(delta.delta, error);
}

bool TestDelta_BaseMismatchRefused() {
    auto pkgA = BuildTestPackage("A");
    auto pkgB = BuildTestPackage("B");
    auto pkgC = BuildTestPackage("C");

    DeltaGenerator generator;
    DeltaResult delta = generator.Generate(pkgB, pkgC);  // base is B

    Runtime::UltraWebRuntime runtime;
    if (!runtime.LoadPackage(pkgA).success) return false;  // client has A
    std::string error;
    return !runtime.ApplyDelta(delta.delta, error) &&
           error.find("mismatch") != std::string::npos;
}

bool TestDelta_ChainedDeltas() {
    auto v1 = BuildTestPackage("v1");
    auto v2 = BuildTestPackage("v2");
    auto v3 = BuildTestPackage("v3");

    DeltaGenerator generator;
    auto d12 = generator.Generate(v1, v2);
    auto d23 = generator.Generate(v2, v3);

    Runtime::UltraWebRuntime runtime;
    if (!runtime.LoadPackage(v1).success) return false;
    std::string error;
    if (!runtime.ApplyDelta(d12.delta, error)) return false;
    if (!runtime.ApplyDelta(d23.delta, error)) return false;  // chains via targetCrc32
    auto* el = runtime.GetElementById("title");
    return el && el->textContent == "v3";
}

bool TestDelta_IdenticalPackages() {
    auto pkg = BuildTestPackage("Same");
    DeltaGenerator generator;
    DeltaResult delta = generator.Generate(pkg, pkg);
    return delta.success && delta.identical && delta.opCount == 0;
}

bool TestDelta_MalformedStream() {
    std::vector<uint8_t> garbage = {1, 2, 3, 4, 5, 6, 7, 8};
    auto parsed = ParseDelta(garbage);
    return !parsed.success;
}

// ============================================================================
// FILE WATCHER TESTS
// ============================================================================

bool TestWatcher_DetectsChanges() {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "ultraweb-watch-test";
    fs::remove_all(dir);
    fs::create_directories(dir);

    {
        std::ofstream f(dir / "app.css");
        f << ".a { color: red; }";
    }

    FileWatcher watcher;
    watcher.AddPath(dir.string());
    watcher.SetExtensionFilter({".css", ".ucml"});
    watcher.PollChanges();  // baseline

    if (!watcher.PollChanges().empty()) return false;  // no change yet

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    {
        std::ofstream f(dir / "app.css");
        f << ".a { color: blue; }";
    }
    auto changes = watcher.PollChanges();
    bool sawModify = changes.size() == 1;

    // Filtered extension is ignored
    {
        std::ofstream f(dir / "notes.txt");
        f << "ignore me";
    }
    bool ignoredFiltered = watcher.PollChanges().empty();

    // Removal is a change
    fs::remove(dir / "app.css");
    bool sawRemove = watcher.PollChanges().size() == 1;

    fs::remove_all(dir);
    return sawModify && ignoredFiltered && sawRemove;
}

// ============================================================================
// LIVE SERVER TESTS
// ============================================================================

bool TestServer_HttpServing() {
    UltraWebServer server;
    server.ServeStatic("/", "text/html", std::string("<h1>uw</h1>"));
    server.ServePackage("/app.ucpkg", BuildTestPackage("Hi"));

    UltraWebServer::Config config;
    config.port = 0;  // ephemeral
    std::string error;
    if (!server.Start(config, error)) {
        std::cout << "  " << error << std::endl;
        return false;
    }

    TcpClient client;
    if (!client.Connect(server.GetPort())) return false;
    client.Send("GET /app.ucpkg HTTP/1.1\r\nHost: localhost\r\n\r\n");
    std::string response = client.ReadAll();

    // UCPKG magic 0x5543504B is little-endian on the wire: "KPCU"
    bool ok = response.find("HTTP/1.1 200 OK") == 0 &&
              response.find("application/x-ucpkg") != std::string::npos &&
              response.find("KPCU") != std::string::npos;

    // Unknown route 404s
    TcpClient client2;
    if (!client2.Connect(server.GetPort())) return false;
    client2.Send("GET /nope HTTP/1.1\r\nHost: localhost\r\n\r\n");
    bool notFound = client2.ReadAll().find("HTTP/1.1 404") == 0;

    server.Stop();
    return ok && notFound;
}

bool TestServer_WebSocketSession() {
    UltraWebServer server;
    std::string received;
    server.SetWSMessageHandler(
        [&](uint32_t, const WebSocketHandler::Frame& frame) {
            received.assign(frame.payload.begin(), frame.payload.end());
        });

    UltraWebServer::Config config;
    config.port = 0;
    std::string error;
    if (!server.Start(config, error)) return false;

    TcpClient client;
    if (!client.Connect(server.GetPort())) return false;
    client.Send(
        "GET / HTTP/1.1\r\nHost: localhost\r\n"
        "Upgrade: websocket\r\nConnection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n");

    // 101 with the RFC-correct accept key
    std::string response;
    {
        char buf[1024];
        timeval tv{2, 0};
        ::setsockopt(client.fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        while (response.find("\r\n\r\n") == std::string::npos) {
            ssize_t n = ::recv(client.fd, buf, sizeof(buf), 0);
            if (n <= 0) break;
            response.append(buf, static_cast<size_t>(n));
        }
    }
    if (response.find("101 Switching Protocols") == std::string::npos) return false;
    if (response.find("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") == std::string::npos) return false;

    // Give the session a moment to register, then broadcast to the client
    for (int i = 0; i < 50 && server.GetWSClientCount() == 0; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (server.GetWSClientCount() != 1) return false;

    server.BroadcastText("delta-ready");
    auto frameBytes = client.ReadExact(2 + 11);  // header + payload
    WebSocketHandler::Frame frame;
    std::string decodeError;
    size_t consumed = WebSocketHandler::DecodeFrame(
        frameBytes.data(), frameBytes.size(), frame, decodeError);
    if (consumed == 0 || consumed == SIZE_MAX) return false;
    if (std::string(frame.payload.begin(), frame.payload.end()) != "delta-ready")
        return false;

    // Client -> server (masked) frame reaches the handler
    const uint8_t masked[] = {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d,
                              0x7f, 0x9f, 0x4d, 0x51, 0x58};  // "Hello"
    client.Send(std::string(reinterpret_cast<const char*>(masked), sizeof(masked)));
    for (int i = 0; i < 50 && received.empty(); i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    server.Stop();
    return received == "Hello";
}

bool TestDevServer_RebuildAndDelta() {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "ultraweb-dev-test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    {
        std::ofstream f(dir / "app.ucml");
        f << "<div id=\"app\"><text id=\"title\">First</text></div>";
    }
    {
        std::ofstream f(dir / "app.css");
        f << "#title { font-size: 20px; }";
    }
    {
        std::ofstream f(dir / "app.js");
        f << "console.log('app loaded');";
    }

    DevServerConfig config;
    config.sourceDir = dir.string();
    config.port = 0;
    config.pollIntervalMs = 60 * 1000;  // no background rebuilds during test

    DevServer dev;
    std::string error;
    if (!dev.Start(config, error)) {
        std::cout << "  " << error << std::endl;
        return false;
    }

    // Client loads the initial package...
    auto v1 = dev.GetCurrentPackage();
    Runtime::UltraWebRuntime runtime;
    if (!runtime.LoadPackage(v1).success) return false;

    // ...source changes, dev server rebuilds and produces a delta
    {
        std::ofstream f(dir / "app.ucml");
        f << "<div id=\"app\"><text id=\"title\">Second</text></div>";
    }
    if (!dev.RebuildAndPush(error)) {
        std::cout << "  " << error << std::endl;
        return false;
    }
    if (dev.GetLastDeltaOpCount() == 0) return false;
    std::cout << "  hot update delta: " << dev.GetLastDeltaSize()
              << " bytes, " << dev.GetLastDeltaOpCount() << " op(s)"
              << std::endl;

    // The delta the server would broadcast applies cleanly to the client
    DeltaGenerator generator;
    auto delta = generator.Generate(v1, dev.GetCurrentPackage());
    if (!delta.success) return false;
    if (!runtime.ApplyDelta(delta.delta, error)) return false;

    auto* el = runtime.GetElementById("title");
    bool ok = el && el->textContent == "Second";

    dev.Stop();
    fs::remove_all(dir);
    return ok;
}

} // namespace

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "UltraWeb Server Framework Test Suite (Phase 4)\n";
    PrintSeparator();

    int passed = 0;
    int failed = 0;
    auto run = [&](const char* name, bool ok) {
        if (ok) passed++; else failed++;
        PrintTestResult(name, ok);
    };

    std::cout << "\n[WebSocket Protocol Tests]\n";
    run("SHA-1 Known Vector", TestWS_SHA1KnownVector());
    run("Accept Key (RFC 6455 Vector)", TestWS_AcceptKeyRFCVector());
    run("Upgrade Parsing", TestWS_UpgradeParsing());
    run("Frame Round Trip", TestWS_FrameRoundTrip());

    std::cout << "\n[Delta Update Tests]\n";
    run("Text Change Op", TestDelta_TextChange());
    run("Class Change Op", TestDelta_ClassChange());
    run("Style Section Replace", TestDelta_StyleSectionReplace());
    run("Base Mismatch Refused", TestDelta_BaseMismatchRefused());
    run("Chained Deltas", TestDelta_ChainedDeltas());
    run("Identical Packages", TestDelta_IdenticalPackages());
    run("Malformed Stream", TestDelta_MalformedStream());

    std::cout << "\n[File Watcher Tests]\n";
    run("Detects Changes", TestWatcher_DetectsChanges());

    std::cout << "\n[Live Server Tests]\n";
    run("HTTP Serving", TestServer_HttpServing());
    run("WebSocket Session", TestServer_WebSocketSession());
    run("DevServer Rebuild & Delta", TestDevServer_RebuildAndDelta());

    std::cout << "\n";
    PrintSeparator();
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    PrintSeparator();
    std::cout << "\n";

    return failed > 0 ? 1 : 0;
}
