/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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

#include "misc.h"
#include "async.h"
#include "message.h"
#include "charset.h"
#include "unicode.h"
#include "drivers.h"
#include "brl.h"
#include "ttb.h"
#include "ktb.h"
#include "brl.auto.h"
#include "cmd.h"
#include "queue.h"

#define BRLSYMBOL noBraille
#define DRIVER_NAME NoBraille
#define DRIVER_CODE no
#define DRIVER_COMMENT "no braille support"
#define DRIVER_VERSION ""
#define DRIVER_DEVELOPERS ""
#include "brl_driver.h"

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  return 1;
}

static void
brl_destruct (BrailleDisplay *brl) {
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return EOF;
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *characters) {
  return 1;
}

const BrailleDriver *braille = &noBraille;

/*
 * Output braille translation tables.
 * The files *.auto.h (the default tables) are generated at compile-time.
 */
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
                    "braille", 'b', "brl",
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
  brl->textColumns = 80;
  brl->textRows = 1;
  brl->statusColumns = 0;
  brl->statusRows = 0;

  brl->keyBindings = "all";
  brl->keyNameTables = NULL;
  brl->keyTable = NULL;

  brl->buffer = NULL;
  brl->writeDelay = 0;
  brl->bufferResized = NULL;
  brl->dataDirectory = NULL;
  brl->touchEnabled = 0;
  brl->highlightWindow = 0;
  brl->data = NULL;
}

unsigned int
drainBrailleOutput (BrailleDisplay *brl, int minimumDelay) {
  int duration = brl->writeDelay + 1;
  if (duration < minimumDelay) duration = minimumDelay;
  brl->writeDelay = 0;
  asyncWait(duration);
  return duration;
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
  memset(brl->buffer, 0, brl->textColumns*brl->textRows);
  LogPrint(infoLevel, "Braille Display Dimensions: %d %s, %d %s",
           brl->textRows, (brl->textRows == 1)? "row": "rows",
           brl->textColumns, (brl->textColumns == 1)? "column": "columns");
  if (brl->bufferResized) brl->bufferResized(infoLevel, brl->textRows, brl->textColumns);
}

static int
resizeBrailleBuffer (BrailleDisplay *brl, int infoLevel) {
  if (brl->resizeRequired) {
    brl->resizeRequired = 0;

    if (brl->isCoreBuffer) {
      static void *currentAddress = NULL;
      static size_t currentSize = 0;
      size_t newSize = brl->textColumns * brl->textRows;

      if (newSize > currentSize) {
        void *newAddress = malloc(newSize);

        if (!newAddress) {
          LogError("malloc");
          return 0;
        }

        if (currentAddress) free(currentAddress);
        currentAddress = newAddress;
        currentSize = newSize;
      }

      brl->buffer = currentAddress;
    }

    brailleBufferResized(brl, infoLevel);
  }

  return 1;
}

int
ensureBrailleBuffer (BrailleDisplay *brl, int infoLevel) {
  if ((brl->isCoreBuffer = brl->resizeRequired = brl->buffer == NULL)) {
    if (!resizeBrailleBuffer(brl, infoLevel)) return 0;
  } else {
    brailleBufferResized(brl, infoLevel);
  }
  return 1;
}

int
readBrailleCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  int command = dequeueCommand();

  if (command == EOF) {
    command = braille->readCommand(brl, context);
    resizeBrailleBuffer(brl, LOG_INFO);

    {
      unsigned char set;
      unsigned char key;
      int press;

      while (dequeueKeyEvent(&set, &key, &press)) {
        if (brl->keyTable) {
          processKeyEvent(brl->keyTable, context, set, key, press);
        }
      }
    }

    if (command == EOF) command = dequeueCommand();
  }

  return command;
}

#ifdef ENABLE_LEARN_MODE
int
learnMode (BrailleDisplay *brl, int poll, int timeout) {
  const char *mode = "lrn";

  if (!setStatusText(brl, mode)) return 0;
  if (!message(mode, "command learn mode", MSG_NODELAY)) return 0;

  hasTimedOut(0);
  do {
    int command = readBrailleCommand(brl, BRL_CTX_DEFAULT);
    if (command != EOF) {
      LogPrint(LOG_DEBUG, "Learn: command=%06X", command);
      if (IS_DELAYED_COMMAND(command)) continue;

      {
        int cmd = command & BRL_MSK_CMD;
        if (cmd == BRL_CMD_NOOP) continue;
        if (cmd == BRL_CMD_LEARN) return 1;
      }

      {
        char buffer[0X100];
        describeCommand(command, buffer, sizeof(buffer));
        LogPrint(LOG_DEBUG, "Learn: %s", buffer);
        if (!message(mode, buffer, MSG_NODELAY|MSG_SILENT)) return 0;
      }

      hasTimedOut(0);
    }

    drainBrailleOutput(brl, poll);
  } while (!hasTimedOut(timeout));

  return message(mode, "done", 0);
}
#endif /* ENABLE_LEARN_MODE */

void
reverseTranslationTable (TranslationTable from, TranslationTable to) {
  int byte;
  memset(to, 0, sizeof(TranslationTable));
  for (byte=TRANSLATION_TABLE_SIZE-1; byte>=0; byte--) to[from[byte]] = byte;
}

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
setBrailleFirmness (BrailleDisplay *brl, BrailleFirmness setting) {
  LogPrint(LOG_DEBUG, "setting braille firmness: %d", setting);
  braille->firmness(brl, setting);
}

void
setBrailleSensitivity (BrailleDisplay *brl, BrailleSensitivity setting) {
  LogPrint(LOG_DEBUG, "setting braille sensitivity: %d", setting);
  braille->sensitivity(brl, setting);
}

typedef struct {
  unsigned char set;
  unsigned char key;
  unsigned press:1;
} KeyEventQueueItem;

static Queue *
getKeyEventQueue (int create) {
  static Queue *keyEventQueue = NULL;

  if (create && !keyEventQueue) {
    keyEventQueue = newQueue(NULL, NULL);
  }

  return keyEventQueue;
}

int
enqueueKeyEvent (unsigned char set, unsigned char key, int press) {
  Queue *queue = getKeyEventQueue(1);

  if (queue) {
    KeyEventQueueItem *item = malloc(sizeof(KeyEventQueueItem));

    if (item) {
      item->set = set;
      item->key = key;
      item->press = press;
      if (enqueueItem(queue, item)) return 1;

      free(item);
    }
  }

  return 0;
}

int
dequeueKeyEvent (unsigned char *set, unsigned char *key, int *press) {
  Queue *queue = getKeyEventQueue(0);

  if (queue) {
    KeyEventQueueItem *item = dequeueItem(queue);

    if (item) {
      *set = item->set;
      *key = item->key;
      *press = item->press;
      free(item);
      return 1;
    }
  }

  return 0;
}
