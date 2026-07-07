// UltraWeb/runtime/UltraWebRuntime.cpp
// UltraWeb Runtime Implementation
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework

#include "UltraWebRuntime.h"
#include "../include/UltraWebDelta.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <queue>
#include <algorithm>

namespace UltraWeb {
namespace Runtime {

// ============================================================================
// COMPUTED STYLE IMPLEMENTATION
// ============================================================================

ComputedStyle::ComputedStyle() {
    Reset();
}

void ComputedStyle::Reset() {
    display = 1;        // block
    position = 0;       // static
    
    width = height = 0;
    minWidth = maxWidth = 0;
    minHeight = maxHeight = 0;
    marginTop = marginRight = marginBottom = marginLeft = 0;
    paddingTop = paddingRight = paddingBottom = paddingLeft = 0;
    borderWidth = 0;
    borderRadius = 0;
    
    top = right = bottom = left = 0;
    
    flexDirection = 0;
    flexWrap = 0;
    flexGrow = 0;
    flexShrink = 1;
    justifyContent = 0;
    alignItems = 0;
    alignSelf = 0;
    gap = 0;
    
    color = StyleColor(0, 0, 0);
    backgroundColor = StyleColor(255, 255, 255, 0);
    borderColor = StyleColor(0, 0, 0, 0);
    opacity = 1.0f;
    overflow = 0;
    zIndex = 0;
    
    fontFamily = "sans-serif";
    fontSize = 16.0f;
    fontWeight = 400;
    fontStyle = 0;
    lineHeight = 1.2f;
    textAlign = 0;
    textDecoration = 0;
    
    cursor = 0;
    visibility = 1;
}

// ============================================================================
// RUNTIME ELEMENT IMPLEMENTATION
// ============================================================================

RuntimeElement::RuntimeElement()
    : elementId(0)
    , parentId(0)
    , type(UCBElementType::Container)
    , x(0), y(0)
    , layoutWidth(0), layoutHeight(0)
    , visible(true)
    , enabled(true)
    , focused(false)
    , hovered(false)
    , pressed(false) {
}

std::string RuntimeElement::GetStringProp(UCBPropertyId id, const std::string& def) const {
    auto it = properties.find(id);
    if (it != properties.end() && std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }
    return def;
}

int32_t RuntimeElement::GetIntProp(UCBPropertyId id, int32_t def) const {
    auto it = properties.find(id);
    if (it != properties.end() && std::holds_alternative<int32_t>(it->second)) {
        return std::get<int32_t>(it->second);
    }
    return def;
}

float RuntimeElement::GetFloatProp(UCBPropertyId id, float def) const {
    auto it = properties.find(id);
    if (it != properties.end() && std::holds_alternative<float>(it->second)) {
        return std::get<float>(it->second);
    }
    return def;
}

bool RuntimeElement::GetBoolProp(UCBPropertyId id, bool def) const {
    auto it = properties.find(id);
    if (it != properties.end() && std::holds_alternative<bool>(it->second)) {
        return std::get<bool>(it->second);
    }
    return def;
}

bool RuntimeElement::HasEventHandler(UCBPropertyId eventId) const {
    return eventHandlers.find(eventId) != eventHandlers.end();
}

const std::string& RuntimeElement::GetEventHandler(UCBPropertyId eventId) const {
    static const std::string empty;
    auto it = eventHandlers.find(eventId);
    return it != eventHandlers.end() ? it->second : empty;
}

// ============================================================================
// STYLE ENGINE IMPLEMENTATION
// ============================================================================

StyleEngine::StyleEngine() {
}

void StyleEngine::LoadStyles(const UCSLoader& loader) {
    rules = loader.GetRules();
    
    classRules.clear();
    idRules.clear();
    tagRules.clear();
    
    for (const auto& rule : rules) {
        switch (rule.selector.type) {
            case CSSSelectorType::Class:
                classRules[rule.selector.name].push_back(&rule);
                break;
            case CSSSelectorType::Id:
                idRules[rule.selector.name].push_back(&rule);
                break;
            case CSSSelectorType::Element:
                tagRules[rule.selector.name].push_back(&rule);
                break;
            default:
                break;
        }
    }
}

void StyleEngine::ComputeStyle(RuntimeElement& element, const RuntimeElement* parent) {
    element.computedStyle.Reset();
    
    // Inherit from parent if available
    if (parent) {
        element.computedStyle.color = parent->computedStyle.color;
        element.computedStyle.fontFamily = parent->computedStyle.fontFamily;
        element.computedStyle.fontSize = parent->computedStyle.fontSize;
        element.computedStyle.fontWeight = parent->computedStyle.fontWeight;
        element.computedStyle.lineHeight = parent->computedStyle.lineHeight;
    }
    
    // Apply matching rules
    auto matchingRules = GetMatchingRules(element);
    
    // Sort by specificity
    std::sort(matchingRules.begin(), matchingRules.end(),
              [](const StyleRule* a, const StyleRule* b) {
                  return a->selector.specificity < b->selector.specificity;
              });
    
    for (const auto* rule : matchingRules) {
        ApplyRule(*rule, element.computedStyle);
    }
}

void StyleEngine::ComputeAllStyles(std::vector<RuntimeElement>& elements) {
    // Build parent map for inheritance
    std::unordered_map<uint16_t, RuntimeElement*> elementMap;
    for (auto& elem : elements) {
        elementMap[elem.elementId] = &elem;
    }
    
    // Process in order (assumes breadth-first serialization)
    for (auto& elem : elements) {
        RuntimeElement* parent = nullptr;
        if (elem.parentId != 0) {
            auto it = elementMap.find(elem.parentId);
            if (it != elementMap.end()) {
                parent = it->second;
            }
        }
        ComputeStyle(elem, parent);
    }
}

std::vector<const StyleRule*> StyleEngine::GetMatchingRules(const RuntimeElement& element) const {
    std::vector<const StyleRule*> result;
    
    // Match by tag name
    std::string tagName = UltraWeb::GetTagName(element.type);
    auto tagIt = tagRules.find(tagName);
    if (tagIt != tagRules.end()) {
        result.insert(result.end(), tagIt->second.begin(), tagIt->second.end());
    }
    
    // Match by class names
    for (const auto& className : element.classNames) {
        auto classIt = classRules.find(className);
        if (classIt != classRules.end()) {
            result.insert(result.end(), classIt->second.begin(), classIt->second.end());
        }
    }
    
    // Match by ID
    if (!element.id.empty()) {
        auto idIt = idRules.find(element.id);
        if (idIt != idRules.end()) {
            result.insert(result.end(), idIt->second.begin(), idIt->second.end());
        }
    }
    
    return result;
}

void StyleEngine::ApplyRule(const StyleRule& rule, ComputedStyle& style) {
    for (const auto& prop : rule.properties) {
        ApplyProperty(prop, style);
    }
}

void StyleEngine::ApplyProperty(const StyleProperty& prop, ComputedStyle& style) {
    switch (prop.id) {
        case CSSPropertyId::Display:
            if (std::holds_alternative<int32_t>(prop.value)) {
                style.display = std::get<int32_t>(prop.value);
            }
            break;
            
        case CSSPropertyId::Position:
            if (std::holds_alternative<int32_t>(prop.value)) {
                style.position = std::get<int32_t>(prop.value);
            }
            break;
            
        case CSSPropertyId::Width:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.width = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::Height:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.height = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::MarginTop:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.marginTop = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::Margin:
            // Shorthand - apply to all sides
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                float val = std::get<StyleDimension>(prop.value).ToPixels();
                style.marginTop = val;
                style.marginRight = val;
                style.marginBottom = val;
                style.marginLeft = val;
            }
            break;
            
        case CSSPropertyId::MarginRight:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.marginRight = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::MarginBottom:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.marginBottom = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::MarginLeft:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.marginLeft = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::PaddingTop:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.paddingTop = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::Padding:
            // Shorthand - apply to all sides
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                float val = std::get<StyleDimension>(prop.value).ToPixels();
                style.paddingTop = val;
                style.paddingRight = val;
                style.paddingBottom = val;
                style.paddingLeft = val;
            }
            break;
            
