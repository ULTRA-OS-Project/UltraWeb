// UltraWeb/tools/uwc.cpp
// UltraWeb Compiler CLI - compiles .ucml / .css / .js to binary formats
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Usage:
//   uwc <input.ucml> [-o out.ucb]     UCML  -> UCB binary UI
//   uwc <input.css>  [-o out.ucs]     CSS   -> UCS binary styles
//   uwc <input.js>   [-o out.hbc]     JS    -> Hermes bytecode (hermesc)
// The output path defaults to the input with the binary extension.

#include "../include/UltraWebCSSCompiler.h"
#include "../include/UltraWebUICompiler.h"
#include "../server/HermesCompiler.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace UltraWeb;

namespace {

bool ReadFile(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

bool WriteFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
    return f.good();
}

std::string Extension(const std::string& path) {
    size_t dot = path.rfind('.');
    return dot == std::string::npos ? "" : path.substr(dot);
}

std::string ReplaceExtension(const std::string& path, const std::string& ext) {
    size_t dot = path.rfind('.');
    return (dot == std::string::npos ? path : path.substr(0, dot)) + ext;
}

int Usage() {
    std::fprintf(stderr,
        "uwc - UltraWeb Compiler\n"
        "usage: uwc <input.ucml|input.css|input.js> [-o output]\n"
        "  .ucml -> .ucb (binary UI)\n"
        "  .css  -> .ucs (binary styles)\n"
        "  .js   -> .hbc (Hermes bytecode; needs hermesc via PATH or ULTRAWEB_HERMESC)\n");
    return 2;
}

} // namespace

int main(int argc, char** argv) {
    std::string input, output;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else if (argv[i][0] == '-') {
            return Usage();
        } else if (input.empty()) {
            input = argv[i];
        } else {
            return Usage();
        }
    }
    if (input.empty()) return Usage();

    std::string source;
    if (!ReadFile(input, source)) {
        std::fprintf(stderr, "uwc: cannot read %s\n", input.c_str());
        return 1;
    }

    std::string ext = Extension(input);
    std::vector<uint8_t> binary;

    if (ext == ".ucml") {
        UICompiler compiler;
        UICompilationResult result = compiler.Compile(source);
        if (!result.success) {
            for (const auto& e : result.errors) {
                std::fprintf(stderr, "uwc: %s\n", e.c_str());
            }
            return 1;
        }
        binary = std::move(result.data);
        if (output.empty()) output = ReplaceExtension(input, ".ucb");
    } else if (ext == ".css") {
        CSSCompiler compiler;
        CompilationResult result = compiler.Compile(source);
        if (!result.success) {
            for (const auto& e : result.errors) {
                std::fprintf(stderr, "uwc: %s\n", e.c_str());
            }
            return 1;
        }
        binary = std::move(result.data);
        if (output.empty()) output = ReplaceExtension(input, ".ucs");
    } else if (ext == ".js") {
        Server::HermesCompiler hermes;
        if (!hermes.IsAvailable()) {
            std::fprintf(stderr,
                "uwc: hermesc not found (set ULTRAWEB_HERMESC or add to PATH)\n");
            return 1;
        }
        auto result = hermes.CompileFile(input);
        if (!result.success) {
            std::fprintf(stderr, "uwc: %s\n", result.error.c_str());
            return 1;
        }
        binary = std::move(result.bytecode);
        if (output.empty()) output = ReplaceExtension(input, ".hbc");
    } else {
        return Usage();
    }

    if (binary.empty()) {
        std::fprintf(stderr, "uwc: compilation produced no output\n");
        return 1;
    }
    if (!WriteFile(output, binary)) {
        std::fprintf(stderr, "uwc: cannot write %s\n", output.c_str());
        return 1;
    }
    std::printf("uwc: %s -> %s (%zu bytes)\n", input.c_str(), output.c_str(),
                binary.size());
    return 0;
}
