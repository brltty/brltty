/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "scr.h"
#include "tunes.h"
#include "cut.h"

/* Global state variables */
unsigned char *cut_buffer = NULL;
static short int beginColumn = 0, beginRow = 0;
static unsigned int previousLength = 0;

static unsigned char *
cut (int fromColumn, int fromRow, int toColumn, int toRow) {
  unsigned char *newBuffer = NULL;
  int columns = toColumn - fromColumn + 1;
  int rows = toRow - fromRow + 1;

  if ((columns >= 1) && (rows >= 1)) {
    unsigned char *fromBuffer = (char *)malloc(rows * columns);
    if (fromBuffer) {
      unsigned char *toBuffer = (char *)malloc(rows * (columns + 1));
      if (toBuffer) {
        if (readScreen((ScreenBox){fromColumn, fromRow, columns, rows}, fromBuffer, SCR_TEXT)) {
          unsigned char *fromAddress = fromBuffer;
          unsigned char *toAddress = toBuffer;
          int row;
          /* remove spaces at end of line, add return (except to last line),
             and possibly remove non-printables... if any */
          for (row=fromRow; row<=toRow; row++) {
            int spaces = 0;
            int column;
            for (column=fromColumn; column<=toColumn; column++, fromAddress++) {
              if (iscntrl(*fromAddress) || isspace(*fromAddress))
                spaces++;
              else {
                while (spaces) {
                  *(toAddress++) = ' ';
                  spaces--;
                }
                *(toAddress++) = *fromAddress;
              }
            }
            if (row != toRow) *(toAddress++) = '\r';
          }
          *toAddress++ = 0;

          /* make a new permanent buffer of just the right size */
          {
            int length = toAddress - toBuffer;
            if ((newBuffer = (unsigned char *)malloc(length))) {
              memcpy(newBuffer, toBuffer, length);
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
append (unsigned char *buffer) {
  if (cut_buffer) {
    unsigned char *newBuffer = (unsigned char *)malloc(previousLength + strlen(buffer) + 1);
    if (!newBuffer) return 0;
    memcpy(newBuffer, cut_buffer, previousLength);
    strcpy(newBuffer+previousLength, buffer);
    free(buffer);
    free(cut_buffer);
    cut_buffer = newBuffer;
  } else {
    cut_buffer = buffer;
  }
  playTune(&tune_cut_end);
  return 1;
}

static void
start (int column, int row) {
  beginColumn = column;
  beginRow = row;
  playTune(&tune_cut_begin);
}

void
cut_begin (int column, int row) {
  if (cut_buffer) {
    free(cut_buffer);
    cut_buffer = NULL;
  }
  previousLength = 0;
  start(column, row);
}

void
cut_append (int column, int row) {
  previousLength = cut_buffer? strlen(cut_buffer): 0;
  start(column, row);
}

int
cut_rectangle (int column, int row) {
  unsigned char *buffer = cut(beginColumn, beginRow, column, row);
  if (buffer) {
    if (append(buffer)) return 1;
    free(buffer);
  }
  return 0;
}

int
cut_line (int column, int row) {
  ScreenDescription screen;
  describeScreen(&screen);
  {
    int width = screen.cols;
    int rightColumn = width - 1;
    unsigned char *buffer = cut(0, beginRow, rightColumn, row);
    if (buffer) {
      if (column < rightColumn) {
        int length = column + 1;
        unsigned char *start = strrchr(buffer, '\r');
        start = start? start+1: buffer;
        if (length < strlen(start)) start[length] = 0;
      }
      if (beginColumn) {
        unsigned char *start = strchr(buffer, '\r');
        if (!start) start = buffer + strlen(buffer);
        if ((start - buffer) > beginColumn) start = buffer + beginColumn;
        if (start != buffer) memmove(buffer, start, strlen(start)+1);
      }
      if (append(buffer)) return 1;
      free(buffer);
    }
    return 0;
  }
}

int
cut_paste (void) {
  if (cut_buffer)
    if (insertString(cut_buffer))
      return 1;
  return 0;
}
