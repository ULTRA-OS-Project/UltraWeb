// UltraWeb/core/UltraWebUICompiler.cpp
// UCML to UCB Binary Compiler Implementation
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "../include/UltraWebUICompiler.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <stack>

namespace UltraWeb {

// ============================================================================
// STYLE CLASS TABLE IMPLEMENTATION
// ============================================================================

StyleClassTable::StyleClassTable() {
}

uint16_t StyleClassTable::AddClass(const std::string& className) {
    auto it = classToId.find(className);
    if (it != classToId.end()) {
        return it->second;
    }
    
    uint16_t id = static_cast<uint16_t>(classes.size());
    classes.push_back(className);
    classToId[className] = id;
    return id;
}

uint16_t StyleClassTable::FindClass(const std::string& className) const {
    auto it = classToId.find(className);
    if (it != classToId.end()) {
        return it->second;
    }
    return 0xFFFF;
}

void StyleClassTable::WriteTo(BinaryWriter& writer) const {
    writer.WriteUInt16(static_cast<uint16_t>(classes.size()));
    for (const auto& className : classes) {
        writer.WriteString(className);
    }
}

// ============================================================================
// EVENT HANDLER TABLE IMPLEMENTATION
// ============================================================================

EventHandlerTable::EventHandlerTable() {
}

void EventHandlerTable::AddHandler(uint16_t elementId, UCBPropertyId eventType, uint16_t handlerNameId) {
    EventHandlerEntry entry;
    entry.elementId = elementId;
    entry.eventType = eventType;
    entry.handlerNameId = handlerNameId;
    handlers.push_back(entry);
}

std::vector<EventHandlerEntry> EventHandlerTable::GetHandlers(uint16_t elementId) const {
    std::vector<EventHandlerEntry> result;
    for (const auto& handler : handlers) {
        if (handler.elementId == elementId) {
            result.push_back(handler);
        }
    }
    return result;
}

void EventHandlerTable::WriteTo(BinaryWriter& writer) const {
    writer.WriteUInt16(static_cast<uint16_t>(handlers.size()));
    for (const auto& handler : handlers) {
        writer.WriteUInt16(handler.elementId);
        writer.WriteUInt8(static_cast<uint8_t>(handler.eventType));
        writer.WriteUInt16(handler.handlerNameId);
    }
}

// ============================================================================
// UI COMPILER IMPLEMENTATION
// ============================================================================

UICompiler::UICompiler()
    : optimizeOutput(true)
    , elementCount(0)
    , attributeCount(0)
{
}

void UICompiler::Reset() {
    writer.Clear();
    stringTable = StringTable();
    styleClassTable = StyleClassTable();
    eventHandlerTable = EventHandlerTable();
    elementCount = 0;
    attributeCount = 0;
}

UICompilationResult UICompiler::Compile(const std::string& ucml) {
    UCMLParser parser;
    UCMLDocument document = parser.Parse(ucml);
    
    UICompilationResult result = Compile(document);
    result.originalSize = ucml.length();
    
    // Recalculate compression ratio
    if (result.originalSize > 0 && result.compiledSize > 0) {
        result.compressionRatio = static_cast<float>(result.originalSize) / 
                                  static_cast<float>(result.compiledSize);
    }
    
    // Add parse errors
    for (const auto& error : document.errors) {
        result.AddError("Parse error: " + error);
    }
    
    return result;
}

UICompilationResult UICompiler::Compile(const UCMLDocument& document) {
    UICompilationResult result;
    Reset();
    
    if (!document.root) {
        result.AddError("No root element in document");
        return result;
    }
    
    try {
        // Pre-process to collect all strings and classes
        PreProcessElement(document.root);
        
        // Build binary
        result.data = BuildBinary(document);
        
        result.success = true;
        result.elementCount = elementCount;
        result.attributeCount = attributeCount;
        result.stringCount = stringTable.GetCount();
        result.compiledSize = result.data.size();
        
    } catch (const std::exception& e) {
        result.AddError(std::string("Compilation error: ") + e.what());
    }
    
    return result;
}

bool UICompiler::CompileToFile(const std::string& ucml, const std::string& outputPath) {
    UICompilationResult result = Compile(ucml);
    
    if (!result.success) {
        return false;
    }
    
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(result.data.data()), result.data.size());
    file.close();
    
    return true;
}

bool UICompiler::CompileToFile(const UCMLDocument& document, const std::string& outputPath) {
    UICompilationResult result = Compile(document);
    
    if (!result.success) {
        return false;
    }
    
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(result.data.data()), result.data.size());
    file.close();
    
    return true;
}

