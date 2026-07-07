// UltraWeb/tools/uws.cpp
// UltraWeb Server CLI - serves a package, or runs the dev server
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Usage:
//   uws <app.ucpkg> [--port 8080] [--bind 127.0.0.1]
//       Serves the package at /app.ucpkg with WebSocket support.
//   uws --dev <sourceDir> [--port 8080] [--bind 127.0.0.1] [--compress]
//       Development mode: watches sourceDir (app.ucml/app.css/app.js),
//       recompiles on change and pushes UCDELTA hot updates.

#include "../include/UltraWebHTMLGenerator.h"
#include "../server/DevServer.h"
#include "../server/UltraWebServer.h"

#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace UltraWeb::Server;

namespace {

volatile std::sig_atomic_t gStop = 0;
void OnSignal(int) { gStop = 1; }

int Usage() {
    std::fprintf(stderr,
        "uws - UltraWeb Server\n"
        "usage: uws <app.ucpkg> [--port N] [--bind ADDR]\n"
        "       uws --dev <sourceDir> [--port N] [--bind ADDR] [--compress]\n");
    return 2;
}

void WaitForSignal() {
    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);
    while (!gStop) {
        // Sleep in short slices so signals are handled promptly
        struct timespec ts = {0, 200 * 1000 * 1000};
        nanosleep(&ts, nullptr);
    }
}

} // namespace

int main(int argc, char** argv) {
    bool devMode = false;
    bool compress = false;
    std::string target, bind = "127.0.0.1";
    uint16_t port = 8080;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--dev") {
            devMode = true;
        } else if (arg == "--compress") {
            compress = true;
        } else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::atoi(argv[++i]));
        } else if (arg == "--bind" && i + 1 < argc) {
            bind = argv[++i];
        } else if (arg[0] == '-') {
            return Usage();
        } else if (target.empty()) {
            target = arg;
        } else {
            return Usage();
        }
    }
    if (target.empty()) return Usage();

    if (devMode) {
        DevServerConfig config;
        config.sourceDir = target;
        config.bindAddress = bind;
        config.port = port;
        config.compress = compress;

        DevServer dev;
        std::string error;
        if (!dev.Start(config, error)) {
            std::fprintf(stderr, "uws: %s\n", error.c_str());
            return 1;
        }
        std::printf("uws: dev server on http://%s:%u (watching %s)\n",
                    bind.c_str(), dev.GetPort(), target.c_str());
        WaitForSignal();
        dev.Stop();
        return 0;
    }

    // Static package serving
    std::ifstream f(target, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "uws: cannot read %s\n", target.c_str());
        return 1;
    }
    std::vector<uint8_t> package((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());

    UltraWebServer server;
    UltraWebServer::Config config;
    config.bindAddress = bind;
    config.port = port;

    std::string error;
    if (!server.Start(config, error)) {
        std::fprintf(stderr, "uws: %s\n", error.c_str());
        return 1;
    }
    // Mode-A HTML-first index: crawler/fallback HTML generated from the
    // package itself (spec: Crawler & Fallback Rendering)
    {
        UltraWeb::Server::HTMLGenerator generator;
        UltraWeb::Server::HTMLPageMeta meta;
        meta.title = "UltraWeb Application";
        auto page = generator.GenerateFromPackage(package, meta);
        server.ServeStatic("/", "text/html",
            page.success ? page.html
                         : std::string("<!doctype html><title>UltraWeb</title>"
                                       "<p><a href=\"/app.ucpkg\">package</a></p>"));
    }
    server.ServePackage("/app.ucpkg", std::move(package));

    std::printf("uws: serving %s on http://%s:%u\n", target.c_str(),
                bind.c_str(), server.GetPort());
    WaitForSignal();
    server.Stop();
    return 0;
}
