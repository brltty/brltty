/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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

/* MDV/braille.c - Braille display driver for MDV displays.
 *
 * Written by Stéphane Doyon (s.doyon@videotron.ca) in collaboration with
 * Simone Dal Maso <sdalmaso@protec.it>.
 *
 * It is being tested on MB408S, should also support MB208 and MB408L.
 * It is designed to be compiled in BRLTTY version 2.91-3.0.
 *
 * History:
 * 0.8: Stupid mistake processing keycode for SHIFT_PRESS/RELEASE. Swapped
 *    bindings for ATTRVIS and DISPMD. Send ACK for packet_to_process packets.
 *    Safety that forgets held routing keys when getting bad combination with
 *    ordinary keys. Bugs reported about locking/crashing with paste with
 *    routing keys, getting out of help, and getting out of freeze, are
 *    probably not solved.
 * 0.7: They have changed the protocol so that the SHIFT key pressed alone
 *    sends a code. Added plenty of key bindings. Fixed help.
 * 0.6: Added help file. Added hide/show cursor toggle on status cell
 *    routing key 1.
 * Unnumbered version: Fixes for dynmically loading drivers (declare all
 *    non-exported functions and variables static, satized debugging vs print).
 * 0.5: When receiving response to identification query, read all that's
 *    available, because there is usually an ACK packet pending (perhaps it
 *    always sends ACK + the response). Fixed bug that caused combiknation
 *    of routing and movement keys to fail.
 * 0.4: Fixed bug that put garbage instead of logging packet contents.
 *    Added key binding for showing attributes, and also for preferences menu
 *    (might change).
 * 0.3: Fixed bug in interpreting query reply which caused nonsense number
 *    of content and status cells.
 * 0.2: Added a few function keys, such as cursor tracking toggle. Put the
 *    display's line and column in status cells, with the line on top and
 *    the column on the bottom (reverse of what it was), does it work?
 *    Display parameters now set according to query reply.
 * 0.1: First draft ve5rsion. Query reply interpretation is bypassed and
 *    parameters are hard-coded. Has basic movement keys, routing keys
 *    and commands involving combinations of routing keys.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "log.h"
#include "ascii.h"

#define BRL_STATUS_FIELDS sfWindowCoordinates
#define BRL_HAVE_STATUS_CELLS

#include "brl_driver.h"
#include "brldefs-md.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 1000
#define MAXIMUM_RESPONSE_SIZE sizeof(MD_Packet)
#define MAXIMUM_TEXT_CELLS 80
#define MAXIMUM_STATUS_CELLS 2

BEGIN_KEY_NAME_TABLE(navigation)
  KEY_NAME_ENTRY(MD_KEY_F1, "F1"),
  KEY_NAME_ENTRY(MD_KEY_F2, "F2"),
  KEY_NAME_ENTRY(MD_KEY_F3, "F3"),
  KEY_NAME_ENTRY(MD_KEY_F4, "F4"),
  KEY_NAME_ENTRY(MD_KEY_F5, "F5"),
  KEY_NAME_ENTRY(MD_KEY_F6, "F6"),
  KEY_NAME_ENTRY(MD_KEY_F7, "F7"),
  KEY_NAME_ENTRY(MD_KEY_F8, "F8"),
  KEY_NAME_ENTRY(MD_KEY_F9, "F9"),
  KEY_NAME_ENTRY(MD_KEY_F10, "F10"),

  KEY_NAME_ENTRY(MD_KEY_LEFT, "Left"),
  KEY_NAME_ENTRY(MD_KEY_UP, "Up"),
  KEY_NAME_ENTRY(MD_KEY_RIGHT, "Right"),
  KEY_NAME_ENTRY(MD_KEY_DOWN, "Down"),

  KEY_NAME_ENTRY(MD_KEY_SHIFT, "Shift"),
  KEY_NAME_ENTRY(MD_KEY_LONG, "Long"),

  KEY_NAME_ENTRY(MD_KEY_DOT1, "Dot1"),
  KEY_NAME_ENTRY(MD_KEY_DOT2, "Dot2"),
  KEY_NAME_ENTRY(MD_KEY_DOT3, "Dot3"),
  KEY_NAME_ENTRY(MD_KEY_DOT4, "Dot4"),
  KEY_NAME_ENTRY(MD_KEY_DOT5, "Dot5"),
  KEY_NAME_ENTRY(MD_KEY_DOT6, "Dot6"),
  KEY_NAME_ENTRY(MD_KEY_DOT7, "Dot7"),
  KEY_NAME_ENTRY(MD_KEY_DOT8, "Dot8"),

  KEY_NAME_ENTRY(MD_KEY_SPACE, "Space"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routing)
  KEY_GROUP_ENTRY(MD_GRP_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(status)
  KEY_GROUP_ENTRY(MD_GRP_StatusKeys, "StatusKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(navigation),
  KEY_NAME_TABLE(routing),
  KEY_NAME_TABLE(status),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

struct BrailleDataStruct {
  struct {
    unsigned char cells[MAXIMUM_TEXT_CELLS];
    int rewrite;
  } text;

  struct {
    unsigned char cells[MAXIMUM_STATUS_CELLS];
    int rewrite;
  } status;
};

static uint16_t
calculateChecksum (const unsigned char *from, const unsigned char *to) {
  uint16_t checksum = 0;

  while (from < to) {
    checksum += *from++;
  }

  return checksum ^ 0XAA55;
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, size_t count) {
  return writeBraillePacket(brl, NULL, bytes, count);
}

static int
writePacket (BrailleDisplay *brl, unsigned char code, const void *data, unsigned char length) {
  MD_Packet packet;
  unsigned char *byte = packet.fields.data.bytes;

  packet.fields.soh = SOH;
  packet.fields.stx = STX;
  packet.fields.etx = ETX;

  packet.fields.code = code;
  packet.fields.length = length;
  byte = mempcpy(byte, data, length);

  uint16_t checksum = calculateChecksum(&packet.fields.stx, byte);
  *byte++ = checksum & 0XFF;
  *byte++ = checksum >> 8;

  return writeBytes(brl, packet.bytes, byte-packet.bytes);
}

static BraillePacketVerifierResult
verifyPacket (
  BrailleDisplay *brl,
  const unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  unsigned char byte = bytes[size-1];

  switch (size) {
    case 1:
      if (byte != SOH) return BRL_PVR_INVALID;
      *length = 5;
      break;

    case 2:
      if (byte != STX) return BRL_PVR_INVALID;
      break;

    case 4:
      *length += byte + 2;
      break;

    case 5:
      if (byte != ETX) return BRL_PVR_INVALID;
      break;

    default:
      if (size == *length) {
        uint16_t checksum = (byte << 8) | bytes[size-2];

        if (checksum != calculateChecksum(&bytes[1], &bytes[size-2])) {
          return BRL_PVR_INVALID;
        }
      }

      break;
  }

  return BRL_PVR_INCLUDE;
}

static size_t
readBytes (BrailleDisplay *brl, void *packet, size_t size) {
  return readBraillePacket(brl, NULL, packet, size, verifyPacket, NULL);
}

static size_t
readPacket (BrailleDisplay *brl, MD_Packet *packet) {
  return readBytes(brl, packet, sizeof(*packet));
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 19200
  };

  BEGIN_USB_CHANNEL_DEFINITIONS
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
writeIdentityRequest (BrailleDisplay *brl) {
  return writePacket(brl, MD_PT_IDENTIFY, NULL, 0);
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const MD_Packet *response = packet;

  return (response->fields.code == MD_PT_IDENTITY)? BRL_RSP_DONE: BRL_RSP_UNEXPECTED;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    if (connectResource(brl, device)) {
      MD_Packet response;

      if (probeBrailleDisplay(brl, PROBE_RETRY_LIMIT, NULL, PROBE_INPUT_TIMEOUT,
                              writeIdentityRequest,
                              readBytes, &response, sizeof(response),
                              isIdentityResponse)) {
        {
          const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

          brl->keyBindings = ktd->bindings;
          brl->keyNames = ktd->names;
        }

        {
          static const DotsTable dots = {
            0X08, 0X04, 0X02, 0X80, 0X40, 0X20, 0X01, 0X10
          };
          makeOutputTable(dots);
        }

        brl->data->text.rewrite = 1;
        brl->data->status.rewrite = 1;

        brl->textColumns = response.fields.data.identity.textCellCount;
        brl->statusColumns = response.fields.data.identity.statusCellCount;

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
brl_writeStatus (BrailleDisplay *brl, const unsigned char *cells) {
  if (cellsHaveChanged(brl->data->status.cells, cells, brl->statusColumns, NULL, NULL, &brl->data->status.rewrite)) {
    brl->data->text.rewrite = 1;
  }

  return 1;
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(brl->data->text.cells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->text.rewrite)) {
    unsigned char cells[brl->statusColumns + brl->textColumns];
    unsigned char *cell = cells;

    cell = mempcpy(cell, brl->data->status.cells, brl->statusColumns);
    cell = translateOutputCells(cell, brl->data->text.cells, brl->textColumns);

    if (!writePacket(brl, MD_PT_WRITE_ALL, cells, (cell - cells))) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  MD_Packet packet;
  size_t size;

  while ((size = readPacket(brl, &packet))) {
    logUnexpectedPacket(packet.bytes, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
