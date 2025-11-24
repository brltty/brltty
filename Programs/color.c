/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include <stdio.h>
#include "color.h"

typedef enum {
  CI_OFF = 0X00,
  CI_DIM = 0X55,
  CI_REG = 0XAA,
  CI_MAX = 0XFF,
} ColorIntensity;

/* Standard VGA 16-color palette in RGB values
 * These are the standard colors used by VGA text mode and many terminal emulators
 *
 * Color indices follow the IRGB bit pattern:
 *   bit 0 = Blue, bit 1 = Green, bit 2 = Red, bit 3 = Intensity
 *
 * Note: Color 6 (Brown) is a hardware exception. The IRGB bit pattern (0110)
 * would produce dark yellow {0xAA, 0xAA, 0x00}, but CGA monitors had special
 * circuitry to reduce the green component to 0x55, creating brown. This became
 * the standard for EGA/VGA backward compatibility.
 */
static const RGBColor vgaPalette[VGA_COLOR_COUNT] = {
  /* Black */         [ 0] = {CI_OFF, CI_OFF, CI_OFF},
  /* Blue */          [ 1] = {CI_OFF, CI_OFF, CI_REG},
  /* Green */         [ 2] = {CI_OFF, CI_REG, CI_OFF},
  /* Cyan */          [ 3] = {CI_OFF, CI_REG, CI_REG},
  /* Red */           [ 4] = {CI_REG, CI_OFF, CI_OFF},
  /* Magenta */       [ 5] = {CI_REG, CI_OFF, CI_REG},
  /* Brown */         [ 6] = {CI_REG, CI_DIM, CI_OFF}, /* Hardware exception: not CI_REG for green */
  /* Light Gray */    [ 7] = {CI_REG, CI_REG, CI_REG},
  /* Dark Gray */     [ 8] = {CI_DIM, CI_DIM, CI_DIM},
  /* Light Blue */    [ 9] = {CI_DIM, CI_DIM, CI_MAX},
  /* Light Green */   [10] = {CI_DIM, CI_MAX, CI_DIM},
  /* Light Cyan */    [11] = {CI_DIM, CI_MAX, CI_MAX},
  /* Light Red */     [12] = {CI_MAX, CI_DIM, CI_DIM},
  /* Light Magenta */ [13] = {CI_MAX, CI_DIM, CI_MAX},
  /* Yellow */        [14] = {CI_MAX, CI_MAX, CI_DIM},
  /* White */         [15] = {CI_MAX, CI_MAX, CI_MAX}
};

const RGBColor*
vgaColorPalette(void) {
  return vgaPalette;
}

RGBColor
vgaToRgb(int vga) {
  /* Clamp vgaColor to valid range */
  if (vga < 0) vga = 0;
  if (vga >= VGA_COLOR_COUNT) vga = VGA_COLOR_COUNT - 1;

  return vgaPalette[vga];
}

/* Calculate squared Euclidean distance between two colors in RGB space
 * We use squared distance to avoid expensive sqrt() operation
 */
static int
colorDistanceSquared(
  unsigned char r1, unsigned char g1, unsigned char b1,
  unsigned char r2, unsigned char g2, unsigned char b2
) {
  int dr = (int)r1 - (int)r2;
  int dg = (int)g1 - (int)g2;
  int db = (int)b1 - (int)b2;

  return (dr * dr) + (dg * dg) + (db * db);
}

/* Quantize an RGB component to the nearest VGA intensity level
 * VGA has 4 intensity levels: 0x00 (0), 0x55 (85), 0xAA (170), 0xFF (255)
 * Midpoints for rounding:
 *   0-42    -> 0x00 (closer to 0 than to 85)
 *   43-127  -> 0x55 (closer to 85 than to 0 or 170)
 *   128-212 -> 0xAA (closer to 170 than to 85 or 255)
 *   213-255 -> 0xFF (closer to 255 than to 170)
 */
static inline unsigned char
quantizeComponent(unsigned char value) {
  /* Simple threshold-based quantization */
  if (value < 43) {
    return CI_OFF;  /* 0x00 */
  } else if (value < 128) {
    return CI_DIM;  /* 0x55 */
  } else if (value < 213) {
    return CI_REG;  /* 0xAA */
  } else {
    return CI_MAX;  /* 0xFF */
  }
}

