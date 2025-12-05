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

#include "prologue.h"

#include <stdio.h>

#include "color.h"
#include "color_internal.h"
#include "strfmt.h"

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
   *
   * Bit patterns:
   * CI_OFF = 0x00 = 0b00000000
   * CI_DIM = 0x55 = 0b01010101
   * CI_REG = 0xAA = 0b10101010
   * CI_MAX = 0xFF = 0b11111111
   *
   * Key insight: CI_DIM and CI_MAX share the 0x55 bit pattern, while
   * CI_OFF and CI_REG do not. We can detect bright colors (DIM or MAX)
   * using: (qr | qg | qb) & CI_DIM
   */

  int vgaColor = 0;

  /* Decide if this should be a bright color (colors 8-15)
   * Use the brightness bit if any component is DIM or MAX
   * Bit trick: OR all components and check if they share bits with CI_DIM (0x55)
   */
  if (!noBrightBit && ((qr | qg | qb) & CI_DIM)) {
    vgaColor = VGA_BIT_BRIGHT;

    /* Bright colors (8-15): Color bits set if component is MAX
     * Use bit trick: qr & (qr << 1) is non-zero only for CI_MAX (0xFF)
     * CI_MAX (0xFF) & (0xFE) = 0xFE (non-zero)
     * CI_DIM (0x55) & (0xAA) = 0x00 (zero)
     */
    unsigned char qr_max = qr & (qr << 1);
    unsigned char qg_max = qg & (qg << 1);
    unsigned char qb_max = qb & (qb << 1);

    /* Set color bits using bit shift trick instead of conditionals */
    vgaColor |= (qr_max >> 7) * VGA_BIT_RED;
    vgaColor |= (qg_max >> 7) * VGA_BIT_GREEN;
    vgaColor |= (qb_max >> 7) * VGA_BIT_BLUE;

    /* If the quantized color matches the input exactly, we can return immediately */
    RGBColor result = vgaPalette[vgaColor];
    if (result.r == r && result.g == g && result.b == b) {
      return vgaColor;
    }

    /* For ambiguous cases (e.g., pure colors that could be dark or bright),
     * check both possibilities and pick the closer one
     */
    if ((qr_max | qg_max | qb_max) && (!qr || !qg || !qb)) {
      /* At least one MAX component and at least one OFF component
       * e.g., (255, 0, 0) could be Red (4) or Light Red (12)
       * Compare distance to both bright and dark versions
       */
      int darkColor = vgaColor & ~VGA_BIT_BRIGHT;
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
  } else {
    /* Dark colors (0-7): Color bits set if component is REG or higher
     * Use bit shift trick: (qr >> 7) extracts the high bit
     * CI_REG (0xAA) >> 7 = 1, CI_OFF (0x00) >> 7 = 0
     */
    vgaColor = (qr >> 7) * VGA_BIT_RED;
    vgaColor |= (qg >> 7) * VGA_BIT_GREEN;
    vgaColor |= (qb >> 7) * VGA_BIT_BLUE;
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

HLSColor
rgbToHls(unsigned char r, unsigned char g, unsigned char b) {
  float divisor = (float)UINT8_MAX;
  float red = (float)r / divisor;
  float green = (float)g / divisor;
  float blue = (float)b / divisor;

  float min = red;
  if (green < min) min = green;
  if (blue < min) min = blue;

  float max = red;
  if (green > max) max = green;
  if (blue > max) max = blue;

  HLSColor hls;
  hls.l = (min + max) / 2.0f;

  if (min == max) {
    hls.h = 0.0f;
    hls.s = 0.0f;
  } else {
    hls.h = rgbToHsv(r, g, b).h;

    if (hls.l < 0.5f) {
      hls.s = (max - min) / (max + min);
    } else {
      hls.s = (max - min) / (2.0f - max - min);
    }
  }

  return hls;
}

HLSColor
rgbColorToHls(RGBColor color) {
  return rgbToHls(color.r, color.g, color.b);
}

static float
realModulus (double x, double y) {
  if (y == 0.0f) return 0.0 / 0.0;

  // Calculate the modulus
  double modulus = x - (y * ((int)(x / y)));

  // Ensure the result has the same sign as x
  if (modulus < 0.0f) modulus += y;

  return modulus;
}

RGBColor
hlsToRgb(float h, float l, float s) {
  /* Clamp inputs */
  hsvNormalize(&h, &l, &s);
  float red, green, blue;

  if (s == 0.0f) {
    red = green = blue = l;
  } else {
    float dl = (l * 2.0f) - 1.0f;
    if (dl < 0.0f) dl = -dl;

    float hs = h / 60.0f;
    float hf = realModulus(hs, 2.0f) - 1.0f;
    if (hf < 0.0f) hf = -hf;

    float c = (1 - dl) * s;
    float x = c * (1.0f - hf);
    float m = l - (c / 2);

    switch ((int)hs) {
      case  0: red = c; green = x; blue = 0; break;
      case  1: red = x; green = c; blue = 0; break;
      case  2: red = 0; green = c; blue = x; break;
      case  3: red = 0; green = x; blue = c; break;
      case  4: red = x; green = 0; blue = c; break;
      case  5: red = c; green = 0; blue = x; break;
      default: red = 0; green = 0; blue = 0; break;
    }

    red += m;
    green += m;
    blue += m;
  }

  RGBColor rgb = {
    .r = (int)(red * 255.0f),
    .g = (int)(green * 255.0f),
    .b = (int)(blue * 255.0f),
  };

  return rgb;
}

RGBColor
hlsColorToRgb(HLSColor hls) {
  return hlsToRgb(hls.h, hls.l, hls.s);
}

/* Return the name of the color for the specified grayscale brightness */
const char *
gsColorName(float brightness) {
  if (brightness < 0.05f) {
    return "Black";
  }

  if (brightness < 0.1f) {
    return "Near-Black";
  }

  if (brightness < 0.2f) {
    return "coal";
  }

  if (brightness < 0.3f) {
    return "Charcoal";
  }

  if (brightness < 0.5f) {
    return "Dark Gray";
  }

  if (brightness > 0.99f) {
    return "White";
  }

  if (brightness > 0.95f) {
    return "White";
  }

  if (brightness > 0.9f) {
    return "Very Light Gray";
  }

  if (brightness > 0.7f) {
    return "Light Gray";
  }

  return "Natural Gray";
}

/* Return the name of the color for the specified hue angle */
const char *
hueColorName(float hue) {
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

  static const char *const colorNames[] = {
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
  return colorNames[(int)hue / 30];
}

/* Return the HSV modifier for the specified saturation leel */
const HSVModifier *
hsvSaturationModifier (float saturation) {
  if (saturation < 0.1f) {
    static const HSVModifier modifier = {
      .name = "Faint",
      .comment = "obscure, almost indistinguishable from gray",
      .isLowest = 1,
    };

    return &modifier;
  }

  if (saturation < 0.25f) {
    static const HSVModifier modifier = {
      .name = "Pale",
      .comment = "gentle, delicate",
    };

    return &modifier;
  }

  if (saturation < 0.4f) {
    static const HSVModifier modifier = {
      .name = "Weak",
      .comment = "subtle, understated",
    };

    return &modifier;
  }

  if (saturation < 0.55f) {
    static const HSVModifier modifier = {
      .name = "Soft",
      .comment = "balanced, pleasant",
    };

    return &modifier;
  }

  if (saturation > 0.99f) {
    static const HSVModifier modifier = {
      .name = "Pure",
      .comment = "absolute, undiluted",
      .isHighest = 1,
    };

    return &modifier;
  }

  if (saturation > 0.93f) {
    static const HSVModifier modifier = {
      .name = "Rich",
      .comment = "deep, full-bodied",
    };

    return &modifier;
  }

  if (saturation > 0.85f) {
    static const HSVModifier modifier = {
      .name = "Vivid",
      .comment = "striking, intense",
    };

    return &modifier;
  }

  if (saturation > 0.7f) {
    static const HSVModifier modifier = {
      .name = "Vibrant",
      .comment = "lively, energetc",
    };

    return &modifier;
  }

  {
    static const HSVModifier modifier = {
      .name = "Moderate",
      .comment = "clear, distinct",
      .isOptional = 1,
    };

    return &modifier;
  }
}

/* Return the HSV modifier for the specified brightness (value) leel */
const HSVModifier *
hsvBrightnessModifier (float brightness) {
  if (brightness < 0.2f) {
    static const HSVModifier modifier = {
      .name = "Obscure",
      .comment = "very little light, almost black",
      .isLowest = 1,
    };

    return &modifier;
  }

  if (brightness < 0.4f) {
    static const HSVModifier modifier = {
      .name = "Dark",
      .comment = "low light, visible but subdued",
    };

    return &modifier;
  }

  if (brightness > 0.85f) {
    static const HSVModifier modifier = {
      .name = "Bright",
      .comment = "lots of light, vivid and fully illuminated",
      .isHighest = 1,
    };

    return &modifier;
  }

  if (brightness > 0.7f) {
    static const HSVModifier modifier = {
      .name = "Light",
      .comment = "high light, clear and easily distinguishable",
    };

    return &modifier;
  }

  {
    static const HSVModifier modifier = {
      .name = "Medium",
      .comment = "neutral, visible but not prominent",
      .isOptional = 1,
    };

    return &modifier;
  }
}

const HSVColorEntry hsvColorTable[] = {
  { .name = "Olive",
    .hue = {.minimum=55, .maximum=65},
    .saturation = {.minimum=0.50, .maximum=0.70},
    .value = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Dark Brown",
    .hue = {.minimum=20, .maximum=30},
    .saturation = {.minimum=0.60, .maximum=0.80},
    .value = {.minimum=0.20, .maximum=0.40},
  },

  { .name = "Brown",
    .hue = {.minimum=20, .maximum=30},
    .saturation = {.minimum=0.60, .maximum=0.80},
    .value = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Tan",
    .hue = {.minimum=30, .maximum=40},
    .saturation = {.minimum=0.30, .maximum=0.50},
    .value = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Rose",
    .hue = {.minimum=330, .maximum=350},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Light Pink",
    .hue = {.minimum=330, .maximum=345},
    .saturation = {.minimum=0.20, .maximum=0.35},
    .value = {.minimum=0.85, .maximum=1.00},
  },

  { .name = "Pink",
    .hue = {.minimum=320, .maximum=330},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Coral",
    .hue = {.minimum=0, .maximum=10},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.85, .maximum=0.95},
  },

  { .name = "Lime",
    .hue = {.minimum=75, .maximum=85},
    .saturation = {.minimum=0.80, .maximum=1.00},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Dark Teal",
    .hue = {.minimum=170, .maximum=200},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.30, .maximum=0.50},
  },

  { .name = "Teal",
    .hue = {.minimum=170, .maximum=190},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.50, .maximum=0.70},
  },

  { .name = "Turquoise",
    .hue = {.minimum=160, .maximum=170},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Maroon",
    .hue = {.minimum=0, .maximum=10},
    .saturation = {.minimum=0.50, .maximum=0.70},
    .value = {.minimum=0.30, .maximum=0.50},
  },

  { .name = "Navy",
    .hue = {.minimum=210, .maximum=230},
    .saturation = {.minimum=0.50, .maximum=0.70},
    .value = {.minimum=0.20, .maximum=0.40},
  },

  { .name = "Indigo",
    .hue = {.minimum=230, .maximum=250},
    .saturation = {.minimum=0.50, .maximum=0.70},
    .value = {.minimum=0.30, .maximum=0.50},
  },

  { .name = "Lavender",
    .hue = {.minimum=250, .maximum=270},
    .saturation = {.minimum=0.30, .maximum=0.50},
    .value = {.minimum=0.80, .maximum=0.95},
  },

  { .name = "Gold",
    .hue = {.minimum=40, .maximum=50},
    .saturation = {.minimum=0.70, .maximum=0.90},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Purple",
    .hue = {.minimum=270, .maximum=290},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Cream",
    .hue = {.minimum=55, .maximum=65},
    .saturation = {.minimum=0.20, .maximum=0.30},
    .value = {.minimum=0.90, .maximum=1.00},
  },

  { .name = "Beige",
    .hue = {.minimum=30, .maximum=40},
    .saturation = {.minimum=0.10, .maximum=0.20},
    .value = {.minimum=0.80, .maximum=0.90},
  },

  { .name = "Eggshell",
    .hue = {.minimum=40, .maximum=50},
    .saturation = {.minimum=0.05, .maximum=0.15},
    .value = {.minimum=0.90, .maximum=1.00},
  },

  { .name = "Alabaster",
    .hue = {.minimum=0, .maximum=30},
    .saturation = {.minimum=0.05, .maximum=0.10},
    .value = {.minimum=0.95, .maximum=1.00},
  },

  { .name = "Vanilla",
    .hue = {.minimum=50, .maximum=55},
    .saturation = {.minimum=0.10, .maximum=0.20},
    .value = {.minimum=0.95, .maximum=1.00},
  },

  { .name = "Chartreuse",
    .hue = {.minimum=85, .maximum=95},
    .saturation = {.minimum=0.70, .maximum=0.90},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Burgundy",
    .hue = {.minimum=330, .maximum=350},
    .saturation = {.minimum=0.50, .maximum=0.70},
    .value = {.minimum=0.30, .maximum=0.50},
  },

  { .name = "Peach",
    .hue = {.minimum=20, .maximum=30},
    .saturation = {.minimum=0.30, .maximum=0.50},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Mint",
    .hue = {.minimum=150, .maximum=170},
    .saturation = {.minimum=0.20, .maximum=0.40},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Salmon",
    .hue = {.minimum=10, .maximum=20},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.85, .maximum=0.95},
  },

  { .name = "Periwinkle",
    .hue = {.minimum=220, .maximum=240},
    .saturation = {.minimum=0.20, .maximum=0.40},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Crimson",
    .instance = 1,
    .hue = {.minimum=0, .maximum=10},
    .saturation = {.minimum=0.60, .maximum=0.80},
    .value = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Crimson",
    .instance = 2,
    .hue = {.minimum=350, .maximum=360},
    .saturation = {.minimum=0.60, .maximum=0.80},
    .value = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Amber",
    .hue = {.minimum=30, .maximum=40},
    .saturation = {.minimum=0.70, .maximum=0.90},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Khaki",
    .hue = {.minimum=40, .maximum=60},
    .saturation = {.minimum=0.20, .maximum=0.40},
    .value = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Aqua",
    .hue = {.minimum=180, .maximum=190},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Fuchsia",
    .hue = {.minimum=300, .maximum=320},
    .saturation = {.minimum=0.70, .maximum=0.90},
    .value = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Ivory",
    .hue = {.minimum=30, .maximum=40},
    .saturation = {.minimum=0.05, .maximum=0.10},
    .value = {.minimum=0.95, .maximum=1.00},
  },

  { .name = "Off-White",
    .hue = {.minimum=0, .maximum=360},
    .saturation = {.minimum=0.01, .maximum=0.05},
    .value = {.minimum=0.95, .maximum=1.00},
  },

  { .name = "Sienna",
    .hue = {.minimum=10, .maximum=20},
    .saturation = {.minimum=0.60, .maximum=0.80},
    .value = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Plum",
    .hue = {.minimum=290, .maximum=310},
    .saturation = {.minimum=0.30, .maximum=0.50},
    .value = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Orchid",
    .hue = {.minimum=270, .maximum=290},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Sky Blue",
    .hue = {.minimum=195, .maximum=205},
    .saturation = {.minimum=0.30, .maximum=0.50},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Cerulean",
    .hue = {.minimum=200, .maximum=210},
    .saturation = {.minimum=0.50, .maximum=0.70},
    .value = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Forest Green",
    .hue = {.minimum=120, .maximum=140},
    .saturation = {.minimum=0.50, .maximum=0.70},
    .value = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Mauve",
    .hue = {.minimum=270, .maximum=280},
    .saturation = {.minimum=0.20, .maximum=0.40},
    .value = {.minimum=0.70, .maximum=0.85},
  },

  { .name = "Slate Gray",
    .hue = {.minimum=210, .maximum=230},
    .saturation = {.minimum=0.10, .maximum=0.20},
    .value = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Azure",
    .hue = {.minimum=210, .maximum=220},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Scarlet",
    .hue = {.minimum=0, .maximum=10},
    .saturation = {.minimum=0.80, .maximum=1.00},
    .value = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Aquamarine",
    .hue = {.minimum=170, .maximum=180},
    .saturation = {.minimum=0.40, .maximum=0.60},
    .value = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Emerald",
    .hue = {.minimum=140, .maximum=160},
    .saturation = {.minimum=0.60, .maximum=0.80},
    .value = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Mustard",
    .hue = {.minimum=50, .maximum=60},
    .saturation = {.minimum=0.70, .maximum=0.90},
    .value = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Terracotta",
    .hue = {.minimum=10, .maximum=18},
    .saturation = {.minimum=0.50, .maximum=0.65},
    .value = {.minimum=0.70, .maximum=0.80},
  },

  { .name = "Copper",
    .hue = {.minimum=25, .maximum=30},
    .saturation = {.minimum=0.60, .maximum=0.80},
    .value = {.minimum=0.70, .maximum=0.85},
  },

  { .name = "Ebony",
    .hue = {.minimum=0, .maximum=40},
    .saturation = {.minimum=0.05, .maximum=0.25},
    .value = {.minimum=0.10, .maximum=0.20},
  },

  { .name = "Rust",
    .hue = {.minimum=18, .maximum=25},
    .saturation = {.minimum=0.60, .maximum=0.80},
    .value = {.minimum=0.60, .maximum=0.75},
  },

  { .name = "Blush",
    .hue = {.minimum=345, .maximum=355},
    .saturation = {.minimum=0.10, .maximum=0.25},
    .value = {.minimum=0.85, .maximum=0.95},
  },

  { .name = "Warm Blush",
    .hue = {.minimum=10, .maximum=20},
    .saturation = {.minimum=0.20, .maximum=0.30},
    .value = {.minimum=0.85, .maximum=0.95},
  },

  { .name = "Denim",
    .hue = {.minimum=225, .maximum=230},
    .saturation = {.minimum=0.50, .maximum=0.70},
    .value = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Lemon",
    .hue = {.minimum=55, .maximum=65},
    .saturation = {.minimum=0.80, .maximum=1.00},
    .value = {.minimum=0.90, .maximum=1.00},
  },
};

