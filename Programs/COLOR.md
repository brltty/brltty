# BRLTTY Color Conversion Library

This library provides comprehensive color conversion and description functions for BRLTTY.

## Overview

The color conversion library (`color.c` and `color.h`) provides:

- Bidirectional conversion between RGB and VGA 16-color palette
  - Accurate distance-based method (`rgbToVga`)
  - Fast bit-manipulation method (`rgbToVgaFast`)
  - Optional 8-color mode (no bright bit)
- Bidirectional conversion between RGB and HSV color spaces
- ANSI 256-color to RGB conversion
- Human-readable color descriptions based on HSV analysis
  - Extensive named color detection (brown, pink, teal, etc.)
  - HSV-based modifier system for saturation and brightness
- Color interpolation in HSV and RGB spaces
- Standard VGA color palette definitions

## Data Structures

### RGBColor

Represents a color in RGB (Red, Green, Blue) color space.

```c
typedef struct {
  unsigned char r;  /* Red component (0-255) */
  unsigned char g;  /* Green component (0-255) */
  unsigned char b;  /* Blue component (0-255) */
} RGBColor;
```

### HSVColor

Represents a color in HSV (Hue, Saturation, Value) color space.

```c
typedef struct {
  float h;  /* Hue (0-360 degrees) */
  float s;  /* Saturation (0.0-1.0) */
  float v;  /* Value/Brightness (0.0-1.0) */
} HSVColor;
```

### HSVModifier

Describes a modifier for HSV color components (saturation or brightness).

```c
typedef struct {
  const char *name;     /* Modifier name (e.g., "Dark", "Bright", "Pale", "Vivid") */
  const char *comment;  /* Descriptive explanation of the modifier */
  unsigned int optional:1;  /* If 1, this modifier can be omitted for brevity */
  unsigned int lowest:1;    /* If 1, this is the lowest level for this component */
  unsigned int highest:1;   /* If 1, this is the highest level for this component */
} HSVModifier;
```

## VGA Color Palette

The library uses the standard 16-color VGA palette following the IRGB bit pattern:
- bit 0 = Blue, bit 1 = Green, bit 2 = Red, bit 3 = Intensity

| Code | Color Name      | RGB Values      | Bit Pattern (IRGB) |
|------|----------------|-----------------|--------------------|
| 0    | Black          | (0, 0, 0)       | 0000               |
| 1    | Blue           | (0, 0, 170)     | 0001               |
| 2    | Green          | (0, 170, 0)     | 0010               |
| 3    | Cyan           | (0, 170, 170)   | 0011               |
| 4    | Red            | (170, 0, 0)     | 0100               |
| 5    | Magenta        | (170, 0, 170)   | 0101               |
| 6    | Brown          | (170, 85, 0)    | 0110*              |
| 7    | Light Gray     | (170, 170, 170) | 0111               |
| 8    | Dark Gray      | (85, 85, 85)    | 1000               |
| 9    | Light Blue     | (85, 85, 255)   | 1001               |
| 10   | Light Green    | (85, 255, 85)   | 1010               |
| 11   | Light Cyan     | (85, 255, 255)  | 1011               |
| 12   | Light Red      | (255, 85, 85)   | 1100               |
| 13   | Light Magenta  | (255, 85, 255)  | 1101               |
| 14   | Yellow         | (255, 255, 85)  | 1110               |
| 15   | White          | (255, 255, 255) | 1111               |

\* Brown (index 6) is a hardware exception - the IRGB pattern would produce dark yellow (170, 170, 0), but CGA monitors had special circuitry to reduce the green component to 85.

## Functions

### VGA Color Conversion

#### vgaToRgb

```c
RGBColor vgaToRgb(int vga);
```

Converts a VGA color code (0-15) to RGB values.

**Parameters:**
- `vga`: VGA color code (automatically clamped to 0-15 range)

**Returns:** RGBColor structure with the corresponding RGB values

