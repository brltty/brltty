/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2017 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include <string.h>
#include <errno.h>

#include "log.h"

#include "brl_driver.h"
#include "brldefs-do.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 500
#define MAXIMUM_RESPONSE_SIZE (0XFF + 4)
#define MAXIMUM_TEXT_CELLS 0XFF
#define MAX_CELLS_PER_PACKET 33

struct BrailleDataStruct {
  struct {
    unsigned char rewrite;
    unsigned char cells[MAXIMUM_TEXT_CELLS];
  } braille;

  struct {
    unsigned char rewrite;
    wchar_t characters[MAXIMUM_TEXT_CELLS];
  } text;

  struct {
    unsigned char rewrite;
    int position;
  } cursor;
};

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, size_t count) {
  return writeBraillePacket(brl, NULL, bytes, count);
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
  if (size < 2)
    return 0;

  unsigned char* buffer = packet;
  GioEndpoint *endpoint = brl->gioEndpoint;
  if (!gioReadByte(endpoint, &buffer[0], 0)) return 0;
  if (!gioReadByte(endpoint, &buffer[1], 1)) return 0;

  unsigned char packet_type = buffer[0];
  unsigned char count = buffer[1];

  if (count + 2 > size) return 0;

  for (unsigned char i = 0; i < count; i++) {
    if (!gioReadByte(endpoint, &buffer[i + 2], 1)) return 0;
  }

  logInputPacket(packet, count + 2);

  // Send an ACK, unless it's an ACK we just read
  if (packet_type != 0) {
    unsigned char ack[4];
    ack[0] = 0;  // GEN_ACK
    ack[1] = 2;  // data length
    ack[2] = packet_type;  // Ack the packet type
    ack[3] = 0;  // OK result code

    if (gioWriteData(endpoint, ack, sizeof(ack)) == -1) return 0;
  }

  return count + 2;
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 19200
  };

  BEGIN_USB_CHANNEL_DEFINITIONS
    { /* DOT braille tablet (66 cells) */
      .vendor=0X1366, .product=0X1015,
      .configuration=1, .interface=1, .alternative=0,
      .inputEndpoint=0x03, .outputEndpoint=0x04,
      .serial = &serialParameters
    },
  END_USB_CHANNEL_DEFINITIONS

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);
  descriptor.serial.parameters = &serialParameters;
  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) return 1;

  return 0;
}

static int
writeInitRequest (BrailleDisplay *brl) {
  static const unsigned char packet[] = {0x01, 0x03, 0x03, 0x01, 0x01};
  return writePacket(brl, packet, sizeof(packet));
}

static BrailleResponseResult
isInitResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const unsigned char* data = packet;
  if (size != 9) return BRL_RSP_FAIL;
  if (data[0] != 0x02) return BRL_RSP_FAIL;
  return BRL_RSP_DONE;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    if (connectResource(brl, device)) {
      unsigned char response[MAXIMUM_RESPONSE_SIZE];
      if (probeBrailleDisplay(brl, PROBE_RETRY_LIMIT, NULL, PROBE_INPUT_TIMEOUT,
                              writeInitRequest,
                              readPacket, &response, sizeof(response),
                              isInitResponse)) {
        brl->textColumns = response[5];
        brl->textRows = response[6];

        brl->statusRows = 0;
        brl->statusColumns = 0;

        brl->data->braille.rewrite = 1;
        brl->data->text.rewrite = 1;
        brl->data->cursor.rewrite = 1;

        if (gioAwaitInput(brl->gioEndpoint, PROBE_INPUT_TIMEOUT)) {
          unsigned char cap[MAXIMUM_RESPONSE_SIZE];
          readPacket(brl, cap, sizeof(cap));
          // TODO: interpret capabilities.
        }

        // Set all dots down
        {
          unsigned char packet[3];
          packet[0] = 0x26;  // SYS_SET_ALL_DOTS_DN
          packet[1] = 1;
          packet[2] = 0;
          writePacket(brl, packet, sizeof(packet));

          if (gioAwaitInput(brl->gioEndpoint, PROBE_INPUT_TIMEOUT)) {
            unsigned char ack[4];
            readPacket(brl, ack, sizeof(ack));
          }
        }

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

  if (brl->data) {
    free(brl->data);
    brl->data = NULL;
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  size_t cellCount = brl->textColumns * brl->textRows;

  int newBraille =
      cellsHaveChanged(brl->data->braille.cells, brl->buffer, cellCount,
                       NULL, NULL, &brl->data->braille.rewrite);

  int newText =
      textHasChanged(brl->data->text.characters, text, cellCount,
                     NULL, NULL, &brl->data->text.rewrite);

  int newCursor =
      cursorHasChanged(&brl->data->cursor.position, brl->cursor,
                       &brl->data->cursor.rewrite);

  if (!newBraille && !newText && !newCursor)
    return 1;

  unsigned char cells[cellCount];
  translateOutputCells(cells, brl->data->braille.cells, cellCount);

  // Send multiple packets because the device has trouble updating
  // too many cells at the same time.
  size_t startCell = 0;
  while (startCell < cellCount) {
    size_t packetCellCount = MIN(cellCount - startCell, MAX_CELLS_PER_PACKET);
    unsigned char packet[packetCellCount + 3];
    packet[0] = 0x82;  // USB_SET_BRAILLE_CODE
    packet[1] = packetCellCount + 1;
    packet[2] = startCell;

    for (int j = 0; j < packetCellCount; j++)
      packet[j + 3] = cells[startCell + j] & 0x3F;

    if (!writePacket(brl, packet, sizeof(packet))) return 0;

    if (!gioAwaitInput(brl->gioEndpoint, PROBE_INPUT_TIMEOUT)) {
      // We sometimes don't get an ACK, don't treat it as a fatal error.
      startCell += packetCellCount;
      continue;
    }

    // If we do get something make sure it's a valid ACK.
    unsigned char ack[4];
    size_t ack_result = readPacket(brl, ack, sizeof(ack));
    if (!ack_result) {
      fprintf(stderr, "ack_result is 0\n");
      return 0;
    }
    if (ack[2] != 0x82 || ack[3] != 0x00) return 0;

    startCell += packetCellCount;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  // The prototype board from DOT doesn't have any input keys yet.
  // This driver currently only supports braille output.
  return EOF;
}
