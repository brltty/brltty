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
#include "async.h"
#include "message.h"
#include "charset.h"
#include "unicode.h"
#include "drivers.h"
#include "io_generic.h"
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
brl_construct (BrailleDisplay *brl UNUSED, char **parameters UNUSED, const char *device UNUSED) {
  brl->keyBindings = NULL;
  return 1;
}

static void
brl_destruct (BrailleDisplay *brl UNUSED) {
}

static int
brl_readCommand (BrailleDisplay *brl UNUSED, KeyTableCommandContext context UNUSED) {
  return EOF;
}

static int
brl_writeWindow (BrailleDisplay *brl UNUSED, const wchar_t *characters UNUSED) {
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
  brl->textColumns = 0;
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
  logMessage(infoLevel, "Braille Display Dimensions: %d %s, %d %s",
             brl->textRows, (brl->textRows == 1)? "row": "rows",
             brl->textColumns, (brl->textColumns == 1)? "column": "columns");

  memset(brl->buffer, 0, brl->textColumns*brl->textRows);
  if (brl->bufferResized) brl->bufferResized(brl->textRows, brl->textColumns);
}

static int
resizeBrailleBuffer (BrailleDisplay *brl, int resized, int infoLevel) {
  if ((brl->noDisplay = !brl->textColumns)) brl->textColumns = 1;

  if (brl->resizeRequired) {
    brl->resizeRequired = 0;
    resized = 1;

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
  }

  if (resized) brailleBufferResized(brl, infoLevel);
  return 1;
}

int
ensureBrailleBuffer (BrailleDisplay *brl, int infoLevel) {
  brl->resizeRequired = brl->isCoreBuffer = !brl->buffer;
  return resizeBrailleBuffer(brl, 1, infoLevel);
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
} KeyEvent;

static const int keyReleaseTimeout = 0;
static TimePeriod keyReleasePeriod;
static KeyEvent *keyReleaseEvent = NULL;

static Queue *
getKeyEventQueue (int create) {
  static Queue *keyEventQueue = NULL;

  if (create && !keyEventQueue) {
    keyEventQueue = newQueue(NULL, NULL);
  }

  return keyEventQueue;
}

static int
addKeyEvent (KeyEvent *event) {
  Queue *queue = getKeyEventQueue(1);

  if (queue)
    if (enqueueItem(queue, event))
      return 1;

  return 0;
}

int
enqueueKeyEvent (unsigned char set, unsigned char key, int press) {
  if (keyReleaseEvent) {
    if (press && (set == keyReleaseEvent->set) && (key == keyReleaseEvent->key)) {
      if (!afterTimePeriod(&keyReleasePeriod, NULL)) {
        free(keyReleaseEvent);
        keyReleaseEvent = NULL;
        return 1;
      }
    }

    {
      KeyEvent *event = keyReleaseEvent;
      keyReleaseEvent = NULL;

      if (!addKeyEvent(event)) {
        free(event);
        return 0;
      }
    }
  }

  {
    KeyEvent *event;

    if ((event = malloc(sizeof(*event)))) {
      event->set = set;
      event->key = key;
      event->press = press;

      if (keyReleaseTimeout && !press) {
        keyReleaseEvent = event;
        startTimePeriod(&keyReleasePeriod, keyReleaseTimeout);
        return 1;
      }

      if (addKeyEvent(event)) return 1;
      free(event);
    } else {
      logMallocError();
    }
  }

  return 0;
}