void UICompiler::PreProcessElement(const std::shared_ptr<UCMLElement>& element) {
    if (!element) return;
    
    elementCount++;
    
    // Add text content to string table
    if (!element->textContent.empty()) {
        stringTable.AddString(element->textContent);
    }
    
    // Process attributes
    for (const auto& attr : element->attributes) {
        attributeCount++;
        
        // Add class names to style class table
        if (attr.name == "class") {
            const auto* classStr = attr.AsString();
            if (classStr) {
                std::istringstream iss(*classStr);
                std::string className;
                while (iss >> className) {
                    styleClassTable.AddClass(className);
                }
            }
        }
        
        // Add string values to string table
        if (attr.IsString()) {
            const auto* str = attr.AsString();
            if (str) {
                stringTable.AddString(*str);
            }
        }
    }
    
    // Process children
    for (const auto& child : element->children) {
        PreProcessElement(child);
    }
}

std::vector<uint8_t> UICompiler::BuildBinary(const UCMLDocument& document) {
    BinaryWriter mainWriter;
    
    // Collect all elements in breadth-first order
    std::vector<std::shared_ptr<UCMLElement>> elements;
    std::stack<std::shared_ptr<UCMLElement>> stack;
    stack.push(document.root);
    
    while (!stack.empty()) {
        auto elem = stack.top();
        stack.pop();
        elements.push_back(elem);
        
        // Add children in reverse order so they come out in correct order
        for (auto it = elem->children.rbegin(); it != elem->children.rend(); ++it) {
            stack.push(*it);
        }
    }
    
    // Build element table
    BinaryWriter elementWriter;
    for (const auto& elem : elements) {
        auto elemData = SerializeElement(elem);
        elementWriter.WriteBytes(elemData.data(), elemData.size());
    }
    
    // Build string table
    BinaryWriter stringWriter;
    stringTable.WriteTo(stringWriter);
    
    // Build style class table
    BinaryWriter classWriter;
    styleClassTable.WriteTo(classWriter);
    
    // Build event handler table
    BinaryWriter eventWriter;
    eventHandlerTable.WriteTo(eventWriter);
    
    // Calculate offsets
    size_t headerSize = sizeof(UCBHeader);
    size_t stringTableOffset = headerSize + elementWriter.GetData().size();
    // Additional tables follow string table
    
    // Write header
    UCBHeader header;
    header.magic = UCB_MAGIC;
    header.version = UCB_VERSION;
    header.flags = 0;
    header.elementCount = static_cast<uint32_t>(elements.size());
    header.stringTableOffset = static_cast<uint32_t>(stringTableOffset);
    
    mainWriter.WriteUInt32(header.magic);
    mainWriter.WriteUInt16(header.version);
    mainWriter.WriteUInt16(header.flags);
    mainWriter.WriteUInt32(header.elementCount);
    mainWriter.WriteUInt32(header.stringTableOffset);
    
    // Write sections
    const auto& elemData = elementWriter.GetData();
    mainWriter.WriteBytes(elemData.data(), elemData.size());
    
    const auto& strData = stringWriter.GetData();
    mainWriter.WriteBytes(strData.data(), strData.size());
    
    const auto& classData = classWriter.GetData();
    mainWriter.WriteBytes(classData.data(), classData.size());
    
    const auto& eventData = eventWriter.GetData();
    mainWriter.WriteBytes(eventData.data(), eventData.size());
    
    return mainWriter.TakeData();
}

