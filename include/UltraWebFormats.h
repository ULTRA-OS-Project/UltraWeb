// UltraWeb/include/UltraWebFormats.h
// Binary format constants, structures, and magic numbers for UltraWeb
// Version: 1.0.0
// Last Modified: 2025-12-04
// Author: UltraCanvas Framework
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace UltraWeb {

// ============================================================================
// FILE FORMAT MAGIC NUMBERS
// ============================================================================

constexpr uint32_t UCS_MAGIC = 0x55435331;  // 'UCS1' - UltraCanvas Styles
constexpr uint32_t UCB_MAGIC = 0x55434231;  // 'UCB1' - UltraCanvas Bundle (UI)
constexpr uint32_t UCA_MAGIC = 0x55434131;  // 'UCA1' - UltraCanvas Assets
constexpr uint32_t UCPKG_MAGIC = 0x5543504B; // 'UCPK' - UltraCanvas Package

constexpr uint16_t UCS_VERSION = 0x0100;    // Version 1.0
constexpr uint16_t UCB_VERSION = 0x0100;    // Version 1.0
constexpr uint16_t UCA_VERSION = 0x0100;    // Version 1.0
constexpr uint16_t UCPKG_VERSION = 0x0100;  // Version 1.0

// ============================================================================
// UCPKG FILE HEADER (48 bytes)
// ============================================================================

// UCPKG Flags
enum class UCPKGFlags : uint16_t {
    None            = 0x0000,
    Compressed      = 0x0001,   // Payload after the header is one LZ4 frame
                                // (standard frame format, magic 04 22 4D 18)
                                // of the concatenated sections. Header
                                // offsets/sizes/CRC32 always describe the
                                // uncompressed layout.
    Encrypted       = 0x0002,   // Encryption enabled
    HasUI           = 0x0010,   // Contains UI section
    HasStyles       = 0x0020,   // Contains style section
    HasCode         = 0x0040,   // Contains code section
    HasAssets       = 0x0080,   // Contains asset section
    Debug           = 0x8000    // Debug build
};

inline UCPKGFlags operator|(UCPKGFlags a, UCPKGFlags b) {
    return static_cast<UCPKGFlags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

inline UCPKGFlags operator&(UCPKGFlags a, UCPKGFlags b) {
    return static_cast<UCPKGFlags>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

inline bool HasFlag(UCPKGFlags flags, UCPKGFlags flag) {
    return (static_cast<uint16_t>(flags) & static_cast<uint16_t>(flag)) != 0;
}

#pragma pack(push, 1)
struct UCPKGHeader {
    uint32_t magic;              // 0x00-0x03: Magic number (UCPKG_MAGIC)
    uint16_t version;            // 0x04-0x05: Format version
    uint16_t flags;              // 0x06-0x07: Flags
    uint32_t totalSize;          // 0x08-0x0B: Total file size
    uint32_t crc32;              // 0x0C-0x0F: CRC32 checksum of content
    uint32_t uiSectionOffset;    // 0x10-0x13: UI section offset
    uint32_t uiSectionSize;      // 0x14-0x17: UI section size
    uint32_t styleSectionOffset; // 0x18-0x1B: Style section offset
    uint32_t styleSectionSize;   // 0x1C-0x1F: Style section size
    uint32_t codeSectionOffset;  // 0x20-0x23: Code section offset
    uint32_t codeSectionSize;    // 0x24-0x27: Code section size
    uint32_t assetSectionOffset; // 0x28-0x2B: Asset section offset
    uint32_t assetSectionSize;   // 0x2C-0x2F: Asset section size
    
    UCPKGHeader()
        : magic(UCPKG_MAGIC)
        , version(UCPKG_VERSION)
        , flags(0)
        , totalSize(0)
        , crc32(0)
        , uiSectionOffset(0)
        , uiSectionSize(0)
        , styleSectionOffset(0)
        , styleSectionSize(0)
        , codeSectionOffset(0)
        , codeSectionSize(0)
        , assetSectionOffset(0)
        , assetSectionSize(0) {}
};
#pragma pack(pop)

static_assert(sizeof(UCPKGHeader) == 48, "UCPKGHeader must be exactly 48 bytes");

// ============================================================================
// UCA FILE HEADER (16 bytes)
// ============================================================================

// Asset types
enum class UCAAssetType : uint8_t {
    Image       = 0x01,
    Font        = 0x02,
    Audio       = 0x03,
    Video       = 0x04,
    Data        = 0x05,
    Binary      = 0x06,
    Unknown     = 0xFF
};

// Compression types for assets
enum class UCACompressionType : uint8_t {
    None        = 0x00,
    LZ4         = 0x01,
    Deflate     = 0x02,
    Brotli      = 0x03
};

#pragma pack(push, 1)
struct UCAHeader {
    uint32_t magic;          // 0x00-0x03: Magic number (UCA_MAGIC)
    uint16_t version;        // 0x04-0x05: Format version
    uint16_t assetCount;     // 0x06-0x07: Number of assets
    uint32_t totalSize;      // 0x08-0x0B: Total size
    uint32_t flags;          // 0x0C-0x0F: Flags
    
    UCAHeader()
        : magic(UCA_MAGIC)
        , version(UCA_VERSION)
        , assetCount(0)
        , totalSize(0)
        , flags(0) {}
};

// Asset index entry (18 bytes)
struct UCAAssetEntry {
    uint32_t nameHash;           // Hash of asset name
    UCAAssetType type;           // Asset type
    UCACompressionType compression; // Compression type
    uint32_t offset;             // Offset to data
    uint32_t compressedSize;     // Compressed size
    uint32_t originalSize;       // Original size
};
#pragma pack(pop)

static_assert(sizeof(UCAHeader) == 16, "UCAHeader must be exactly 16 bytes");
static_assert(sizeof(UCAAssetEntry) == 18, "UCAAssetEntry must be exactly 18 bytes");

// ============================================================================
// UCB FILE HEADER (16 bytes)
// ============================================================================

#pragma pack(push, 1)
struct UCBHeader {
    uint32_t magic;              // 0x00-0x03: Magic number (UCB_MAGIC)
    uint16_t version;            // 0x04-0x05: Format version
    uint16_t flags;              // 0x06-0x07: Flags
    uint32_t elementCount;       // 0x08-0x0B: Number of elements
    uint32_t stringTableOffset;  // 0x0C-0x0F: Offset to string table
    
    UCBHeader() 
        : magic(UCB_MAGIC)
        , version(UCB_VERSION)
        , flags(0)
        , elementCount(0)
        , stringTableOffset(0) {}
};
#pragma pack(pop)

static_assert(sizeof(UCBHeader) == 16, "UCBHeader must be exactly 16 bytes");

// ============================================================================
// UCB ELEMENT TYPES
// ============================================================================

enum class UCBElementType : uint16_t {
    // Layout Containers (0x0001 - 0x000F)
    Container       = 0x0001,
    FlexBox         = 0x0002,
    Grid            = 0x0003,
    ScrollView      = 0x0004,
    
    // Form Elements (0x0010 - 0x001F)
    Text            = 0x0010,
    Button          = 0x0011,
    Input           = 0x0012,
    TextArea        = 0x0013,
    Checkbox        = 0x0014,
    Radio           = 0x0015,
    Select          = 0x0016,
    Slider          = 0x0017,
    
    // Media Elements (0x0020 - 0x002F)
    Image           = 0x0020,
    SVG             = 0x0021,
    Canvas          = 0x0022,
    Video           = 0x0023,
    Audio           = 0x0024,
    
    // Data Display (0x0030 - 0x003F)
    List            = 0x0030,
    Table           = 0x0031,
    Tree            = 0x0032,
    
    // Complex Components (0x0040 - 0x004F)
    Tabs            = 0x0040,
    Modal           = 0x0041,
    Menu            = 0x0042,
    Tooltip         = 0x0043,
    Popover         = 0x0044,
    
    // Special (0xFFxx)
    Custom          = 0xFF00,
    Invalid         = 0xFFFF
};

// ============================================================================
// UCB ELEMENT PROPERTY IDs
// ============================================================================

enum class UCBPropertyId : uint8_t {
    // Core Properties (0x01 - 0x0F)
    Id              = 0x01,
    Class           = 0x02,
    Name            = 0x03,
    Visible         = 0x04,
    Enabled         = 0x05,
    
    // Content Properties (0x10 - 0x1F)
    Text            = 0x10,
    Value           = 0x11,
    Placeholder     = 0x12,
    Src             = 0x13,
    Alt             = 0x14,
    Href            = 0x15,
    
    // Layout Hints (0x20 - 0x2F)
    Width           = 0x20,
    Height          = 0x21,
    MinWidth        = 0x22,
    MaxWidth        = 0x23,
    MinHeight       = 0x24,
    MaxHeight       = 0x25,
    
    // Form Properties (0x30 - 0x3F)
    Type            = 0x30,
    Required        = 0x31,
    ReadOnly        = 0x32,
    Disabled        = 0x33,
    Checked         = 0x34,
    Selected        = 0x35,
    Min             = 0x36,
    Max             = 0x37,
    Step            = 0x38,
    
    // Event Handlers (0x40 - 0x4F)
    OnClick         = 0x40,
    OnChange        = 0x41,
    OnInput         = 0x42,
    OnFocus         = 0x43,
    OnBlur          = 0x44,
    OnSubmit        = 0x45,
    OnKeyDown       = 0x46,
    OnKeyUp         = 0x47,
    OnMouseEnter    = 0x48,
    OnMouseLeave    = 0x49,
    OnScroll        = 0x4A,
    
    // Data Binding (0x50 - 0x5F)
    Bind            = 0x50,
    Model           = 0x51,
    For             = 0x52,
    If              = 0x53,
    
    // Custom Data (0xF0 - 0xFF)
    Data            = 0xF0,
    Custom          = 0xFF
};

// ============================================================================
// UCB PROPERTY VALUE TYPES
// ============================================================================

enum class UCBValueType : uint8_t {
    Null        = 0x00,
    Bool        = 0x01,
    Int32       = 0x02,
    Float32     = 0x03,
    String      = 0x04,  // String table reference
    StyleClass  = 0x05,  // Style class reference
    Handler     = 0x06,  // Event handler reference
    Binding     = 0x07,  // Data binding expression
    Array       = 0x08,  // Array of values
    Invalid     = 0xFF
};

// ============================================================================
// UCB BINARY STRUCTURES
// ============================================================================

#pragma pack(push, 1)

// Element Header (8 bytes minimum)
struct UCBElementHeader {
    UCBElementType type;         // Element type
    uint16_t elementId;          // Unique element ID
    uint16_t parentId;           // Parent element ID (0 = root)
    uint8_t styleClassCount;     // Number of style classes
    uint8_t propertyCount;       // Number of properties
    // Followed by:
    // - StyleClasses: uint16[styleClassCount]
    // - Properties: UCBProperty[propertyCount]
};

static_assert(sizeof(UCBElementHeader) == 8, "UCBElementHeader must be exactly 8 bytes");

// Property Entry (3+ bytes)
struct UCBPropertyEntry {
    UCBPropertyId propertyId;    // Property identifier
    UCBValueType valueType;      // Value type
    uint8_t valueSize;           // Size of value data
    // Followed by value data
};

static_assert(sizeof(UCBPropertyEntry) == 3, "UCBPropertyEntry must be exactly 3 bytes");

#pragma pack(pop)

// ============================================================================
// UCS FILE HEADER (20 bytes)
// ============================================================================

#pragma pack(push, 1)
struct UCSHeader {
    uint32_t magic;              // 0x00-0x03: Magic number (UCS_MAGIC)
    uint16_t version;            // 0x04-0x05: Format version
    uint16_t ruleCount;          // 0x06-0x07: Number of CSS rules
    uint32_t selectorTableOffset; // 0x08-0x0B: Offset to selector table
    uint32_t propertyTableOffset; // 0x0C-0x0F: Offset to property table
    uint32_t variableTableOffset; // 0x10-0x13: Offset to variable table
    
    UCSHeader() 
        : magic(UCS_MAGIC)
        , version(UCS_VERSION)
        , ruleCount(0)
        , selectorTableOffset(0)
        , propertyTableOffset(0)
        , variableTableOffset(0) {}
};
#pragma pack(pop)

static_assert(sizeof(UCSHeader) == 20, "UCSHeader must be exactly 20 bytes");

// ============================================================================
// CSS SELECTOR TYPES
// ============================================================================

enum class CSSSelectorType : uint8_t {
    Element         = 0x00,
    Class           = 0x01,
    Id              = 0x02,
    Universal       = 0x03,
    Attribute       = 0x04,
    PseudoClass     = 0x05,
    PseudoElement   = 0x06,
    Combinator      = 0x07
};

// ============================================================================
// CSS UNITS
// ============================================================================

enum class CSSUnit : uint8_t {
    None    = 0x00,
    Px      = 0x01,
    Em      = 0x02,
    Rem     = 0x03,
    Percent = 0x04,
    Vw      = 0x05,
    Vh      = 0x06,
    Vmin    = 0x07,
    Vmax    = 0x08,
    Ch      = 0x09,
    Ex      = 0x0A,
    Cm      = 0x0B,
    Mm      = 0x0C,
    In      = 0x0D,
    Pt      = 0x0E,
    Pc      = 0x0F,
    Fr      = 0x10,
    Deg     = 0x11,
    Rad     = 0x12,
    Turn    = 0x13,
    S       = 0x14,
    Ms      = 0x15
};

// ============================================================================
// CSS PROPERTY IDs (1 byte each)
// ============================================================================

enum class CSSPropertyId : uint8_t {
    // ===== LAYOUT PROPERTIES (0x01 - 0x0F) =====
    Display             = 0x01,
    Position            = 0x02,
    FlexDirection       = 0x03,
    JustifyContent      = 0x04,
    AlignItems          = 0x05,
    AlignContent        = 0x06,
    FlexWrap            = 0x07,
    FlexGrow            = 0x08,
    FlexShrink          = 0x09,
    FlexBasis           = 0x0A,
    Order               = 0x0B,
    Gap                 = 0x0C,
    RowGap              = 0x0D,
    ColumnGap           = 0x0E,
    AlignSelf           = 0x0F,
    
    // ===== BOX MODEL PROPERTIES (0x10 - 0x1F) =====
    Width               = 0x10,
    Height              = 0x11,
    Padding             = 0x12,  // Shorthand (4 values)
    PaddingTop          = 0x13,
    PaddingRight        = 0x14,
    PaddingBottom       = 0x15,
    PaddingLeft         = 0x16,
    Margin              = 0x17,  // Shorthand (4 values)
    MarginTop           = 0x18,
    MarginRight         = 0x19,
    MarginBottom        = 0x1A,
    MarginLeft          = 0x1B,
    MinWidth            = 0x1C,
    MaxWidth            = 0x1D,
    MinHeight           = 0x1E,
    MaxHeight           = 0x1F,
    
    // ===== TYPOGRAPHY PROPERTIES (0x20 - 0x2F) =====
    FontFamily          = 0x20,
    FontSize            = 0x21,
    FontWeight          = 0x22,
    FontStyle           = 0x23,
    Color               = 0x24,
    TextAlign           = 0x25,
    TextDecoration      = 0x26,
    TextTransform       = 0x27,
    LineHeight          = 0x28,
    LetterSpacing       = 0x29,
    WordSpacing         = 0x2A,
    WhiteSpace          = 0x2B,
    TextOverflow        = 0x2C,
    
    // ===== VISUAL PROPERTIES (0x30 - 0x3F) =====
    Background          = 0x30,
    BackgroundColor     = 0x31,
    BackgroundImage     = 0x32,
    BorderRadius        = 0x33,  // Shorthand (4 values)
    BorderTopLeftRadius = 0x34,
    BorderTopRightRadius = 0x35,
    BorderBottomRightRadius = 0x36,
    BorderBottomLeftRadius = 0x37,
    Border              = 0x38,  // Shorthand
    BorderWidth         = 0x39,
    BorderStyle         = 0x3A,
    BorderColor         = 0x3B,
    BoxShadow           = 0x3C,
    Opacity             = 0x3D,
    
    // ===== VISIBILITY & INTERACTION (0x40 - 0x4F) =====
    Visibility          = 0x40,
    Overflow            = 0x41,
    OverflowX           = 0x42,
    OverflowY           = 0x43,
    PointerEvents       = 0x44,
    Cursor              = 0x45,
    ZIndex              = 0x46,
    
    // ===== POSITIONING (0x50 - 0x5F) =====
    Top                 = 0x50,
    Right               = 0x51,
    Bottom              = 0x52,
    Left                = 0x53,
    
    // ===== TRANSITIONS (0x60 - 0x6F) =====
    Transition          = 0x60,
    TransitionProperty  = 0x61,
    TransitionDuration  = 0x62,
    TransitionTimingFunction = 0x63,
    
    // ===== SPECIAL =====
    Invalid             = 0xFF
};

// ============================================================================
// CSS VALUE TYPES (1 byte each)
// ============================================================================

enum class CSSValueType : uint8_t {
    Enum        = 0x00,  // Enumerated value (1 byte)
    Color       = 0x01,  // RGBA color (4 bytes)
    Length      = 0x02,  // Length with unit (3 bytes: int16 + unit)
    StringRef   = 0x03,  // Reference to string table (2 bytes)
    Float       = 0x04,  // Float32 (4 bytes)
    Gradient    = 0x05,  // Gradient definition (variable)
    Integer     = 0x06,  // Int32 (4 bytes)
    Percentage  = 0x07,  // Percentage as float (4 bytes)
    Time        = 0x08,  // Time in milliseconds (4 bytes)
    Shadow      = 0x09,  // Box shadow (14 bytes)
    Auto        = 0x0A,  // 'auto' keyword (0 bytes)
    None        = 0x0B,  // 'none' keyword (0 bytes)
    Inherit     = 0x0C,  // 'inherit' keyword (0 bytes)
    VarRef      = 0x0D,  // CSS variable reference (2 bytes)
    MultiLength = 0x0E,  // Multiple lengths (e.g., padding: 1 2 3 4)
    Invalid     = 0xFF
};

// ============================================================================
// CSS LENGTH UNITS
// ============================================================================

enum class CSSLengthUnit : uint8_t {
    Px      = 0x00,  // Pixels (default)
    Em      = 0x01,  // Relative to font-size
    Rem     = 0x02,  // Relative to root font-size
    Percent = 0x03,  // Percentage
    Vw      = 0x04,  // Viewport width
    Vh      = 0x05,  // Viewport height
    Vmin    = 0x06,  // Viewport minimum
    Vmax    = 0x07,  // Viewport maximum
    Pt      = 0x08,  // Points
    Ch      = 0x09,  // Character width
    Ex      = 0x0A,  // x-height
    Invalid = 0xFF
};

// ============================================================================
// CSS ENUM VALUES
// ============================================================================

// Display values
enum class CSSDisplay : uint8_t {
    Block       = 0x00,
    Flex        = 0x01,
    Grid        = 0x02,
    InlineBlock = 0x03,
    Inline      = 0x04,
    InlineFlex  = 0x05,
    None        = 0x06
};

// Position values
enum class CSSPosition : uint8_t {
    Static      = 0x00,
    Relative    = 0x01,
    Absolute    = 0x02,
    Fixed       = 0x03,
    Sticky      = 0x04
};

// Flex direction values
enum class CSSFlexDirection : uint8_t {
    Row             = 0x00,
    RowReverse      = 0x01,
    Column          = 0x02,
    ColumnReverse   = 0x03
};

// Justify content values
enum class CSSJustifyContent : uint8_t {
    FlexStart       = 0x00,
    FlexEnd         = 0x01,
    Center          = 0x02,
    SpaceBetween    = 0x03,
    SpaceAround     = 0x04,
    SpaceEvenly     = 0x05
};

// Align items values
enum class CSSAlignItems : uint8_t {
    FlexStart   = 0x00,
    FlexEnd     = 0x01,
    Center      = 0x02,
    Stretch     = 0x03,
    Baseline    = 0x04
};

// Flex wrap values
enum class CSSFlexWrap : uint8_t {
    NoWrap      = 0x00,
    Wrap        = 0x01,
    WrapReverse = 0x02
};

// Text align values
enum class CSSTextAlign : uint8_t {
    Left    = 0x00,
    Right   = 0x01,
    Center  = 0x02,
    Justify = 0x03
};

// Font weight values (numeric)
enum class CSSFontWeight : uint16_t {
    Thin        = 100,
    ExtraLight  = 200,
    Light       = 300,
    Normal      = 400,
    Medium      = 500,
    SemiBold    = 600,
    Bold        = 700,
    ExtraBold   = 800,
    Black       = 900
};

// Font style values
enum class CSSFontStyle : uint8_t {
    Normal  = 0x00,
    Italic  = 0x01,
    Oblique = 0x02
};

// Text decoration values
enum class CSSTextDecoration : uint8_t {
    None        = 0x00,
    Underline   = 0x01,
    LineThrough = 0x02,
    Overline    = 0x03
};

// Text transform values
enum class CSSTextTransform : uint8_t {
    None       = 0x00,
    Uppercase  = 0x01,
    Lowercase  = 0x02,
    Capitalize = 0x03
};

// Visibility values
enum class CSSVisibility : uint8_t {
    Visible = 0x00,
    Hidden  = 0x01
};

// Overflow values
enum class CSSOverflow : uint8_t {
    Visible = 0x00,
    Hidden  = 0x01,
    Scroll  = 0x02,
    Auto    = 0x03
};

// Pointer events values
enum class CSSPointerEvents : uint8_t {
    Auto = 0x00,
    None = 0x01
};

// Cursor values
enum class CSSCursor : uint8_t {
    Auto        = 0x00,
    Pointer     = 0x01,
    Text        = 0x02,
    Move        = 0x03,
    NotAllowed  = 0x04,
    Grab        = 0x05,
    Grabbing    = 0x06,
    Default     = 0x07,
    Crosshair   = 0x08,
    Help        = 0x09,
    Wait        = 0x0A,
    Progress    = 0x0B
};

// Border style values
enum class CSSBorderStyle : uint8_t {
    None    = 0x00,
    Solid   = 0x01,
    Dashed  = 0x02,
    Dotted  = 0x03,
    Double  = 0x04,
    Groove  = 0x05,
    Ridge   = 0x06,
    Inset   = 0x07,
    Outset  = 0x08
};

// White space values
enum class CSSWhiteSpace : uint8_t {
    Normal  = 0x00,
    NoWrap  = 0x01,
    Pre     = 0x02,
    PreWrap = 0x03,
    PreLine = 0x04
};

// Text overflow values
enum class CSSTextOverflow : uint8_t {
    Clip     = 0x00,
    Ellipsis = 0x01
};

// Timing function values
enum class CSSTimingFunction : uint8_t {
    Linear      = 0x00,
    Ease        = 0x01,
    EaseIn      = 0x02,
    EaseOut     = 0x03,
    EaseInOut   = 0x04
};

// Gradient type
enum class CSSGradientType : uint8_t {
    Linear = 0x00,
    Radial = 0x01
};

// ============================================================================
// PSEUDO-CLASS FLAGS (bit flags for state rules)
// ============================================================================

enum class CSSPseudoClass : uint8_t {
    None        = 0x00,
    Hover       = 0x01,
    Active      = 0x02,
    Focus       = 0x04,
    Disabled    = 0x08,
    FirstChild  = 0x10,
    LastChild   = 0x20,
    NthChild    = 0x40  // Requires additional data
};

// Allow bitwise operations on CSSPseudoClass
inline CSSPseudoClass operator|(CSSPseudoClass a, CSSPseudoClass b) {
    return static_cast<CSSPseudoClass>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline CSSPseudoClass operator&(CSSPseudoClass a, CSSPseudoClass b) {
    return static_cast<CSSPseudoClass>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

// ============================================================================
// BINARY DATA STRUCTURES
// ============================================================================

#pragma pack(push, 1)

// CSS Length Value (3 bytes)
struct UCSLength {
    int16_t value;          // Value (can be negative for margins)
    CSSLengthUnit unit;     // Unit type
    
    UCSLength() : value(0), unit(CSSLengthUnit::Px) {}
    UCSLength(int16_t v, CSSLengthUnit u) : value(v), unit(u) {}
};

static_assert(sizeof(UCSLength) == 3, "UCSLength must be exactly 3 bytes");

// CSS Color Value (4 bytes)
struct UCSColor {
    uint8_t r, g, b, a;
    
    UCSColor() : r(0), g(0), b(0), a(255) {}
    UCSColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
};

static_assert(sizeof(UCSColor) == 4, "UCSColor must be exactly 4 bytes");

// CSS Box Shadow (14 bytes)
struct UCSBoxShadow {
    int16_t offsetX;        // X offset in pixels
    int16_t offsetY;        // Y offset in pixels
    uint16_t blurRadius;    // Blur radius
    uint16_t spreadRadius;  // Spread radius
    UCSColor color;         // Shadow color
    uint8_t inset;          // 1 = inset, 0 = outset
    uint8_t reserved;       // Padding for alignment
    
    UCSBoxShadow() 
        : offsetX(0), offsetY(0), blurRadius(0), spreadRadius(0)
        , color(), inset(0), reserved(0) {}
};

static_assert(sizeof(UCSBoxShadow) == 14, "UCSBoxShadow must be exactly 14 bytes");

// CSS Property Entry (variable size, header is 2 bytes)
struct UCSPropertyHeader {
    CSSPropertyId propertyId;   // Property identifier
    CSSValueType valueType;     // Value type
    // Value data follows (variable size based on valueType)
};

static_assert(sizeof(UCSPropertyHeader) == 2, "UCSPropertyHeader must be exactly 2 bytes");

// CSS Rule Entry Header (6 bytes)
struct UCSRuleHeader {
    uint16_t selectorId;    // Reference to selector in selector table
    uint16_t specificity;   // Pre-calculated specificity score
    uint16_t propertyCount; // Number of properties in this rule
    // Properties follow immediately after
};

static_assert(sizeof(UCSRuleHeader) == 6, "UCSRuleHeader must be exactly 6 bytes");

// CSS Variable Entry Header
struct UCSVariableHeader {
    uint16_t nameId;        // Reference to variable name in string table
    CSSValueType valueType; // Type of value
    uint8_t valueSize;      // Size of value data in bytes
    // Value data follows
};

static_assert(sizeof(UCSVariableHeader) == 4, "UCSVariableHeader must be exactly 4 bytes");

// Selector Entry Header
struct UCSSelectorHeader {
    uint32_t hash;              // Hash of selector string for fast matching
    uint16_t stringOffset;      // Offset in string table
    uint16_t stringLength;      // Length of selector string
    CSSPseudoClass pseudoClass; // Pseudo-class flags
    uint8_t reserved;           // Padding
};

static_assert(sizeof(UCSSelectorHeader) == 10, "UCSSelectorHeader must be exactly 10 bytes");

// Gradient Stop (5 bytes)
struct UCSGradientStop {
    UCSColor color;     // Stop color
    uint8_t position;   // Position 0-255 (maps to 0.0-1.0)
};

static_assert(sizeof(UCSGradientStop) == 5, "UCSGradientStop must be exactly 5 bytes");

// Linear Gradient Header
struct UCSLinearGradient {
    uint16_t angle;     // Angle in degrees (0-360)
    uint8_t stopCount;  // Number of color stops
    // Gradient stops follow
};

static_assert(sizeof(UCSLinearGradient) == 3, "UCSLinearGradient must be exactly 3 bytes");

// Radial Gradient Header
struct UCSRadialGradient {
    uint8_t shape;      // 0 = circle, 1 = ellipse
    uint8_t stopCount;  // Number of color stops
    // Gradient stops follow
};

static_assert(sizeof(UCSRadialGradient) == 2, "UCSRadialGradient must be exactly 2 bytes");

#pragma pack(pop)

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Calculate specificity from selector components
// Format: 0x0ABC where A=IDs, B=classes/attributes, C=elements
inline uint16_t CalculateSpecificity(uint8_t ids, uint8_t classes, uint8_t elements) {
    return static_cast<uint16_t>((ids << 8) | (classes << 4) | elements);
}

// Simple hash function for selector strings (FNV-1a)
inline uint32_t HashSelector(const std::string& selector) {
    uint32_t hash = 2166136261u;
    for (char c : selector) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

// Get size of value data based on type
inline size_t GetValueSize(CSSValueType type) {
    switch (type) {
        case CSSValueType::Enum:        return 1;
        case CSSValueType::Color:       return 4;
        case CSSValueType::Length:      return 3;
        case CSSValueType::StringRef:   return 2;
        case CSSValueType::Float:       return 4;
        case CSSValueType::Integer:     return 4;
        case CSSValueType::Percentage:  return 4;
        case CSSValueType::Time:        return 4;
        case CSSValueType::Shadow:      return 14;
        case CSSValueType::Auto:        return 0;
        case CSSValueType::None:        return 0;
        case CSSValueType::Inherit:     return 0;
        case CSSValueType::VarRef:      return 2;
        case CSSValueType::MultiLength: return 0;  // Variable
        case CSSValueType::Gradient:    return 0;  // Variable
        default:                        return 0;
    }
}

} // namespace UltraWeb
