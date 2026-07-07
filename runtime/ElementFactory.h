// UltraWeb/runtime/ElementFactory.h
// Element Factory - Creates UCComponents from RuntimeElement data
// Version: 1.0.0

#pragma once

#include "UCComponents.h"
#include "UltraWebRuntime.h"
#include <memory>
#include <unordered_map>
#include <functional>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// STYLE APPLICATOR
// ============================================================================

// Applies ComputedStyle to UCComponent
class StyleApplicator {
public:
    static void Apply(UCComponent* component, const ComputedStyle& style);
    static void ApplyLayout(UCComponent* component, const ComputedStyle& style);
    static void ApplyVisual(UCComponent* component, const ComputedStyle& style);
    static void ApplyText(UCText* text, const ComputedStyle& style);
    static void ApplyContainer(UCContainer* container, const ComputedStyle& style);
};

// ============================================================================
// ELEMENT FACTORY
// ============================================================================

class ElementFactory {
public:
    ElementFactory();
    ~ElementFactory();
    
    // Create single component from RuntimeElement
    std::shared_ptr<UCComponent> CreateComponent(const RuntimeElement& element);
    
    // Build entire component tree from runtime
    std::shared_ptr<UCComponent> BuildComponentTree(UltraWebRuntime& runtime);
    
    // Get component by element ID
    UCComponent* GetComponent(uint16_t elementId);
    
    // Get element ID by component
    uint16_t GetElementId(UCComponent* component);
    
    // Update component from RuntimeElement (for incremental updates)
    void UpdateComponent(UCComponent* component, const RuntimeElement& element);
    
    // Apply styles to component tree
    void ApplyStyles(UCComponent* root);
    
    // Clear all mappings
    void Clear();
    
    // Custom component factory registration
    using ComponentCreator = std::function<std::shared_ptr<UCComponent>(const RuntimeElement&)>;
    void RegisterCreator(UCBElementType type, ComponentCreator creator);
    
    // Debug
    std::string DumpComponentTree(UCComponent* root, int depth = 0);
    
private:
    std::shared_ptr<UCComponent> CreateByType(UCBElementType type);
    void PopulateComponent(UCComponent* component, const RuntimeElement& element);
    void SetupEventHandlers(UCComponent* component, const RuntimeElement& element);
    
    std::unordered_map<uint16_t, std::shared_ptr<UCComponent>> elementToComponent;
    std::unordered_map<UCComponent*, uint16_t> componentToElement;
    std::unordered_map<UCBElementType, ComponentCreator> customCreators;
    
    UltraWebRuntime* currentRuntime;  // For event dispatch
};

// ============================================================================
// COMPONENT TREE RENDERER
// ============================================================================

// Manages rendering of component tree to a render context
class ComponentTreeRenderer {
public:
    ComponentTreeRenderer();
    
    void SetRoot(std::shared_ptr<UCComponent> root);
    UCComponent* GetRoot() const { return rootComponent.get(); }
    
    // Render to context
    void Render(UCRenderContext& ctx);
    
    // Layout
    void PerformLayout(float width, float height);
    
    // Hit testing
    UCComponent* HitTest(float x, float y);
    
    // Event handling
    void HandleMouseMove(float x, float y);
    void HandleMouseDown(float x, float y, int button = 0);
    void HandleMouseUp(float x, float y, int button = 0);
    void HandleClick(float x, float y);
    void HandleKeyDown(int keyCode, const std::string& key);
    void HandleKeyUp(int keyCode, const std::string& key);
    
    // Focus management
    void SetFocused(UCComponent* component);
    UCComponent* GetFocused() const { return focusedComponent; }
    
    // Statistics
    struct RenderStats {
        int componentCount;
        int drawCalls;
        double layoutTimeMs;
        double renderTimeMs;
    };
    RenderStats GetLastStats() const { return lastStats; }
    
private:
    void DispatchEvent(UCComponent* target, UCEventType type, float x, float y);
    
    std::shared_ptr<UCComponent> rootComponent;
    UCComponent* hoveredComponent;
    UCComponent* pressedComponent;
    UCComponent* focusedComponent;
    RenderStats lastStats;
};

// ============================================================================
// INTEGRATION BRIDGE
// ============================================================================

// High-level integration: Runtime + Factory + Renderer
class UltraWebBridge {
public:
    UltraWebBridge();
    ~UltraWebBridge();
    
    // Load and build from package
    bool LoadPackage(const std::vector<uint8_t>& data);
    bool LoadPackage(const uint8_t* data, size_t size);
    
    // Access runtime
    UltraWebRuntime& GetRuntime() { return runtime; }
    const UltraWebRuntime& GetRuntime() const { return runtime; }
    
    // Access component tree
    UCComponent* GetRootComponent() { return renderer.GetRoot(); }
    UCComponent* GetComponent(uint16_t elementId) { return factory.GetComponent(elementId); }
    
    // Rendering
    void SetViewport(float width, float height);
    void Render(UCRenderContext& ctx);
    
    // Events
    void HandleMouseMove(float x, float y);
    void HandleMouseDown(float x, float y, int button = 0);
    void HandleMouseUp(float x, float y, int button = 0);
    void HandleClick(float x, float y);
    void HandleKeyDown(int keyCode, const std::string& key);
    void HandleKeyUp(int keyCode, const std::string& key);
    
    // Event callback (for JS bridge integration)
    using JSEventCallback = std::function<void(uint16_t elementId, const std::string& handler, const UCEvent& event)>;
    void SetJSEventCallback(JSEventCallback callback);
    
    // Update element properties (from JS)
    void SetElementText(uint16_t elementId, const std::string& text);
    void SetElementValue(uint16_t elementId, const std::string& value);
    void SetElementVisible(uint16_t elementId, bool visible);
    
    // Re-layout and re-render after changes
    void Invalidate();
    
    // Statistics
    ComponentTreeRenderer::RenderStats GetStats() const { return renderer.GetLastStats(); }
    
private:
    void BuildComponentTree();
    void OnRuntimeEvent(uint16_t elementId, const std::string& handler);
    
    UltraWebRuntime runtime;
    ElementFactory factory;
    ComponentTreeRenderer renderer;
    JSEventCallback jsCallback;
    
    float viewportWidth;
    float viewportHeight;
    bool needsLayout;
    bool needsRender;
};

} // namespace Runtime
} // namespace UltraWeb
