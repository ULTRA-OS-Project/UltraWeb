// UltraWeb/runtime/UCComponents.cpp
// Mock UltraCanvas Components Implementation
// Version: 1.0.0

#include "UCComponents.h"
#include <sstream>
#include <algorithm>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// BASE COMPONENT IMPLEMENTATION
// ============================================================================

UCComponent::UCComponent()
    : componentId(0)
    , parent(nullptr)
    , borderWidth(0)
    , borderRadius(0)
    , opacity(1.0f)
    , visible(true)
    , enabled(true)
    , focused(false)
    , hovered(false)
    , pressed(false) {
    backgroundColor = UCColor(255, 255, 255, 0);  // Transparent
    borderColor = UCColor(0, 0, 0, 0);
}

void UCComponent::AddChild(std::shared_ptr<UCComponent> child) {
    child->SetParent(this);
    children.push_back(child);
}

void UCComponent::RemoveChild(UCComponent* child) {
    auto it = std::find_if(children.begin(), children.end(),
        [child](const std::shared_ptr<UCComponent>& c) { return c.get() == child; });
    if (it != children.end()) {
        (*it)->SetParent(nullptr);
        children.erase(it);
    }
}

void UCComponent::ClearChildren() {
    for (auto& child : children) {
        child->SetParent(nullptr);
    }
    children.clear();
}

void UCComponent::SetEventHandler(UCEventType type, UCEventHandler handler) {
    eventHandlers[type] = handler;
}

void UCComponent::RemoveEventHandler(UCEventType type) {
    eventHandlers.erase(type);
}

bool UCComponent::HasEventHandler(UCEventType type) const {
    return eventHandlers.find(type) != eventHandlers.end();
}

void UCComponent::DispatchEvent(const UCEvent& event) {
    auto it = eventHandlers.find(event.type);
    if (it != eventHandlers.end()) {
        it->second(event);
    }
}

void UCComponent::Render(UCRenderContext& ctx) {
    if (!visible) return;
    
    ctx.Save();
    ctx.Translate(frame.x, frame.y);
    
    if (opacity < 1.0f) {
        // In real implementation, would set global alpha
    }
    
    RenderBackground(ctx);
    RenderContent(ctx);
    RenderBorder(ctx);
    RenderChildren(ctx);
    
    ctx.Restore();
}

void UCComponent::RenderBackground(UCRenderContext& ctx) {
    if (backgroundColor.a == 0) return;
    
    ctx.SetFillColor(backgroundColor);
    UCRect contentRect = {0, 0, frame.width, frame.height};
    
    if (borderRadius > 0) {
        ctx.FillRoundedRect(contentRect, borderRadius);
    } else {
        ctx.FillRect(contentRect);
    }
}

void UCComponent::RenderBorder(UCRenderContext& ctx) {
    if (borderWidth <= 0 || borderColor.a == 0) return;
    
    ctx.SetStrokeColor(borderColor);
    ctx.SetLineWidth(borderWidth);
    UCRect borderRect = {0, 0, frame.width, frame.height};
    
    if (borderRadius > 0) {
        ctx.StrokeRoundedRect(borderRect, borderRadius);
    } else {
        ctx.StrokeRect(borderRect);
    }
}

void UCComponent::RenderContent(UCRenderContext& ctx) {
    // Base class has no content
    (void)ctx;
}

void UCComponent::RenderChildren(UCRenderContext& ctx) {
    for (auto& child : children) {
        child->Render(ctx);
    }
}

UCComponent* UCComponent::HitTest(float x, float y) {
    if (!visible || !enabled) return nullptr;
    
    // Transform to local coordinates
    float localX = x - frame.x;
    float localY = y - frame.y;
    
    // Check if within bounds
    if (localX < 0 || localX >= frame.width || 
        localY < 0 || localY >= frame.height) {
        return nullptr;
    }
    
    // Check children in reverse order (front to back)
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        UCComponent* hit = (*it)->HitTest(localX, localY);
        if (hit) return hit;
    }
    
    return this;
}

void UCComponent::Layout() {
    // Default layout: just layout children
    for (auto& child : children) {
        child->Layout();
    }
}

UCRect UCComponent::GetContentBounds() const {
    return UCRect(
        padding.left,
        padding.top,
        frame.width - padding.left - padding.right,
        frame.height - padding.top - padding.bottom
    );
}

std::string UCComponent::DebugDescription() const {
    std::ostringstream oss;
    oss << GetTypeName();
    if (!name.empty()) {
        oss << " #" << name;
    }
    oss << " [" << componentId << "]";
    oss << " frame=(" << frame.x << "," << frame.y 
        << " " << frame.width << "x" << frame.height << ")";
    return oss.str();
}

// ============================================================================
// CONTAINER IMPLEMENTATION
// ============================================================================

UCContainer::UCContainer()
    : layoutDirection(LayoutDirection::Column)
    , justifyContent(JustifyContent::FlexStart)
    , alignItems(AlignItems::Stretch)
    , gap(0)
    , wrap(false) {
}

