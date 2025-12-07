// UltraWeb/runtime/ElementFactory.cpp
// Element Factory Implementation
// Version: 1.0.0

#include "ElementFactory.h"
#include <sstream>
#include <chrono>
#include <algorithm>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// STYLE APPLICATOR IMPLEMENTATION
// ============================================================================

void StyleApplicator::Apply(UCComponent* component, const ComputedStyle& style) {
    if (!component) return;
    
    ApplyLayout(component, style);
    ApplyVisual(component, style);
    
    // Type-specific styling
    if (auto* text = dynamic_cast<UCText*>(component)) {
        ApplyText(text, style);
    }
    if (auto* container = dynamic_cast<UCContainer*>(component)) {
        ApplyContainer(container, style);
    }
}

void StyleApplicator::ApplyLayout(UCComponent* component, const ComputedStyle& style) {
    // Padding
    component->SetPadding(
        style.paddingTop,
        style.paddingRight,
        style.paddingBottom,
        style.paddingLeft
    );
    
    // Margin
    component->SetMargin(UCEdgeInsets(
        style.marginTop,
        style.marginRight,
        style.marginBottom,
        style.marginLeft
    ));
    
    // Size (only if explicitly set)
    UCRect frame = component->GetFrame();
    if (style.width > 0) {
        frame.width = style.width;
    }
    if (style.height > 0) {
        frame.height = style.height;
    }
    component->SetFrame(frame);
}

void StyleApplicator::ApplyVisual(UCComponent* component, const ComputedStyle& style) {
    // Background
    if (style.backgroundColor.a > 0) {
        component->SetBackgroundColor(UCColor(style.backgroundColor));
    }
    
    // Border
    if (style.borderWidth > 0) {
        component->SetBorderWidth(style.borderWidth);
        component->SetBorderColor(UCColor(style.borderColor));
    }
    
    if (style.borderRadius > 0) {
        component->SetBorderRadius(style.borderRadius);
    }
    
    // Opacity
    component->SetOpacity(style.opacity);
    
    // Visibility
    component->SetVisible(style.visibility != 0);
}

void StyleApplicator::ApplyText(UCText* text, const ComputedStyle& style) {
    text->SetTextColor(UCColor(style.color));
    text->SetFontFamily(style.fontFamily);
    text->SetFontSize(style.fontSize);
    text->SetFontWeight(style.fontWeight);
    text->SetLineHeight(style.lineHeight);
    text->SetTextAlign(style.textAlign);
}

void StyleApplicator::ApplyContainer(UCContainer* container, const ComputedStyle& style) {
    // Flex direction
    switch (style.flexDirection) {
        case 0:  // row
            container->SetLayoutDirection(UCContainer::LayoutDirection::Row);
            break;
        case 1:  // column
            container->SetLayoutDirection(UCContainer::LayoutDirection::Column);
            break;
        case 2:  // row-reverse
            container->SetLayoutDirection(UCContainer::LayoutDirection::RowReverse);
            break;
        case 3:  // column-reverse
            container->SetLayoutDirection(UCContainer::LayoutDirection::ColumnReverse);
            break;
    }
    
    // Justify content
    switch (style.justifyContent) {
        case 0: container->SetJustifyContent(UCContainer::JustifyContent::FlexStart); break;
        case 1: container->SetJustifyContent(UCContainer::JustifyContent::FlexEnd); break;
        case 2: container->SetJustifyContent(UCContainer::JustifyContent::Center); break;
        case 3: container->SetJustifyContent(UCContainer::JustifyContent::SpaceBetween); break;
        case 4: container->SetJustifyContent(UCContainer::JustifyContent::SpaceAround); break;
        case 5: container->SetJustifyContent(UCContainer::JustifyContent::SpaceEvenly); break;
    }
    
    // Align items
    switch (style.alignItems) {
        case 0: container->SetAlignItems(UCContainer::AlignItems::FlexStart); break;
        case 1: container->SetAlignItems(UCContainer::AlignItems::FlexEnd); break;
        case 2: container->SetAlignItems(UCContainer::AlignItems::Center); break;
        case 3: container->SetAlignItems(UCContainer::AlignItems::Stretch); break;
        case 4: container->SetAlignItems(UCContainer::AlignItems::Baseline); break;
    }
    
    // Gap
    container->SetGap(style.gap);
    
    // Wrap
    container->SetWrap(style.flexWrap != 0);
}

// ============================================================================
// ELEMENT FACTORY IMPLEMENTATION
// ============================================================================

ElementFactory::ElementFactory()
    : currentRuntime(nullptr) {
}

