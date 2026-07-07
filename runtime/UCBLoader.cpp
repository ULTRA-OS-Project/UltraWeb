// UltraWeb/runtime/UCBLoader.cpp
// UCB Binary UI Format Loader Implementation
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "UCBLoader.h"
#include <cstring>
#include <sstream>
#include <queue>
#include <iomanip>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// ELEMENT NODE IMPLEMENTATION
// ============================================================================

bool ElementNode::HasProperty(UCBPropertyId propId) const {
    for (const auto& prop : properties) {
        if (prop.id == propId) return true;
    }
    return false;
}

const PropertyValue* ElementNode::GetProperty(UCBPropertyId propId) const {
    for (const auto& prop : properties) {
        if (prop.id == propId) {
            return &prop.value;
        }
    }
    return nullptr;
}

std::string ElementNode::GetStringProperty(UCBPropertyId propId, const std::string& defaultVal) const {
    const PropertyValue* val = GetProperty(propId);
    if (val && std::holds_alternative<std::string>(*val)) {
        return std::get<std::string>(*val);
    }
    return defaultVal;
}

int32_t ElementNode::GetIntProperty(UCBPropertyId propId, int32_t defaultVal) const {
    const PropertyValue* val = GetProperty(propId);
    if (val && std::holds_alternative<int32_t>(*val)) {
        return std::get<int32_t>(*val);
    }
    return defaultVal;
}

float ElementNode::GetFloatProperty(UCBPropertyId propId, float defaultVal) const {
    const PropertyValue* val = GetProperty(propId);
    if (val && std::holds_alternative<float>(*val)) {
        return std::get<float>(*val);
    }
    return defaultVal;
}

bool ElementNode::GetBoolProperty(UCBPropertyId propId, bool defaultVal) const {
    const PropertyValue* val = GetProperty(propId);
    if (val && std::holds_alternative<bool>(*val)) {
        return std::get<bool>(*val);
    }
    return defaultVal;
}

// ============================================================================
// UCB LOADER IMPLEMENTATION
// ============================================================================

UCBLoader::UCBLoader() : isLoaded(false) {
    std::memset(&header, 0, sizeof(header));
}

UCBLoader::~UCBLoader() {
    Clear();
}

UCBLoadResult UCBLoader::Load(const std::vector<uint8_t>& data) {
    return Load(data.data(), data.size());
}

UCBLoadResult UCBLoader::Load(const uint8_t* data, size_t size) {
    UCBLoadResult result;
    
    Clear();
    
    // Validate minimum size
    if (size < sizeof(UCBHeader)) {
        result.error = "Data too small for UCB header";
        return result;
    }
    
    // Parse header
    if (!ParseHeader(data, size)) {
        result.error = "Invalid UCB header";
        return result;
    }
    
    result.version = header.version;
    result.flags = header.flags;
    result.elementCount = header.elementCount;
    
    // Parse string table first (needed for element parsing)
    if (!ParseStringTable(data, size)) {
        result.error = "Failed to parse string table";
        return result;
    }
    
    // Parse elements
    if (!ParseElements(data, size)) {
        result.error = "Failed to parse elements";
        return result;
    }
    
    // Build lookup indices
    BuildIndices();
    
    isLoaded = true;
    result.success = true;
    return result;
}

bool UCBLoader::ParseHeader(const uint8_t* data, size_t size) {
    if (size < sizeof(UCBHeader)) return false;
    
    std::memcpy(&header, data, sizeof(UCBHeader));
    
    // Validate magic
    if (header.magic != UCB_MAGIC) {
        return false;
    }
    
    return true;
}

