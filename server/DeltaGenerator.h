// UltraWeb/server/DeltaGenerator.h
// Incremental update generation - diffs two .ucpkg packages into a UCDELTA
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Strategy: when the UI trees are structurally identical (same element ids,
// types and parents), changes are expressed as element-level ops (text,
// value, classes, visibility) - the "2-5KB per interaction" path. When the
// structure changed, or for the style/code/asset sections, the whole
// section is replaced. Falls back cleanly: a full-section replace delta is
// still smaller than re-sending the package (headers and unchanged
// sections are omitted).

#pragma once

#include "../include/UltraWebDelta.h"

#include <cstdint>
#include <string>
#include <vector>

namespace UltraWeb {
namespace Server {

struct DeltaResult {
    bool success = false;
    std::string error;
    std::vector<uint8_t> delta;    // UCDELTA stream
    size_t opCount = 0;
    bool identical = false;        // packages equal - delta has zero ops
    bool uiStructureChanged = false;  // fell back to UI section replace
};

class DeltaGenerator {
public:
    // Diffs two .ucpkg packages (compressed or raw). The delta's baseCrc32
    // is the old package's content CRC; targetCrc32 the new one's.
    DeltaResult Generate(const std::vector<uint8_t>& oldPackage,
                         const std::vector<uint8_t>& newPackage);
};

} // namespace Server
} // namespace UltraWeb
