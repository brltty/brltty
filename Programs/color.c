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
  if (brightness < 0.04f) {
    return "Black";
  }

  if (brightness < 0.12f) {
    return "Near-Black";
  }

  if (brightness < 0.20f) {
    return "coal";
  }

  if (brightness < 0.28f) {
    return "Charcoal";
  }

  if (brightness < 0.38f) {
    return "Dark Gray";
  }

  if (brightness > 0.99f) {
    return "White";
  }

  if (brightness > 0.95f) {
    return "White";
  }

  if (brightness > 0.75f) {
    return "Near-White";
  }

  if (brightness > 0.60f) {
    return "Very Light Gray";
  }

  if (brightness > 0.48f) {
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
  if (saturation < 0.11f) {
    static const HSVModifier modifier = {
      .name = "Faint",
      .comment = "obscure, almost indistinguishable from gray",
      .isLowest = 1,
    };

    return &modifier;
  }

  if (saturation < 0.22f) {
    static const HSVModifier modifier = {
      .name = "Pale",
      .comment = "gentle, delicate",
    };

    return &modifier;
  }

  if (saturation < 0.33f) {
    static const HSVModifier modifier = {
      .name = "Weak",
      .comment = "subtle, understated",
    };

    return &modifier;
  }

  if (saturation < 0.44f) {
    static const HSVModifier modifier = {
      .name = "Soft",
      .comment = "balanced, pleasant",
    };

    return &modifier;
  }

  if (saturation > 0.88f) {
    static const HSVModifier modifier = {
      .name = "Pure",
      .comment = "absolute, undiluted",
      .isHighest = 1,
    };

    return &modifier;
  }

  if (saturation > 0.77f) {
    static const HSVModifier modifier = {
      .name = "Rich",
      .comment = "deep, full-bodied",
    };

    return &modifier;
  }

  if (saturation > 0.66f) {
    static const HSVModifier modifier = {
      .name = "Vivid",
      .comment = "striking, intense",
    };

    return &modifier;
  }

  if (saturation > 0.55f) {
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
  { .name = "Pink",
    .hue = {.minimum=320, .maximum=330},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Burgundy",
    .hue = {.minimum=330, .maximum=335},
    .sat = {.minimum=0.50, .maximum=0.70},
    .val = {.minimum=0.30, .maximum=0.50},
  },

  { .name = "Rose",
    .hue = {.minimum=335, .maximum=340},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Light Pink",
    .hue = {.minimum=340, .maximum=345},
    .sat = {.minimum=0.20, .maximum=0.35},
    .val = {.minimum=0.85, .maximum=1.00},
  },

  { .name = "Blush",
    .hue = {.minimum=345, .maximum=350},
    .sat = {.minimum=0.10, .maximum=0.25},
    .val = {.minimum=0.85, .maximum=0.95},
  },

  { .name = "Crimson",
    .instance = 1,
    .hue = {.minimum=0, .maximum=10},
    .sat = {.minimum=0.60, .maximum=0.80},
    .val = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Crimson",
    .instance = 2,
    .hue = {.minimum=350, .maximum=360},
    .sat = {.minimum=0.60, .maximum=0.80},
    .val = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Coral",
    .hue = {.minimum=10, .maximum=15},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.85, .maximum=0.95},
  },

  { .name = "Maroon",
    .hue = {.minimum=15, .maximum=19},
    .sat = {.minimum=0.50, .maximum=0.70},
    .val = {.minimum=0.30, .maximum=0.50},
  },

  { .name = "Terracotta",
    .hue = {.minimum=19, .maximum=22},
    .sat = {.minimum=0.50, .maximum=0.65},
    .val = {.minimum=0.70, .maximum=0.80},
  },

  { .name = "Warm Blush",
    .hue = {.minimum=22, .maximum=25},
    .sat = {.minimum=0.20, .maximum=0.30},
    .val = {.minimum=0.85, .maximum=0.95},
  },

  { .name = "Salmon",
    .hue = {.minimum=25, .maximum=28},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.85, .maximum=0.95},
  },

  { .name = "Rust",
    .hue = {.minimum=28, .maximum=31},
    .sat = {.minimum=0.60, .maximum=0.80},
    .val = {.minimum=0.60, .maximum=0.75},
  },

  { .name = "Dark Brown",
    .hue = {.minimum=31, .maximum=34},
    .sat = {.minimum=0.60, .maximum=0.80},
    .val = {.minimum=0.20, .maximum=0.40},
  },

  { .name = "Peach",
    .hue = {.minimum=34, .maximum=37},
    .sat = {.minimum=0.30, .maximum=0.50},
    .val = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Copper",
    .hue = {.minimum=37, .maximum=39},
    .sat = {.minimum=0.60, .maximum=0.80},
    .val = {.minimum=0.70, .maximum=0.85},
  },

  { .name = "Brown",
    .hue = {.minimum=39, .maximum=42},
    .sat = {.minimum=0.60, .maximum=0.80},
    .val = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Tan",
    .hue = {.minimum=42, .maximum=45},
    .sat = {.minimum=0.30, .maximum=0.50},
    .val = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Amber",
    .hue = {.minimum=45, .maximum=47},
    .sat = {.minimum=0.70, .maximum=0.90},
    .val = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Beige",
    .hue = {.minimum=47, .maximum=49},
    .sat = {.minimum=0.10, .maximum=0.20},
    .val = {.minimum=0.80, .maximum=0.90},
  },

  { .name = "Ivory",
    .hue = {.minimum=49, .maximum=51},
    .sat = {.minimum=0.05, .maximum=0.10},
    .val = {.minimum=0.95, .maximum=1.00},
  },

  { .name = "Alabaster",
    .hue = {.minimum=51, .maximum=54},
    .sat = {.minimum=0.04, .maximum=0.08},
    .val = {.minimum=0.96, .maximum=1.00},
  },

  { .name = "Eggshell",
    .hue = {.minimum=54, .maximum=57},
    .sat = {.minimum=0.05, .maximum=0.15},
    .val = {.minimum=0.90, .maximum=1.00},
  },

  { .name = "Vanilla",
    .hue = {.minimum=57, .maximum=59},
    .sat = {.minimum=0.10, .maximum=0.20},
    .val = {.minimum=0.95, .maximum=1.00},
  },

  { .name = "Mustard",
    .hue = {.minimum=59, .maximum=61},
    .sat = {.minimum=0.70, .maximum=0.90},
    .val = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Olive",
    .hue = {.minimum=61, .maximum=64},
    .sat = {.minimum=0.50, .maximum=0.70},
    .val = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Lemon",
    .hue = {.minimum=64, .maximum=67},
    .sat = {.minimum=0.80, .maximum=1.00},
    .val = {.minimum=0.90, .maximum=1.00},
  },

  { .name = "Khaki",
    .hue = {.minimum=67, .maximum=71},
    .sat = {.minimum=0.20, .maximum=0.40},
    .val = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Chartreuse",
    .hue = {.minimum=85, .maximum=95},
    .sat = {.minimum=0.70, .maximum=0.90},
    .val = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Forest Green",
    .hue = {.minimum=120, .maximum=140},
    .sat = {.minimum=0.50, .maximum=0.70},
    .val = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Emerald",
    .hue = {.minimum=140, .maximum=150},
    .sat = {.minimum=0.60, .maximum=0.80},
    .val = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Mint",
    .hue = {.minimum=150, .maximum=160},
    .sat = {.minimum=0.20, .maximum=0.40},
    .val = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Turquoise",
    .hue = {.minimum=160, .maximum=165},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Aquamarine",
    .hue = {.minimum=165, .maximum=170},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Teal",
    .hue = {.minimum=170, .maximum=178},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.50, .maximum=0.70},
  },

  { .name = "Dark Teal",
    .hue = {.minimum=178, .maximum=190},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.30, .maximum=0.50},
  },

  { .name = "Aqua",
    .hue = {.minimum=190, .maximum=195},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Sky Blue",
    .hue = {.minimum=195, .maximum=198},
    .sat = {.minimum=0.30, .maximum=0.50},
    .val = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Cerulean",
    .hue = {.minimum=198, .maximum=205},
    .sat = {.minimum=0.50, .maximum=0.70},
    .val = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Azure",
    .hue = {.minimum=205, .maximum=210},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Navy",
    .hue = {.minimum=210, .maximum=215},
    .sat = {.minimum=0.50, .maximum=0.70},
    .val = {.minimum=0.20, .maximum=0.40},
  },

  { .name = "Slate Gray",
    .hue = {.minimum=215, .maximum=218},
    .sat = {.minimum=0.10, .maximum=0.20},
    .val = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Denim",
    .hue = {.minimum=218, .maximum=222},
    .sat = {.minimum=0.50, .maximum=0.70},
    .val = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Periwinkle",
    .hue = {.minimum=222, .maximum=230},
    .sat = {.minimum=0.20, .maximum=0.40},
    .val = {.minimum=0.80, .maximum=1.00},
  },

  { .name = "Indigo",
    .hue = {.minimum=230, .maximum=240},
    .sat = {.minimum=0.50, .maximum=0.70},
    .val = {.minimum=0.30, .maximum=0.50},
  },

  { .name = "Lavender",
    .hue = {.minimum=250, .maximum=260},
    .sat = {.minimum=0.30, .maximum=0.50},
    .val = {.minimum=0.80, .maximum=0.95},
  },

  { .name = "Mauve",
    .hue = {.minimum=260, .maximum=270},
    .sat = {.minimum=0.20, .maximum=0.40},
    .val = {.minimum=0.70, .maximum=0.85},
  },

  { .name = "Purple",
    .hue = {.minimum=270, .maximum=280},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Orchid",
    .hue = {.minimum=280, .maximum=290},
    .sat = {.minimum=0.40, .maximum=0.60},
    .val = {.minimum=0.60, .maximum=0.80},
  },

  { .name = "Plum",
    .hue = {.minimum=290, .maximum=300},
    .sat = {.minimum=0.30, .maximum=0.50},
    .val = {.minimum=0.40, .maximum=0.60},
  },

  { .name = "Fuchsia",
    .hue = {.minimum=300, .maximum=315},
    .sat = {.minimum=0.70, .maximum=0.90},
    .val = {.minimum=0.70, .maximum=0.90},
  },

  { .name = "Off-White",
    .hue = {.minimum=0, .maximum=360},
    .sat = {.minimum=0.01, .maximum=0.04},
    .val = {.minimum=0.95, .maximum=1.00},
  },
};