        case CSSPropertyId::PaddingRight:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.paddingRight = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::PaddingBottom:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.paddingBottom = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::PaddingLeft:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.paddingLeft = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::BorderWidth:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.borderWidth = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::BorderRadius:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.borderRadius = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::Color:
            if (std::holds_alternative<StyleColor>(prop.value)) {
                style.color = std::get<StyleColor>(prop.value);
            }
            break;
            
        case CSSPropertyId::BackgroundColor:
            if (std::holds_alternative<StyleColor>(prop.value)) {
                style.backgroundColor = std::get<StyleColor>(prop.value);
            }
            break;
            
        case CSSPropertyId::BorderColor:
            if (std::holds_alternative<StyleColor>(prop.value)) {
                style.borderColor = std::get<StyleColor>(prop.value);
            }
            break;
            
        case CSSPropertyId::FontFamily:
            if (std::holds_alternative<std::string>(prop.value)) {
                style.fontFamily = std::get<std::string>(prop.value);
            }
            break;
            
        case CSSPropertyId::FontSize:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.fontSize = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::FontWeight:
            if (std::holds_alternative<int32_t>(prop.value)) {
                style.fontWeight = std::get<int32_t>(prop.value);
            }
            break;
            
        case CSSPropertyId::LineHeight:
            if (std::holds_alternative<float>(prop.value)) {
                style.lineHeight = std::get<float>(prop.value);
            } else if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.lineHeight = std::get<StyleDimension>(prop.value).value;
            }
            break;
            
