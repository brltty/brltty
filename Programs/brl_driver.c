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

#include <errno.h>

#include "log.h"
#include "async_wait.h"
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
