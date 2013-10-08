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
#include <errno.h>
#include <fcntl.h>

#include "log.h"
#include "timing.h"
#include "async_wait.h"
#include "message.h"
#include "charset.h"
#include "unicode.h"
#include "drivers.h"
#include "io_generic.h"
#include "brl.h"
#include "ttb.h"
#include "ktb.h"
#include "cmd.h"
#include "prefs.h"
#include "brltty.h"

void
initializeBrailleDisplay (BrailleDisplay *brl) {
  brl->textColumns = 0;
  brl->textRows = 1;
  brl->statusColumns = 0;
  brl->statusRows = 0;

  brl->keyBindings = "all";
  brl->keyNameTables = NULL;
  brl->keyTable = NULL;

  brl->gioEndpoint = NULL;
  brl->buffer = NULL;
  brl->writeDelay = 0;

  brl->bufferResized = NULL;
  brl->touchEnabled = 0;
  brl->highlightWindow = 0;
  brl->data = NULL;
  brl->setFirmness = NULL;
  brl->setSensitivity = NULL;
  brl->rotateKey = NULL;
}

static void
fillRegion (
  wchar_t *text, unsigned char *dots,
  unsigned int start, unsigned int count,
  unsigned int columns, unsigned int rows,
  void *data, unsigned int length,
  void (*fill) (wchar_t *text, unsigned char *dots, void *data)
) {
  text += start;
  dots += start;

  while (rows > 0) {
    unsigned int index = 0;
    size_t amount = length;
    if (amount > count) amount = count;

    while (index < amount) {
      fill(&text[index], &dots[index], data);
      index += 1;
    }
    length -= amount;

    amount = count - index;
    wmemset(&text[index], WC_C(' '), amount);
    memset(&dots[index], 0, amount);

    text += columns;
    dots += columns;
    rows -= 1;
  }
}

static void
fillText (wchar_t *text, unsigned char *dots, void *data) {
  wchar_t **character = data;
  *dots = convertCharacterToDots(textTable, (*text = *(*character)++));
}

void
fillTextRegion (
  wchar_t *text, unsigned char *dots,
  unsigned int start, unsigned int count,
  unsigned int columns, unsigned int rows,
  const wchar_t *characters, size_t length
) {
  fillRegion(text, dots, start, count, columns, rows, &characters, length, fillText);
}

static void
fillDots (wchar_t *text, unsigned char *dots, void *data) {
  unsigned char **cell = data;
  *text = UNICODE_BRAILLE_ROW | (*dots = *(*cell)++);
}

void
fillDotsRegion (
  wchar_t *text, unsigned char *dots,
  unsigned int start, unsigned int count,
  unsigned int columns, unsigned int rows,
  const unsigned char *cells, size_t length
) {
  fillRegion(text, dots, start, count, columns, rows, &cells, length, fillDots);
}


int
setStatusText (BrailleDisplay *brl, const char *text) {
  unsigned int length = brl->statusColumns * brl->statusRows;

  if (braille->writeStatus && (length > 0)) {
    unsigned char cells[length];

    {
      unsigned int index = 0;

      while (index < length) {
        char c;
        wint_t wc;

        if (!(c = text[index])) break;
        if ((wc = convertCharToWchar(c)) == WEOF) wc = WC_C('?');
        cells[index++] = convertCharacterToDots(textTable, wc);
      }

      memset(&cells[index], 0, length-index);
    }

    if (!braille->writeStatus(brl, cells)) return 0;
  }

  return 1;
}

int
clearStatusCells (BrailleDisplay *brl) {
  return setStatusText(brl, "");
}

static void
brailleBufferResized (BrailleDisplay *brl, int infoLevel) {
  logMessage(infoLevel, "Braille Display Dimensions: %d %s, %d %s",
             brl->textRows, (brl->textRows == 1)? "row": "rows",
             brl->textColumns, (brl->textColumns == 1)? "column": "columns");

  memset(brl->buffer, 0, brl->textColumns*brl->textRows);
  if (brl->bufferResized) brl->bufferResized(brl->textRows, brl->textColumns);
}

static int
resizeBrailleBuffer (BrailleDisplay *brl, int resized, int infoLevel) {
  if (brl->resizeRequired) {
    brl->resizeRequired = 0;
    resized = 1;

    if (brl->isCoreBuffer) {
      size_t newSize = brl->textColumns * brl->textRows;
      unsigned char *newAddress = malloc(newSize);

      if (!newAddress) {
        logMallocError();
        return 0;
      }

      if (brl->buffer) free(brl->buffer);
      brl->buffer = newAddress;
    }
  }

  if (resized) brailleBufferResized(brl, infoLevel);
  return 1;
}

int
ensureBrailleBuffer (BrailleDisplay *brl, int infoLevel) {
  brl->resizeRequired = brl->isCoreBuffer = !brl->buffer;
  if ((brl->noDisplay = !brl->textColumns)) brl->textColumns = 1;
  return resizeBrailleBuffer(brl, 1, infoLevel);
}

int
readBrailleCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  int command = braille->readCommand(brl, context);

  resizeBrailleBuffer(brl, 0, LOG_INFO);
  return command;
}

