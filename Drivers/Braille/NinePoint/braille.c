/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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
#include "io_generic.h"

#include "brl_driver.h"
#include "brldefs-np.h"

#define MAXIMUM_RESPONSE_SIZE 3
#define MAXIMUM_CELL_COUNT (NP_KEY_ROUTING_MAX - NP_KEY_ROUTING_MIN + 1)

BEGIN_KEY_NAME_TABLE(navigation)
  KEY_SET_ENTRY(NP_SET_RoutingKeys, "RoutingKey"),
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
        case 0XFD:
          length = 2;
          break;

        default:
          if ((byte >= NP_KEY_ROUTING_MIN) && (byte <= NP_KEY_ROUTING_MAX)) {
            length = 1;
          } else {
            logIgnoredByte(byte);
            continue;
          }
          break;
      }
    } else {
      int unexpected = 0;

      if (offset == 1) {
        switch (byte) {
          case 0X2F:
            length = 3;
            break;

          default:
            unexpected = 1;
            break;
        }
      }

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
  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.bluetooth.channelNumber = 1;

  if ((brl->data->gioEndpoint = gioConnectResource(identifier, &descriptor))) {
    return 1;
  }

  return 0;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));
    brl->data->gioEndpoint = NULL;

    if (connectResource(brl, device)) {
      {
        const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

        brl->keyBindings = ktd->bindings;
        brl->keyNameTables = ktd->names;
      }

      makeOutputTable(dotsTable_ISO11548_1);
      brl->data->forceRewrite = 1;
      brl->textColumns = 8;
      return 1;
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
    unsigned char bytes[(brl->textColumns * 2) + 2];
    unsigned char *byte = bytes;

    {
      unsigned int i;

      for (i=0; i<brl->textColumns; i+=1) {
        *byte++ = 0XFC;
        *byte++ = translateOutputCell(brl->data->textCells[i]);
      }
    }

    *byte++ = 0XFD;
    *byte++ = 0X10;

    if (!writeBytes(brl, bytes, byte-bytes)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  unsigned char packet[MAXIMUM_RESPONSE_SIZE];
  size_t size;

  while ((size = readPacket(brl, packet, sizeof(packet)))) {
    switch (packet[0]) {
      case 0XFD:
        switch (packet[1]) {
          case 0X2F:
            continue;

          default:
            break;
        }
        break;

      default:
        if ((packet[0] >= NP_KEY_ROUTING_MIN) && (packet[0] <= NP_KEY_ROUTING_MAX)) {
          enqueueKey(NP_SET_RoutingKeys, (packet[0] - NP_KEY_ROUTING_MIN));
          continue;
        }
        break;
    }

    logUnexpectedPacket(packet, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
