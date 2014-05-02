/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include "brl_base.h"
#include "brl_utils.h"
#include "brl_dots.h"
#include "prefs.h"
#include "io_generic.h"
#include "cmd_queue.h"
#include "ktb.h"
#include "api_control.h"

const DotsTable dotsTable_ISO11548_1 = {
  BRL_DOT_1, BRL_DOT_2, BRL_DOT_3, BRL_DOT_4,
  BRL_DOT_5, BRL_DOT_6, BRL_DOT_7, BRL_DOT_8
};

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
          BRL_DOT_8, BRL_DOT_6, BRL_DOT_5, BRL_DOT_7,
          BRL_DOT_3, BRL_DOT_2, BRL_DOT_4, BRL_DOT_1
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

int
connectBrailleResource (
  BrailleDisplay *brl,
  const char *identifier,
  const GioDescriptor *descriptor,
  BrailleSessionInitializer *initializeSession
) {
  if ((brl->gioEndpoint = gioConnectResource(identifier, descriptor))) {
    if (!initializeSession || initializeSession(brl)) {
      if (gioDiscardInput(brl->gioEndpoint)) {
        return 1;
      }
    }

    gioDisconnectResource(brl->gioEndpoint);
    brl->gioEndpoint = NULL;
  }

  return 0;
}

void
disconnectBrailleResource (
  BrailleDisplay *brl,
  BrailleSessionEnder *endSession
) {
  if (brl->gioEndpoint) {
    if (endSession) endSession(brl);
    drainBrailleOutput(brl, 0);
    gioDisconnectResource(brl->gioEndpoint);
    brl->gioEndpoint = NULL;
  }
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

      {
        BraillePacketVerifierResult result = verifyPacket(brl, bytes, count, &length, data);

        switch (result) {
          case BRL_PVR_EXCLUDE:
            count -= 1;
          case BRL_PVR_INCLUDE:
            break;

          default:
            logMessage(LOG_WARNING, "unimplemented braille packet verifier result: %u", result);
          case BRL_PVR_INVALID:
            if (--count) {
              logShortPacket(bytes, count);
              count = 0;
              length = 1;
              goto gotByte;
            }

            logIgnoredByte(byte);
            continue;
        }
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

    if (errno != EAGAIN)
#ifdef ETIMEDOUT
      if (errno != ETIMEDOUT)
#endif /* ETIMEDOUT */
        break;

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

typedef struct {
  const unsigned char set;
  KeyValueSet keys;
} IncludeKeyValueData;

static int
includeKeyValue (const KeyNameEntry *kne, void *data) {
  if (kne) {
    IncludeKeyValueData *ikv = data;

    if (kne->value.set == ikv->set) ikv->keys |= KEY_VALUE_BIT(kne->value.key);
  }

  return 1;
}

KeyValueSet
makeKeyValueSet (KEY_NAME_TABLES_REFERENCE keys, unsigned char set) {
  IncludeKeyValueData ikv = {
    .set = set,

    .keys = 0
  };

  forEachKeyName(keys, includeKeyValue, &ikv);
  return ikv.keys;
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
enqueueKeyEvents (
  BrailleDisplay *brl,
  KeyValueSet bits, unsigned char set, unsigned char key, int press
) {
  while (bits) {
    if (bits & 0X1) {
      if (!enqueueKeyEvent(brl, set, key, press)) {
        return 0;
      }
    }

    bits >>= 1;
    key += 1;
  }

  return 1;
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
  KeyValueSet bits, unsigned char set, unsigned char key
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
  KeyValueSet new, KeyValueSet *old, unsigned char set, unsigned char key
) {
  KeyValueSet bit = KEY_VALUE_BIT(0);
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

  while (count) {
    if (!enqueueKeyEvent(brl, set, stack[--count], 1)) {
      return 0;
    }
  }

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
