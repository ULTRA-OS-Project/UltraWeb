// UltraWeb/include/UltraWebCompression.h
// Compression backend for .ucpkg payloads
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Thin wrapper around the compression primitive so the bundler and package
// reader do not depend on a specific library. The server-side build uses
// VirtualFS (UltraCanvas module, ULTRAWEB_USE_VIRTUALFS); the browser WASM
// runtime will instead embed a minimal LZ4 frame decoder (~16KB) to stay
// within the runtime size budget. Both sides speak the standard LZ4 frame
// format (magic 04 22 4D 18), so any conforming decoder can read a .ucpkg
// payload.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace UltraWeb {
namespace Compression {

// True when a compression backend was compiled in. When false, the bundler
// emits uncompressed packages (with a warning) and the reader rejects
// compressed ones.
bool IsAvailable();

// Compresses input into a standard LZ4 frame. Returns false on failure or
// when no backend is available; output is cleared on failure.
bool CompressLZ4(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);

// Decompresses a standard LZ4 frame. expectedSize pre-allocates the output
// buffer (pass the known uncompressed size; 0 if unknown). Returns false on
// corrupt input or when no backend is available.
bool DecompressLZ4(const uint8_t* data, size_t size,
                   std::vector<uint8_t>& output, size_t expectedSize = 0);

} // namespace Compression
} // namespace UltraWeb
