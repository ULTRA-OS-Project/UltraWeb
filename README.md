# UltraWeb Project Description

## Document Information
- **Project Name:** UltraWeb
- **Version:** 1.3.0
- **Created:** 2025-06-02
- **Author:** UltraCanvas Framework Team
- **Status:** Planning / Initial Development

---

## Executive Summary

UltraWeb is a revolutionary web application platform that replaces traditional browser rendering (HTML/CSS/JavaScript) with a high-performance binary protocol powered by UltraCanvas. It delivers web applications with minimal data transfer, maximum performance, and consistent cross-platform rendering by using compiled JavaScript bytecode, binary CSS, and a binary UI definition format.

**Core Philosophy:** The browser becomes a thin display client. All rendering is handled by UltraCanvas WASM, eliminating DOM overhead entirely.

---

## Project Goals

### Primary Objectives

1. **Minimal Data Transfer**
   - Binary formats reduce payload size by 60-85% compared to traditional web
   - Delta updates send only changed elements (2-5KB per interaction vs 50KB+)
   - LZ4 compression for all transfers

2. **Maximum Performance**
   - Zero DOM manipulation overhead
   - Pre-compiled JavaScript bytecode (Hermes) eliminates parsing
   - Pre-calculated CSS specificity and property resolution
   - Native-speed rendering via UltraCanvas WASM

3. **Unified Rendering**
   - Pixel-perfect consistency across all browsers
   - No browser-specific CSS quirks
   - Single rendering engine (UltraCanvas)

4. **Developer Familiarity**
   - CSS syntax for styling (compiled to binary)
   - JavaScript for application logic (compiled to bytecode)
   - Component-based UI architecture

5. **Server-Side Compilation**
   - All compilation happens on the server
   - Client receives optimized binary bundles
   - Reduces client-side processing requirements

### Secondary Objectives

- Progressive enhancement for legacy browser fallback
- Crawler-optimized static HTML generation for SEO, AI crawlers, and link
  previews (see *Crawler & Fallback Rendering*)
- Offline-first capability with efficient caching
- Real-time collaborative applications via WebSocket
- Seamless integration with existing UltraCanvas components
- Mobile-optimized touch event handling

---

## Architecture Overview

### System Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              SERVER SIDE                                    │
│                                                                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────────┐  │
│  │   Source Files  │  │   Compilers     │  │   Binary Outputs            │  │
│  │                 │  │                 │  │                             │  │
│  │  ┌───────────┐  │  │  ┌───────────┐  │  │  ┌───────────────────────┐  │  │
│  │  │ .js files │──┼──┼─▶│ Hermes   │──┼──┼─▶│ .hbc (bytecode)       │  │  │
│  │  └───────────┘  │  │  └───────────┘  │  │  └───────────────────────┘  │  │
│  │                 │  │                 │  │                             │  │
│  │  ┌───────────┐  │  │  ┌───────────┐  │  │  ┌───────────────────────┐  │  │
│  │  │ .css files│──┼──┼─▶│ UCSCompile│──┼──┼─▶│ .ucs (binary styles) │  │  │
│  │  └───────────┘  │  │  └───────────┘  │  │  └───────────────────────┘  │  │
│  │                 │  │                 │  │                             │  │
│  │  ┌───────────┐  │  │  ┌───────────┐  │  │  ┌───────────────────────┐  │  │
│  │  │ .ucml     │──┼──┼─▶│ UCBCompile│─┼──┼─▶│ .ucb (binary UI)      │  │  │
│  │  └───────────┘  │  │  └───────────┘  │  │  └───────────────────────┘  │  │
│  │                 │  │                 │  │                             │  │
│  │  ┌───────────┐  │  │  ┌───────────┐  │  │  ┌───────────────────────┐  │  │
│  │  │ assets    │──┼──┼─▶│ UCAPack  │──┼──┼─▶│ .uca (binary assets)  │  │  │
│  │  └───────────┘  │  │  └───────────┘  │  │  └───────────────────────┘  │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────────────────┘  │
│                                                      │                      │
│                              ┌───────────────────────▼───────────────────┐  │
│                              │         UltraWeb Bundler                  │  │
│                              │         Creates .ucpkg bundle             │  │
│                              │         Applies LZ4 compression           │  │
│                              └───────────────────────┬───────────────────┘  │
└──────────────────────────────────────────────────────┼──────────────────────┘
                                                       │
                                          HTTP/2 or WebSocket
                                                       │
┌──────────────────────────────────────────────────────┼──────────────────────┐
│                              BROWSER SIDE            │                      │
│                                                      ▼                      │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │                    UltraWeb Runtime (WASM)                            │  │
│  │                                                                       │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────┐    │  │
│  │  │ Package Loader  │  │ Hermes VM       │  │ Style Engine        │    │  │
│  │  │ - Decompress    │  │ - Execute .hbc  │  │ - Apply .ucs        │    │  │
│  │  │ - Parse bundle  │  │ - JS runtime    │  │ - State management  │    │  │
│  │  └────────┬────────┘  └────────┬────────┘  └──────────┬──────────┘    │  │
│  │           │                    │                      │               │  │
│  │           ▼                    ▼                      ▼               │  │
│  │  ┌────────────────────────────────────────────────────────────────┐   │  │
│  │  │                    UltraCanvas Renderer                        │   │  │
│  │  │  - Canvas 2D API (primary)                                     │   │  │
│  │  │  - WebGL (optional, for 3D)                                    │   │  │
│  │  │  - Full UltraCanvas component library                          │   │  │
│  │  └────────────────────────────────────────────────────────────────┘   │  │
│  │                                    │                                  │  │
│  │  ┌─────────────────────────────────▼──────────────────────────────┐   │  │
│  │  │                    Event Handler                               │   │  │
│  │  │  - Mouse / Touch / Keyboard                                    │   │  │
│  │  │  - Maps to UltraCanvas events                                  │   │  │
│  │  │  - Dispatches to Hermes callbacks                              │   │  │
│  │  └────────────────────────────────────────────────────────────────┘   │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                          │                                  │
│                                          ▼                                  │
│                                 ┌─────────────────┐                         │
│                                 │  HTML5 Canvas   │ ◀── Single DOM element │
│                                 └─────────────────┘                         │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Data Flow

