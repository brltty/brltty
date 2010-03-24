/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

BEGIN_KEY_NAME_TABLE(all)
  KEY_NAME_ENTRY(SK_KEY_K1, "K1"),
  KEY_NAME_ENTRY(SK_KEY_K2, "K2"),
  KEY_NAME_ENTRY(SK_KEY_K3, "K3"),
  KEY_NAME_ENTRY(SK_KEY_K4, "K4"),
  KEY_NAME_ENTRY(SK_KEY_K5, "K5"),
  KEY_NAME_ENTRY(SK_KEY_K6, "K6"),
  KEY_NAME_ENTRY(SK_KEY_K7, "K7"),
  KEY_NAME_ENTRY(SK_KEY_K8, "K8"),

  KEY_SET_ENTRY(SK_SET_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(all),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

typedef enum {
  IPT_identity,
  IPT_keys,
  IPT_routing
} InputPacketType;

typedef struct {
  unsigned char bytes[24];
  unsigned char type;

  union {
    uint16_t keys;
    const unsigned char *routing;

    struct {
      unsigned int version;
      unsigned char model;
      unsigned char size;
    } identity;
  } fields;
} InputPacket;

typedef struct {
  const char *name;
  int (*readPacket) (BrailleDisplay *brl, InputPacket *packet);
  int (*probeDisplay) (BrailleDisplay *brl, InputPacket *response);
  int (*writeCells) (BrailleDisplay *brl);
} ProtocolOperations;
static const ProtocolOperations *protocol;

typedef struct {
  const ProtocolOperations *const *protocols;
  int (*openPort) (const char *device);
  void (*closePort) ();
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, int length, int wait);
  int (*writeBytes) (const unsigned char *buffer, int length);
} InputOutputOperations;
static const InputOutputOperations *io;

#define SERIAL_BAUD 9600
static const int charactersPerSecond = SERIAL_BAUD / 10;

static TranslationTable outputTable;
static unsigned char textCells[40];

static int
readByte (unsigned char *byte, int wait) {
  int count = io->readBytes(byte, 1, wait);
  if (count > 0) return 1;

  if (count == 0) errno = EAGAIN;
  return 0;
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *buffer, size_t length) {
  logOutputPacket(buffer, length);
  if (io->writeBytes(buffer, length) == -1) return 0;
  brl->writeDelay += (length * 1000 / charactersPerSecond) + 1;
  return 1;
}

static int
probeDisplay (
  BrailleDisplay *brl, InputPacket *response,
  const unsigned char *requestAddress, size_t requestSize
) {
  int probeCount = 0;

  do {
    if (!writeBytes(brl, requestAddress, requestSize)) break;

    while (io->awaitInput(200)) {
      if (!protocol->readPacket(NULL, response)) break;
      if (response->type == IPT_identity) return 1;
    }
    if (errno != EAGAIN) break;
  } while (++probeCount < 3);

  return 0;
}

typedef enum {
  TBT_ANY = 0X80,
  TBT_DECIMAL,
  TBT_KEYS
} TemplateByteType;

typedef struct {
  const unsigned char *bytes;
  unsigned char length;
  unsigned char type;
} TemplateEntry;

#define TEMPLATE_ENTRY(name) { \
  .bytes=templateString_##name, \
  .length=sizeof(templateString_##name), \
  .type=IPT_##name \
}

static const unsigned char templateString_keys[] = {
  TBT_KEYS, TBT_KEYS
};
static const TemplateEntry templateEntry_keys = TEMPLATE_ENTRY(keys);

static const unsigned char templateString_routing[] = {
  0X00, 0X08, 0X09, 0X00, 0X00, 0X00, 0X00,
  TBT_ANY, TBT_ANY, TBT_ANY, TBT_ANY, TBT_ANY,
  0X00, 0X08, 0X09, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00
};
static const TemplateEntry templateEntry_routing = TEMPLATE_ENTRY(routing);

