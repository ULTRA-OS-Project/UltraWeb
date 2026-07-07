// UltraWeb/runtime/UCComponents.h
// Mock UltraCanvas Components for Element Factory Integration
// Version: 1.0.0
// 
// These are stub implementations that define the interface UltraCanvas
// components would provide. In production, these would be replaced by
// actual UltraCanvas library components.

#pragma once

#include "../include/UltraWebFormats.h"
#include "UCSLoader.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

class UCComponent;
class UCContainer;
class UCText;
class UCButton;
class UCInput;
class UCImage;

// ============================================================================
// COLOR AND GEOMETRY TYPES
// ============================================================================

struct UCColor {
    uint8_t r, g, b, a;
    
    UCColor() : r(0), g(0), b(0), a(255) {}
    UCColor(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}
    UCColor(const StyleColor& sc) : r(sc.r), g(sc.g), b(sc.b), a(sc.a) {}
    
    uint32_t ToRGBA() const {
        return (r << 24) | (g << 16) | (b << 8) | a;
    }
};

struct UCRect {
    float x, y, width, height;
    
    UCRect() : x(0), y(0), width(0), height(0) {}
    UCRect(float x_, float y_, float w_, float h_) 
        : x(x_), y(y_), width(w_), height(h_) {}
    
    bool Contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

struct UCEdgeInsets {
    float top, right, bottom, left;
    
    UCEdgeInsets() : top(0), right(0), bottom(0), left(0) {}
    UCEdgeInsets(float t, float r, float b, float l)
        : top(t), right(r), bottom(b), left(l) {}
    UCEdgeInsets(float all) : top(all), right(all), bottom(all), left(all) {}
};

// ============================================================================
// EVENT TYPES
// ============================================================================

enum class UCEventType {
    Click,
    MouseDown,
    MouseUp,
    MouseEnter,
    MouseLeave,
    Focus,
    Blur,
    Change,
    Input,
    KeyDown,
    KeyUp,
    Scroll
};

struct UCEvent {
    UCEventType type;
    float x, y;           // Mouse position
    int button;           // Mouse button (0=left, 1=middle, 2=right)
    int keyCode;          // Key code
    std::string key;      // Key string
    bool shiftKey;
    bool ctrlKey;
    bool altKey;
    bool metaKey;
    UCComponent* target;
    
    UCEvent() : type(UCEventType::Click), x(0), y(0), button(0), 
                keyCode(0), shiftKey(false), ctrlKey(false),
                altKey(false), metaKey(false), target(nullptr) {}
};

using UCEventHandler = std::function<void(const UCEvent&)>;

// ============================================================================
// RENDER CONTEXT (Mock Canvas API)
// ============================================================================

class UCRenderContext {
public:
    virtual ~UCRenderContext() = default;
    
    // State
    virtual void Save() = 0;
    virtual void Restore() = 0;
    
    // Transform
    virtual void Translate(float x, float y) = 0;
    virtual void Scale(float sx, float sy) = 0;
    virtual void Rotate(float angle) = 0;
    virtual void SetClipRect(const UCRect& rect) = 0;
    
    // Drawing
    virtual void SetFillColor(const UCColor& color) = 0;
    virtual void SetStrokeColor(const UCColor& color) = 0;
    virtual void SetLineWidth(float width) = 0;
    
    virtual void FillRect(const UCRect& rect) = 0;
    virtual void StrokeRect(const UCRect& rect) = 0;
    virtual void FillRoundedRect(const UCRect& rect, float radius) = 0;
    virtual void StrokeRoundedRect(const UCRect& rect, float radius) = 0;
    
    // Text
    virtual void SetFont(const std::string& family, float size, int weight = 400) = 0;
    virtual void SetTextAlign(int align) = 0;  // 0=left, 1=center, 2=right
    virtual void FillText(const std::string& text, float x, float y) = 0;
    virtual float MeasureTextWidth(const std::string& text) = 0;
    