        case CSSPropertyId::TextAlign:
            if (std::holds_alternative<int32_t>(prop.value)) {
                style.textAlign = std::get<int32_t>(prop.value);
            }
            break;
            
        case CSSPropertyId::FlexDirection:
            if (std::holds_alternative<int32_t>(prop.value)) {
                style.flexDirection = std::get<int32_t>(prop.value);
            }
            break;
            
        case CSSPropertyId::FlexWrap:
            if (std::holds_alternative<int32_t>(prop.value)) {
                style.flexWrap = std::get<int32_t>(prop.value);
            }
            break;
            
        case CSSPropertyId::FlexGrow:
            if (std::holds_alternative<float>(prop.value)) {
                style.flexGrow = std::get<float>(prop.value);
            }
            break;
            
        case CSSPropertyId::FlexShrink:
            if (std::holds_alternative<float>(prop.value)) {
                style.flexShrink = std::get<float>(prop.value);
            }
            break;
            
        case CSSPropertyId::JustifyContent:
            if (std::holds_alternative<int32_t>(prop.value)) {
                style.justifyContent = std::get<int32_t>(prop.value);
            }
            break;
            
        case CSSPropertyId::AlignItems:
            if (std::holds_alternative<int32_t>(prop.value)) {
                style.alignItems = std::get<int32_t>(prop.value);
            }
            break;
            
        case CSSPropertyId::Gap:
            if (std::holds_alternative<StyleDimension>(prop.value)) {
                style.gap = std::get<StyleDimension>(prop.value).ToPixels();
            }
            break;
            
        case CSSPropertyId::Opacity:
            if (std::holds_alternative<float>(prop.value)) {
                style.opacity = std::get<float>(prop.value);
            }
            break;
            
        case CSSPropertyId::ZIndex:
            if (std::holds_alternative<int32_t>(prop.value)) {
                style.zIndex = std::get<int32_t>(prop.value);
            }
            break;
            
        default:
            break;
    }
}

void StyleEngine::Clear() {
    rules.clear();
    classRules.clear();
    idRules.clear();
    tagRules.clear();
}

// ============================================================================
// ULTRAWEB RUNTIME IMPLEMENTATION
// ============================================================================

UltraWebRuntime::UltraWebRuntime()
    : viewportWidth(800)
    , viewportHeight(600)
    , isLoaded(false)
    , focusedElementId(0)
    , hoveredElementId(0)
    , pressedElementId(0) {
}

UltraWebRuntime::~UltraWebRuntime() {
    Clear();
}

RuntimeLoadResult UltraWebRuntime::LoadPackage(const std::vector<uint8_t>& data) {
    return LoadPackage(data.data(), data.size());
}

