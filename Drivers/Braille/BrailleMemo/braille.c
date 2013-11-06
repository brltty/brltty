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
#define START_INPUT_TIMEOUT 1000

#define MM_KEY_SET_ENTRY(s,n) BRL_KEY_SET_ENTRY(MM, s, n)
#define MM_KEY_NAME_ENTRY(s,k,n) BRL_KEY_NAME_ENTRY(MM, s, k, n)

#define MM_SHIFT_KEY_ENTRY(k,n) MM_KEY_NAME_ENTRY(SHIFT, k, n)
#define MM_DOT_KEY_ENTRY(k) MM_KEY_NAME_ENTRY(DOT, k, "dot" #k)
#define MM_EDIT_KEY_ENTRY(k,n) MM_KEY_NAME_ENTRY(EDIT, k, n)
#define MM_ARROW_KEY_ENTRY(k,n) MM_KEY_NAME_ENTRY(ARROW, k, n)
#define MM_DISPLAY_KEY_ENTRY(k,n) MM_KEY_NAME_ENTRY(DISPLAY, k, n)

BEGIN_KEY_NAME_TABLE(shift)
  MM_SHIFT_KEY_ENTRY(F1, "Backward"),
  MM_SHIFT_KEY_ENTRY(F2, "F2"),
  MM_SHIFT_KEY_ENTRY(F3, "Chord"),
  MM_SHIFT_KEY_ENTRY(F4, "Forward"),

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
  MM_EDIT_KEY_ENTRY(ESC, "Escape"),
  MM_EDIT_KEY_ENTRY(INF, "Info"),

  MM_EDIT_KEY_ENTRY(BS, "Backspace"),
  MM_EDIT_KEY_ENTRY(DEL, "Delete"),
  MM_EDIT_KEY_ENTRY(INS, "Insert"),

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
  int forceRewrite;
  unsigned char textCells[MM_MAXIMUM_CELL_COUNT];
};

static const unsigned char sizeTable[] = {16, 32, 40};
static const unsigned char sizeCount = ARRAY_COUNT(sizeTable);

static int
isValidSize (unsigned char size) {
  return memchr(sizeTable, size, sizeCount) != NULL;
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, size_t count) {
  return writeBraillePacket(brl, NULL, bytes, count);
}

static int
writePacket (
  BrailleDisplay *brl,
  unsigned char code, unsigned char subcode,
  const unsigned char *data, size_t length
) {
  unsigned char bytes[sizeof(MM_CommandHeader) + length];
  unsigned char *byte = bytes;

  *byte++ = MM_HEADER_ID1;
  *byte++ = MM_HEADER_ID2;

  *byte++ = code;
  *byte++ = subcode;

  *byte++ = (length >> 0) & 0XFF;
  *byte++ = (length >> 8) & 0XFF;

  byte = mempcpy(byte, data, length);

  return writeBytes(brl, bytes, byte-bytes);
}

static int
verifyPacket (
  BrailleDisplay *brl,
  const unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  unsigned char byte = bytes[size-1];

  switch (size) {
    case 1:
      switch (byte) {
        case MM_HEADER_ACK:
        case MM_HEADER_NAK:
          *length = 1;
          break;

        case MM_HEADER_ID1:
          *length = sizeof(MM_CommandHeader);
          break;

        default:
          if (isValidSize(byte)) {
            *length = 1;
            break;
          }

          return 0;
      }
      break;

    case 2:
      if (byte != MM_HEADER_ID2) return 0;
      break;

    case 5:
      *length += byte;
      break;

    case 6:
      *length += byte << 8;
      break;

    default:
      break;
  }

  return 1;
}

static size_t
readBytes (BrailleDisplay *brl, void *packet, size_t size) {
  return readBraillePacket(brl, NULL, packet, size, verifyPacket, NULL);
}

static size_t
readPacket (BrailleDisplay *brl, MM_CommandPacket *packet) {
  return readBytes(brl, packet, sizeof(*packet));
}

static int
startDisplayMode (BrailleDisplay *brl) {
  static const unsigned char data[] = {MM_BLINK_NO, 0};

  if (writePacket(brl, MM_CMD_StartDisplayMode, 0, data, sizeof(data))) {
    if (gioAwaitInput(brl->gioEndpoint, START_INPUT_TIMEOUT)) {
      MM_CommandPacket response;
      size_t size = readPacket(brl, &response);

      if (size) {
        if (response.fields.header.id1 == MM_HEADER_ACK) return 1;
        logUnexpectedPacket(response.bytes, size);
      }
    }
  }

  return 0;
}

