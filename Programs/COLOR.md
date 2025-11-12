# Color Conversion Library

This library provides comprehensive color conversion and description functions for BRLTTY.

## Overview

The color conversion library (`color.c` and `color.h`) provides:

- Bidirectional conversion between RGB and VGA 16-color palette
- Bidirectional conversion between RGB and HSV color spaces
- Human-readable color descriptions based on HSV analysis
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
| 7    | Light Grey     | (170, 170, 170) | 0111               |
| 8    | Dark Grey      | (85, 85, 85)    | 1000               |
| 9    | Light Blue     | (85, 85, 255)   | 1001               |
| 10   | Light Green    | (85, 255, 85)   | 1010               |
| 11   | Yellow         | (255, 255, 85)  | 1011               |
| 12   | Light Red      | (255, 85, 85)   | 1100               |
| 13   | Light Magenta  | (255, 85, 255)  | 1101               |
| 14   | Light Cyan     | (85, 255, 255)  | 1110               |
| 15   | White          | (255, 255, 255) | 1111               |

\* Brown (index 6) is a hardware exception - the IRGB pattern would produce dark yellow (170, 170, 0), but CGA monitors had special circuitry to reduce the green component to 85.

## Functions

### VGA Color Conversion

#### vgaToRgb

```c
RGBColor vgaToRgb(int vgaColor);
```

Converts a VGA color code (0-15) to RGB values.

**Parameters:**
- `vgaColor`: VGA color code (automatically clamped to 0-15 range)

**Returns:** RGBColor structure with the corresponding RGB values

**Example:**
```c
RGBColor rgb = vgaToRgb(12);  /* Light Blue */
/* rgb.r = 85, rgb.g = 85, rgb.b = 255 */
```

#### rgbToVga

```c
int rgbToVga(unsigned char r, unsigned char g, unsigned char b);
```

Converts RGB color values to the nearest VGA color code using Euclidean distance calculation in RGB color space.

**Parameters:**
- `r`: Red component (0-255)
- `g`: Green component (0-255)
- `b`: Blue component (0-255)

**Returns:** VGA color code (0-15) of the closest matching color

**Example:**
```c
int vgaColor = rgbToVga(200, 100, 50);  /* Returns closest VGA color */
```

#### rgbColorToVga

```c
int rgbColorToVga(RGBColor color);
```

Convenience wrapper for `rgbToVga()` that accepts an RGBColor structure.

**Example:**
```c
RGBColor color = {200, 100, 50};
int vgaColor = rgbColorToVga(color);
```

#### getVgaPalette

```c
const RGBColor* getVgaPalette(void);
```

Returns a pointer to the standard VGA palette array (16 colors).

**Returns:** Pointer to static array of 16 RGBColor structures

**Example:**
```c
const RGBColor *palette = getVgaPalette();
for (int i = 0; i < 16; i++) {
  printf("Color %d: R=%d G=%d B=%d\n", i,
         palette[i].r, palette[i].g, palette[i].b);
}
```

#### getVgaColorName

```c
const char* getVgaColorName(int vgaColor);
```

Returns the standard name for a VGA color code.

**Parameters:**
- `vgaColor`: VGA color code (automatically clamped to 0-15 range)

**Returns:** Static string with the color name (e.g., "Red", "Light Blue")

**Example:**
```c
const char *name = getVgaColorName(12);  /* Returns "Light Blue" */
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

### Color Description

#### rgbToColorDescription

```c
char* rgbToColorDescription(unsigned char r, unsigned char g, unsigned char b,
                            char *buffer, size_t bufferSize);
