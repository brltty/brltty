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
#include "brldefs-sk.h"

BEGIN_KEY_NAME_TABLE(display)
  KEY_NAME_ENTRY(SK_BDP_K1, "K1"),
  KEY_NAME_ENTRY(SK_BDP_K2, "K2"),
  KEY_NAME_ENTRY(SK_BDP_K3, "K3"),
  KEY_NAME_ENTRY(SK_BDP_K4, "K4"),
  KEY_NAME_ENTRY(SK_BDP_K5, "K5"),
  KEY_NAME_ENTRY(SK_BDP_K6, "K6"),
  KEY_NAME_ENTRY(SK_BDP_K7, "K7"),
  KEY_NAME_ENTRY(SK_BDP_K8, "K8"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(notetaker)
  KEY_NAME_ENTRY(SK_NTK_Dot1, "Dot1"),
  KEY_NAME_ENTRY(SK_NTK_Dot2, "Dot2"),
  KEY_NAME_ENTRY(SK_NTK_Dot3, "Dot3"),
  KEY_NAME_ENTRY(SK_NTK_Dot4, "Dot4"),
  KEY_NAME_ENTRY(SK_NTK_Dot5, "Dot5"),
  KEY_NAME_ENTRY(SK_NTK_Dot6, "Dot6"),
  KEY_NAME_ENTRY(SK_NTK_Dot7, "Dot7"),
  KEY_NAME_ENTRY(SK_NTK_Dot8, "Dot8"),

  KEY_NAME_ENTRY(SK_NTK_Backspace, "Backspace"),
  KEY_NAME_ENTRY(SK_NTK_Space, "Space"),

  KEY_NAME_ENTRY(SK_NTK_LeftButton, "LeftButton"),
  KEY_NAME_ENTRY(SK_NTK_RightButton, "RightButton"),

  KEY_NAME_ENTRY(SK_NTK_LeftJoystickPress, "LeftJoystickPress"),
  KEY_NAME_ENTRY(SK_NTK_LeftJoystickLeft, "LeftJoystickLeft"),
  KEY_NAME_ENTRY(SK_NTK_LeftJoystickRight, "LeftJoystickRight"),
  KEY_NAME_ENTRY(SK_NTK_LeftJoystickUp, "LeftJoystickUp"),
  KEY_NAME_ENTRY(SK_NTK_LeftJoystickDown, "LeftJoystickDown"),

  KEY_NAME_ENTRY(SK_NTK_RightJoystickPress, "RightJoystickPress"),
  KEY_NAME_ENTRY(SK_NTK_RightJoystickLeft, "RightJoystickLeft"),
  KEY_NAME_ENTRY(SK_NTK_RightJoystickRight, "RightJoystickRight"),
  KEY_NAME_ENTRY(SK_NTK_RightJoystickUp, "RightJoystickUp"),
  KEY_NAME_ENTRY(SK_NTK_RightJoystickDown, "RightJoystickDown"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routing)
  KEY_SET_ENTRY(SK_SET_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(bdp)
  KEY_NAME_TABLE(display),
  KEY_NAME_TABLE(routing),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(ntk)
  KEY_NAME_TABLE(notetaker),
  KEY_NAME_TABLE(routing),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(bdp)
DEFINE_KEY_TABLE(ntk)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(bdp),
  &KEY_TABLE_DEFINITION(ntk),
END_KEY_TABLE_LIST

typedef enum {
  IPT_identity,
  IPT_keys,
  IPT_routing,
  IPT_combined
} InputPacketType;

typedef struct {
  unsigned char bytes[4 + 0XFF];
  unsigned char type;

  union {
    uint32_t keys;
    const unsigned char *routing;

    struct {
      uint32_t keys;
      const unsigned char *routing;
    } combined;

    struct {
      unsigned char cellCount;
      unsigned char keyCount;
      unsigned char routingCount;
    } identity;
  } fields;
} InputPacket;

typedef struct {
  const char *name;
  const KeyTableDefinition *keyTableDefinition;
  void (*initializeData) (void);
  int (*readPacket) (BrailleDisplay *brl, InputPacket *packet);
  BrailleRequestWriter *writeIdentifyRequest;
  int (*writeCells) (BrailleDisplay *brl);
} ProtocolOperations;

typedef struct {
  const ProtocolOperations *const *protocols;
} InputOutputOperations;

static GioEndpoint *gioEndpoint = NULL;
static const InputOutputOperations *io;
static const ProtocolOperations *protocol;

static unsigned char keyCount;
static unsigned char routingCount;

static int forceRewrite;
static unsigned char textCells[80];

static size_t
readPacket (BrailleDisplay *brl, void *packet, size_t size) {
  return protocol->readPacket(brl, packet);
}

static int
writePacket (BrailleDisplay *brl, const void *packet, size_t size) {
  return writeBraillePacket(brl, gioEndpoint, packet, size);
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const InputPacket *response = packet;

  return (response->type == IPT_identity)? BRL_RSP_DONE: BRL_RSP_UNEXPECTED;
}

typedef enum {
  TBT_ANY = 0X80,
  TBT_DECIMAL,
  TBT_SIZE,
  TBT_ID1,
  TBT_ID2,
  TBT_KEYS
} TemplateByteType;

typedef struct {
  const unsigned char *bytes;
  unsigned char length;
  unsigned char type;
} TemplateEntry;

#define TEMPLATE_ENTRY(name) { \
  .bytes = templateString_##name, \
  .length = sizeof(templateString_##name), \
  .type = IPT_##name \
}

static const unsigned char templateString_keys[] = {
  TBT_KEYS, TBT_KEYS
};
static const TemplateEntry templateEntry_keys = TEMPLATE_ENTRY(keys);

static int
ntvWriteCells0 (BrailleDisplay *brl) {
  return 1;
}

static int
ntvWriteCells40 (BrailleDisplay *brl) {
  static const unsigned char header[] = {
    0XFF, 0XFF,
    0X73, 0X65, 0X69, 0X6B, 0X61,
    0X00
  };

  unsigned char packet[sizeof(header) + (brl->textColumns*2)];
  unsigned char *byte = packet;

  byte = mempcpy(byte, header, sizeof(header));

  {
    unsigned int i;

    for (i=0; i<brl->textColumns; i+=1) {
      *byte++ = 0;
      *byte++ = translateOutputCell(textCells[i]);
    }
  }

  return writePacket(brl, packet, byte-packet);
}

static int
ntvWriteCells80 (BrailleDisplay *brl) {
  static const unsigned char header[] = {
    0XFF, 0XFF,
    0X73, 0X38, 0X30,
    0X00, 0X00, 0X00
  };

  unsigned char packet[sizeof(header) + brl->textColumns];
  unsigned char *byte = packet;

  byte = mempcpy(byte, header, sizeof(header));
  byte = translateOutputCells(byte, textCells, brl->textColumns);
  return writePacket(brl, packet, byte-packet);
}

typedef struct {
  int (*ntvWriteCells) (BrailleDisplay *brl);
  const TemplateEntry *routingTemplate;
} ModelEntry;

static const ModelEntry *bdpModel;

static int
bdpSetModel (unsigned char cellCount) {
  switch (cellCount) {
    case 0: {
      static const ModelEntry modelEntry = {
        .ntvWriteCells = ntvWriteCells0
      };

      bdpModel = &modelEntry;
      return 1;
    }

    case 40: {
      static const unsigned char templateString_routing[] = {
        0X00, 0X08, 0X09, 0X00, 0X00, 0X00, 0X00,
        TBT_ANY, TBT_ANY, TBT_ANY, TBT_ANY, TBT_ANY,
        0X00, 0X08, 0X09, 0X00, 0X00, 0X00, 0X00,
        0X00, 0X00, 0X00, 0X00, 0X00
      };
      static const TemplateEntry templateEntry_routing = TEMPLATE_ENTRY(routing);

      static const ModelEntry modelEntry = {
        .ntvWriteCells = ntvWriteCells40,
        .routingTemplate = &templateEntry_routing
      };

      bdpModel = &modelEntry;
      return 1;
    }

    case 80: {
      static const unsigned char templateString_routing[] = {
        0X00, 0X08, 0X0F, 0X00, 0X00, 0X00, 0X00,
        TBT_ANY, TBT_ANY, TBT_ANY, TBT_ANY, TBT_ANY,
        TBT_ANY, TBT_ANY, TBT_ANY, TBT_ANY, TBT_ANY,
        0X00, 0X00, 0X00, 0X00, 0X00
      };
      static const TemplateEntry templateEntry_routing = TEMPLATE_ENTRY(routing);

      static const ModelEntry modelEntry = {
        .ntvWriteCells = ntvWriteCells80,
        .routingTemplate = &templateEntry_routing
      };

      bdpModel = &modelEntry;
      return 1;
    }

    default:
      break;
  }

  return 0;
}

static void
bdpInitializeData (void) {
  bdpSetModel(0);
}

static int
bdpReadPacket (
  InputPacket *packet,
  const TemplateEntry *identityTemplate,
  const TemplateEntry *alternateTemplate,
  void (*interpretIdentity) (InputPacket *packet)
) {
  const TemplateEntry *const templateTable[] = {
    identityTemplate,
    bdpModel->routingTemplate,
    NULL
  };

  const TemplateEntry *template = NULL;
  unsigned int offset = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;

      if (!gioReadByte(gioEndpoint, &byte, started)) {
        if (started) logPartialPacket(packet->bytes, offset);
        return 0;
      }
    }

  gotByte:
    if (!offset) {
      const TemplateEntry *const *templateAddress = templateTable;

      while ((template = *templateAddress++))
        if (byte == *template->bytes)
          break;

      if (!template) {
        if ((byte & 0XE0) == 0X60) {
          template = &templateEntry_keys;
        } else {
          logIgnoredByte(byte);
          template = NULL;
          continue;
        }
      }
    } else {
      int unexpected = 0;

      switch (template->bytes[offset]) {
        case TBT_ANY:
          break;

        case TBT_DECIMAL:
          if ((byte < '0') || (byte > '9')) unexpected = 1;
          break;

        case TBT_SIZE:
          if ((byte != 40) && (byte != 80)) unexpected = 1;
          break;

        case TBT_ID1:
          if (!strchr("3458", byte)) unexpected = 1;
          break;

        case TBT_ID2:
//          if (byte != ((packet->bytes[offset] == '8')? '0': ' ')) unexpected = 1;
          if (!strchr("0 ", byte)) unexpected = 1;
          break;

        case TBT_KEYS:
          if ((byte & 0XE0) != 0XE0) unexpected = 1;
          break;

        default:
          if (byte != template->bytes[offset]) unexpected = 1;
          break;
      }

      if (unexpected) {
        if ((offset == 1) && (template->type == IPT_identity)) {
          template = alternateTemplate;
        } else {
          logShortPacket(packet->bytes, offset);
          offset = 0;
          template = NULL;
        }

        goto gotByte;
      }
    }

    packet->bytes[offset++] = byte;
    if (template && (offset == template->length)) {
      logInputPacket(packet->bytes, offset);

      switch ((packet->type = template->type)) {
        case IPT_identity:
          interpretIdentity(packet);
          bdpSetModel(packet->fields.identity.cellCount);
          break;

        case IPT_keys: {
          const unsigned char *byte = packet->bytes + offset;
          packet->fields.keys = 0;

          do {
            packet->fields.keys <<= 8;
            packet->fields.keys |= *--byte & 0X1F;
          } while (byte != packet->bytes);

          break;
        }

        case IPT_routing:
          packet->fields.routing = &packet->bytes[7];
          break;
      }

      return offset;
    }
  }
}

