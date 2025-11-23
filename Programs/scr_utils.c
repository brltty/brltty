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

#include "scr_utils.h"

const ScreenCharacter defaultScreenCharacter = {
  .text = WC_C(' '),
  .color.vgaAttributes = VGA_COLOR_DEFAULT,
};

void
toRGBScreenColor (ScreenColor *color) {
  if (!color->usingRGB) {
    unsigned char attributes = color->vgaAttributes;
    color->foreground = vgaToRgb(vgaGetForegroundColor(attributes));
    color->background = vgaToRgb(vgaGetBackgroundColor(attributes));
    color->usingRGB = 1;
  }
}

void
setScreenCharacterText (ScreenCharacter *characters, wchar_t text, size_t count) {
  while (count > 0) {
    characters[--count].text = text;
  }
}

void
setScreenCharacterColor (ScreenCharacter *characters, const ScreenColor *color, size_t count) {
  while (count > 0) {
    characters[--count].color = *color;
  }
}

void
clearScreenCharacters (ScreenCharacter *characters, size_t count) {
  setScreenCharacterText(characters, defaultScreenCharacter.text, count);
  setScreenCharacterColor(characters, &defaultScreenCharacter.color, count);
}
