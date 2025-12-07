// UltraWeb/core/UltraWebBundler.cpp
// UltraWeb Package Bundler Implementation
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "../include/UltraWebBundler.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace UltraWeb {

// ============================================================================
// CRC32 IMPLEMENTATION
// ============================================================================

const uint32_t CRC32::table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD706B3, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

bool CRC32::tableInitialized = true;

CRC32::CRC32() : crc(0xFFFFFFFF) {
}

uint32_t CRC32::Calculate(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

uint32_t CRC32::Calculate(const std::vector<uint8_t>& data) {
    return Calculate(data.data(), data.size());
}

void CRC32::Update(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
}

void CRC32::Update(const std::vector<uint8_t>& data) {
    Update(data.data(), data.size());
}

uint32_t CRC32::Finalize() {
    return crc ^ 0xFFFFFFFF;
}

void CRC32::Reset() {
    crc = 0xFFFFFFFF;
}

// ============================================================================
// ASSET MANAGER IMPLEMENTATION
// ============================================================================

AssetManager::AssetManager() {
}

bool AssetManager::AddAsset(const std::string& name, const std::string& filePath) {
    // Check for duplicate
    if (nameToIndex.count(name) > 0) {
        return false;
    }
    
    Asset asset;
    asset.name = name;
    asset.sourcePath = filePath;
    asset.type = DetectAssetType(filePath);
    asset.nameHash = HashSelector(name);  // Reuse hash function from formats
    
    if (!ReadFile(filePath, asset.data)) {
        return false;
    }
    
    nameToIndex[name] = assets.size();
    assets.push_back(std::move(asset));
    
    return true;
}

bool AssetManager::AddAsset(const std::string& name, const std::vector<uint8_t>& data, UCAAssetType type) {
    if (nameToIndex.count(name) > 0) {
        return false;
    }
    
    Asset asset;
    asset.name = name;
    asset.type = type;
    asset.data = data;
    asset.nameHash = HashSelector(name);
    
    nameToIndex[name] = assets.size();
    assets.push_back(std::move(asset));
    
    return true;
}

const Asset* AssetManager::GetAsset(const std::string& name) const {
    auto it = nameToIndex.find(name);
    if (it != nameToIndex.end()) {
        return &assets[it->second];
    }
    return nullptr;
}

std::vector<uint8_t> AssetManager::BuildUCA() const {
    BinaryWriter writer;
    
    // Calculate data section size
    size_t dataOffset = sizeof(UCAHeader) + assets.size() * sizeof(UCAAssetEntry);
    
    // Write header
    UCAHeader header;
    header.magic = UCA_MAGIC;
    header.version = UCA_VERSION;
    header.assetCount = static_cast<uint16_t>(assets.size());
    header.flags = 0;
    
    writer.WriteUInt32(header.magic);
    writer.WriteUInt16(header.version);
    writer.WriteUInt16(header.assetCount);
    writer.WriteUInt32(0);  // Total size - will patch later
    writer.WriteUInt32(header.flags);
    
    // Write asset index
    size_t currentOffset = dataOffset;
    for (const auto& asset : assets) {
        writer.WriteUInt32(asset.nameHash);
        writer.WriteUInt8(static_cast<uint8_t>(asset.type));
        writer.WriteUInt8(static_cast<uint8_t>(UCACompressionType::None));
        writer.WriteUInt32(static_cast<uint32_t>(currentOffset));
        writer.WriteUInt32(static_cast<uint32_t>(asset.data.size()));  // Compressed = original
        writer.WriteUInt32(static_cast<uint32_t>(asset.data.size()));  // Original size
        
        currentOffset += asset.data.size();
    }
    
    // Write asset data
    for (const auto& asset : assets) {
        writer.WriteBytes(asset.data.data(), asset.data.size());
    }
    
    // Patch total size
    auto data = writer.TakeData();
    uint32_t totalSize = static_cast<uint32_t>(data.size());
    data[8] = totalSize & 0xFF;
    data[9] = (totalSize >> 8) & 0xFF;
    data[10] = (totalSize >> 16) & 0xFF;
    data[11] = (totalSize >> 24) & 0xFF;
    
    return data;
}

void AssetManager::Clear() {
    assets.clear();
    nameToIndex.clear();
}

UCAAssetType AssetManager::DetectAssetType(const std::string& filePath) const {
    // Get extension
    size_t dotPos = filePath.rfind('.');
    if (dotPos == std::string::npos) {
        return UCAAssetType::Binary;
    }
    
    std::string ext = filePath.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Image formats
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "gif" || 
        ext == "webp" || ext == "avif" || ext == "bmp" || ext == "ico") {
        return UCAAssetType::Image;
    }
    
    // Font formats
    if (ext == "ttf" || ext == "otf" || ext == "woff" || ext == "woff2") {
        return UCAAssetType::Font;
    }
    
    // Audio formats
    if (ext == "mp3" || ext == "ogg" || ext == "wav" || ext == "aac" || ext == "flac") {
        return UCAAssetType::Audio;
    }
    
    // Video formats
    if (ext == "mp4" || ext == "webm" || ext == "ogv" || ext == "avi") {
        return UCAAssetType::Video;
    }
    
    // Data formats
    if (ext == "json" || ext == "xml" || ext == "csv" || ext == "txt") {
        return UCAAssetType::Data;
    }
    
    return UCAAssetType::Binary;
}

