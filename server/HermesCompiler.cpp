// UltraWeb/server/HermesCompiler.cpp
// hermesc CLI wrapper implementation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "HermesCompiler.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <unistd.h>

namespace UltraWeb {
namespace Server {

namespace {

const uint8_t kHermesMagic[8] = {0xC6, 0x1F, 0xBC, 0x03, 0xC1, 0x03, 0x19, 0x1F};

// Runs a command, capturing combined stdout/stderr; returns exit status
int RunCommand(const std::string& command, std::string& output) {
    output.clear();
    FILE* pipe = ::popen((command + " 2>&1").c_str(), "r");
    if (!pipe) return -1;
    std::array<char, 4096> buf;
    size_t n;
    while ((n = std::fread(buf.data(), 1, buf.size(), pipe)) > 0) {
        output.append(buf.data(), n);
    }
    return ::pclose(pipe);
}

std::string ShellQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

} // namespace

bool HermesCompiler::IsHermesBytecode(const uint8_t* data, size_t size) {
    return data && size >= sizeof(kHermesMagic) &&
           std::memcmp(data, kHermesMagic, sizeof(kHermesMagic)) == 0;
}

bool HermesCompiler::IsHermesBytecode(const std::vector<uint8_t>& data) {
    return IsHermesBytecode(data.data(), data.size());
}

uint32_t HermesCompiler::GetBytecodeVersion(const std::vector<uint8_t>& data) {
    if (!IsHermesBytecode(data) || data.size() < 12) return 0;
    uint32_t version;
    std::memcpy(&version, data.data() + 8, sizeof(version));
    return version;
}

std::string HermesCompiler::FindHermesc() const {
    if (!hermescPath.empty()) {
        return std::filesystem::exists(hermescPath) ? hermescPath : "";
    }
    if (const char* env = std::getenv("ULTRAWEB_HERMESC")) {
        if (std::filesystem::exists(env)) return env;
    }
    std::string output;
    if (RunCommand("command -v hermesc", output) == 0 && !output.empty()) {
        while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
            output.pop_back();
        }
        return output;
    }
    return "";
}

HermesCompileResult HermesCompiler::CompileSource(const std::string& jsSource,
                                                  const std::string& sourceName) {
    HermesCompileResult result;

    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() /
                   ("ultraweb-hermesc-" + std::to_string(
                        static_cast<unsigned long>(::getpid())));
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        result.error = "cannot create temp directory: " + dir.string();
        return result;
    }

    fs::path jsPath = dir / (sourceName.empty() ? "app.js" : sourceName);
    {
        std::ofstream f(jsPath, std::ios::binary);
        if (!f) {
            result.error = "cannot write temp source file: " + jsPath.string();
            return result;
        }
        f.write(jsSource.data(), static_cast<std::streamsize>(jsSource.size()));
    }

    result = CompileFile(jsPath.string());
    fs::remove_all(dir, ec);
    return result;
}

HermesCompileResult HermesCompiler::CompileFile(const std::string& jsPath) {
    HermesCompileResult result;

    std::string hermesc = FindHermesc();
    if (hermesc.empty()) {
        result.error = "hermesc not found (set ULTRAWEB_HERMESC or add it to PATH); "
                       "hermesc ships with the Hermes SDK or the "
                       "hermes-engine-cli npm package";
        return result;
    }
    if (!std::filesystem::exists(jsPath)) {
        result.error = "source file not found: " + jsPath;
        return result;
    }

    std::string hbcPath = jsPath + ".hbc";
    std::string command = ShellQuote(hermesc) + " -O -emit-binary -out " +
                          ShellQuote(hbcPath) + " " + ShellQuote(jsPath);

    std::string output;
    int status = RunCommand(command, output);
    if (status != 0) {
        result.error = output.empty() ? "hermesc failed" : output;
        return result;
    }

    std::ifstream f(hbcPath, std::ios::binary);
    if (!f) {
        result.error = "hermesc produced no output file";
        return result;
    }
    result.bytecode.assign(std::istreambuf_iterator<char>(f),
                           std::istreambuf_iterator<char>());
    std::error_code ec;
    std::filesystem::remove(hbcPath, ec);

    if (!IsHermesBytecode(result.bytecode)) {
        result.error = "hermesc output is not valid Hermes bytecode";
        result.bytecode.clear();
        return result;
    }

    result.success = true;
    return result;
}

} // namespace Server
} // namespace UltraWeb