static int
endDisplayMode (BrailleDisplay *brl) {
  return writePacket(brl, MM_CMD_EndDisplayMode, 0, NULL, 0);
}

static int
sendBrailleData (BrailleDisplay *brl, const unsigned char *cells, size_t count) {
  return writePacket(brl, MM_CMD_SendBrailleData, 0, cells, count);
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 9600
  };

  static const UsbChannelDefinition usbChannelDefinitions[] = {
    { /* all models */
      .vendor=0X10C4, .product=0XEA60,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=1,
      .serial=&serialParameters
    },

    { .vendor=0 }
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  descriptor.bluetooth.channelNumber = 1;

  if (connectBrailleResource(brl, identifier, &descriptor)) {
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
  const MM_CommandPacket *cmd = packet;
  unsigned char byte = cmd->bytes[0];

  if (!isValidSize(byte)) return BRL_RSP_UNEXPECTED;
  brl->textColumns = byte;
  return BRL_RSP_DONE;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    if (connectResource(brl, device)) {
      MM_CommandPacket response;

      if (probeBrailleDisplay(brl, PROBE_RETRY_LIMIT, NULL, PROBE_INPUT_TIMEOUT,
                              writeIdentityRequest,
                              readBytes, &response, sizeof(response),
                              isIdentityResponse)) {
        if (startDisplayMode(brl)) {
          {
            const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

            brl->keyBindings = ktd->bindings;
            brl->keyNameTables = ktd->names;
          }

          {
            static const DotsTable dots = {
              0X80, 0X40, 0X20, 0X08,
              0X04, 0X02, 0X10, 0X01
            };

            makeOutputTable(dots);
          }

          brl->data->forceRewrite = 1;
          return 1;
        }
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
  disconnectBrailleResource(brl, endDisplayMode);

  if (brl->data) {
    free(brl->data);
    brl->data = NULL;
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(brl->data->textCells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->forceRewrite)) {
    unsigned char cells[brl->textColumns];

    translateOutputCells(cells, brl->data->textCells, brl->textColumns);
    if (!sendBrailleData(brl, cells, sizeof(cells))) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  MM_CommandPacket packet;
  size_t size;

  while ((size = readPacket(brl, &packet))) {
    if ((packet.fields.header.id1 == MM_HEADER_ID1) &&
        (packet.fields.header.id2 == MM_HEADER_ID2)) {
      switch (packet.fields.header.code) {
        case MM_CMD_KeyCombination:
          switch (packet.fields.data.keys.group) {
            case MM_SET_SHIFT:
              if (!packet.fields.data.keys.value) {
                enqueueKeys(brl, packet.fields.data.keys.shift, MM_SET_SHIFT, 0);
                continue;
              }
              break;

            case MM_SET_DOT:
            case MM_SET_EDIT:
            case MM_SET_ARROW:
            case MM_SET_DISPLAY:
            {
              uint32_t shift = 0;

              enqueueUpdatedKeys(brl, packet.fields.data.keys.shift, &shift, MM_SET_SHIFT, 0);
              enqueueKeys(brl, packet.fields.data.keys.value, packet.fields.data.keys.group, 0);
              enqueueUpdatedKeys(brl, 0, &shift, MM_SET_SHIFT, 0);
              continue;
            }

            case MM_SET_ROUTE:
            {
              unsigned char key = packet.fields.data.keys.value;

              if ((key > 0) && (key <= brl->textColumns)) {
                uint32_t shift = 0;

                enqueueUpdatedKeys(brl, packet.fields.data.keys.shift, &shift, MM_SET_SHIFT, 0);
                enqueueKey(brl, packet.fields.data.keys.group, key-1);
                enqueueUpdatedKeys(brl, 0, &shift, MM_SET_SHIFT, 0);
                continue;
              }

              break;
            }

            default:
              break;
          }
          break;

        case MM_CMD_ShiftPress:
        case MM_CMD_ShiftRelease:
          continue;

        default:
          break;
      }
    }

    logUnexpectedPacket(packet.bytes, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