bool AssetManager::ReadFile(const std::string& filePath, std::vector<uint8_t>& data) const {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    return true;
}

// ============================================================================
// PACKAGE BUNDLER IMPLEMENTATION
// ============================================================================

PackageBundler::PackageBundler() {
}

void PackageBundler::SetUISection(const std::vector<uint8_t>& data) {
    uiSection = data;
}

void PackageBundler::SetStyleSection(const std::vector<uint8_t>& data) {
    styleSection = data;
}

void PackageBundler::SetCodeSection(const std::vector<uint8_t>& data) {
    codeSection = data;
}

void PackageBundler::SetAssetSection(const std::vector<uint8_t>& data) {
    assetSection = data;
}

bool PackageBundler::SetUIFromSource(const std::string& ucml) {
    UICompiler compiler;
    UICompilationResult result = compiler.Compile(ucml);
    
    if (!result.success) {
        return false;
    }
    
    uiSection = std::move(result.data);
    return true;
}

bool PackageBundler::SetStyleFromSource(const std::string& css) {
    CSSCompiler compiler;
    CompilationResult result = compiler.Compile(css);
    
    if (!result.success) {
        return false;
    }
    
    styleSection = std::move(result.data);
    return true;
}

bool PackageBundler::SetCodeFromFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    codeSection.resize(size);
    file.read(reinterpret_cast<char*>(codeSection.data()), size);
    
    return true;
}

BundleResult PackageBundler::Bundle() {
    BundleResult result;
    
    // Build asset section if assets exist
    if (assetManager.GetCount() > 0 && assetSection.empty()) {
        assetSection = assetManager.BuildUCA();
    }
    
    // Build package
    result.data = BuildPackage(result);
    
    if (result.data.empty()) {
        result.AddError("Failed to build package");
        return result;
    }
    
    result.success = true;
    return result;
}

bool PackageBundler::BundleToFile(const std::string& outputPath) {
    BundleResult result;
    return BundleToFile(outputPath, result);
}

bool PackageBundler::BundleToFile(const std::string& outputPath, BundleResult& result) {
    result = Bundle();
    
    if (!result.success) {
        return false;
    }
    
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        result.AddError("Cannot open output file: " + outputPath);
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(result.data.data()), result.data.size());
    file.close();
    
    return true;
}

void PackageBundler::Clear() {
    uiSection.clear();
    styleSection.clear();
    codeSection.clear();
    assetSection.clear();
    assetManager.Clear();
}