RuntimeLoadResult UltraWebRuntime::LoadPackage(const uint8_t* data, size_t size) {
    RuntimeLoadResult result;
    
    Clear();
    
    // Open package
    PackageReader reader;
    if (!reader.Open(std::vector<uint8_t>(data, data + size))) {
        result.error = "Failed to open package";
        return result;
    }
    
    // Verify CRC
    if (!reader.VerifyCRC()) {
        result.error = "Package CRC verification failed";
        return result;
    }
    
    PackageInfo info = reader.GetInfo();
    loadedCrc32 = info.crc32;

    // Load UI section
    if (info.hasUI) {
        auto uiData = reader.ExtractUISection();
        if (!LoadUI(uiData)) {
            result.error = "Failed to load UI section";
            return result;
        }
        result.elementCount = static_cast<uint32_t>(elements.size());
    }
    
    // Load style section
    if (info.hasStyles) {
        auto styleData = reader.ExtractStyleSection();
        if (!LoadStyles(styleData)) {
            result.error = "Failed to load style section";
            return result;
        }
        result.styleRuleCount = static_cast<uint32_t>(ucsLoader.GetRuleCount());
    }
    
    // Load code section
    if (info.hasCode) {
        codeSection = reader.ExtractCodeSection();
        result.codeSize = static_cast<uint32_t>(codeSection.size());
    }
    
    // Load asset section
    if (info.hasAssets) {
        assetSection = reader.ExtractAssetSection();
        // Count assets from UCA header
        if (assetSection.size() >= sizeof(UCAHeader)) {
            const UCAHeader* ucaHeader = reinterpret_cast<const UCAHeader*>(assetSection.data());
            result.assetCount = ucaHeader->assetCount;
        }
    }
    
    // Compute styles
    if (!elements.empty()) {
        RecomputeStyles();
    }
    
    isLoaded = true;
    result.success = true;
    return result;
}

RuntimeLoadResult UltraWebRuntime::LoadPackageFromFile(const std::string& filePath) {
    RuntimeLoadResult result;
    
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        result.error = "Cannot open file: " + filePath;
        return result;
    }
    
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();
    
    return LoadPackage(data);
}

bool UltraWebRuntime::LoadUI(const std::vector<uint8_t>& ucbData) {
    auto result = ucbLoader.Load(ucbData);
    if (!result.success) {
        return false;
    }
    
    BuildRuntimeElements();
    BuildIndices();
    
    return true;
}

bool UltraWebRuntime::LoadStyles(const std::vector<uint8_t>& ucsData) {
    auto result = ucsLoader.Load(ucsData);
    if (!result.success) {
        return false;
    }
    
    styleEngine.LoadStyles(ucsLoader);
    return true;
}

bool UltraWebRuntime::LoadCode(const std::vector<uint8_t>& hbcData) {
    codeSection = hbcData;
    return true;
}

bool UltraWebRuntime::LoadAssets(const std::vector<uint8_t>& ucaData) {
    assetSection = ucaData;
    return true;
}

// ============================================================================
// JAVASCRIPT (Phase 3)
// ============================================================================

bool UltraWebRuntime::AttachJSEngine(std::shared_ptr<JSEngine> engine,
                                     std::string& error) {
    if (!engine) {
        error = "AttachJSEngine: null engine";
        return false;
    }
    jsEngine = std::move(engine);
    jsApi.reset(new UCApi(*this, *jsEngine));

    jsEngine->SetNativeDispatcher(
        [this](const std::string& fn, const std::string& argsJson) {
            return jsApi ? jsApi->Dispatch(fn, argsJson) : std::string("null");
        });

    return jsEngine->EvaluateSource(UCApi::GetPreludeSource(),
                                    "<uc-prelude>", error);
}

bool UltraWebRuntime::ExecuteCodeSection(std::string& error) {
    if (!jsEngine) {
        error = "no JS engine attached (call AttachJSEngine first)";
        return false;
    }
    if (codeSection.empty()) {
        error = "no code section loaded";
        return false;
    }

    // Hermes bytecode magic (same constant as server/HermesCompiler.h;
    // duplicated because the runtime must not depend on server code)
    static const uint8_t kHbcMagic[8] = {0xC6, 0x1F, 0xBC, 0x03,
                                         0xC1, 0x03, 0x19, 0x1F};
    bool isBytecode = codeSection.size() >= sizeof(kHbcMagic) &&
                      std::memcmp(codeSection.data(), kHbcMagic,
                                  sizeof(kHbcMagic)) == 0;

    if (isBytecode) {
        if (!jsEngine->SupportsBytecode()) {
            error = "code section is Hermes bytecode but the attached " +
                    jsEngine->GetName() +
                    " engine executes source only (build with "
                    "ULTRAWEB_USE_HERMES for bytecode execution)";
            return false;
        }
        return jsEngine->EvaluateBytecode(codeSection, error);
    }

    // Development mode: plain UTF-8 JavaScript source in the code section
    std::string source(codeSection.begin(), codeSection.end());
    return jsEngine->EvaluateSource(source, "<code-section>", error);
}