std::vector<uint8_t> UICompiler::SerializeElement(const std::shared_ptr<UCMLElement>& element) {
    BinaryWriter elemWriter;
    
    // Get style classes
    auto classes = element->GetClasses();
    
    // Count non-class properties
    uint8_t propertyCount = 0;
    for (const auto& attr : element->attributes) {
        if (attr.name != "class") {
            propertyCount++;
        }
    }
    
    // Add text content as property if present
    if (!element->textContent.empty()) {
        propertyCount++;
    }
    
    // Write element header
    elemWriter.WriteUInt16(static_cast<uint16_t>(element->elementType));
    elemWriter.WriteUInt16(element->elementId);
    elemWriter.WriteUInt16(element->parentId);
    elemWriter.WriteUInt8(static_cast<uint8_t>(classes.size()));
    elemWriter.WriteUInt8(propertyCount);
    
    // Write style class IDs
    for (const auto& className : classes) {
        uint16_t classId = styleClassTable.FindClass(className);
        elemWriter.WriteUInt16(classId);
    }
    
    // Write properties
    for (const auto& attr : element->attributes) {
        if (attr.name == "class") continue;  // Already handled
        
        // Check if this is an event handler
        if (IsEventHandler(attr.name)) {
            const auto* handlerName = attr.AsString();
            if (handlerName) {
                uint16_t handlerId = stringTable.AddString(*handlerName);
                eventHandlerTable.AddHandler(element->elementId, attr.propertyId, handlerId);
            }
            
            // Still write the property
            elemWriter.WriteUInt8(static_cast<uint8_t>(attr.propertyId));
            elemWriter.WriteUInt8(static_cast<uint8_t>(UCBValueType::Handler));
            elemWriter.WriteUInt8(2);  // Size
            
            const auto* handler = attr.AsString();
            if (handler) {
                uint16_t handlerId = stringTable.FindString(*handler);
                if (handlerId == 0xFFFF) handlerId = stringTable.AddString(*handler);
                elemWriter.WriteUInt16(handlerId);
            } else {
                elemWriter.WriteUInt16(0);
            }
        } else {
            SerializePropertyValue(elemWriter, attr);
        }
    }
    
    // Write text content as property
    if (!element->textContent.empty()) {
        elemWriter.WriteUInt8(static_cast<uint8_t>(UCBPropertyId::Text));
        elemWriter.WriteUInt8(static_cast<uint8_t>(UCBValueType::String));
        elemWriter.WriteUInt8(2);  // Size (string ref)
        
        uint16_t textId = stringTable.FindString(element->textContent);
        if (textId == 0xFFFF) textId = stringTable.AddString(element->textContent);
        elemWriter.WriteUInt16(textId);
    }
    
    return elemWriter.TakeData();
}

void UICompiler::SerializePropertyValue(BinaryWriter& writer, const UCMLAttribute& attr) {
    UCBValueType valueType = GetValueType(attr);
    uint8_t valueSize = CalculatePropertySize(attr);
    
    // Write property header
    writer.WriteUInt8(static_cast<uint8_t>(attr.propertyId));
    writer.WriteUInt8(static_cast<uint8_t>(valueType));
    writer.WriteUInt8(valueSize);
    
    // Write value
    switch (valueType) {
        case UCBValueType::Null:
            // No data
            break;
            
        case UCBValueType::Bool: {
            const auto* boolVal = attr.AsBool();
            writer.WriteUInt8(boolVal && *boolVal ? 1 : 0);
            break;
        }
        
        case UCBValueType::Int32: {
            const auto* intVal = attr.AsInt();
            writer.WriteInt32(intVal ? *intVal : 0);
            break;
        }
        
        case UCBValueType::Float32: {
            const auto* floatVal = attr.AsFloat();
            writer.WriteFloat(floatVal ? static_cast<float>(*floatVal) : 0.0f);
            break;
        }
        
        case UCBValueType::String: {
            const auto* strVal = attr.AsString();
            if (strVal) {
                uint16_t strId = stringTable.FindString(*strVal);
                if (strId == 0xFFFF) strId = stringTable.AddString(*strVal);
                writer.WriteUInt16(strId);
            } else {
                writer.WriteUInt16(0);
            }
            break;
        }
        
        case UCBValueType::Handler:
        case UCBValueType::Binding: {
            const auto* strVal = attr.AsString();
            if (strVal) {
                uint16_t strId = stringTable.FindString(*strVal);
                if (strId == 0xFFFF) strId = stringTable.AddString(*strVal);
                writer.WriteUInt16(strId);
            } else {
                writer.WriteUInt16(0);
            }
            break;
        }
        
        default:
            break;
    }
}

UCBValueType UICompiler::GetValueType(const UCMLAttribute& attr) {
    if (!attr.HasValue()) {
        return UCBValueType::Null;
    }
    
    if (std::holds_alternative<bool>(attr.value)) {
        return UCBValueType::Bool;
    }
    
    if (std::holds_alternative<int32_t>(attr.value)) {
        return UCBValueType::Int32;
    }
    
    if (std::holds_alternative<double>(attr.value)) {
        return UCBValueType::Float32;
    }
    
    if (std::holds_alternative<std::string>(attr.value)) {
        // Check for special types
        if (IsEventHandler(attr.name)) {
            return UCBValueType::Handler;
        }
        if (IsDataBinding(attr.name)) {
            return UCBValueType::Binding;
        }
        return UCBValueType::String;
    }
    
    return UCBValueType::Invalid;
}

