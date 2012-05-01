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

BEGIN_KEY_NAME_TABLES(all)
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

typedef struct {
  unsigned char modelIdentifier;
  unsigned char cellCount;
} ModelEntry;

static const ModelEntry modelTable[] = {
  { .modelIdentifier = 0X70,
    .cellCount = 0
  },

  { .modelIdentifier = 0X72,
    .cellCount = 20
  },

  { .modelIdentifier = 0X74,
    .cellCount = 40
  },

  { .modelIdentifier = 0X76,
    .cellCount = 60
  },

  { .modelIdentifier = 0X78,
    .cellCount = 80
  },

  { .modelIdentifier = 0X7A,
    .cellCount = 100
  },

  { .modelIdentifier = 0X7C,
    .cellCount = 120
  },

  { .modelIdentifier = 0X7E,
    .cellCount = 140
  },

  { .modelIdentifier = 0 }
};

struct BrailleDataStruct {
  GioEndpoint *gioEndpoint;
  const ModelEntry *model;
  int forceRewrite;
  int acknowledgementPending;
  unsigned char textCells[140];
};

static const ModelEntry *
getModelEntry (unsigned char identifier) {
  const ModelEntry *entry = modelTable;

  while (entry->modelIdentifier) {
    if (identifier == entry->modelIdentifier) return entry;
    entry += 1;
  }

  logMessage(LOG_WARNING, "unknown NinePoint model: 0X%02X", identifier);
  return NULL;
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, size_t count) {
  return gioWriteData(brl->data->gioEndpoint, bytes, count) != -1;
}

static int
writePacket (BrailleDisplay *brl, unsigned char type, size_t size, const unsigned char *data) {
  unsigned char bytes[size + 5];
  unsigned char *byte = bytes;

  *byte++ = NP_PKT_BEGIN;
  *byte++ = brl->data->model->modelIdentifier;
  *byte++ = size + 1;
  *byte++ = type;
  byte = mempcpy(byte, data, size);
  *byte++ = NP_PKT_END;

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
        case NP_RSP_Identity:
          length = 2;
          break;

        case NP_PKT_BEGIN:
          length = 3;
          break;

        default:
          logIgnoredByte(byte);
          continue;
      }
    } else {
      int unexpected = 0;

      switch (bytes[0]) {
        case NP_PKT_BEGIN:
          if (offset == 1) {
            if (byte != brl->data->model->modelIdentifier) unexpected = 1;
          } else if (offset == 2) {
            length += byte + 1;
          } else if (offset == (length-1)) {
            if (byte != NP_PKT_END) unexpected = 1;
          }
          break;

        default:
          break;
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
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 19200,
    .parity = SERIAL_PARITY_ODD
  };

  static const UsbChannelDefinition usbChannelDefinitions[] = {
    { .vendor=0X0403, .product=0X6001, 
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .serial = &serialParameters
    }
    ,
    { .vendor=0 }
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  if ((brl->data->gioEndpoint = gioConnectResource(identifier, &descriptor))) {
    return 1;
  }

  return 0;
}

static int
writeIdentityRequest (BrailleDisplay *brl) {
  static const unsigned char bytes[] = {NP_REQ_Identify};
  return writeBytes(brl, bytes, sizeof(bytes));
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const unsigned char *bytes = packet;

  return (bytes[0] == NP_RSP_Identity)? BRL_RSP_DONE: BRL_RSP_UNEXPECTED;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));
    brl->data->gioEndpoint = NULL;

    if (connectResource(brl, device)) {
      unsigned char response[0X200];

      if (probeBrailleDisplay(brl, 2, brl->data->gioEndpoint, 1000,
                              writeIdentityRequest,
                              readPacket, &response, sizeof(response),
                              isIdentityResponse)) {
        if ((brl->data->model = getModelEntry(response[1]))) {
          brl->textColumns = brl->data->model->cellCount;

          {
            const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

            brl->keyBindings = ktd->bindings;
            brl->keyNameTables = ktd->names;
          }

          makeOutputTable(dotsTable_ISO11548_1);
          brl->data->forceRewrite = 1;
          brl->data->acknowledgementPending = 0;
          return 1;
        }
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
  if (!brl->data->acknowledgementPending) {
    if (cellsHaveChanged(brl->data->textCells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->forceRewrite)) {
      unsigned char cells[brl->textColumns];

      translateOutputCells(cells, brl->data->textCells, brl->textColumns);
      if (!writePacket(brl, NP_PKT_REQ_Write, brl->textColumns, cells)) return 0;
      brl->data->acknowledgementPending = 1;
    }
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  unsigned char packet[0X200];
  size_t size;

  while ((size = readPacket(brl, packet, sizeof(packet)))) {
    switch (packet[0]) {
      case NP_PKT_BEGIN: {
        const unsigned char *bytes = &packet[4];
        size_t count = packet[2] - 1;

        switch (packet[3]) {
          case NP_PKT_RSP_Confirm:
            if (count > 0) {
              switch (bytes[0]) {
                case 0X7D:
                  brl->data->forceRewrite = 1;
                case 0X7E:
                  brl->data->acknowledgementPending = 0;
                  continue;

                default:
                  break;
              }
            }
            break;

          default:
            break;
        }

        break;
      }

      default:
        break;
    }

    logUnexpectedPacket(packet, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
  return EOF;
}