```

Generates a human-readable description of an RGB color using HSV analysis.

**Parameters:**
- `r`: Red component (0-255)
- `g`: Green component (0-255)
- `b`: Blue component (0-255)
- `buffer`: Output buffer (recommended minimum 64 bytes)
- `bufferSize`: Size of the output buffer

**Returns:** Pointer to buffer containing the color description

**Description Algorithm:**

1. **Special Cases:**
   - Very dark colors (V < 0.08) → "black"
   - Low saturation (S < 0.08) → shades of grey based on value
   - **Common named colors** (checked before generic hue names):
     - **Brown**: Orange/yellow hues (15-90°) with low brightness (V < 0.7) and moderate saturation (S > 0.3) → "brown" or "dark brown"
     - **Tan/Beige**: Desaturated orange (15-75°) with high brightness (V > 0.6, S < 0.35) → "tan" or "beige"
     - **Pink**: Desaturated red (0-15°, 345-360°) with high brightness (V > 0.7, S < 0.5) → "pink" or "light pink"
     - **Coral**: Orange-pink (5-25°) with high brightness and moderate saturation → "coral"
     - **Olive**: Dark yellow-green (60-120°) with low brightness (V < 0.55) → "olive"
     - **Lime**: Bright yellow-green (75-105°) with high brightness and saturation (V > 0.8, S > 0.7) → "lime"
     - **Teal**: Cyan (165-195°) with moderate values (V: 0.35-0.75, S > 0.4) → "teal" or "dark teal"
     - **Turquoise**: Bright cyan (170-200°) with high brightness and saturation (V > 0.75, S > 0.6) → "turquoise"
     - **Maroon**: Very dark red (0-15°, 345-360°) with low brightness (V < 0.4, S > 0.5) → "maroon"
     - **Navy**: Very dark blue (210-270°) with low brightness (V < 0.4, S > 0.5) → "navy"
     - **Indigo**: Blue-violet (255-285°) with moderate values (V: 0.35-0.75, S > 0.5) → "indigo"
     - **Lavender**: Pale violet/magenta (270-315°) with high brightness and low saturation (V > 0.7, S < 0.5) → "lavender"
     - **Gold**: Saturated yellow-orange (40-60°) with high brightness and saturation (V > 0.7, S > 0.6) → "gold"

2. **Hue Names** (12 primary hues for colors not matching special cases):
   - Red (0-15°, 345-360°)
   - Orange (15-45°)
   - Yellow (45-75°)
   - Yellow-green (75-105°)
   - Green (105-135°)
   - Cyan-green (135-165°)
   - Cyan (165-195°)
   - Blue-cyan (195-225°)
   - Blue (225-255°)
   - Violet (255-285°)
   - Magenta (285-315°)
   - Red-magenta (315-345°)

3. **Brightness Modifiers:**
   - "very dark" (V < 0.3)
   - "dark" (V < 0.5)
   - "bright" (V > 0.85 and S > 0.6)
   - (no modifier for normal brightness)

4. **Saturation Modifiers:**
   - "pale" (S < 0.2)
   - "light" (S < 0.4)
   - "vivid" (S > 0.8 and V > 0.5)
   - "saturated" (S > 0.6 and V > 0.5)
   - (no modifier for normal saturation)

**Examples:**
```c
char buffer[64];

rgbToColorDescription(255, 0, 0, buffer, sizeof(buffer));
/* Returns: "vivid red" */

rgbToColorDescription(50, 50, 150, buffer, sizeof(buffer));
/* Returns: "dark blue" */

rgbToColorDescription(200, 150, 150, buffer, sizeof(buffer));
/* Returns: "light red" */

rgbToColorDescription(100, 200, 150, buffer, sizeof(buffer));
/* Returns: "saturated cyan-green" */

rgbToColorDescription(220, 220, 220, buffer, sizeof(buffer));
/* Returns: "light grey" */

rgbToColorDescription(255, 165, 0, buffer, sizeof(buffer));
/* Returns: "bright vivid orange" */

rgbToColorDescription(128, 0, 128, buffer, sizeof(buffer));
/* Returns: "dark magenta" */

rgbToColorDescription(10, 10, 10, buffer, sizeof(buffer));
/* Returns: "black" */

rgbToColorDescription(245, 245, 245, buffer, sizeof(buffer));
/* Returns: "white" */

rgbToColorDescription(170, 85, 0, buffer, sizeof(buffer));
/* Returns: "brown" (VGA Brown color) */

rgbToColorDescription(101, 67, 33, buffer, sizeof(buffer));
/* Returns: "dark brown" */

rgbToColorDescription(255, 192, 203, buffer, sizeof(buffer));
/* Returns: "light pink" */

rgbToColorDescription(255, 127, 80, buffer, sizeof(buffer));
/* Returns: "coral" */

rgbToColorDescription(128, 128, 0, buffer, sizeof(buffer));
/* Returns: "olive" */

rgbToColorDescription(50, 205, 50, buffer, sizeof(buffer));
/* Returns: "lime" */

rgbToColorDescription(0, 128, 128, buffer, sizeof(buffer));
/* Returns: "teal" */

rgbToColorDescription(64, 224, 208, buffer, sizeof(buffer));
/* Returns: "turquoise" */

rgbToColorDescription(128, 0, 0, buffer, sizeof(buffer));
/* Returns: "maroon" */

