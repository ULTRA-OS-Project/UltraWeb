// UltraWeb/runtime/UltraWebRuntime.h
// UltraWeb Runtime - Main coordinator for client-side rendering
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework
#pragma once

#include "../include/UltraWebFormats.h"
#include "../include/UltraWebBundler.h"
#include "UCBLoader.h"
#include "UCSLoader.h"
#include "JSEngine.h"
#include "UCApi.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// COMPUTED STYLE
// ============================================================================

// Computed style properties for a single element
struct ComputedStyle {
    // Layout
    int32_t display;        // 0=none, 1=block, 2=flex, 3=grid
    int32_t position;       // 0=static, 1=relative, 2=absolute, 3=fixed
    
    // Box model (in pixels)
    float width, height;
    float minWidth, maxWidth;
    float minHeight, maxHeight;
    float marginTop, marginRight, marginBottom, marginLeft;
    float paddingTop, paddingRight, paddingBottom, paddingLeft;
    float borderWidth;
    float borderRadius;
    
    // Position
    float top, right, bottom, left;
    
    // Flex
    int32_t flexDirection;  // 0=row, 1=column, 2=row-reverse, 3=column-reverse
    int32_t flexWrap;
    float flexGrow, flexShrink;
    int32_t justifyContent;
    int32_t alignItems;
    int32_t alignSelf;
    float gap;
    
    // Visual
    Runtime::StyleColor color;
    Runtime::StyleColor backgroundColor;
    Runtime::StyleColor borderColor;
    float opacity;
    int32_t overflow;
    int32_t zIndex;
    
    // Text
    std::string fontFamily;
    float fontSize;
    int32_t fontWeight;
    int32_t fontStyle;
    float lineHeight;
    int32_t textAlign;
    int32_t textDecoration;
    
    // Misc
    int32_t cursor;
    int32_t visibility;
    
    ComputedStyle();
    void Reset();
};

// ============================================================================
// RUNTIME ELEMENT
// ============================================================================

// Runtime element with computed properties
struct RuntimeElement {
    // Identity
    uint16_t elementId;
    uint16_t parentId;
    UCBElementType type;
    std::string id;
    std::vector<std::string> classNames;
    
    // Content
    std::string textContent;
    std::unordered_map<UCBPropertyId, PropertyValue> properties;
    
    // Event handlers
    std::unordered_map<UCBPropertyId, std::string> eventHandlers;
    
    // Computed layout
    ComputedStyle computedStyle;
    
    // Resolved layout (after layout pass)
    float x, y;             // Position relative to parent
    float layoutWidth;      // Actual computed width
    float layoutHeight;     // Actual computed height
    
    // State
    bool visible;
    bool enabled;
    bool focused;
    bool hovered;
    bool pressed;
    
    // Children
    std::vector<uint16_t> childIds;
    
    RuntimeElement();
    
    // Property access
    std::string GetStringProp(UCBPropertyId id, const std::string& def = "") const;
    int32_t GetIntProp(UCBPropertyId id, int32_t def = 0) const;
    float GetFloatProp(UCBPropertyId id, float def = 0) const;
    bool GetBoolProp(UCBPropertyId id, bool def = false) const;
    
    // Event handler check
    bool HasEventHandler(UCBPropertyId eventId) const;
    const std::string& GetEventHandler(UCBPropertyId eventId) const;
};

// ============================================================================
// STYLE ENGINE
// ============================================================================

class StyleEngine {
public:
    StyleEngine();
    
    // Load styles
    void LoadStyles(const UCSLoader& loader);
    
    // Compute style for element
    void ComputeStyle(RuntimeElement& element, const RuntimeElement* parent = nullptr);
    
    // Re-compute all styles
    void ComputeAllStyles(std::vector<RuntimeElement>& elements);
    
    // Get matching rules for element
    std::vector<const StyleRule*> GetMatchingRules(const RuntimeElement& element) const;
    
    // Clear
    void Clear();
    
private:
    std::vector<StyleRule> rules;
    std::unordered_map<std::string, std::vector<const StyleRule*>> classRules;
    std::unordered_map<std::string, std::vector<const StyleRule*>> idRules;
    std::unordered_map<std::string, std::vector<const StyleRule*>> tagRules;
    
    void ApplyRule(const StyleRule& rule, ComputedStyle& style);
    void ApplyProperty(const StyleProperty& prop, ComputedStyle& style);
};

// ============================================================================
// RUNTIME LOAD RESULT
// ============================================================================

struct RuntimeLoadResult {
    bool success;
    std::string error;
    
    // Stats
    uint32_t elementCount;
    uint32_t styleRuleCount;
    uint32_t codeSize;
    uint32_t assetCount;
    
    RuntimeLoadResult() 
        : success(false), elementCount(0), styleRuleCount(0), codeSize(0), assetCount(0) {}
};

// ============================================================================
// EVENT CALLBACK TYPES
// ============================================================================

using EventCallback = std::function<void(uint16_t elementId, const std::string& handlerName)>;
using RenderCallback = std::function<void(const RuntimeElement& element)>;

// ============================================================================
// ULTRAWEB RUNTIME
// ============================================================================

class UltraWebRuntime {
public:
    UltraWebRuntime();
    ~UltraWebRuntime();
    
    // Load package
    RuntimeLoadResult LoadPackage(const std::vector<uint8_t>& data);
    RuntimeLoadResult LoadPackage(const uint8_t* data, size_t size);
    RuntimeLoadResult LoadPackageFromFile(const std::string& filePath);
    
