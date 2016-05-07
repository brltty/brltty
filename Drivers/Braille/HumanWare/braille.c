/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2016 by The BRLTTY Developers.
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
#include "ascii.h"
#include "bitmask.h"

#include "brl_driver.h"
#include "brldefs-hw.h"

BEGIN_KEY_NAME_TABLE(nav)
  KEY_NAME_ENTRY(HW_KEY_Reset, "Reset"),

  KEY_NAME_ENTRY(HW_KEY_Command1, "Display1"),
  KEY_NAME_ENTRY(HW_KEY_Command2, "Display2"),
  KEY_NAME_ENTRY(HW_KEY_Command3, "Display3"),
  KEY_NAME_ENTRY(HW_KEY_Command4, "Display4"),
  KEY_NAME_ENTRY(HW_KEY_Command5, "Display5"),
  KEY_NAME_ENTRY(HW_KEY_Command6, "Display6"),

  KEY_NAME_ENTRY(HW_KEY_Thumb1, "Thumb1"),
  KEY_NAME_ENTRY(HW_KEY_Thumb2, "Thumb2"),
  KEY_NAME_ENTRY(HW_KEY_Thumb3, "Thumb3"),
  KEY_NAME_ENTRY(HW_KEY_Thumb4, "Thumb4"),

  KEY_GROUP_ENTRY(HW_GRP_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(kbd)
  KEY_NAME_ENTRY(HW_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(HW_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(HW_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(HW_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(HW_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(HW_KEY_Dot6, "Dot6"),
  KEY_NAME_ENTRY(HW_KEY_Dot7, "Dot7"),
  KEY_NAME_ENTRY(HW_KEY_Dot8, "Dot8"),
  KEY_NAME_ENTRY(HW_KEY_Space, "Space"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(mb1)
  KEY_NAME_TABLE(nav),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(mb2)
  KEY_NAME_TABLE(nav),
  KEY_NAME_TABLE(kbd),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(mb1)
DEFINE_KEY_TABLE(mb2)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(mb1),
  &KEY_TABLE_DEFINITION(mb2),
END_KEY_TABLE_LIST

#define SERIAL_PROBE_RETRIES 2
#define SERIAL_PROBE_TIMEOUT 1000

#define MAXIMUM_TEXT_CELL_COUNT 0XFF

#define MAXIMUM_KEY_VALUE 0XFF
#define KEYS_BITMASK(name) BITMASK(name, (MAXIMUM_KEY_VALUE + 1), int)

typedef struct {
  const char *name;
  int (*probeDisplay) (BrailleDisplay *brl);
  int (*writeCells) (BrailleDisplay *brl, const unsigned char *cells, unsigned char count);
  int (*processKeys) (BrailleDisplay *brl);
} ProtocolEntry;

struct BrailleDataStruct {
  const ProtocolEntry *protocol;

  struct {
    unsigned char rewrite;
    unsigned char cells[MAXIMUM_TEXT_CELL_COUNT];
  } text;

  struct {
    struct {
      unsigned char count;
      KEYS_BITMASK(mask);
    } pressedKeys;
  } hid;
};

static int
handleKeyEvent (BrailleDisplay *brl, unsigned char key, int press) {
  KeyGroup group;

  if (key < HW_KEY_ROUTING) {
    group = HW_GRP_NavigationKeys;
  } else {
    group = HW_GRP_RoutingKeys;
    key -= HW_KEY_ROUTING;
  }

  return enqueueKeyEvent(brl, group, key, press);
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

  if (data) memcpy(packet.fields.data.bytes, data, length);
  length += packet.fields.data.bytes - packet.bytes;

  return writeBraillePacket(brl, NULL, &packet, length);
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
probeSerialDisplay (BrailleDisplay *brl) {
  HW_Packet response;

  if (probeBrailleDisplay(brl, SERIAL_PROBE_RETRIES,
                          NULL, SERIAL_PROBE_TIMEOUT,
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
    return 1;
  }

  return 0;
}

static int
writeSerialCells (BrailleDisplay *brl, const unsigned char *cells, unsigned char count) {
  return writePacket(brl, HW_MSG_DISPLAY, count, cells);
}

static int
processSerialKeys (BrailleDisplay *brl) {
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

  return errno == EAGAIN;
}

static const ProtocolEntry serialProtocol = {
  .name = "serial",
  .probeDisplay = probeSerialDisplay,
  .writeCells = writeSerialCells,
  .processKeys = processSerialKeys
};

static ssize_t
readFeature (
  BrailleDisplay *brl, unsigned char report,
  unsigned char *buffer, size_t size
) {
  ssize_t length = gioGetHidFeature(brl->gioEndpoint, report, buffer, size);

  if (length != -1) {
    logInputPacket(buffer, length);
  } else {
    logSystemError("HID feature read");
  }

  return length;
}

static ssize_t
readReport (
  BrailleDisplay *brl, unsigned char report,
  unsigned char *buffer, size_t size
) {
  ssize_t length = gioGetHidReport(brl->gioEndpoint, report, buffer, size);

  if (length != -1) {
    logInputPacket(buffer, length);
  } else {
    logSystemError("HID report read");
  }

  return length;
}

static int
writeReport (BrailleDisplay *brl, const unsigned char *data, size_t size) {
  logOutputPacket(data, size);

  {
    ssize_t result = gioWriteHidReport(brl->gioEndpoint, data, size);
    if (result != -1) return 1;
  }

  logSystemError("HID report write");
  return 0;
}

static int
probeHidDisplay (BrailleDisplay *brl) {
  HW_CapabilitiesReport capabilities;
  unsigned char *const buffer = (unsigned char *)&capabilities;
  const size_t size = sizeof(capabilities);
  ssize_t length = readFeature(brl, HW_REP_FTR_Capabilities, buffer, size);

  if (length != -1) {
    memset(&buffer[length], 0, (size - length));

    logMessage(LOG_INFO, "Firmware Version: %c.%c.%c%c",
               capabilities.version.major, capabilities.version.minor,
               capabilities.version.revision[0], capabilities.version.revision[1]);

    brl->textColumns = capabilities.cellCount;
    return 1;
  }

  return 0;
}

static int
writeHidCells (BrailleDisplay *brl, const unsigned char *cells, unsigned char count) {
  unsigned char buffer[4 + count];
  unsigned char *byte = buffer;

  *byte++ = HW_REP_OUT_WriteCells;
  *byte++ = 1;
  *byte++ = 0;
  *byte++ = count;
  byte = mempcpy(byte, cells, count);

  return writeReport(brl, buffer, byte-buffer);
}

static int
processHidKeys (BrailleDisplay *brl) {
  unsigned char buffer[0X100];
  ssize_t length = readReport(brl, HW_REP_IN_PressedKeys, buffer, sizeof(buffer));
  if (length == -1) return 0;

  KEYS_BITMASK(pressedMask);
  BITMASK_ZERO(pressedMask);
  unsigned int pressedCount = 0;

  {
    const unsigned char *key = buffer + 1;
    const unsigned char *end = buffer + length;

    while (key < end) {
      if (!*key) break;

      if (!BITMASK_TEST(pressedMask, *key)) {
        BITMASK_SET(pressedMask, *key);
        pressedCount += 1;

        if (!BITMASK_TEST(brl->data->hid.pressedKeys.mask, *key)) {
          handleKeyEvent(brl, *key, 1);
          BITMASK_SET(brl->data->hid.pressedKeys.mask, *key);
          brl->data->hid.pressedKeys.count += 1;
        }
      }

      key += 1;
    }
  }

  if (brl->data->hid.pressedKeys.count > pressedCount) {
    for (unsigned int key=0; key<=MAXIMUM_KEY_VALUE; key+=1) {
      if (BITMASK_TEST(brl->data->hid.pressedKeys.mask, key)) {
        if (!BITMASK_TEST(pressedMask, key)) {
          handleKeyEvent(brl, key, 0);
          BITMASK_CLEAR(brl->data->hid.pressedKeys.mask, key);
          if (--brl->data->hid.pressedKeys.count == pressedCount) break;
        }
      }
    }
  }

  return 1;
}

static const ProtocolEntry hidProtocol = {
  .name = "HID",
  .probeDisplay = probeHidDisplay,
  .writeCells = writeHidCells,
  .processKeys = processHidKeys
};

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 115200,
    .parity = SERIAL_PARITY_EVEN
  };

  BEGIN_USB_CHANNEL_DEFINITIONS
    { /* all models (serial protocol) */
      .vendor=0X1C71, .product=0XC005, 
      .configuration=1, .interface=1, .alternative=0,
      .inputEndpoint=2, .outputEndpoint=3,
      .disableEndpointReset = 1,
      .serial = &serialParameters,
      .data = &serialProtocol
    },

    { /* all models (HID protocol) */
      .vendor=0X1C71, .product=0XC006,
      .configuration=1, .interface=0, .alternative=0,
      .data = &hidProtocol
    },
  END_USB_CHANNEL_DEFINITIONS

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;
  descriptor.serial.options.applicationData = &serialProtocol;
  descriptor.serial.options.readyDelay = 100;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  descriptor.bluetooth.channelNumber = 1;
  descriptor.bluetooth.options.applicationData = &serialProtocol;
  descriptor.bluetooth.options.readyDelay = 100;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) {
    brl->data->protocol = gioGetApplicationData(brl->gioEndpoint);
    return 1;
  }

  return 0;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    if (connectResource(brl, device)) {
      if (brl->data->protocol->probeDisplay(brl)) {
        {
          const KeyTableDefinition *ktd;
          switch (brl->textColumns) {
            case 80:
              ktd = &KEY_TABLE_DEFINITION(mb1);
              break;

            default:
            case 40:
            case 32:
              ktd = &KEY_TABLE_DEFINITION(mb2);
              break;
          }

          setBrailleKeyTable(brl, ktd);
        }

        makeOutputTable(dotsTable_ISO11548_1);
        brl->data->text.rewrite = 1;
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
  const size_t count = brl->textColumns;

  if (cellsHaveChanged(brl->data->text.cells, brl->buffer, count, NULL, NULL, &brl->data->text.rewrite)) {
    unsigned char cells[count];

    translateOutputCells(cells, brl->data->text.cells, count);
    if (!brl->data->protocol->writeCells(brl, cells, count)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  return brl->data->protocol->processKeys(brl)? EOF: BRL_CMD_RESTARTBRL;
}