1. **Development Phase**
   - Developer writes JS, CSS, and UCML (UI markup)
   - Server compiles to binary formats
   - Bundler creates optimized .ucpkg package

2. **Initial Load**
   - Browser requests page
   - Server sends UltraWeb runtime (WASM) + initial .ucpkg
   - Runtime initializes and renders UI

3. **Runtime Interaction**
   - User events captured by UltraCanvas
   - Events dispatched to Hermes VM
   - JS callbacks execute application logic
   - UI updates via binary command protocol

4. **Server Communication**
   - Delta updates sent as binary commands
   - WebSocket for real-time applications
   - HTTP/2 for request-response patterns

---

## Binary File Formats

### 1. UltraCanvas Bundle (.ucb) - UI Definition

**Purpose:** Compact binary representation of the UI element tree.

```
┌────────────────────────────────────────────────────┐
│                    UCB FILE FORMAT                 │
├────────────────────────────────────────────────────┤
│ HEADER (16 bytes)                                  │
│ ┌────────────┬──────────────────────────────────┐  │
│ │ Offset     │ Field                            │  │
│ ├────────────┼──────────────────────────────────┤  │
│ │ 0x00-0x03  │ Magic: 0x55434231 ('UCB1')       │  │
│ │ 0x04-0x05  │ Version: uint16                  │  │
│ │ 0x06-0x07  │ Flags: uint16                    │  │
│ │ 0x08-0x0B  │ Element Count: uint32            │  │
│ │ 0x0C-0x0F  │ String Table Offset: uint32      │  │
│ └────────────┴──────────────────────────────────┘  │
├────────────────────────────────────────────────────┤
│ ELEMENT TABLE (variable)                           │
│ ┌──────────────────────────────────────────────┐   │
│ │ Element Entry (8-64 bytes each)              │   │
│ │ - ElementType: uint16                        │   │
│ │ - ElementID: uint16                          │   │
│ │ - ParentID: uint16                           │   │
│ │ - StyleClassCount: uint16                    │   │
│ │ - StyleClasses[]: uint16[]                   │   │
│ │ - PropertyCount: uint8                       │   │
│ │ - Properties[]: (PropertyID, Type, Value)[]  │   │
│ └──────────────────────────────────────────────┘   │
├────────────────────────────────────────────────────┤
│ STRING TABLE (variable)                            │
│ - Length-prefixed UTF-8 strings                    │
│ - Referenced by index in elements                  │
└────────────────────────────────────────────────────┘
```

**Element Types:**

| ID     | Type       | Maps To                    |
|--------|------------|----------------------------|
| 0x0001 | Container  | UltraCanvasContainer       |
| 0x0002 | FlexBox    | UltraCanvasFlexLayout      |
| 0x0003 | Grid       | UltraCanvasGridLayout      |
| 0x0004 | ScrollView | UltraCanvasScrollContainer |
| 0x0010 | Text       | UltraCanvasLabel           |
| 0x0011 | Button     | UltraCanvasButton          |
| 0x0012 | Input      | UltraCanvasTextInput       |
| 0x0013 | TextArea   | UltraCanvasTextArea        |
| 0x0014 | Checkbox   | UltraCanvasCheckbox        |
| 0x0015 | Radio      | UltraCanvasRadioButton     |
| 0x0016 | Select     | UltraCanvasDropdown        |
| 0x0017 | Slider     | UltraCanvasSlider          |
| 0x0020 | Image      | UltraCanvasImage           |
| 0x0021 | SVG        | UltraCanvasSVG             |
| 0x0022 | Canvas     | UltraCanvasDrawingSurface  |
| 0x0030 | List       | UltraCanvasListView        |
| 0x0031 | Table      | UltraCanvasTableView       |
| 0x0032 | Tree       | UltraCanvasTreeView        |
| 0x0040 | Tabs       | UltraCanvasTabbedContainer |
| 0x0041 | Modal      | UltraCanvasModal           |
| 0x0042 | Menu       | UltraCanvasMenu            |

---

### 2. UltraCanvas Styles (.ucs) - Compiled CSS

**Purpose:** Pre-parsed, binary CSS with pre-calculated specificity.