bool UCBLoader::ParseStringTable(const uint8_t* data, size_t size) {
    if (header.stringTableOffset == 0) {
        return true;  // No string table
    }
    
    if (header.stringTableOffset >= size) {
        return false;
    }
    
    const uint8_t* ptr = data + header.stringTableOffset;
    const uint8_t* end = data + size;
    
    // Read string count
    if (ptr + 2 > end) return false;
    uint16_t stringCount = *reinterpret_cast<const uint16_t*>(ptr);
    ptr += 2;
    
    stringTable.reserve(stringCount);
    
    // Read strings
    for (uint16_t i = 0; i < stringCount; i++) {
        if (ptr + 2 > end) return false;
        
        uint16_t len = *reinterpret_cast<const uint16_t*>(ptr);
        ptr += 2;
        
        if (ptr + len > end) return false;
        
        stringTable.emplace_back(reinterpret_cast<const char*>(ptr), len);
        ptr += len;
    }
    
    // Style class table follows string table
    size_t styleClassTableOffset = ptr - data;
    return ParseStyleClassTable(data, size, styleClassTableOffset);
}

bool UCBLoader::ParseStyleClassTable(const uint8_t* data, size_t size, size_t startOffset) {
    if (startOffset >= size) {
        return true;  // No style class table
    }
    
    const uint8_t* ptr = data + startOffset;
    const uint8_t* end = data + size;
    
    // Read class count
    if (ptr + 2 > end) return true;  // End of data is OK
    uint16_t classCount = *reinterpret_cast<const uint16_t*>(ptr);
    ptr += 2;
    
    styleClassTable.reserve(classCount);
    
    // Read class names
    for (uint16_t i = 0; i < classCount && ptr + 2 <= end; i++) {
        uint16_t len = *reinterpret_cast<const uint16_t*>(ptr);
        ptr += 2;
        
        if (ptr + len > end) break;
        
        styleClassTable.emplace_back(reinterpret_cast<const char*>(ptr), len);
        ptr += len;
    }
    
    return true;
}

bool UCBLoader::ParseElements(const uint8_t* data, size_t size) {
    const uint8_t* ptr = data + sizeof(UCBHeader);
    const uint8_t* end = data + header.stringTableOffset;
    
    if (header.stringTableOffset == 0) {
        end = data + size;
    }
    
    elements.reserve(header.elementCount);
    
    for (uint32_t i = 0; i < header.elementCount; i++) {
        ElementNode node;
        if (!ParseElement(ptr, end, node)) {
            return false;
        }
        elements.push_back(std::move(node));
    }
    
    return true;
}

bool UCBLoader::ParseElement(const uint8_t*& ptr, const uint8_t* end, ElementNode& node) {
    // Parse element header (8 bytes minimum)
    if (ptr + 8 > end) return false;
    
    // Read element type (2 bytes)
    node.type = static_cast<UCBElementType>(*reinterpret_cast<const uint16_t*>(ptr));
    ptr += 2;
    
    // Read element ID (2 bytes)
    node.elementId = *reinterpret_cast<const uint16_t*>(ptr);
    ptr += 2;
    
    // Read parent ID (2 bytes)
    node.parentId = *reinterpret_cast<const uint16_t*>(ptr);
    ptr += 2;
    
    // Read style class count (1 byte)
    uint8_t styleClassCount = *ptr++;
    
    // Read property count (1 byte)
    uint8_t propertyCount = *ptr++;
    
    // Read style classes
    if (ptr + styleClassCount * 2 > end) return false;
    node.styleClasses.reserve(styleClassCount);
    for (uint8_t i = 0; i < styleClassCount; i++) {
        node.styleClasses.push_back(*reinterpret_cast<const uint16_t*>(ptr));
        ptr += 2;
    }
    
    // Read properties
    node.properties.reserve(propertyCount);
    for (uint8_t i = 0; i < propertyCount; i++) {
        ElementProperty prop;
        if (!ParseProperty(ptr, end, prop)) {
            return false;
        }
        
        // Cache common properties
        if (prop.id == UCBPropertyId::Id && std::holds_alternative<std::string>(prop.value)) {
            node.id = std::get<std::string>(prop.value);
        } else if (prop.id == UCBPropertyId::Text && std::holds_alternative<std::string>(prop.value)) {
            node.textContent = std::get<std::string>(prop.value);
        }
        
        node.properties.push_back(std::move(prop));
    }
    
    return true;
}