static int
readPacket_old (
  InputPacket *packet,
  const TemplateEntry *identityTemplate,
  const TemplateEntry *alternateTemplate,
  void (*interpretIdentity) (InputPacket *packet)
) {
  const TemplateEntry *const templateTable[] = {
    identityTemplate,
    &templateEntry_routing,
    NULL
  };

  const TemplateEntry *template = NULL;
  int offset = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;
      if (!readByte(&byte, started)) {
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
interpretIdentity_330 (InputPacket *packet) {
  packet->fields.identity.version = ((packet->bytes[5] - '0') << (4 * 2)) |
                                    ((packet->bytes[7] - '0') << (4 * 1));
  packet->fields.identity.model = 0;
  packet->fields.identity.size = packet->bytes[2];
}

static int
readPacket_330 (BrailleDisplay *brl, InputPacket *packet) {
  static const unsigned char templateString_identity[] = {
    0X00, 0X05, 0X28, 0X08,
    0X76, TBT_DECIMAL, 0X2E, TBT_DECIMAL,
    0X01, 0X01, 0X01, 0X01
  };
  static const TemplateEntry identityTemplate = TEMPLATE_ENTRY(identity);

  return readPacket_old(packet, &identityTemplate, &templateEntry_routing, interpretIdentity_330);
}

static int
probeDisplay_330 (BrailleDisplay *brl, InputPacket *response) {
  static const unsigned char request[] = {0XFF, 0XFF, 0X0A};
  return probeDisplay(brl, response, request, sizeof(request));
}

static int
writeCells_330 (BrailleDisplay *brl) {
  static const unsigned char header[] = {
    0XFF, 0XFF, 0X04,
    0X00, 0X63, 0X00
  };

  unsigned char packet[sizeof(header) + 2 + (brl->textColumns * 2)];
  unsigned char *byte = packet;

  memcpy(byte, header, sizeof(header));
  byte += sizeof(header);

  *byte++ = brl->textColumns * 2;
  *byte++ = 0;

  {
    int i;
    for (i=0; i<brl->textColumns; i+=1) {
      *byte++ = 0;
      *byte++ = outputTable[textCells[i]];
    }
  }

  return writeBytes(brl, packet, byte-packet);
}

static const ProtocolOperations protocolOperations_330 = {
  "V3.30",
  readPacket_330,
  probeDisplay_330,
  writeCells_330
};

static void
interpretIdentity_331 (InputPacket *packet) {
  packet->fields.identity.version = ((packet->bytes[8] - '0') << (4 * 2)) |
                                    ((packet->bytes[10] - '0') << (4 * 1)) |
                                    ((packet->bytes[11] - '0') << (4 * 0));
  packet->fields.identity.model = packet->bytes[5] - '0';
  packet->fields.identity.size = 40;
}

static int
readPacket_331 (BrailleDisplay *brl, InputPacket *packet) {
  static const unsigned char templateString_identity[] = {
    0X73, 0X65, 0X69, 0X6B, 0X61, TBT_DECIMAL,
    0X20, 0X76, TBT_DECIMAL, 0X2E, TBT_DECIMAL, TBT_DECIMAL
  };
  static const TemplateEntry identityTemplate = TEMPLATE_ENTRY(identity);

  return readPacket_old(packet, &identityTemplate, &templateEntry_keys, interpretIdentity_331);
}

static int
probeDisplay_331 (BrailleDisplay *brl, InputPacket *response) {
  static const unsigned char request[] = {0XFF, 0XFF, 0X1C};
  return probeDisplay(brl, response, request, sizeof(request));
}

static int
writeCells_331 (BrailleDisplay *brl) {
  static const unsigned char header[] = {
    0XFF, 0XFF,
    0X73, 0X65, 0X69, 0X6B, 0X61,
    0X00
  };

  unsigned char packet[sizeof(header) + (brl->textColumns * 2)];
  unsigned char *byte = packet;

  memcpy(byte, header, sizeof(header));
  byte += sizeof(header);

  {
    int i;
    for (i=0; i<brl->textColumns; i+=1) {
      *byte++ = 0;
      *byte++ = outputTable[textCells[i]];
    }
  }

  return writeBytes(brl, packet, byte-packet);
}

static const ProtocolOperations protocolOperations_331 = {
  "V3.31",
  readPacket_331,
  probeDisplay_331,
  writeCells_331
};

static inline int
routingBytes_332 (BrailleDisplay *brl) {
  return (brl->textColumns + 7) / 8;
}

static int
readPacket_332 (BrailleDisplay *brl, InputPacket *packet) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;
      if (!readByte(&byte, started)) {
        if (started) logPartialPacket(packet->bytes, offset);
        return 0;
      }
    }

  gotByte:
    if (!offset) {
      switch (byte) {
        case 0X1D:
          length = 2;
          break;

        default:
          logIgnoredByte(byte);
          continue;
      }
    } else {
      int unexpected = 0;

      if (offset == 1) {
        switch (byte) {
          case 0X02:
            packet->type = IPT_identity;
            length = 10;
            break;

          case 0X04:
            packet->type = IPT_routing;
            length = 2;
            if (brl) length += routingBytes_332(brl);
            break;

          case 0X05:
            packet->type = IPT_keys;
            length = 4;
            break;

          default:
            unexpected = 1;
            break;
        }
      }

      if (unexpected) {
        logShortPacket(packet->bytes, offset);
        offset = 0;
        length = 0;
        goto gotByte;
      }
    }

    packet->bytes[offset++] = byte;
    if (offset == length) {
      logInputPacket(packet->bytes, offset);

      switch (packet->type) {
        case IPT_identity:
          packet->fields.identity.version = 0;
          packet->fields.identity.model = packet->bytes[9] - '0';
          packet->fields.identity.size = packet->bytes[3];
          break;

        case IPT_keys: {
          const unsigned char *byte = packet->bytes + offset;
          packet->fields.keys = 0;

          do {
            packet->fields.keys <<= 8;
            packet->fields.keys |= *--byte;
          } while (byte != packet->bytes);

          break;
        }

        case IPT_routing:
          packet->fields.routing = &packet->bytes[2];
          break;
      }

      return offset;
    }
  }
}