```
┌────────────────────────────────────────────────────┐
│                    UCS FILE FORMAT                 │
├────────────────────────────────────────────────────┤
│ HEADER (20 bytes)                                  │
│ ┌────────────┬──────────────────────────────────┐  │
│ │ 0x00-0x03  │ Magic: 0x55435331 ('UCS1')       │  │
│ │ 0x04-0x05  │ Version: uint16                  │  │
│ │ 0x06-0x07  │ Rule Count: uint16               │  │
│ │ 0x08-0x0B  │ Selector Table Offset: uint32    │  │
│ │ 0x0C-0x0F  │ Property Table Offset: uint32    │  │
│ │ 0x10-0x13  │ Variable Table Offset: uint32    │  │
│ └────────────┴──────────────────────────────────┘  │
├────────────────────────────────────────────────────┤
│ VARIABLE TABLE                                     │
│ - CSS custom properties (--name: value)            │
│ - Pre-resolved for fast lookup                     │
├────────────────────────────────────────────────────┤
│ SELECTOR TABLE                                     │
│ - Selector strings (hashed for matching)           │
│ - Pre-calculated specificity scores                │
├────────────────────────────────────────────────────┤
│ RULE TABLE                                         │
│ ┌──────────────────────────────────────────────┐   │
│ │ Rule Entry                                   │   │
│ │ - SelectorID: uint16                         │   │
│ │ - Specificity: uint16                        │   │
│ │ - PropertyCount: uint16                      │   │
│ │ - Properties[]: (ID, Type, Value)[]          │   │
│ └──────────────────────────────────────────────┘   │
├────────────────────────────────────────────────────┤
│ STATE RULES (hover, active, focus, disabled)       │
│ - Separate tables for each pseudo-class            │
│ - Fast state-based style switching                 │
└────────────────────────────────────────────────────┘
```

**Property Encoding:**

| Property ID | Name            | Value Type     | Bytes |
|-------------|-----------------|----------------|-------|
| 0x01        | display         | enum           | 3     |
| 0x02        | position        | enum           | 3     |
| 0x03        | flex-direction  | enum           | 3     |
| 0x04        | justify-content | enum           | 3     |
| 0x05        | align-items     | enum           | 3     |
| 0x10        | width           | length         | 5     |
| 0x11        | height          | length         | 5     |
| 0x12        | padding         | length×4       | 11    |
| 0x13        | margin          | length×4       | 11    |
| 0x20        | font-family     | string-ref     | 4     |
| 0x21        | font-size       | length         | 5     |
| 0x22        | font-weight     | uint16         | 4     |
| 0x23        | color           | rgba           | 6     |
| 0x30        | background      | color/gradient | 6-20  |
| 0x31        | border-radius   | length×4       | 11    |
| 0x32        | box-shadow      | shadow         | 14    |
| 0x33        | opacity         | float          | 5     |

**Value Type Encoding:**

| Type ID | Type       | Encoding                 |
|---------|------------|--------------------------|
| 0x00    | enum       | uint8 value              |
| 0x01    | color      | R, G, B, A (4 bytes)     |
| 0x02    | length     | int16 value + uint8 unit |
| 0x03    | string-ref | uint16 index             |
| 0x04    | float      | float32                  |
| 0x05    | gradient   | type + stops             |

---

### 3. UltraCanvas Assets (.uca) - Binary Assets

**Purpose:** Optimized asset container for images, fonts, and other resources.

```
┌────────────────────────────────────────────────────┐
│                    UCA FILE FORMAT                 │
├────────────────────────────────────────────────────┤
│ HEADER (16 bytes)                                  │
│ - Magic: 0x55434131 ('UCA1')                       │
│ - Version: uint16                                  │
│ - Asset Count: uint16                              │
│ - Total Size: uint32                               │
│ - Flags: uint32                                    │
├────────────────────────────────────────────────────┤
│ ASSET INDEX                                        │
│ ┌──────────────────────────────────────────────┐   │
│ │ Per Asset:                                   │   │
│ │ - NameHash: uint32                           │   │
│ │ - Type: uint8 (image, font, data)            │   │
│ │ - Compression: uint8                         │   │
│ │ - Offset: uint32                             │   │
│ │ - CompressedSize: uint32                     │   │
│ │ - OriginalSize: uint32                       │   │
│ └──────────────────────────────────────────────┘   │
├────────────────────────────────────────────────────┤
│ ASSET DATA                                         │
│ - Concatenated asset blobs                         │
│ - Individually compressed (LZ4)                    │
│ - Images: WebP preferred, PNG fallback             │
│ - Fonts: WOFF2 format                              │
└────────────────────────────────────────────────────┘
```

---

### 4. UltraCanvas Package (.ucpkg) - Complete Bundle

**Purpose:** Single-file distribution containing all resources.