**Example:**
```c
RGBColor rgb = vgaToRgb(12);  /* Light Blue */
/* rgb.r = 85, rgb.g = 85, rgb.b = 255 */
```

#### rgbToVga

```c
int rgbToVga(unsigned char r, unsigned char g, unsigned char b, int noBrightBit);
```

Converts RGB color values to the nearest VGA color code using Euclidean distance calculation in RGB color space.

**Parameters:**
- `r`: Red component (0-255)
- `g`: Green component (0-255)
- `b`: Blue component (0-255)
- `noBrightBit`: If non-zero, only use VGA colors 0-7 (no bright/intensity bit)

**Returns:** VGA color code (0-15 or 0-7 if noBrightBit) of the closest matching color

**Example:**
```c
int vga = rgbToVga(200, 100, 50, 0);  /* Returns closest VGA color from all 16 */
int vga8 = rgbToVga(200, 100, 50, 1); /* Returns closest VGA color from first 8 */
```

#### rgbColorToVga

```c
int rgbColorToVga(RGBColor color, int noBrightBit);
```

Convenience wrapper for `rgbToVga()` that accepts an RGBColor structure.

**Example:**
```c
RGBColor color = {200, 100, 50};
int vga = rgbColorToVga(color, 0);
```

#### rgbToVgaFast

```c
int rgbToVgaFast(unsigned char r, unsigned char g, unsigned char b, int noBrightBit);
```

Fast RGB to VGA conversion using bit-based quantization. Significantly faster than distance-based search for most cases by exploiting the regular pattern of VGA color intensities.

**Parameters:**
- `r`: Red component (0-255)
- `g`: Green component (0-255)
- `b`: Blue component (0-255)
- `noBrightBit`: If non-zero, only use VGA colors 0-7 (no bright/intensity bit)

**Returns:** VGA color code (0-15 or 0-7 if noBrightBit)

**Algorithm:**
1. Quantizes each RGB component to nearest VGA intensity level (0x00, 0x55, 0xAA, 0xFF)
2. Uses bit manipulation to determine VGA color code from quantized values
3. Handles special cases like Brown (color 6) and ambiguous colors
4. Falls back to distance comparison for edge cases

**Example:**
```c
int vga = rgbToVgaFast(255, 85, 85, 0);  /* Returns 12 (Light Red) */
```

#### rgbColorToVgaFast

```c
int rgbColorToVgaFast(RGBColor color, int noBrightBit);
```

Convenience wrapper for `rgbToVgaFast()` that accepts an RGBColor structure.

**Example:**
```c
RGBColor color = {255, 85, 85};
int vga = rgbColorToVgaFast(color, 0);
```

#### vgaColorPalette

```c
const RGBColor *vgaColorPalette(void);
```

Returns a pointer to the standard VGA palette array (16 colors).

**Returns:** Pointer to static array of 16 RGBColor structures

**Example:**
```c
const RGBColor *palette = vgaColorPalette();
for (int i = 0; i < 16; i++) {
  printf("Color %d: R=%d G=%d B=%d\n", i,
         palette[i].r, palette[i].g, palette[i].b);
}
```

#### vgaColorName

```c
const char *vgaColorName(int vga);
```

Returns the standard name for a VGA color code.

**Parameters:**
- `vga`: VGA color code

**Returns:** Static string with the color name (e.g. "Red", "Light Blue")

**Example:**
```c
const char *name = vgaColorName(12);  /* Returns "Light Blue" */
```

### ANSI 256-Color Conversion

#### ansiToRgb

```c
RGBColor ansiToRgb(unsigned int code);
```

Converts an ANSI 256-color code to RGB values.

**Parameters:**
- `code`: ANSI color code (0-255)

**Returns:** RGBColor structure with the corresponding RGB values

**Color Ranges:**
- 0-15: Standard ANSI colors (uses VGA palette with swapped red/blue bits)
- 16-231: 6x6x6 color cube (216 colors)
- 232-255: Grayscale ramp (24 shades)

