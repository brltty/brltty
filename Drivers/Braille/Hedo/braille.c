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

#include "brl_driver.h"
#include "brldefs-hd.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 1000

#define MAXIMUM_RESPONSE_SIZE 1
#define MAXIMUM_TEXT_CELL_COUNT 80
#define MAXIMUM_STATUS_CELL_COUNT 4

BEGIN_KEY_NAME_TABLE(pfl)
  KEY_NAME_ENTRY(HD_PFL_K1, "K1"),
  KEY_NAME_ENTRY(HD_PFL_K2, "K2"),
  KEY_NAME_ENTRY(HD_PFL_K3, "K3"),

  KEY_NAME_ENTRY(HD_PFL_B1, "B1"),
  KEY_NAME_ENTRY(HD_PFL_B2, "B2"),
  KEY_NAME_ENTRY(HD_PFL_B3, "B3"),
  KEY_NAME_ENTRY(HD_PFL_B4, "B4"),
  KEY_NAME_ENTRY(HD_PFL_B5, "B5"),
  KEY_NAME_ENTRY(HD_PFL_B6, "B6"),
  KEY_NAME_ENTRY(HD_PFL_B7, "B7"),
  KEY_NAME_ENTRY(HD_PFL_B8, "B8"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(mbl)
  KEY_NAME_ENTRY(HD_MBL_B1, "B1"),
  KEY_NAME_ENTRY(HD_MBL_B2, "B2"),
  KEY_NAME_ENTRY(HD_MBL_B3, "B3"),

  KEY_NAME_ENTRY(HD_MBL_B4, "B4"),
  KEY_NAME_ENTRY(HD_MBL_B5, "B5"),
  KEY_NAME_ENTRY(HD_MBL_B6, "B6"),

  KEY_NAME_ENTRY(HD_MBL_K1, "K1"),
  KEY_NAME_ENTRY(HD_MBL_K2, "K2"),
  KEY_NAME_ENTRY(HD_MBL_K3, "K3"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routing)
  KEY_SET_ENTRY(HD_SET_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(pfl)
  KEY_NAME_TABLE(pfl),
  KEY_NAME_TABLE(routing),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(mbl)
  KEY_NAME_TABLE(mbl),
  KEY_NAME_TABLE(routing),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(pfl)
DEFINE_KEY_TABLE(mbl)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(pfl),
  &KEY_TABLE_DEFINITION(mbl),
END_KEY_TABLE_LIST

typedef int KeyCodeInterpreter (BrailleDisplay *brl, unsigned char code);

typedef struct {
  const char *modelName;
  const KeyTableDefinition *keyTableDefinition;
  KeyCodeInterpreter *interpretKeyCode;
  unsigned char textCellCount;
  unsigned char statusCellCount;
  unsigned char firstRoutingKey;
  unsigned char acknowledgementResponse;
} ModelEntry;

struct BrailleDataStruct {
  const ModelEntry *model;

  int forceRewrite;
  unsigned char textCells[MAXIMUM_TEXT_CELL_COUNT];
  unsigned char statusCells[MAXIMUM_STATUS_CELL_COUNT];

  uint32_t pressedKeys;
};

static int
interpretKeyCode_ProfiLine (BrailleDisplay *brl, unsigned char code) {
  const unsigned char release = 0X80;
  int press = !(code & release);
  unsigned char key = code & ~release;
  unsigned char set;

  if (key < brl->data->model->firstRoutingKey) {
    set = HD_SET_NavigationKeys;
  } else if (key < (brl->data->model->firstRoutingKey + brl->textColumns)) {
    set = HD_SET_RoutingKeys;
    key -= brl->data->model->firstRoutingKey;
  } else {
    return 0;
  }

  enqueueKeyEvent(brl, set, key, press);
  return 1;
}

static int
interpretKeyCode_MobilLine (BrailleDisplay *brl, unsigned char code) {
  if ((code >= brl->data->model->firstRoutingKey) &&
      (code < (brl->data->model->firstRoutingKey + brl->textColumns))) {
    enqueueKey(brl, HD_SET_RoutingKeys, (code - brl->data->model->firstRoutingKey));
    return 1;
  }

  {
    unsigned char group = (code & 0XF0) >> 4;

    if (group <= 2) {
      unsigned char shift = group * 4;
      uint32_t pressedKeys = (brl->data->pressedKeys & ~(0XF << shift)) | ((code & 0XF) << shift);

      enqueueUpdatedKeys(brl, pressedKeys, &brl->data->pressedKeys,
                         HD_SET_NavigationKeys, 0);
      return 1;
    }
  }

  return 0;
}

static const ModelEntry modelEntry_ProfiLine = {
  .modelName = "ProfiLine USB",
  .textCellCount = 80,
  .statusCellCount = 4,

  .keyTableDefinition = &KEY_TABLE_DEFINITION(pfl),
  .interpretKeyCode = interpretKeyCode_ProfiLine,
  .firstRoutingKey = 0X20,

  .acknowledgementResponse = 0X7E
};

static const ModelEntry modelEntry_MobilLine = {
  .modelName = "MobilLine USB",
  .textCellCount = 40,
  .statusCellCount = 2,

  .keyTableDefinition = &KEY_TABLE_DEFINITION(mbl),
  .interpretKeyCode = interpretKeyCode_MobilLine,
  .firstRoutingKey = 0X40,

  .acknowledgementResponse = 0X30
};

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, size_t count) {
  return writeBraillePacket(brl, NULL, bytes, count);
}

static BraillePacketVerifierResult
verifyPacket (
  BrailleDisplay *brl,
  const unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  switch (size) {
    case 1:
      *length = 1;
      break;

    default:
      break;
  }

  return BRL_PVR_INCLUDE;
}

static size_t
readPacket (BrailleDisplay *brl, void *packet, size_t size) {
  return readBraillePacket(brl, NULL, packet, size, verifyPacket, NULL);
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters_ProfiLine = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 19200,
    .parity = SERIAL_PARITY_ODD
  };

  static const SerialParameters serialParameters_MobilLine = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 9600,
    .parity = SERIAL_PARITY_ODD
  };

  static const UsbChannelDefinition usbChannelDefinitions[] = {
    { /* ProfiLine */
      .vendor=0X0403, .product=0XDE59,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .serial = &serialParameters_ProfiLine,
      .data = &modelEntry_ProfiLine
    },

    { /* MobilLine */
      .vendor=0X0403, .product=0XDE58,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .serial = &serialParameters_MobilLine,
      .data = &modelEntry_MobilLine
    },

    { .vendor=0 }
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) {
    return 1;
  }

  return 0;
}