void
applyBrailleOrientation (unsigned char *cells, size_t count) {
  switch (prefs.brailleOrientation) {
    case BRL_ORIENTATION_ROTATED: {
      static TranslationTable rotateTable = {[1] = 0};

      const unsigned char *source = cells;
      const unsigned char *end = source + count;

      unsigned char buffer[count];
      unsigned char *target = &buffer[count];

      if (!rotateTable[1]) {
        static const DotsTable dotsTable = {
          BRL_DOT8, BRL_DOT6, BRL_DOT5, BRL_DOT7,
          BRL_DOT3, BRL_DOT2, BRL_DOT4, BRL_DOT1
        };

        makeTranslationTable(dotsTable, rotateTable);
      }

      while (source < end) *--target = rotateTable[*source++];
      memcpy(cells, buffer, count);
      break;
    }

    default:
    case BRL_ORIENTATION_NORMAL:
      break;
  }
}

/* Functions which support vertical and horizontal status cells. */

unsigned char
lowerDigit (unsigned char upper) {
  unsigned char lower = 0;
  if (upper & BRL_DOT1) lower |= BRL_DOT3;
  if (upper & BRL_DOT2) lower |= BRL_DOT7;
  if (upper & BRL_DOT4) lower |= BRL_DOT6;
  if (upper & BRL_DOT5) lower |= BRL_DOT8;
  return lower;
}

/* Dots for landscape (counterclockwise-rotated) digits. */
const unsigned char landscapeDigits[11] = {
  BRL_DOT1+BRL_DOT5+BRL_DOT2, BRL_DOT4,
  BRL_DOT4+BRL_DOT1,          BRL_DOT4+BRL_DOT5,
  BRL_DOT4+BRL_DOT5+BRL_DOT2, BRL_DOT4+BRL_DOT2,
  BRL_DOT4+BRL_DOT1+BRL_DOT5, BRL_DOT4+BRL_DOT1+BRL_DOT5+BRL_DOT2,
  BRL_DOT4+BRL_DOT1+BRL_DOT2, BRL_DOT1+BRL_DOT5,
  BRL_DOT1+BRL_DOT2+BRL_DOT4+BRL_DOT5
};

/* Format landscape representation of numbers 0 through 99. */
int
landscapeNumber (int x) {
  return landscapeDigits[(x / 10) % 10] | lowerDigit(landscapeDigits[x % 10]);  
}

/* Format landscape flag state indicator. */
int
landscapeFlag (int number, int on) {
  int dots = landscapeDigits[number % 10];
  if (on) dots |= lowerDigit(landscapeDigits[10]);
  return dots;
}

/* Dots for seascape (clockwise-rotated) digits. */
const unsigned char seascapeDigits[11] = {
  BRL_DOT5+BRL_DOT1+BRL_DOT4, BRL_DOT2,
  BRL_DOT2+BRL_DOT5,          BRL_DOT2+BRL_DOT1,
  BRL_DOT2+BRL_DOT1+BRL_DOT4, BRL_DOT2+BRL_DOT4,
  BRL_DOT2+BRL_DOT5+BRL_DOT1, BRL_DOT2+BRL_DOT5+BRL_DOT1+BRL_DOT4,
  BRL_DOT2+BRL_DOT5+BRL_DOT4, BRL_DOT5+BRL_DOT1,
  BRL_DOT1+BRL_DOT2+BRL_DOT4+BRL_DOT5
};

/* Format seascape representation of numbers 0 through 99. */
int
seascapeNumber (int x) {
  return lowerDigit(seascapeDigits[(x / 10) % 10]) | seascapeDigits[x % 10];  
}

/* Format seascape flag state indicator. */
int
seascapeFlag (int number, int on) {
  int dots = lowerDigit(seascapeDigits[number % 10]);
  if (on) dots |= seascapeDigits[10];
  return dots;
}

/* Dots for portrait digits - 2 numbers in one cells */
const unsigned char portraitDigits[11] = {
  BRL_DOT2+BRL_DOT4+BRL_DOT5, BRL_DOT1,
  BRL_DOT1+BRL_DOT2,          BRL_DOT1+BRL_DOT4,
  BRL_DOT1+BRL_DOT4+BRL_DOT5, BRL_DOT1+BRL_DOT5,
  BRL_DOT1+BRL_DOT2+BRL_DOT4, BRL_DOT1+BRL_DOT2+BRL_DOT4+BRL_DOT5,
  BRL_DOT1+BRL_DOT2+BRL_DOT5, BRL_DOT2+BRL_DOT4,
  BRL_DOT1+BRL_DOT2+BRL_DOT4+BRL_DOT5
};

/* Format portrait representation of numbers 0 through 99. */
int
portraitNumber (int x) {
  return portraitDigits[(x / 10) % 10] | lowerDigit(portraitDigits[x % 10]);  
}

/* Format portrait flag state indicator. */
int
portraitFlag (int number, int on) {
  int dots = lowerDigit(portraitDigits[number % 10]);
  if (on) dots |= portraitDigits[10];
  return dots;
}

int
setBrailleFirmness (BrailleDisplay *brl, BrailleFirmness setting) {
  if (!brl->setFirmness) return 0;
  logMessage(LOG_DEBUG, "setting braille firmness: %d", setting);
  return brl->setFirmness(brl, setting);
}

int
setBrailleSensitivity (BrailleDisplay *brl, BrailleSensitivity setting) {
  if (!brl->setSensitivity) return 0;
  logMessage(LOG_DEBUG, "setting braille sensitivity: %d", setting);
  return brl->setSensitivity(brl, setting);
}
