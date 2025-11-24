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

#include "alert.h"
#include "brl_cmds.h"
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
isTextOffset (int arg, int *row, int *first, int *last, int relaxed) {
  int y = arg / brl.textColumns;
  if (y >= brl.textRows) return 0;
  if ((ses->winy + y) >= scr.rows) return 0;

  int x = arg % brl.textColumns;
  if (x < textStart) return 0;
  if ((x -= textStart) >= textCount) return 0;

  if (isContracted) {
    BrailleRowDescriptor *brd = getBrailleRowDescriptor(y);
    if (!brd) return 0;

    int *offsets = brd->contracted.offsets.array;
    if (!offsets) return 0;

    int start = 0;
    int end = 0;

    {
      int textIndex = 0;

      while (textIndex < brd->contracted.length) {
        int cellIndex = offsets[textIndex];

        if (cellIndex != CTB_NO_OFFSET) {
          if (cellIndex > x) {
            end = textIndex - 1;
            break;
          }

          start = textIndex;
        }

        textIndex += 1;
      }

      if (textIndex == brd->contracted.length) end = textIndex - 1;
    }

    if (first) *first = start;
    if (last) *last = end;
  } else {
    if ((ses->winx + x) >= scr.cols) {
      if (!relaxed) return 0;
      x = scr.cols - ses->winx - 1;
    }

    if (prefs.wordWrap) {
      int length = getWordWrapLength(ses->winy, ses->winx, textCount);
      if (length > textCount) length = textCount;
      if (x >= length) x = length - 1;
    }

    if (first) *first = x;
    if (last) *last = x;
  }

  if (row) *row = y;
  return 1;
}

int
getCharacterCoordinates (int arg, int *row, int *first, int *last, int relaxed) {
  if (arg == BRL_MSK_ARG) {
    if (!SCR_CURSOR_OK()) return 0;
    if (row) *row = scr.posy;
    if (first) *first = scr.posx;
    if (last) *last = scr.posx;
  } else {
    if (!isTextOffset(arg, row, first, last, relaxed)) return 0;
    if (row) *row += ses->winy;
    if (first) *first += ses->winx;
    if (last) *last += ses->winx;
  }

  return 1;
}

int
getScreenCharacter (ScreenCharacter *character, int column, int row) {
  return readScreen(column, row, 1, 1, character);
}