```
┌────────────────────────────────────────────────────┐
│                   UCPKG FILE FORMAT                │
├────────────────────────────────────────────────────┤
│ HEADER (48 bytes)                                  │
│ ┌────────────┬──────────────────────────────────┐  │
│ │ 0x00-0x03  │ Magic: 0x5543504B ('UCPK')       │  │
│ │ 0x04-0x05  │ Version: uint16                  │  │
│ │ 0x06-0x07  │ Flags: uint16                    │  │
│ │ 0x08-0x0B  │ Total Size: uint32               │  │
│ │ 0x0C-0x0F  │ CRC32 Checksum: uint32           │  │
│ │ 0x10-0x13  │ UI Section Offset: uint32        │  │
│ │ 0x14-0x17  │ UI Section Size: uint32          │  │
│ │ 0x18-0x1B  │ Style Section Offset: uint32     │  │
│ │ 0x1C-0x1F  │ Style Section Size: uint32       │  │
│ │ 0x20-0x23  │ Code Section Offset: uint32      │  │
│ │ 0x24-0x27  │ Code Section Size: uint32        │  │
│ │ 0x28-0x2B  │ Asset Section Offset: uint32     │  │
│ │ 0x2C-0x2F  │ Asset Section Size: uint32       │  │
│ └────────────┴──────────────────────────────────┘  │
├────────────────────────────────────────────────────┤
│ UI SECTION (.ucb data)                             │
├────────────────────────────────────────────────────┤
│ STYLE SECTION (.ucs data)                          │
├────────────────────────────────────────────────────┤
│ CODE SECTION (.hbc bytecode)                       │
├────────────────────────────────────────────────────┤
│ ASSET SECTION (.uca data)                          │
└────────────────────────────────────────────────────┘

Compression (UCPKGFlags::Compressed, bit 0x0001): the payload after the
48-byte header is a single LZ4 frame (standard frame format, magic
04 22 4D 18) containing the concatenated sections. Header offsets, sizes
and CRC32 always describe the UNCOMPRESSED layout; readers decompress the
payload first, then apply offsets. The bundler clears the flag and stores
the payload raw when compression is unavailable or does not reduce size.
Server-side compression is provided by the VirtualFS module
(VirtualFS_CompressBuffer); the WASM runtime embeds a minimal LZ4 frame
decoder. Implemented in core/UltraWebCompression.cpp (backend wrapper),
core/UltraWebBundler.cpp (compress) and PackageReader (decompress);
enable with -DULTRAWEB_USE_VIRTUALFS=ON
-DULTRAWEB_VIRTUALFS_DIR=<UltraCanvas>/VirtualFS.
```

---

## Supported CSS Subset

### Layout Properties

```css
/* Display & Position */
display: flex | block | grid | none;
position: static | relative | absolute | fixed;

/* Flexbox */
flex-direction: row | column | row-reverse | column-reverse;
flex-wrap: nowrap | wrap | wrap-reverse;
justify-content: flex-start | flex-end | center | space-between | space-around | space-evenly;
align-items: flex-start | flex-end | center | stretch | baseline;
align-content: flex-start | flex-end | center | stretch | space-between | space-around;
flex: <grow> <shrink> <basis>;
flex-grow: <number>;
flex-shrink: <number>;
flex-basis: <length> | auto;
order: <integer>;
gap: <length>;
row-gap: <length>;
column-gap: <length>;

/* Grid */
grid-template-columns: <track-list>;
grid-template-rows: <track-list>;
grid-column: <start> / <end>;
grid-row: <start> / <end>;

/* Sizing */
width: <length> | <percentage> | auto;
height: <length> | <percentage> | auto;
min-width: <length> | <percentage>;
max-width: <length> | <percentage> | none;
min-height: <length> | <percentage>;
max-height: <length> | <percentage> | none;
```

### Box Model Properties

```css
/* Padding */
padding: <length>;
padding-top: <length>;
padding-right: <length>;
padding-bottom: <length>;
padding-left: <length>;

/* Margin */
margin: <length> | auto;
margin-top: <length> | auto;
margin-right: <length> | auto;
margin-bottom: <length> | auto;
margin-left: <length> | auto;

/* Border */
border: <width> <style> <color>;
border-width: <length>;
border-style: none | solid | dashed | dotted;
border-color: <color>;
border-radius: <length>;
border-top-left-radius: <length>;
border-top-right-radius: <length>;
border-bottom-right-radius: <length>;
border-bottom-left-radius: <length>;

/* Overflow */
overflow: visible | hidden | scroll | auto;
overflow-x: visible | hidden | scroll | auto;
overflow-y: visible | hidden | scroll | auto;
```

### Typography Properties

```css
/* Font */
font-family: <family-name>;
font-size: <length>;
font-weight: normal | bold | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900;
font-style: normal | italic;
line-height: <number> | <length>;
letter-spacing: <length>;
word-spacing: <length>;

/* Text */
color: <color>;
text-align: left | right | center | justify;
text-decoration: none | underline | line-through | overline;
text-transform: none | uppercase | lowercase | capitalize;
white-space: normal | nowrap | pre | pre-wrap | pre-line;
text-overflow: clip | ellipsis;
```

### Visual Properties

```css
/* Background */
background: <color>;
background-color: <color>;
background-image: linear-gradient(<direction>, <color-stops>);
background-image: radial-gradient(<shape>, <color-stops>);

/* Effects */
box-shadow: <offset-x> <offset-y> <blur-radius> <spread-radius> <color>;
opacity: <number>;

/* Visibility */
visibility: visible | hidden;
pointer-events: auto | none;

/* Cursor */
cursor: auto | pointer | text | move | not-allowed | grab | grabbing;

/* Transitions (limited) */
transition: <property> <duration> <timing-function>;
transition-property: all | <property-name>;
transition-duration: <time>;
transition-timing-function: linear | ease | ease-in | ease-out | ease-in-out;
```

### Pseudo-Classes

```css
/* Interactive states */
:hover { }
:active { }
:focus { }
:disabled { }

/* Structural (compile-time only) */
:first-child { }
:last-child { }
:nth-child(n) { }
```

### CSS Variables

```css
:root {
    --primary-color: #3399FF;
    --secondary-color: #FF6600;
    --text-color: #333333;
    --background-color: #FFFFFF;
    --border-radius: 8px;
    --spacing-unit: 16px;
}

.element {
    color: var(--text-color);
    background: var(--primary-color);
    border-radius: var(--border-radius);
    padding: var(--spacing-unit);
}
```

---

## JavaScript API

### Core Runtime API

