// UltraWeb/include/UltraWebBundler.h
// UltraWeb Package Bundler - Combines all assets into .ucpkg
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework
#pragma once

#include "UltraWebFormats.h"
#include "UltraWebCSSCompiler.h"
#include "UltraWebUICompiler.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <memory>
#include <functional>

namespace UltraWeb {

// ============================================================================
// CRC32 UTILITY
// ============================================================================

class CRC32 {
public:
    CRC32();
    
    // Calculate CRC32 of data
    static uint32_t Calculate(const uint8_t* data, size_t length);
    static uint32_t Calculate(const std::vector<uint8_t>& data);
    
    // Incremental calculation
    void Update(const uint8_t* data, size_t length);
    void Update(const std::vector<uint8_t>& data);
    uint32_t Finalize();
    void Reset();
    
private:
    uint32_t crc;
    static const uint32_t table[256];
    static bool tableInitialized;
    static void InitializeTable();
};

// ============================================================================
// ASSET MANAGER
// ============================================================================

struct Asset {
    std::string name;           // Asset name/path
    std::string sourcePath;     // Source file path
    UCAAssetType type;          // Asset type
    std::vector<uint8_t> data;  // Asset data
    uint32_t nameHash;          // Pre-computed name hash
    
    Asset() : type(UCAAssetType::Unknown), nameHash(0) {}
};

class AssetManager {
public:
    AssetManager();
    
    // Add asset from file
    bool AddAsset(const std::string& name, const std::string& filePath);
    
    // Add asset from memory
    bool AddAsset(const std::string& name, const std::vector<uint8_t>& data, UCAAssetType type);
    
    // Get asset by name
    const Asset* GetAsset(const std::string& name) const;
    
    // Get all assets
    const std::vector<Asset>& GetAssets() const { return assets; }
    
    // Get asset count
    size_t GetCount() const { return assets.size(); }
    
    // Build UCA binary
    std::vector<uint8_t> BuildUCA() const;
    
    // Clear all assets
    void Clear();
    
private:
    std::vector<Asset> assets;
    std::unordered_map<std::string, size_t> nameToIndex;
    
    // Detect asset type from file extension
    UCAAssetType DetectAssetType(const std::string& filePath) const;
    
    // Read file to bytes
    bool ReadFile(const std::string& filePath, std::vector<uint8_t>& data) const;
};

// ============================================================================
// BUNDLE CONFIGURATION
// ============================================================================

struct BundleConfig {
    bool enableCompression;     // Enable LZ4 compression (Phase 2)
    bool enableEncryption;      // Enable encryption (Phase 2)
    bool includeDebugInfo;      // Include debug information
    std::string entryPoint;     // Entry point file
    
    BundleConfig()
        : enableCompression(false)
        , enableEncryption(false)
        , includeDebugInfo(false)
        , entryPoint("main") {}
};

// ============================================================================
// BUNDLE RESULT
// ============================================================================

struct BundleResult {
    bool success;
    std::vector<uint8_t> data;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    // Statistics
    size_t uiSize;
    size_t styleSize;
    size_t codeSize;
    size_t assetSize;
    size_t totalSize;
    size_t uncompressedSize;
    float compressionRatio;
    
    // Section info
    bool hasUI;
    bool hasStyles;
    bool hasCode;
    bool hasAssets;
    
    BundleResult()
        : success(false)
        , uiSize(0)
        , styleSize(0)
        , codeSize(0)
        , assetSize(0)
        , totalSize(0)
        , uncompressedSize(0)
        , compressionRatio(1.0f)
        , hasUI(false)
        , hasStyles(false)
        , hasCode(false)
        , hasAssets(false) {}
    
    void AddError(const std::string& error) {
        errors.push_back(error);
        success = false;
    }
    
    void AddWarning(const std::string& warning) {
        warnings.push_back(warning);
    }
};

// ============================================================================
// PACKAGE BUNDLER
// ============================================================================

class PackageBundler {
public:
    PackageBundler();
    
    // Set configuration
    void SetConfig(const BundleConfig& config) { this->config = config; }
    const BundleConfig& GetConfig() const { return config; }
    
    // Set sections (binary data)
    void SetUISection(const std::vector<uint8_t>& data);
    void SetStyleSection(const std::vector<uint8_t>& data);
    void SetCodeSection(const std::vector<uint8_t>& data);
    void SetAssetSection(const std::vector<uint8_t>& data);
    
    // Set sections from source (will compile)
    bool SetUIFromSource(const std::string& ucml);
    bool SetStyleFromSource(const std::string& css);
    bool SetCodeFromFile(const std::string& filePath);  // .hbc file
    
    // Asset management
    AssetManager& GetAssetManager() { return assetManager; }
    
    // Bundle everything
    BundleResult Bundle();
    
    // Bundle to file
    bool BundleToFile(const std::string& outputPath);
    bool BundleToFile(const std::string& outputPath, BundleResult& result);
    
    // Clear all sections
    void Clear();
    
private:
    BundleConfig config;
    std::vector<uint8_t> uiSection;
    std::vector<uint8_t> styleSection;
    std::vector<uint8_t> codeSection;
    std::vector<uint8_t> assetSection;
    AssetManager assetManager;
    
    // Build final package
    std::vector<uint8_t> BuildPackage(BundleResult& result);
    
    // Calculate flags; `compressed` reflects the actual compression outcome,
    // not just the config request (compression may be skipped when the
    // backend is missing or the content is incompressible)
    uint16_t CalculateFlags(bool compressed) const;
};

// ============================================================================
// PACKAGE READER (for verification)
// ============================================================================

struct PackageInfo {
    bool valid;
    uint16_t version;
    uint16_t flags;
    uint32_t totalSize;
    uint32_t crc32;
    
    bool hasUI;
    bool hasStyles;
    bool hasCode;
    bool hasAssets;
    
    uint32_t uiOffset;
    uint32_t uiSize;
    uint32_t styleOffset;
    uint32_t styleSize;
    uint32_t codeOffset;
    uint32_t codeSize;
    uint32_t assetOffset;
    uint32_t assetSize;
    
    std::string error;
    
    PackageInfo() 
        : valid(false)
        , version(0)
        , flags(0)
        , totalSize(0)
        , crc32(0)
        , hasUI(false)
        , hasStyles(false)
        , hasCode(false)
        , hasAssets(false)
        , uiOffset(0)
        , uiSize(0)
        , styleOffset(0)
        , styleSize(0)
        , codeOffset(0)
        , codeSize(0)
        , assetOffset(0)
        , assetSize(0) {}
};

class PackageReader {
public:
    PackageReader();
    
    // Open package from file
    bool Open(const std::string& filePath);
    
    // Open package from memory
    bool Open(const std::vector<uint8_t>& data);
    
    // Get package info
    PackageInfo GetInfo() const;
    
    // Verify CRC32
    bool VerifyCRC() const;
    
    // Extract sections
    std::vector<uint8_t> ExtractUISection() const;
    std::vector<uint8_t> ExtractStyleSection() const;
    std::vector<uint8_t> ExtractCodeSection() const;
    std::vector<uint8_t> ExtractAssetSection() const;
    
    // Dump package info
    std::string DumpInfo() const;
    
private:
    std::vector<uint8_t> data;
    UCPKGHeader header;
    bool isOpen;
    
    bool ParseHeader();
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get UCPKG file info
PackageInfo GetPackageInfo(const std::string& filePath);

// Validate UCPKG file
bool ValidatePackage(const std::string& filePath, std::string& error);

// Dump package contents
std::string DumpPackage(const std::string& filePath);

} // namespace UltraWeb
