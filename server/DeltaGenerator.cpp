// UltraWeb/server/DeltaGenerator.cpp
// Incremental update generation implementation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "DeltaGenerator.h"

#include "../include/UltraWebBundler.h"
#include "../runtime/UCBLoader.h"

#include <algorithm>
#include <set>

namespace UltraWeb {
namespace Server {

namespace {

// Class names of an element, resolved through the loader's tables
std::set<std::string> ClassSet(const Runtime::UCBLoader& loader,
                               const Runtime::ElementNode& el) {
    std::set<std::string> out;
    for (uint16_t classId : el.styleClasses) {
        const std::string& name = loader.GetStyleClassName(classId);
        if (!name.empty()) out.insert(name);
    }
    return out;
}

bool SameStructure(const std::vector<Runtime::ElementNode>& a,
                   const std::vector<Runtime::ElementNode>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i].elementId != b[i].elementId ||
            a[i].parentId != b[i].parentId ||
            a[i].type != b[i].type) {
            return false;
        }
    }
    return true;
}

} // namespace

DeltaResult DeltaGenerator::Generate(const std::vector<uint8_t>& oldPackage,
                                     const std::vector<uint8_t>& newPackage) {
    DeltaResult result;

    PackageReader oldReader, newReader;
    if (!oldReader.Open(oldPackage)) {
        result.error = "cannot open old package";
        return result;
    }
    if (!newReader.Open(newPackage)) {
        result.error = "cannot open new package";
        return result;
    }
    if (!oldReader.VerifyCRC() || !newReader.VerifyCRC()) {
        result.error = "package CRC verification failed";
        return result;
    }

    PackageInfo oldInfo = oldReader.GetInfo();
    PackageInfo newInfo = newReader.GetInfo();
    DeltaWriter writer(oldInfo.crc32, newInfo.crc32);

    if (oldInfo.crc32 == newInfo.crc32) {
        // Byte-identical content: an empty delta
        result.identical = true;
        result.delta = writer.Serialize();
        result.success = true;
        return result;
    }

    // ----- UI section -----
    auto oldUI = oldReader.ExtractUISection();
    auto newUI = newReader.ExtractUISection();
    if (oldUI != newUI) {
        Runtime::UCBLoader oldLoader, newLoader;
        bool parsed = !oldUI.empty() && !newUI.empty() &&
                      oldLoader.Load(oldUI).success &&
                      newLoader.Load(newUI).success;

        if (parsed && SameStructure(oldLoader.GetElements(),
                                    newLoader.GetElements())) {
            // Element-level ops
            const auto& oldEls = oldLoader.GetElements();
            const auto& newEls = newLoader.GetElements();
            for (size_t i = 0; i < newEls.size(); i++) {
                const auto& o = oldEls[i];
                const auto& n = newEls[i];

                if (o.textContent != n.textContent) {
                    writer.SetText(n.elementId, n.textContent);
                }

                std::string oldValue = o.GetStringProperty(UCBPropertyId::Value);
                std::string newValue = n.GetStringProperty(UCBPropertyId::Value);
                if (oldValue != newValue) {
                    writer.SetValue(n.elementId, newValue);
                }

                bool oldVisible = o.GetBoolProperty(UCBPropertyId::Visible, true);
                bool newVisible = n.GetBoolProperty(UCBPropertyId::Visible, true);
                if (oldVisible != newVisible) {
                    writer.SetVisible(n.elementId, newVisible);
                }

                bool oldEnabled = o.GetBoolProperty(UCBPropertyId::Enabled, true);
                bool newEnabled = n.GetBoolProperty(UCBPropertyId::Enabled, true);
                if (oldEnabled != newEnabled) {
                    writer.SetEnabled(n.elementId, newEnabled);
                }

                std::set<std::string> oldClasses = ClassSet(oldLoader, o);
                std::set<std::string> newClasses = ClassSet(newLoader, n);
                for (const auto& cls : newClasses) {
                    if (!oldClasses.count(cls)) writer.AddClass(n.elementId, cls);
                }
                for (const auto& cls : oldClasses) {
                    if (!newClasses.count(cls)) writer.RemoveClass(n.elementId, cls);
                }
            }
        } else {
            // Structure changed (or a side is empty/unparseable): replace
            result.uiStructureChanged = true;
            writer.ReplaceSection(DeltaSectionId::UI, newUI);
        }
    }

    // ----- other sections: byte compare, replace on change -----
    if (oldReader.ExtractStyleSection() != newReader.ExtractStyleSection()) {
        writer.ReplaceSection(DeltaSectionId::Style,
                              newReader.ExtractStyleSection());
    }
    if (oldReader.ExtractCodeSection() != newReader.ExtractCodeSection()) {
        writer.ReplaceSection(DeltaSectionId::Code,
                              newReader.ExtractCodeSection());
    }
    if (oldReader.ExtractAssetSection() != newReader.ExtractAssetSection()) {
        writer.ReplaceSection(DeltaSectionId::Assets,
                              newReader.ExtractAssetSection());
    }

    result.opCount = writer.GetOpCount();
    result.delta = writer.Serialize();
    result.success = true;
    return result;
}

} // namespace Server
} // namespace UltraWeb