    // Images
    virtual void DrawImage(int imageId, const UCRect& dest) = 0;
    virtual void DrawImage(int imageId, const UCRect& src, const UCRect& dest) = 0;
};

// ============================================================================
// MOCK RENDER CONTEXT (for testing)
// ============================================================================

class UCMockRenderContext : public UCRenderContext {
public:
    struct DrawCall {
        std::string type;
        UCRect rect;
        UCColor color;
        std::string text;
        float radius;
    };
    
    std::vector<DrawCall> drawCalls;
    
    void Save() override { drawCalls.push_back({"save", {}, {}, "", 0}); }
    void Restore() override { drawCalls.push_back({"restore", {}, {}, "", 0}); }
    
    void Translate(float x, float y) override {
        DrawCall dc; dc.type = "translate"; dc.rect = {x, y, 0, 0};
        drawCalls.push_back(dc);
    }
    void Scale(float sx, float sy) override {
        DrawCall dc; dc.type = "scale"; dc.rect = {sx, sy, 0, 0};
        drawCalls.push_back(dc);
    }
    void Rotate(float angle) override {
        DrawCall dc; dc.type = "rotate"; dc.rect = {angle, 0, 0, 0};
        drawCalls.push_back(dc);
    }
    void SetClipRect(const UCRect& rect) override {
        drawCalls.push_back({"clipRect", rect, {}, "", 0});
    }
    
    void SetFillColor(const UCColor& color) override {
        currentFill = color;
    }
    void SetStrokeColor(const UCColor& color) override {
        currentStroke = color;
    }
    void SetLineWidth(float width) override {
        currentLineWidth = width;
    }
    
    void FillRect(const UCRect& rect) override {
        drawCalls.push_back({"fillRect", rect, currentFill, "", 0});
    }
    void StrokeRect(const UCRect& rect) override {
        drawCalls.push_back({"strokeRect", rect, currentStroke, "", 0});
    }
    void FillRoundedRect(const UCRect& rect, float radius) override {
        drawCalls.push_back({"fillRoundedRect", rect, currentFill, "", radius});
    }
    void StrokeRoundedRect(const UCRect& rect, float radius) override {
        drawCalls.push_back({"strokeRoundedRect", rect, currentStroke, "", radius});
    }
    
    void SetFont(const std::string& family, float size, int weight) override {
        currentFontFamily = family;
        currentFontSize = size;
        currentFontWeight = weight;
    }
    void SetTextAlign(int align) override {
        currentTextAlign = align;
    }
    void FillText(const std::string& text, float x, float y) override {
        drawCalls.push_back({"fillText", {x, y, 0, 0}, currentFill, text, 0});
    }
    float MeasureTextWidth(const std::string& text) override {
        return text.length() * currentFontSize * 0.6f;  // Rough estimate
    }
    
    void DrawImage(int imageId, const UCRect& dest) override {
        DrawCall dc; dc.type = "drawImage"; dc.rect = dest;
        dc.text = std::to_string(imageId);
        drawCalls.push_back(dc);
    }
    void DrawImage(int imageId, const UCRect& src, const UCRect& dest) override {
        DrawImage(imageId, dest);
    }
    
    void Clear() { drawCalls.clear(); }
    size_t GetDrawCallCount() const { return drawCalls.size(); }
    
private:
    UCColor currentFill;
    UCColor currentStroke;
    float currentLineWidth = 1.0f;
    std::string currentFontFamily = "sans-serif";
    float currentFontSize = 16.0f;
    int currentFontWeight = 400;
    int currentTextAlign = 0;
};

// ============================================================================
// BASE COMPONENT
// ============================================================================

class UCComponent {
public:
    UCComponent();
    virtual ~UCComponent() = default;
    
    // Identity
    uint16_t GetId() const { return componentId; }
    void SetId(uint16_t id) { componentId = id; }
    
    const std::string& GetName() const { return name; }
    void SetName(const std::string& n) { name = n; }
    
    // Hierarchy
    UCComponent* GetParent() const { return parent; }
    void SetParent(UCComponent* p) { parent = p; }
    
    const std::vector<std::shared_ptr<UCComponent>>& GetChildren() const { return children; }
    void AddChild(std::shared_ptr<UCComponent> child);
    void RemoveChild(UCComponent* child);
    void ClearChildren();
    
