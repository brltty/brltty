/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "scr.h"
#include "tunes.h"
#include "cut.h"

/* Global state variables */
unsigned char *cutBuffer = NULL;
size_t cutLength = 0;

static int beginColumn = 0;
static int beginRow = 0;

static unsigned char *
cut (size_t *length, int fromColumn, int fromRow, int toColumn, int toRow) {
  unsigned char *newBuffer = NULL;
  int columns = toColumn - fromColumn + 1;
  int rows = toRow - fromRow + 1;

  if ((columns >= 1) && (rows >= 1)) {
    unsigned char *fromBuffer = malloc(rows * columns);
    if (fromBuffer) {
      unsigned char *toBuffer = malloc(rows * (columns + 1));
      if (toBuffer) {
        if (readScreen(fromColumn, fromRow, columns, rows, fromBuffer, SCR_TEXT)) {
          unsigned char *fromAddress = fromBuffer;
          unsigned char *toAddress = toBuffer;
          int row;

          /* remove spaces at end of line, add return (except to last line),
           * and possibly remove non-printables... if any
           */
          for (row=fromRow; row<=toRow; row++) {
            int spaces = 0;
            int column;

            for (column=fromColumn; column<=toColumn; column++, fromAddress++) {
              if (iscntrl(*fromAddress) || isspace(*fromAddress)) {
                spaces++;
              } else {
                while (spaces) {
                  *(toAddress++) = ' ';
                  spaces--;
                }

                *(toAddress++) = *fromAddress;
              }
            }

            if (row != toRow) *(toAddress++) = '\r';
          }

          /* make a new permanent buffer of just the right size */
          {
            size_t newLength = toAddress - toBuffer;
            if ((newBuffer = malloc(newLength))) {
              memcpy(newBuffer, toBuffer, (*length = newLength));
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
append (unsigned char *buffer, size_t length) {
  if (cutBuffer) {
    size_t newLength = cutLength + length;
    unsigned char *newBuffer = malloc(newLength);
    if (!newBuffer) return 0;

    memcpy(newBuffer, cutBuffer, cutLength);
    memcpy(newBuffer+cutLength, buffer, length);

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
cutBegin (int column, int row) {
  if (cutBuffer) {
    free(cutBuffer);
    cutBuffer = NULL;
  }
  cutLength = 0;

  cutAppend(column, row);
}

void
cutAppend (int column, int row) {
  beginColumn = column;
  beginRow = row;
  playTune(&tune_cut_begin);
}

int
cutRectangle (int column, int row) {
  size_t length;
  unsigned char *buffer = cut(&length, beginColumn, beginRow, column, row);

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
    unsigned char *buffer = cut(&length, 0, beginRow, rightColumn, row);

    if (buffer) {
      if (column < rightColumn) {
        unsigned char *start = buffer + length;
        while (start != buffer) {
          if (*--start == '\r') {
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
        unsigned char *start = memchr(buffer, '\r', length);
        if (!start) start = buffer + length;
        if ((start - buffer) > beginColumn) start = buffer + beginColumn;
        if (start != buffer) memmove(buffer, start, (length -= start - buffer));
      }

      if (append(buffer, length)) return 1;
      free(buffer);
    }
  }

  return 0;
}

int
cutPaste (void) {
  return cutBuffer && insertCharacters((char *)cutBuffer, cutLength);
}
