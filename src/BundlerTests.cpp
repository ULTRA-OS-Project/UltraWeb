// UltraWeb/tests/BundlerTests.cpp
// Unit tests for Package Bundler
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "../include/UltraWebBundler.h"
#include <iostream>
#include <cassert>
#include <iomanip>
#include <cstdio>

using namespace UltraWeb;

// ============================================================================
// TEST HELPERS
// ============================================================================

void PrintTestResult(const std::string& testName, bool passed) {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << testName << std::endl;
}

void PrintSeparator() {
    std::cout << std::string(60, '=') << std::endl;
}

// ============================================================================
// CRC32 TESTS
// ============================================================================

bool TestCRC32_EmptyData() {
    uint32_t crc = CRC32::Calculate(nullptr, 0);
    // CRC32 of empty data should be 0
    return crc == 0;
}

bool TestCRC32_SimpleData() {
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
    uint32_t crc = CRC32::Calculate(data);
    
    // Known CRC32 of "Hello"
    return crc == 0xF7D18982;
}

bool TestCRC32_Incremental() {
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
    
    CRC32 crc;
    crc.Update(data.data(), 2);  // "He"
    crc.Update(data.data() + 2, 3);  // "llo"
    uint32_t result = crc.Finalize();
    
    uint32_t direct = CRC32::Calculate(data);
    
    return result == direct;
}

// ============================================================================
// ASSET MANAGER TESTS
// ============================================================================

bool TestAssetManager_AddFromMemory() {
    AssetManager manager;
    
    std::vector<uint8_t> data = {0x89, 0x50, 0x4E, 0x47};  // PNG header
    
    bool added = manager.AddAsset("test.png", data, UCAAssetType::Image);
    if (!added) return false;
    
    if (manager.GetCount() != 1) return false;
    
    const Asset* asset = manager.GetAsset("test.png");
    if (!asset) return false;
    
    if (asset->data != data) return false;
    if (asset->type != UCAAssetType::Image) return false;
    
    return true;
}

bool TestAssetManager_DuplicateRejected() {
    AssetManager manager;
    
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    
    bool first = manager.AddAsset("test", data, UCAAssetType::Data);
    bool second = manager.AddAsset("test", data, UCAAssetType::Data);
    
    return first && !second;
}

bool TestAssetManager_BuildUCA() {
    AssetManager manager;
    
    std::vector<uint8_t> data1 = {0x01, 0x02, 0x03, 0x04};
    std::vector<uint8_t> data2 = {0x05, 0x06, 0x07, 0x08, 0x09};
    
    manager.AddAsset("asset1", data1, UCAAssetType::Data);
    manager.AddAsset("asset2", data2, UCAAssetType::Binary);
    
    std::vector<uint8_t> uca = manager.BuildUCA();
    
    // Check magic number
    if (uca.size() < 16) return false;
    
    uint32_t magic = *reinterpret_cast<uint32_t*>(uca.data());
    if (magic != UCA_MAGIC) {
        std::cout << "  UCA magic: 0x" << std::hex << magic 
                  << " (expected 0x" << UCA_MAGIC << ")" << std::dec << std::endl;
        return false;
    }
    
    // Check asset count
    uint16_t count = *reinterpret_cast<uint16_t*>(uca.data() + 6);
    if (count != 2) {
        std::cout << "  Asset count: " << count << " (expected 2)" << std::endl;
        return false;
    }
    
    return true;
}

// ============================================================================
// BUNDLER TESTS
// ============================================================================

bool TestBundler_EmptyBundle() {
    PackageBundler bundler;
    
    BundleResult result = bundler.Bundle();
    
    // Empty bundle should succeed but have minimal size
    if (!result.success) return false;
    if (result.data.size() != sizeof(UCPKGHeader)) return false;
    
    return true;
}

