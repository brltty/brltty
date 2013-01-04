/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "log.h"
#include "scr.h"
#include "tunes.h"
#include "clipboard.h"

static wchar_t *clipboardCharacters = NULL;
static size_t clipboardSize = 0;
static size_t clipboardLength = 0;

static int beginColumn = 0;
static int beginRow = 0;
static int beginOffset = -1;

static wchar_t *
cpbAllocateCharacters (size_t count) {
  {
    wchar_t *characters;
    if ((characters = malloc(count * sizeof(*characters)))) return characters;
  }

  logMallocError();
  return NULL;
}

const wchar_t *
cpbGetContent (size_t *length) {
  *length = clipboardLength;
  return clipboardCharacters;
}

void
cpbTruncateContent (size_t length) {
  if (length < clipboardLength) clipboardLength = length;
}

void
cpbClearContent (void) {
  cpbTruncateContent(0);
}

int
cpbAddContent (const wchar_t *characters, size_t length) {
  size_t newLength = clipboardLength + length;

  if (newLength > clipboardSize) {
    size_t newSize = newLength | 0XFF;
    wchar_t *newCharacters = cpbAllocateCharacters(newSize);

    if (!newCharacters) {
      logMallocError();
      return 0;
    }

    wmemcpy(newCharacters, clipboardCharacters, clipboardLength);
    if (clipboardCharacters) free(clipboardCharacters);
    clipboardCharacters = newCharacters;
    clipboardSize = newSize;
  }

  wmemcpy(&clipboardCharacters[clipboardLength], characters, length);
  clipboardLength += length;
  return 1;
}

int
cpbSetContent (const wchar_t *characters, size_t length) {
  cpbClearContent();
  return cpbAddContent(characters, length);
}

static wchar_t *
cpbReadScreen (size_t *length, int fromColumn, int fromRow, int toColumn, int toRow) {
  wchar_t *newBuffer = NULL;
  int columns = toColumn - fromColumn + 1;
  int rows = toRow - fromRow + 1;

  if ((columns >= 1) && (rows >= 1) && (beginOffset >= 0)) {
    wchar_t fromBuffer[rows * columns];

    if (readScreenText(fromColumn, fromRow, columns, rows, fromBuffer)) {
      wchar_t toBuffer[rows * (columns + 1)];
      wchar_t *toAddress = toBuffer;

      const wchar_t *fromAddress = fromBuffer;
      int row;

      for (row=fromRow; row<=toRow; row+=1) {
        int column;

        for (column=fromColumn; column<=toColumn; column+=1) {
          wchar_t character = *fromAddress++;
          if (iswcntrl(character) || iswspace(character)) character = WC_C(' ');
          *toAddress++ = character;
        }

        if (row != toRow) *toAddress++ = WC_C('\r');
      }

      /* make a new permanent buffer of just the right size */
      {
        size_t newLength = toAddress - toBuffer;

        if ((newBuffer = cpbAllocateCharacters(newLength))) {
          wmemcpy(newBuffer, toBuffer, (*length = newLength));
        }
      }
    }
  }

  return newBuffer;
}

static int
cpbEndOperation (const wchar_t *characters, size_t length) {
  cpbTruncateContent(beginOffset);
  if (!cpbAddContent(characters, length)) return 0;
  playTune(&tune_clipboard_end);
  return 1;
}

void
cpbBeginOperation (int column, int row) {
  beginColumn = column;
  beginRow = row;
  beginOffset = clipboardLength;
  playTune(&tune_clipboard_begin);
}

int
cpbRectangularCopy (int column, int row) {
  int copied = 0;
  size_t length;
  wchar_t *buffer = cpbReadScreen(&length, beginColumn, beginRow, column, row);

  if (buffer) {
    {
      const wchar_t *from = buffer;
      const wchar_t *end = from + length;
      wchar_t *to = buffer;
      int spaces = 0;

      while (from != end) {
        wchar_t character = *from++;

        switch (character) {
          case WC_C(' '):
            spaces += 1;
            continue;

          case WC_C('\r'):
            spaces = 0;

          default:
            break;
        }

        while (spaces) {
          *to++ = WC_C(' ');
          spaces -= 1;
        }

        *to++ = character;
      }

      length = to - buffer;
    }

    if (cpbEndOperation(buffer, length)) copied = 1;
    free(buffer);
  }

  return copied;
}

int
cpbLinearCopy (int column, int row) {
  int copied = 0;
  ScreenDescription screen;
  describeScreen(&screen);

  {
    int rightColumn = screen.cols - 1;
    size_t length;
    wchar_t *buffer = cpbReadScreen(&length, 0, beginRow, rightColumn, row);

    if (buffer) {
      if (column < rightColumn) {
        wchar_t *start = buffer + length;
        while (start != buffer) {
          if (*--start == WC_C('\r')) {
            start += 1;
            break;
          }
        }

        {
          int adjustment = (column + 1) - (buffer + length - start);
          if (adjustment < 0) length += adjustment;
        }
      }

      if (beginColumn) {
        wchar_t *start = wmemchr(buffer, WC_C('\r'), length);
        if (!start) start = buffer + length;
        if ((start - buffer) > beginColumn) start = buffer + beginColumn;
        if (start != buffer) wmemmove(buffer, start, (length -= start - buffer));
      }

      {
        const wchar_t *from = buffer;
        const wchar_t *end = from + length;
        wchar_t *to = buffer;
        int spaces = 0;
        int newlines = 0;

        while (from != end) {
          wchar_t character = *from++;

          switch (character) {
            case WC_C(' '):
              spaces += 1;
              continue;

            case WC_C('\r'):
              newlines += 1;
              continue;

            default:
              break;
          }

          if (newlines) {
            if ((newlines > 1) || (spaces > 0)) spaces = 1;
            newlines = 0;
          }

          while (spaces) {
            *to++ = WC_C(' ');
            spaces -= 1;
          }

          *to++ = character;
        }

        length = to - buffer;
      }

      if (cpbEndOperation(buffer, length)) copied = 1;
      free(buffer);
    }
  }

  return copied;
}

int
cpbPaste (void) {
  size_t length;
  const wchar_t *characters = cpbGetContent(&length);

  if (!length) return 0;

  {
    unsigned int i;

    for (i=0; i<length; i+=1)
      if (!insertScreenKey(characters[i]))
        return 0;
  }

  return 1;
}
