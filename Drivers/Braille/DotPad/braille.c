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
#include "bitfield.h"

#include "brl_driver.h"
#include "brldefs-dp.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 1000

#define KEY_ENTRY(s,t,k,n) {.value = {.group=DP_GRP_##s, .number=DP_##t##_##k}, .name=n}
#define BASIC_KEY_ENTRY(k,n) KEY_ENTRY(BasicKeys, BSC, k, n)
#define KEYBOARD_KEY_ENTRY(k,n) KEY_ENTRY(PerkinsKeys, KBD, k, n)
#define PANNING_KEY_ENTRY(k,n) KEY_ENTRY(PerkinsKeys, PAN, k, n)
#define NAVIGATION_KEY_ENTRY(k,n) KEY_ENTRY(PerkinsKeys, NAV, k, n)

BEGIN_KEY_NAME_TABLE(basic)
  BASIC_KEY_ENTRY(LEFT_PREV, "LeftPrev"),
  BASIC_KEY_ENTRY(LEFT_NEXT, "LeftNext"),
  BASIC_KEY_ENTRY(RIGHT_PREV, "RightPrev"),
  BASIC_KEY_ENTRY(RIGHT_NEXT, "RightNext"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(keyboard)
  KEYBOARD_KEY_ENTRY(DOT1, "Dot1"),
  KEYBOARD_KEY_ENTRY(DOT2, "Dot2"),
  KEYBOARD_KEY_ENTRY(DOT3, "Dot3"),
  KEYBOARD_KEY_ENTRY(DOT4, "Dot4"),
  KEYBOARD_KEY_ENTRY(DOT5, "Dot5"),
  KEYBOARD_KEY_ENTRY(DOT6, "Dot6"),
  KEYBOARD_KEY_ENTRY(DOT7, "Dot7"),
  KEYBOARD_KEY_ENTRY(DOT8, "Dot8"),

  KEYBOARD_KEY_ENTRY(SPACE, "Space"),
  KEYBOARD_KEY_ENTRY(SHIFT_LEFT, "LeftShift"),
  KEYBOARD_KEY_ENTRY(SHIFT_RIGHT, "RightShift"),
  KEYBOARD_KEY_ENTRY(CONTROL_LEFT, "LeftControl"),
  KEYBOARD_KEY_ENTRY(CONTROL_RIGHT, "RightControl"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(panning)
  PANNING_KEY_ENTRY(LEFT, "PanLeft"),
  PANNING_KEY_ENTRY(RIGHT, "PanRight"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(navigation)
  NAVIGATION_KEY_ENTRY(CENTER, "NavCenter"),
  NAVIGATION_KEY_ENTRY(LEFT, "NavLeft"),
  NAVIGATION_KEY_ENTRY(RIGHT, "NavRight"),
  NAVIGATION_KEY_ENTRY(UP, "NavUp"),
  NAVIGATION_KEY_ENTRY(DOWN, "NavDown"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(function)
  KEY_GROUP_ENTRY(DP_GRP_FunctionKeys, "FunctionKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routing)
  KEY_GROUP_ENTRY(DP_GRP_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(basic),
  KEY_NAME_TABLE(keyboard),
  KEY_NAME_TABLE(panning),
  KEY_NAME_TABLE(navigation),
  KEY_NAME_TABLE(routing),
  KEY_NAME_TABLE(function),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(panfn4)
  KEY_NAME_TABLE(panning),
  KEY_NAME_TABLE(function),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)
DEFINE_KEY_TABLE(panfn4)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
  &KEY_TABLE_DEFINITION(panfn4),
END_KEY_TABLE_LIST

struct BrailleDataStruct {
  DP_BoardInformation boardInformation;
  unsigned char firmwareVersion[8];
  unsigned char deviceName[10];
  const KeyNameEntry *keyNameTable[7];

  struct {
    unsigned char basic[4];
    unsigned char perkins[4];
    unsigned char routing[8];
    unsigned char function[4];
  } keys;

  struct {
    unsigned char rewrite;
    unsigned char cells[DP_MAXIMUM_TEXT_COLUMNS];
  } text;
};

static uint16_t
getUint16 (const unsigned char bytes[2]) {
  union {
    const unsigned char *ofBytes;
    const uint16_t *ofUint16;
  } address;

  address.ofBytes = bytes;
  return getBigEndian16(*address.ofUint16);
}

static void
putUint16 (unsigned char bytes[2], uint16_t value) {
  union {
    unsigned char *ofBytes;
    uint16_t *ofUint16;
  } address;

  address.ofBytes = bytes;
  putBigEndian16(address.ofUint16, value);
}

static unsigned char
makePacketChecksum (const DP_Packet *packet) {
  unsigned char checksum = 0XA5;

  {
    const unsigned char *byte = &packet->fields.destination;
    const unsigned char *end = byte + getUint16(packet->fields.length) - 1;
    while (byte < end) checksum ^= *byte++;
  }

  return checksum;
}

static int
writePacket (BrailleDisplay *brl, const DP_Packet *packet) {
  size_t size = getUint16(packet->fields.length);
  size += &packet->fields.destination - packet->bytes;

  unsigned int type = getUint16(packet->fields.command) << 8;
  type |= packet->fields.destination;

  return writeBrailleMessage(brl, NULL, type, packet, size);
}

static int
writeRequest (BrailleDisplay *brl, uint16_t command, uint8_t destination, const void *data, size_t size) {
  if (!data) size = 0;
  DP_Packet packet;

  packet.fields.sync[0] = DP_PSB_SYNC1;
  packet.fields.sync[1] = DP_PSB_SYNC2;

  packet.fields.destination = destination;
  putUint16(packet.fields.command, command);
  packet.fields.seq = 0;

  uint8_t *checksum = mempcpy(packet.fields.data, data, size);
  uint16_t length = (checksum - &packet.fields.destination) + 1;
  putUint16(packet.fields.length, length);
  *checksum = makePacketChecksum(&packet);

  return writePacket(brl, &packet);
}

static int
verifyPacketChecksum (const DP_Packet *packet, unsigned char received) {
  unsigned char expected = makePacketChecksum(packet);
  if (received == expected) return 1;

  logMessage(LOG_WARNING,
    "checksum mismatch: Received:%02X Expected:%02X",
    received, expected
  );

  return 0;
}

static int
verifyPacketLength (const DP_Packet *packet, const BrailleDisplay *brl) {
  uint16_t received = getUint16(packet->fields.length);
  uint16_t command = getUint16(packet->fields.command);
  uint16_t expected = 5;

  switch (command) {
    case DP_RSP_FIRMWARE_VERSION:
      expected += sizeof(brl->data->firmwareVersion);
      break;

    case DP_RSP_DEVICE_NAME:
      expected += sizeof(brl->data->deviceName);
      break;

    case DP_RSP_BOARD_INFORMATION:
      expected += sizeof(brl->data->boardInformation);
      break;

    case DP_RSP_DISPLAY_LINE:
      expected += 1;
      break;

    case DP_NTF_DISPLAY_LINE:
      expected += 1;
      break;

    case DP_NTF_KEYS_BASIC:
      expected += sizeof(brl->data->keys.basic);
      break;

    case DP_NTF_KEYS_PERKINS:
      expected += sizeof(brl->data->keys.perkins);
      break;

    case DP_NTF_KEYS_ROUTING:
      expected += sizeof(brl->data->keys.routing);
      break;

    case DP_NTF_KEYS_FUNCTION:
      expected += sizeof(brl->data->keys.function);
      break;

    case DP_NTF_ERROR:
      expected += 1;
      break;
  }

  if (received != expected) {
    logMessage(LOG_WARNING,
      "length mismatch (command %04X): Received:%u Expected:%u",
      getUint16(packet->fields.command), received, expected
    );
  }

  return 1;
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
      if (byte != DP_PSB_SYNC1) return BRL_PVR_INVALID;
      *length = 4;
      break;

    case 2:
      if (byte != DP_PSB_SYNC2) return BRL_PVR_INVALID;
      break;

    case 4:
      *length += getUint16(&bytes[2]);
      break;

    default:
      break;
  }

  if (size == *length) {
    const void *packet = bytes;
    verifyPacketChecksum(packet, byte);
    verifyPacketLength(packet, brl);
  }

  return BRL_PVR_INCLUDE;
}

static size_t
readPacket (BrailleDisplay *brl, void *packet, size_t size) {
  return readBraillePacket(brl, NULL, packet, size, verifyPacket, NULL);
}

static const KeyNameEntry **
makeKeyNameTable (BrailleDisplay *brl) {
  typedef struct {
    const char *type;
    const KeyNameEntry *keyNames;
    unsigned char featureBit;
  } OptionalKeysDescriptor;

  static const OptionalKeysDescriptor optionalKeysTable[] = {
    { .type = "basic",
      .keyNames = KEY_NAME_TABLE(basic),
      // not actually used
    },

    { .type = "keyboard",
      .keyNames = KEY_NAME_TABLE(keyboard),
      .featureBit = DP_HAS_PERKINS_KEYS,
    },

    { .type = "panning",
      .keyNames = KEY_NAME_TABLE(panning),
      .featureBit = DP_HAS_PANNING_KEYS,
    },

    { .type = "navigation",
      .keyNames = KEY_NAME_TABLE(navigation),
      .featureBit = DP_HAS_NAVIGATION_KEYS,
    },

    { .type = "routing",
      .keyNames = KEY_NAME_TABLE(routing),
      .featureBit = DP_HAS_ROUTING_KEYS,
    },

    { .type = "function",
      .keyNames = KEY_NAME_TABLE(function),
      .featureBit = DP_HAS_FUNCTION_KEYS,
    },
  };

  const KeyNameEntry **names = brl->data->keyNameTable;
  const OptionalKeysDescriptor *okd = optionalKeysTable;
  const OptionalKeysDescriptor *end = okd + ARRAY_COUNT(optionalKeysTable);

  while (okd < end) {
    if (brl->data->boardInformation.features & okd->featureBit) {
      logMessage(LOG_CATEGORY(BRAILLE_DRIVER), "has %s keys", okd->type);
      *names++ = okd->keyNames;
    }

    okd += 1;
  }

  *names = LAST_KEY_NAME_TABLE;
  return brl->data->keyNameTable;
}

static void
setKeyTable (BrailleDisplay *brl) {
  const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);
  brl->keyBindings = ktd->bindings;
  brl->keyNames = makeKeyNameTable(brl);
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 115200,
  };

  BEGIN_USB_CHANNEL_DEFINITIONS
    { /* all models */
      .vendor=0X0403, .product=0X6010,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .serial=&serialParameters
    },
  END_USB_CHANNEL_DEFINITIONS

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;
//descriptor.usb.options.readyDelay = 3000;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) {
    return 1;
  }

  return 0;
}

static int
writeIdentifyRequest (BrailleDisplay *brl) {
  return writeRequest(brl, DP_REQ_BOARD_INFORMATION, 0, NULL, 0);
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const DP_Packet *response = packet;

  if (getUint16(response->fields.command) != DP_RSP_BOARD_INFORMATION) {
    return BRL_RSP_UNEXPECTED;
  }

  memcpy(
    &brl->data->boardInformation, response->fields.data,
    sizeof(brl->data->boardInformation)
  );

  logBytes(LOG_CATEGORY(BRAILLE_DRIVER),
    "Board Information",
    &brl->data->boardInformation,
    sizeof(brl->data->boardInformation)
  );

  acknowledgeBrailleMessage(brl);
  return BRL_RSP_DONE;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    if (connectResource(brl, device)) {
      DP_Packet response;

      if (probeBrailleDisplay(brl, PROBE_RETRY_LIMIT, NULL, PROBE_INPUT_TIMEOUT,
                              writeIdentifyRequest,
                              readPacket, &response, sizeof(response),
                              isIdentityResponse)) {
        brl->acknowledgements.missing.timeout = (brl->data->boardInformation.text.refreshTime * 100) + 1000;

        if (writeRequest(brl, DP_REQ_FIRMWARE_VERSION, 0, NULL, 0)) {
          if (writeRequest(brl, DP_REQ_DEVICE_NAME, 0, NULL, 0)) {
            setKeyTable(brl);
            MAKE_OUTPUT_TABLE(0X01, 0X02, 0X04, 0X10, 0X20, 0X40, 0X08, 0X80);

            brl->textColumns = brl->data->boardInformation.text.columnCount;
            brl->textRows = brl->data->boardInformation.text.rowCount;

            brl->data->text.rewrite = 1;
            return 1;
          }
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
  endBrailleMessages(brl);
  disconnectBrailleResource(brl, NULL);

  if (brl->data) {
    free(brl->data);
    brl->data = NULL;
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(brl->data->text.cells, brl->buffer, brl->textColumns,
                       NULL, NULL, &brl->data->text.rewrite)) {
    size_t size = brl->textColumns * brl->textRows;
    unsigned char data[1 + size];

    data[0] = 0;
    translateOutputCells(&data[1], brl->data->text.cells, size);
    if (!writeRequest(brl, DP_REQ_DISPLAY_LINE, 0, data, sizeof(data))) return 0;
  }

  return 1;
}

static int
getDataSize (const DP_Packet *packet) {
  return getUint16(packet->fields.length)
       - 1 // checksum
       - (packet->fields.data - &packet->fields.destination) // header
       ;
}

static void
reportRequestError (unsigned char code) {
  static const char *const reasons[] = {
    [DP_ERR_LENGTH]    = "unexpected length",
    [DP_ERR_COMMAND]   = "unrecognized command",
    [DP_ERR_CHECKSUM]  = "incorrect checksum",
    [DP_ERR_PARAMETER] = "invalid parameter",
    [DP_ERR_TIMEOUT]   = "read timed out",
  };

  const char *reason = NULL;
  if (code < ARRAY_COUNT(reasons)) reason = reasons[code];
  if (!reason) reason = "unknown problem";

  logMessage(LOG_WARNING,
    "request rejected by device: %u (%s)",
    code, reason
  );
}

static void
reportDisplayError (unsigned char code) {
  static const char *const reasons[] = {
    [DP_DRC_ACK]      = "positive acknowledgement",
    [DP_DRC_NACK]     = "negative acknowledgement",
    [DP_DRC_WAIT]     = "wait",
    [DP_DRC_CHECKSUM] = "incorrect checksum",
  };

  const char *reason = NULL;
  if (code < ARRAY_COUNT(reasons)) reason = reasons[code];
  if (!reason) reason = "unknown problem";

  logMessage(LOG_WARNING,
    "display rejected by device: %u (%s)",
    code, reason
  );
}

static void
saveField (
  const DP_Packet *packet, const char *label,
  unsigned char *field, int fieldSize
) {
  int dataSize = getDataSize(packet);

  if (dataSize > fieldSize) dataSize = fieldSize;
  memcpy(field, packet->fields.data, dataSize);
  while (dataSize < fieldSize) field[dataSize++] = ' ';

  logMessage(LOG_CATEGORY(BRAILLE_DRIVER),
    "%s: %.*s", label, fieldSize, field
  );
}

static unsigned char
reverseByteBits (unsigned char fromByte) {
  unsigned char toByte = 0;

  unsigned char fromBit = 0X80;
  unsigned char toBit = 0X01;

  while (fromBit) {
    if (fromByte & fromBit) toByte |= toBit;
    fromBit >>= 1;
    toBit <<= 1;
  }

  return toByte;
}

static int
updateKeyGroup (
  BrailleDisplay *brl, const DP_Packet *packet, KeyGroup keyGroup,
  unsigned char *array, size_t arraySize
) {
  int dataSize = getDataSize(packet);

  if (dataSize > 0) {
    unsigned char data[arraySize];
    if (dataSize > arraySize) dataSize = arraySize;

    for (int i=0; i<dataSize; i+=1) {
      data[i] = reverseByteBits(packet->fields.data[i]);
    }

    while (dataSize < arraySize) {
      data[dataSize++] = 0;
    }

    if (!enqueueUpdatedKeyGroup(brl, (arraySize * 8), data, array, keyGroup)) {
      return 0;
    }
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  DP_Packet packet;
  size_t size;

  while ((size = readPacket(brl, packet.bytes, sizeof(packet)))) {
    switch (getUint16(packet.fields.command)) {
      case DP_RSP_FIRMWARE_VERSION: {
        saveField(
          &packet, "Firmware Version",
          brl->data->firmwareVersion,
          sizeof(brl->data->firmwareVersion)
        );

        acknowledgeBrailleMessage(brl);
        continue;
      }

      case DP_RSP_DEVICE_NAME: {
        saveField(
          &packet, "Device Name",
          brl->data->deviceName,
          sizeof(brl->data->deviceName)
        );

        acknowledgeBrailleMessage(brl);
        continue;
      }

      case DP_RSP_DISPLAY_LINE: {
        unsigned char code = packet.fields.data[0];

        if (code != DP_DRC_ACK) {
          reportDisplayError(code);
          acknowledgeBrailleMessage(brl);
        }

        continue;
      }

      case DP_NTF_DISPLAY_LINE:
        acknowledgeBrailleMessage(brl);
        continue;

      case DP_NTF_KEYS_BASIC: {
        updateKeyGroup(
          brl, &packet, DP_GRP_BasicKeys,
          brl->data->keys.basic,
          sizeof(brl->data->keys.basic)
        );

        continue;
      }

      case DP_NTF_KEYS_PERKINS: {
        updateKeyGroup(
          brl, &packet, DP_GRP_PerkinsKeys,
          brl->data->keys.perkins,
          sizeof(brl->data->keys.perkins)
        );

        continue;
      }

      case DP_NTF_KEYS_ROUTING: {
        updateKeyGroup(
          brl, &packet, DP_GRP_RoutingKeys,
          brl->data->keys.routing,
          sizeof(brl->data->keys.routing)
        );

        continue;
      }

      case DP_NTF_KEYS_FUNCTION: {
        updateKeyGroup(
          brl, &packet, DP_GRP_FunctionKeys,
          brl->data->keys.function,
          sizeof(brl->data->keys.function)
        );

        continue;
      }

      case DP_NTF_ERROR: {
        reportRequestError(packet.fields.data[0]);
        acknowledgeBrailleMessage(brl);
        continue;
      }

      default:
        break;
    }

    logUnexpectedPacket(packet.bytes, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