void UCContainer::Layout() {
    if (children.empty()) return;
    
    UCRect content = GetContentBounds();
    float currentX = content.x;
    float currentY = content.y;
    
    bool isRow = (layoutDirection == LayoutDirection::Row || 
                  layoutDirection == LayoutDirection::RowReverse);
    bool isReverse = (layoutDirection == LayoutDirection::RowReverse ||
                      layoutDirection == LayoutDirection::ColumnReverse);
    
    // Calculate total size needed
    float totalMainSize = 0;
    float maxCrossSize = 0;
    int visibleCount = 0;
    
    for (auto& child : children) {
        if (!child->IsVisible()) continue;
        visibleCount++;
        
        if (isRow) {
            totalMainSize += child->GetFrame().width;
            maxCrossSize = std::max(maxCrossSize, child->GetFrame().height);
        } else {
            totalMainSize += child->GetFrame().height;
            maxCrossSize = std::max(maxCrossSize, child->GetFrame().width);
        }
    }
    
    totalMainSize += gap * (visibleCount - 1);
    
    // Calculate starting position based on justify-content
    float availableMain = isRow ? content.width : content.height;
    float mainStart = 0;
    float gapSize = gap;
    
    switch (justifyContent) {
        case JustifyContent::FlexStart:
            mainStart = 0;
            break;
        case JustifyContent::FlexEnd:
            mainStart = availableMain - totalMainSize;
            break;
        case JustifyContent::Center:
            mainStart = (availableMain - totalMainSize) / 2;
            break;
        case JustifyContent::SpaceBetween:
            if (visibleCount > 1) {
                gapSize = (availableMain - totalMainSize + gap * (visibleCount - 1)) / (visibleCount - 1);
            }
            break;
        case JustifyContent::SpaceAround:
            if (visibleCount > 0) {
                float space = (availableMain - totalMainSize + gap * (visibleCount - 1)) / visibleCount;
                mainStart = space / 2;
                gapSize = space;
            }
            break;
        case JustifyContent::SpaceEvenly:
            if (visibleCount > 0) {
                float space = (availableMain - totalMainSize + gap * (visibleCount - 1)) / (visibleCount + 1);
                mainStart = space;
                gapSize = space;
            }
            break;
    }
    
    if (isRow) {
        currentX = content.x + mainStart;
    } else {
        currentY = content.y + mainStart;
    }
    
    // Position children
    std::vector<std::shared_ptr<UCComponent>> orderedChildren = children;
    if (isReverse) {
        std::reverse(orderedChildren.begin(), orderedChildren.end());
    }
    
    for (auto& child : orderedChildren) {
        if (!child->IsVisible()) continue;
        
        UCRect childFrame = child->GetFrame();
        float crossSize = isRow ? content.height : content.width;
        
        // Align item
        float crossPos = 0;
        float childCross = isRow ? childFrame.height : childFrame.width;
        
        switch (alignItems) {
            case AlignItems::FlexStart:
                crossPos = 0;
                break;
            case AlignItems::FlexEnd:
                crossPos = crossSize - childCross;
                break;
            case AlignItems::Center:
                crossPos = (crossSize - childCross) / 2;
                break;
            case AlignItems::Stretch:
                childCross = crossSize;
                crossPos = 0;
                break;
            case AlignItems::Baseline:
                crossPos = 0;  // Simplified
                break;
        }
        
        if (isRow) {
            childFrame.x = currentX;
            childFrame.y = content.y + crossPos;
            if (alignItems == AlignItems::Stretch) {
                childFrame.height = crossSize;
            }
            currentX += childFrame.width + gapSize;
        } else {
            childFrame.x = content.x + crossPos;
            childFrame.y = currentY;
            if (alignItems == AlignItems::Stretch) {
                childFrame.width = crossSize;
            }
            currentY += childFrame.height + gapSize;
        }
        
        child->SetFrame(childFrame);
        child->Layout();
    }
}

// ============================================================================
// SCROLL VIEW IMPLEMENTATION
// ============================================================================

UCScrollView::UCScrollView()
    : scrollX(0)
    , scrollY(0)
    , contentWidth(0)
    , contentHeight(0) {
}

void UCScrollView::Render(UCRenderContext& ctx) {
    if (!visible) return;
    
    ctx.Save();
    ctx.Translate(frame.x, frame.y);
    
    RenderBackground(ctx);
    
    // Set clip region
    ctx.SetClipRect({0, 0, frame.width, frame.height});
    
    // Translate by scroll offset
    ctx.Translate(-scrollX, -scrollY);
    
    RenderChildren(ctx);
    
    ctx.Restore();
    
    // Render border on top
    ctx.Save();
    ctx.Translate(frame.x, frame.y);
    RenderBorder(ctx);
    ctx.Restore();
}

// ============================================================================
// TEXT IMPLEMENTATION
// ============================================================================

UCText::UCText()
    : fontFamily("sans-serif")
    , fontSize(16)
    , fontWeight(400)
    , lineHeight(1.2f)
    , textAlign(0) {
    textColor = UCColor(0, 0, 0);
}

UCText::UCText(const std::string& t)
    : UCText() {
    text = t;
}

