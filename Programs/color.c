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
  /* Black */         [ 0] = {0X00, 0X00, 0X00},
  /* Blue */          [ 1] = {0X00, 0X00, 0XAA},
  /* Green */         [ 2] = {0X00, 0XAA, 0X00},
  /* Cyan */          [ 3] = {0X00, 0XAA, 0XAA},
  /* Red */           [ 4] = {0XAA, 0X00, 0X00},
  /* Magenta */       [ 5] = {0XAA, 0X00, 0XAA},
  /* Brown */         [ 6] = {0XAA, 0X55, 0X00}, /* Hardware exception: not 0xAA for green */
  /* Light Grey */    [ 7] = {0XAA, 0XAA, 0XAA},
  /* Dark Grey */     [ 8] = {0X55, 0X55, 0X55},
  /* Light Blue */    [ 9] = {0X55, 0X55, 0XFF},
  /* Light Green */   [10] = {0X55, 0XFF, 0X55},
  /* Light Cyan */    [11] = {0X55, 0XFF, 0XFF},
  /* Light Red */     [12] = {0XFF, 0X55, 0X55},
  /* Light Magenta */ [13] = {0XFF, 0X55, 0XFF},
  /* Yellow */        [14] = {0XFF, 0XFF, 0X55},
  /* White */         [15] = {0XFF, 0XFF, 0XFF}
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
colorDistanceSquared(unsigned char r1, unsigned char g1, unsigned char b1,
                    unsigned char r2, unsigned char g2, unsigned char b2) {
  int dr = (int)r1 - (int)r2;
  int dg = (int)g1 - (int)g2;
  int db = (int)b1 - (int)b2;

  return dr * dr + dg * dg + db * db;
}

