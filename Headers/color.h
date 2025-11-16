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
extern int rgbToVga (unsigned char r, unsigned char g, unsigned char b);

/* Convert RGB color structure to nearest VGA color code (0-15) */
extern int rgbColorToVga (RGBColor color);

/* Get the standard VGA palette RGB values as an array
 * Returns pointer to static array of 16 RGBColor structures
 */
extern const RGBColor *vgaColorPalette (void);

/* Convert RGB to HSV color space */
extern HSVColor rgbToHsv (unsigned char r, unsigned char g, unsigned char b);

/* Convert RGB color structure to HSV */
extern HSVColor rgbColorToHsv (RGBColor color);

/* Convert HSV to RGB color space */
extern RGBColor hsvToRgb (float h, float s, float v);

/* Convert HSV color structure to RGB */
extern RGBColor hsvColorToRgb (HSVColor color);

/* Get the color name for a VGA color code (0-15)
 * Returns a static string with the color name (e.g., "Red", "Light Blue")
 */
extern const char *vgaColorName (int vga);

/* Describe an RGB color as a human-readable string
 * Uses HSV analysis to provide detailed descriptions like:
 * "vivid red", "dark blue", "pale cyan", "bright orange", etc.
 * buffer must be at least 64 bytes
 * Returns pointer to buffer
 */
extern const char *rgbToDescription (char *buffer, size_t bufferSize, unsigned char r, unsigned char g, unsigned char b);

/* Describe an RGB color structure as a human-readable string
 * buffer must be at least 64 bytes
 * Returns pointer to buffer
 */
extern const char *rgbColorToDescription (char *buffer, size_t bufferSize, RGBColor color);

/* Convert ANSI 256-color code to RGB */
extern RGBColor ansiToRgb (int ansi);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_COLOR */
