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

#include "brl_driver.h"
#include "brldefs-xx.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 1000
#define MAXIMUM_RESPONSE_SIZE (0XFF + 4)
#define MAXIMUM_CELL_COUNT 140

BEGIN_KEY_NAME_TABLE(navigation)
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(navigation),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

struct BrailleDataStruct {
  GioEndpoint *gioEndpoint;
  int forceRewrite;
  unsigned char textCells[MAXIMUM_CELL_COUNT];
};

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, size_t count) {
  return writeBraillePacket(brl, brl->data->gioEndpoint, bytes, count);
}

static int
writePacket (BrailleDisplay *brl, const unsigned char *packet, size_t size) {
  unsigned char bytes[size];
  unsigned char *byte = bytes;

  byte = mempcpy(byte, packet, size);

  return writeBytes(brl, bytes, byte-bytes);
}

static size_t
readPacket (BrailleDisplay *brl, void *packet, size_t size) {
  unsigned char *bytes = packet;
  size_t offset = 0;
  size_t length = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;

      if (!gioReadByte(brl->data->gioEndpoint, &byte, started)) {
        if (started) logPartialPacket(bytes, offset);
        return 0;
      }
    }

  gotByte:
    if (offset == 0) {
      switch (byte) {
        default:
          logIgnoredByte(byte);
          continue;
      }
    } else {
      int unexpected = 0;

      if (unexpected) {
        logShortPacket(bytes, offset);
        offset = 0;
        length = 0;
        goto gotByte;
      }
    }

    if (offset < size) {
      bytes[offset] = byte;

      if (offset == (length - 1)) {
        logInputPacket(bytes, length);
        return length;
      }
    } else {
      if (offset == size) logTruncatedPacket(bytes, offset);
      logDiscardedByte(byte);
    }

    offset += 1;
  }
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS
  };

  static const UsbChannelDefinition usbChannelDefinitions[] = {
    { .vendor=0 }
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  if ((brl->data->gioEndpoint = gioConnectResource(identifier, &descriptor))) {
    return 1;
  }

  return 0;
}

static int
writeIdentityRequest (BrailleDisplay *brl) {
  static const unsigned char packet[] = {0};
  return writePacket(brl, packet, sizeof(packet));
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  return BRL_RSP_UNEXPECTED;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));
    brl->data->gioEndpoint = NULL;

    if (connectResource(brl, device)) {
      unsigned char response[MAXIMUM_RESPONSE_SIZE];

      if (probeBrailleDisplay(brl, PROBE_RETRY_LIMIT,
                              brl->data->gioEndpoint, PROBE_INPUT_TIMEOUT,
                              writeIdentityRequest,
                              readPacket, &response, sizeof(response),
                              isIdentityResponse)) {
        {
          const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

          brl->keyBindings = ktd->bindings;
          brl->keyNameTables = ktd->names;
        }

        makeOutputTable(dotsTable_ISO11548_1);
        brl->data->forceRewrite = 1;
        return 1;
      }

      gioDisconnectResource(brl->data->gioEndpoint);
    }

    free(brl->data);
  } else {
    logMallocError();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  if (brl->data) {
    if (brl->data->gioEndpoint) gioDisconnectResource(brl->data->gioEndpoint);
    free(brl->data);
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(brl->data->textCells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->forceRewrite)) {
    unsigned char cells[brl->textColumns];

    translateOutputCells(cells, brl->data->textCells, brl->textColumns);
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  unsigned char packet[MAXIMUM_RESPONSE_SIZE];
  size_t size;

  while ((size = readPacket(brl, packet, sizeof(packet)))) {
    logUnexpectedPacket(packet, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