ElementFactory::~ElementFactory() {
    Clear();
}

std::shared_ptr<UCComponent> ElementFactory::CreateComponent(const RuntimeElement& element) {
    // Check for custom creator
    auto customIt = customCreators.find(element.type);
    if (customIt != customCreators.end()) {
        auto component = customIt->second(element);
        if (component) {
            PopulateComponent(component.get(), element);
            return component;
        }
    }
    
    // Create by type
    auto component = CreateByType(element.type);
    if (component) {
        PopulateComponent(component.get(), element);
    }
    
    return component;
}

std::shared_ptr<UCComponent> ElementFactory::CreateByType(UCBElementType type) {
    switch (type) {
        // Containers
        case UCBElementType::Container:
            return std::make_shared<UCContainer>();
        case UCBElementType::FlexBox:
            return std::make_shared<UCFlexBox>();
        case UCBElementType::ScrollView:
            return std::make_shared<UCScrollView>();
        case UCBElementType::Grid:
            return std::make_shared<UCContainer>();  // Grid as container for now
            
        // Text
        case UCBElementType::Text:
            return std::make_shared<UCText>();
            
        // Interactive
        case UCBElementType::Button:
            return std::make_shared<UCButton>();
        case UCBElementType::Input:
            return std::make_shared<UCInput>();
        case UCBElementType::TextArea:
            return std::make_shared<UCInput>();  // TextArea as Input for now
        case UCBElementType::Checkbox:
            return std::make_shared<UCCheckbox>();
        case UCBElementType::Radio:
            return std::make_shared<UCCheckbox>();  // Radio as Checkbox for now
        case UCBElementType::Select:
            return std::make_shared<UCContainer>();  // Select as container for now
        case UCBElementType::Slider:
            return std::make_shared<UCComponent>();  // Basic for now
            
        // Media
        case UCBElementType::Image:
            return std::make_shared<UCImage>();
        case UCBElementType::SVG:
            return std::make_shared<UCComponent>();
        case UCBElementType::Canvas:
            return std::make_shared<UCComponent>();
        case UCBElementType::Video:
            return std::make_shared<UCComponent>();
        case UCBElementType::Audio:
            return std::make_shared<UCComponent>();
            
        // Data display
        case UCBElementType::List:
            return std::make_shared<UCList>();
        case UCBElementType::Table:
            return std::make_shared<UCContainer>();
        case UCBElementType::Tree:
            return std::make_shared<UCList>();
            
        // Complex
        case UCBElementType::Tabs:
        case UCBElementType::Modal:
        case UCBElementType::Menu:
        case UCBElementType::Tooltip:
        case UCBElementType::Popover:
            return std::make_shared<UCContainer>();
            
        default:
            return std::make_shared<UCComponent>();
    }
}

void ElementFactory::PopulateComponent(UCComponent* component, const RuntimeElement& element) {
    component->SetId(element.elementId);
    component->SetName(element.id);
    
    // Set visibility/enabled
    component->SetVisible(element.visible);
    component->SetEnabled(element.enabled);
    
    // Apply computed style
    StyleApplicator::Apply(component, element.computedStyle);
    
    // Set frame from layout
    component->SetFrame(element.x, element.y, element.layoutWidth, element.layoutHeight);
    
    // Type-specific population
    if (auto* text = dynamic_cast<UCText*>(component)) {
        text->SetText(element.textContent);
    }
    
    if (auto* button = dynamic_cast<UCButton*>(component)) {
        if (!element.textContent.empty()) {
            button->SetText(element.textContent);
        }
    }
    
    if (auto* input = dynamic_cast<UCInput*>(component)) {
        input->SetPlaceholder(element.GetStringProp(UCBPropertyId::Placeholder, ""));
        input->SetValue(element.GetStringProp(UCBPropertyId::Value, ""));
        input->SetInputType(element.GetStringProp(UCBPropertyId::Type, "text"));
    }
    
    if (auto* checkbox = dynamic_cast<UCCheckbox*>(component)) {
        checkbox->SetChecked(element.GetBoolProp(UCBPropertyId::Checked, false));
    }
    
    if (auto* image = dynamic_cast<UCImage*>(component)) {
        image->SetSource(element.GetStringProp(UCBPropertyId::Src, ""));
        image->SetAltText(element.GetStringProp(UCBPropertyId::Alt, ""));
    }
    
    // Setup event handlers
    SetupEventHandlers(component, element);
    
    // Store mapping
    elementToComponent[element.elementId] = std::shared_ptr<UCComponent>(component, [](UCComponent*){});
    componentToElement[component] = element.elementId;
}