```javascript
// ===== ELEMENT CREATION =====

// Create element with properties
const button = UC.createElement('Button', {
    id: 'submitBtn',
    text: 'Submit',
    className: 'btn primary large'
});

// Create with children
const container = UC.createElement('FlexBox', {
    direction: 'column',
    gap: 16,
    children: [
        UC.createElement('Text', { text: 'Title', className: 'heading' }),
        UC.createElement('Text', { text: 'Description', className: 'body' })
    ]
});

// ===== DOM-LIKE OPERATIONS =====

// Append/remove children
container.append(button);
container.prepend(header);
container.insertBefore(newElement, referenceElement);
container.removeChild(element);

// Query elements
const element = UC.getElementById('submitBtn');
const elements = UC.getElementsByClassName('btn');
const first = UC.querySelector('.container > .btn');
const all = UC.querySelectorAll('.btn.primary');

// ===== PROPERTIES & ATTRIBUTES =====

element.setText('New Text');
element.getText();
element.setProperty('disabled', true);
element.getProperty('disabled');

// ===== STYLING =====

// Class manipulation
element.addClass('active');
element.removeClass('disabled');
element.toggleClass('expanded');
element.hasClass('active');

// Inline styles
element.setStyle('background', '#FF0000');
element.setStyle({
    background: '#FF0000',
    padding: '16px',
    borderRadius: '8px'
});
element.getStyle('background');

// ===== EVENT HANDLING =====

// Add event listeners
element.on('click', (event) => {
    console.log('Clicked at', event.x, event.y);
});

element.on('mouseenter', () => element.addClass('hover'));
element.on('mouseleave', () => element.removeClass('hover'));

// Remove listeners
const handler = (e) => { /* ... */ };
element.on('click', handler);
element.off('click', handler);

// Once
element.once('click', () => console.log('First click only'));

// ===== LAYOUT & DIMENSIONS =====

element.getBounds();  // { x, y, width, height }
element.setPosition(100, 200);
element.setSize(300, 150);

// ===== VISIBILITY =====

element.show();
element.hide();
element.isVisible();
```

### State Management

```javascript
// ===== REACTIVE STATE =====

// Create state
const [count, setCount] = UC.useState(0);
const [user, setUser] = UC.useState({ name: '', email: '' });

// Bind state to element
label.bind('text', () => `Count: ${count}`);

// Update state (triggers re-render)
button.on('click', () => setCount(count + 1));

// Object state updates
setUser({ ...user, name: 'John' });

// ===== COMPUTED VALUES =====

const fullName = UC.computed(() => `${user.firstName} ${user.lastName}`);
label.bind('text', fullName);

// ===== EFFECTS =====

UC.effect(() => {
    console.log('Count changed to:', count);
    // Cleanup function (optional)
    return () => console.log('Cleanup');
});
```

### Network API

```javascript
// ===== HTTP REQUESTS =====

// GET request
const response = await UC.fetch('/api/users');
const users = response.json();

// POST request
await UC.fetch('/api/users', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ name: 'John', email: 'john@example.com' })
});

// ===== WEBSOCKET =====

const ws = UC.websocket('/ws/updates');

ws.on('open', () => console.log('Connected'));
ws.on('message', (data) => handleUpdate(data));
ws.on('close', () => console.log('Disconnected'));
ws.on('error', (err) => console.error(err));

ws.send({ type: 'subscribe', channel: 'updates' });
ws.close();
```

### Navigation & Routing

```javascript
// ===== ROUTING =====

// Define routes
UC.router.define({
    '/': HomePage,
    '/users': UsersPage,
    '/users/:id': UserDetailPage,
    '/settings': SettingsPage,
    '*': NotFoundPage
});

// Navigate programmatically
UC.router.navigate('/users/123');
UC.router.back();
UC.router.forward();

// Get current route
const route = UC.router.current();
const params = UC.router.params();  // { id: '123' }
const query = UC.router.query();    // { tab: 'profile' }
```

### Storage API

```javascript
// ===== LOCAL STORAGE =====

UC.storage.set('theme', 'dark');
const theme = UC.storage.get('theme');
UC.storage.remove('theme');
UC.storage.clear();

// ===== SESSION STORAGE =====

UC.session.set('token', 'abc123');
const token = UC.session.get('token');
```

---

## Implementation Strategy

### Phase 1: Foundation (Weeks 1-4)

**Goal:** Core binary formats and basic compilation pipeline.

| Week | Deliverables |
|------|--------------|
| 1 | UCB format specification finalization |
| 1 | UCS format specification finalization |
| 2 | CSS Parser (subset) implementation |
| 2 | UCS Binary Writer implementation |
| 3 | UCB Binary Writer implementation |
| 3 | Basic UI element encoding |
| 4 | UCPKG bundler (no compression) |
| 4 | Unit tests for all formats |

**Files to Create:**

```
UltraWeb/
├── include/
│   ├── UltraWebFormats.h         # Format constants and structures
│   ├── UltraWebCSSParser.h       # CSS tokenizer and parser
│   ├── UltraWebCSSCompiler.h     # CSS to UCS compiler
│   ├── UltraWebUICompiler.h      # UI to UCB compiler
│   └── UltraWebBundler.h         # Package bundler
├── core/
│   ├── UltraWebCSSParser.cpp
│   ├── UltraWebCSSCompiler.cpp
│   ├── UltraWebUICompiler.cpp
│   └── UltraWebBundler.cpp
└── tests/
    ├── CSSParserTests.cpp
    ├── CSSCompilerTests.cpp
    └── BundlerTests.cpp
```

