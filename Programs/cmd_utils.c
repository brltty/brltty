/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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
#include <string.h>
#include <ctype.h>

#include "log.h"
#include "strfmt.h"
#include "alert.h"
#include "brl_cmds.h"
#include "unicode.h"
#include "scr.h"
#include "core.h"

void
alertLineSkipped (unsigned int *count) {
  const unsigned int interval = 4;

  if (!*count) {
    alert(ALERT_SKIP_FIRST);
  } else if (*count <= interval) {
    alert(ALERT_SKIP_ONE);
  } else if (!(*count % interval)) {
    alert(ALERT_SKIP_SEVERAL);
  }

  *count += 1;
}

int
isTextOffset (int arg, int *first, int *last, int relaxed) {
  int value = arg;

  if (value < textStart) return 0;
  if ((value -= textStart) >= textCount) return 0;

#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracted) {
    int start = 0;
    int end = 0;

    {
      int textIndex = 0;

      while (textIndex < contractedLength) {
        int cellIndex = contractedOffsets[textIndex];

        if (cellIndex != CTB_NO_OFFSET) {
          if (cellIndex > value) {
            end = textIndex - 1;
            break;
          }

          start = textIndex;
        }

        textIndex += 1;
      }

      if (textIndex == contractedLength) end = textIndex - 1;
    }

    if (first) *first = start;
    if (last) *last = end;
    return 1;
  }
#endif /* ENABLE_CONTRACTED_BRAILLE */

  if ((ses->winx + value) >= scr.cols) {
    if (!relaxed) return 0;
    value = scr.cols - 1 - ses->winx;
  }

  if (prefs.wordWrap) {
    int length = getWordWrapLength(ses->winy, ses->winx, textCount);
    if (length > textCount) length = textCount;
    if (value >= length) value = length - 1;
  }

  if (first) *first = value;
  if (last) *last = value;
  return 1;
}

int
getCharacterCoordinates (int arg, int *row, int *first, int *last, int relaxed) {
  if (arg == BRL_MSK_ARG) {
    if (!SCR_CURSOR_OK()) return 0;
    *row = scr.posy;
    if (first) *first = scr.posx;
    if (last) *last = scr.posx;
  } else {
    if (!isTextOffset(arg, first, last, relaxed)) return 0;
    if (row) *row = ses->winy;
    if (first) *first += ses->winx;
    if (last) *last += ses->winx;
  }

  return 1;
}

STR_BEGIN_FORMATTER(formatCharacterDescription, int column, int row)
  ScreenCharacter character;
  readScreen(column, row, 1, 1, &character);

  {
    char name[0X40];

    if (getCharacterName(character.text, name, sizeof(name))) {
      {
        size_t length = strlen(name);
        for (int i=0; i<length; i+=1) name[i] = tolower(name[i]);
      }

      STR_PRINTF(" %s: ", name);
    }
  }

  {
    uint32_t text = character.text;
    STR_PRINTF("U+%04" PRIX32 " (%" PRIu32 "):", text, text);
  }

  {
    static const char *const colours[] = {
      /*      */ strtext("black"),
      /*    B */ strtext("blue"),
      /*   G  */ strtext("green"),
      /*   GB */ strtext("cyan"),
      /*  R   */ strtext("red"),
      /*  R B */ strtext("magenta"),
      /*  RG  */ strtext("brown"),
      /*  RGB */ strtext("light grey"),
      /* L    */ strtext("dark grey"),
      /* L  B */ strtext("light blue"),
      /* L G  */ strtext("light green"),
      /* L GB */ strtext("light cyan"),
      /* LR   */ strtext("light red"),
      /* LR B */ strtext("light magenta"),
      /* LRG  */ strtext("yellow"),
      /* LRGB */ strtext("white")
    };

    unsigned char attributes = character.attributes;
    STR_PRINTF(" %s", gettext(colours[attributes & SCR_MASK_FG]));
    STR_PRINTF(" on %s", gettext(colours[(attributes & SCR_MASK_BG) >> 4]));
  }

  if (character.attributes & SCR_ATTR_BLINK) {
    STR_PRINTF(" %s", gettext("blink"));
  }
STR_END_FORMATTER
