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

#include <string.h>
#include <errno.h>

#include "log.h"
#include "prefs.h"
#include "ktb.h"
#include "async_wait.h"
#include "api_control.h"
#include "drivers.h"
#include "driver.h"
#include "brl.h"
#include "brl.auto.h"

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

const DotsTable dotsTable_ISO11548_1 = {
  BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4,
  BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8
};

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

int
connectBrailleResource (
  BrailleDisplay *brl,
  const char *identifier,
  const GioDescriptor *descriptor
) {
  GioEndpoint *endpoint = gioConnectResource(identifier, descriptor);
  if (!endpoint) return 0;

  brl->gioEndpoint = endpoint;
  return 1;
}

void
disconnectBrailleResource (
  BrailleDisplay *brl,
  BrailleSessionEnder *endSession
) {
  if (brl->gioEndpoint) {
    if (endSession) endSession(brl);
    gioDisconnectResource(brl->gioEndpoint);
    brl->gioEndpoint = NULL;
  }
}

unsigned int
drainBrailleOutput (BrailleDisplay *brl, int minimumDelay) {
  int duration = brl->writeDelay + 1;
  if (duration < minimumDelay) duration = minimumDelay;
  brl->writeDelay = 0;
  asyncWait(duration);
  return duration;
}

size_t
readBraillePacket (
  BrailleDisplay *brl,
  GioEndpoint *endpoint,
  void *packet, size_t size,
  BraillePacketVerifier *verifyPacket, void *data
) {
  unsigned char *bytes = packet;
  size_t count = 0;
  size_t length = 1;

  if (!endpoint) endpoint = brl->gioEndpoint;

  while (1) {
    unsigned char byte;

    {
      int started = count > 0;

      if (!gioReadByte(endpoint, &byte, started)) {
        if (started) logPartialPacket(bytes, count);
        return 0;
      }
    }

  gotByte:
    if (count < size) {
      bytes[count++] = byte;

      if (!verifyPacket(brl, bytes, count, &length, data)) {
        if (--count) {
          logShortPacket(bytes, count);
          count = 0;
          length = 1;
          goto gotByte;
        }

        logIgnoredByte(byte);
        continue;
      }

      if (count == length) {
        logInputPacket(bytes, length);
        return length;
      }
    } else {
      if (count++ == size) logTruncatedPacket(bytes, size);
      logDiscardedByte(byte);
    }
  }
}

int
writeBraillePacket (
  BrailleDisplay *brl,
  GioEndpoint *endpoint,
  const void *packet, size_t size
) {
  if (!endpoint) endpoint = brl->gioEndpoint;
  logOutputPacket(packet, size);
  if (gioWriteData(endpoint, packet, size) == -1) return 0;

  if (endpoint == brl->gioEndpoint) {
    brl->writeDelay += gioGetMillisecondsToTransfer(endpoint, size);
  }

  return 1;
}

int
probeBrailleDisplay (
  BrailleDisplay *brl, unsigned int retryLimit,
  GioEndpoint *endpoint, int inputTimeout,
  BrailleRequestWriter *writeRequest,
  BraillePacketReader *readPacket, void *responsePacket, size_t responseSize,
  BrailleResponseHandler *handleResponse
) {
  unsigned int retryCount = 0;

  if (!endpoint) endpoint = brl->gioEndpoint;

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

void
makeTranslationTable (const DotsTable dots, TranslationTable table) {
  int byte;

  for (byte=0; byte<TRANSLATION_TABLE_SIZE; byte+=1) {
    unsigned char cell = 0;
    int dot;

    for (dot=0; dot<DOTS_TABLE_SIZE; dot+=1) {
      if (byte & dotsTable_ISO11548_1[dot]) {
        cell |= dots[dot];
      }
    }

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

int
enqueueKeyEvent (
  BrailleDisplay *brl,
  unsigned char set, unsigned char key, int press
) {
#ifdef ENABLE_API
  if (apiStarted) {
    if (api_handleKeyEvent(set, key, press)) {
      return 1;
    }
  }
#endif /* ENABLE_API */

  if (brl->keyTable) {
    switch (prefs.brailleOrientation) {
      case BRL_ORIENTATION_ROTATED:
        if (brl->rotateKey) brl->rotateKey(brl, &set, &key);
        break;

      default:
      case BRL_ORIENTATION_NORMAL:
        break;
    }

    processKeyEvent(brl->keyTable, getCurrentCommandContext(), set, key, press);
    return 1;
  }

  return 0;
}

int
enqueueKey (
  BrailleDisplay *brl,
  unsigned char set, unsigned char key
) {
  if (enqueueKeyEvent(brl, set, key, 1))
    if (enqueueKeyEvent(brl, set, key, 0))
      return 1;

  return 0;
}

int
enqueueKeys (
  BrailleDisplay *brl,
  uint32_t bits, unsigned char set, unsigned char key
) {
  unsigned char stack[0X20];
  unsigned char count = 0;

  while (bits) {
    if (bits & 0X1) {
      if (!enqueueKeyEvent(brl, set, key, 1)) return 0;
      stack[count++] = key;
    }

    bits >>= 1;
    key += 1;
  }

  while (count)
    if (!enqueueKeyEvent(brl, set, stack[--count], 0))
      return 0;

  return 1;
}

int
enqueueUpdatedKeys (
  BrailleDisplay *brl,
  uint32_t new, uint32_t *old, unsigned char set, unsigned char key
) {
  uint32_t bit = 0X1;
  unsigned char stack[0X20];
  unsigned char count = 0;

  while (*old != new) {
    if ((new & bit) && !(*old & bit)) {
      stack[count++] = key;
      *old |= bit;
    } else if (!(new & bit) && (*old & bit)) {
      if (!enqueueKeyEvent(brl, set, key, 0)) return 0;
      *old &= ~bit;
    }

    key += 1;
    bit <<= 1;
  }

  while (count)
    if (!enqueueKeyEvent(brl, set, stack[--count], 1))
      return 0;

  return 1;
}

int
enqueueXtScanCode (
  BrailleDisplay *brl,
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

  return enqueueKey(brl, set, key);
}
