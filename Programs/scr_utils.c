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

#include <string.h>

#include "scr_utils.h"
#include "color.h"

const ScreenCharacter defaultScreenCharacter = {
  .text = WC_C(' '),
  .color.vgaAttributes = VGA_COLOR_DEFAULT,
};

void
toVGAScreenColor (ScreenColor *color) {
  if (color->usingRGB) {
    int foreground = rgbColorToVga(color->foreground, 0);
    int background = rgbColorToVga(color->background, 1);
    unsigned char attributes = (foreground << VGA_SHIFT_FG)
                             | (background << VGA_SHIFT_BG);

    if (color->isBlinking) attributes |= VGA_BIT_BLINK;
    if (color->isBold) attributes |= VGA_BIT_FG_BRIGHT;

    memset(color, 0, sizeof(*color));
    color->vgaAttributes = attributes;
  }
}

void
toRGBScreenColor (ScreenColor *color) {
  if (!color->usingRGB) {
    unsigned char attributes = color->vgaAttributes;
    memset(color, 0, sizeof(*color));
    color->usingRGB = 1;

    color->foreground = vgaToRgb(vgaGetForegroundColor(attributes));
    color->background = vgaToRgb(vgaGetBackgroundColor(attributes));
    if (attributes & VGA_BIT_BLINK) color->isBlinking = 1;
  }
}

void
setScreenCharacterText (ScreenCharacter *characters, size_t count, wchar_t text) {
  while (count > 0) {
    characters[--count].text = text;
  }
}

void
setScreenCharacterColor (ScreenCharacter *characters, size_t count, const ScreenColor *color) {
  while (count > 0) {
    characters[--count].color = *color;
  }
}

void
clearScreenCharacters (ScreenCharacter *characters, size_t count) {
  setScreenCharacterText(characters, count, defaultScreenCharacter.text);
  setScreenCharacterColor(characters, count, &defaultScreenCharacter.color);
}