const size_t hsvColorCount = ARRAY_COUNT(hsvColorTable);
unsigned char useHSVColorTable = 0;

static const HSVColorEntry *
hsvColorEntry(HSVColor hsv) {
  for (int i=0; i<ARRAY_COUNT(hsvColorTable); i+=1) {
    const HSVColorEntry *color = &hsvColorTable[i];

    if (hsv.h < color->hue.minimum) continue;
    if (hsv.h > color->hue.maximum) continue;

    if (hsv.s < color->saturation.minimum) continue;
    if (hsv.s > color->saturation.maximum) continue;

    if (hsv.v < color->value.minimum) continue;
    if (hsv.v > color->value.maximum) continue;

    return color;
  }

  return NULL;
}

static const char *
hsvCodedColorName(HSVColor hsv) {
  /* Special case for olive: dark yellow-green - check BEFORE brown */
  if ((hsv.h >= 60.0f && hsv.h < 120.0f) && hsv.v >= 0.48f && hsv.v < 0.55f && hsv.s > 0.85f) {
    return "Olive";
  }

  /* Special case for brown: dark orange/yellow with low brightness */
  if ((hsv.h >= 15.0f && hsv.h < 90.0f) && hsv.v < 0.7f && hsv.s > 0.3f) {
    if (hsv.v < 0.42f) {
      return "Dark Brown";
    } else {
      return "Brown";
    }
  }

  /* Special case for tan/beige: very desaturated brown/orange */
  if ((hsv.h >= 15.0f && hsv.h < 75.0f) && hsv.v > 0.6f && hsv.s < 0.35f && hsv.s > 0.08f) {
    if (hsv.v > 0.85f) {
      return "Beige";
    } else {
      return "Tan";
    }
  }

  /* Special case for pink: light/desaturated red */
  if ((hsv.h < 15.0f || hsv.h >= 345.0f) && hsv.v > 0.7f && hsv.s < 0.5f && hsv.s > 0.15f) {
    /* Web color standards: Pink (255,192,203) S=0.247, LightPink (255,182,193) S=0.286
     * Counterintuitively, "LightPink" has higher saturation than "Pink" */
    if (hsv.s > 0.27f) {
      return "Light Pink";
    } else {
      return "Pink";
    }
  }

  /* Special case for coral: orangish-pink */
  if ((hsv.h >= 5.0f && hsv.h < 25.0f) && hsv.v > 0.7f && hsv.s > 0.4f && hsv.s < 0.75f) {
    return "Coral";
  }

  /* Special case for lime: bright yellow-green */
  if ((hsv.h >= 90.0f && hsv.h < 135.0f) && hsv.v > 0.75f && hsv.s > 0.75f) {
    return "Lime";
  }

  /* Special case for teal: cyan with moderate saturation and value */
  if ((hsv.h >= 165.0f && hsv.h < 195.0f) && hsv.v > 0.35f && hsv.v < 0.75f && hsv.s > 0.4f) {
    if (hsv.v < 0.5f) {
      return "Dark Teal";
    } else {
      return "Teal";
    }
  }

  /* Special case for turquoise: bright cyan */
  if ((hsv.h >= 170.0f && hsv.h < 200.0f) && hsv.v > 0.75f && hsv.s > 0.6f) {
    return "Turquoise";
  }

  /* Special case for maroon: very dark red */
  if ((hsv.h < 15.0f || hsv.h >= 345.0f) && hsv.v >= 0.45f && hsv.v <= 0.55f && hsv.s > 0.9f) {
    return "Maroon";
  }

  /* Special case for navy: very dark blue */
  if ((hsv.h >= 210.0f && hsv.h < 270.0f) && hsv.v >= 0.45f && hsv.v <= 0.55f && hsv.s > 0.9f) {
    return "Navy";
  }

  /* Special case for indigo: between blue and violet */
  if ((hsv.h >= 255.0f && hsv.h < 285.0f) && hsv.v > 0.35f && hsv.v < 0.75f && hsv.s > 0.5f) {
    return "Indigo";
  }

  /* Special case for lavender: pale violet/magenta */
  if ((hsv.h >= 230.0f && hsv.h < 280.0f) && hsv.v > 0.85f && hsv.s < 0.2f && hsv.s > 0.05f) {
    return "Lavender";
  }

  /* Special case for gold: saturated yellow with orange tint */
  if ((hsv.h >= 40.0f && hsv.h < 60.0f) && hsv.v > 0.7f && hsv.s > 0.6f) {
    return "Gold";
  }

  return NULL;
}