int
rgbToVga(unsigned char r, unsigned char g, unsigned char b) {
  /* Find the closest VGA color by minimum Euclidean distance in RGB space
   * This is similar to the approximation used in the tmux driver but more accurate
   */
  int closestColor = 0;
  int minDistance = colorDistanceSquared(r, g, b,
                                         vgaPalette[0].r,
                                         vgaPalette[0].g,
                                         vgaPalette[0].b);

  /* Check all 16 VGA colors */
  for (int i=1; i<VGA_COLOR_COUNT; i+=1) {
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
rgbColorToVga(RGBColor color) {
  return rgbToVga(color.r, color.g, color.b);
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
  [ 7] = "Light Grey",
  [ 8] = "Dark Grey",
  [ 9] = "Light Blue",
  [10] = "Light Green",
  [11] = "Yellow",
  [12] = "Light Red",
  [13] = "Light Magenta",
  [14] = "Light Cyan",
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

RGBColor
hsvToRgb(float h, float s, float v) {
  RGBColor rgb;

  /* Clamp inputs */
  while (h < 0.0f) h += 360.0f;
  while (h >= 360.0f) h -= 360.0f;
  if (s < 0.0f) s = 0.0f;
  if (s > 1.0f) s = 1.0f;
  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;

  if (s == 0.0f) {
    /* Achromatic (grey) */
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
  if (hue < 15.0f || hue >= 345.0f) return "red";
  if (hue < 45.0f) return "orange";
  if (hue < 75.0f) return "yellow";
  if (hue < 105.0f) return "yellow-green";
  if (hue < 135.0f) return "green";
  if (hue < 165.0f) return "cyan-green";
  if (hue < 195.0f) return "cyan";
  if (hue < 225.0f) return "blue-cyan";
  if (hue < 255.0f) return "blue";
  if (hue < 285.0f) return "violet";
  if (hue < 315.0f) return "magenta";
  return "red-magenta";
}

const char *
rgbToDescription(char *buffer, size_t bufferSize, unsigned char r, unsigned char g, unsigned char b) {
  HSVColor hsv = rgbToHsv(r, g, b);

  /* Handle special cases */
  if (hsv.v < 0.08f) {
    snprintf(buffer, bufferSize, "black");
    return buffer;
  }

  if (hsv.s < 0.08f) {
    /* Achromatic - shades of grey */
    if (hsv.v > 0.92f) {
      snprintf(buffer, bufferSize, "white");
    } else if (hsv.v > 0.65f) {
      snprintf(buffer, bufferSize, "light grey");
    } else if (hsv.v > 0.35f) {
      snprintf(buffer, bufferSize, "grey");
    } else {
      snprintf(buffer, bufferSize, "dark grey");
    }
    return buffer;
  }

  /* Special case for olive: dark yellow-green - check BEFORE brown */
  if ((hsv.h >= 60.0f && hsv.h < 120.0f) && hsv.v >= 0.48f && hsv.v < 0.55f && hsv.s > 0.85f) {
    snprintf(buffer, bufferSize, "olive");
    return buffer;
  }

  /* Special case for brown: dark orange/yellow with low brightness */
  if ((hsv.h >= 15.0f && hsv.h < 90.0f) && hsv.v < 0.7f && hsv.s > 0.3f) {
    if (hsv.v < 0.42f) {
      snprintf(buffer, bufferSize, "dark brown");
    } else {
      snprintf(buffer, bufferSize, "brown");
    }
    return buffer;
  }

  /* Special case for tan/beige: very desaturated brown/orange */
  if ((hsv.h >= 15.0f && hsv.h < 75.0f) && hsv.v > 0.6f && hsv.s < 0.35f && hsv.s > 0.08f) {
    if (hsv.v > 0.85f) {
      snprintf(buffer, bufferSize, "beige");
    } else {
      snprintf(buffer, bufferSize, "tan");
    }
    return buffer;
  }

  /* Special case for pink: light/desaturated red */
  if ((hsv.h < 15.0f || hsv.h >= 345.0f) && hsv.v > 0.7f && hsv.s < 0.5f && hsv.s > 0.15f) {
    /* Web color standards: Pink (255,192,203) S=0.247, LightPink (255,182,193) S=0.286
     * Counterintuitively, "LightPink" has higher saturation than "Pink" */
    if (hsv.s > 0.27f) {
      snprintf(buffer, bufferSize, "light pink");
    } else {
      snprintf(buffer, bufferSize, "pink");
    }
    return buffer;
  }

  /* Special case for coral: orangish-pink */
  if ((hsv.h >= 5.0f && hsv.h < 25.0f) && hsv.v > 0.7f && hsv.s > 0.4f && hsv.s < 0.75f) {
    snprintf(buffer, bufferSize, "coral");
    return buffer;
  }

  /* Special case for lime: bright yellow-green */
  if ((hsv.h >= 90.0f && hsv.h < 135.0f) && hsv.v > 0.75f && hsv.s > 0.75f) {
    snprintf(buffer, bufferSize, "lime");
    return buffer;
  }

  /* Special case for teal: cyan with moderate saturation and value */
  if ((hsv.h >= 165.0f && hsv.h < 195.0f) && hsv.v > 0.35f && hsv.v < 0.75f && hsv.s > 0.4f) {
    if (hsv.v < 0.5f) {
      snprintf(buffer, bufferSize, "dark teal");
    } else {
      snprintf(buffer, bufferSize, "teal");
    }
    return buffer;
  }

  /* Special case for turquoise: bright cyan */
  if ((hsv.h >= 170.0f && hsv.h < 200.0f) && hsv.v > 0.75f && hsv.s > 0.6f) {
    snprintf(buffer, bufferSize, "turquoise");
    return buffer;
  }

  /* Special case for maroon: very dark red */
  if ((hsv.h < 15.0f || hsv.h >= 345.0f) && hsv.v >= 0.45f && hsv.v <= 0.55f && hsv.s > 0.9f) {
    snprintf(buffer, bufferSize, "maroon");
    return buffer;
  }

  /* Special case for navy: very dark blue */
  if ((hsv.h >= 210.0f && hsv.h < 270.0f) && hsv.v >= 0.45f && hsv.v <= 0.55f && hsv.s > 0.9f) {
    snprintf(buffer, bufferSize, "navy");
    return buffer;
  }

  /* Special case for indigo: between blue and violet */
  if ((hsv.h >= 255.0f && hsv.h < 285.0f) && hsv.v > 0.35f && hsv.v < 0.75f && hsv.s > 0.5f) {
    snprintf(buffer, bufferSize, "indigo");
    return buffer;
  }

  /* Special case for lavender: pale violet/magenta */
  if ((hsv.h >= 230.0f && hsv.h < 280.0f) && hsv.v > 0.85f && hsv.s < 0.2f && hsv.s > 0.05f) {
    snprintf(buffer, bufferSize, "lavender");
    return buffer;
  }

  /* Special case for gold: saturated yellow with orange tint */
  if ((hsv.h >= 40.0f && hsv.h < 60.0f) && hsv.v > 0.7f && hsv.s > 0.6f) {
    snprintf(buffer, bufferSize, "gold");
    return buffer;
  }

  /* Get the base hue name */
  const char *hueName = getHueName(hsv.h);

  /* Determine brightness modifier */
  const char *brightnessModifier = "";
  if (hsv.v < 0.3f) {
    brightnessModifier = "very dark ";
  } else if (hsv.v < 0.5f) {
    brightnessModifier = "dark ";
  } else if (hsv.v > 0.92f && hsv.s > 0.6f && hsv.s < 0.95f) {
    brightnessModifier = "bright ";
  }

  /* Determine saturation modifier */
  const char *saturationModifier = "";
  if (hsv.s < 0.2f) {
    saturationModifier = "pale ";
  } else if (hsv.s < 0.4f) {
    saturationModifier = "light ";
  } else if (hsv.s > 0.95f && hsv.v > 0.9f) {
    saturationModifier = "vivid ";
  } else if (hsv.s > 0.7f && hsv.v > 0.5f && hsv.v < 0.9f) {
    saturationModifier = "saturated ";
  }

  /* Combine modifiers and hue name */
  snprintf(buffer, bufferSize, "%s%s%s",
           brightnessModifier, saturationModifier, hueName);

  return buffer;
}

const char *
rgbColorToDescription(char *buffer, size_t bufferSize, RGBColor color) {
  return rgbToDescription(buffer, bufferSize, color.r, color.g, color.b);
}