**Example:**
```c
RGBColor rgb = ansiToRgb(196);  /* Bright red from 256-color palette */
```

### HSV Color Conversion

#### rgbToHsv

```c
HSVColor rgbToHsv(unsigned char r, unsigned char g, unsigned char b);
```

Converts RGB color values to HSV color space.

**Parameters:**
- `r`: Red component (0-255)
- `g`: Green component (0-255)
- `b`: Blue component (0-255)

**Returns:** HSVColor structure with h (0-360), s (0-1), v (0-1)

**Example:**
```c
HSVColor hsv = rgbToHsv(255, 0, 0);
/* hsv.h ≈ 0, hsv.s = 1.0, hsv.v = 1.0 (pure red) */
```

#### rgbColorToHsv

```c
HSVColor rgbColorToHsv(RGBColor color);
```

Convenience wrapper for `rgbToHsv()` that accepts an RGBColor structure.

#### hsvToRgb

```c
RGBColor hsvToRgb(float h, float s, float v);
```

Converts HSV color values to RGB color space.

**Parameters:**
- `h`: Hue (0-360 degrees, automatically normalized)
- `s`: Saturation (0.0-1.0, automatically clamped)
- `v`: Value/Brightness (0.0-1.0, automatically clamped)

**Returns:** RGBColor structure

**Example:**
```c
RGBColor rgb = hsvToRgb(120.0, 1.0, 1.0);  /* Pure green */
/* rgb.r = 0, rgb.g = 255, rgb.b = 0 */
```

#### hsvColorToRgb

```c
RGBColor hsvColorToRgb(HSVColor color);
```

Convenience wrapper for `hsvToRgb()` that accepts an HSVColor structure.

#### hsvNormalize

```c
void hsvNormalize(float *h, float *s, float *v);
```

Normalizes HSV color component values to their valid ranges.

**Parameters:**
- `h`: Pointer to hue value (will be normalized to 0-360 degrees)
- `s`: Pointer to saturation value (will be clamped to 0.0-1.0)
- `v`: Pointer to value/brightness (will be clamped to 0.0-1.0)

**Note:** Any parameter can be NULL if normalization is not needed for that component.

**Example:**
```c
float h = 400.0, s = 1.5, v = -0.1;
hsvNormalize(&h, &s, &v);
/* h = 40.0, s = 1.0, v = 0.0 */
```

#### hsvColorNormalize

```c
void hsvColorNormalize(HSVColor *hsv);
```

Convenience wrapper for `hsvNormalize()` that accepts an HSVColor structure pointer.

### HSV Analysis Functions

#### hsvHueName

```c
const char *hsvHueName(float hue);
```

Returns the name for the specified hue angle.

**Parameters:**
- `hue`: Hue value in degrees (0-360)

**Returns:** Static string with the hue name

**Hue Ranges:**
- Red: 0-15°, 345-360°
- Orange: 15-45°
- Yellow: 45-75°
- Yellow-Green: 75-105°
- Green: 105-135°
- Cyan-Green: 135-165°
- Cyan: 165-195°
- Blue-Cyan: 195-225°
- Blue: 225-255°
- Violet: 255-285°
- Magenta: 285-315°
- Red-Magenta: 315-345°

#### hsvSaturationModifier

```c
const HSVModifier *hsvSaturationModifier(float saturation);
```

Returns the HSV modifier descriptor for the specified saturation level.

**Parameters:**
- `saturation`: Saturation value (0.0-1.0)

**Returns:** Pointer to static HSVModifier structure

**Saturation Levels:**
- Pale (< 0.1): Minimal color presence
- Faint (0.1-0.2): Subdued color presence
- Soft (0.2-0.4): Low color presence
- Moderate (0.4-0.5): Balanced color presence (optional)
- Vibrant (0.5-0.7): High color presence
- Rich (0.7-0.85): Strong color presence
- Full (0.85-0.95): Intense color presence
- Pure (> 0.95): Absolute color presence (highest)

