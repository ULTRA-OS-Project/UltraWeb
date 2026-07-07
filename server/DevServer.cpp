// UltraWeb/server/DevServer.cpp
// Development server implementation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "DevServer.h"

#include "../include/UltraWebBundler.h"
#include "../runtime/JSONUtil.h"
#include "DeltaGenerator.h"
#include "HermesCompiler.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace UltraWeb {
namespace Server {

namespace {

bool ReadTextFile(const fs::path& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f),
               std::istreambuf_iterator<char>());
    return true;
}

} // namespace

DevServer::~DevServer() { Stop(); }

std::string DevServer::ShellPage() {
    return "<!doctype html><title>UltraWeb Dev Server</title>"
           "<p>UltraWeb development server. Package: "
           "<a href=\"/app.ucpkg\">/app.ucpkg</a>; connect a WebSocket to "
           "this origin for UCDELTA hot updates.</p>";
}

std::vector<uint8_t> DevServer::BuildPackage(std::string& error) {
    fs::path dir(config.sourceDir);

    std::string ucml;
    if (!ReadTextFile(dir / config.ucmlFile, ucml)) {
        error = "cannot read " + (dir / config.ucmlFile).string();
        return {};
    }

    PackageBundler bundler;
    BundleConfig bundleConfig;
    bundleConfig.enableCompression = config.compress;
    bundler.SetConfig(bundleConfig);

    if (!bundler.SetUIFromSource(ucml)) {
        error = "UCML compilation failed: " + config.ucmlFile;
        return {};
    }

    std::string css;
    if (ReadTextFile(dir / config.cssFile, css) && !css.empty()) {
        if (!bundler.SetStyleFromSource(css)) {
            error = "CSS compilation failed: " + config.cssFile;
            return {};
        }
    }

    std::string js;
    if (ReadTextFile(dir / config.jsFile, js) && !js.empty()) {
        // Prefer Hermes bytecode; fall back to plain source, which the
        // runtime executes in dev mode (Phase 3 ExecuteCodeSection)
        HermesCompiler hermes;
        if (hermes.IsAvailable()) {
            auto compiled = hermes.CompileSource(js, config.jsFile);
            if (!compiled.success) {
                error = "JS compilation failed: " + compiled.error;
                return {};
            }
            bundler.SetCodeSection(compiled.bytecode);
        } else {
            bundler.SetCodeSection(std::vector<uint8_t>(js.begin(), js.end()));
        }
    }

    BundleResult result = bundler.Bundle();
    if (!result.success) {
        error = "bundling failed";
        for (const auto& e : result.errors) error += "; " + e;
        return {};
    }
    return result.data;
}

bool DevServer::Start(const DevServerConfig& cfg, std::string& error) {
    config = cfg;

    std::vector<uint8_t> package = BuildPackage(error);
    if (package.empty()) return false;
    {
        std::lock_guard<std::mutex> lock(packageMutex);
        currentPackage = package;
    }

    UltraWebServer::Config serverConfig;
    serverConfig.bindAddress = config.bindAddress;
    serverConfig.port = config.port;
    if (!server.Start(serverConfig, error)) return false;

    server.ServeStatic("/", "text/html", ShellPage());
    server.ServePackage("/app.ucpkg", std::move(package));

    watcher.AddPath(config.sourceDir);
    watcher.SetExtensionFilter({".ucml", ".css", ".js"});
    watcher.Start(config.pollIntervalMs,
                  [this](const std::vector<std::string>&) {
                      std::string rebuildError;
                      RebuildAndPush(rebuildError);
                  });
    return true;
}

bool DevServer::RebuildAndPush(std::string& error) {
    std::vector<uint8_t> newPackage = BuildPackage(error);
    if (newPackage.empty()) {
        // Compile error: tell connected clients instead of going silent
        server.BroadcastText("{\"type\":\"error\",\"message\":" +
                             Runtime::JSONValue::Quote(error) + "}");
        return false;
    }

    std::vector<uint8_t> oldPackage;
    {
        std::lock_guard<std::mutex> lock(packageMutex);
        oldPackage = currentPackage;
        currentPackage = newPackage;
    }
    server.UpdateRoute("/app.ucpkg", newPackage);

    DeltaGenerator generator;
    DeltaResult delta = generator.Generate(oldPackage, newPackage);
    if (delta.success && !delta.identical) {
        lastDeltaSize = delta.delta.size();
        lastDeltaOpCount = delta.opCount;
        server.BroadcastBinary(delta.delta);
    } else if (!delta.success) {
        // No delta possible: full reload
        server.BroadcastText("{\"type\":\"reload\"}");
    }
    return true;
}

std::vector<uint8_t> DevServer::GetCurrentPackage() const {
    std::lock_guard<std::mutex> lock(packageMutex);
    return currentPackage;
}

void DevServer::Stop() {
    watcher.Stop();
    server.Stop();
}

} // namespace Server
} // namespace UltraWeb
