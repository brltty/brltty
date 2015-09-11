/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>

#include "brl_cmds.h"
#include "unicode.h"
#include "scr.h"
#include "brltty.h"

int
isTextOffset (int *arg, int end, int relaxed) {
  int value = *arg;

  if (value < textStart) return 0;
  if ((value -= textStart) >= textCount) return 0;

  if ((ses->winx + value) >= scr.cols) {
    if (!relaxed) return 0;
    value = scr.cols - 1 - ses->winx;
  }

#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracted) {
    int result = 0;
    int index;

    for (index=0; index<contractedLength; index+=1) {
      int offset = contractedOffsets[index];

      if (offset != CTB_NO_OFFSET) {
        if (offset > value) {
          if (end) result = index - 1;
          break;
        }

        result = index;
      }
    }
    if (end && (index == contractedLength)) result = contractedLength - 1;

    value = result;
  }
#endif /* ENABLE_CONTRACTED_BRAILLE */

  *arg = value;
  return 1;
}

int
getCharacterCoordinates (int arg, int *column, int *row, int end, int relaxed) {
  if (arg == BRL_MSK_ARG) {
    if (!SCR_CURSOR_OK()) return 0;
    *column = scr.posx;
    *row = scr.posy;
  } else {
    if (!isTextOffset(&arg, end, relaxed)) return 0;
    *column = ses->winx + arg;
    *row = ses->winy;
  }
  return 1;
}

size_t
formatCharacterDescription (char *buffer, size_t size, int column, int row) {
  static char *const colours[] = {
    strtext("black"),
    strtext("blue"),
    strtext("green"),
    strtext("cyan"),
    strtext("red"),
    strtext("magenta"),
    strtext("brown"),
    strtext("light grey"),
    strtext("dark grey"),
    strtext("light blue"),
    strtext("light green"),
    strtext("light cyan"),
    strtext("light red"),
    strtext("light magenta"),
    strtext("yellow"),
    strtext("white")
  };

  size_t length;
  ScreenCharacter character;

  readScreen(column, row, 1, 1, &character);
  STR_BEGIN(buffer, size);

  {
    uint32_t text = character.text;

    STR_PRINTF("char %" PRIu32 " (U+%04" PRIX32 "): %s on %s",
               text, text,
               gettext(colours[character.attributes & 0X0F]),
               gettext(colours[(character.attributes & 0X70) >> 4]));
  }

  if (character.attributes & SCR_ATTR_BLINK) {
    STR_PRINTF(" %s", gettext("blink"));
  }

  {
    char name[0X40];

    if (getCharacterName(character.text, name, sizeof(name))) {
      STR_PRINTF(" [%s]", name);
    }
  }

  length = STR_LENGTH;
  STR_END;
  return length;
}