#### hsvBrightnessModifier

```c
const HSVModifier *hsvBrightnessModifier(float brightness);
```

Returns the HSV modifier descriptor for the specified brightness (value) level.

**Parameters:**
- `brightness`: Value/brightness (0.0-1.0)

**Returns:** Pointer to static HSVModifier structure

**Brightness Levels:**
- Faded (< 0.2): Almost black, very little light (lowest)
- Dark (0.2-0.4): Low light, deep shades
- Medium (0.4-0.7): Balanced light (optional)
- Light (0.7-0.85): High light level
- Bright (> 0.85): Almost white, very bright (highest)

### Color Description

#### hsvColorToDescription

```c
const char *hsvColorToDescription(char *buffer, size_t bufferSize, HSVColor hsv);
```

Generates a human-readable description of an HSV color.

**Parameters:**
- `buffer`: Output buffer (recommended minimum 64 bytes)
- `bufferSize`: Size of the output buffer
- `hsv`: HSVColor structure

**Returns:** Pointer to buffer containing the color description

**Description Algorithm:**

1. **Special Cases:**
   - Very dark colors (V < 0.08) → "Black"
   - Low saturation (S < 0.08) → shades of gray:
     - V > 0.92 → "White"
     - V > 0.65 → "Light Gray"
     - V < 0.35 → "Dark Gray"
     - Otherwise → "Gray"
   - **Common named colors** (checked before generic hue names):
     - **Olive**: Dark yellow-green (60-120°, V: 0.48-0.55, S > 0.85) → "Olive"
     - **Brown**: Orange/yellow hues (15-90°) with V < 0.7 and S > 0.3 → "Brown" or "Dark Brown" (V < 0.42)
     - **Tan/Beige**: Desaturated orange (15-75°, V > 0.6, S: 0.08-0.35) → "Tan" or "Beige" (V > 0.85)
     - **Pink**: Desaturated red (0-15°, 345-360°, V > 0.7, S: 0.15-0.5) → "Pink" or "Light Pink" (S > 0.27)
     - **Coral**: Orange-pink (5-25°, V > 0.7, S: 0.4-0.75) → "Coral"
     - **Lime**: Bright yellow-green (90-135°, V > 0.75, S > 0.75) → "Lime"
     - **Teal**: Cyan (165-195°, V: 0.35-0.75, S > 0.4) → "Teal" or "Dark Teal" (V < 0.5)
     - **Turquoise**: Bright cyan (170-200°, V > 0.75, S > 0.6) → "Turquoise"
     - **Maroon**: Very dark red (0-15°, 345-360°, V: 0.45-0.55, S > 0.9) → "Maroon"
     - **Navy**: Very dark blue (210-270°, V: 0.45-0.55, S > 0.9) → "Navy"
     - **Indigo**: Blue-violet (255-285°, V: 0.35-0.75, S > 0.5) → "Indigo"
     - **Lavender**: Pale violet/magenta (230-280°, V > 0.85, S: 0.05-0.2) → "Lavender"
     - **Gold**: Saturated yellow-orange (40-60°, V > 0.7, S > 0.6) → "Gold"

2. **Generic Color Descriptions** (for colors not matching special cases):
   - Combines optional brightness and saturation modifiers with hue name
   - Format: "[Brightness] [Saturation] Hue"
   - Modifiers marked as "optional" are omitted for moderate values

**Examples:**
```c
char buffer[64];
HSVColor hsv;

hsv = (HSVColor){0.0, 1.0, 1.0};
hsvColorToDescription(buffer, sizeof(buffer), hsv);
/* Returns: "Pure Red" */

hsv = (HSVColor){240.0, 0.6, 0.3};
hsvColorToDescription(buffer, sizeof(buffer), hsv);
/* Returns: "Dark Blue" */
```

#### hsvToDescription

```c
const char *hsvToDescription(char *buffer, size_t bufferSize, float h, float s, float v);
```