void ElementFactory::SetupEventHandlers(UCComponent* component, const RuntimeElement& element) {
    // Map UCBPropertyId event handlers to UCComponent events
    static const std::pair<UCBPropertyId, UCEventType> eventMap[] = {
        {UCBPropertyId::OnClick, UCEventType::Click},
        {UCBPropertyId::OnChange, UCEventType::Change},
        {UCBPropertyId::OnInput, UCEventType::Input},
        {UCBPropertyId::OnFocus, UCEventType::Focus},
        {UCBPropertyId::OnBlur, UCEventType::Blur},
        {UCBPropertyId::OnMouseEnter, UCEventType::MouseEnter},
        {UCBPropertyId::OnMouseLeave, UCEventType::MouseLeave},
        {UCBPropertyId::OnKeyDown, UCEventType::KeyDown},
        {UCBPropertyId::OnKeyUp, UCEventType::KeyUp},
    };
    
    for (const auto& [propId, eventType] : eventMap) {
        if (element.HasEventHandler(propId)) {
            const std::string& handlerName = element.GetEventHandler(propId);
            uint16_t elemId = element.elementId;
            UltraWebRuntime* runtime = currentRuntime;
            
            component->SetEventHandler(eventType, [runtime, elemId, handlerName, propId](const UCEvent& event) {
                (void)event;
                if (runtime) {
                    runtime->DispatchEvent(elemId, propId);
                }
            });
        }
    }
}

std::shared_ptr<UCComponent> ElementFactory::BuildComponentTree(UltraWebRuntime& runtime) {
    currentRuntime = &runtime;
    Clear();
    
    const auto& elements = runtime.GetElements();
    if (elements.empty()) return nullptr;
    
    // First pass: create all components
    for (const auto& elem : elements) {
        auto component = CreateComponent(elem);
        elementToComponent[elem.elementId] = component;
        componentToElement[component.get()] = elem.elementId;
    }
    
    // Second pass: build hierarchy
    for (const auto& elem : elements) {
        if (elem.parentId != 0) {
            auto parentIt = elementToComponent.find(elem.parentId);
            auto childIt = elementToComponent.find(elem.elementId);
            
            if (parentIt != elementToComponent.end() && childIt != elementToComponent.end()) {
                parentIt->second->AddChild(childIt->second);
            }
        }
    }
    
    // Find root (element with parentId == 0)
    for (const auto& elem : elements) {
        if (elem.parentId == 0) {
            auto it = elementToComponent.find(elem.elementId);
            if (it != elementToComponent.end()) {
                return it->second;
            }
        }
    }
    
    // Return first element if no explicit root
    if (!elementToComponent.empty()) {
        return elementToComponent.begin()->second;
    }
    
    return nullptr;
}

UCComponent* ElementFactory::GetComponent(uint16_t elementId) {
    auto it = elementToComponent.find(elementId);
    return (it != elementToComponent.end()) ? it->second.get() : nullptr;
}

uint16_t ElementFactory::GetElementId(UCComponent* component) {
    auto it = componentToElement.find(component);
    return (it != componentToElement.end()) ? it->second : 0;
}

void ElementFactory::UpdateComponent(UCComponent* component, const RuntimeElement& element) {
    if (!component) return;
    PopulateComponent(component, element);
}

void ElementFactory::ApplyStyles(UCComponent* root) {
    if (!root || !currentRuntime) return;
    
    // Get element ID
    uint16_t elemId = GetElementId(root);
    RuntimeElement* elem = currentRuntime->GetElement(elemId);
    
    if (elem) {
        StyleApplicator::Apply(root, elem->computedStyle);
    }
    
    // Recurse to children
    for (auto& child : root->GetChildren()) {
        ApplyStyles(child.get());
    }
}

void ElementFactory::Clear() {
    elementToComponent.clear();
    componentToElement.clear();
    currentRuntime = nullptr;
}

void ElementFactory::RegisterCreator(UCBElementType type, ComponentCreator creator) {
    customCreators[type] = creator;
}

std::string ElementFactory::DumpComponentTree(UCComponent* root, int depth) {
    if (!root) return "";
    
    std::ostringstream oss;
    std::string indent(depth * 2, ' ');
    
    oss << indent << root->DebugDescription() << "\n";
    
    for (auto& child : root->GetChildren()) {
        oss << DumpComponentTree(child.get(), depth + 1);
    }
    
    return oss.str();
}

// ============================================================================
// COMPONENT TREE RENDERER IMPLEMENTATION
// ============================================================================

