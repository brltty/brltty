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
#include "brldefs-mm.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 1000
#define MAXIMUM_RESPONSE_SIZE (0XFF + 4)
#define MAXIMUM_CELL_COUNT 140

#define MM_KEY_SET_ENTRY(s,n) BRL_KEY_SET_ENTRY(MM, s, n)
#define MM_KEY_NAME_ENTRY(s,k,n) BRL_KEY_NAME_ENTRY(MM, s, k, n)

#define MM_SHIFT_KEY_ENTRY(k,n) MM_KEY_NAME_ENTRY(SHIFT, k, n)
#define MM_DOT_KEY_ENTRY(k) MM_KEY_NAME_ENTRY(DOT, k, "dot" #k)
#define MM_EDIT_KEY_ENTRY(k,n) MM_KEY_NAME_ENTRY(EDIT, k, n)
#define MM_ARROW_KEY_ENTRY(k,n) MM_KEY_NAME_ENTRY(ARROW, k, n)
#define MM_DISPLAY_KEY_ENTRY(k,n) MM_KEY_NAME_ENTRY(DISPLAY, k, n)

BEGIN_KEY_NAME_TABLE(shift)
  MM_SHIFT_KEY_ENTRY(F1, "F1"),
  MM_SHIFT_KEY_ENTRY(F2, "F2"),
  MM_SHIFT_KEY_ENTRY(F3, "F3"),
  MM_SHIFT_KEY_ENTRY(F4, "F4"),

  MM_SHIFT_KEY_ENTRY(CONTROL, "Control"),
  MM_SHIFT_KEY_ENTRY(ALT, "Alt"),
  MM_SHIFT_KEY_ENTRY(SELECT, "Select"),
  MM_SHIFT_KEY_ENTRY(READ, "Read"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(dot)
  MM_DOT_KEY_ENTRY(1),
  MM_DOT_KEY_ENTRY(2),
  MM_DOT_KEY_ENTRY(3),
  MM_DOT_KEY_ENTRY(4),
  MM_DOT_KEY_ENTRY(5),
  MM_DOT_KEY_ENTRY(6),
  MM_DOT_KEY_ENTRY(7),
  MM_DOT_KEY_ENTRY(8),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(edit)
  MM_EDIT_KEY_ENTRY(ESCAPE, "Escape"),
  MM_EDIT_KEY_ENTRY(INFO, "Info"),

  MM_EDIT_KEY_ENTRY(BACKSPACE, "Backspace"),
  MM_EDIT_KEY_ENTRY(DELETE, "Delete"),
  MM_EDIT_KEY_ENTRY(INSERT, "Insert"),

  MM_EDIT_KEY_ENTRY(CHANGE, "Change"),
  MM_EDIT_KEY_ENTRY(OK, "OK"),
  MM_EDIT_KEY_ENTRY(SET, "Set"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(arrow)
  MM_ARROW_KEY_ENTRY(UP, "UpArrow"),
  MM_ARROW_KEY_ENTRY(DOWN, "DownArrow"),
  MM_ARROW_KEY_ENTRY(LEFT, "LeftArrow"),
  MM_ARROW_KEY_ENTRY(RIGHT, "RightArrow"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(route)
  MM_KEY_SET_ENTRY(ROUTE, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(display)
  MM_DISPLAY_KEY_ENTRY(BACKWARD, "Backward"),
  MM_DISPLAY_KEY_ENTRY(FORWARD, "Forward"),

  MM_DISPLAY_KEY_ENTRY(LSCROLL, "LeftScroll"),
  MM_DISPLAY_KEY_ENTRY(RSCROLL, "RightScroll"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(shift),
  KEY_NAME_TABLE(dot),
  KEY_NAME_TABLE(edit),
  KEY_NAME_TABLE(arrow),
  KEY_NAME_TABLE(route),
  KEY_NAME_TABLE(display),
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
writePacket (
  BrailleDisplay *brl,
  unsigned char code, unsigned char subcode,
  const unsigned char *data, size_t length
) {
  unsigned char bytes[sizeof(MM_CommandHeader) + length];
  unsigned char *byte = bytes;

  *byte++ = 0XFF;
  *byte++ = 0XFF;

  *byte++ = code;
  *byte++ = subcode;

  *byte++ = (length >> 0) & 0XFF;
  *byte++ = (length >> 8) & 0XFF;

  byte = mempcpy(byte, data, length);

  return writeBytes(brl, bytes, byte-bytes);
}

static int
startDisplayMode (BrailleDisplay *brl) {
  static const unsigned char data[] = {MM_BLINK_NO, 0};

  return writePacket(brl, MM_CMD_StartDisplayMode, 0, data, sizeof(data));
}

static int
endDisplayMode (BrailleDisplay *brl) {
  return writePacket(brl, MM_CMD_EndDisplayMode, 0, NULL, 0);
}

static int
sendBrailleData (BrailleDisplay *brl, const unsigned char *cells, size_t count) {
  return writePacket(brl, MM_CMD_SendBrailleData, 0, cells, count);
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
  return writePacket(brl, MM_CMD_QueryLineSize, 0, NULL, 0);
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