Convenience function that accepts individual HSV components instead of an HSVColor structure.

**Example:**
```c
char buffer[64];
hsvToDescription(buffer, sizeof(buffer), 120.0, 0.8, 0.6);
```

#### rgbToDescription

```c
const char *rgbToDescription(char *buffer, size_t bufferSize,
                             unsigned char r, unsigned char g, unsigned char b);
```

Generates a human-readable description of an RGB color by converting to HSV and using `hsvColorToDescription()`.

**Parameters:**
- `buffer`: Output buffer (recommended minimum 64 bytes)
- `bufferSize`: Size of the output buffer
- `r`: Red component (0-255)
- `g`: Green component (0-255)
- `b`: Blue component (0-255)

**Returns:** Pointer to buffer containing the color description

**Examples:**
```c
char buffer[64];

rgbToDescription(buffer, sizeof(buffer), 255, 0, 0);
/* Returns: "Pure Red" */

rgbToDescription(buffer, sizeof(buffer), 50, 50, 150);
/* Returns: "Dark Blue" */

rgbToDescription(buffer, sizeof(buffer), 10, 10, 10);
/* Returns: "Black" */

rgbToDescription(buffer, sizeof(buffer), 245, 245, 245);
/* Returns: "White" */

rgbToDescription(buffer, sizeof(buffer), 170, 85, 0);
/* Returns: "Brown" (VGA Brown color) */

rgbToDescription(buffer, sizeof(buffer), 101, 67, 33);
/* Returns: "Dark Brown" */

rgbToDescription(buffer, sizeof(buffer), 255, 192, 203);
/* Returns: "Pink" */

rgbToDescription(buffer, sizeof(buffer), 255, 127, 80);
/* Returns: "Coral" */

rgbToDescription(buffer, sizeof(buffer), 128, 128, 0);
/* Returns: "Olive" */

rgbToDescription(buffer, sizeof(buffer), 50, 205, 50);
/* Returns: "Lime" */

rgbToDescription(buffer, sizeof(buffer), 0, 128, 128);
/* Returns: "Teal" */

rgbToDescription(buffer, sizeof(buffer), 64, 224, 208);
/* Returns: "Turquoise" */

rgbToDescription(buffer, sizeof(buffer), 128, 0, 0);
/* Returns: "Maroon" */

rgbToDescription(buffer, sizeof(buffer), 0, 0, 128);
/* Returns: "Navy" */

rgbToDescription(buffer, sizeof(buffer), 75, 0, 130);
/* Returns: "Indigo" */

rgbToDescription(buffer, sizeof(buffer), 230, 230, 250);
/* Returns: "Lavender" */

rgbToDescription(buffer, sizeof(buffer), 255, 215, 0);
/* Returns: "Gold" */

rgbToDescription(buffer, sizeof(buffer), 210, 180, 140);
/* Returns: "Tan" */

rgbToDescription(buffer, sizeof(buffer), 245, 245, 220);
/* Returns: "Beige" */
```

#### rgbColorToDescription

```c
const char *rgbColorToDescription(char *buffer, size_t bufferSize, RGBColor color);
```

Convenience wrapper for `rgbToDescription()` that accepts an RGBColor structure.

### Color Interpolation

#### hsvColorInterpolate

```c
HSVColor hsvColorInterpolate(HSVColor hsv1, HSVColor hsv2, float factor);
```

Interpolates between two HSV colors.

**Parameters:**
- `hsv1`: First HSV color
- `hsv2`: Second HSV color
- `factor`: Interpolation factor (0.0 = hsv1, 1.0 = hsv2)

**Returns:** Interpolated HSVColor

**Example:**
```c
HSVColor red = {0.0, 1.0, 1.0};
HSVColor blue = {240.0, 1.0, 1.0};
HSVColor purple = hsvColorInterpolate(red, blue, 0.5);
/* purple will be a color halfway between red and blue in HSV space */
```

#### rgbColorInterpolate