bool TestBundler_MagicNumber() {
    PackageBundler bundler;
    
    // Add some content
    std::vector<uint8_t> ui = {0x01, 0x02, 0x03};
    bundler.SetUISection(ui);
    
    BundleResult result = bundler.Bundle();
    
    if (!result.success) return false;
    if (result.data.size() < 4) return false;
    
    uint32_t magic = *reinterpret_cast<uint32_t*>(result.data.data());
    if (magic != UCPKG_MAGIC) {
        std::cout << "  Magic: 0x" << std::hex << magic 
                  << " (expected 0x" << UCPKG_MAGIC << ")" << std::dec << std::endl;
        return false;
    }
    
    return true;
}

bool TestBundler_HeaderSize() {
    PackageBundler bundler;
    BundleResult result = bundler.Bundle();
    
    // Header should be exactly 48 bytes
    return sizeof(UCPKGHeader) == 48;
}

bool TestBundler_WithUISection() {
    PackageBundler bundler;
    
    std::string ucml = "<div id=\"app\"><button>Click</button></div>";
    bool compiled = bundler.SetUIFromSource(ucml);
    if (!compiled) {
        std::cout << "  Failed to compile UI" << std::endl;
        return false;
    }
    
    BundleResult result = bundler.Bundle();
    
    if (!result.success) {
        for (const auto& err : result.errors) {
            std::cout << "  Error: " << err << std::endl;
        }
        return false;
    }
    
    if (!result.hasUI) {
        std::cout << "  Bundle does not have UI section" << std::endl;
        return false;
    }
    
    if (result.uiSize == 0) {
        std::cout << "  UI section size is 0" << std::endl;
        return false;
    }
    
    return true;
}

bool TestBundler_WithStyleSection() {
    PackageBundler bundler;
    
    std::string css = ".button { display: flex; color: #FF0000; }";
    bool compiled = bundler.SetStyleFromSource(css);
    if (!compiled) {
        std::cout << "  Failed to compile CSS" << std::endl;
        return false;
    }
    
    BundleResult result = bundler.Bundle();
    
    if (!result.success) return false;
    if (!result.hasStyles) return false;
    if (result.styleSize == 0) return false;
    
    return true;
}

bool TestBundler_WithAllSections() {
    PackageBundler bundler;
    
    // UI
    bundler.SetUIFromSource("<div><button>Test</button></div>");
    
    // Styles
    bundler.SetStyleFromSource(".btn { color: blue; }");
    
    // Code (mock bytecode)
    std::vector<uint8_t> code = {0x48, 0x42, 0x43, 0x00, 0x01, 0x02, 0x03};
    bundler.SetCodeSection(code);
    
    // Assets
    std::vector<uint8_t> assetData = {0x89, 0x50, 0x4E, 0x47};
    bundler.GetAssetManager().AddAsset("icon.png", assetData, UCAAssetType::Image);
    
    BundleResult result = bundler.Bundle();
    
    if (!result.success) {
        for (const auto& err : result.errors) {
            std::cout << "  Error: " << err << std::endl;
        }
        return false;
    }
    
    if (!result.hasUI) return false;
    if (!result.hasStyles) return false;
    if (!result.hasCode) return false;
    if (!result.hasAssets) return false;
    
    std::cout << "  Total size: " << result.totalSize << " bytes" << std::endl;
    std::cout << "  UI: " << result.uiSize << " | Styles: " << result.styleSize 
              << " | Code: " << result.codeSize << " | Assets: " << result.assetSize << std::endl;
    
    return true;
}

bool TestBundler_CRC32Verification() {
    PackageBundler bundler;
    
    bundler.SetUIFromSource("<div>Test</div>");
    bundler.SetStyleFromSource(".test { color: red; }");
    
    BundleResult result = bundler.Bundle();
    if (!result.success) return false;
    
    // Read back and verify
    PackageReader reader;
    if (!reader.Open(result.data)) return false;
    
    if (!reader.VerifyCRC()) {
        std::cout << "  CRC verification failed" << std::endl;
        return false;
    }
    
    return true;
}

bool TestBundler_WriteToFile() {
    PackageBundler bundler;
    
    bundler.SetUIFromSource("<div><span>Hello</span></div>");
    bundler.SetStyleFromSource("div { display: flex; }");
    
    std::string outputPath = "/tmp/test_bundle.ucpkg";
    
    BundleResult result;
    bool written = bundler.BundleToFile(outputPath, result);
    
    if (!written) {
        std::cout << "  Failed to write file" << std::endl;
        return false;
    }
    
    // Read back and verify
    std::string error;
    if (!ValidatePackage(outputPath, error)) {
        std::cout << "  Validation failed: " << error << std::endl;
        std::remove(outputPath.c_str());
        return false;
    }
    
    // Cleanup
    std::remove(outputPath.c_str());
    
    return true;
}