/* Fast RGB to VGA conversion using bit-based quantization
 * This is significantly faster than distance-based search for most cases
 * by exploiting the regular pattern of VGA color intensities
 */
int
rgbToVgaFast(unsigned char r, unsigned char g, unsigned char b, int noBrightBit) {
  /* Quantize each component to nearest VGA intensity */
  unsigned char qr = quantizeComponent(r);
  unsigned char qg = quantizeComponent(g);
  unsigned char qb = quantizeComponent(b);

  /* Special case: Brown (color 6)
   * Brown is {0xAA, 0x55, 0x00} - a hardware quirk
   * Must check this before general bright color processing
   */
  if (!noBrightBit && qr == CI_REG && qg == CI_DIM && qb == CI_OFF) {
    return 6;  /* Brown */
  }

  /* VGA color encoding uses IRGB pattern:
   * bit 0 (1) = Blue
   * bit 1 (2) = Green
   * bit 2 (4) = Red
   * bit 3 (8) = Intensity/Brightness
   *
   * VGA palette structure:
   * Colors 0-7 (no bright bit): Use OFF (0x00) or REG (0xAA) for color components
   * Colors 8-15 (bright bit set): Use DIM (0x55) as base, MAX (0xFF) for color
   *   Exception: Color 8 (Dark Gray) uses DIM (0x55) for all components
   */

  int vgaColor = 0;

  /* Decide if this should be a bright color (colors 8-15)
   * Use the brightness bit if any component is DIM or MAX
   */
  int useBrightBit = 0;
  if (!noBrightBit) {
    if (qr == CI_DIM || qr == CI_MAX ||
        qg == CI_DIM || qg == CI_MAX ||
        qb == CI_DIM || qb == CI_MAX) {
      useBrightBit = 1;
      vgaColor |= VGA_BIT_BRIGHT;
    }
  }

  /* Set the RGB color bits based on the intensity levels */
  if (useBrightBit) {
    /* Bright colors (8-15): Color bits set if component is MAX */
    if (qr == CI_MAX) vgaColor |= VGA_BIT_RED;
    if (qg == CI_MAX) vgaColor |= VGA_BIT_GREEN;
    if (qb == CI_MAX) vgaColor |= VGA_BIT_BLUE;
  } else {
    /* Dark colors (0-7): Color bits set if component is REG or higher */
    if (qr >= CI_REG) vgaColor |= VGA_BIT_RED;
    if (qg >= CI_REG) vgaColor |= VGA_BIT_GREEN;
    if (qb >= CI_REG) vgaColor |= VGA_BIT_BLUE;
  }

  /* If the quantized color matches the input exactly, we can return immediately */
  RGBColor result = vgaPalette[vgaColor];
  if (result.r == r && result.g == g && result.b == b) {
    return vgaColor;
  }

  /* For ambiguous cases (e.g., pure colors that could be dark or bright),
   * check both possibilities and pick the closer one
   */
  if (useBrightBit && (qr == CI_MAX || qg == CI_MAX || qb == CI_MAX)) {
    /* Check if there's only one MAX component and others are OFF
     * e.g., (255, 0, 0) could be Red (4) or Light Red (12)
     */
    int maxCount = (qr == CI_MAX ? 1 : 0) + (qg == CI_MAX ? 1 : 0) + (qb == CI_MAX ? 1 : 0);
    int offCount = (qr == CI_OFF ? 1 : 0) + (qg == CI_OFF ? 1 : 0) + (qb == CI_OFF ? 1 : 0);

    if (maxCount >= 1 && offCount >= 1) {
      /* Compare distance to both bright and dark versions */
      int darkColor = vgaColor & ~VGA_BIT_BRIGHT;
      if (darkColor < VGA_COLOR_COUNT) {
        int distBright = colorDistanceSquared(r, g, b,
                                             vgaPalette[vgaColor].r,
                                             vgaPalette[vgaColor].g,
                                             vgaPalette[vgaColor].b);
        int distDark = colorDistanceSquared(r, g, b,
                                           vgaPalette[darkColor].r,
                                           vgaPalette[darkColor].g,
                                           vgaPalette[darkColor].b);
        if (distDark < distBright) {
          return darkColor;
        }
      }
    }
  }

  return vgaColor;
}

