// UltraWeb/runtime/UCBLoader.h
// UCB Binary UI Format Loader - Client-side decoder
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework
#pragma once

#include "../include/UltraWebFormats.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <variant>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// RUNTIME ELEMENT NODE
// ============================================================================

// Property value variant for runtime
using PropertyValue = std::variant<
    std::monostate,     // Null
    bool,               // Boolean
    int32_t,            // Integer
    float,              // Float
    std::string,        // String
    uint16_t            // Reference (style class, handler)
>;

struct ElementProperty {
    UCBPropertyId id;
    UCBValueType type;
    PropertyValue value;
    
    ElementProperty() : id(UCBPropertyId::Id), type(UCBValueType::Null) {}
};

struct ElementNode {
    uint16_t elementId;
    uint16_t parentId;
    UCBElementType type;
    std::vector<uint16_t> styleClasses;
    std::vector<ElementProperty> properties;
    std::vector<uint16_t> childIds;
    
    // Cached lookups
    std::string id;         // Cached string id
    std::string textContent;
    
    ElementNode() : elementId(0), parentId(0), type(UCBElementType::Container) {}
    
    // Property accessors
    bool HasProperty(UCBPropertyId propId) const;
    const PropertyValue* GetProperty(UCBPropertyId propId) const;
    std::string GetStringProperty(UCBPropertyId propId, const std::string& defaultVal = "") const;
    int32_t GetIntProperty(UCBPropertyId propId, int32_t defaultVal = 0) const;
    float GetFloatProperty(UCBPropertyId propId, float defaultVal = 0.0f) const;
    bool GetBoolProperty(UCBPropertyId propId, bool defaultVal = false) const;
};

// ============================================================================
// UCB LOADER
// ============================================================================

struct UCBLoadResult {
    bool success;
    std::string error;
    uint16_t version;
    uint16_t flags;
    uint32_t elementCount;
    
    UCBLoadResult() : success(false), version(0), flags(0), elementCount(0) {}
};

class UCBLoader {
public:
    UCBLoader();
    ~UCBLoader();
    
    // Load from binary data
    UCBLoadResult Load(const std::vector<uint8_t>& data);
    UCBLoadResult Load(const uint8_t* data, size_t size);
    
    // Access loaded elements
    const std::vector<ElementNode>& GetElements() const { return elements; }
    const ElementNode* GetElement(uint16_t elementId) const;
    const ElementNode* GetElementById(const std::string& id) const;
    const ElementNode* GetRootElement() const;
    
    // Get children of an element
    std::vector<const ElementNode*> GetChildren(uint16_t parentId) const;
    
    // String table access
    const std::string& GetString(uint16_t index) const;
    size_t GetStringCount() const { return stringTable.size(); }
    
    // Style class name lookup
    const std::string& GetStyleClassName(uint16_t classId) const;
    
    // Event handler lookup
    const std::string& GetEventHandler(uint16_t handlerId) const;
    
    // Tree traversal
    void TraverseDepthFirst(std::function<void(const ElementNode&, int depth)> visitor) const;
    void TraverseBreadthFirst(std::function<void(const ElementNode&)> visitor) const;
    
    // Debug
    std::string DumpTree() const;
    
    // Clear loaded data
    void Clear();
    
private:
    std::vector<ElementNode> elements;
    std::vector<std::string> stringTable;
    std::vector<std::string> styleClassTable;  // Separate table for class names
    std::unordered_map<std::string, uint16_t> idToElement;
    std::unordered_map<uint16_t, std::vector<uint16_t>> parentToChildren;
    
    UCBHeader header;
    bool isLoaded;
    
    // Internal parsing
    bool ParseHeader(const uint8_t* data, size_t size);
    bool ParseElements(const uint8_t* data, size_t size);
    bool ParseStringTable(const uint8_t* data, size_t size);
    bool ParseStyleClassTable(const uint8_t* data, size_t size, size_t startOffset);
    bool ParseElement(const uint8_t*& ptr, const uint8_t* end, ElementNode& node);
    bool ParseProperty(const uint8_t*& ptr, const uint8_t* end, ElementProperty& prop);
    
    // Build helper indices
    void BuildIndices();
    
    // Tree traversal helpers
    void TraverseDepthFirstImpl(uint16_t elementId, int depth,
                                 std::function<void(const ElementNode&, int)>& visitor) const;
    std::string DumpElement(const ElementNode& node, int indent) const;
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get element type name
const char* GetElementTypeName(UCBElementType type);

// Get property name
const char* GetPropertyName(UCBPropertyId id);

// Get value type name
const char* GetValueTypeName(UCBValueType type);

} // namespace Runtime
} // namespace UltraWeb