std::vector<uint8_t> PackageBundler::BuildPackage(BundleResult& result) {
    BinaryWriter writer;
    
    // Calculate section sizes
    result.uiSize = uiSection.size();
    result.styleSize = styleSection.size();
    result.codeSize = codeSection.size();
    result.assetSize = assetSection.size();
    
    result.hasUI = !uiSection.empty();
    result.hasStyles = !styleSection.empty();
    result.hasCode = !codeSection.empty();
    result.hasAssets = !assetSection.empty();
    
    // Calculate offsets
    size_t headerSize = sizeof(UCPKGHeader);
    size_t uiOffset = headerSize;
    size_t styleOffset = uiOffset + result.uiSize;
    size_t codeOffset = styleOffset + result.styleSize;
    size_t assetOffset = codeOffset + result.codeSize;
    size_t totalSize = assetOffset + result.assetSize;
    
    result.totalSize = totalSize;
    result.uncompressedSize = totalSize;
    result.compressionRatio = 1.0f;
    
    // Build content for CRC calculation
    std::vector<uint8_t> content;
    content.reserve(result.uiSize + result.styleSize + result.codeSize + result.assetSize);
    content.insert(content.end(), uiSection.begin(), uiSection.end());
    content.insert(content.end(), styleSection.begin(), styleSection.end());
    content.insert(content.end(), codeSection.begin(), codeSection.end());
    content.insert(content.end(), assetSection.begin(), assetSection.end());
    
    uint32_t crc = CRC32::Calculate(content);
    
    // Write header
    UCPKGHeader header;
    header.magic = UCPKG_MAGIC;
    header.version = UCPKG_VERSION;
    header.flags = CalculateFlags();
    header.totalSize = static_cast<uint32_t>(totalSize);
    header.crc32 = crc;
    header.uiSectionOffset = result.hasUI ? static_cast<uint32_t>(uiOffset) : 0;
    header.uiSectionSize = static_cast<uint32_t>(result.uiSize);
    header.styleSectionOffset = result.hasStyles ? static_cast<uint32_t>(styleOffset) : 0;
    header.styleSectionSize = static_cast<uint32_t>(result.styleSize);
    header.codeSectionOffset = result.hasCode ? static_cast<uint32_t>(codeOffset) : 0;
    header.codeSectionSize = static_cast<uint32_t>(result.codeSize);
    header.assetSectionOffset = result.hasAssets ? static_cast<uint32_t>(assetOffset) : 0;
    header.assetSectionSize = static_cast<uint32_t>(result.assetSize);
    
    writer.WriteUInt32(header.magic);
    writer.WriteUInt16(header.version);
    writer.WriteUInt16(header.flags);
    writer.WriteUInt32(header.totalSize);
    writer.WriteUInt32(header.crc32);
    writer.WriteUInt32(header.uiSectionOffset);
    writer.WriteUInt32(header.uiSectionSize);
    writer.WriteUInt32(header.styleSectionOffset);
    writer.WriteUInt32(header.styleSectionSize);
    writer.WriteUInt32(header.codeSectionOffset);
    writer.WriteUInt32(header.codeSectionSize);
    writer.WriteUInt32(header.assetSectionOffset);
    writer.WriteUInt32(header.assetSectionSize);
    
    // Write sections
    if (!uiSection.empty()) {
        writer.WriteBytes(uiSection.data(), uiSection.size());
    }
    if (!styleSection.empty()) {
        writer.WriteBytes(styleSection.data(), styleSection.size());
    }
    if (!codeSection.empty()) {
        writer.WriteBytes(codeSection.data(), codeSection.size());
    }
    if (!assetSection.empty()) {
        writer.WriteBytes(assetSection.data(), assetSection.size());
    }
    
    return writer.TakeData();
}