bool UltraWebRuntime::FireDomEvent(uint16_t elementId,
                                   const std::string& eventType,
                                   const std::string& eventJSON) {
    return jsApi ? jsApi->FireEvent(elementId, eventType, eventJSON) : false;
}

// ============================================================================
// DELTA UPDATES (Phase 4)
// ============================================================================

bool UltraWebRuntime::ApplyDelta(const std::vector<uint8_t>& delta,
                                 std::string& error) {
    if (!isLoaded) {
        error = "no package loaded";
        return false;
    }

    DeltaParseResult parsed = ParseDelta(delta);
    if (!parsed.success) {
        error = parsed.error;
        return false;
    }
    if (parsed.baseCrc32 != loadedCrc32) {
        error = "delta base mismatch: client state CRC does not match the "
                "package this delta was generated against";
        return false;
    }

    bool stylesReplaced = false;
    for (const DeltaOp& op : parsed.ops) {
        switch (op.type) {
            case DeltaOpType::SetText:
                SetElementText(op.elementId, op.stringValue);
                break;
            case DeltaOpType::SetValue:
                SetElementValue(op.elementId, op.stringValue);
                break;
            case DeltaOpType::AddClass:
            case DeltaOpType::RemoveClass: {
                RuntimeElement* el = GetElement(op.elementId);
                if (!el) break;
                auto it = std::find(el->classNames.begin(),
                                    el->classNames.end(), op.stringValue);
                if (op.type == DeltaOpType::AddClass) {
                    if (it == el->classNames.end()) {
                        el->classNames.push_back(op.stringValue);
                        RecomputeStyle(op.elementId);
                    }
                } else if (it != el->classNames.end()) {
                    el->classNames.erase(it);
                    RecomputeStyle(op.elementId);
                }
                break;
            }
            case DeltaOpType::SetVisible:
                SetElementVisible(op.elementId, op.boolValue);
                break;
            case DeltaOpType::SetEnabled:
                SetElementEnabled(op.elementId, op.boolValue);
                break;
            case DeltaOpType::ReplaceSection:
                switch (op.sectionId) {
                    case DeltaSectionId::UI:
                        if (!LoadUI(op.sectionData)) {
                            error = "delta UI section failed to load";
                            return false;
                        }
                        stylesReplaced = true;  // new tree needs styling
                        break;
                    case DeltaSectionId::Style:
                        if (!LoadStyles(op.sectionData)) {
                            error = "delta style section failed to load";
                            return false;
                        }
                        stylesReplaced = true;
                        break;
                    case DeltaSectionId::Code:
                        LoadCode(op.sectionData);
                        break;
                    case DeltaSectionId::Assets:
                        LoadAssets(op.sectionData);
                        break;
                }
                break;
        }
    }

    if (stylesReplaced) {
        RecomputeStyles();
    }

    loadedCrc32 = parsed.targetCrc32;
    return true;
}

void UltraWebRuntime::BuildRuntimeElements() {
    elements.clear();
    
    const auto& sourceElements = ucbLoader.GetElements();
    elements.reserve(sourceElements.size());
    
    for (const auto& src : sourceElements) {
        RuntimeElement elem;
        elem.elementId = src.elementId;
        elem.parentId = src.parentId;
        elem.type = src.type;
        elem.id = src.id;
        elem.textContent = src.textContent;
        
        // Copy style class names
        for (uint16_t classId : src.styleClasses) {
            elem.classNames.push_back(ucbLoader.GetStyleClassName(classId));
        }
        
        // Copy properties
        for (const auto& prop : src.properties) {
            elem.properties[prop.id] = prop.value;
            
            // Check for event handlers
            if (prop.id >= UCBPropertyId::OnClick && prop.id <= UCBPropertyId::OnScroll) {
                if (std::holds_alternative<std::string>(prop.value)) {
                    elem.eventHandlers[prop.id] = std::get<std::string>(prop.value);
                }
            }
        }
        
        // Set visibility/enabled from properties
        elem.visible = elem.GetBoolProp(UCBPropertyId::Visible, true);
        elem.enabled = elem.GetBoolProp(UCBPropertyId::Enabled, true);
        
        elements.push_back(std::move(elem));
    }
}