```c
RGBColor rgbColorInterpolate(RGBColor rgb1, RGBColor rgb2, float factor);
```

Interpolates between two RGB colors using HSV color space for more perceptually uniform results.

**Parameters:**
- `rgb1`: First RGB color
- `rgb2`: Second RGB color
- `factor`: Interpolation factor (0.0 = rgb1, 1.0 = rgb2)

**Returns:** Interpolated RGBColor

**Algorithm:**
1. Converts both RGB colors to HSV
2. Interpolates in HSV space
3. Converts result back to RGB

**Example:**
```c
RGBColor red = {255, 0, 0};
RGBColor blue = {0, 0, 255};
RGBColor purple = rgbColorInterpolate(red, blue, 0.5);
```

## Usage Examples

### Example 1: Converting Terminal Colors

```c
#include "color.h"
#include <stdio.h>

/* Convert ANSI 256-color code to description */
void describe_ansi_color(unsigned int ansi_color) {
    char description[64];

    /* Convert ANSI color to RGB */
    RGBColor rgb = ansiToRgb(ansi_color);

    /* Get human-readable description */
    rgbColorToDescription(description, sizeof(description), rgb);
    printf("ANSI color %u: RGB(%u,%u,%u) = %s\n",
           ansi_color, rgb.r, rgb.g, rgb.b, description);
}
```

### Example 2: Color Picker Tool

```c
#include "color.h"
#include <stdio.h>

void analyze_color(unsigned char r, unsigned char g, unsigned char b) {
    char description[64];
    HSVColor hsv;
    int vgaColor;
    int vgaColorFast;

    /* Get HSV values */
    hsv = rgbToHsv(r, g, b);

    /* Get description */
    rgbToDescription(description, sizeof(description), r, g, b);

    /* Find closest VGA color using both methods */
    vgaColor = rgbToVga(r, g, b, 0);
    vgaColorFast = rgbToVgaFast(r, g, b, 0);

    printf("RGB: (%u, %u, %u)\n", r, g, b);
    printf("HSV: (%.1f°, %.2f, %.2f)\n", hsv.h, hsv.s, hsv.v);
    printf("Description: %s\n", description);
    printf("Closest VGA (accurate): %d (%s)\n", vgaColor, vgaColorName(vgaColor));
    printf("Closest VGA (fast): %d (%s)\n", vgaColorFast, vgaColorName(vgaColorFast));

    /* Show saturation and brightness modifiers */
    const HSVModifier *satMod = hsvSaturationModifier(hsv.s);
    const HSVModifier *brightMod = hsvBrightnessModifier(hsv.v);
    printf("Saturation: %s (%s)\n", satMod->name, satMod->comment);
    printf("Brightness: %s (%s)\n", brightMod->name, brightMod->comment);
}
```

### Example 3: Color Interpolation

```c
#include "color.h"

/* Interpolate between two colors using HSV */
RGBColor interpolate_colors(RGBColor color1, RGBColor color2, float t) {
    /* Use the built-in interpolation function */
    return rgbColorInterpolate(color1, color2, t);
}

/* Create a color gradient */
void create_gradient(RGBColor start, RGBColor end, int steps) {
    char description[64];
    for (int i = 0; i < steps; i++) {
        float t = (float)i / (float)(steps - 1);
        RGBColor color = rgbColorInterpolate(start, end, t);
        rgbColorToDescription(description, sizeof(description), color);
        printf("Step %d: RGB(%u,%u,%u) - %s\n", i, color.r, color.g, color.b, description);
    }
}
```

## Algorithm Details

### RGB to VGA Conversion

Two methods are provided for RGB to VGA conversion, each with different performance and accuracy characteristics:

#### Accurate Method: `rgbToVga()`

Uses Euclidean distance in RGB color space to find the closest VGA color:

```
distance² = (R₁ - R₂)² + (G₁ - G₂)² + (B₁ - B₂)²
```

