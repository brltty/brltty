/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "log.h"

#include "brl_driver.h"
#include "brldefs-tr.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 1000

struct BrailleDataStruct {
  struct {
    unsigned char refresh;
    unsigned char cells[TR_MAX_DATA_LENGTH];
  } text;
};

static int
writePacket (BrailleDisplay *brl, const unsigned char *packet, size_t size) {
  return writeBraillePacket(brl, NULL, packet, size);
}

static unsigned char
makeChecksum (const unsigned char *from, const unsigned char *to) {
  unsigned char checksum = 0;
  while (from < to) checksum ^= *from++;
  return checksum;
}

static int
writeCommand (BrailleDisplay *brl, unsigned char command, const unsigned char *data, unsigned char size) {
  unsigned char packet[TR_MAX_PACKET_SIZE];
  unsigned char *byte = packet;

  *byte++ = TR_PKT_START_OF_MESSAGE;
  unsigned char *length = byte++;
  unsigned char *start = byte;

  *byte++ = command;
  byte = mempcpy(byte, data, size);
  *length = byte - start;

  *byte = makeChecksum(start, byte);
  byte += 1;
  *byte++ = TR_PKT_END_OF_MESSAGE;

  return writePacket(brl, packet, (byte - packet));
}

static BraillePacketVerifierResult
verifyPacket (
  BrailleDisplay *brl,
  unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  unsigned char byte = bytes[size-1];

  switch (size) {
    case 1:
      switch (byte) {
        case TR_PKT_START_OF_MESSAGE:
          *length = 2;
          break;

        default:
          return BRL_PVR_INVALID;
      }
      break;

    case 2:
      *length += byte + 2;
      break;

    default:
      if (size == *length) {
        if (byte != TR_PKT_END_OF_MESSAGE) return BRL_PVR_INVALID;
      } else if (size == (*length - 1)) {
        const unsigned char *command = &bytes[2];
        const unsigned char *checksum = &bytes[size - 1];
        unsigned char actual = *checksum;
        unsigned char expected = makeChecksum(command, checksum);

        if (actual != expected) {
          logBytes(LOG_WARNING,
            "input packet checksum mismatch:"
            " Actual:%02X Expected:%02X",
            bytes, size,
            actual, expected
          );

          return BRL_PVR_INVALID;
        }
      }
      break;
  }

  return BRL_PVR_INCLUDE;
}

static size_t
readPacket (BrailleDisplay *brl, void *packet, size_t size) {
  return readBraillePacket(brl, NULL, packet, size, verifyPacket, NULL);
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(brl->data->text.cells, brl->buffer, brl->textColumns,
                       NULL, NULL, &brl->data->text.refresh)) {
    unsigned char cells[brl->textColumns];

    translateOutputCells(cells, brl->data->text.cells, brl->textColumns);
    if (!writeCommand(brl, TR_CMD_ACTUATE, cells, brl->textColumns)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  return EOF;
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 115200
  };

  BEGIN_USB_CHANNEL_DEFINITIONS
    { /* all models */
      .vendor=0X2E8A, .product=0X0005,
      .configuration=1, .interface=1, .alternative=0,
      .inputEndpoint=2, .outputEndpoint=2,
      .serial = &serialParameters,
      .resetDevice = 1
    },
  END_USB_CHANNEL_DEFINITIONS

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  descriptor.bluetooth.discoverChannel = 1;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) {
    return 1;
  }

  return 0;
}

static int
writeIdentifyRequest (BrailleDisplay *brl) {
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

    if (connectResource(brl, device)) {
      unsigned char response[TR_MAX_PACKET_SIZE];

      int probed = 1 || probeBrailleDisplay(
        brl, PROBE_RETRY_LIMIT, NULL, PROBE_INPUT_TIMEOUT,
        writeIdentifyRequest,
        readPacket, &response, sizeof(response),
        isIdentityResponse
      );

      if (probed) {
        makeOutputTable(dotsTable_ISO11548_1);
        brl->textColumns = 20;
        brl->data->text.refresh = 1;
        return 1;
      }

      disconnectBrailleResource(brl, NULL);
    }

    free(brl->data);
  } else {
    logMallocError();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  disconnectBrailleResource(brl, NULL);
  free(brl->data);
}