uint16_t PackageBundler::CalculateFlags() const {
    uint16_t flags = 0;
    
    if (config.enableCompression) {
        flags |= static_cast<uint16_t>(UCPKGFlags::Compressed);
    }
    if (config.enableEncryption) {
        flags |= static_cast<uint16_t>(UCPKGFlags::Encrypted);
    }
    if (config.includeDebugInfo) {
        flags |= static_cast<uint16_t>(UCPKGFlags::Debug);
    }
    if (!uiSection.empty()) {
        flags |= static_cast<uint16_t>(UCPKGFlags::HasUI);
    }
    if (!styleSection.empty()) {
        flags |= static_cast<uint16_t>(UCPKGFlags::HasStyles);
    }
    if (!codeSection.empty()) {
        flags |= static_cast<uint16_t>(UCPKGFlags::HasCode);
    }
    if (!assetSection.empty()) {
        flags |= static_cast<uint16_t>(UCPKGFlags::HasAssets);
    }
    
    return flags;
}

// ============================================================================
// PACKAGE READER IMPLEMENTATION
// ============================================================================

PackageReader::PackageReader() : isOpen(false) {
}

bool PackageReader::Open(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();
    
    return ParseHeader();
}

bool PackageReader::Open(const std::vector<uint8_t>& inputData) {
    data = inputData;
    return ParseHeader();
}

bool PackageReader::ParseHeader() {
    if (data.size() < sizeof(UCPKGHeader)) {
        isOpen = false;
        return false;
    }
    
    std::memcpy(&header, data.data(), sizeof(UCPKGHeader));
    
    if (header.magic != UCPKG_MAGIC) {
        isOpen = false;
        return false;
    }
    
    isOpen = true;
    return true;
}

PackageInfo PackageReader::GetInfo() const {
    PackageInfo info;
    
    if (!isOpen) {
        info.error = "Package not open";
        return info;
    }
    
    info.valid = true;
    info.version = header.version;
    info.flags = header.flags;
    info.totalSize = header.totalSize;
    info.crc32 = header.crc32;
    
    info.hasUI = HasFlag(static_cast<UCPKGFlags>(header.flags), UCPKGFlags::HasUI);
    info.hasStyles = HasFlag(static_cast<UCPKGFlags>(header.flags), UCPKGFlags::HasStyles);
    info.hasCode = HasFlag(static_cast<UCPKGFlags>(header.flags), UCPKGFlags::HasCode);
    info.hasAssets = HasFlag(static_cast<UCPKGFlags>(header.flags), UCPKGFlags::HasAssets);
    
    info.uiOffset = header.uiSectionOffset;
    info.uiSize = header.uiSectionSize;
    info.styleOffset = header.styleSectionOffset;
    info.styleSize = header.styleSectionSize;
    info.codeOffset = header.codeSectionOffset;
    info.codeSize = header.codeSectionSize;
    info.assetOffset = header.assetSectionOffset;
    info.assetSize = header.assetSectionSize;
    
    return info;
}

bool PackageReader::VerifyCRC() const {
    if (!isOpen) return false;
    
    // Calculate CRC of content (everything after header)
    if (data.size() <= sizeof(UCPKGHeader)) {
        return header.crc32 == 0;
    }
    
    uint32_t calculated = CRC32::Calculate(data.data() + sizeof(UCPKGHeader), 
                                            data.size() - sizeof(UCPKGHeader));
    return calculated == header.crc32;
}

std::vector<uint8_t> PackageReader::ExtractUISection() const {
    if (!isOpen || header.uiSectionSize == 0) {
        return {};
    }
    
    if (header.uiSectionOffset + header.uiSectionSize > data.size()) {
        return {};
    }
    
    return std::vector<uint8_t>(
        data.begin() + header.uiSectionOffset,
        data.begin() + header.uiSectionOffset + header.uiSectionSize
    );
}

std::vector<uint8_t> PackageReader::ExtractStyleSection() const {
    if (!isOpen || header.styleSectionSize == 0) {
        return {};
    }
    
    if (header.styleSectionOffset + header.styleSectionSize > data.size()) {
        return {};
    }
    
    return std::vector<uint8_t>(
        data.begin() + header.styleSectionOffset,
        data.begin() + header.styleSectionOffset + header.styleSectionSize
    );
}