static int
probeDisplay_332 (BrailleDisplay *brl, InputPacket *response) {
  static const unsigned char request[] = {0XFF, 0XFF, 0X1D, 0X01};
  return probeDisplay(brl, response, request, sizeof(request));
}

static int
writeCells_332 (BrailleDisplay *brl) {
  static const unsigned char header[] = {0XFF, 0XFF, 0X1D, 0X03};
  unsigned char packet[sizeof(header) + 1 + brl->textColumns];
  unsigned char *byte = packet;

  memcpy(byte, header, sizeof(header));
  byte += sizeof(header);

  *byte++ = brl->textColumns;

  {
    int i;
    for (i=0; i<brl->textColumns; i+=1) *byte++ = outputTable[textCells[i]];
  }

  return writeBytes(brl, packet, byte-packet);
}

static const ProtocolOperations protocolOperations_332 = {
  "V3.32",
  readPacket_332,
  probeDisplay_332,
  writeCells_332
};

static const ProtocolOperations *const allProtocols[] = {
  &protocolOperations_332,
  &protocolOperations_331,
  &protocolOperations_330,
  NULL
};

static const ProtocolOperations *const nativeProtocols[] = {
  &protocolOperations_332,
  &protocolOperations_331,
  NULL
};

/* Serial IO */
#include "io_serial.h"

static SerialDevice *serialDevice = NULL;

static int
openSerialPort (const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, SERIAL_BAUD)) return 1;

    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }

  return 0;
}

static void
closeSerialPort (void) {
  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
}

static int
awaitSerialInput (int milliseconds) {
  return serialAwaitInput(serialDevice, milliseconds);
}

static int
readSerialBytes (unsigned char *buffer, int count, int wait) {
  const int timeout = 100;
  return serialReadData(serialDevice, buffer, count,
                        (wait? timeout: 0), timeout);
}

static int
writeSerialBytes (const unsigned char *buffer, int length) {
  return serialWriteData(serialDevice, buffer, length);
}

static const InputOutputOperations serialOperations = {
  nativeProtocols,
  openSerialPort, closeSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialBytes
};

/* USB IO */
#include "io_usb.h"

static UsbChannel *usbChannel = NULL;

static int
openUsbPort (const char *device) {
  static const SerialParameters serial = {
    .baud = SERIAL_BAUD,
    .flow = SERIAL_FLOW_NONE,
    .data = 8,
    .stop = 1,
    .parity = SERIAL_PARITY_NONE
  };

  static const UsbChannelDefinition definitions[] = {
    { /* Seika */
      .vendor=0X10C4, .product=0XEA60,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=1,
      .serial=&serial
    }
    ,
    { .vendor=0 }
  };

  if ((usbChannel = usbFindChannel(definitions, (void *)device))) {
    return 1;
  }
  return 0;
}

static void
closeUsbPort (void) {
  if (usbChannel) {
    usbCloseChannel(usbChannel);
    usbChannel = NULL;
  }
}

static int
awaitUsbInput (int milliseconds) {
  return usbAwaitInput(usbChannel->device,
                       usbChannel->definition.inputEndpoint,
                       milliseconds);
}

static int
readUsbBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  int count = usbReapInput(usbChannel->device,
                           usbChannel->definition.inputEndpoint,
                           buffer, length,
                           (wait? timeout: 0), timeout);
  if (count != -1) return count;
  if (errno == EAGAIN) return 0;
  return -1;
}

