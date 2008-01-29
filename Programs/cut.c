/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "scr.h"
#include "tunes.h"
#include "charset.h"
#include "cut.h"

/* Global state variables */
wchar_t *cutBuffer = NULL;
size_t cutLength = 0;

static int beginColumn = 0;
static int beginRow = 0;
static int beginOffset = -1;

static wchar_t *
mallocWchars (size_t count) {
  return malloc(count * sizeof(wchar_t));
}

static wchar_t *
cut (size_t *length, int fromColumn, int fromRow, int toColumn, int toRow) {
  wchar_t *newBuffer = NULL;
  int columns = toColumn - fromColumn + 1;
  int rows = toRow - fromRow + 1;

  if ((columns >= 1) && (rows >= 1) && (beginOffset >= 0)) {
    wchar_t *fromBuffer = mallocWchars(rows * columns);
    if (fromBuffer) {
      wchar_t *toBuffer = mallocWchars(rows * (columns + 1));
      if (toBuffer) {
        if (readScreenText(fromColumn, fromRow, columns, rows, fromBuffer)) {
          wchar_t *fromAddress = fromBuffer;
          wchar_t *toAddress = toBuffer;
          int row;

          /* remove spaces at end of line, add return (except to last line),
           * and possibly remove non-printables... if any
           */
          for (row=fromRow; row<=toRow; row++) {
            int spaces = 0;
            int column;

            for (column=fromColumn; column<=toColumn; column++, fromAddress++) {
              if (iswcntrl(*fromAddress) || iswspace(*fromAddress)) {
                spaces++;
              } else {
                while (spaces) {
                  *(toAddress++) = L' ';
                  spaces--;
                }

                *toAddress++ = *fromAddress;
              }
            }

            if (row != toRow) *toAddress++ = L'\r';
          }

          /* make a new permanent buffer of just the right size */
          {
            size_t newLength = toAddress - toBuffer;
            if ((newBuffer = mallocWchars(newLength))) {
              wmemcpy(newBuffer, toBuffer, (*length = newLength));
            }
          }
        }
        
        free(toBuffer);
      }

      free(fromBuffer);
    }
  }

  return newBuffer;
}

static int
append (wchar_t *buffer, size_t length) {
  if (cutBuffer) {
    size_t newLength = beginOffset + length;
    wchar_t *newBuffer = mallocWchars(newLength);
    if (!newBuffer) return 0;

    wmemcpy(newBuffer, cutBuffer, beginOffset);
    wmemcpy(newBuffer+beginOffset, buffer, length);

    free(buffer);
    free(cutBuffer);

    cutBuffer = newBuffer;
    cutLength = newLength;
  } else {
    cutBuffer = buffer;
    cutLength = length;
  }

  playTune(&tune_cut_end);
  return 1;
}

void
cutClear (void) {
  if (cutBuffer) {
    free(cutBuffer);
    cutBuffer = NULL;
  }
  cutLength = 0;
}

void
cutBegin (int column, int row) {
  cutClear();
  cutAppend(column, row);
}

void
cutAppend (int column, int row) {
  beginColumn = column;
  beginRow = row;
  beginOffset = cutLength;
  playTune(&tune_cut_begin);
}

int
cutRectangle (int column, int row) {
  size_t length;
  wchar_t *buffer = cut(&length, beginColumn, beginRow, column, row);

  if (buffer) {
    if (append(buffer, length)) return 1;
    free(buffer);
  }
  return 0;
}

int
cutLine (int column, int row) {
  ScreenDescription screen;
  describeScreen(&screen);

  {
    int rightColumn = screen.cols - 1;
    size_t length;
    wchar_t *buffer = cut(&length, 0, beginRow, rightColumn, row);

    if (buffer) {
      if (column < rightColumn) {
        wchar_t *start = buffer + length;
        while (start != buffer) {
          if (*--start == L'\r') {
            ++start;
            break;
          }
        }

        {
          int adjustment = (column + 1) - (buffer + length - start);
          if (adjustment < 0) length += adjustment;
        }
      }

      if (beginColumn) {
        wchar_t *start = wmemchr(buffer, L'\r', length);
        if (!start) start = buffer + length;
        if ((start - buffer) > beginColumn) start = buffer + beginColumn;
        if (start != buffer) wmemmove(buffer, start, (length -= start - buffer));
      }

      if (append(buffer, length)) return 1;
      free(buffer);
    }
  }

  return 0;
}

int
cutPaste (void) {
  if (cutLength) {
    char characters[cutLength];
    int i;

    for (i=0; i<cutLength; ++i) {
      int c = convertWcharToChar(cutBuffer[i]);
      if (c == EOF) c = '?';
      characters[i] = c;
    }

    if (insertCharacters(characters, cutLength)) return 1;
  }

  return 0;
}