void UltraWebRuntime::BuildIndices() {
    idToElement.clear();
    parentToChildren.clear();
    
    for (auto& elem : elements) {
        if (!elem.id.empty()) {
            idToElement[elem.id] = elem.elementId;
        }
        
        if (elem.parentId != 0 || elem.elementId != 1) {
            parentToChildren[elem.parentId].push_back(elem.elementId);
        }
    }
    
    // Set child IDs on each element
    for (auto& elem : elements) {
        auto it = parentToChildren.find(elem.elementId);
        if (it != parentToChildren.end()) {
            elem.childIds = it->second;
        }
    }
}

RuntimeElement* UltraWebRuntime::GetElement(uint16_t elementId) {
    for (auto& elem : elements) {
        if (elem.elementId == elementId) {
            return &elem;
        }
    }
    return nullptr;
}

RuntimeElement* UltraWebRuntime::GetElementById(const std::string& id) {
    auto it = idToElement.find(id);
    if (it != idToElement.end()) {
        return GetElement(it->second);
    }
    return nullptr;
}

RuntimeElement* UltraWebRuntime::GetRootElement() {
    if (elements.empty()) return nullptr;
    
    for (auto& elem : elements) {
        if (elem.parentId == 0) {
            return &elem;
        }
    }
    
    return &elements[0];
}

std::vector<RuntimeElement*> UltraWebRuntime::GetChildren(uint16_t parentId) {
    std::vector<RuntimeElement*> children;
    
    auto it = parentToChildren.find(parentId);
    if (it != parentToChildren.end()) {
        for (uint16_t childId : it->second) {
            RuntimeElement* child = GetElement(childId);
            if (child) {
                children.push_back(child);
            }
        }
    }
    
    return children;
}

void UltraWebRuntime::TraverseDepthFirst(std::function<void(RuntimeElement&, int depth)> visitor) {
    RuntimeElement* root = GetRootElement();
    if (root) {
        TraverseDepthFirstImpl(root->elementId, 0, visitor);
    }
}

void UltraWebRuntime::TraverseDepthFirstImpl(uint16_t elementId, int depth,
                                              std::function<void(RuntimeElement&, int)>& visitor) {
    RuntimeElement* elem = GetElement(elementId);
    if (!elem) return;
    
    visitor(*elem, depth);
    
    for (uint16_t childId : elem->childIds) {
        TraverseDepthFirstImpl(childId, depth + 1, visitor);
    }
}

void UltraWebRuntime::TraverseBreadthFirst(std::function<void(RuntimeElement&)> visitor) {
    RuntimeElement* root = GetRootElement();
    if (!root) return;
    
    std::queue<uint16_t> queue;
    queue.push(root->elementId);
    
    while (!queue.empty()) {
        uint16_t currentId = queue.front();
        queue.pop();
        
        RuntimeElement* elem = GetElement(currentId);
        if (!elem) continue;
        
        visitor(*elem);
        
        for (uint16_t childId : elem->childIds) {
            queue.push(childId);
        }
    }
}

void UltraWebRuntime::RecomputeStyles() {
    styleEngine.ComputeAllStyles(elements);
}

void UltraWebRuntime::RecomputeStyle(uint16_t elementId) {
    RuntimeElement* elem = GetElement(elementId);
    if (!elem) return;
    
    RuntimeElement* parent = elem->parentId != 0 ? GetElement(elem->parentId) : nullptr;
    styleEngine.ComputeStyle(*elem, parent);
}

void UltraWebRuntime::PerformLayout(float vpWidth, float vpHeight) {
    viewportWidth = vpWidth;
    viewportHeight = vpHeight;
    
    RuntimeElement* root = GetRootElement();
    if (root) {
        LayoutElement(*root, 0, 0, vpWidth, vpHeight);
    }
}