static void
pbcInterpretIdentity (InputPacket *packet) {
  packet->fields.identity.cellCount = packet->bytes[2];
  packet->fields.identity.keyCount = 8;
  packet->fields.identity.routingCount = packet->fields.identity.cellCount;
}

static int
pbcReadPacket (BrailleDisplay *brl, InputPacket *packet) {
  static const unsigned char templateString_identity[] = {
    0X00, 0X05, TBT_SIZE, 0X08,
    TBT_ANY, TBT_ANY, TBT_ANY, TBT_ANY,
    TBT_ANY, TBT_ANY, TBT_ANY, TBT_ANY
  };
  static const TemplateEntry identityTemplate = TEMPLATE_ENTRY(identity);

  return bdpReadPacket(packet, &identityTemplate, bdpModel->routingTemplate, pbcInterpretIdentity);
}

static int
pbcWriteIdentifyRequest (BrailleDisplay *brl) {
  static const unsigned char packet[] = {0XFF, 0XFF, 0X0A};
  return writePacket(brl, packet, sizeof(packet));
}

static int
pbcWriteCells (BrailleDisplay *brl) {
  static const unsigned char header[] = {
    0XFF, 0XFF, 0X04,
    0X00, 0X63, 0X00
  };

  unsigned char packet[sizeof(header) + 2 + (brl->textColumns * 2)];
  unsigned char *byte = packet;

  byte = mempcpy(byte, header, sizeof(header));
  *byte++ = brl->textColumns * 2;
  *byte++ = 0;

  {
    int i;
    for (i=0; i<brl->textColumns; i+=1) {
      *byte++ = 0;
      *byte++ = translateOutputCell(textCells[i]);
    }
  }

  return writePacket(brl, packet, byte-packet);
}

