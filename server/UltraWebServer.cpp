// UltraWeb/server/UltraWebServer.cpp
// Built-in POSIX socket HTTP/WebSocket backend
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "UltraWebServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

namespace UltraWeb {
namespace Server {

// ============================================================================
// SHARED STATE (owned jointly by the server object and its threads)
// ============================================================================

struct UltraWebServer::State {
    // Routes
    struct Route {
        std::string contentType;
        std::vector<uint8_t> body;
    };
    std::map<std::string, Route> routes;
    mutable std::mutex routesMutex;

    // Lifecycle
    std::atomic<bool> running{false};
    int listenFd = -1;
    uint16_t boundPort = 0;
    std::thread acceptThread;

    // WebSocket clients
    std::map<uint32_t, int> wsClients;   // clientId -> fd
    mutable std::mutex wsMutex;
    uint32_t nextClientId = 1;
    WSMessageHandler wsHandler;
    std::mutex wsHandlerMutex;

    ~State() { CloseListen(); }

    void CloseListen() {
        if (listenFd >= 0) {
            ::shutdown(listenFd, SHUT_RDWR);
            ::close(listenFd);
            listenFd = -1;
        }
    }

    void SendAll(int fd, const uint8_t* data, size_t size) {
        size_t sent = 0;
        while (sent < size) {
            ssize_t n = ::send(fd, data + sent, size - sent, MSG_NOSIGNAL);
            if (n <= 0) return;
            sent += static_cast<size_t>(n);
        }
    }

    void SendString(int fd, const std::string& s) {
        SendAll(fd, reinterpret_cast<const uint8_t*>(s.data()), s.size());
    }

    void SendHttpResponse(int fd, int status, const std::string& statusText,
                          const std::string& contentType,
                          const uint8_t* body, size_t bodySize) {
        std::ostringstream head;
        head << "HTTP/1.1 " << status << " " << statusText << "\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Content-Length: " << bodySize << "\r\n"
             << "Connection: close\r\n"
             << "Server: UltraWeb\r\n"
             << "\r\n";
        SendString(fd, head.str());
        if (body && bodySize) SendAll(fd, body, bodySize);
    }

    // Serves one accepted connection; runs on its own thread
    void HandleConnection(int fd) {
        // Read until end of HTTP headers (bounded)
        std::string request;
        char buf[4096];
        while (request.find("\r\n\r\n") == std::string::npos) {
            if (request.size() > 64 * 1024) { ::close(fd); return; }
            ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
            if (n <= 0) { ::close(fd); return; }
            request.append(buf, static_cast<size_t>(n));
        }

        // Request line: METHOD PATH VERSION
        std::istringstream line(request.substr(0, request.find("\r\n")));
        std::string method, path, version;
        line >> method >> path >> version;
        // Strip query string
        size_t query = path.find('?');
        if (query != std::string::npos) path.resize(query);

        if (method != "GET" && method != "HEAD") {
            SendHttpResponse(fd, 405, "Method Not Allowed", "text/plain",
                             nullptr, 0);
            ::close(fd);
            return;
        }

        // WebSocket upgrade?
        std::string wsKey;
        if (WebSocketHandler::ParseUpgradeRequest(request, wsKey)) {
            SendString(fd, WebSocketHandler::BuildUpgradeResponse(wsKey));
            RunWebSocketSession(fd);
            return;
        }

        // Plain HTTP route
        Route route;
        bool found = false;
        {
            std::lock_guard<std::mutex> lock(routesMutex);
            auto it = routes.find(path);
            if (it != routes.end()) {
                route = it->second;
                found = true;
            }
        }
        if (found) {
            SendHttpResponse(fd, 200, "OK", route.contentType,
                             method == "HEAD" ? nullptr : route.body.data(),
                             method == "HEAD" ? route.body.size()
                                              : route.body.size());
        } else {
            static const char* kNotFound = "404 - no such route";
            SendHttpResponse(fd, 404, "Not Found", "text/plain",
                             reinterpret_cast<const uint8_t*>(kNotFound),
                             std::strlen(kNotFound));
        }
        ::close(fd);
    }

    void RunWebSocketSession(int fd) {
        uint32_t clientId;
        {
            std::lock_guard<std::mutex> lock(wsMutex);
            clientId = nextClientId++;
            wsClients[clientId] = fd;
        }

        std::vector<uint8_t> buffer;
        char buf[4096];
        while (running) {
            ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
            if (n <= 0) break;
            buffer.insert(buffer.end(), buf, buf + n);

            // Drain complete frames
            while (true) {
                WebSocketHandler::Frame frame;
                std::string error;
                size_t consumed = WebSocketHandler::DecodeFrame(
                    buffer.data(), buffer.size(), frame, error);
                if (consumed == 0) break;                 // need more bytes
                if (consumed == SIZE_MAX) { n = -1; break; }  // protocol error
                buffer.erase(buffer.begin(), buffer.begin() + consumed);

                if (frame.opcode == WebSocketHandler::OpCode::Close) {
                    auto close = WebSocketHandler::EncodeClose();
                    SendAll(fd, close.data(), close.size());
                    n = -1;
                    break;
                }
                if (frame.opcode == WebSocketHandler::OpCode::Ping) {
                    auto pong = WebSocketHandler::EncodePong(frame.payload);
                    SendAll(fd, pong.data(), pong.size());
                    continue;
                }
                if (frame.opcode == WebSocketHandler::OpCode::Text ||
                    frame.opcode == WebSocketHandler::OpCode::Binary) {
                    WSMessageHandler handler;
                    {
                        std::lock_guard<std::mutex> lock(wsHandlerMutex);
                        handler = wsHandler;
                    }
                    if (handler) handler(clientId, frame);
                }
            }
            if (n < 0) break;
        }

        {
            std::lock_guard<std::mutex> lock(wsMutex);
            wsClients.erase(clientId);
        }
        ::close(fd);
    }