ComponentTreeRenderer::ComponentTreeRenderer()
    : hoveredComponent(nullptr)
    , pressedComponent(nullptr)
    , focusedComponent(nullptr) {
    lastStats = {0, 0, 0, 0};
}

void ComponentTreeRenderer::SetRoot(std::shared_ptr<UCComponent> root) {
    rootComponent = root;
    hoveredComponent = nullptr;
    pressedComponent = nullptr;
    focusedComponent = nullptr;
}

void ComponentTreeRenderer::Render(UCRenderContext& ctx) {
    if (!rootComponent) return;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    rootComponent->Render(ctx);
    
    auto end = std::chrono::high_resolution_clock::now();
    lastStats.renderTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Count components and draw calls
    lastStats.componentCount = 0;
    std::function<void(UCComponent*)> countComponents = [&](UCComponent* comp) {
        if (comp) {
            lastStats.componentCount++;
            for (auto& child : comp->GetChildren()) {
                countComponents(child.get());
            }
        }
    };
    countComponents(rootComponent.get());
    
    if (auto* mockCtx = dynamic_cast<UCMockRenderContext*>(&ctx)) {
        lastStats.drawCalls = static_cast<int>(mockCtx->GetDrawCallCount());
    }
}

void ComponentTreeRenderer::PerformLayout(float width, float height) {
    if (!rootComponent) return;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    rootComponent->SetFrame(0, 0, width, height);
    rootComponent->Layout();
    
    auto end = std::chrono::high_resolution_clock::now();
    lastStats.layoutTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
}

UCComponent* ComponentTreeRenderer::HitTest(float x, float y) {
    if (!rootComponent) return nullptr;
    return rootComponent->HitTest(x, y);
}

void ComponentTreeRenderer::HandleMouseMove(float x, float y) {
    UCComponent* hit = HitTest(x, y);
    
    if (hit != hoveredComponent) {
        if (hoveredComponent) {
            hoveredComponent->SetHovered(false);
            DispatchEvent(hoveredComponent, UCEventType::MouseLeave, x, y);
        }
        
        hoveredComponent = hit;
        
        if (hoveredComponent) {
            hoveredComponent->SetHovered(true);
            DispatchEvent(hoveredComponent, UCEventType::MouseEnter, x, y);
        }
    }
}

void ComponentTreeRenderer::HandleMouseDown(float x, float y, int button) {
    (void)button;
    UCComponent* hit = HitTest(x, y);
    
    if (hit) {
        pressedComponent = hit;
        hit->SetPressed(true);
        DispatchEvent(hit, UCEventType::MouseDown, x, y);
    }
}

void ComponentTreeRenderer::HandleMouseUp(float x, float y, int button) {
    (void)button;
    if (pressedComponent) {
        pressedComponent->SetPressed(false);
        DispatchEvent(pressedComponent, UCEventType::MouseUp, x, y);
        pressedComponent = nullptr;
    }
}

void ComponentTreeRenderer::HandleClick(float x, float y) {
    UCComponent* hit = HitTest(x, y);
    
    if (hit) {
        DispatchEvent(hit, UCEventType::Click, x, y);
        SetFocused(hit);
    } else {
        SetFocused(nullptr);
    }
}

void ComponentTreeRenderer::HandleKeyDown(int keyCode, const std::string& key) {
    if (focusedComponent) {
        UCEvent event;
        event.type = UCEventType::KeyDown;
        event.keyCode = keyCode;
        event.key = key;
        event.target = focusedComponent;
        focusedComponent->DispatchEvent(event);
    }
}

void ComponentTreeRenderer::HandleKeyUp(int keyCode, const std::string& key) {
    if (focusedComponent) {
        UCEvent event;
        event.type = UCEventType::KeyUp;
        event.keyCode = keyCode;
        event.key = key;
        event.target = focusedComponent;
        focusedComponent->DispatchEvent(event);
    }
}

void ComponentTreeRenderer::SetFocused(UCComponent* component) {
    if (focusedComponent != component) {
        if (focusedComponent) {
            focusedComponent->SetFocused(false);
            DispatchEvent(focusedComponent, UCEventType::Blur, 0, 0);
        }
        
        focusedComponent = component;
        
        if (focusedComponent) {
            focusedComponent->SetFocused(true);
            DispatchEvent(focusedComponent, UCEventType::Focus, 0, 0);
        }
    }
}

void ComponentTreeRenderer::DispatchEvent(UCComponent* target, UCEventType type, float x, float y) {
    if (!target) return;
    
    UCEvent event;
    event.type = type;
    event.x = x;
    event.y = y;
    event.target = target;
    target->DispatchEvent(event);
}