static const ProtocolOperations pbcProtocolOperations = {
  .name = "PowerBraille Compatibility",
  .keyTableDefinition = &KEY_TABLE_DEFINITION(bdp),
  .initializeData = bdpInitializeData,
  .readPacket = pbcReadPacket,
  .writeIdentifyRequest = pbcWriteIdentifyRequest,
  .writeCells = pbcWriteCells
};

static void
ntvInterpretIdentity (InputPacket *packet) {
  packet->fields.identity.cellCount = (packet->bytes[5] == '8')? 80: 40;
  packet->fields.identity.keyCount = 16;
  packet->fields.identity.routingCount = packet->fields.identity.cellCount;
}

static int
ntvReadPacket (BrailleDisplay *brl, InputPacket *packet) {
  static const unsigned char templateString_identity[] = {
    0X73, 0X65, 0X69, 0X6B, 0X61, TBT_ID1, TBT_ID2,
    0X76, TBT_DECIMAL, 0X2E, TBT_DECIMAL, TBT_DECIMAL
  };
  static const TemplateEntry identityTemplate = TEMPLATE_ENTRY(identity);

  return bdpReadPacket(packet, &identityTemplate, &templateEntry_keys, ntvInterpretIdentity);
}

static int
ntvWriteIdentifyRequest (BrailleDisplay *brl) {
  static const unsigned char packet[] = {0XFF, 0XFF, 0X1C};
  return writePacket(brl, packet, sizeof(packet));
}

