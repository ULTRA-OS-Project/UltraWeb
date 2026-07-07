// UltraWeb/core/UltraWebCompression.cpp
// Compression backend implementation (VirtualFS-backed)
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "../include/UltraWebCompression.h"

#ifdef ULTRAWEB_USE_VIRTUALFS
#include <VirtualFS/VirtualFSCompression.h>
#endif

namespace UltraWeb {
namespace Compression {

#ifdef ULTRAWEB_USE_VIRTUALFS

bool IsAvailable() {
    return VirtualFS::VirtualFS_IsCompressionMethodAvailable(
        VirtualFS::VirtualFSCompressionMethod::LZ4);
}

bool CompressLZ4(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    return VirtualFS::VirtualFS_CompressBuffer(
               input, output,
               VirtualFS::VirtualFSCompressionMethod::LZ4)
           == VirtualFS::VirtualFSResult::Success;
}

bool DecompressLZ4(const uint8_t* data, size_t size,
                   std::vector<uint8_t>& output, size_t expectedSize) {
    return VirtualFS::VirtualFS_DecompressBuffer(
               data, size, output,
               VirtualFS::VirtualFSCompressionMethod::LZ4, expectedSize)
           == VirtualFS::VirtualFSResult::Success;
}

#else // no backend

bool IsAvailable() {
    return false;
}

bool CompressLZ4(const std::vector<uint8_t>&, std::vector<uint8_t>& output) {
    output.clear();
    return false;
}

bool DecompressLZ4(const uint8_t*, size_t, std::vector<uint8_t>& output, size_t) {
    output.clear();
    return false;
}

#endif // ULTRAWEB_USE_VIRTUALFS

} // namespace Compression
} // namespace UltraWeb