std::vector<uint8_t> PackageReader::ExtractCodeSection() const {
    if (!isOpen || header.codeSectionSize == 0) {
        return {};
    }
    
    if (header.codeSectionOffset + header.codeSectionSize > data.size()) {
        return {};
    }
    
    return std::vector<uint8_t>(
        data.begin() + header.codeSectionOffset,
        data.begin() + header.codeSectionOffset + header.codeSectionSize
    );
}

std::vector<uint8_t> PackageReader::ExtractAssetSection() const {
    if (!isOpen || header.assetSectionSize == 0) {
        return {};
    }
    
    if (header.assetSectionOffset + header.assetSectionSize > data.size()) {
        return {};
    }
    
    return std::vector<uint8_t>(
        data.begin() + header.assetSectionOffset,
        data.begin() + header.assetSectionOffset + header.assetSectionSize
    );
}

std::string PackageReader::DumpInfo() const {
    std::ostringstream oss;
    
    if (!isOpen) {
        oss << "Package not open\n";
        return oss.str();
    }
    
    PackageInfo info = GetInfo();
    
    oss << "=== UCPKG Package Info ===\n";
    oss << "Version: 0x" << std::hex << std::setw(4) << std::setfill('0') << info.version << std::dec << "\n";
    oss << "Flags: 0x" << std::hex << std::setw(4) << std::setfill('0') << info.flags << std::dec << "\n";
    oss << "Total Size: " << info.totalSize << " bytes\n";
    oss << "CRC32: 0x" << std::hex << std::setw(8) << std::setfill('0') << info.crc32 << std::dec << "\n";
    oss << "CRC Valid: " << (VerifyCRC() ? "Yes" : "No") << "\n";
    oss << "\n";
    
    oss << "[Sections]\n";
    if (info.hasUI) {
        oss << "  UI:     offset=" << info.uiOffset << ", size=" << info.uiSize << " bytes\n";
    }
    if (info.hasStyles) {
        oss << "  Styles: offset=" << info.styleOffset << ", size=" << info.styleSize << " bytes\n";
    }
    if (info.hasCode) {
        oss << "  Code:   offset=" << info.codeOffset << ", size=" << info.codeSize << " bytes\n";
    }
    if (info.hasAssets) {
        oss << "  Assets: offset=" << info.assetOffset << ", size=" << info.assetSize << " bytes\n";
    }
    
    oss << "\n[Flags]\n";
    oss << "  Compressed: " << (HasFlag(static_cast<UCPKGFlags>(info.flags), UCPKGFlags::Compressed) ? "Yes" : "No") << "\n";
    oss << "  Encrypted:  " << (HasFlag(static_cast<UCPKGFlags>(info.flags), UCPKGFlags::Encrypted) ? "Yes" : "No") << "\n";
    oss << "  Debug:      " << (HasFlag(static_cast<UCPKGFlags>(info.flags), UCPKGFlags::Debug) ? "Yes" : "No") << "\n";
    
    oss << "==========================\n";
    
    return oss.str();
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

PackageInfo GetPackageInfo(const std::string& filePath) {
    PackageReader reader;
    if (!reader.Open(filePath)) {
        PackageInfo info;
        info.error = "Cannot open file";
        return info;
    }
    return reader.GetInfo();
}

bool ValidatePackage(const std::string& filePath, std::string& error) {
    PackageReader reader;
    if (!reader.Open(filePath)) {
        error = "Cannot open file";
        return false;
    }
    
    PackageInfo info = reader.GetInfo();
    if (!info.valid) {
        error = info.error;
        return false;
    }
    
    if (!reader.VerifyCRC()) {
        error = "CRC32 verification failed";
        return false;
    }
    
    return true;
}

std::string DumpPackage(const std::string& filePath) {
    PackageReader reader;
    if (!reader.Open(filePath)) {
        return "Cannot open file: " + filePath + "\n";
    }
    return reader.DumpInfo();
}

} // namespace UltraWeb
