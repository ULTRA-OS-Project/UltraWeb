// UltraWeb/tools/uwb.cpp
// UltraWeb Bundler CLI - packages sources/binaries into a .ucpkg
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Usage:
//   uwb --ui app.ucml [--css app.css] [--js app.js | --hbc app.hbc]
//       [-o app.ucpkg] [--compress] [--debug]
// .ucml/.css compile inline; .js compiles via hermesc when available and
// otherwise ships as source (executed by the dev-mode engine).

#include "../include/UltraWebBundler.h"
#include "../server/HermesCompiler.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace UltraWeb;

namespace {

bool ReadFileText(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

bool ReadFileBytes(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

int Usage() {
    std::fprintf(stderr,
        "uwb - UltraWeb Bundler\n"
        "usage: uwb --ui app.ucml [--css app.css] [--js app.js | --hbc app.hbc]\n"
        "           [-o app.ucpkg] [--compress] [--debug]\n");
    return 2;
}

} // namespace

int main(int argc, char** argv) {
    std::string uiPath, cssPath, jsPath, hbcPath, output = "app.ucpkg";
    BundleConfig config;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        auto next = [&](std::string& target) -> bool {
            if (i + 1 >= argc) return false;
            target = argv[++i];
            return true;
        };
        if (arg == "--ui")            { if (!next(uiPath)) return Usage(); }
        else if (arg == "--css")      { if (!next(cssPath)) return Usage(); }
        else if (arg == "--js")       { if (!next(jsPath)) return Usage(); }
        else if (arg == "--hbc")      { if (!next(hbcPath)) return Usage(); }
        else if (arg == "-o")         { if (!next(output)) return Usage(); }
        else if (arg == "--compress") { config.enableCompression = true; }
        else if (arg == "--debug")    { config.includeDebugInfo = true; }
        else return Usage();
    }
    if (uiPath.empty() || (!jsPath.empty() && !hbcPath.empty())) return Usage();

    PackageBundler bundler;
    bundler.SetConfig(config);

    std::string ucml;
    if (!ReadFileText(uiPath, ucml)) {
        std::fprintf(stderr, "uwb: cannot read %s\n", uiPath.c_str());
        return 1;
    }
    if (!bundler.SetUIFromSource(ucml)) {
        std::fprintf(stderr, "uwb: UCML compilation failed\n");
        return 1;
    }

    if (!cssPath.empty()) {
        std::string css;
        if (!ReadFileText(cssPath, css)) {
            std::fprintf(stderr, "uwb: cannot read %s\n", cssPath.c_str());
            return 1;
        }
        if (!bundler.SetStyleFromSource(css)) {
            std::fprintf(stderr, "uwb: CSS compilation failed\n");
            return 1;
        }
    }

    if (!hbcPath.empty()) {
        std::vector<uint8_t> hbc;
        if (!ReadFileBytes(hbcPath, hbc)) {
            std::fprintf(stderr, "uwb: cannot read %s\n", hbcPath.c_str());
            return 1;
        }
        if (!Server::HermesCompiler::IsHermesBytecode(hbc)) {
            std::fprintf(stderr, "uwb: %s is not Hermes bytecode\n",
                         hbcPath.c_str());
            return 1;
        }
        bundler.SetCodeSection(hbc);
    } else if (!jsPath.empty()) {
        Server::HermesCompiler hermes;
        if (hermes.IsAvailable()) {
            auto compiled = hermes.CompileFile(jsPath);
            if (!compiled.success) {
                std::fprintf(stderr, "uwb: %s\n", compiled.error.c_str());
                return 1;
            }
            bundler.SetCodeSection(compiled.bytecode);
        } else {
            std::string js;
            if (!ReadFileText(jsPath, js)) {
                std::fprintf(stderr, "uwb: cannot read %s\n", jsPath.c_str());
                return 1;
            }
            std::fprintf(stderr,
                "uwb: hermesc not found; shipping JS as source (dev mode)\n");
            bundler.SetCodeSection(std::vector<uint8_t>(js.begin(), js.end()));
        }
    }

    BundleResult result;
    if (!bundler.BundleToFile(output, result)) {
        std::fprintf(stderr, "uwb: bundling failed\n");
        for (const auto& e : result.errors) {
            std::fprintf(stderr, "uwb:   %s\n", e.c_str());
        }
        return 1;
    }
    for (const auto& w : result.warnings) {
        std::fprintf(stderr, "uwb: warning: %s\n", w.c_str());
    }
    std::printf("uwb: %s (%zu bytes", output.c_str(), result.totalSize);
    if (result.totalSize != result.uncompressedSize) {
        std::printf(", %zu uncompressed", result.uncompressedSize);
    }
    std::printf(")\n  UI %zu B, styles %zu B, code %zu B, assets %zu B\n",
                result.uiSize, result.styleSize, result.codeSize,
                result.assetSize);
    return 0;
}