### Phase 2: Runtime (Weeks 5-8)

**Goal:** WASM client runtime for loading and rendering.

| Week | Deliverables |
|------|--------------|
| 5 | UCB Loader (WASM) |
| 5 | UCS Loader and Style Engine (WASM) |
| 6 | Element factory integration |
| 6 | Style application to UltraCanvas components |
| 7 | Event binding system |
| 7 | Basic JS API bridge |
| 8 | Integration testing |
| 8 | Performance benchmarking |

**Files to Create:**

```
UltraWeb/
├── runtime/
│   ├── UltraWebRuntime.h         # Main runtime class
│   ├── UltraWebRuntime.cpp
│   ├── UCBLoader.h               # Binary UI loader
│   ├── UCBLoader.cpp
│   ├── UCSLoader.h               # Binary style loader
│   ├── UCSLoader.cpp
│   ├── StyleEngine.h             # Style resolution engine
│   ├── StyleEngine.cpp
│   ├── ElementFactory.h          # Element instantiation
│   ├── ElementFactory.cpp
│   ├── JSBridge.h                # JavaScript interop
│   └── JSBridge.cpp
└── wasm/
    ├── UltraWebWASM.cpp          # WASM entry point
    └── CMakeLists.txt            # WASM build config
```

### Phase 3: Hermes Integration (Weeks 9-12)

**Goal:** JavaScript execution via Hermes bytecode.

**Status (v1.3.0):** Core layer implemented. The runtime programs against a
`JSEngine` abstraction (`runtime/JSEngine.h`) with two backends: **Hermes**
(`HermesRuntime.*`, JSI-based, executes .hbc, build with
`ULTRAWEB_USE_HERMES` + the Hermes SDK) and **QuickJS**
(`QuickJSEngine.*`, source-only, `ULTRAWEB_USE_QUICKJS`) as the
development/CI engine — the engine fallback anticipated in Risks. All
JS↔native traffic crosses one host function
(`__uc_native(name, argsJson)` → JSON), so backends stay tiny and
behave identically. Implemented: UC element API (query, text/value,
classes, visibility, bounds), event dispatch to JS (`element.on` /
`FireDomEvent`), reactive state (`UC.useState` / `computed` / `effect`
backed by the native `StateManager`), `console.log`, code-section
execution (HBC via Hermes; plain-JS dev mode on any backend), and the
server-side `HermesCompiler` (hermesc wrapper producing .hbc).
`UC.fetch`/`UC.websocket` land with Phase 4, `UC.router`/`UC.storage`
with Phase 5 (stubs throw descriptive errors).

| Week | Deliverables |
|------|--------------|
| 9 | Hermes library integration |
| 9 | .hbc loading in WASM |
| 10 | UC JavaScript API implementation |
| 10 | Event dispatching to JS |
| 11 | State management (useState, computed) |
| 11 | Fetch API implementation |
| 12 | Full API testing |
| 12 | Documentation |

**Files to Create:**

```
UltraWeb/
├── runtime/
│   ├── HermesRuntime.h           # Hermes VM wrapper
│   ├── HermesRuntime.cpp
│   ├── UCApi.h                   # UC.* JavaScript API
│   ├── UCApi.cpp
│   ├── StateManager.h            # Reactive state system
│   └── StateManager.cpp
└── server/
    ├── HermesCompiler.h          # JS to bytecode compiler
    └── HermesCompiler.cpp
```

### Phase 4: Server Framework (Weeks 13-16)

**Goal:** Complete server-side toolchain.

| Week | Deliverables |
|------|--------------|
| 13 | Development server with hot reload |
| 13 | File watcher and incremental compilation |
| 14 | Asset optimization pipeline |
| 14 | LZ4 compression integration |
| 15 | WebSocket server for updates |
| 15 | Delta update generation |
| 16 | CLI tools |
| 16 | Build system integration |

**Files to Create:**

```
UltraWeb/
├── server/
│   ├── UltraWebServer.h          # HTTP/WebSocket server
│   ├── UltraWebServer.cpp
│   ├── DevServer.h               # Development server
│   ├── DevServer.cpp
│   ├── FileWatcher.h             # File change detection
│   ├── FileWatcher.cpp
│   ├── DeltaGenerator.h          # Incremental updates
│   └── DeltaGenerator.cpp
├── tools/
│   ├── uwc.cpp                   # UltraWeb Compiler CLI
│   ├── uws.cpp                   # UltraWeb Server CLI
│   └── uwb.cpp                   # UltraWeb Bundler CLI
└── assets/
    ├── AssetOptimizer.h          # Image/font optimization
    └── AssetOptimizer.cpp
```

### Phase 5: Polish & Ecosystem (Weeks 17-20)

**Goal:** Developer experience and production readiness.

| Week | Deliverables |
|------|--------------|
| 17 | Error reporting and debugging |
| 17 | Source maps for bytecode |
| 18 | Router implementation |
| 18 | Component library (forms, tables, etc.) |
| 19 | Documentation site |
| 19 | Example applications |
| 20 | Performance optimization |
| 20 | Release preparation |

---

## Directory Structure

