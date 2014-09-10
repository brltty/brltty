/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include "ascii.h"

#include "brl_driver.h"
#include "brldefs-hw.h"

BEGIN_KEY_NAME_TABLE(all)
  KEY_NAME_ENTRY(HW_KEY_Power, "Power"),

  KEY_NAME_ENTRY(HW_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(HW_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(HW_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(HW_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(HW_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(HW_KEY_Dot6, "Dot6"),
  KEY_NAME_ENTRY(HW_KEY_Dot7, "Dot7"),
  KEY_NAME_ENTRY(HW_KEY_Dot8, "Dot8"),
  KEY_NAME_ENTRY(HW_KEY_Space, "Space"),

  KEY_NAME_ENTRY(HW_KEY_Nav1, "Display1"),
  KEY_NAME_ENTRY(HW_KEY_Nav2, "Display2"),
  KEY_NAME_ENTRY(HW_KEY_Nav3, "Display3"),
  KEY_NAME_ENTRY(HW_KEY_Nav4, "Display4"),
  KEY_NAME_ENTRY(HW_KEY_Nav5, "Display5"),
  KEY_NAME_ENTRY(HW_KEY_Nav6, "Display6"),

  KEY_NAME_ENTRY(HW_KEY_Thumb1, "Thumb1"),
  KEY_NAME_ENTRY(HW_KEY_Thumb2, "Thumb2"),
  KEY_NAME_ENTRY(HW_KEY_Thumb3, "Thumb3"),
  KEY_NAME_ENTRY(HW_KEY_Thumb4, "Thumb4"),

  KEY_GROUP_ENTRY(HW_GRP_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(all),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

struct BrailleDataStruct {
  int forceWrite;
  unsigned char textCells[0XFF];
};

static BraillePacketVerifierResult
verifyPacket (
  BrailleDisplay *brl,
  const unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  unsigned char byte = bytes[size-1];

  switch (size) {
    case 1:
      if (byte != ESC) return BRL_PVR_INVALID;
      *length = 3;
      break;

    case 3:
      *length += byte;
      break;

    default:
      break;
  }

  return BRL_PVR_INCLUDE;
}

static int
writePacket (BrailleDisplay *brl, unsigned char type, unsigned char length, const void *data) {
  HW_Packet packet;

  packet.fields.header = ESC;
  packet.fields.type = type;
  packet.fields.length = length;

  memcpy(packet.fields.data.bytes, data, length);
  length += packet.fields.data.bytes - packet.bytes;

  return writeBraillePacket(brl, NULL, &packet, length);
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 115200,
    .parity = SERIAL_PARITY_EVEN
  };

  static const UsbChannelDefinition usbChannelDefinitions[] = {
    { /* all models */
      .vendor=0X1C71, .product=0XC005,
      .configuration=1, .interface=1, .alternative=0,
      .inputEndpoint=2, .outputEndpoint=3,
      .serial = &serialParameters
    }
    ,
    { .vendor=0 }
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;
  descriptor.serial.options.readyDelay = 100;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  descriptor.bluetooth.channelNumber = 1;
  descriptor.bluetooth.options.readyDelay = 100;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) {
    return 1;
  }

  return 0;
}

static int
writeIdentifyRequest (BrailleDisplay *brl) {
  return writePacket(brl, HW_MSG_INIT, 0, NULL);
}

static size_t
readResponse (BrailleDisplay *brl, void *packet, size_t size) {
  return readBraillePacket(brl, NULL, packet, size, verifyPacket, NULL);
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const HW_Packet *response = packet;

  return (response->fields.type == HW_MSG_INIT_RESP)? BRL_RSP_DONE: BRL_RSP_UNEXPECTED;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    if (connectResource(brl, device)) {
      HW_Packet response;

      if (probeBrailleDisplay(brl, 0, NULL, 1000,
                              writeIdentifyRequest,
                              readResponse, &response, sizeof(response.bytes),
                              isIdentityResponse)) {
        logMessage(LOG_INFO, "detected Humanware device: model=%u cells=%u",
                   response.fields.data.init.modelIdentifier,
                   response.fields.data.init.cellCount);

        if (response.fields.data.init.communicationDisabled) {
          logMessage(LOG_WARNING, "communication channel not available");
        }

        brl->textColumns = response.fields.data.init.cellCount;
        brl->textRows = 1;

        {
          const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);
          brl->keyBindings = ktd->bindings;
          brl->keyNames = ktd->names;
        }

        makeOutputTable(dotsTable_ISO11548_1);
        brl->data->forceWrite = 1;
        return 1;
      }

      disconnectBrailleResource(brl, NULL);
    }

    free(brl->data);
    brl->data = NULL;
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
  if (cellsHaveChanged(brl->data->textCells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->forceWrite)) {
    unsigned char cells[brl->textColumns];

    translateOutputCells(cells, brl->data->textCells, brl->textColumns);
    if (!writePacket(brl, HW_MSG_DISPLAY, brl->textColumns, cells)) return 0;
  }

  return 1;
}

static void
handleKeyEvent (BrailleDisplay *brl, unsigned char key, int press) {
  KeyGroup group;

  if (key < HW_KEY_ROUTING) {
    group = HW_GRP_NavigationKeys;
  } else {
    group = HW_GRP_RoutingKeys;
    key -= HW_KEY_ROUTING;
  }

  enqueueKeyEvent(brl, group, key, press);
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  HW_Packet packet;
  size_t length;

  while ((length = readBraillePacket(brl, NULL, &packet, sizeof(packet), verifyPacket, NULL))) {
    switch (packet.fields.type) {
      case HW_MSG_KEY_DOWN:
        handleKeyEvent(brl, packet.fields.data.key.id, 1);
        continue;

      case HW_MSG_KEY_UP:
        handleKeyEvent(brl, packet.fields.data.key.id, 0);
        continue;

      default:
        break;
    }

    logUnexpectedPacket(&packet, length);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
