// UltraWeb/server/DevServer.h
// Development server - watch, recompile, hot-reload over WebSocket
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Composes the Phase 4 pieces: FileWatcher detects source changes,
// the UltraWeb compilers rebuild the package (UCML/CSS via the bundler;
// JS via hermesc when available, otherwise shipped as source for the
// dev-mode engine), DeltaGenerator diffs old vs new, and UltraWebServer
// broadcasts the UCDELTA to connected clients as a binary WebSocket
// frame. When no delta can be generated a {"type":"reload"} text frame
// asks clients to re-fetch the full package.
//
// Routes: /            HTML shell page
//         /app.ucpkg   current package
//         (WebSocket upgrade on any route for the update channel)

#pragma once

#include "FileWatcher.h"
#include "UltraWebServer.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace UltraWeb {
namespace Server {

struct DevServerConfig {
    std::string sourceDir;              // watched directory
    std::string ucmlFile = "app.ucml";  // relative to sourceDir
    std::string cssFile = "app.css";    // optional (may be absent)
    std::string jsFile = "app.js";      // optional (may be absent)
    std::string bindAddress = "127.0.0.1";
    uint16_t port = 8080;               // 0 = ephemeral
    uint32_t pollIntervalMs = 200;
    bool compress = false;              // LZ4-compress the served package
};

class DevServer {
public:
    ~DevServer();

    bool Start(const DevServerConfig& config, std::string& error);
    void Stop();
    bool IsRunning() const { return server.IsRunning(); }
    uint16_t GetPort() const { return server.GetPort(); }

    // Recompile + rebundle + broadcast; public so tests and CLI --once
    // builds can drive it without the watcher thread
    bool RebuildAndPush(std::string& error);

    std::vector<uint8_t> GetCurrentPackage() const;
    UltraWebServer& GetServer() { return server; }

    // Stats from the last rebuild
    size_t GetLastDeltaSize() const { return lastDeltaSize; }
    size_t GetLastDeltaOpCount() const { return lastDeltaOpCount; }

private:
    DevServerConfig config;
    UltraWebServer server;
    FileWatcher watcher;

    std::vector<uint8_t> currentPackage;
    mutable std::mutex packageMutex;

    size_t lastDeltaSize = 0;
    size_t lastDeltaOpCount = 0;

    // Compiles sources into a package; empty vector + error on failure
    std::vector<uint8_t> BuildPackage(std::string& error);
    static std::string ShellPage();
};

} // namespace Server
} // namespace UltraWeb
