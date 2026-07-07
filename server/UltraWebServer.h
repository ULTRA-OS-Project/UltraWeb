// UltraWeb/server/UltraWebServer.h
// HTTP/WebSocket server for UltraWeb packages and delta updates
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Built-in backend: plain POSIX sockets with a thread per connection -
// dependency-free, suitable for development and moderate production loads.
// The public API is backend-neutral so a uWebSockets-based backend can be
// swapped in for high-concurrency deployments without touching callers
// (spec dependency; the protocol work - handshake, frames, deltas - lives
// in WebSocketHandler/DeltaGenerator and is shared by any backend).
//
// HTTP: registered routes are served with their content type; everything
// else 404s. WebSocket: any registered route can be upgraded; incoming
// frames go to the message handler, Broadcast* pushes to every client
// (delta updates, dev-server live reload).

#pragma once

#include "WebSocketHandler.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace UltraWeb {
namespace Server {

class UltraWebServer {
public:
    struct Config {
        std::string bindAddress = "127.0.0.1";
        uint16_t port = 8080;        // 0 = ephemeral (GetPort() reports it)
    };

    // Incoming WebSocket data frame from a client
    using WSMessageHandler =
        std::function<void(uint32_t clientId, const WebSocketHandler::Frame& frame)>;

    UltraWebServer();
    ~UltraWebServer();

    UltraWebServer(const UltraWebServer&) = delete;
    UltraWebServer& operator=(const UltraWebServer&) = delete;

    // ===== CONTENT =====

    // Serves a .ucpkg at `path` (application/x-ucpkg, immutable caching)
    void ServePackage(const std::string& path, std::vector<uint8_t> ucpkg);

    // Serves an arbitrary body at `path`
    void ServeStatic(const std::string& path, const std::string& contentType,
                     std::vector<uint8_t> body);
    void ServeStatic(const std::string& path, const std::string& contentType,
                     const std::string& body);

    // Replaces the content of an already-registered route (hot reload)
    bool UpdateRoute(const std::string& path, std::vector<uint8_t> body);

    // ===== LIFECYCLE =====

    bool Start(const Config& config, std::string& error);
    void Stop();
    bool IsRunning() const;
    uint16_t GetPort() const;    // actual bound port

    // ===== WEBSOCKET =====

    void SetWSMessageHandler(WSMessageHandler handler);
    void BroadcastText(const std::string& text);
    void BroadcastBinary(const std::vector<uint8_t>& data);
    size_t GetWSClientCount() const;

private:
    struct State;
    std::shared_ptr<State> state;
};

} // namespace Server
} // namespace UltraWeb
