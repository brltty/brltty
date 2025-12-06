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

#ifndef BRLTTY_INCLUDED_COLOR_INTERNAL
#define BRLTTY_INCLUDED_COLOR_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  float minimum;
  float maximum;
} HSVComponentRange;

typedef struct {
  const char *name;
  HSVComponentRange hue;
  HSVComponentRange saturation;
  HSVComponentRange value;
  unsigned char instance;
} HSVColorEntry;

extern const HSVColorEntry hsvColorTable[];
extern const size_t hsvColorCount;
extern const HSVColorEntry *hsvColorEntry (HSVColor hsv);

extern unsigned char useHSVColorTable;
extern unsigned char useHSVColorSorting;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_COLOR_INTERNAL */