/* Wrapper for rgbToVgaFast(() */
int
rgbColorToVgaFast (RGBColor rgb, int noBrightBit) {
  return rgbToVgaFast(rgb.r, rgb.g, rgb.b, noBrightBit);
}

int
rgbToVga(unsigned char r, unsigned char g, unsigned char b, int noBrightBit) {
  /* Find the closest VGA color by minimum Euclidean distance in RGB space
   * This is similar to the approximation used in the tmux driver but more accurate
   */
  int closestColor = 0;
  int minDistance = colorDistanceSquared(r, g, b,
                                         vgaPalette[0].r,
                                         vgaPalette[0].g,
                                         vgaPalette[0].b);

  /* Check all of the VGA colors */
  int count = VGA_COLOR_COUNT;
  if (noBrightBit) count >>= 1;

  for (int i=1; i<count; i+=1) {
    int distance = colorDistanceSquared(r, g, b,
                                       vgaPalette[i].r,
                                       vgaPalette[i].g,
                                       vgaPalette[i].b);

    if (distance < minDistance) {
      minDistance = distance;
      closestColor = i;

      /* Early exit if we found an exact match */
      if (distance == 0) {
        break;
      }
    }
  }

  return closestColor;
}

int
rgbColorToVga(RGBColor color, int noBrightBit) {
  return rgbToVga(color.r, color.g, color.b, noBrightBit);
}

/* Color names for each VGA color code */
static const char *vgaColorNames[VGA_COLOR_COUNT] = {
  [ 0] = "Black",
  [ 1] = "Blue",
  [ 2] = "Green",
  [ 3] = "Cyan",
  [ 4] = "Red",
  [ 5] = "Magenta",
  [ 6] = "Brown",
  [ 7] = "Light Gray",
  [ 8] = "Dark Gray",
  [ 9] = "Light Blue",
  [10] = "Light Green",
  [11] = "Light Cyan",
  [12] = "Light Red",
  [13] = "Light Magenta",
  [14] = "Yellow",
  [15] = "White"
};

const char *
vgaColorName(int vga) {
  if (vga< 0) return NULL;
  if (vga>= VGA_COLOR_COUNT) return NULL;
  return vgaColorNames[vga];
}

HSVColor
rgbToHsv(unsigned char r, unsigned char g, unsigned char b) {
  HSVColor hsv;
  float rf = r / 255.0f;
  float gf = g / 255.0f;
  float bf = b / 255.0f;

  float max = rf;
  if (gf > max) max = gf;
  if (bf > max) max = bf;

  float min = rf;
  if (gf < min) min = gf;
  if (bf < min) min = bf;

  float delta = max - min;

  /* Value */
  hsv.v = max;

  /* Saturation */
  if (max > 0.0f) {
    hsv.s = delta / max;
  } else {
    hsv.s = 0.0f;
  }

  /* Hue */
  if (delta == 0.0f) {
    hsv.h = 0.0f;  /* Undefined, but we'll use 0 */
  } else {
    if (max == rf) {
      hsv.h = 60.0f * (((gf - bf) / delta) + (gf < bf ? 6.0f : 0.0f));
    } else if (max == gf) {
      hsv.h = 60.0f * (((bf - rf) / delta) + 2.0f);
    } else {
      hsv.h = 60.0f * (((rf - gf) / delta) + 4.0f);
    }
  }

  return hsv;
}

HSVColor
rgbColorToHsv(RGBColor color) {
  return rgbToHsv(color.r, color.g, color.b);
}

void
hsvNormalize(float *h, float *s, float *v) {
  if (h) {
    while (*h < 0.0f) *h += 360.0f;
    while (*h >= 360.0f) *h -= 360.0f;
  }

  if (s) {
    if (*s < 0.0f) *s = 0.0f;
    if (*s > 1.0f) *s = 1.0f;
  }

  if (v) {
    if (*v < 0.0f) *v = 0.0f;
    if (*v > 1.0f) *v = 1.0f;
  }
}

void
hsvColorNormalize(HSVColor *hsv) {
  hsvNormalize(&hsv->h, &hsv->s, &hsv->v);
}