const size_t hsvColorCount = ARRAY_COUNT(hsvColorTable);
unsigned char useHSVColorTable = 0;
unsigned char useHSVColorSorting = 0;

static inline float
hsvRangeMidpoint (const HSVComponentRange *range) {
  return (range->minimum + range->maximum) / 2.0;
}

static inline float
hsvRangeSize (const HSVComponentRange *range) {
  return range->maximum - range->minimum;
}

static int
hsvCompareRanges (const HSVComponentRange *range1, const HSVComponentRange *range2) {
  {
    float midpoint1 = hsvRangeMidpoint(range1);
    float midpoint2 = hsvRangeMidpoint(range2);

    if (midpoint1 < midpoint2) return -1;
    if (midpoint1 > midpoint2) return 1;
  }

  {
    float size1 = hsvRangeSize(range1);
    float size2 = hsvRangeSize(range2);

    if (size1 < size2) return -1;
    if (size1 > size2) return 1;
  }

  return 0;
}

static int
hsvSortComparer (const void *item1, const void *item2) {
  const HSVColorEntry *const *color1 = item1;
  const HSVColorEntry *const *color2 = item2;
  int relation;

  relation = hsvCompareRanges(&(*color1)->hue, &(*color2)->hue);
  if (relation != 0) return relation;

  relation = hsvCompareRanges(&(*color1)->sat, &(*color2)->sat);
  if (relation != 0) return relation;

  relation = hsvCompareRanges(&(*color1)->val, &(*color2)->val);
  if (relation != 0) return relation;

  return 0;
}