bool TestBundler_ExtractSections() {
    PackageBundler bundler;
    
    // Create known content
    std::vector<uint8_t> uiData = {0x55, 0x43, 0x42, 0x31, 0x01, 0x02};
    std::vector<uint8_t> styleData = {0x55, 0x43, 0x53, 0x31, 0x03, 0x04};
    std::vector<uint8_t> codeData = {0x48, 0x42, 0x43, 0x00, 0x05, 0x06};
    
    bundler.SetUISection(uiData);
    bundler.SetStyleSection(styleData);
    bundler.SetCodeSection(codeData);
    
    BundleResult result = bundler.Bundle();
    if (!result.success) return false;
    
    // Read back
    PackageReader reader;
    if (!reader.Open(result.data)) return false;
    
    auto extractedUI = reader.ExtractUISection();
    auto extractedStyle = reader.ExtractStyleSection();
    auto extractedCode = reader.ExtractCodeSection();
    
    if (extractedUI != uiData) {
        std::cout << "  UI section mismatch" << std::endl;
        return false;
    }
    
    if (extractedStyle != styleData) {
        std::cout << "  Style section mismatch" << std::endl;
        return false;
    }
    
    if (extractedCode != codeData) {
        std::cout << "  Code section mismatch" << std::endl;
        return false;
    }
    
    return true;
}

bool TestBundler_Flags() {
    PackageBundler bundler;
    
    BundleConfig config;
    config.includeDebugInfo = true;
    bundler.SetConfig(config);
    
    bundler.SetUIFromSource("<div>Test</div>");
    
    BundleResult result = bundler.Bundle();
    if (!result.success) return false;
    
    PackageReader reader;
    if (!reader.Open(result.data)) return false;
    
    PackageInfo info = reader.GetInfo();
    
    // Check debug flag
    if (!HasFlag(static_cast<UCPKGFlags>(info.flags), UCPKGFlags::Debug)) {
        std::cout << "  Debug flag not set" << std::endl;
        return false;
    }
    
    // Check HasUI flag
    if (!HasFlag(static_cast<UCPKGFlags>(info.flags), UCPKGFlags::HasUI)) {
        std::cout << "  HasUI flag not set" << std::endl;
        return false;
    }
    
    return true;
}