void UltraWebRuntime::LayoutElement(RuntimeElement& element, float parentX, float parentY,
                                     float availableWidth, float availableHeight) {
    const auto& style = element.computedStyle;
    
    // Calculate position
    element.x = parentX + style.marginLeft;
    element.y = parentY + style.marginTop;
    
    // Calculate dimensions
    element.layoutWidth = style.width > 0 ? style.width : availableWidth - style.marginLeft - style.marginRight;
    
    // Calculate minimum height for leaf elements
    float minHeight = 0;
    switch (element.type) {
        case UCBElementType::Text:
            minHeight = style.fontSize * style.lineHeight;
            break;
        case UCBElementType::Button:
            minHeight = style.fontSize * style.lineHeight + style.paddingTop + style.paddingBottom;
            break;
        case UCBElementType::Input:
        case UCBElementType::TextArea:
            minHeight = style.fontSize * style.lineHeight + style.paddingTop + style.paddingBottom;
            break;
        case UCBElementType::Checkbox:
        case UCBElementType::Radio:
            minHeight = 20;  // Default checkbox/radio height
            break;
        case UCBElementType::Image:
            minHeight = 100;  // Default image height
            break;
        default:
            break;
    }
    
    element.layoutHeight = style.height > 0 ? style.height : minHeight;
    
    // Layout children
    float contentX = style.paddingLeft;
    float contentY = style.paddingTop;
    float contentWidth = element.layoutWidth - style.paddingLeft - style.paddingRight;
    float contentHeight = availableHeight - style.paddingTop - style.paddingBottom;
    
    // Simple block layout (stack children vertically)
    float currentY = contentY;
    
    for (uint16_t childId : element.childIds) {
        RuntimeElement* child = GetElement(childId);
        if (!child || !child->visible) continue;
        
        LayoutElement(*child, contentX, currentY, contentWidth, contentHeight);
        
        currentY += child->layoutHeight + child->computedStyle.marginTop + child->computedStyle.marginBottom;
    }
    
    // Auto height for containers
    if (style.height <= 0 && !element.childIds.empty()) {
        element.layoutHeight = currentY + style.paddingBottom;
    }
}

void UltraWebRuntime::Render(RenderCallback callback) {
    RuntimeElement* root = GetRootElement();
    if (root) {
        RenderElement(*root, callback, 0);
    }
}

void UltraWebRuntime::RenderElement(const RuntimeElement& element, RenderCallback& callback, int depth) {
    if (!element.visible) return;
    
    callback(element);
    
    for (uint16_t childId : element.childIds) {
        const RuntimeElement* child = const_cast<UltraWebRuntime*>(this)->GetElement(childId);
        if (child) {
            RenderElement(*child, callback, depth + 1);
        }
    }
}

void UltraWebRuntime::SetEventCallback(EventCallback callback) {
    eventCallback = callback;
}

void UltraWebRuntime::DispatchEvent(uint16_t elementId, UCBPropertyId eventType) {
    RuntimeElement* elem = GetElement(elementId);
    if (!elem) return;
    
    if (elem->HasEventHandler(eventType) && eventCallback) {
        eventCallback(elementId, elem->GetEventHandler(eventType));
    }
}

RuntimeElement* UltraWebRuntime::HitTest(float x, float y) {
    RuntimeElement* root = GetRootElement();
    if (!root) return nullptr;
    
    return HitTestElement(*root, x, y);
}

RuntimeElement* UltraWebRuntime::HitTestElement(RuntimeElement& element, float x, float y) {
    if (!element.visible) return nullptr;
    
    // Check if point is within element bounds
    float ex = element.x;
    float ey = element.y;
    float ew = element.layoutWidth;
    float eh = element.layoutHeight;
    
    if (x < ex || x > ex + ew || y < ey || y > ey + eh) {
        return nullptr;
    }
    
    // Check children (in reverse order for z-index)
    for (auto it = element.childIds.rbegin(); it != element.childIds.rend(); ++it) {
        RuntimeElement* child = GetElement(*it);
        if (child) {
            RuntimeElement* hit = HitTestElement(*child, x, y);
            if (hit) return hit;
        }
    }
    
    return &element;
}

void UltraWebRuntime::SetElementVisible(uint16_t elementId, bool visible) {
    RuntimeElement* elem = GetElement(elementId);
    if (elem) elem->visible = visible;
}

void UltraWebRuntime::SetElementEnabled(uint16_t elementId, bool enabled) {
    RuntimeElement* elem = GetElement(elementId);
    if (elem) elem->enabled = enabled;
}