    // Geometry
    const UCRect& GetFrame() const { return frame; }
    void SetFrame(const UCRect& f) { frame = f; }
    void SetFrame(float x, float y, float w, float h) { frame = {x, y, w, h}; }
    
    const UCRect& GetBounds() const { return bounds; }
    void SetBounds(const UCRect& b) { bounds = b; }
    
    // Styling
    void SetBackgroundColor(const UCColor& color) { backgroundColor = color; }
    const UCColor& GetBackgroundColor() const { return backgroundColor; }
    
    void SetBorderColor(const UCColor& color) { borderColor = color; }
    void SetBorderWidth(float width) { borderWidth = width; }
    void SetBorderRadius(float radius) { borderRadius = radius; }
    
    void SetPadding(const UCEdgeInsets& p) { padding = p; }
    void SetPadding(float top, float right, float bottom, float left) {
        padding = {top, right, bottom, left};
    }
    
    void SetMargin(const UCEdgeInsets& m) { margin = m; }
    
    void SetOpacity(float o) { opacity = o; }
    float GetOpacity() const { return opacity; }
    
    // Visibility
    bool IsVisible() const { return visible; }
    void SetVisible(bool v) { visible = v; }
    
    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool e) { enabled = e; }
    
    // State
    bool IsFocused() const { return focused; }
    void SetFocused(bool f) { focused = f; }
    
    bool IsHovered() const { return hovered; }
    void SetHovered(bool h) { hovered = h; }
    
    bool IsPressed() const { return pressed; }
    void SetPressed(bool p) { pressed = p; }
    
    // Events
    void SetEventHandler(UCEventType type, UCEventHandler handler);
    void RemoveEventHandler(UCEventType type);
    bool HasEventHandler(UCEventType type) const;
    void DispatchEvent(const UCEvent& event);
    
    // Rendering
    virtual void Render(UCRenderContext& ctx);
    virtual void RenderChildren(UCRenderContext& ctx);
    
    // Hit testing
    virtual UCComponent* HitTest(float x, float y);
    
    // Layout
    virtual void Layout();
    virtual UCRect GetContentBounds() const;
    
    // Debug
    virtual std::string GetTypeName() const { return "Component"; }
    std::string DebugDescription() const;
    
protected:
    virtual void RenderBackground(UCRenderContext& ctx);
    virtual void RenderBorder(UCRenderContext& ctx);
    virtual void RenderContent(UCRenderContext& ctx);
    
    uint16_t componentId;
    std::string name;
    UCComponent* parent;
    std::vector<std::shared_ptr<UCComponent>> children;
    
    UCRect frame;       // Position/size relative to parent
    UCRect bounds;      // Content bounds (usually 0,0,width,height)
    
    UCColor backgroundColor;
    UCColor borderColor;
    float borderWidth;
    float borderRadius;
    UCEdgeInsets padding;
    UCEdgeInsets margin;
    float opacity;
    
    bool visible;
    bool enabled;
    bool focused;
    bool hovered;
    bool pressed;
    
    std::unordered_map<UCEventType, UCEventHandler> eventHandlers;
};

// ============================================================================
// CONTAINER COMPONENTS
// ============================================================================

class UCContainer : public UCComponent {
public:
    enum class LayoutDirection { Row, Column, RowReverse, ColumnReverse };
    enum class JustifyContent { FlexStart, FlexEnd, Center, SpaceBetween, SpaceAround, SpaceEvenly };
    enum class AlignItems { FlexStart, FlexEnd, Center, Stretch, Baseline };
    
    UCContainer();
    
    void SetLayoutDirection(LayoutDirection dir) { layoutDirection = dir; }
    void SetJustifyContent(JustifyContent jc) { justifyContent = jc; }
    void SetAlignItems(AlignItems ai) { alignItems = ai; }
    void SetGap(float g) { gap = g; }
    void SetWrap(bool w) { wrap = w; }
    