void UCText::RenderContent(UCRenderContext& ctx) {
    if (text.empty()) return;
    
    ctx.SetFillColor(textColor);
    ctx.SetFont(fontFamily, fontSize, fontWeight);
    ctx.SetTextAlign(textAlign);
    
    float textX = padding.left;
    float textY = padding.top + fontSize;  // Baseline
    
    if (textAlign == 1) {  // Center
        textX = frame.width / 2;
    } else if (textAlign == 2) {  // Right
        textX = frame.width - padding.right;
    }
    
    ctx.FillText(text, textX, textY);
}

// ============================================================================
// BUTTON IMPLEMENTATION
// ============================================================================

UCButton::UCButton() : UCText() {
    backgroundColor = UCColor(76, 175, 80);  // Green
    textColor = UCColor(255, 255, 255);
    hoverColor = UCColor(69, 160, 73);
    pressedColor = UCColor(56, 142, 60);
    borderRadius = 4;
    padding = UCEdgeInsets(8, 16, 8, 16);
}

UCButton::UCButton(const std::string& label) : UCButton() {
    text = label;
}

void UCButton::Render(UCRenderContext& ctx) {
    if (!visible) return;
    
    ctx.Save();
    ctx.Translate(frame.x, frame.y);
    
    // Use state-dependent background color
    UCColor bgColor = backgroundColor;
    if (pressed) {
        bgColor = pressedColor;
    } else if (hovered) {
        bgColor = hoverColor;
    }
    
    ctx.SetFillColor(bgColor);
    UCRect bgRect = {0, 0, frame.width, frame.height};
    
    if (borderRadius > 0) {
        ctx.FillRoundedRect(bgRect, borderRadius);
    } else {
        ctx.FillRect(bgRect);
    }
    
    RenderContent(ctx);
    RenderBorder(ctx);
    
    ctx.Restore();
}

// ============================================================================
// INPUT IMPLEMENTATION
// ============================================================================

UCInput::UCInput()
    : fontFamily("sans-serif")
    , fontSize(16) {
    textColor = UCColor(0, 0, 0);
    placeholderColor = UCColor(128, 128, 128);
    backgroundColor = UCColor(255, 255, 255);
    borderColor = UCColor(200, 200, 200);
    borderWidth = 1;
    borderRadius = 4;
    padding = UCEdgeInsets(8, 12, 8, 12);
    inputType = "text";
}

void UCInput::RenderContent(UCRenderContext& ctx) {
    std::string displayText = value.empty() ? placeholder : value;
    UCColor displayColor = value.empty() ? placeholderColor : textColor;
    
    if (displayText.empty()) return;
    
    // Mask password
    if (inputType == "password" && !value.empty()) {
        displayText = std::string(value.length(), '*');
    }
    
    ctx.SetFillColor(displayColor);
    ctx.SetFont(fontFamily, fontSize, 400);
    ctx.FillText(displayText, padding.left, padding.top + fontSize);
}

// ============================================================================
// CHECKBOX IMPLEMENTATION
// ============================================================================

UCCheckbox::UCCheckbox()
    : checked(false) {
    frame = {0, 0, 20, 20};
}

void UCCheckbox::RenderContent(UCRenderContext& ctx) {
    // Draw checkbox box
    UCRect boxRect = {0, 0, 20, 20};
    
    ctx.SetFillColor(UCColor(255, 255, 255));
    ctx.FillRect(boxRect);
    
    ctx.SetStrokeColor(UCColor(100, 100, 100));
    ctx.SetLineWidth(2);
    ctx.StrokeRect(boxRect);
    
    // Draw checkmark if checked
    if (checked) {
        ctx.SetFillColor(UCColor(76, 175, 80));
        ctx.FillRect({4, 4, 12, 12});
    }
    
    // Draw label
    if (!label.empty()) {
        ctx.SetFillColor(UCColor(0, 0, 0));
        ctx.SetFont("sans-serif", 16, 400);
        ctx.FillText(label, 28, 16);
    }
}

// ============================================================================
// IMAGE IMPLEMENTATION
// ============================================================================

UCImage::UCImage()
    : imageId(-1) {
}

void UCImage::RenderContent(UCRenderContext& ctx) {
    if (imageId >= 0) {
        ctx.DrawImage(imageId, {0, 0, frame.width, frame.height});
    } else {
        // Placeholder
        ctx.SetFillColor(UCColor(200, 200, 200));
        ctx.FillRect({0, 0, frame.width, frame.height});
        
        if (!altText.empty()) {
            ctx.SetFillColor(UCColor(100, 100, 100));
            ctx.SetFont("sans-serif", 12, 400);
            ctx.FillText(altText, 4, frame.height / 2);
        }
    }
}

// ============================================================================
// LIST IMPLEMENTATION
// ============================================================================

UCList::UCList() {
    layoutDirection = LayoutDirection::Column;
    gap = 0;
}

UCListItem::UCListItem() {
    padding = UCEdgeInsets(8, 12, 8, 12);
}

} // namespace Runtime
} // namespace UltraWeb