uint8_t UICompiler::CalculatePropertySize(const UCMLAttribute& attr) {
    UCBValueType type = GetValueType(attr);
    
    switch (type) {
        case UCBValueType::Null:    return 0;
        case UCBValueType::Bool:    return 1;
        case UCBValueType::Int32:   return 4;
        case UCBValueType::Float32: return 4;
        case UCBValueType::String:  return 2;  // String table reference
        case UCBValueType::Handler: return 2;  // String table reference
        case UCBValueType::Binding: return 2;  // String table reference
        default:                    return 0;
    }
}

// ============================================================================
// UCB FILE UTILITIES
// ============================================================================

UCBFileInfo GetUCBFileInfo(const std::string& filePath) {
    UCBFileInfo info;
    info.valid = false;
    
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        info.error = "Cannot open file";
        return info;
    }
    
    info.fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    if (info.fileSize < sizeof(UCBHeader)) {
        info.error = "File too small to contain valid header";
        return info;
    }
    
    UCBHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    file.close();
    
    if (header.magic != UCB_MAGIC) {
        info.error = "Invalid magic number";
        return info;
    }
    
    info.valid = true;
    info.version = header.version;
    info.flags = header.flags;
    info.elementCount = header.elementCount;
    
    return info;
}

bool ValidateUCBFile(const std::string& filePath, std::string& error) {
    UCBFileInfo info = GetUCBFileInfo(filePath);
    if (!info.valid) {
        error = info.error;
        return false;
    }
    return true;
}

std::string DumpUCBFile(const std::string& filePath) {
    std::ostringstream oss;
    
    UCBFileInfo info = GetUCBFileInfo(filePath);
    if (!info.valid) {
        oss << "Invalid UCB file: " << info.error << "\n";
        return oss.str();
    }
    
    oss << "=== UCB File Dump ===\n";
    oss << "File: " << filePath << "\n";
    oss << "Size: " << info.fileSize << " bytes\n";
    oss << "Version: 0x" << std::hex << info.version << std::dec << "\n";
    oss << "Flags: 0x" << std::hex << info.flags << std::dec << "\n";
    oss << "Element Count: " << info.elementCount << "\n";
    oss << "=====================\n";
    
    // Read full file for detailed dump
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        oss << "Cannot open file for detailed dump\n";
        return oss.str();
    }
    
    std::vector<uint8_t> data(info.fileSize);
    file.read(reinterpret_cast<char*>(data.data()), info.fileSize);
    file.close();
    
    // Dump header
    oss << "\n[Header]\n";
    oss << "  Magic: 0x" << std::hex << std::setfill('0') << std::setw(8) 
        << *reinterpret_cast<uint32_t*>(data.data()) << std::dec << "\n";
    oss << "  Version: 0x" << std::hex << std::setw(4) 
        << *reinterpret_cast<uint16_t*>(data.data() + 4) << std::dec << "\n";
    oss << "  Flags: 0x" << std::hex << std::setw(4)
        << *reinterpret_cast<uint16_t*>(data.data() + 6) << std::dec << "\n";
    oss << "  Element Count: " << *reinterpret_cast<uint32_t*>(data.data() + 8) << "\n";
    oss << "  String Table Offset: " << *reinterpret_cast<uint32_t*>(data.data() + 12) << "\n";
    
    // Dump first few elements
    oss << "\n[Elements]\n";
    size_t offset = sizeof(UCBHeader);
    for (uint32_t i = 0; i < std::min(info.elementCount, 5u) && offset < info.fileSize; i++) {
        if (offset + 8 > info.fileSize) break;
        
        uint16_t type = *reinterpret_cast<uint16_t*>(data.data() + offset);
        uint16_t elemId = *reinterpret_cast<uint16_t*>(data.data() + offset + 2);
        uint16_t parentId = *reinterpret_cast<uint16_t*>(data.data() + offset + 4);
        uint8_t classCount = data[offset + 6];
        uint8_t propCount = data[offset + 7];
        
        oss << "  Element " << i << ": type=0x" << std::hex << type << std::dec
            << " id=" << elemId << " parent=" << parentId
            << " classes=" << (int)classCount << " props=" << (int)propCount << "\n";
        
        // Skip to next element (approximate)
        offset += 8 + (classCount * 2) + (propCount * 6);
    }
    
    if (info.elementCount > 5) {
        oss << "  ... and " << (info.elementCount - 5) << " more elements\n";
    }
    
    return oss.str();
}

} // namespace UltraWeb