```
UltraWeb/
├── include/                      # Public headers
│   ├── UltraWebFormats.h
│   ├── UltraWebCSSParser.h
│   ├── UltraWebCSSCompiler.h
│   ├── UltraWebUICompiler.h
│   ├── UltraWebBundler.h
│   ├── UltraWebRuntime.h
│   └── UltraWebApi.h
│
├── core/                         # Core implementation
│   ├── formats/
│   │   ├── UCBWriter.cpp
│   │   ├── UCBReader.cpp
│   │   ├── UCSWriter.cpp
│   │   ├── UCSReader.cpp
│   │   ├── UCAWriter.cpp
│   │   ├── UCAReader.cpp
│   │   └── UCPKGBundler.cpp
│   ├── compiler/
│   │   ├── CSSTokenizer.cpp
│   │   ├── CSSParser.cpp
│   │   ├── CSSCompiler.cpp
│   │   ├── UICompiler.cpp
│   │   ├── AssetCompiler.cpp
│   │   └── HTMLGenerator.cpp
│   └── UltraWebCompression.cpp   # LZ4 backend wrapper (VirtualFS server-side)
│
├── runtime/                      # Client runtime (WASM)
│   ├── UltraWebRuntime.cpp
│   ├── UCBLoader.cpp
│   ├── UCSLoader.cpp
│   ├── StyleEngine.cpp
│   ├── ElementFactory.cpp
│   ├── EventDispatcher.cpp
│   ├── JSBridge.cpp
│   ├── HermesRuntime.cpp
│   ├── StateManager.cpp
│   └── Router.cpp
│
├── server/                       # Server components
│   ├── UltraWebServer.cpp
│   ├── DevServer.cpp
│   ├── FileWatcher.cpp
│   ├── HermesCompiler.cpp
│   ├── DeltaGenerator.cpp
│   ├── StaticPageHandler.cpp
│   └── WebSocketHandler.cpp
│
├── api/                          # JavaScript API
│   ├── UCApi.cpp                 # UC.createElement, etc.
│   ├── UCDom.cpp                 # DOM-like operations
│   ├── UCEvents.cpp              # Event handling
│   ├── UCState.cpp               # useState, computed
│   ├── UCNetwork.cpp             # fetch, websocket
│   ├── UCStorage.cpp             # localStorage, session
│   └── UCRouter.cpp              # Routing
│
├── tools/                        # CLI tools
│   ├── uwc/                      # UltraWeb Compiler
│   │   └── main.cpp
│   ├── uws/                      # UltraWeb Server
│   │   └── main.cpp
│   └── uwb/                      # UltraWeb Bundler
│       └── main.cpp
│
├── wasm/                         # WASM build
│   ├── UltraWebWASM.cpp
│   ├── shell.html
│   └── CMakeLists.txt
│
├── tests/                        # Unit tests
│   ├── CSSParserTests.cpp
│   ├── CSSCompilerTests.cpp
│   ├── UCBTests.cpp
│   ├── UCSTests.cpp
│   ├── RuntimeTests.cpp
│   └── IntegrationTests.cpp
│
├── examples/                     # Example applications
│   ├── hello-world/
│   ├── todo-app/
│   ├── dashboard/
│   └── e-commerce/
│
├── docs/                         # Documentation
│   ├── getting-started.md
│   ├── css-reference.md
│   ├── javascript-api.md
│   ├── binary-formats.md
│   └── deployment.md
│
├── CMakeLists.txt
├── README.md
└── LICENSE
```

---

## Performance Targets

| Metric | Target | Traditional Web |
|--------|--------|-----------------|
| Initial load (runtime) | < 200KB | N/A (browser built-in) |
| Initial load (app) | < 50KB | 200-500KB |
| Time to interactive | < 500ms | 1-3 seconds |
| Per-interaction update | < 5KB | 20-100KB |
| Render frame time | < 8ms (120fps capable) | 16ms (60fps) |
| Memory usage | < 50MB baseline | 100-300MB |

---

## Crawler & Fallback Rendering

UltraWeb's binary formats (.ucb / .ucs / .ucpkg) are invisible to search
engines, AI crawlers, and link-preview bots: these clients do not download
the WASM runtime, most of them execute no JavaScript at all, and none of
them can parse UltraWeb binaries. UltraWeb therefore generates
crawler-optimized static HTML alongside the binary bundles. These files
live on the server only — a normal browser session loads the WASM runtime
and .ucpkg bundles and never requests them.

### Design Principles

1. **One source of truth.** The static HTML is emitted by an additional
   compiler backend (`HTMLGenerator`) in the same build step that produces
   .ucb/.ucs/.ucpkg — from the same UCML, CSS, and asset sources. Content
   parity between the binary application and the HTML pages is therefore
   guaranteed by construction, not by policy. This keeps the technique on
   the right side of search engines' cloaking rules: serving different
   *bytes* to crawlers is acceptable; serving different *content* is not.

2. **Build-time generation, not runtime rendering.** Pages are generated
   once at compile time (static site generation, comparable to a
   Docusaurus build), never rendered on demand by a headless browser. This
   avoids the operational problems — latency, drift, fragility — that led
   search engines to deprecate classic "dynamic rendering" setups.

3. **Plain, dependency-free HTML.** Generated pages carry no framework
   runtime and require no JavaScript: semantic HTML plus minimal inline
   CSS, in the spirit of a pre-rendered documentation page. Target < 15KB
   per page before compression.

### Generated Artifacts

| Artifact | Purpose |
|----------|---------|
| `<route>.html` (one per public route) | Semantic HTML rendering of the route's content |
| `sitemap.xml` | Route inventory for crawlers |
| `robots.txt` | Crawler policy |
| Open Graph / Twitter Card meta tags (per page) | Link previews in chat and social platforms |
| JSON-LD structured data (optional, per route) | Rich search results |

