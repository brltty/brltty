/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "misc.h"
#include "scr.h"
#include "scr_help.h"

typedef struct {
  wchar_t *characters;
  size_t length;
} HelpLineEntry;

static HelpLineEntry *lineTable = NULL;
static unsigned int lineCount = 0;

static unsigned int lineTableSize;
static size_t lineLength;
static unsigned char cursorRow, cursorColumn;

static void
clearHelpScreen (void) {
  while (lineCount) {
    HelpLineEntry *hle = &lineTable[--lineCount];
    free(hle->characters);
  }

  if (lineTable) {
    free(lineTable);
    lineTable = NULL;
  }

  lineTableSize = 0;
  lineLength = 0;
  cursorRow = 0;
  cursorColumn = 0;
}

static int
addLine_HelpScreen (const wchar_t *line) {
  if (lineCount == lineTableSize) {
    unsigned int newSize = lineTableSize? lineTableSize<<1: 0X40;
    HelpLineEntry *newTable = realloc(lineTable, ARRAY_SIZE(newTable, newSize));

    if (!newTable) {
      LogError("realloc");
      return 0;
    }

    lineTable = newTable;
    lineTableSize = newSize;
  }

  {
    HelpLineEntry *hle = &lineTable[lineCount];
    size_t length = wcslen(line);
    if ((hle->length = length) > lineLength) lineLength = length;

    if (!(hle->characters = malloc(ARRAY_SIZE(hle->characters, length)))) {
      LogError("malloc");
      return 0;
    }

    wmemcpy(hle->characters, line, length);
  }
  lineCount += 1;

  return 1;
}

static int
construct_HelpScreen (void) {
  clearHelpScreen();
  return 1;
}

static void
destruct_HelpScreen (void) {
  clearHelpScreen();
}

static int
currentVirtualTerminal_HelpScreen (void) {
  return userVirtualTerminal(1);
}

static void
describe_HelpScreen (ScreenDescription *description) {
  description->posx = cursorColumn;
  description->posy = cursorRow;
  description->cols = lineLength;
  description->rows = lineCount;
  description->number = currentVirtualTerminal_HelpScreen();
}

static int
readCharacters_HelpScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  if (validateScreenBox(box, lineLength, lineCount)) {
    ScreenCharacter *character = buffer;
    int row;

    for (row=0; row<box->height; row+=1) {
      const HelpLineEntry *hle = &lineTable[box->top + row];
      int column;

      for (column=0; column<box->width; column+=1) {
        int index = box->left + column;

        if (index < hle->length) {
          character->text = hle->characters[index];
        } else {
          character->text = WC_C(' ');
        }

        character->attributes = 0X07;
        character += 1;
      }
    }

    return 1;
  }
  return 0;
}

static int
insertKey_HelpScreen (ScreenKey key) {
  switch (key) {
    case SCR_KEY_CURSOR_UP:
      if (cursorRow > 0) {
        cursorRow -= 1;
        return 1;
      }
      break;

    case SCR_KEY_CURSOR_DOWN:
      if (cursorRow < (lineCount - 1)) {
        cursorRow += 1;
        return 1;
      }
      break;

    case SCR_KEY_CURSOR_LEFT:
      if (cursorColumn > 0) {
        cursorColumn -= 1;
        return 1;
      }
      break;

    case SCR_KEY_CURSOR_RIGHT:
      if (cursorColumn < (lineLength - 1)) {
        cursorColumn += 1;
        return 1;
      }
      break;

    default:
      break;
  }

  return 0;
}

static int
routeCursor_HelpScreen (int column, int row, int screen) {
  if (row != -1) {
    if ((row < 0) || (row >= lineCount)) return 0;
    cursorRow = row;
  }

  if (column != -1) {
    if ((column < 0) || (column >= lineLength)) return 0;
    cursorColumn = column;
  }

  return 1;
}

void
initializeHelpScreen (HelpScreen *help) {
  initializeBaseScreen(&help->base);
  help->base.currentVirtualTerminal = currentVirtualTerminal_HelpScreen;
  help->base.describe = describe_HelpScreen;
  help->base.readCharacters = readCharacters_HelpScreen;
  help->base.insertKey = insertKey_HelpScreen;
  help->base.routeCursor = routeCursor_HelpScreen;
  help->construct = construct_HelpScreen;
  help->destruct = destruct_HelpScreen;
  help->addLine = addLine_HelpScreen;
}