    void Broadcast(const std::vector<uint8_t>& frameBytes) {
        std::lock_guard<std::mutex> lock(wsMutex);
        for (const auto& kv : wsClients) {
            SendAll(kv.second, frameBytes.data(), frameBytes.size());
        }
    }
};

// ============================================================================
// PUBLIC API
// ============================================================================

UltraWebServer::UltraWebServer() : state(std::make_shared<State>()) {}

UltraWebServer::~UltraWebServer() { Stop(); }

void UltraWebServer::ServePackage(const std::string& path,
                                  std::vector<uint8_t> ucpkg) {
    ServeStatic(path, "application/x-ucpkg", std::move(ucpkg));
}

void UltraWebServer::ServeStatic(const std::string& path,
                                 const std::string& contentType,
                                 std::vector<uint8_t> body) {
    std::lock_guard<std::mutex> lock(state->routesMutex);
    state->routes[path] = {contentType, std::move(body)};
}

void UltraWebServer::ServeStatic(const std::string& path,
                                 const std::string& contentType,
                                 const std::string& body) {
    ServeStatic(path, contentType,
                std::vector<uint8_t>(body.begin(), body.end()));
}

bool UltraWebServer::UpdateRoute(const std::string& path,
                                 std::vector<uint8_t> body) {
    std::lock_guard<std::mutex> lock(state->routesMutex);
    auto it = state->routes.find(path);
    if (it == state->routes.end()) return false;
    it->second.body = std::move(body);
    return true;
}

bool UltraWebServer::Start(const Config& config, std::string& error) {
    if (state->running) {
        error = "server already running";
        return false;
    }

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        error = "socket() failed";
        return false;
    }
    int yes = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.port);
    if (::inet_pton(AF_INET, config.bindAddress.c_str(), &addr.sin_addr) != 1) {
        ::close(fd);
        error = "invalid bind address: " + config.bindAddress;
        return false;
    }
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(fd);
        error = "bind failed on port " + std::to_string(config.port);
        return false;
    }
    if (::listen(fd, 64) != 0) {
        ::close(fd);
        error = "listen failed";
        return false;
    }

    // Report the actual port (ephemeral when config.port == 0)
    sockaddr_in bound{};
    socklen_t len = sizeof(bound);
    ::getsockname(fd, reinterpret_cast<sockaddr*>(&bound), &len);
    state->boundPort = ntohs(bound.sin_port);

    state->listenFd = fd;
    state->running = true;

    std::shared_ptr<State> s = state;
    state->acceptThread = std::thread([s]() {
        while (s->running) {
            int client = ::accept(s->listenFd, nullptr, nullptr);
            if (client < 0) {
                if (s->running) continue;
                break;
            }
            int yes = 1;
            ::setsockopt(client, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
            std::thread([s, client]() { s->HandleConnection(client); }).detach();
        }
    });

    return true;
}

void UltraWebServer::Stop() {
    if (!state->running) return;
    state->running = false;
    state->CloseListen();

    // Close WS clients so their session threads exit
    {
        std::lock_guard<std::mutex> lock(state->wsMutex);
        for (const auto& kv : state->wsClients) {
            ::shutdown(kv.second, SHUT_RDWR);
        }
    }
    if (state->acceptThread.joinable()) state->acceptThread.join();
}

bool UltraWebServer::IsRunning() const { return state->running; }

uint16_t UltraWebServer::GetPort() const { return state->boundPort; }

void UltraWebServer::SetWSMessageHandler(WSMessageHandler handler) {
    std::lock_guard<std::mutex> lock(state->wsHandlerMutex);
    state->wsHandler = std::move(handler);
}

void UltraWebServer::BroadcastText(const std::string& text) {
    state->Broadcast(WebSocketHandler::EncodeText(text));
}

void UltraWebServer::BroadcastBinary(const std::vector<uint8_t>& data) {
    state->Broadcast(WebSocketHandler::EncodeBinary(data));
}

size_t UltraWebServer::GetWSClientCount() const {
    std::lock_guard<std::mutex> lock(state->wsMutex);
    return state->wsClients.size();
}

} // namespace Server
} // namespace UltraWeb