The `uwc` compiler gains an `--emit-html` option; the bundler places the
generated pages next to the .ucpkg output for the server to pick up.

### Serving Strategy

Two modes, selectable per application in the server configuration:

**Mode A — HTML-first (recommended default).** The server answers every
initial page request with the generated HTML for that route. The page
includes a small loader script; capable browsers download the WASM runtime
and .ucpkg in the background, then swap the live application in place of
the static content. Crawlers, no-JS clients, and legacy browsers simply
keep the HTML. Benefits: no bot detection at all (nothing to misclassify,
no user-agent lists to maintain), a meaningful first paint while the
runtime loads, and the legacy-browser fallback (see Secondary Objectives)
comes for free.

**Mode B — bot-only.** The server returns HTML only to verified crawlers
and the binary flow to everyone else. Verification requires user-agent
matching **plus** reverse-DNS / published-IP-range validation — a
user-agent string alone is trivially spoofed. Intended for applications
where an HTML first response is undesirable (e.g. authenticated app shells
with no public content).

### Content Parity Rules

To stay clear of cloaking penalties, generated pages MUST:

- contain the same primary content (text, headings, images, links) a user
  sees on that route in the running application;
- never contain crawler-only keywords, links, or content;
- return the same HTTP status codes as the binary route (a missing route
  must 404 in both worlds);
- carry a `rel="canonical"` link to the route's public URL.

### Explicit Non-Goals

- **Accessibility is not solved by this mechanism.** Screen readers run
  inside real browsers as normal users and receive the canvas-rendered
  application, never the crawler files. Screen reader support requires an
  accessibility tree / ARIA projection in the host page (see Open
  Questions).
- Interactive or per-user views (dashboards behind login, personalized
  data) are not generated — only publicly reachable content routes.

---

## Dependencies

### Server-Side

| Dependency | Purpose | License |
|------------|---------|---------|
| Hermes | JavaScript to bytecode compiler | MIT |
| LZ4 (via VirtualFS, UltraCanvas module) | Fast compression | BSD |
| uWebSockets | HTTP/WebSocket server | Apache 2.0 |
| libwebp | WebP image encoding | BSD |
| woff2 | Font compression | MIT |

### Client-Side (WASM)

| Dependency | Purpose | License |
|------------|---------|---------|
| UltraCanvas | UI rendering | UltraCanvas License |
| Hermes (embedded) | JS bytecode execution | MIT |
| QuickJS (optional, dev/CI only) | JS source execution where the Hermes SDK is unavailable | MIT |
| LZ4 (embedded) | Decompression | BSD |

---

## Success Criteria

1. **Bundle size reduction:** 60%+ smaller than equivalent React/Vue app
2. **Load time improvement:** 50%+ faster time-to-interactive
3. **Render performance:** Consistent 60fps on mid-range devices
4. **Developer experience:** Familiar CSS/JS workflow
5. **Cross-browser consistency:** Pixel-perfect rendering on all browsers
6. **Documentation:** Complete API reference and tutorials

---

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Hermes WASM performance | High | Fallback to V8 if needed; profile early |
| CSS subset limitations | Medium | Document limitations; provide escape hatches |
| Browser compatibility | Medium | Test matrix; progressive enhancement |
| Developer adoption | High | Excellent docs; migration tools; examples |
| Debugging complexity | Medium | Source maps; browser devtools integration |

---

## Open Questions

1. **UCML Syntax:** Should we define a custom UI markup language or use JSON/YAML?
2. **Animation System:** CSS transitions only, or full animation API?
3. **Accessibility:** How to provide screen reader support without DOM?
   Note: the crawler HTML described in *Crawler & Fallback Rendering* does
   **not** solve this — screen readers run in real browsers and receive the
   canvas-rendered application. Requires an accessibility tree / ARIA
   projection in the host page.
4. **SEO:** ~~Server-side rendering strategy for search engines?~~
   **Resolved in v1.1.0** — see *Crawler & Fallback Rendering*.
5. **Mobile:** Native app packaging (Capacitor/similar) or PWA only?

---

## References

- UltraCanvas WASM Implementation: `/OS/WASM/`
- UltraCanvas ThemeControl: `ThemeControl_README.md`
- UltraCanvas FlexLayout: `UltraCanvasFlexLayout.h`
- Hermes Engine: https://github.com/facebook/hermes
- LZ4 Compression: https://github.com/lz4/lz4

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2025-06-02 | Initial document creation |
| 1.1.0 | 2026-07-07 | Added *Crawler & Fallback Rendering* section (static HTML for crawlers, serving modes, content parity rules); resolved SEO open question; annotated accessibility open question |
| 1.2.0 | 2026-07-07 | Implemented .ucpkg LZ4 compression via VirtualFS raw-buffer API; specified compressed payload semantics (LZ4 frame after header, uncompressed offsets/CRC); restructured sources into spec directory layout |
| 1.3.0 | 2026-07-07 | Phase 3 core: JSEngine abstraction with Hermes (JSI/.hbc) and QuickJS (dev/CI) backends, UC JavaScript API (elements, classes, events, reactive state), StateManager, event dispatch to JS, code-section execution, server-side HermesCompiler (hermesc wrapper) |

---

*This document serves as the authoritative reference for UltraWeb development. All implementation decisions should align with the goals and specifications outlined herein.*