static int
ntvWriteCells (BrailleDisplay *brl) {
  return bdpModel->ntvWriteCells(brl);
}

static const ProtocolOperations ntvProtocolOperations = {
  .name = "Seika Braille Display",
  .keyTableDefinition = &KEY_TABLE_DEFINITION(bdp),
  .initializeData = bdpInitializeData,
  .readPacket = ntvReadPacket,
  .writeIdentifyRequest = ntvWriteIdentifyRequest,
  .writeCells = ntvWriteCells
};

static void
ntkInitializeData (void) {
}

static int
ntkReadPacket (BrailleDisplay *brl, InputPacket *packet) {
  int offset = 0;
  unsigned int length = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;

      if (!gioReadByte(gioEndpoint, &byte, started)) {
        if (started) logPartialPacket(packet->bytes, offset);
        return 0;
      }
    }

  gotByte:
    {
      int unexpected = 0;

      switch (offset) {
        case 0:
          length = 4;
        case 1:
          if (byte != 0XFF) unexpected = 1;
          break;

        case 3:
          length += byte;
          break;

        default:
          break;
      }

      if (unexpected) {
        if (offset) {
          logShortPacket(packet->bytes, offset);
          offset = 0;
          length = 0;
          goto gotByte;
        }

        logIgnoredByte(byte);
        continue;
      }
    }

    packet->bytes[offset++] = byte;
    if (offset == length) {
      logInputPacket(packet->bytes, offset);

      switch (packet->bytes[2]) {
        case 0XA2:
          packet->type = IPT_identity;
          packet->fields.identity.cellCount = packet->bytes[5];
          packet->fields.identity.keyCount = packet->bytes[4];
          packet->fields.identity.routingCount = packet->bytes[6];
          break;

        case 0XA4:
          packet->type = IPT_routing;
          packet->fields.routing = &packet->bytes[4];
          break;

        {
          uint32_t *keys;
          const unsigned char *byte;

        case 0XA6:
          packet->type = IPT_keys;
          keys = &packet->fields.keys;
          byte = packet->bytes + offset;
          goto doKeys;

        case 0XA8:
          packet->type = IPT_combined;
          keys = &packet->fields.combined.keys;
          byte = packet->fields.combined.routing = packet->bytes +  4 + ((keyCount + 7) / 8);
          goto doKeys;

        doKeys:
          *keys = 0;

          while (--byte != &packet->bytes[3]) {
            *keys <<= 8;
            *keys |= *byte;
          }

          break;
        }

        default:
          logUnknownPacket(packet->bytes[2]);
          offset = 0;
          length = 0;
          continue;
      }

      return offset;
    }
  }
}