// ============================================================================
// ULTRAWEB BRIDGE IMPLEMENTATION
// ============================================================================

UltraWebBridge::UltraWebBridge()
    : viewportWidth(800)
    , viewportHeight(600)
    , needsLayout(true)
    , needsRender(true) {
    
    // Set up runtime event callback
    runtime.SetEventCallback([this](uint16_t elementId, const std::string& handler) {
        OnRuntimeEvent(elementId, handler);
    });
}

UltraWebBridge::~UltraWebBridge() {
}

bool UltraWebBridge::LoadPackage(const std::vector<uint8_t>& data) {
    return LoadPackage(data.data(), data.size());
}

bool UltraWebBridge::LoadPackage(const uint8_t* data, size_t size) {
    auto result = runtime.LoadPackage(data, size);
    if (!result.success) {
        return false;
    }
    
    // Compute styles and layout in runtime
    runtime.RecomputeStyles();
    runtime.PerformLayout(viewportWidth, viewportHeight);
    
    // Build component tree
    BuildComponentTree();
    
    needsLayout = false;
    needsRender = true;
    
    return true;
}

void UltraWebBridge::BuildComponentTree() {
    auto root = factory.BuildComponentTree(runtime);
    renderer.SetRoot(root);
    
    if (root) {
        renderer.PerformLayout(viewportWidth, viewportHeight);
    }
}

void UltraWebBridge::SetViewport(float width, float height) {
    if (width != viewportWidth || height != viewportHeight) {
        viewportWidth = width;
        viewportHeight = height;
        needsLayout = true;
    }
}

void UltraWebBridge::Render(UCRenderContext& ctx) {
    if (needsLayout) {
        runtime.PerformLayout(viewportWidth, viewportHeight);
        
        // Update component positions from runtime
        for (const auto& elem : runtime.GetElements()) {
            UCComponent* comp = factory.GetComponent(elem.elementId);
            if (comp) {
                comp->SetFrame(elem.x, elem.y, elem.layoutWidth, elem.layoutHeight);
            }
        }
        
        renderer.PerformLayout(viewportWidth, viewportHeight);
        needsLayout = false;
    }
    
    renderer.Render(ctx);
    needsRender = false;
}

void UltraWebBridge::HandleMouseMove(float x, float y) {
    renderer.HandleMouseMove(x, y);
}

void UltraWebBridge::HandleMouseDown(float x, float y, int button) {
    renderer.HandleMouseDown(x, y, button);
}

void UltraWebBridge::HandleMouseUp(float x, float y, int button) {
    renderer.HandleMouseUp(x, y, button);
}

void UltraWebBridge::HandleClick(float x, float y) {
    renderer.HandleClick(x, y);
}

void UltraWebBridge::HandleKeyDown(int keyCode, const std::string& key) {
    renderer.HandleKeyDown(keyCode, key);
}

void UltraWebBridge::HandleKeyUp(int keyCode, const std::string& key) {
    renderer.HandleKeyUp(keyCode, key);
}

void UltraWebBridge::SetJSEventCallback(JSEventCallback callback) {
    jsCallback = callback;
}

void UltraWebBridge::OnRuntimeEvent(uint16_t elementId, const std::string& handler) {
    if (jsCallback) {
        UCEvent event;
        event.type = UCEventType::Click;  // Generic
        event.target = factory.GetComponent(elementId);
        jsCallback(elementId, handler, event);
    }
}

void UltraWebBridge::SetElementText(uint16_t elementId, const std::string& text) {
    runtime.SetElementText(elementId, text);
    
    UCComponent* comp = factory.GetComponent(elementId);
    if (auto* textComp = dynamic_cast<UCText*>(comp)) {
        textComp->SetText(text);
    }
    
    needsRender = true;
}

void UltraWebBridge::SetElementValue(uint16_t elementId, const std::string& value) {
    runtime.SetElementValue(elementId, value);
    
    UCComponent* comp = factory.GetComponent(elementId);
    if (auto* input = dynamic_cast<UCInput*>(comp)) {
        input->SetValue(value);
    }
    
    needsRender = true;
}

void UltraWebBridge::SetElementVisible(uint16_t elementId, bool visible) {
    runtime.SetElementVisible(elementId, visible);
    
    UCComponent* comp = factory.GetComponent(elementId);
    if (comp) {
        comp->SetVisible(visible);
    }
    
    needsLayout = true;
    needsRender = true;
}

void UltraWebBridge::Invalidate() {
    needsLayout = true;
    needsRender = true;
}

} // namespace Runtime
} // namespace UltraWeb
