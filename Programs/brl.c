/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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
#include <errno.h>
#include <fcntl.h>

#include "misc.h"
#include "message.h"
#include "drivers.h"
#include "brl.h"
#include "tbl.h"
#include "brl.auto.h"
#include "cmd.h"

#define BRLSYMBOL noBraille
#define DRIVER_NAME NoBraille
#define DRIVER_CODE no
#define DRIVER_COMMENT "no braille support"
#define DRIVER_VERSION ""
#define DRIVER_COPYRIGHT ""
#define BRLHELP "/dev/null"
#include "brl_driver.h"
static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  brl->x = 80;
  brl->y = 1;
  return 1;
}
static void
brl_close (BrailleDisplay *brl) {
}
static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return EOF;
}
static void
brl_writeWindow (BrailleDisplay *brl) {
}
static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
}

const BrailleDriver *braille = &noBraille;

/*
 * Output braille translation tables.
 * The files *.auto.h (the default tables) are generated at compile-time.
 */
TranslationTable textTable = {
  #include "text.auto.h"
};
TranslationTable untextTable;

TranslationTable attributesTable = {
  #include "attrib.auto.h"
};

void *contractionTable = NULL;

int
haveBrailleDriver (const char *code) {
  return haveDriver(code, BRAILLE_DRIVER_CODES, driverTable);
}

const char *
getDefaultBrailleDriver (void) {
  return getDefaultDriver(driverTable);
}

const BrailleDriver *
loadBrailleDriver (const char *code, void **driverObject, const char *driverDirectory) {
  return loadDriver(code, driverObject,
                    driverDirectory, driverTable,
                    "braille", 'b', "brl_driver",
                    &noBraille, &noBraille.definition);
}

void
identifyBrailleDriver (const BrailleDriver *driver, int full) {
  identifyDriver("Braille", &driver->definition, full);
}

void
identifyBrailleDrivers (int full) {
  const DriverEntry *entry = driverTable;
  while (entry->address) {
    const BrailleDriver *driver = entry++->address;
    identifyBrailleDriver(driver, full);
  }
}

void
initializeBrailleDisplay (BrailleDisplay *brl) {
   brl->x = brl->y = 0;
   brl->helpPage = 0;
   brl->buffer = NULL;
   brl->writeDelay = 0;
   brl->bufferResized = NULL;
   brl->dataDirectory = NULL;
}

unsigned int
drainBrailleOutput (BrailleDisplay *brl, int minimumDelay) {
  int duration = brl->writeDelay + 1;
  if (duration < minimumDelay) duration = minimumDelay;
  brl->writeDelay = 0;
  approximateDelay(duration);
  return duration;
}

void
writeBrailleBuffer (BrailleDisplay *brl) {
  brl->cursor = -1;
  if (braille->writeVisual) braille->writeVisual(brl);

  {
    int i;
    /* Do Braille translation using text table. Six-dot mode is ignored
     * since case can be important, and blinking caps won't work. 
     */
    for (i=0; i<brl->x*brl->y; ++i) brl->buffer[i] = textTable[brl->buffer[i]];
  }
  braille->writeWindow(brl);
}

void
writeBrailleText (BrailleDisplay *brl, const char *text, int length) {
  int width = brl->x * brl->y;
  if (length > width) length = width;
  memcpy(brl->buffer, text, length);
  memset(&brl->buffer[length], ' ', width-length);
  writeBrailleBuffer(brl);
}

void
writeBrailleString (BrailleDisplay *brl, const char *string) {
  writeBrailleText(brl, string, strlen(string));
}

void
showBrailleString (BrailleDisplay *brl, const char *string, unsigned int duration) {
  writeBrailleString(brl, string);
  drainBrailleOutput(brl, duration);
}

void
clearStatusCells (BrailleDisplay *brl) {
  unsigned char status[BRL_MAX_STATUS_CELL_COUNT];        /* status cell buffer */
  memset(status, 0, sizeof(status));
  braille->writeStatus(brl, status);
}

void
setStatusText (BrailleDisplay *brl, const char *text) {
  unsigned char status[BRL_MAX_STATUS_CELL_COUNT];        /* status cell buffer */
  int i;
  memset(status, 0, sizeof(status));
  for (i=0; i<sizeof(status); ++i) {
    unsigned char character = text[i];
    if (!character) break;
    status[i] = textTable[character];
  }
  braille->writeStatus(brl, status);
}

static void
brailleBufferResized (BrailleDisplay *brl) {
  memset(brl->buffer, 0, brl->x*brl->y);
  LogPrint(LOG_INFO, "Braille Display Dimensions: %d %s, %d %s",
           brl->y, (brl->y == 1)? "row": "rows",
           brl->x, (brl->x == 1)? "column": "columns");
  if (brl->bufferResized) brl->bufferResized(brl->y, brl->x);
}

static int
resizeBrailleBuffer (BrailleDisplay *brl) {
  if (brl->resizeRequired) {
    brl->resizeRequired = 0;
    if (brl->isCoreBuffer) {
      int size = brl->x * brl->y;
      unsigned char *buffer = realloc(brl->buffer, size);
      if (!buffer) {
        LogError("braille buffer allocation");
        return 0;
      }
      brl->buffer = buffer;
    }
    brailleBufferResized(brl);
  }
  return 1;
}

int
allocateBrailleBuffer (BrailleDisplay *brl) {
  if ((brl->isCoreBuffer = brl->resizeRequired = brl->buffer == NULL)) {
    if (!resizeBrailleBuffer(brl)) return 0;
  } else {
    brailleBufferResized(brl);
  }
  return 1;
}

int
readBrailleCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  int command = braille->readCommand(brl, context);
  resizeBrailleBuffer(brl);
  return command;
}

#ifdef ENABLE_LEARN_MODE
void
learnMode (BrailleDisplay *brl, int poll, int timeout) {
  setStatusText(brl, "lrn");
  message("command learn mode", MSG_NODELAY);

  hasTimedOut(0);
  do {
    int command = readBrailleCommand(brl, BRL_CTX_SCREEN);
    if (command != EOF) {
      LogPrint(LOG_DEBUG, "Learn: command=%06X", command);
      if (IS_DELAYED_COMMAND(command)) continue;

      {
        int cmd = command & BRL_MSK_CMD;
        if (cmd == BRL_CMD_NOOP) continue;
        if (cmd == BRL_CMD_LEARN) return;
      }

      {
        char buffer[0X100];
        describeCommand(command, buffer, sizeof(buffer));
        LogPrint(LOG_DEBUG, "Learn: %s", buffer);
        message(buffer, MSG_NODELAY|MSG_SILENT);
      }

      hasTimedOut(0);
    }

    drainBrailleOutput(brl, poll);
  } while (!hasTimedOut(timeout));
  message("done", 0);
}
#endif /* ENABLE_LEARN_MODE */

void
makeOutputTable (const DotsTable dots, TranslationTable table) {
  static const DotsTable internalDots = {BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4, BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8};
  int byte, dot;
  memset(table, 0, sizeof(TranslationTable));
  for (byte=0; byte<TRANSLATION_TABLE_SIZE; byte++)
    for (dot=0; dot<DOTS_TABLE_SIZE; dot++)
      if (byte & internalDots[dot])
        table[byte] |= dots[dot];
}

void
makeUntextTable (void) {
  reverseTranslationTable(textTable, untextTable);
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

void
setBrailleFirmness (BrailleDisplay *brl, int setting) {
  LogPrint(LOG_DEBUG, "setting braille firmness: %d", setting);
  braille->firmness(brl, setting);
}