static int
ntkWriteIdentifyRequest (BrailleDisplay *brl) {
  static const unsigned char packet[] = {0XFF, 0XFF, 0XA1};
  return writePacket(brl, packet, sizeof(packet));
}

static int
ntkWriteCells (BrailleDisplay *brl) {
  static const unsigned char header[] = {0XFF, 0XFF, 0XA3};
  unsigned char packet[sizeof(header) + 1 + brl->textColumns];
  unsigned char *byte = packet;

  byte = mempcpy(byte, header, sizeof(header));
  *byte++ = brl->textColumns;
  byte = translateOutputCells(byte, textCells, brl->textColumns);

  return writePacket(brl, packet, byte-packet);
}

static const ProtocolOperations ntkProtocolOperations = {
  .name = "Seika Note Taker",
  .keyTableDefinition = &KEY_TABLE_DEFINITION(ntk),
  .initializeData = ntkInitializeData,
  .readPacket = ntkReadPacket,
  .writeIdentifyRequest = ntkWriteIdentifyRequest,
  .writeCells = ntkWriteCells
};

static const ProtocolOperations *const allProtocols[] = {
  &ntkProtocolOperations,
  &ntvProtocolOperations,
  &pbcProtocolOperations,
  NULL
};

static const ProtocolOperations *const nativeProtocols[] = {
  &ntkProtocolOperations,
  &ntvProtocolOperations,
  NULL
};

static const InputOutputOperations serialOperations = {
  .protocols = nativeProtocols
};

static const InputOutputOperations usbOperations = {
  .protocols = allProtocols
};

static const InputOutputOperations bluetoothOperations = {
  .protocols = nativeProtocols
};

