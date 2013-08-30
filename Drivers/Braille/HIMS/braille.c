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
#include "brldefs-hm.h"

#define MAXIMUM_CELL_COUNT 40

BEGIN_KEY_NAME_TABLE(routing)
  KEY_SET_ENTRY(HM_SET_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(dots)
  KEY_NAME_ENTRY(HM_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(HM_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(HM_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(HM_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(HM_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(HM_KEY_Dot6, "Dot6"),
  KEY_NAME_ENTRY(HM_KEY_Dot7, "Dot7"),
  KEY_NAME_ENTRY(HM_KEY_Dot8, "Dot8"),
  KEY_NAME_ENTRY(HM_KEY_Space, "Space"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(f14)
  KEY_NAME_ENTRY(HM_KEY_F1, "F1"),
  KEY_NAME_ENTRY(HM_KEY_F2, "F2"),
  KEY_NAME_ENTRY(HM_KEY_F3, "F3"),
  KEY_NAME_ENTRY(HM_KEY_F4, "F4"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(bf)
  KEY_NAME_ENTRY(HM_KEY_Backward, "Backward"),
  KEY_NAME_ENTRY(HM_KEY_Forward, "Forward"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(sense)
  KEY_NAME_TABLE(routing),
  KEY_NAME_TABLE(dots),
  KEY_NAME_TABLE(f14),
  KEY_NAME_TABLE(bf),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLE(sync)
  KEY_NAME_ENTRY(HM_KEY_LeftUp, "LeftUp"),
  KEY_NAME_ENTRY(HM_KEY_LeftDown, "LeftDown"),
  KEY_NAME_ENTRY(HM_KEY_RightUp, "RightUp"),
  KEY_NAME_ENTRY(HM_KEY_RightDown, "RightDown"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(sync)
  KEY_NAME_TABLE(routing),
  KEY_NAME_TABLE(sync),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLE(edge)
  KEY_NAME_ENTRY(HM_KEY_LeftScrollUp, "LeftScrollUp"),
  KEY_NAME_ENTRY(HM_KEY_RightScrollUp, "RightScrollUp"),
  KEY_NAME_ENTRY(HM_KEY_LeftScrollDown, "LeftScrollDown"),
  KEY_NAME_ENTRY(HM_KEY_RightScrollDown, "RightScrollDown"),

  KEY_NAME_ENTRY(HM_KEY_F5, "F5"),
  KEY_NAME_ENTRY(HM_KEY_F6, "F6"),
  KEY_NAME_ENTRY(HM_KEY_F7, "F7"),
  KEY_NAME_ENTRY(HM_KEY_F8, "F8"),

  KEY_NAME_ENTRY(HM_KEY_LeftPadUp, "LeftPadUp"),
  KEY_NAME_ENTRY(HM_KEY_LeftPadDown, "LeftPadDown"),
  KEY_NAME_ENTRY(HM_KEY_LeftPadLeft, "LeftPadLeft"),
  KEY_NAME_ENTRY(HM_KEY_LeftPadRight, "LeftPadRight"),

  KEY_NAME_ENTRY(HM_KEY_RightPadUp, "RightPadUp"),
  KEY_NAME_ENTRY(HM_KEY_RightPadDown, "RightPadDown"),
  KEY_NAME_ENTRY(HM_KEY_RightPadLeft, "RightPadLeft"),
  KEY_NAME_ENTRY(HM_KEY_RightPadRight, "RightPadRight"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(edge)
  KEY_NAME_TABLE(routing),
  KEY_NAME_TABLE(dots),
  KEY_NAME_TABLE(f14),
  KEY_NAME_TABLE(edge),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(sense)
DEFINE_KEY_TABLE(sync)
DEFINE_KEY_TABLE(edge)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(sense),
  &KEY_TABLE_DEFINITION(sync),
  &KEY_TABLE_DEFINITION(edge),
END_KEY_TABLE_LIST

typedef enum {
  IPT_CURSOR  = 0X00,
  IPT_KEYS    = 0X01,
  IPT_CELLS   = 0X02
} InputPacketType;

typedef union {
  unsigned char bytes[10];

  struct {
    unsigned char start;
    unsigned char type;
    unsigned char count;
    unsigned char data;
    unsigned char reserved[4];
    unsigned char checksum;
    unsigned char end;
  } PACKED data;
} PACKED InputPacket;

typedef struct {
  const char *modelName;
  const char *resourceNamePrefix;
  const KeyTableDefinition *keyTableDefinition;
  int (*getDefaultCellCount) (BrailleDisplay *brl, unsigned int *count);
} ProtocolEntry;

struct BrailleDataStruct {
  GioEndpoint *gioEndpoint;
  const ProtocolEntry *protocol;
  unsigned char previousCells[MAXIMUM_CELL_COUNT];
};

static size_t
readPacket (BrailleDisplay *brl, InputPacket *packet) {
  const size_t length = 10;
  size_t offset = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;
      if (!gioReadByte(brl->data->gioEndpoint, &byte, started)) {
        if (started) logPartialPacket(packet->bytes, offset);
        return 0;
      }
    }

    if (!offset) {
      if (byte != 0XFA) {
        logIgnoredByte(byte);
        continue;
      }
    }

    packet->bytes[offset++] = byte;
    if (offset == length) {
      if (byte == 0XFB) {
        {
          int checksum = -packet->data.checksum;

          {
            size_t i;
            for (i=0; i<offset; i+=1) checksum += packet->bytes[i];
          }

          if ((checksum & 0XFF) != packet->data.checksum) {
            logInputProblem("Incorrect Input Checksum", packet->bytes, offset);
          }
        }

        logInputPacket(packet->bytes, offset);
        return length;
      }

      {
        const unsigned char *start = memchr(packet->bytes+1, packet->bytes[0], offset-1);
        const unsigned char *end = packet->bytes + offset;
        if (!start) start = end;
        logDiscardedBytes(packet->bytes, start-packet->bytes);
        memmove(packet->bytes, start, (offset = end - start));
      }
    }
  }
}

static size_t
readBytes (BrailleDisplay *brl, void *packet, size_t size) {
  return readPacket(brl, packet);
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, size_t count) {
  return writeBraillePacket(brl, brl->data->gioEndpoint, bytes, count);
}

static int
writePacket (
  BrailleDisplay *brl,
  unsigned char type, unsigned char mode,
  const unsigned char *data1, size_t length1,
  const unsigned char *data2, size_t length2
) {
  unsigned char packet[2 + 1 + 1 + 2 + length1 + 1 + 1 + 2 + length2 + 1 + 4 + 1 + 2];
  unsigned char *byte = packet;
  unsigned char *checksum;

  /* DS */
  *byte++ = type;
  *byte++ = type;

  /* M */
  *byte++ = mode;

  /* DS1 */
  *byte++ = 0XF0;

  /* Cnt1 */
  *byte++ = (length1 >> 0) & 0XFF;
  *byte++ = (length1 >> 8) & 0XFF;

  /* D1 */
  byte = mempcpy(byte, data1, length1);

  /* DE1 */
  *byte++ = 0XF1;

  /* DS2 */
  *byte++ = 0XF2;

  /* Cnt2 */
  *byte++ = (length2 >> 0) & 0XFF;
  *byte++ = (length2 >> 8) & 0XFF;

  /* D2 */
  byte = mempcpy(byte, data2, length2);

  /* DE2 */
  *byte++ = 0XF3;

  /* Reserved */
  {
    int count = 4;
    while (count--) *byte++ = 0;
  }

  /* Chk */
  *(checksum = byte++) = 0;

  /* DE */
  *byte++ = 0XFD;
  *byte++ = 0XFD;

  {
    unsigned char sum = 0;
    const unsigned char *ptr = packet;
    while (ptr != byte) sum += *ptr++;
    *checksum = sum;
  }

  return writeBytes(brl, packet, byte - packet);
}


static int
getBrailleSenseDefaultCellCount (BrailleDisplay *brl, unsigned int *count) {
  *count = 32;
  return 1;
}

static const ProtocolEntry brailleSenseProtocol = {
  .modelName = "Braille Sense",
  .keyTableDefinition = &KEY_TABLE_DEFINITION(sense),
  .getDefaultCellCount = getBrailleSenseDefaultCellCount
};


static int
getSyncBrailleDefaultCellCount (BrailleDisplay *brl, unsigned int *count) {
  return 0;
}

static const ProtocolEntry syncBrailleProtocol = {
  .modelName = "SyncBraille",
  .keyTableDefinition = &KEY_TABLE_DEFINITION(sync),
  .getDefaultCellCount = getSyncBrailleDefaultCellCount
};


static int
getBrailleEdgeDefaultCellCount (BrailleDisplay *brl, unsigned int *count) {
  *count = 40;
  return 1;
}

static const ProtocolEntry brailleEdgeProtocol = {
  .modelName = "Braille Edge",
  .resourceNamePrefix = "BrailleEDGE",
  .keyTableDefinition = &KEY_TABLE_DEFINITION(edge),
  .getDefaultCellCount = getBrailleEdgeDefaultCellCount
};


static const ProtocolEntry *protocolTable[] = {
  &brailleSenseProtocol,
  &syncBrailleProtocol,
  &brailleEdgeProtocol,
  NULL
};


static int
writeCells (BrailleDisplay *brl) {
  const size_t count = MIN(brl->textColumns*brl->textRows, MAXIMUM_CELL_COUNT);
  unsigned char cells[count];

  translateOutputCells(cells, brl->data->previousCells, count);
  return writePacket(brl, 0XFC, 0X01, cells, count, NULL, 0);
}

static int
clearCells (BrailleDisplay *brl) {
  memset(brl->data->previousCells, 0, MIN(brl->textColumns*brl->textRows, MAXIMUM_CELL_COUNT));
  return writeCells(brl);
}

static int
writeCellCountRequest (BrailleDisplay *brl) {
  static const unsigned char data[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  return writePacket(brl, 0XFB, 0X01, data, sizeof(data), NULL, 0);
}

static BrailleResponseResult
isCellCountResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const InputPacket *response = packet;

  return (response->data.type == IPT_CELLS)? BRL_RSP_DONE: BRL_RSP_UNEXPECTED;
}

static int
getCellCount (BrailleDisplay *brl, unsigned int *count) {
  InputPacket response;

  if (probeBrailleDisplay(brl, 2, brl->data->gioEndpoint, 1000,
                          writeCellCountRequest,
                          readBytes, &response, sizeof(response.bytes),
                          isCellCountResponse)) {
    *count = response.data.data;
    return 1;
  }

  return brl->data->protocol->getDefaultCellCount(brl, count);
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 115200
  };

  static const UsbChannelDefinition usbChannelDefinitions[] = {
    { /* Braille Sense (USB 1.1) */
      .version = UsbSpecificationVersion_1_1,
      .vendor=0X045E, .product=0X930A,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1,
      .data=&brailleSenseProtocol
    },

    { /* Braille Sense (USB 2.0) */
      .version = UsbSpecificationVersion_2_0,
      .vendor=0X045E, .product=0X930A,
      .configuration=1, .interface=1, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1,
      .data=&brailleSenseProtocol
    },

    { /* Braille Sense U2 (USB 2.0) */
      .version = UsbSpecificationVersion_2_0,
      .vendor=0X045E, .product=0X930A,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1,
      .data=&brailleSenseProtocol
    },

    { /* Sync Braille */
      .vendor=0X0403, .product=0X6001,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .data=&syncBrailleProtocol
    },

    { /* Braille Edge */
      .vendor=0X045E, .product=0X930B,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1,
      .data=&brailleEdgeProtocol
    },

    { .vendor=0 }
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;
  descriptor.serial.options.applicationData = &brailleSenseProtocol;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  descriptor.bluetooth.channelNumber = 4;

  if ((brl->data->gioEndpoint = gioConnectResource(identifier, &descriptor))) {
    return 1;
  }

  return 0;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));
    brl->data->gioEndpoint = NULL;

    if (connectResource(brl, device)) {
      if (!(brl->data->protocol = gioGetApplicationData(brl->data->gioEndpoint))) {
        char *name = gioGetResourceName(brl->data->gioEndpoint);
        brl->data->protocol = &brailleSenseProtocol;

        if (name) {
          const ProtocolEntry *const *protocolAddress = protocolTable;

          while (*protocolAddress) {
            const ProtocolEntry *protocol = *protocolAddress;
            const char *prefix = protocol->resourceNamePrefix;

            if (prefix) {
              if (strncasecmp(name, prefix, strlen(prefix)) == 0) {
                brl->data->protocol = protocol;
                break;
              }
            }

            protocolAddress += 1;
          }

          free(name);
        }
      }

      logMessage(LOG_INFO, "detected: %s", brl->data->protocol->modelName);

      if (getCellCount(brl, &brl->textColumns)) {
        brl->textRows = 1;
        brl->keyBindings = brl->data->protocol->keyTableDefinition->bindings;
        brl->keyNameTables = brl->data->protocol->keyTableDefinition->names;

        makeOutputTable(dotsTable_ISO11548_1);
  
        if (clearCells(brl)) return 1;
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
  size_t count = brl->textColumns * brl->textRows;

  if (cellsHaveChanged(brl->data->previousCells, brl->buffer, count, NULL, NULL, NULL)) {
    if (!writeCells(brl)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  InputPacket packet;
  int length;

  while ((length = readPacket(brl, &packet))) {
    switch (packet.data.type) {
      case IPT_CURSOR: {
        unsigned char key = packet.data.data;

        enqueueKey(HM_SET_RoutingKeys, key);
        continue;
      }

      case IPT_KEYS: {
        uint32_t bits = (packet.data.reserved[0] << 0X00)
                      | (packet.data.reserved[1] << 0X08)
                      | (packet.data.reserved[2] << 0X10)
                      | (packet.data.reserved[3] << 0X18);

        enqueueKeys(bits, HM_SET_NavigationKeys, 0);
        continue;
      }

      default:
        break;
    }

    logUnexpectedPacket(&packet, length);
  }
  if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;

  return EOF;
}