static int
hsvRangeContains (const HSVComponentRange *range, float value) {
  if (value <= range->minimum) return -1;
  if (value >= range->maximum) return 1;
  return 0;
}

static int
hsvSearchComparer (const void *target, const void *item) {
  const HSVColor *hsv = target;
  const HSVColorEntry *const *color = item;
  int relation;

  relation = hsvRangeContains(&(*color)->hue, hsv->h);
  if (relation != 0) return relation;

  relation = hsvRangeContains(&(*color)->sat, hsv->s);
  if (relation != 0) return relation;

  relation = hsvRangeContains(&(*color)->val, hsv->v);
  if (relation != 0) return relation;

  return 0;
}

const HSVColorEntry *
hsvColorEntry (HSVColor hsv) {
  if (useHSVColorSorting) {
    static const HSVColorEntry *sortedTable[ARRAY_COUNT(hsvColorTable)] = {NULL};

    if (!sortedTable[0]) {
      for (int i=0; i<hsvColorCount; i+=1) {
        sortedTable[i] = &hsvColorTable[i];
      }

      qsort(
        sortedTable, ARRAY_COUNT(sortedTable),
        sizeof(sortedTable[0]), hsvSortComparer
      );
    }

    const HSVColorEntry *const *color = bsearch(
      &hsv, sortedTable, ARRAY_COUNT(sortedTable),
      sizeof(sortedTable[0]), hsvSearchComparer
    );

    if (color) return *color;
  } else {
    for (int i=0; i<ARRAY_COUNT(hsvColorTable); i+=1) {
      const HSVColorEntry *color = &hsvColorTable[i];

      if (hsv.h < color->hue.minimum) continue;
      if (hsv.h > color->hue.maximum) continue;

      if (hsv.s < color->sat.minimum) continue;
      if (hsv.s > color->sat.maximum) continue;

      if (hsv.v < color->val.minimum) continue;
      if (hsv.v > color->val.maximum) continue;

      return color;
    }
  }

  return NULL;
}

static const char *
hsvInlineColorName(HSVColor hsv) {
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
    name = hsvInlineColorName(hsv);
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