    void Layout() override;
    std::string GetTypeName() const override { return "Container"; }
    
protected:
    LayoutDirection layoutDirection;
    JustifyContent justifyContent;
    AlignItems alignItems;
    float gap;
    bool wrap;
};

class UCFlexBox : public UCContainer {
public:
    UCFlexBox() { layoutDirection = LayoutDirection::Row; }
    std::string GetTypeName() const override { return "FlexBox"; }
};

class UCScrollView : public UCContainer {
public:
    UCScrollView();
    
    void SetScrollX(float x) { scrollX = x; }
    void SetScrollY(float y) { scrollY = y; }
    float GetScrollX() const { return scrollX; }
    float GetScrollY() const { return scrollY; }
    
    void Render(UCRenderContext& ctx) override;
    std::string GetTypeName() const override { return "ScrollView"; }
    
private:
    float scrollX, scrollY;
    float contentWidth, contentHeight;
};

// ============================================================================
// TEXT COMPONENTS
// ============================================================================

class UCText : public UCComponent {
public:
    UCText();
    UCText(const std::string& text);
    
    void SetText(const std::string& t) { text = t; }
    const std::string& GetText() const { return text; }
    
    void SetTextColor(const UCColor& color) { textColor = color; }
    void SetFontFamily(const std::string& family) { fontFamily = family; }
    void SetFontSize(float size) { fontSize = size; }
    void SetFontWeight(int weight) { fontWeight = weight; }
    void SetLineHeight(float lh) { lineHeight = lh; }
    void SetTextAlign(int align) { textAlign = align; }
    
    void RenderContent(UCRenderContext& ctx) override;
    std::string GetTypeName() const override { return "Text"; }
    
protected:
    std::string text;
    UCColor textColor;
    std::string fontFamily;
    float fontSize;
    int fontWeight;
    float lineHeight;
    int textAlign;
};

// ============================================================================
// INTERACTIVE COMPONENTS
// ============================================================================

class UCButton : public UCText {
public:
    UCButton();
    UCButton(const std::string& label);
    
    void Render(UCRenderContext& ctx) override;
    std::string GetTypeName() const override { return "Button"; }
    
protected:
    UCColor hoverColor;
    UCColor pressedColor;
};

class UCInput : public UCComponent {
public:
    UCInput();
    
    void SetValue(const std::string& v) { value = v; }
    const std::string& GetValue() const { return value; }
    
    void SetPlaceholder(const std::string& p) { placeholder = p; }
    const std::string& GetPlaceholder() const { return placeholder; }
    
    void SetInputType(const std::string& t) { inputType = t; }
    
    void RenderContent(UCRenderContext& ctx) override;
    std::string GetTypeName() const override { return "Input"; }
    
protected:
    std::string value;
    std::string placeholder;
    std::string inputType;
    UCColor textColor;
    UCColor placeholderColor;
    std::string fontFamily;
    float fontSize;
};

class UCCheckbox : public UCComponent {
public:
    UCCheckbox();
    
    void SetChecked(bool c) { checked = c; }
    bool IsChecked() const { return checked; }
    
    void SetLabel(const std::string& l) { label = l; }
    
    void RenderContent(UCRenderContext& ctx) override;
    std::string GetTypeName() const override { return "Checkbox"; }
    
protected:
    bool checked;
    std::string label;
};

// ============================================================================
// MEDIA COMPONENTS
// ============================================================================

class UCImage : public UCComponent {
public:
    UCImage();
    
    void SetSource(const std::string& src) { source = src; }
    void SetImageId(int id) { imageId = id; }
    void SetAltText(const std::string& alt) { altText = alt; }
    
    void RenderContent(UCRenderContext& ctx) override;
    std::string GetTypeName() const override { return "Image"; }
    
protected:
    std::string source;
    int imageId;
    std::string altText;
};

// ============================================================================
// LIST COMPONENTS
// ============================================================================

class UCList : public UCContainer {
public:
    UCList();
    std::string GetTypeName() const override { return "List"; }
};

class UCListItem : public UCContainer {
public:
    UCListItem();
    std::string GetTypeName() const override { return "ListItem"; }
};

} // namespace Runtime
} // namespace UltraWeb