static int
dequeueKeyEvent (unsigned char *set, unsigned char *key, int *press) {
  Queue *queue = getKeyEventQueue(0);

  if (keyReleaseEvent) {
    if (afterTimePeriod(&keyReleasePeriod, NULL)) {
      if (!addKeyEvent(keyReleaseEvent)) return 0;
      keyReleaseEvent = NULL;
    }
  }

  if (queue) {
    KeyEvent *event;

    while ((event = dequeueItem(queue))) {
#ifdef ENABLE_API
      if (apiStarted) {
        if ((api_handleKeyEvent(event->set, event->key, event->press)) == EOF) {
          free(event);
	  continue;
	}
      }
#endif /* ENABLE_API */

      *set = event->set;
      *key = event->key;
      *press = event->press;
      free(event);
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
  unsigned char set;

  switch (escape) {
    case 0X00: set = set00; break;
    case 0XE0: set = setE0; break;
    case 0XE1: set = setE1; break;

    default:
      logMessage(LOG_WARNING, "unsupported XT scan code: %02X %02X", escape, key);
      return 0;
  }

  return enqueueKey(set, key);
}

static KeyTableCommandContext currentCommandContext = KTB_CTX_DEFAULT;

KeyTableCommandContext
getCurrentCommandContext (void) {
  return currentCommandContext;
}

int
readBrailleCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  currentCommandContext = context;

  {
    int command = dequeueCommand();
    if (command != EOF) return command;
  }

  {
    int command = braille->readCommand(brl, context);
    resizeBrailleBuffer(brl, 0, LOG_INFO);

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

  {
    int command = dequeueCommand();

#ifdef ENABLE_API
    if (command == EOF) command = api_handleCommand(command);
#endif /* ENABLE_API */

    return command;
  }
}

int
writeBraillePacket (
  BrailleDisplay *brl, GioEndpoint *endpoint,
  const void *packet, size_t size
) {
  logOutputPacket(packet, size);
  if (gioWriteData(endpoint, packet, size) == -1) return 0;
  brl->writeDelay += gioGetMillisecondsToTransfer(endpoint, size);
  return 1;
}

int
probeBrailleDisplay (
  BrailleDisplay *brl, unsigned int retryLimit,
  GioEndpoint *endpoint, int inputTimeout,
  BrailleRequestWriter writeRequest,
  BraillePacketReader readPacket, void *responsePacket, size_t responseSize,
  BrailleResponseHandler *handleResponse
) {
  unsigned int retryCount = 0;

  while (writeRequest(brl)) {
    drainBrailleOutput(brl, 0);

    while (gioAwaitInput(endpoint, inputTimeout)) {
      size_t size = readPacket(brl, responsePacket, responseSize);
      if (!size) break;

      {
        BrailleResponseResult result = handleResponse(brl, responsePacket, size);

        switch (result) {
          case BRL_RSP_DONE:
            return 1;

          case BRL_RSP_UNEXPECTED:
            logUnexpectedPacket(responsePacket, size);
          case BRL_RSP_CONTINUE:
            break;

          default:
            logMessage(LOG_WARNING, "unimplemented braille response result: %u", result);
          case BRL_RSP_FAIL:
            return 0;
        }
      }
    }

    if (errno != EAGAIN) break;
    if (retryCount == retryLimit) break;
    retryCount += 1;
  }

  return 0;
}

#ifdef ENABLE_LEARN_MODE
int
learnMode (BrailleDisplay *brl, int poll, int timeout) {
  const char *mode = "lrn";
  TimePeriod period;

  if (!setStatusText(brl, mode)) return 0;
  if (!message(mode, gettext("Command Learn Mode"), MSG_NODELAY)) return 0;

  startTimePeriod(&period, timeout);
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
        if (!message(mode, buffer, MSG_NODELAY)) return 0;
      }

      restartTimePeriod(&period);
    }

    drainBrailleOutput(brl, poll);
  } while (!afterTimePeriod(&period, NULL));

  return message(mode, gettext("done"), 0);
}
#endif /* ENABLE_LEARN_MODE */

int
cellsHaveChanged (
  unsigned char *cells, const unsigned char *new, unsigned int count,
  unsigned int *from, unsigned int *to, int *force
) {
  unsigned int first = 0;

  if (force && *force) {
    *force = 0;
  } else if (memcmp(cells, new, count) != 0) {
    if (to) {
      while (count) {
        unsigned int last = count - 1;
        if (cells[last] != new[last]) break;
        count = last;
      }
    }

    if (from) {
      while (first < count) {
        if (cells[first] != new[first]) break;
        first += 1;
      }
    }
  } else {
    return 0;
  }

  if (from) *from = first;
  if (to) *to = count;

  memcpy(cells+first, new+first, count-first);
  return 1;
}

int
textHasChanged (
  wchar_t *text, const wchar_t *new, unsigned int count,
  unsigned int *from, unsigned int *to, int *force
) {
  unsigned int first = 0;

  if (force && *force) {
    *force = 0;
  } else if (wmemcmp(text, new, count) != 0) {
    if (to) {
      while (count) {
        unsigned int last = count - 1;
        if (text[last] != new[last]) break;
        count = last;
      }
    }

    if (from) {
      while (first < count) {
        if (text[first] != new[first]) break;
        first += 1;
      }
    }
  } else {
    return 0;
  }

  if (from) *from = first;
  if (to) *to = count;

  wmemcpy(text+first, new+first, count-first);
  return 1;
}

int
cursorHasChanged (int *cursor, int new, int *force) {
  if (force && *force) {
    *force = 0;
  } else if (new == *cursor) {
    return 0;
  }

  *cursor = new;
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