bool UCBLoader::ParseProperty(const uint8_t*& ptr, const uint8_t* end, ElementProperty& prop) {
    // Property header: id (1 byte) + type (1 byte) + size (1 byte)
    if (ptr + 3 > end) return false;
    
    prop.id = static_cast<UCBPropertyId>(*ptr++);
    prop.type = static_cast<UCBValueType>(*ptr++);
    uint8_t valueSize = *ptr++;
    
    if (ptr + valueSize > end) return false;
    
    // Parse value based on type
    switch (prop.type) {
        case UCBValueType::Null:
            prop.value = std::monostate{};
            break;
            
        case UCBValueType::Bool:
            if (valueSize >= 1) {
                prop.value = (*ptr != 0);
            }
            break;
            
        case UCBValueType::Int32:
            if (valueSize >= 4) {
                prop.value = *reinterpret_cast<const int32_t*>(ptr);
            }
            break;
            
        case UCBValueType::Float32:
            if (valueSize >= 4) {
                prop.value = *reinterpret_cast<const float*>(ptr);
            }
            break;
            
        case UCBValueType::String:
        case UCBValueType::StyleClass:
        case UCBValueType::Handler:
        case UCBValueType::Binding:
            if (valueSize >= 2) {
                uint16_t stringRef = *reinterpret_cast<const uint16_t*>(ptr);
                if (stringRef < stringTable.size()) {
                    prop.value = stringTable[stringRef];
                } else {
                    prop.value = std::string();
                }
            }
            break;
            
        case UCBValueType::Array:
            // Arrays not fully implemented yet
            prop.value = std::monostate{};
            break;
            
        default:
            break;
    }
    
    ptr += valueSize;
    return true;
}

void UCBLoader::BuildIndices() {
    idToElement.clear();
    parentToChildren.clear();
    
    for (const auto& elem : elements) {
        // ID index
        if (!elem.id.empty()) {
            idToElement[elem.id] = elem.elementId;
        }
        
        // Parent-child index
        if (elem.parentId != 0 || elem.elementId != 1) {
            parentToChildren[elem.parentId].push_back(elem.elementId);
        }
    }
}

const ElementNode* UCBLoader::GetElement(uint16_t elementId) const {
    for (const auto& elem : elements) {
        if (elem.elementId == elementId) {
            return &elem;
        }
    }
    return nullptr;
}

const ElementNode* UCBLoader::GetElementById(const std::string& id) const {
    auto it = idToElement.find(id);
    if (it != idToElement.end()) {
        return GetElement(it->second);
    }
    return nullptr;
}

const ElementNode* UCBLoader::GetRootElement() const {
    if (elements.empty()) return nullptr;
    
    // Root is typically element with parentId = 0
    for (const auto& elem : elements) {
        if (elem.parentId == 0) {
            return &elem;
        }
    }
    
    // Fallback to first element
    return &elements[0];
}

std::vector<const ElementNode*> UCBLoader::GetChildren(uint16_t parentId) const {
    std::vector<const ElementNode*> children;
    
    auto it = parentToChildren.find(parentId);
    if (it != parentToChildren.end()) {
        for (uint16_t childId : it->second) {
            const ElementNode* child = GetElement(childId);
            if (child) {
                children.push_back(child);
            }
        }
    }
    
    return children;
}

const std::string& UCBLoader::GetString(uint16_t index) const {
    static const std::string empty;
    if (index < stringTable.size()) {
        return stringTable[index];
    }
    return empty;
}

const std::string& UCBLoader::GetStyleClassName(uint16_t classId) const {
    static const std::string empty;
    if (classId < styleClassTable.size()) {
        return styleClassTable[classId];
    }
    return empty;
}

const std::string& UCBLoader::GetEventHandler(uint16_t handlerId) const {
    return GetString(handlerId);
}

