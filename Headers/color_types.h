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

#ifndef BRLTTY_INCLUDED_COLOR_TYPES
#define BRLTTY_INCLUDED_COLOR_TYPES

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* RGB Color Structure */
typedef struct {
  unsigned char r;  /* Red component (0-255) */
  unsigned char g;  /* Green component (0-255) */
  unsigned char b;  /* Blue component (0-255) */
} RGBColor;

/* HSV Color Structure */
typedef struct {
  float h;  /* Hue (0-360 degrees) */
  float s;  /* Saturation (0.0-1.0) */
  float v;  /* Value/Brightness (0.0-1.0) */
} HSVColor;

/* HLS Color Structure */
typedef struct {
  float h;  /* Hue (0-360 degrees) */
  float l;  /* Lightness (0.0-1.0) */
  float s;  /* Saturation (0.0-1.0) */
} HLSColor;

/* VGA Color Codes (0-15)
 * Standard 16-color VGA palette:
 * 0=Black, 1=Red, 2=Green, 3=Brown/Yellow, 4=Blue, 5=Magenta, 6=Cyan, 7=Light Gray
 * 8=Dark Gray, 9=Light Red, 10=Light Green, 11=Yellow, 12=Light Blue,
 * 13=Light Magenta, 14=Light Cyan, 15=White
 */

#define VGA_COLOR_COUNT 16
#define VGA_BIT_BLUE   0X1
#define VGA_BIT_GREEN  0X2
#define VGA_BIT_RED    0X4
#define VGA_BIT_BRIGHT 0X8

typedef char ColorDescriptionBuffer[64];

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_COLOR_TYPES */