This method provides the most accurate color matching. The function iterates through all 16 VGA colors (or 8 if `noBrightBit` is set) and returns the one with minimum distance. An early exit optimization is used when an exact match (distance = 0) is found.

**Complexity:** O(n) where n is 16 or 8

#### Fast Method: `rgbToVgaFast()`

Uses bit-based quantization and manipulation to quickly determine VGA color:

1. **Component Quantization:** Each RGB component is quantized to the nearest VGA intensity level:
   - 0-42 → 0x00 (CI_OFF)
   - 43-127 → 0x55 (CI_DIM)
   - 128-212 → 0xAA (CI_REG)
   - 213-255 → 0xFF (CI_MAX)

2. **Special Case Detection:** Brown (color 6) is detected as a special case: {0xAA, 0x55, 0x00}

3. **Bit Pattern Analysis:** Uses the IRGB bit pattern to determine color:
   - Detects bright colors using bit trick: `(qr | qg | qb) & 0x55`
   - For bright colors (8-15): Uses `component & (component << 1)` to detect MAX values
   - For dark colors (0-7): Uses `component >> 7` to detect REG values

4. **Ambiguity Resolution:** For ambiguous cases (e.g., pure colors that could be dark or bright), compares distance to both options

**Complexity:** O(1) with small constant factor

**Performance:** Significantly faster than distance-based search for most cases

### RGB to HSV Conversion

The conversion follows the standard algorithm:

1. Normalize RGB values to [0, 1]
2. Calculate Value: `V = max(R, G, B)`
3. Calculate Saturation: `S = (V - min(R, G, B)) / V` (or 0 if V = 0)
4. Calculate Hue based on which component is maximum:
   - If R is max: `H = 60° × ((G - B) / delta)`
   - If G is max: `H = 60° × (2 + (B - R) / delta)`
   - If B is max: `H = 60° × (4 + (R - G) / delta)`
   - Adjust H to [0, 360°) range

### HSV to RGB Conversion

The reverse conversion uses the standard formula:

1. Calculate chroma: `C = V × S`
2. Calculate intermediate value: `X = C × (1 - |((H/60°) mod 2) - 1|)`
3. Map to RGB based on hue sector (0-60°, 60-120°, etc.)
4. Add brightness offset: `RGB = (R', G', B') + (V - C)`

## Performance Considerations

- **VGA conversion (accurate)**: O(n) with n=16 or n=8, performs distance calculations (can exit early on exact match)
- **VGA conversion (fast)**: O(1) using bit manipulation, significantly faster for most cases
- **HSV conversions**: Use floating-point arithmetic
- **Color descriptions**: Use static strings (no dynamic allocation)
- **Color interpolation**: Converts to HSV, interpolates, then converts back to RGB
- **ANSI color conversion**: O(1) lookup/calculation
- **All functions are thread-safe** (no mutable global state)

## Notes

- The VGA palette uses standard PC/DOS colors, matching most terminal emulators
- HSV color space is more intuitive for color manipulation than RGB
- Color descriptions are designed for accessibility (screen readers, braille displays)
- Buffer size of 64 bytes is recommended for color descriptions to accommodate all possible combinations
- **Choosing RGB to VGA conversion method:**
  - Use `rgbToVgaFast()` for performance-critical code (real-time rendering, large batch conversions)
  - Use `rgbToVga()` when accuracy is paramount and performance is not critical
  - For most use cases, `rgbToVgaFast()` provides excellent results with much better performance
- The `noBrightBit` parameter allows restricting to 8-color mode for terminals/devices that don't support the bright/intensity bit
- Color interpolation uses HSV space for more perceptually uniform gradients compared to RGB interpolation

## See Also

- `Headers/color_types.h` - Header file with type definitions
- `Headers/color.h` - Header file with function declarations
- `Programs/color.c` - Implementation file
- `Programs/colortest.c` - test program
- `Drivers/Screen/Tmux/screen.c` - Example usage in tmux driver (RGB to VGA conversion for ANSI sequences)