rgbToColorDescription(0, 0, 128, buffer, sizeof(buffer));
/* Returns: "navy" */

rgbToColorDescription(75, 0, 130, buffer, sizeof(buffer));
/* Returns: "indigo" */

rgbToColorDescription(230, 230, 250, buffer, sizeof(buffer));
/* Returns: "lavender" */

rgbToColorDescription(255, 215, 0, buffer, sizeof(buffer));
/* Returns: "gold" */

rgbToColorDescription(210, 180, 140, buffer, sizeof(buffer));
/* Returns: "tan" */

rgbToColorDescription(245, 245, 220, buffer, sizeof(buffer));
/* Returns: "beige" */
```

#### rgbColorToDescription

```c
char* rgbColorToDescription(RGBColor color, char *buffer, size_t bufferSize);
```

Convenience wrapper for `rgbToColorDescription()` that accepts an RGBColor structure.

## Usage Examples

### Example 1: Converting Terminal Colors

```c
#include "color.h"
#include <stdio.h>

/* Convert ANSI 256-color code to description */
void describe_ansi_color(int ansi_color) {
    unsigned char r, g, b;
    char description[64];

    /* Simplified 256-color to RGB conversion */
    if (ansi_color < 16) {
        RGBColor rgb = vgaToRgb(ansi_color);
        r = rgb.r; g = rgb.g; b = rgb.b;
    } else if (ansi_color < 232) {
        /* 6x6x6 color cube */
        int idx = ansi_color - 16;
        r = (idx / 36) * 51;
        g = ((idx / 6) % 6) * 51;
        b = (idx % 6) * 51;
    } else {
        /* Grayscale */
        int gray = 8 + (ansi_color - 232) * 10;
        r = g = b = gray;
    }

    rgbToColorDescription(r, g, b, description, sizeof(description));
    printf("ANSI color %d: %s\n", ansi_color, description);
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

    /* Get HSV values */
    hsv = rgbToHsv(r, g, b);

    /* Get description */
    rgbToColorDescription(r, g, b, description, sizeof(description));

    /* Find closest VGA color */
    vgaColor = rgbToVga(r, g, b);

    printf("RGB: (%d, %d, %d)\n", r, g, b);
    printf("HSV: (%.1f°, %.2f, %.2f)\n", hsv.h, hsv.s, hsv.v);
    printf("Description: %s\n", description);
    printf("Closest VGA color: %d (%s)\n", vgaColor, getVgaColorName(vgaColor));
}
```

### Example 3: Color Interpolation

```c
#include "color.h"

/* Interpolate between two colors using HSV */
RGBColor interpolate_colors(RGBColor color1, RGBColor color2, float t) {
    HSVColor hsv1 = rgbColorToHsv(color1);
    HSVColor hsv2 = rgbColorToHsv(color2);

    /* Interpolate in HSV space */
    HSVColor result;
    result.h = hsv1.h + (hsv2.h - hsv1.h) * t;
    result.s = hsv1.s + (hsv2.s - hsv1.s) * t;
    result.v = hsv1.v + (hsv2.v - hsv1.v) * t;

    return hsvColorToRgb(result);
}
```

## Algorithm Details

### RGB to VGA Conversion

The `rgbToVga()` function uses Euclidean distance in RGB color space to find the closest VGA color:

```
distance² = (R₁ - R₂)² + (G₁ - G₂)² + (B₁ - B₂)²
```

This method provides more accurate color matching than simple threshold-based approaches. The function iterates through all 16 VGA colors and returns the one with minimum distance. An early exit optimization is used when an exact match (distance = 0) is found.

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

- VGA conversion performs 16 distance calculations (can exit early)
- HSV conversions use floating-point arithmetic
- Color descriptions use static strings (no dynamic allocation)
- All functions are thread-safe (no mutable global state)

## Notes

- The VGA palette uses standard PC/DOS colors, matching most terminal emulators
- HSV color space is more intuitive for color manipulation than RGB
- Color descriptions are designed for accessibility (screen readers, braille displays)
- Buffer size of 64 bytes is recommended for color descriptions to accommodate all possible combinations

## See Also

- `Programs/color.h` - Header file with function declarations
- `Programs/color.c` - Implementation file
- `Programs/colortest.c` - test program
- `Drivers/Screen/Tmux/screen.c` - Example usage in tmux driver (RGB to VGA conversion for ANSI sequences)
