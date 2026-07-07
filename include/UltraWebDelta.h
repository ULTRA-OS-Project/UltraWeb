// UltraWeb/include/UltraWebDelta.h
// UCDELTA binary format - incremental updates from server to client
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// A delta is the unit of the "2-5KB per interaction" goal: instead of
// re-sending a .ucpkg, the server sends a compact op stream that mutates
// the client's loaded application in place. Both sides share this module:
// the server's DeltaGenerator writes deltas, the client runtime applies
// them (UltraWebRuntime::ApplyDelta).
//
// Layout (little-endian):
//   UCDeltaHeader (20 bytes)
//     magic       u32  'UCDT' (0x54444355)
//     version     u16
//     flags       u16  (reserved, 0)
//     opCount     u32
//     baseCrc32   u32  CRC32 of the uncompressed content of the package this
//                      delta was generated against; the client refuses deltas
//                      whose base does not match its loaded state
//     targetCrc32 u32  CRC32 of the package content after applying; the
//                      client adopts it as its new state so deltas chain
//   opCount operations, each:
//     op u8, then per-op payload (strings are u16 length + UTF-8 bytes):
//       SetText        elementId u16, string
//       SetValue       elementId u16, string
//       AddClass       elementId u16, string
//       RemoveClass    elementId u16, string
//       SetVisible     elementId u16, u8
//       SetEnabled     elementId u16, u8
//       ReplaceSection sectionId u8 (UI/Style/Code/Assets), u32 size, bytes

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace UltraWeb {

constexpr uint32_t UCDELTA_MAGIC = 0x54444355;  // 'UCDT'
constexpr uint16_t UCDELTA_VERSION = 0x0001;

enum class DeltaOpType : uint8_t {
    SetText        = 0x01,
    SetValue       = 0x02,
    AddClass       = 0x03,
    RemoveClass    = 0x04,
    SetVisible     = 0x05,
    SetEnabled     = 0x06,
    ReplaceSection = 0x10
};

enum class DeltaSectionId : uint8_t {
    UI     = 0x01,
    Style  = 0x02,
    Code   = 0x03,
    Assets = 0x04
};

struct DeltaOp {
    DeltaOpType type = DeltaOpType::SetText;
    uint16_t elementId = 0;              // element ops
    std::string stringValue;             // text / class ops
    bool boolValue = false;              // visible / enabled ops
    DeltaSectionId sectionId = DeltaSectionId::UI;  // ReplaceSection
    std::vector<uint8_t> sectionData;               // ReplaceSection
};

// Serializes ops into a UCDELTA byte stream
class DeltaWriter {
public:
    DeltaWriter(uint32_t baseCrc32, uint32_t targetCrc32)
        : baseCrc32(baseCrc32), targetCrc32(targetCrc32) {}

    void SetText(uint16_t elementId, const std::string& text);
    void SetValue(uint16_t elementId, const std::string& value);
    void AddClass(uint16_t elementId, const std::string& className);
    void RemoveClass(uint16_t elementId, const std::string& className);
    void SetVisible(uint16_t elementId, bool visible);
    void SetEnabled(uint16_t elementId, bool enabled);
    void ReplaceSection(DeltaSectionId sectionId, const std::vector<uint8_t>& data);

    size_t GetOpCount() const { return ops.size(); }
    std::vector<uint8_t> Serialize() const;

private:
    uint32_t baseCrc32;
    uint32_t targetCrc32;
    std::vector<DeltaOp> ops;
};

// Parses a UCDELTA byte stream
struct DeltaParseResult {
    bool success = false;
    std::string error;
    uint16_t version = 0;
    uint32_t baseCrc32 = 0;
    uint32_t targetCrc32 = 0;
    std::vector<DeltaOp> ops;
};

DeltaParseResult ParseDelta(const uint8_t* data, size_t size);
DeltaParseResult ParseDelta(const std::vector<uint8_t>& data);

} // namespace UltraWeb