bool TestBundler_CompleteApplication() {
    PackageBundler bundler;
    
    // Simulate a complete application
    std::string ucml = R"(
        <div id="app" class="container">
            <flex class="header">
                <text class="title">Todo App</text>
            </flex>
            <div class="content">
                <input type="text" placeholder="Add todo..." />
                <button onclick="addTodo">Add</button>
                <list id="todoList"></list>
            </div>
        </div>
    )";
    
    std::string css = R"(
        .container {
            display: flex;
            flex-direction: column;
            padding: 16px;
        }
        .header {
            background-color: #3399FF;
            padding: 12px;
            border-radius: 8px;
        }
        .title {
            color: white;
            font-size: 24px;
            font-weight: bold;
        }
        .content {
            padding: 16px;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 8px 16px;
            border-radius: 4px;
        }
        button:hover {
            background-color: #45a049;
        }
    )";
    
    bundler.SetUIFromSource(ucml);
    bundler.SetStyleFromSource(css);
    
    // Mock bytecode
    std::vector<uint8_t> code(100, 0x00);
    bundler.SetCodeSection(code);
    
    // Mock assets
    std::vector<uint8_t> iconData(256, 0x89);
    bundler.GetAssetManager().AddAsset("icon.png", iconData, UCAAssetType::Image);
    
    BundleResult result = bundler.Bundle();
    
    if (!result.success) {
        for (const auto& err : result.errors) {
            std::cout << "  Error: " << err << std::endl;
        }
        return false;
    }
    
    std::cout << "  Complete app bundle:" << std::endl;
    std::cout << "    Total size: " << result.totalSize << " bytes" << std::endl;
    std::cout << "    UI: " << result.uiSize << " bytes" << std::endl;
    std::cout << "    Styles: " << result.styleSize << " bytes" << std::endl;
    std::cout << "    Code: " << result.codeSize << " bytes" << std::endl;
    std::cout << "    Assets: " << result.assetSize << " bytes" << std::endl;
    
    // Verify
    PackageReader reader;
    if (!reader.Open(result.data)) return false;
    if (!reader.VerifyCRC()) return false;
    
    return true;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "UltraWeb Package Bundler Test Suite\n";
    PrintSeparator();
    
    int passed = 0;
    int failed = 0;
    
    // CRC32 Tests
    std::cout << "\n[CRC32 Tests]\n";
    
    if (TestCRC32_EmptyData()) { passed++; PrintTestResult("Empty Data", true); }
    else { failed++; PrintTestResult("Empty Data", false); }
    
    if (TestCRC32_SimpleData()) { passed++; PrintTestResult("Simple Data", true); }
    else { failed++; PrintTestResult("Simple Data", false); }
    
    if (TestCRC32_Incremental()) { passed++; PrintTestResult("Incremental", true); }
    else { failed++; PrintTestResult("Incremental", false); }
    
    // Asset Manager Tests
    std::cout << "\n[Asset Manager Tests]\n";
    
    if (TestAssetManager_AddFromMemory()) { passed++; PrintTestResult("Add From Memory", true); }
    else { failed++; PrintTestResult("Add From Memory", false); }
    
    if (TestAssetManager_DuplicateRejected()) { passed++; PrintTestResult("Duplicate Rejected", true); }
    else { failed++; PrintTestResult("Duplicate Rejected", false); }
    
    if (TestAssetManager_BuildUCA()) { passed++; PrintTestResult("Build UCA", true); }
    else { failed++; PrintTestResult("Build UCA", false); }
    
    // Bundler Tests
    std::cout << "\n[Bundler Tests]\n";
    
    if (TestBundler_EmptyBundle()) { passed++; PrintTestResult("Empty Bundle", true); }
    else { failed++; PrintTestResult("Empty Bundle", false); }
    
    if (TestBundler_MagicNumber()) { passed++; PrintTestResult("Magic Number", true); }
    else { failed++; PrintTestResult("Magic Number", false); }
    
    if (TestBundler_HeaderSize()) { passed++; PrintTestResult("Header Size", true); }
    else { failed++; PrintTestResult("Header Size", false); }
    
    if (TestBundler_WithUISection()) { passed++; PrintTestResult("With UI Section", true); }
    else { failed++; PrintTestResult("With UI Section", false); }
    
    if (TestBundler_WithStyleSection()) { passed++; PrintTestResult("With Style Section", true); }
    else { failed++; PrintTestResult("With Style Section", false); }
    
    if (TestBundler_WithAllSections()) { passed++; PrintTestResult("With All Sections", true); }
    else { failed++; PrintTestResult("With All Sections", false); }
    
    if (TestBundler_CRC32Verification()) { passed++; PrintTestResult("CRC32 Verification", true); }
    else { failed++; PrintTestResult("CRC32 Verification", false); }
    
    if (TestBundler_WriteToFile()) { passed++; PrintTestResult("Write To File", true); }
    else { failed++; PrintTestResult("Write To File", false); }
    
    if (TestBundler_ExtractSections()) { passed++; PrintTestResult("Extract Sections", true); }
    else { failed++; PrintTestResult("Extract Sections", false); }
    
    if (TestBundler_Flags()) { passed++; PrintTestResult("Flags", true); }
    else { failed++; PrintTestResult("Flags", false); }
    
    if (TestBundler_CompleteApplication()) { passed++; PrintTestResult("Complete Application", true); }
    else { failed++; PrintTestResult("Complete Application", false); }
    
    // Summary
    std::cout << "\n";
    PrintSeparator();
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    PrintSeparator();
    std::cout << "\n";
    
    return failed > 0 ? 1 : 0;
}