RGBColor
hsvToRgb(float h, float s, float v) {
  RGBColor rgb;

  /* Clamp inputs */
  hsvNormalize(&h, &s, &v);

  if (s == 0.0f) {
    /* Achromatic (gray) */
    rgb.r = rgb.g = rgb.b = (unsigned char)(v * 255.0f + 0.5f);
    return rgb;
  }

  /* Standard HSV to RGB conversion algorithm */
  float c = v * s;  /* Chroma */
  float h_prime = h / 60.0f;
  float h_mod = h_prime - 2.0f * (int)(h_prime / 2.0f);  /* h_prime mod 2.0 */
  float x_temp = h_mod - 1.0f;
  float x = c * (1.0f - (x_temp < 0.0f ? -x_temp : x_temp));  /* abs(h_mod - 1.0) */
  float m = v - c;

  float rf, gf, bf;

  int hi = (int)h_prime;
  switch (hi) {
    case 0: rf = c; gf = x; bf = 0.0f; break;
    case 1: rf = x; gf = c; bf = 0.0f; break;
    case 2: rf = 0.0f; gf = c; bf = x; break;
    case 3: rf = 0.0f; gf = x; bf = c; break;
    case 4: rf = x; gf = 0.0f; bf = c; break;
    case 5: rf = c; gf = 0.0f; bf = x; break;
    default: rf = 0.0f; gf = 0.0f; bf = 0.0f; break;
  }

  rgb.r = (unsigned char)((rf + m) * 255.0f + 0.5f);
  rgb.g = (unsigned char)((gf + m) * 255.0f + 0.5f);
  rgb.b = (unsigned char)((bf + m) * 255.0f + 0.5f);

  return rgb;
}

RGBColor
hsvColorToRgb(HSVColor color) {
  return hsvToRgb(color.h, color.s, color.v);
}

/* Helper function to get hue name */
static const char *
getHueName(float hue) {
  /* Hue ranges (degrees):
   * Red: 0-15, 345-360 (30° total, wraps around 0°)
   * Orange: 15-45
   * Yellow: 45-75
   * Yellow-Green: 75-105
   * Green: 105-135
   * Cyan-Green: 135-165
   * Cyan: 165-195
   * Blue-Cyan: 195-225
   * Blue: 225-255
   * Violet: 255-285
   * Magenta: 285-315
   * Red-Magenta: 315-345
   */

  static const char *const hueNames[] = {
    /*   0° */ "Red",
    /*  30° */ "Orange",
    /*  60° */ "Yellow",
    /*  90° */ "Yellow-Green",
    /* 120° */ "Green",
    /* 150° */ "Cyan-Green",
    /* 180° */ "Cyan",
    /* 210° */ "Blue-Cyan",
    /* 240° */ "Blue",
    /* 270° */ "Violet",
    /* 300° */ "Magenta",
    /* 330° */ "Red-Magenta",
  };

  hue += 15.0f;
  hsvNormalize(&hue, NULL, NULL);
  return hueNames[(int)hue / 30];
}