void UCBLoader::TraverseDepthFirst(std::function<void(const ElementNode&, int depth)> visitor) const {
    const ElementNode* root = GetRootElement();
    if (root) {
        TraverseDepthFirstImpl(root->elementId, 0, visitor);
    }
}

void UCBLoader::TraverseDepthFirstImpl(uint16_t elementId, int depth,
                                        std::function<void(const ElementNode&, int)>& visitor) const {
    const ElementNode* elem = GetElement(elementId);
    if (!elem) return;
    
    visitor(*elem, depth);
    
    auto children = GetChildren(elementId);
    for (const auto* child : children) {
        TraverseDepthFirstImpl(child->elementId, depth + 1, visitor);
    }
}

void UCBLoader::TraverseBreadthFirst(std::function<void(const ElementNode&)> visitor) const {
    const ElementNode* root = GetRootElement();
    if (!root) return;
    
    std::queue<uint16_t> queue;
    queue.push(root->elementId);
    
    while (!queue.empty()) {
        uint16_t currentId = queue.front();
        queue.pop();
        
        const ElementNode* elem = GetElement(currentId);
        if (!elem) continue;
        
        visitor(*elem);
        
        auto children = GetChildren(currentId);
        for (const auto* child : children) {
            queue.push(child->elementId);
        }
    }
}

std::string UCBLoader::DumpTree() const {
    std::ostringstream oss;
    oss << "=== UCB Element Tree ===\n";
    oss << "Version: 0x" << std::hex << header.version << std::dec << "\n";
    oss << "Elements: " << elements.size() << "\n";
    oss << "Strings: " << stringTable.size() << "\n\n";
    
    TraverseDepthFirst([&oss, this](const ElementNode& node, int depth) {
        oss << DumpElement(node, depth);
    });
    
    return oss.str();
}

std::string UCBLoader::DumpElement(const ElementNode& node, int indent) const {
    std::ostringstream oss;
    std::string pad(indent * 2, ' ');
    
    oss << pad << GetElementTypeName(node.type);
    if (!node.id.empty()) {
        oss << " #" << node.id;
    }
    
    // Style classes
    if (!node.styleClasses.empty()) {
        oss << " [";
        for (size_t i = 0; i < node.styleClasses.size(); i++) {
            if (i > 0) oss << ", ";
            oss << "." << GetString(node.styleClasses[i]);
        }
        oss << "]";
    }
    
    oss << "\n";
    
    // Properties
    for (const auto& prop : node.properties) {
        if (prop.id == UCBPropertyId::Id) continue;  // Already shown
        
        oss << pad << "  " << GetPropertyName(prop.id) << ": ";
        
        if (std::holds_alternative<std::string>(prop.value)) {
            oss << "\"" << std::get<std::string>(prop.value) << "\"";
        } else if (std::holds_alternative<int32_t>(prop.value)) {
            oss << std::get<int32_t>(prop.value);
        } else if (std::holds_alternative<float>(prop.value)) {
            oss << std::get<float>(prop.value);
        } else if (std::holds_alternative<bool>(prop.value)) {
            oss << (std::get<bool>(prop.value) ? "true" : "false");
        } else {
            oss << "(null)";
        }
        oss << "\n";
    }
    
    return oss.str();
}