void UltraWebRuntime::SetElementFocused(uint16_t elementId, bool focused) {
    if (focused && focusedElementId != elementId) {
        // Unfocus previous
        RuntimeElement* prev = GetElement(focusedElementId);
        if (prev) prev->focused = false;
    }
    
    RuntimeElement* elem = GetElement(elementId);
    if (elem) {
        elem->focused = focused;
        focusedElementId = focused ? elementId : 0;
    }
}

void UltraWebRuntime::SetElementHovered(uint16_t elementId, bool hovered) {
    if (hovered && hoveredElementId != elementId) {
        RuntimeElement* prev = GetElement(hoveredElementId);
        if (prev) prev->hovered = false;
    }
    
    RuntimeElement* elem = GetElement(elementId);
    if (elem) {
        elem->hovered = hovered;
        hoveredElementId = hovered ? elementId : 0;
    }
}

void UltraWebRuntime::SetElementPressed(uint16_t elementId, bool pressed) {
    RuntimeElement* elem = GetElement(elementId);
    if (elem) {
        elem->pressed = pressed;
        pressedElementId = pressed ? elementId : 0;
    }
}

void UltraWebRuntime::SetElementText(uint16_t elementId, const std::string& text) {
    RuntimeElement* elem = GetElement(elementId);
    if (elem) {
        elem->textContent = text;
        elem->properties[UCBPropertyId::Text] = text;
    }
}

void UltraWebRuntime::SetElementValue(uint16_t elementId, const std::string& value) {
    RuntimeElement* elem = GetElement(elementId);
    if (elem) {
        elem->properties[UCBPropertyId::Value] = value;
    }
}

std::string UltraWebRuntime::DumpElementTree() const {
    std::ostringstream oss;
    oss << "=== Runtime Element Tree ===\n";
    oss << "Elements: " << elements.size() << "\n\n";
    
    const_cast<UltraWebRuntime*>(this)->TraverseDepthFirst(
        [&oss](RuntimeElement& elem, int depth) {
            std::string indent(depth * 2, ' ');
            oss << indent << UltraWeb::GetTagName(elem.type);
            if (!elem.id.empty()) oss << " #" << elem.id;
            if (!elem.classNames.empty()) {
                oss << " [";
                for (size_t i = 0; i < elem.classNames.size(); i++) {
                    if (i > 0) oss << ", ";
                    oss << "." << elem.classNames[i];
                }
                oss << "]";
            }
            if (!elem.textContent.empty()) {
                oss << " \"" << elem.textContent.substr(0, 30);
                if (elem.textContent.size() > 30) oss << "...";
                oss << "\"";
            }
            oss << "\n";
            oss << indent << "  pos: (" << elem.x << "," << elem.y << ") ";
            oss << "size: " << elem.layoutWidth << "x" << elem.layoutHeight << "\n";
        });
    
    return oss.str();
}

std::string UltraWebRuntime::DumpStyles() const {
    return ucsLoader.DumpRules();
}

void UltraWebRuntime::Clear() {
    elements.clear();
    idToElement.clear();
    parentToChildren.clear();
    codeSection.clear();
    assetSection.clear();
    ucbLoader.Clear();
    ucsLoader.Clear();
    styleEngine.Clear();
    isLoaded = false;
    loadedCrc32 = 0;
    focusedElementId = 0;
    hoveredElementId = 0;
    pressedElementId = 0;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool IsContainerType(UCBElementType type) {
    switch (type) {
        case UCBElementType::Container:
        case UCBElementType::FlexBox:
        case UCBElementType::Grid:
        case UCBElementType::ScrollView:
        case UCBElementType::List:
        case UCBElementType::Table:
        case UCBElementType::Tree:
        case UCBElementType::Tabs:
        case UCBElementType::Modal:
        case UCBElementType::Menu:
            return true;
        default:
            return false;
    }
}

bool IsInteractiveType(UCBElementType type) {
    switch (type) {
        case UCBElementType::Button:
        case UCBElementType::Input:
        case UCBElementType::TextArea:
        case UCBElementType::Checkbox:
        case UCBElementType::Radio:
        case UCBElementType::Select:
        case UCBElementType::Slider:
            return true;
        default:
            return false;
    }
}

} // namespace Runtime
} // namespace UltraWeb