const char *
hsvColorToDescription(char *buffer, size_t bufferSize, HSVColor hsv) {
  hsvNormalize(&hsv.h, &hsv.s, &hsv.v);

  /* Handle special cases */
  if (hsv.v < 0.08f) {
    snprintf(buffer, bufferSize, "Black");
    return buffer;
  }

  if (hsv.s < 0.08f) {
    /* Achromatic - shades of gray */
    if (hsv.v > 0.92f) {
      snprintf(buffer, bufferSize, "White");
    } else if (hsv.v > 0.65f) {
      snprintf(buffer, bufferSize, "Light Gray");
    } else if (hsv.v > 0.35f) {
      snprintf(buffer, bufferSize, "Gray");
    } else {
      snprintf(buffer, bufferSize, "Dark Gray");
    }
    return buffer;
  }

  /* Special case for olive: dark yellow-green - check BEFORE brown */
  if ((hsv.h >= 60.0f && hsv.h < 120.0f) && hsv.v >= 0.48f && hsv.v < 0.55f && hsv.s > 0.85f) {
    snprintf(buffer, bufferSize, "Olive");
    return buffer;
  }

  /* Special case for brown: dark orange/yellow with low brightness */
  if ((hsv.h >= 15.0f && hsv.h < 90.0f) && hsv.v < 0.7f && hsv.s > 0.3f) {
    if (hsv.v < 0.42f) {
      snprintf(buffer, bufferSize, "Dark Brown");
    } else {
      snprintf(buffer, bufferSize, "Brown");
    }
    return buffer;
  }

  /* Special case for tan/beige: very desaturated brown/orange */
  if ((hsv.h >= 15.0f && hsv.h < 75.0f) && hsv.v > 0.6f && hsv.s < 0.35f && hsv.s > 0.08f) {
    if (hsv.v > 0.85f) {
      snprintf(buffer, bufferSize, "Beige");
    } else {
      snprintf(buffer, bufferSize, "Tan");
    }
    return buffer;
  }

  /* Special case for pink: light/desaturated red */
  if ((hsv.h < 15.0f || hsv.h >= 345.0f) && hsv.v > 0.7f && hsv.s < 0.5f && hsv.s > 0.15f) {
    /* Web color standards: Pink (255,192,203) S=0.247, LightPink (255,182,193) S=0.286
     * Counterintuitively, "LightPink" has higher saturation than "Pink" */
    if (hsv.s > 0.27f) {
      snprintf(buffer, bufferSize, "Light Pink");
    } else {
      snprintf(buffer, bufferSize, "Pink");
    }
    return buffer;
  }

  /* Special case for coral: orangish-pink */
  if ((hsv.h >= 5.0f && hsv.h < 25.0f) && hsv.v > 0.7f && hsv.s > 0.4f && hsv.s < 0.75f) {
    snprintf(buffer, bufferSize, "Coral");
    return buffer;
  }

  /* Special case for lime: bright yellow-green */
  if ((hsv.h >= 90.0f && hsv.h < 135.0f) && hsv.v > 0.75f && hsv.s > 0.75f) {
    snprintf(buffer, bufferSize, "Lime");
    return buffer;
  }

  /* Special case for teal: cyan with moderate saturation and value */
  if ((hsv.h >= 165.0f && hsv.h < 195.0f) && hsv.v > 0.35f && hsv.v < 0.75f && hsv.s > 0.4f) {
    if (hsv.v < 0.5f) {
      snprintf(buffer, bufferSize, "Dark Teal");
    } else {
      snprintf(buffer, bufferSize, "Teal");
    }
    return buffer;
  }

  /* Special case for turquoise: bright cyan */
  if ((hsv.h >= 170.0f && hsv.h < 200.0f) && hsv.v > 0.75f && hsv.s > 0.6f) {
    snprintf(buffer, bufferSize, "Turquoise");
    return buffer;
  }

  /* Special case for maroon: very dark red */
  if ((hsv.h < 15.0f || hsv.h >= 345.0f) && hsv.v >= 0.45f && hsv.v <= 0.55f && hsv.s > 0.9f) {
    snprintf(buffer, bufferSize, "Maroon");
    return buffer;
  }

  /* Special case for navy: very dark blue */
  if ((hsv.h >= 210.0f && hsv.h < 270.0f) && hsv.v >= 0.45f && hsv.v <= 0.55f && hsv.s > 0.9f) {
    snprintf(buffer, bufferSize, "Navy");
    return buffer;
  }

  /* Special case for indigo: between blue and violet */
  if ((hsv.h >= 255.0f && hsv.h < 285.0f) && hsv.v > 0.35f && hsv.v < 0.75f && hsv.s > 0.5f) {
    snprintf(buffer, bufferSize, "Indigo");
    return buffer;
  }

  /* Special case for lavender: pale violet/magenta */
  if ((hsv.h >= 230.0f && hsv.h < 280.0f) && hsv.v > 0.85f && hsv.s < 0.2f && hsv.s > 0.05f) {
    snprintf(buffer, bufferSize, "Lavender");
    return buffer;
  }

  /* Special case for gold: saturated yellow with orange tint */
  if ((hsv.h >= 40.0f && hsv.h < 60.0f) && hsv.v > 0.7f && hsv.s > 0.6f) {
    snprintf(buffer, bufferSize, "Gold");
    return buffer;
  }

  /* Get the base hue name */
  const char *hueName = getHueName(hsv.h);

  /* Determine brightness modifier */
  const char *brightnessModifier = "";
  if (hsv.v < 0.2f) {
    /* Almost black, very little light */
    brightnessModifier = "Faded";
  } else if (hsv.v < 0.4f) {
    /* Low light, shades are deep */
    brightnessModifier = "Dark";
  } else if (hsv.v > 0.85f) {
    /* Almost white, very bright and clear */
    brightnessModifier = "Bright";
  } else if (hsv.v > 0.7f) {
    /* High light level, easily visible */
    brightnessModifier = "Light";
  } else {
    /* Balanced light, clear but not bright */
  //brightnessModifier = "Medium";
  }

  /* Determine saturation modifier */
  const char *saturationModifier = "";
  if (hsv.s < 0.1f) {
    /* Very little color, mostly gray */
    saturationModifier = "Pale";
  } else if (hsv.s < 0.2f) {
    /* Minimum color presence, almost indistinguishable from gray */
    saturationModifier = "Faint";
  } else if (hsv.s < 0.4f) {
    /* Low color presence, muted and lacking vibrancy */
    saturationModifier = "Dull";
  } else if (hsv.s > 0.98f) {
    /* Absolute color presence, vivid and unaltered */
    saturationModifier = "Pure";
  } else if (hsv.s > 0.95f) {
    /* Maximum color presence, pure and striking */
    saturationModifier = "Full";
  } else if (hsv.s > 0.8f) {
    /* Strong color presence, full-bodied and vivid */
    saturationModifier = "Rich";
  } else if (hsv.s > 0.6f) {
    /* High color presence, noticeable and lively */
    saturationModifier = "Vibrant";
  } else {
    /* Moderate color presence, gentle and not overpowering */
  //saturationModifier = "Soft";
  }

  /* Combine modifiers and hue name */
  snprintf(
    buffer, bufferSize,
    "%s%s%s%s%s",
    brightnessModifier, (*brightnessModifier? " ": ""),
    saturationModifier, (*saturationModifier? " ": ""),
    hueName
  );

  return buffer;
}

