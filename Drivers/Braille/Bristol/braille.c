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

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "log.h"

#include "brl_driver.h"
#include "brldefs-br.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 250
#define MAXIMUM_RESPONSE_SIZE (0XFF + 4)
#define MAXIMUM_TEXT_CELLS 360

struct BrailleDataStruct {
  struct {
    unsigned char rewrite;
    unsigned char cells[MAXIMUM_TEXT_CELLS];
    unsigned char previousCells[MAXIMUM_TEXT_CELLS];
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
  unsigned char* buffer = packet;
  GioEndpoint *endpoint = brl->gioEndpoint;
  size_t result = 0;
  while (gioReadByte(endpoint, &buffer[result], 0) && result < size)
    result++;
  return result;
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 115200
  };

  BEGIN_USB_CHANNEL_DEFINITIONS
    { /* Canute (360 cells) */
      .vendor=0X2341, .product=0X8036,
      .configuration=1, .interface=1, .alternative=0,
      .inputEndpoint=0x83, .outputEndpoint=0x02,
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
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    if (connectResource(brl, device)) {
      // Get n characters per row
      {
        unsigned char packet[1];
        packet[0] = 0x00;  // COMMAND_N_CHARACTERS
        if (!writePacket(brl, packet, sizeof(packet))) return 0;

        if (gioAwaitInput(brl->gioEndpoint, PROBE_INPUT_TIMEOUT)) {
          unsigned char response[MAXIMUM_RESPONSE_SIZE];
          size_t result = readPacket(brl, response, sizeof(response));
          if (!result) return 0;
          brl->textColumns = response[1];
        } else {
          disconnectBrailleResource(brl, NULL);
          return 0;
        }
      }

      // Get n rows
      {
        unsigned char packet[1];
        packet[0] = 0x01;  // COMMAND_N_ROWS
        int result = writePacket(brl, packet, sizeof(packet));
        if (!result) return 0;
        if (gioAwaitInput(brl->gioEndpoint, PROBE_INPUT_TIMEOUT)) {
          unsigned char response[MAXIMUM_RESPONSE_SIZE];
          size_t result = readPacket(brl, response, sizeof(response));
          brl->textRows = response[1];
        } else {
          disconnectBrailleResource(brl, NULL);
          return 0;
        }
      }

      brl->statusRows = 0;
      brl->statusColumns = 0;

      brl->data->braille.rewrite = 1;
      brl->data->text.rewrite = 1;
      brl->data->cursor.rewrite = 1;

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
  for (size_t row = 0; row < brl->textRows; row++) {
    // Send 1 packet per row, only if that row has changed.
    size_t index = row * brl->textColumns;
    if (!cellsHaveChanged(&brl->data->braille.previousCells[index], &cells[index], brl->textColumns, NULL, NULL, NULL))
      continue;

    memcpy(&brl->data->braille.previousCells[index], &cells[index], brl->textColumns);

    unsigned char packet[brl->textColumns + 2];
    packet[0] = 0x06;  // COMMAND_SEND_LINE
    packet[1] = row;
    for (size_t j = 0; j < brl->textColumns; j++)
      packet[2 + j] = cells[index + j];
    int result = writePacket(brl, packet, sizeof(packet));

    if (gioAwaitInput(brl->gioEndpoint, PROBE_INPUT_TIMEOUT)) {
      unsigned char ack[2];
      size_t ack_result = readPacket(brl, ack, sizeof(ack));
      if (ack_result != 2)
        return 0;
    }
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  // TODO: handle input
  return EOF;
}
