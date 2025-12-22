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

#ifndef BRLTTY_INCLUDED_COLOR
#define BRLTTY_INCLUDED_COLOR

#include "color_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Convert VGA color code (0-15) to RGB values */
extern RGBColor vgaToRgb (int vga);

/* Convert RGB color to nearest VGA color code (0-15)
 * Uses color distance calculation to find the closest match
 */
extern int rgbToVga (unsigned char r, unsigned char g, unsigned char b, int noBrightBit);

/* Convert RGB color structure to nearest VGA color code (0-15) */
extern int rgbColorToVga (RGBColor color, int noBrightBit);

/* Get the standard VGA palette RGB values as an array
 * Returns pointer to static array of 16 RGBColor structures
 */
extern const RGBColor *vgaColorPalette (void);

/* Convert RGB to HSV color space */
extern HSVColor rgbToHsv (unsigned char r, unsigned char g, unsigned char b);

/* Convert RGB color structure to HSV */
extern HSVColor rgbColorToHsv (RGBColor color);

/* Ensure that HSV components are within their respective ranges */
extern void hsvNormalize (float *h, float *s, float *v);

/* Ensure that HSV color structure fields are within their respective ranges */
extern void hsvColorNormalize(HSVColor *hsv);

/* Convert HSV to RGB color space */
extern RGBColor hsvToRgb (float h, float s, float v);

/* Convert HSV color structure to RGB */
extern RGBColor hsvColorToRgb (HSVColor color);

/* Convert RGB to HLS color space */
extern HLSColor rgbToHls(unsigned char r, unsigned char g, unsigned char b);

/* Convert RGB color structure to HLS */
extern HLSColor rgbColorToHls(RGBColor color);

/* Convert HLS to RGB color space */
extern RGBColor hlsToRgb(float h, float l, float s);

/* Convert HLS color structure to RGB */
extern RGBColor hlsColorToRgb(HLSColor hls);

/* Get the color name for a VGA color code (0-15)
 * Returns a static string with the color name (e.g., "Red", "Light Blue")
 */
extern const char *vgaColorName (int vga);

/* Return the name of the color for the specified grayscale brightness */
extern const char *gsColorName(float brightness);

/* Return the name of the color for the specified hue angle */
extern const char *hueColorName(float hue);

typedef struct {
  const char *name;
  const char *comment;
  unsigned char isOptional:1;
  unsigned char isLowest:1;
  unsigned char isHighest:1;
} HSVModifier;

/* Return the HSV modifier for the specified saturation leel */
extern const HSVModifier *hsvSaturationModifier (float saturation);

/* Return the HSV modifier for the specified brightness (value) leel */
extern const HSVModifier *hsvBrightnessModifier (float brightness);

/* Describe an HSV color as a human-readable string
 * buffer must be at least 64 bytes
 * Returns pointer to buffer
 */
extern const char *hsvToName(char *buffer, size_t bufferSize, float h, float s, float v);

/* Describe an HSV color structure as a human-readable string
 * buffer must be at least 64 bytes
 * Returns pointer to buffer
 */
extern const char *hsvColorToName(char *buffer, size_t bufferSize, HSVColor hsv);

/* Describe an RGB color as a human-readable string
 * Uses HSV analysis to provide more accurate names like:
 * "vivid red", "dark blue", "pale cyan", "bright orange", etc.
 * buffer must be at least 64 bytes
 * Returns pointer to buffer
 */
extern const char *rgbToName (char *buffer, size_t bufferSize, unsigned char r, unsigned char g, unsigned char b);

/* Describe an RGB color structure as a human-readable string
 * buffer must be at least 64 bytes
 * Returns pointer to buffer
 */
extern const char *rgbColorToName (char *buffer, size_t bufferSize, RGBColor color);

/* Describe an HLS color as a human-readable string
 * Uses HSV analysis to provide more accurate names like:
 * "vivid red", "dark blue", "pale cyan", "bright orange", etc.
 * buffer must be at least 64 bytes
 * Returns pointer to buffer
 */
extern const char *hlsToName(char *buffer, size_t bufferSize, float h, float l, float s);

/* Describe an HLS color structure as a human-readable string
 * buffer must be at least 64 bytes
 * Returns pointer to buffer
 */
extern const char *hlsColorToName(char *buffer, size_t bufferSize, HLSColor hls);

/* Convert ANSI 256-color code to RGB */
extern RGBColor ansiToRgb (unsigned int code);

/* Interpolate between two HSV colors */
extern HSVColor hsvColorInterpolate (HSVColor hsv1, HSVColor hsv2, float factor);

/* Interpolate between two RGB colors using HSV */
extern RGBColor rgbColorInterpolate (RGBColor rgb1, RGBColor rgb2, float factor);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_COLOR */