const char *
hsvToDescription(char *buffer, size_t bufferSize, float h, float s, float v) {
  HSVColor hsv = {.h=h, .s=s, .v=v};
  return hsvColorToDescription(buffer, bufferSize, hsv);
}

const char *
rgbToDescription(char *buffer, size_t bufferSize, unsigned char r, unsigned char g, unsigned char b) {
  return hsvColorToDescription(buffer, bufferSize, rgbToHsv(r, g, b));
}

const char *
rgbColorToDescription(char *buffer, size_t bufferSize, RGBColor color) {
  return rgbToDescription(buffer, bufferSize, color.r, color.g, color.b);
}

/* Convert ANSI 256-color code to RGB */
RGBColor
ansiToRgb (unsigned int code) {
  RGBColor rgb;

  if (code < 16) {
    /* Standard ANSI colors */
    int vga = code & (VGA_BIT_BRIGHT | VGA_BIT_GREEN);
    if (code & VGA_BIT_BLUE) vga |= VGA_BIT_RED;
    if (code & VGA_BIT_RED) vga |= VGA_BIT_BLUE;
    rgb = vgaPalette[vga];
  } else if (code < 232) {
    /* 6x6x6 color cube */
    code -= 16;

    const int multiplier = 51;
    const int granularity = 6;

    rgb.b = (code % granularity) * multiplier;
    code /= granularity;

    rgb.g = (code % granularity) * multiplier;
    rgb.r = (code / granularity) * multiplier;
  } else {
    /* Grayscale */
    unsigned int gray = 8 + ((code - 232) * 10);
    rgb.r = rgb.g = rgb.b = gray;
  }

  return rgb;
}

/* Interpolate between two HSV colors */
HSVColor
hsvColorInterpolate (HSVColor hsv1, HSVColor hsv2, float factor) {
  HSVColor result = {
    .h = hsv1.h + ((hsv2.h - hsv1.h) * factor),
    .s = hsv1.s + ((hsv2.s - hsv1.s) * factor),
    .v = hsv1.v + ((hsv2.v - hsv1.v) * factor),
  };

  return result;
}

/* Interpolate between two RGB colors using HSV */
RGBColor
rgbColorInterpolate (RGBColor rgb1, RGBColor rgb2, float factor) {
  return hsvColorToRgb(hsvColorInterpolate(rgbColorToHsv(rgb1), rgbColorToHsv(rgb2), factor));
}
