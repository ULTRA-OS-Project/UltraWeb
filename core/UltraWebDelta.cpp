// UltraWeb/core/UltraWebDelta.cpp
// UCDELTA serialization and parsing
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "../include/UltraWebDelta.h"

#include <cstring>

namespace UltraWeb {

namespace {

void PutU8(std::vector<uint8_t>& out, uint8_t v)  { out.push_back(v); }
void PutU16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>(v >> 8));
}
void PutU32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; i++) out.push_back(static_cast<uint8_t>(v >> (8 * i)));
}
void PutString(std::vector<uint8_t>& out, const std::string& s) {
    uint16_t len = static_cast<uint16_t>(
        s.size() > 0xFFFF ? 0xFFFF : s.size());
    PutU16(out, len);
    out.insert(out.end(), s.begin(), s.begin() + len);
}

struct Reader {
    const uint8_t* p;
    const uint8_t* end;
    bool ok = true;

    uint8_t U8()   { if (end - p < 1) { ok = false; return 0; } return *p++; }
    uint16_t U16() { if (end - p < 2) { ok = false; return 0; }
                     uint16_t v = static_cast<uint16_t>(p[0] | (p[1] << 8)); p += 2; return v; }
    uint32_t U32() { if (end - p < 4) { ok = false; return 0; }
                     uint32_t v; std::memcpy(&v, p, 4); p += 4; return v; }
    std::string Str() {
        uint16_t len = U16();
        if (!ok || end - p < len) { ok = false; return ""; }
        std::string s(reinterpret_cast<const char*>(p), len);
        p += len;
        return s;
    }
    std::vector<uint8_t> Bytes(uint32_t len) {
        if (!ok || static_cast<size_t>(end - p) < len) { ok = false; return {}; }
        std::vector<uint8_t> out(p, p + len);
        p += len;
        return out;
    }
};

} // namespace

void DeltaWriter::SetText(uint16_t elementId, const std::string& text) {
    DeltaOp op;
    op.type = DeltaOpType::SetText;
    op.elementId = elementId;
    op.stringValue = text;
    ops.push_back(std::move(op));
}

void DeltaWriter::SetValue(uint16_t elementId, const std::string& value) {
    DeltaOp op;
    op.type = DeltaOpType::SetValue;
    op.elementId = elementId;
    op.stringValue = value;
    ops.push_back(std::move(op));
}

void DeltaWriter::AddClass(uint16_t elementId, const std::string& className) {
    DeltaOp op;
    op.type = DeltaOpType::AddClass;
    op.elementId = elementId;
    op.stringValue = className;
    ops.push_back(std::move(op));
}

void DeltaWriter::RemoveClass(uint16_t elementId, const std::string& className) {
    DeltaOp op;
    op.type = DeltaOpType::RemoveClass;
    op.elementId = elementId;
    op.stringValue = className;
    ops.push_back(std::move(op));
}

void DeltaWriter::SetVisible(uint16_t elementId, bool visible) {
    DeltaOp op;
    op.type = DeltaOpType::SetVisible;
    op.elementId = elementId;
    op.boolValue = visible;
    ops.push_back(std::move(op));
}

void DeltaWriter::SetEnabled(uint16_t elementId, bool enabled) {
    DeltaOp op;
    op.type = DeltaOpType::SetEnabled;
    op.elementId = elementId;
    op.boolValue = enabled;
    ops.push_back(std::move(op));
}

void DeltaWriter::ReplaceSection(DeltaSectionId sectionId,
                                 const std::vector<uint8_t>& data) {
    DeltaOp op;
    op.type = DeltaOpType::ReplaceSection;
    op.sectionId = sectionId;
    op.sectionData = data;
    ops.push_back(std::move(op));
}

std::vector<uint8_t> DeltaWriter::Serialize() const {
    std::vector<uint8_t> out;
    PutU32(out, UCDELTA_MAGIC);
    PutU16(out, UCDELTA_VERSION);
    PutU16(out, 0);  // flags
    PutU32(out, static_cast<uint32_t>(ops.size()));
    PutU32(out, baseCrc32);
    PutU32(out, targetCrc32);

    for (const auto& op : ops) {
        PutU8(out, static_cast<uint8_t>(op.type));
        switch (op.type) {
            case DeltaOpType::SetText:
            case DeltaOpType::SetValue:
            case DeltaOpType::AddClass:
            case DeltaOpType::RemoveClass:
                PutU16(out, op.elementId);
                PutString(out, op.stringValue);
                break;
            case DeltaOpType::SetVisible:
            case DeltaOpType::SetEnabled:
                PutU16(out, op.elementId);
                PutU8(out, op.boolValue ? 1 : 0);
                break;
            case DeltaOpType::ReplaceSection:
                PutU8(out, static_cast<uint8_t>(op.sectionId));
                PutU32(out, static_cast<uint32_t>(op.sectionData.size()));
                out.insert(out.end(), op.sectionData.begin(), op.sectionData.end());
                break;
        }
    }
    return out;
}

DeltaParseResult ParseDelta(const uint8_t* data, size_t size) {
    DeltaParseResult result;
    Reader r{data, data + size};

    if (r.U32() != UCDELTA_MAGIC) {
        result.error = "not a UCDELTA stream (bad magic)";
        return result;
    }
    result.version = r.U16();
    if (result.version > UCDELTA_VERSION) {
        result.error = "unsupported UCDELTA version";
        return result;
    }
    r.U16();  // flags
    uint32_t opCount = r.U32();
    result.baseCrc32 = r.U32();
    result.targetCrc32 = r.U32();
    if (!r.ok) {
        result.error = "truncated UCDELTA header";
        return result;
    }

    result.ops.reserve(opCount);
    for (uint32_t i = 0; i < opCount && r.ok; i++) {
        DeltaOp op;
        op.type = static_cast<DeltaOpType>(r.U8());
        switch (op.type) {
            case DeltaOpType::SetText:
            case DeltaOpType::SetValue:
            case DeltaOpType::AddClass:
            case DeltaOpType::RemoveClass:
                op.elementId = r.U16();
                op.stringValue = r.Str();
                break;
            case DeltaOpType::SetVisible:
            case DeltaOpType::SetEnabled:
                op.elementId = r.U16();
                op.boolValue = r.U8() != 0;
                break;
            case DeltaOpType::ReplaceSection: {
                op.sectionId = static_cast<DeltaSectionId>(r.U8());
                uint32_t len = r.U32();
                op.sectionData = r.Bytes(len);
                break;
            }
            default:
                result.error = "unknown delta op 0x" +
                               std::to_string(static_cast<int>(op.type));
                return result;
        }
        if (r.ok) result.ops.push_back(std::move(op));
    }

    if (!r.ok || result.ops.size() != opCount) {
        result.error = "truncated UCDELTA op stream";
        result.ops.clear();
        return result;
    }

    result.success = true;
    return result;
}

DeltaParseResult ParseDelta(const std::vector<uint8_t>& data) {
    return ParseDelta(data.data(), data.size());
}

} // namespace UltraWeb
