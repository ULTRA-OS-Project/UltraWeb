// UltraWeb/include/UltraWebUICompiler.h
// UCML to UCB Binary Compiler - Converts parsed UI AST to binary format
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework
#pragma once

#include "UltraWebFormats.h"
#include "UltraWebUIParser.h"
#include "UltraWebCSSCompiler.h"  // For BinaryWriter and StringTable
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

namespace UltraWeb {

// ============================================================================
// UI COMPILATION RESULT
// ============================================================================

struct UICompilationResult {
    bool success;
    std::vector<uint8_t> data;      // Binary output
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    // Statistics
    size_t elementCount;
    size_t attributeCount;
    size_t stringCount;
    size_t originalSize;            // Original UCML size
    size_t compiledSize;            // Compiled binary size
    float compressionRatio;
    
    UICompilationResult()
        : success(false)
        , elementCount(0)
        , attributeCount(0)
        , stringCount(0)
        , originalSize(0)
        , compiledSize(0)
        , compressionRatio(0.0f) {}
    
    void AddError(const std::string& error) {
        errors.push_back(error);
        success = false;
    }
    
    void AddWarning(const std::string& warning) {
        warnings.push_back(warning);
    }
};

// ============================================================================
// STYLE CLASS TABLE
// ============================================================================

class StyleClassTable {
public:
    StyleClassTable();
    
    // Add style class and get its ID
    uint16_t AddClass(const std::string& className);
    
    // Get class ID (returns 0xFFFF if not found)
    uint16_t FindClass(const std::string& className) const;
    
    // Get all classes
    const std::vector<std::string>& GetClasses() const { return classes; }
    
    // Get count
    size_t GetCount() const { return classes.size(); }
    
    // Write to binary
    void WriteTo(BinaryWriter& writer) const;
    
private:
    std::vector<std::string> classes;
    std::unordered_map<std::string, uint16_t> classToId;
};

// ============================================================================
// EVENT HANDLER TABLE
// ============================================================================

struct EventHandlerEntry {
    uint16_t elementId;
    UCBPropertyId eventType;
    uint16_t handlerNameId;  // String table reference
};

class EventHandlerTable {
public:
    EventHandlerTable();
    
    // Add event handler
    void AddHandler(uint16_t elementId, UCBPropertyId eventType, uint16_t handlerNameId);
    
    // Get handlers for element
    std::vector<EventHandlerEntry> GetHandlers(uint16_t elementId) const;
    
    // Get all handlers
    const std::vector<EventHandlerEntry>& GetAllHandlers() const { return handlers; }
    
    // Get count
    size_t GetCount() const { return handlers.size(); }
    
    // Write to binary
    void WriteTo(BinaryWriter& writer) const;
    
private:
    std::vector<EventHandlerEntry> handlers;
};

// ============================================================================
// UI COMPILER
// ============================================================================

class UICompiler {
public:
    UICompiler();
    
    // Compile UCML string to UCB binary
    UICompilationResult Compile(const std::string& ucml);
    
    // Compile parsed document to UCB binary
    UICompilationResult Compile(const UCMLDocument& document);
    
    // Compile and write to file
    bool CompileToFile(const std::string& ucml, const std::string& outputPath);
    bool CompileToFile(const UCMLDocument& document, const std::string& outputPath);
    
    // Configuration
    void SetOptimizeOutput(bool optimize) { optimizeOutput = optimize; }
    
private:
    bool optimizeOutput;
    StringTable stringTable;
    StyleClassTable styleClassTable;
    EventHandlerTable eventHandlerTable;
    BinaryWriter writer;
    
    // Compilation state
    size_t elementCount;
    size_t attributeCount;
    
    // Reset for new compilation
    void Reset();
    
    // Compile element tree recursively
    void CompileElement(const std::shared_ptr<UCMLElement>& element);
    
    // Compile element properties
    void CompileProperty(const UCMLAttribute& attr, uint16_t elementId);
    
    // Build final binary
    std::vector<uint8_t> BuildBinary(const UCMLDocument& document);
    
    // Helper: Serialize element to binary
    std::vector<uint8_t> SerializeElement(const std::shared_ptr<UCMLElement>& element);
    
    // Helper: Serialize property value
    void SerializePropertyValue(BinaryWriter& writer, const UCMLAttribute& attr);
    
    // Helper: Get value type for attribute
    UCBValueType GetValueType(const UCMLAttribute& attr);
    
    // Helper: Calculate property size
    uint8_t CalculatePropertySize(const UCMLAttribute& attr);
    
    // Pre-process: Collect all strings and classes
    void PreProcessElement(const std::shared_ptr<UCMLElement>& element);
};

// ============================================================================
// UCB FILE UTILITIES
// ============================================================================

// UCB File Info
struct UCBFileInfo {
    bool valid;
    uint16_t version;
    uint16_t flags;
    uint32_t elementCount;
    size_t fileSize;
    std::string error;
};

// Get UCB file info
UCBFileInfo GetUCBFileInfo(const std::string& filePath);

// Validate UCB file
bool ValidateUCBFile(const std::string& filePath, std::string& error);

// Dump UCB file to human-readable format
std::string DumpUCBFile(const std::string& filePath);

} // namespace UltraWeb