static int
connectResource (const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 9600
  };

  static const UsbChannelDefinition usbChannelDefinitions[] = {
    { /* Braille Display */
      .vendor=0X10C4, .product=0XEA60,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=1,
      .serial=&serialParameters
    }
    ,
    { /* Note Taker */
      .vendor=0X10C4, .product=0XEA80,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1,
      .serial=&serialParameters
    }
    ,
    { .vendor=0 }
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;
  descriptor.serial.options.applicationData = &serialOperations;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;
  descriptor.usb.options.applicationData = &usbOperations;

  descriptor.bluetooth.channelNumber = 1;
  descriptor.bluetooth.options.applicationData = &bluetoothOperations;

  if ((gioEndpoint = gioConnectResource(identifier, &descriptor))) {
    io = gioGetApplicationData(gioEndpoint);
    return 1;
  }

  return 0;
}

static void
disconnectResource (void) {
  if (gioEndpoint) {
    gioDisconnectResource(gioEndpoint);
    gioEndpoint = NULL;
  }
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if (connectResource(device)) {
    const ProtocolOperations *const *protocolAddress = io->protocols;

    while ((protocol = *protocolAddress++)) {
      InputPacket response;

      logMessage(LOG_DEBUG, "trying protocol %s", protocol->name);
      protocol->initializeData();

      if (probeBrailleDisplay(brl, 2, gioEndpoint, 200,
                              protocol->writeIdentifyRequest,
                              readPacket, &response, sizeof(response.bytes),
                              isIdentityResponse)) {
        logMessage(LOG_DEBUG, "Seika Protocol: %s", protocol->name);
        logMessage(LOG_DEBUG, "Seika Size: %u", response.fields.identity.cellCount);

        brl->textColumns = response.fields.identity.cellCount;
        keyCount = response.fields.identity.keyCount;
        routingCount = response.fields.identity.routingCount;

        makeOutputTable(dotsTable_ISO11548_1);
  
        {
          const KeyTableDefinition *ktd = protocol->keyTableDefinition;
          brl->keyBindings = ktd->bindings;
          brl->keyNameTables = ktd->names;
        }

        forceRewrite = 1;;
        return 1;
      }
    }

    disconnectResource();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  disconnectResource();
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(textCells, brl->buffer, brl->textColumns, NULL, NULL, &forceRewrite)) {
    if (!protocol->writeCells(brl)) return 0;
  }

  return 1;
}

static void
processKeys (uint32_t keys, const unsigned char *routing) {
  KeyValue pressedKeys[keyCount + routingCount];
  unsigned int pressedCount = 0;

  if (keys) {
    uint32_t bit = UINT32_C(0X1);
    unsigned char key = 0;

    while (key < keyCount) {
      if (keys & bit) {
        KeyValue *kv = &pressedKeys[pressedCount++];
        enqueueKeyEvent((kv->set = SK_SET_NavigationKeys), (kv->key = key), 1);
        if (!(keys &= ~bit)) break;
      }

      bit <<= 1;
      key += 1;
    }
  }

  if (routing) {
    const unsigned char *byte = routing;
    unsigned char key = 0;

    while (key < routingCount) {
      if (*byte) {
        unsigned char bit = 0X1;

        do {
          if (*byte & bit) {
            KeyValue *kv = &pressedKeys[pressedCount++];
            enqueueKeyEvent((kv->set = SK_SET_RoutingKeys), (kv->key = key), 1);
          }

          key += 1;
        } while ((bit <<= 1));
      } else {
        key += 8;
      }

      byte += 1;
    }
  }

  while (pressedCount) {
    KeyValue *kv = &pressedKeys[--pressedCount];
    enqueueKeyEvent(kv->set, kv->key, 0);
  }
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  InputPacket packet;
  size_t length;

  while ((length = protocol->readPacket(brl, &packet))) {
    switch (packet.type) {
      case IPT_keys:
        processKeys(packet.fields.keys, NULL);
        continue;

      case IPT_routing:
        processKeys(0, packet.fields.routing);
        continue;

      case IPT_combined:
        processKeys(packet.fields.combined.keys, packet.fields.combined.routing);
        continue;

      default:
        break;
    }

    logUnexpectedPacket(packet.bytes, length);
  }
  if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;

  return EOF;
}