    // Load individual sections
    bool LoadUI(const std::vector<uint8_t>& ucbData);
    bool LoadStyles(const std::vector<uint8_t>& ucsData);
    bool LoadCode(const std::vector<uint8_t>& hbcData);
    bool LoadAssets(const std::vector<uint8_t>& ucaData);
    
    // Element access
    const std::vector<RuntimeElement>& GetElements() const { return elements; }
    RuntimeElement* GetElement(uint16_t elementId);
    RuntimeElement* GetElementById(const std::string& id);
    RuntimeElement* GetRootElement();
    std::vector<RuntimeElement*> GetChildren(uint16_t parentId);
    
    // Tree traversal
    void TraverseDepthFirst(std::function<void(RuntimeElement&, int depth)> visitor);
    void TraverseBreadthFirst(std::function<void(RuntimeElement&)> visitor);
    
    // Style operations
    void RecomputeStyles();
    void RecomputeStyle(uint16_t elementId);
    
    // Layout operations
    void PerformLayout(float viewportWidth, float viewportHeight);
    
    // Rendering
    void Render(RenderCallback callback);
    
    // Event handling
    void SetEventCallback(EventCallback callback);
    void DispatchEvent(uint16_t elementId, UCBPropertyId eventType);
    RuntimeElement* HitTest(float x, float y);
    
    // State management
    void SetElementVisible(uint16_t elementId, bool visible);
    void SetElementEnabled(uint16_t elementId, bool enabled);
    void SetElementFocused(uint16_t elementId, bool focused);
    void SetElementHovered(uint16_t elementId, bool hovered);
    void SetElementPressed(uint16_t elementId, bool pressed);
    
    // Update text/value
    void SetElementText(uint16_t elementId, const std::string& text);
    void SetElementValue(uint16_t elementId, const std::string& value);
    
    // Code section access (for Hermes integration)
    const std::vector<uint8_t>& GetCodeSection() const { return codeSection; }

    // ===== JAVASCRIPT (Phase 3) =====
    // Attaches a JS engine (HermesEngine in production, QuickJSEngine for
    // development): registers the __uc_native dispatcher and evaluates the
    // UC prelude. Returns false with `error` set if the prelude fails.
    bool AttachJSEngine(std::shared_ptr<JSEngine> engine, std::string& error);

    // Executes the loaded code section: Hermes bytecode when the buffer
    // carries the HBC magic (requires a bytecode-capable engine), otherwise
    // treated as UTF-8 JavaScript source (development mode).
    bool ExecuteCodeSection(std::string& error);

    JSEngine* GetJSEngine() const { return jsEngine.get(); }
    UCApi* GetJSApi() const { return jsApi.get(); }

    // Forwards a UI event to JS handlers registered via element.on(...).
    // eventJSON is the event payload; returns true if a JS handler ran.
    bool FireDomEvent(uint16_t elementId, const std::string& eventType,
                      const std::string& eventJSON = "{}");

    // ===== DELTA UPDATES (Phase 4) =====
    // Applies a UCDELTA stream (see include/UltraWebDelta.h) to the loaded
    // application. Refuses deltas whose baseCrc32 does not match the
    // client's current state; on success the state CRC advances to the
    // delta's targetCrc32 so subsequent deltas chain.
    bool ApplyDelta(const std::vector<uint8_t>& delta, std::string& error);

    // CRC32 of the currently loaded package content (0 = nothing loaded)
    uint32_t GetLoadedCrc32() const { return loadedCrc32; }
    
    // Asset access
    const std::vector<uint8_t>& GetAssetSection() const { return assetSection; }
    
    // Debug
    std::string DumpElementTree() const;
    std::string DumpStyles() const;
    
    // Clear everything
    void Clear();
    
private:
    // Loaders
    UCBLoader ucbLoader;
    UCSLoader ucsLoader;
    StyleEngine styleEngine;
    
    // Runtime data
    std::vector<RuntimeElement> elements;
    std::unordered_map<std::string, uint16_t> idToElement;
    std::unordered_map<uint16_t, std::vector<uint16_t>> parentToChildren;
    
    // Raw sections
    std::vector<uint8_t> codeSection;
    std::vector<uint8_t> assetSection;

    // JavaScript (Phase 3)
    std::shared_ptr<JSEngine> jsEngine;
    std::unique_ptr<UCApi> jsApi;
    
    // Callbacks
    EventCallback eventCallback;
    
    // Viewport
    float viewportWidth;
    float viewportHeight;
    
    // State
    bool isLoaded;
    uint32_t loadedCrc32 = 0;
    uint16_t focusedElementId;
    uint16_t hoveredElementId;
    uint16_t pressedElementId;
    
    // Internal
    void BuildRuntimeElements();
    void BuildIndices();
    void TraverseDepthFirstImpl(uint16_t elementId, int depth,
                                 std::function<void(RuntimeElement&, int)>& visitor);
    void LayoutElement(RuntimeElement& element, float parentX, float parentY,
                       float availableWidth, float availableHeight);
    void RenderElement(const RuntimeElement& element, RenderCallback& callback, int depth);
    RuntimeElement* HitTestElement(RuntimeElement& element, float x, float y);
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Check if element type is a container
bool IsContainerType(UCBElementType type);

// Check if element type is interactive
bool IsInteractiveType(UCBElementType type);

} // namespace Runtime
} // namespace UltraWeb