static int
writeCells (BrailleDisplay *brl) {
  unsigned char packet[1 + brl->statusColumns + brl->textColumns];
  unsigned char *byte = packet;

  *byte++ = HD_REQ_WRITE_CELLS;
  byte = mempcpy(byte, brl->data->statusCells, brl->statusColumns);
  byte = translateOutputCells(byte, brl->data->textCells, brl->textColumns);

  return writeBytes(brl, packet, (byte - packet));
}

static int
writeIdentityRequest (BrailleDisplay *brl) {
  memset(brl->data->textCells, 0, sizeof(brl->data->textCells));
  memset(brl->data->statusCells, 0, sizeof(brl->data->statusCells));
  return writeCells(brl);
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const unsigned char *bytes = packet;

  return (bytes[0] == brl->data->model->acknowledgementResponse)? BRL_RSP_DONE: BRL_RSP_UNEXPECTED;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    if (connectResource(brl, device)) {
      unsigned char response[MAXIMUM_RESPONSE_SIZE];

      brl->data->model = gioGetApplicationData(brl->gioEndpoint);
      brl->textColumns = brl->data->model->textCellCount;
      brl->statusColumns = brl->data->model->statusCellCount;
      makeOutputTable(dotsTable_ISO11548_1);

      if (probeBrailleDisplay(brl, PROBE_RETRY_LIMIT, NULL, PROBE_INPUT_TIMEOUT,
                              writeIdentityRequest,
                              readPacket, &response, sizeof(response),
                              isIdentityResponse)) {
        {
          const KeyTableDefinition *ktd = brl->data->model->keyTableDefinition;

          brl->keyBindings = ktd->bindings;
          brl->keyNameTables = ktd->names;
        }

        brl->data->forceRewrite = 1;
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
  if (cellsHaveChanged(brl->data->textCells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->forceRewrite)) {
    if (!writeCells(brl)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  unsigned char packet[MAXIMUM_RESPONSE_SIZE];
  size_t size;

  while ((size = readPacket(brl, packet, sizeof(packet)))) {
    unsigned char byte = packet[0];

    if (byte == brl->data->model->acknowledgementResponse) continue;
    if (brl->data->model->interpretKeyCode(brl, byte)) continue;
    logUnexpectedPacket(packet, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
