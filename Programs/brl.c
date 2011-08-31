/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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
#include "brltty.h"

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
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
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
  brl->touchEnabled = 0;
  brl->highlightWindow = 0;
  brl->data = NULL;
  brl->setFirmness = NULL;
  brl->setSensitivity = NULL;
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
  logMessage(infoLevel, "Braille Display Dimensions: %d %s, %d %s",
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
          logMallocError();
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

typedef struct {
  int command;
} CommandQueueItem;

static Queue *
getCommandQueue (int create) {
  static Queue *commandQueue = NULL;

  if (create && !commandQueue) {
    commandQueue = newQueue(NULL, NULL);
  }

  return commandQueue;
}

int
enqueueCommand (int command) {
  if (command != EOF) {
    Queue *queue = getCommandQueue(1);

    if (queue) {
      CommandQueueItem *item = malloc(sizeof(CommandQueueItem));

      if (item) {
        item->command = command;
        if (enqueueItem(queue, item)) return 1;

        free(item);
      }
    }
  }

  return 0;
}

static int
dequeueCommand (void) {
  Queue *queue = getCommandQueue(0);

  if (queue) {
    CommandQueueItem *item;

    while ((item = dequeueItem(queue))) {
      int command = item->command;
      free(item);

#ifdef ENABLE_API
      if (apiStarted)
        if ((command = api_handleCommand(command)) == EOF)
          continue;
#endif /* ENABLE_API */

      return command;
    }
  }

  return EOF;
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

static int
dequeueKeyEvent (unsigned char *set, unsigned char *key, int *press) {
  Queue *queue = getKeyEventQueue(0);

  if (queue) {
    KeyEventQueueItem *item;

    while ((item = dequeueItem(queue))) {
#ifdef ENABLE_API
      if (apiStarted) {
        if ((api_handleKeyEvent(item->set, item->key, item->press)) == EOF) {
          free(item);
	  continue;
	}
      }
#endif /* ENABLE_API */

      *set = item->set;
      *key = item->key;
      *press = item->press;
      free(item);
      return 1;
    }
  }

  return 0;
}

int
enqueueKey (unsigned char set, unsigned char key) {
  if (enqueueKeyEvent(set, key, 1))
    if (enqueueKeyEvent(set, key, 0))
      return 1;

  return 0;
}

int
enqueueKeys (uint32_t bits, unsigned char set, unsigned char key) {
  unsigned char stack[0X20];
  unsigned char count = 0;

  while (bits) {
    if (bits & 0X1) {
      if (!enqueueKeyEvent(set, key, 1)) return 0;
      stack[count++] = key;
    }

    bits >>= 1;
    key += 1;
  }

  while (count)
    if (!enqueueKeyEvent(set, stack[--count], 0))
      return 0;

  return 1;
}

int
enqueueUpdatedKeys (uint32_t new, uint32_t *old, unsigned char set, unsigned char key) {
  uint32_t bit = 0X1;
  unsigned char stack[0X20];
  unsigned char count = 0;

  while (*old != new) {
    if ((new & bit) && !(*old & bit)) {
      stack[count++] = key;
      *old |= bit;
    } else if (!(new & bit) && (*old & bit)) {
      if (!enqueueKeyEvent(set, key, 0)) return 0;
      *old &= ~bit;
    }

    key += 1;
    bit <<= 1;
  }

  while (count)
    if (!enqueueKeyEvent(set, stack[--count], 1))
      return 0;

  return 1;
}

int
enqueueXtScanCode (
  unsigned char key, unsigned char escape,
  unsigned char set00, unsigned char setE0, unsigned char setE1
) {
  int command = BRL_BLK_PASSXT | key;
  switch (escape) {
    case 0XE0:
      command |= BRL_FLG_KBD_EMUL0;
      break;

    case 0XE1:
      command |= BRL_FLG_KBD_EMUL1;
      break;

    default:
    case 0X00:
      break;
  }

  return enqueueCommand(command);
}

static KeyTableCommandContext currentCommandContext = KTB_CTX_DEFAULT;

int
readBrailleCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  currentCommandContext = context;

  {
    int command = dequeueCommand();
    if (command != EOF) return command;
  }

  {
    int command = braille->readCommand(brl, context);
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

    if (command != EOF) enqueueCommand(command);
  }

  return dequeueCommand();
}

KeyTableCommandContext
getCurrentCommandContext (void) {
  return currentCommandContext;
}

#ifdef ENABLE_LEARN_MODE
int
learnMode (BrailleDisplay *brl, int poll, int timeout) {
  const char *mode = "lrn";

  if (!setStatusText(brl, mode)) return 0;
  if (!message(mode, gettext("command learn mode"), MSG_NODELAY)) return 0;

  hasTimedOut(0);
  do {
    int command = readBrailleCommand(brl, KTB_CTX_DEFAULT);
    if (command != EOF) {
      logMessage(LOG_DEBUG, "Learn: command=%06X", command);
      if (BRL_DELAYED_COMMAND(command)) continue;

      {
        int cmd = command & BRL_MSK_CMD;
        if (cmd == BRL_CMD_NOOP) continue;
        if (cmd == BRL_CMD_LEARN) return 1;
      }

      {
        char buffer[0X100];
        describeCommand(command, buffer, sizeof(buffer),
                        CDO_IncludeName | CDO_IncludeOperand);
        logMessage(LOG_DEBUG, "Learn: %s", buffer);
        if (!message(mode, buffer, MSG_NODELAY|MSG_SILENT)) return 0;
      }

      hasTimedOut(0);
    }

    drainBrailleOutput(brl, poll);
  } while (!hasTimedOut(timeout));

  return message(mode, "done", 0);
}
#endif /* ENABLE_LEARN_MODE */

int
cellsHaveChanged (
  unsigned char *cells, const unsigned char *new, unsigned int count,
  unsigned int *from, unsigned int *to
) {
  if (memcmp(cells, new, count) == 0) return 0;

  {
    unsigned int first = 0;

    if (to) {
      while (count) {
        unsigned int last = count - 1;
        if (cells[last] != new[last]) break;
        count = last;
      }

      *to = count;
    }

    if (from) {
      while (first < count) {
        if (cells[first] != new[first]) break;
        first += 1;
      }

      *from = first;
    }

    memcpy(cells+first, new+first, count-first);
  }

  return 1;
}

const DotsTable dotsTable_ISO11548_1 = {
  BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4,
  BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8
};

void
makeTranslationTable (const DotsTable dots, TranslationTable table) {
  int byte;

  for (byte=0; byte<TRANSLATION_TABLE_SIZE; byte+=1) {
    unsigned char cell = 0;
    int dot;

    for (dot=0; dot<DOTS_TABLE_SIZE; dot+=1)
      if (byte & dotsTable_ISO11548_1[dot])
        cell |= dots[dot];

    table[byte] = cell;
  }
}

void
reverseTranslationTable (const TranslationTable from, TranslationTable to) {
  int byte;
  memset(to, 0, sizeof(TranslationTable));
  for (byte=TRANSLATION_TABLE_SIZE-1; byte>=0; byte--) to[from[byte]] = byte;
}

static inline void *
translateCells (
  const TranslationTable table,
  unsigned char *target, const unsigned char *source, size_t count
) {
  if (table) {
    while (count--) *target++ = table[*source++];
    return target;
  }

  if (target == source) return target + count;
  return mempcpy(target, source, count);
}

static inline unsigned char
translateCell (const TranslationTable table, unsigned char cell) {
  return table? table[cell]: cell;
}

static TranslationTable internalOutputTable;
static const unsigned char *outputTable;

void
setOutputTable (const TranslationTable table) {
  outputTable = table;
}

void
makeOutputTable (const DotsTable dots) {
  if (memcmp(dots, dotsTable_ISO11548_1, DOTS_TABLE_SIZE) == 0) {
    outputTable = NULL;
  } else {
    makeTranslationTable(dots, internalOutputTable);
    outputTable = internalOutputTable;
  }
}

void *
translateOutputCells (unsigned char *target, const unsigned char *source, size_t count) {
  return translateCells(outputTable, target, source, count);
}

unsigned char
translateOutputCell (unsigned char cell) {
  return translateCell(outputTable, cell);
}

static TranslationTable internalInputTable;
static const unsigned char *inputTable;

void
makeInputTable (void) {
  if (outputTable) {
    reverseTranslationTable(outputTable, internalInputTable);
    inputTable = internalInputTable;
  } else {
    inputTable = NULL;
  }
}

void *
translateInputCells (unsigned char *target, const unsigned char *source, size_t count) {
  return translateCells(inputTable, target, source, count);
}

unsigned char
translateInputCell (unsigned char cell) {
  return translateCell(inputTable, cell);
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
  if (brl->setFirmness) {
    logMessage(LOG_DEBUG, "setting braille firmness: %d", setting);
    if (brl->setFirmness(brl, setting)) return 1;
  }

  return 0;
}

int
setBrailleSensitivity (BrailleDisplay *brl, BrailleSensitivity setting) {
  if (brl->setSensitivity) {
    logMessage(LOG_DEBUG, "setting braille sensitivity: %d", setting);
    if (brl->setSensitivity(brl, setting)) return 1;
  }

  return 0;
}
