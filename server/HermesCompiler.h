// UltraWeb/server/HermesCompiler.h
// JS to Hermes bytecode compiler (hermesc CLI wrapper)
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Server-side compilation of application JavaScript to .hbc bytecode for
// the code section of a .ucpkg bundle. Wraps the hermesc binary (shipped
// with the Hermes SDK, or via the hermes-engine-cli npm package). Lookup
// order: explicit SetHermescPath, ULTRAWEB_HERMESC env var, then PATH.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace UltraWeb {
namespace Server {

struct HermesCompileResult {
    bool success = false;
    std::string error;              // compiler diagnostics on failure
    std::vector<uint8_t> bytecode;  // .hbc bytes on success
};

class HermesCompiler {
public:
    // True when the buffer starts with the Hermes bytecode magic
    // (C6 1F BC 03 C1 03 19 1F)
    static bool IsHermesBytecode(const uint8_t* data, size_t size);
    static bool IsHermesBytecode(const std::vector<uint8_t>& data);

    // Bytecode format version (u32 at offset 8), 0 if not Hermes bytecode
    static uint32_t GetBytecodeVersion(const std::vector<uint8_t>& data);

    // Override hermesc discovery with an explicit binary path
    void SetHermescPath(const std::string& path) { hermescPath = path; }

    // Resolved hermesc path, empty when unavailable
    std::string FindHermesc() const;
    bool IsAvailable() const { return !FindHermesc().empty(); }

    // Compile JS source text / a .js file to bytecode. -O optimized output.
    HermesCompileResult CompileSource(const std::string& jsSource,
                                      const std::string& sourceName = "app.js");
    HermesCompileResult CompileFile(const std::string& jsPath);

private:
    std::string hermescPath;
};

} // namespace Server
} // namespace UltraWeb