const char *
hsvColorToName(char *buffer, size_t bufferSize, HSVColor hsv) {
  hsvNormalize(&hsv.h, &hsv.s, &hsv.v);
  const char *name = NULL;

  if (hsv.v < 0.08f) {
    name = "Black";
  } else if (hsv.s < 0.05f) {
    name = gsColorName(hsv.v);
  } else if (useHSVColorTable) {
    const HSVColorEntry *color = hsvColorEntry(hsv);
    if (color) name = color->name;
  } else {
    name = hsvCodedColorName(hsv);
  }

  if (!name) {
    if (hsv.v < 0.1f) {
      name = gsColorName(hsv.v);
    }
  }

  if (name) {
    snprintf(buffer, bufferSize, "%s", name);
  } else {
    /* Combine modifiers and hue color */
    const char *hue = hueColorName(hsv.h);
    const HSVModifier *saturation = hsvSaturationModifier(hsv.s);
    const HSVModifier *brightness = hsvBrightnessModifier(hsv.v);

    STR_BEGIN(buffer, bufferSize);
    if (!brightness->isOptional) STR_PRINTF("%s ", brightness->name);
    if (!saturation->isOptional) STR_PRINTF("%s ", saturation->name);
    STR_PRINTF("%s", hue);
    STR_END;
  }

  return buffer;
}

const char *
hsvToName(char *buffer, size_t bufferSize, float h, float s, float v) {
  HSVColor hsv = {.h=h, .s=s, .v=v};
  return hsvColorToName(buffer, bufferSize, hsv);
}

const char *
rgbToName(char *buffer, size_t bufferSize, unsigned char r, unsigned char g, unsigned char b) {
  return hsvColorToName(buffer, bufferSize, rgbToHsv(r, g, b));
}

const char *
rgbColorToName(char *buffer, size_t bufferSize, RGBColor color) {
  return rgbToName(buffer, bufferSize, color.r, color.g, color.b);
}

const char *
hlsToName(char *buffer, size_t bufferSize, float h, float l, float s) {
  return rgbColorToName(buffer, bufferSize, hlsToRgb(h, l, s));
}

const char *
hlsColorToName(char *buffer, size_t bufferSize, HLSColor hls) {
  return hlsToName(buffer, bufferSize, hls.h, hls.l, hls.s);
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