void UCBLoader::Clear() {
    elements.clear();
    stringTable.clear();
    styleClassTable.clear();
    idToElement.clear();
    parentToChildren.clear();
    std::memset(&header, 0, sizeof(header));
    isLoaded = false;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char* GetElementTypeName(UCBElementType type) {
    switch (type) {
        case UCBElementType::Container:  return "Container";
        case UCBElementType::FlexBox:    return "FlexBox";
        case UCBElementType::Grid:       return "Grid";
        case UCBElementType::ScrollView: return "ScrollView";
        case UCBElementType::Text:       return "Text";
        case UCBElementType::Button:     return "Button";
        case UCBElementType::Input:      return "Input";
        case UCBElementType::TextArea:   return "TextArea";
        case UCBElementType::Checkbox:   return "Checkbox";
        case UCBElementType::Radio:      return "Radio";
        case UCBElementType::Select:     return "Select";
        case UCBElementType::Slider:     return "Slider";
        case UCBElementType::Image:      return "Image";
        case UCBElementType::SVG:        return "SVG";
        case UCBElementType::Canvas:     return "Canvas";
        case UCBElementType::Video:      return "Video";
        case UCBElementType::Audio:      return "Audio";
        case UCBElementType::List:       return "List";
        case UCBElementType::Table:      return "Table";
        case UCBElementType::Tree:       return "Tree";
        case UCBElementType::Tabs:       return "Tabs";
        case UCBElementType::Modal:      return "Modal";
        case UCBElementType::Menu:       return "Menu";
        case UCBElementType::Tooltip:    return "Tooltip";
        case UCBElementType::Popover:    return "Popover";
        default: return "Unknown";
    }
}

const char* GetPropertyName(UCBPropertyId id) {
    switch (id) {
        case UCBPropertyId::Id:          return "id";
        case UCBPropertyId::Class:       return "class";
        case UCBPropertyId::Name:        return "name";
        case UCBPropertyId::Visible:     return "visible";
        case UCBPropertyId::Enabled:     return "enabled";
        case UCBPropertyId::Text:        return "text";
        case UCBPropertyId::Value:       return "value";
        case UCBPropertyId::Placeholder: return "placeholder";
        case UCBPropertyId::Src:         return "src";
        case UCBPropertyId::Alt:         return "alt";
        case UCBPropertyId::Href:        return "href";
        case UCBPropertyId::Width:       return "width";
        case UCBPropertyId::Height:      return "height";
        case UCBPropertyId::MinWidth:    return "minWidth";
        case UCBPropertyId::MaxWidth:    return "maxWidth";
        case UCBPropertyId::MinHeight:   return "minHeight";
        case UCBPropertyId::MaxHeight:   return "maxHeight";
        case UCBPropertyId::Type:        return "type";
        case UCBPropertyId::Required:    return "required";
        case UCBPropertyId::ReadOnly:    return "readonly";
        case UCBPropertyId::Disabled:    return "disabled";
        case UCBPropertyId::Checked:     return "checked";
        case UCBPropertyId::Selected:    return "selected";
        case UCBPropertyId::Min:         return "min";
        case UCBPropertyId::Max:         return "max";
        case UCBPropertyId::Step:        return "step";
        case UCBPropertyId::OnClick:     return "onclick";
        case UCBPropertyId::OnChange:    return "onchange";
        case UCBPropertyId::OnInput:     return "oninput";
        case UCBPropertyId::OnFocus:     return "onfocus";
        case UCBPropertyId::OnBlur:      return "onblur";
        case UCBPropertyId::OnSubmit:    return "onsubmit";
        case UCBPropertyId::OnKeyDown:   return "onkeydown";
        case UCBPropertyId::OnKeyUp:     return "onkeyup";
        case UCBPropertyId::OnMouseEnter:return "onmouseenter";
        case UCBPropertyId::OnMouseLeave:return "onmouseleave";
        case UCBPropertyId::OnScroll:    return "onscroll";
        case UCBPropertyId::Bind:        return "bind";
        case UCBPropertyId::Model:       return "model";
        case UCBPropertyId::For:         return "for";
        case UCBPropertyId::If:          return "if";
        default: return "unknown";
    }
}

const char* GetValueTypeName(UCBValueType type) {
    switch (type) {
        case UCBValueType::Null:       return "null";
        case UCBValueType::Bool:       return "bool";
        case UCBValueType::Int32:      return "int32";
        case UCBValueType::Float32:    return "float32";
        case UCBValueType::String:     return "string";
        case UCBValueType::StyleClass: return "styleClass";
        case UCBValueType::Handler:    return "handler";
        case UCBValueType::Binding:    return "binding";
        case UCBValueType::Array:      return "array";
        default: return "unknown";
    }
}

} // namespace Runtime
} // namespace UltraWeb