static int
writeUsbBytes (const unsigned char *buffer, int length) {
  return usbWriteEndpoint(usbChannel->device,
                          usbChannel->definition.outputEndpoint,
                          buffer, length, 1000);
}

static const InputOutputOperations usbOperations = {
  allProtocols,
  openUsbPort, closeUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes
};

/* Bluetooth IO */
#include "io_bluetooth.h"
#include "io_misc.h"

static int bluetoothConnection = -1;

static int
openBluetoothPort (const char *device) {
  if ((bluetoothConnection = btOpenConnection(device, 1, 0)) == -1) return 0;
  return 1;
}

static void
closeBluetoothPort (void) {
  if (bluetoothConnection != -1) {
    close(bluetoothConnection);
    bluetoothConnection = -1;
  }
}

static int
awaitBluetoothInput (int milliseconds) {
  return awaitInput(bluetoothConnection, milliseconds);
}

static int
readBluetoothBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  size_t offset = 0;
  return readChunk(bluetoothConnection,
                   buffer, &offset, length,
                   (wait? timeout: 0), timeout);
}

static int
writeBluetoothBytes (const unsigned char *buffer, int length) {
  int count = writeData(bluetoothConnection, buffer, length);
  if (count != length) {
    if (count == -1) {
      LogError("Bluetooth write");
    } else {
      LogPrint(LOG_WARNING, "Trunccated bluetooth write: %d < %d", count, length);
    }
  }
  return count;
}

static const InputOutputOperations bluetoothOperations = {
  nativeProtocols,
  openBluetoothPort, closeBluetoothPort,
  awaitBluetoothInput, readBluetoothBytes, writeBluetoothBytes
};

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
  }
  
  if (isSerialDevice(&device)) {
    io = &serialOperations;
  } else if (isUsbDevice(&device)) {
    io = &usbOperations;
  } else if (isBluetoothDevice(&device)) {
    io = &bluetoothOperations;
  } else {
    unsupportedDevice(device);
    return 0;
  }

  if (io->openPort(device)) {
    const ProtocolOperations *const *protocolAddress = io->protocols;

    while ((protocol = *protocolAddress++)) {
      InputPacket response;

      LogPrint(LOG_DEBUG, "trying protocol %s", protocol->name);
      if (protocol->probeDisplay(brl, &response)) {
        LogPrint(LOG_DEBUG, "Seika Protocol: %s", protocol->name);
        LogPrint(LOG_DEBUG, "Seika Model: %u", response.fields.identity.model);
        LogPrint(LOG_DEBUG, "Seika Size: %u", response.fields.identity.size);

        brl->textColumns = response.fields.identity.size;
        brl->textRows = 1;

        {
          const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

          brl->keyBindings = ktd->bindings;
          brl->keyNameTables = ktd->names;
        }

        memset(textCells, 0XFF, brl->textColumns);
        return 1;
      }
    }

    io->closePort();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  io->closePort();
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (memcmp(textCells, brl->buffer, brl->textColumns) != 0) {
    memcpy(textCells, brl->buffer, brl->textColumns);
    if (!protocol->writeCells(brl)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  InputPacket packet;
  size_t length;

nextPacket:
  while ((length = protocol->readPacket(brl, &packet))) {
    switch (packet.type) {
      case IPT_keys: {
        uint16_t bit = 0X1;
        unsigned char key = 0;

        unsigned char keys[0X10];
        unsigned int keyCount = 0;

        while (++key <= 0X10) {
          if (packet.fields.keys & bit) {
            enqueueKeyEvent(SK_SET_NavigationKeys, key, 1);
            keys[keyCount++] = key;
          }

          bit <<= 1;
        }

        if (keyCount) {
          do {
            enqueueKeyEvent(SK_SET_NavigationKeys, keys[--keyCount], 0);
          } while (keyCount);

          continue;
        }

        break;
      }

      case IPT_routing: {
        const unsigned char *byte = packet.fields.routing;
        unsigned char key = 0;

        while (key < brl->textColumns) {
          if (*byte) {
            unsigned char bit = 0X1;

            do {
              if (*byte & bit) {
                enqueueKeyEvent(SK_SET_RoutingKeys, key, 1);
                enqueueKeyEvent(SK_SET_RoutingKeys, key, 0);
                goto nextPacket;
              }

              key += 1;
            } while ((bit <<= 1));

            break;
          } else {
            key += 8;
          }

          byte += 1;
        }

        break;
      }

      default:
        break;
    }

    logUnexpectedPacket(packet.bytes, length);
  }
  if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;

  return EOF;
}
