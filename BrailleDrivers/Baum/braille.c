/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "Programs/misc.h"
#include "Programs/iodefs.h"

#define BRL_HAVE_PACKET_IO
#include "Programs/brl_driver.h"

/* Global Definitions */

static int logInputPackets = 0;
static int logOutputPackets = 0;

#define BITS_PER_SECOND 19200
#define BYTES_PER_SECOND (BITS_PER_SECOND / 10)

#define MAXIMUM_CELLS 85
#define MAXIMUM_ROUTING_BYTES ((MAXIMUM_CELLS + 7) / 8)

static int cellCount;
static int cellsUpdated;
static unsigned char internalCells[MAXIMUM_CELLS];
static unsigned char externalCells[MAXIMUM_CELLS];
static TranslationTable outputTable;

typedef struct {
  unsigned int function;
  unsigned char routing[MAXIMUM_CELLS];
} Keys;
static Keys activeKeys;
static Keys pressedKeys;

typedef struct {
  int (*openPort) (char **parameters, const char *device);
  void (*closePort) ();
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, int length, int wait);
  int (*writeBytes) (const unsigned char *buffer, int length);
} InputOutputOperations;
static const InputOutputOperations *io;

typedef struct {
  int serialBaud;
  SerialParity serialParity;
  int (*readPacket) (unsigned char *packet, int size);
  int (*writePacket) (BrailleDisplay *brl, const unsigned char *packet, int length);
  int (*identifyDisplay) (BrailleDisplay *brl);
  int (*updateKeys) (BrailleDisplay *brl, int *keyPressed);
  int (*writeCells) (BrailleDisplay *brl);
} ProtocolOperations;
static const ProtocolOperations *protocol;

/* Internal Routines */

static int
setFunctionKeys (unsigned int mask, unsigned int keys, int *pressed) {
  keys |= pressedKeys.function & ~mask;
  if (keys == pressedKeys.function) return 0;

  if (keys & ~pressedKeys.function) *pressed = 1;
  pressedKeys.function = keys;
  return 1;
}

static int
setRoutingKey (int number, int press, int *pressed) {
  unsigned char *state = &pressedKeys.routing[number];
  if (!press == !*state) return 0;

  if ((*state = !!press)) *pressed = 1;
  return 1;
}

static int
readByte (unsigned char *byte, int wait) {
  int count = io->readBytes(byte, 1, wait);
  if (count > 0) return 1;

  if (count == 0) errno = EAGAIN;
  return 0;
}

static int
flushInput (void) {
  unsigned char byte;
  while (readByte(&byte, 0));
  return errno == EAGAIN;
}

static int
updateCells (BrailleDisplay *brl) {
  if (cellsUpdated) {
    if (!protocol->writeCells(brl)) return 0;
    cellsUpdated = 0;
  }
  return 1;
}

static void
translateCells (int start, int count) {
  while (count-- > 0) {
    externalCells[start] = outputTable[internalCells[start]];
    ++start;
    cellsUpdated = 1;
  }
}

static void
clearCells (int start, int count) {
  memset(&internalCells[start], 0, count);
  translateCells(start, count);
}

static void
logCellCount (void) {
  LogPrint(LOG_INFO, "Cell Count: %d", cellCount);
}

static void
changeCellCount (BrailleDisplay *brl, int count) {
  if (count != cellCount) {
    if (count > cellCount) {
      clearCells(cellCount, count-cellCount);

      {
        int number;
        for (number=cellCount; number<count; ++number) {
          int pressed;
          setRoutingKey(number, 0, &pressed);
        }
      }
    }

    cellCount = count;
    logCellCount();

    brl->x = cellCount;
    brl->resizeRequired = 1;
  }
}

static void
adjustWriteDelay (BrailleDisplay *brl, int bytes) {
  brl->writeDelay += bytes * 1000 / BYTES_PER_SECOND;
}

/* Serial IO */
#include "Programs/serial.h"

static SerialDevice *serialDevice = NULL;

static int
openSerialPort (char **parameters, const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, protocol->serialBaud)) {
      if (serialSetParity(serialDevice, protocol->serialParity)) {
        return 1;
      }
    }

    io->closePort(serialDevice);
    serialDevice = NULL;
  }

  return 0;
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

static void
closeSerialPort (void) {
  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
}

static const InputOutputOperations serialOperations = {
  openSerialPort, closeSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialBytes
};

#ifdef ENABLE_USB_SUPPORT
/* USB IO */
#include "Programs/usb.h"

static UsbChannel *usb = NULL;

static int
openUsbPort (char **parameters, const char *device) {
  static UsbChannelDefinition definitions[] = {
    {0X0403, 0XFE71, 1, 0, 0, 1, 2, 0, 0, 8, 1}, /* 24 cells */
    {0X0403, 0XFE72, 1, 0, 0, 1, 2, 0, 0, 8, 1}, /* 40 cells */
    {0X0403, 0XFE73, 1, 0, 0, 1, 2, 0, 0, 8, 1}, /* 32 cells */
    {0X0403, 0XFE74, 1, 0, 0, 1, 2, 0, 0, 8, 1}, /* 64 cells */
    {0X0403, 0XFE75, 1, 0, 0, 1, 2, 0, 0, 8, 1}, /* 80 cells */
    {0}
  };
  UsbChannelDefinition *def = definitions;

  while (def->vendor) {
    def->baud = protocol->serialBaud;
    def->parity = protocol->serialParity;
    def++;
  }

  if ((usb = usbFindChannel(definitions, (void *)device))) {
    usbBeginInput(usb->device, usb->definition.inputEndpoint, 8);
    return 1;
  }
  return 0;
}

static int
awaitUsbInput (int milliseconds) {
  return usbAwaitInput(usb->device, usb->definition.inputEndpoint, milliseconds);
}

static int
readUsbBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  int count = usbReapInput(usb->device, usb->definition.inputEndpoint, buffer, length,
                           (wait? timeout: 0), timeout);
  if (count != -1) return count;
  if (errno == EAGAIN) return 0;
  return -1;
}

static int
writeUsbBytes (const unsigned char *buffer, int length) {
  return usbWriteEndpoint(usb->device, usb->definition.outputEndpoint, buffer, length, 1000);
}

static void
closeUsbPort (void) {
  if (usb) {
    usbCloseChannel(usb);
    usb = NULL;
  }
}

static const InputOutputOperations usbOperations = {
  openUsbPort, closeUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes
};
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
/* Bluetooth IO */
#include "Programs/bluez.h"
#include "Programs/iomisc.h"

static int bluezConnection = -1;

static int
openBluezPort (char **parameters, const char *device) {
  return (bluezConnection = openRfcommConnection(device, 1)) != -1;
}

static int
awaitBluezInput (int milliseconds) {
  return awaitInput(bluezConnection, milliseconds);
}

static int
readBluezBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  if (!awaitInput(bluezConnection, (wait? timeout: 0)))
    return (errno == EAGAIN)? 0: -1;
  return readData(bluezConnection, buffer, length, 0, timeout);
}

static int
writeBluezBytes (const unsigned char *buffer, int length) {
  int count = writeData(bluezConnection, buffer, length);
  if (count != length) {
    if (count == -1) {
      LogError("Vario Bluetooth write");
    } else {
      LogPrint(LOG_WARNING, "Trunccated bluetooth write: %d < %d", count, length);
    }
  }
  return count;
}

static void
closeBluezPort (void) {
  close(bluezConnection);
  bluezConnection = -1;
}

static const InputOutputOperations bluezOperations = {
  openBluezPort, closeBluezPort,
  awaitBluezInput, readBluezBytes, writeBluezBytes
};
#endif /* ENABLE_BLUETOOTH_SUPPORT */

/* Baum Protocol */

#define ESCAPE 0X1B

typedef enum {
  BAUM_REQ_DisplayData             = 0X01,
  BAUM_REQ_GetVersionNumber        = 0X05,
  BAUM_REQ_GetKeys                 = 0X08,
  BAUM_REQ_GetMode                 = 0X11,
  BAUM_REQ_SetMode                 = 0X12,
  BAUM_REQ_SetProtocolState        = 0X15,
  BAUM_REQ_SetCommunicationChannel = 0X16,
  BAUM_REQ_CausePowerdown          = 0X17,
  BAUM_REQ_GetDeviceIdentity       = 0X84,
  BAUM_REQ_GetSerialNumber         = 0X8A,
  BAUM_REQ_GetBluetoothName        = 0X8C,
  BAUM_REQ_SetBluetoothName        = 0X8D,
  BAUM_REQ_SetBluetoothPin         = 0X8E
} BaumRequestCode;

typedef enum {
  BAUM_RSP_CellCount            = 0X01,
  BAUM_RSP_VersionNumber        = 0X05,
  BAUM_RSP_ModeSetting          = 0X11,
  BAUM_RSP_CommunicationChannel = 0X16,
  BAUM_RSP_PowerdownSignal      = 0X17,
  BAUM_RSP_RoutingKeys          = 0X22,
  BAUM_RSP_TopKeys              = 0X24,
  BAUM_RSP_FrontKeys            = 0X28,
  BAUM_RSP_BackKeys             = 0X29,
  BAUM_RSP_CommandKeys          = 0X2B,
  BAUM_RSP_ErrorCode            = 0X40,
  BAUM_RSP_DeviceIdentity       = 0X84,
  BAUM_RSP_SerialNumber         = 0X8A,
  BAUM_RSP_BluetoothName        = 0X8C
} BaumResponseCode;

typedef enum {
  BAUM_MODE_RoutingEnabled   = 0X08,
  BAUM_MODE_DisplayReversed  = 0X10,
  BAUM_MODE_DisplayEnabled   = 0X20,
  BAUM_MODE_PowerdownEnabled = 0X21,
  BAUM_MODE_PowerdownTime    = 0X22,
  BAUM_MODE_BluetoothEnabled = 0X23,
  BAUM_MODE_UsbCharge        = 0X24
} BaumMode;

typedef enum {
  BAUM_PDT_5Minutes  = 1,
  BAUM_PDT_10Minutes = 2,
  BAUM_PDT_1Hour     = 3,
  BAUM_PDT_2Hours    = 4
} BaumPowerdownTime;

typedef enum {
  BAUM_PDR_ProtocolRequested = 0X01,
  BAUM_PDR_PowerSwitch       = 0X02,
  BAUM_PDR_AutoPowerOff      = 0X04,
  BAUM_PDR_BatteryLow        = 0X08,
  BAUM_PDR_Charging          = 0X80
} BaumPowerdownReason;

typedef enum {
  BAUM_KEY_TL1 = 0X00000001,
  BAUM_KEY_TL2 = 0X00000002,
  BAUM_KEY_TL3 = 0X00000004,
  BAUM_KEY_TR1 = 0X00000008,
  BAUM_KEY_TR2 = 0X00000010,
  BAUM_KEY_TR3 = 0X00000020,

  BAUM_KEY_FLU = 0X00000100,
  BAUM_KEY_FLD = 0X00000200,
  BAUM_KEY_FMU = 0X00000400,
  BAUM_KEY_FMD = 0X00000800,
  BAUM_KEY_FRU = 0X00001000,
  BAUM_KEY_FRD = 0X00002000,

  BAUM_KEY_CK1 = 0X00010000,
  BAUM_KEY_CK2 = 0X00020000,
  BAUM_KEY_CK3 = 0X00040000,
  BAUM_KEY_CK4 = 0X00080000,
  BAUM_KEY_CK5 = 0X00100000,
  BAUM_KEY_CK6 = 0X00200000,
  BAUM_KEY_CK7 = 0X00400000,

  BAUM_KEY_BLU = 0X01000000,
  BAUM_KEY_BLD = 0X02000000,
  BAUM_KEY_BMU = 0X04000000,
  BAUM_KEY_BMD = 0X08000000,
  BAUM_KEY_BRU = 0X10000000,
  BAUM_KEY_BRD = 0X20000000
} BaumKey;

typedef enum {
  BAUM_ERR_BluetoothSupport       = 0X0A,
  BAUM_ERR_TransmitOverrun        = 0X10,
  BAUM_ERR_ReceiveOverrun         = 0X11,
  BAUM_ERR_TransmitTimeout        = 0X12,
  BAUM_ERR_ReceiveTimeout         = 0X13,
  BAUM_ERR_PacketType             = 0X14,
  BAUM_ERR_PacketChecksum         = 0X15,
  BAUM_ERR_PacketData             = 0X16,
  BAUM_ERR_Test                   = 0X18,
  BAUM_ERR_FlashWrite             = 0X19,
  BAUM_ERR_CommunicationChannel   = 0X1F,
  BAUM_ERR_SerialNumber           = 0X20,
  BAUM_ERR_SerialParity           = 0X21,
  BAUM_ERR_SerialOverrun          = 0X22,
  BAUM_ERR_SerialFrame            = 0X24,
  BAUM_ERR_LocalizationIdentifier = 0X25,
  BAUM_ERR_LocalizationIndex      = 0X26,
  BAUM_ERR_LanguageIdentifier     = 0X27,
  BAUM_ERR_LanguageIndex          = 0X28,
  BAUM_ERR_BrailleTableIdentifier = 0X29,
  BAUM_ERR_BrailleTableIndex      = 0X2A
} BaumError;

#define BAUM_DEVICE_IDENTITY_LENGTH 16
#define BAUM_SERIAL_NUMBER_LENGTH 8
#define BAUM_BLUETOOTH_NAME_LENGTH 14

typedef union {
  unsigned char bytes[17];

  struct {
    unsigned char code;

    union {
      unsigned char routingKeys[MAXIMUM_ROUTING_BYTES];
      unsigned char topKeys;
      unsigned char frontKeys;
      unsigned char backKeys;
      unsigned char commandKeys;

      unsigned char cellCount;
      unsigned char versionNumber;
      unsigned char communicationChannel;
      unsigned char powerdownReason;

      struct {
        unsigned char identifier;
        unsigned char setting;
      } mode;

      unsigned char deviceIdentity[BAUM_DEVICE_IDENTITY_LENGTH];
      unsigned char serialNumber[BAUM_SERIAL_NUMBER_LENGTH];
      unsigned char bluetoothName[BAUM_BLUETOOTH_NAME_LENGTH];
    } values;
  } data;
} BaumResponsePacket;

static int
logBaumDeviceIdentity (const BaumResponsePacket *packet) {
  int length = sizeof(packet->data.values.deviceIdentity);
  char identity[length + 1];
  memcpy(identity, packet->data.values.deviceIdentity, length);

  while (length) {
    const char byte = identity[length - 1];
    if ((byte != ' ') || !byte) break;
    --length;
  }
  identity[length] = 0;
  LogPrint(LOG_INFO, "Baum Device Identity: %s", identity);

  {
    const char *number = strpbrk(identity, "123456789");
    if (number) return atoi(number);
  }

  if (strcmp(identity, "PocketVario") == 0) return 24;
  if (strcmp(identity, "SuperVario") == 0) return 40;
  return 0;
}

static void
logBaumSerialNumber (const BaumResponsePacket *packet) {
  int length = sizeof(packet->data.values.serialNumber);
  unsigned char number[length + 1];
  memcpy(number, packet->data.values.serialNumber, length);

  while (length) {
    unsigned char byte = number[length - 1];
    if ((byte != ' ') || !byte) break;
    --length;
  }
  number[length] = 0;

  LogPrint(LOG_INFO, "Baum Serial Number: %s", number);
}

static int
logBaumPowerdownReason (BaumPowerdownReason reason) {
  typedef struct {
    BaumPowerdownReason bit;
    const char *explanation;
  } ReasonEntry;
  static const ReasonEntry reasonTable[] = {
    {BAUM_PDR_ProtocolRequested, "driver request"},
    {BAUM_PDR_PowerSwitch      , "power switch"},
    {BAUM_PDR_AutoPowerOff     , "idle timeout"},
    {BAUM_PDR_BatteryLow       , "battery low"},
    {0}
  };
  const ReasonEntry *entry;

  unsigned char buffer[0X100];
  int length = 0;
  char delimiter = ':';

  sprintf(&buffer[length], "Baum Powerdown%n", &length);
  for (entry=reasonTable; entry->bit; ++entry) {
    if (reason & entry->bit) {
      int count;
      sprintf(&buffer[length], "%c %s%n", delimiter, entry->explanation, &count);
      length += count;
      delimiter = ',';
    }
  }

  LogPrint(LOG_WARNING, "%.*s", length, buffer);
  return 1;
}

static int
readBaumPacket (unsigned char *packet, int size) {
  int started = 0;
  int escape = 0;
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!readByte(&byte, started)) {
      if (offset > 0) LogBytes("Partial Packet", packet, offset);
      return 0;
    }

    if (byte == ESCAPE) {
      if ((escape = !escape)) continue;
    } else if (escape) {
      escape = 0;

      if (offset > 0) {
        LogBytes("Short Packet", packet, offset);
        offset = 0;
        length = 0;
      } else {
        started = 1;
      }
    }

    if (!started) {
      LogBytes("Ignored Byte", &byte, 1);
      continue;
    }

    if (offset < size) {
      if (offset == 0) {
        switch (byte) {
          case BAUM_RSP_CellCount:
          case BAUM_RSP_VersionNumber:
          case BAUM_RSP_CommunicationChannel:
          case BAUM_RSP_PowerdownSignal:
          case BAUM_RSP_TopKeys:
          case BAUM_RSP_FrontKeys:
          case BAUM_RSP_BackKeys:
          case BAUM_RSP_CommandKeys:
          case BAUM_RSP_ErrorCode:
            length = 2;
            break;

          case BAUM_RSP_ModeSetting:
            length = 3;
            break;

          case BAUM_RSP_SerialNumber:
            length = 9;
            break;

          case BAUM_RSP_BluetoothName:
            length = 15;
            break;

          case BAUM_RSP_DeviceIdentity:
            length = 17;
            break;

          case BAUM_RSP_RoutingKeys:
            length = (cellCount > 40)? 11: 6;
            break;

          default:
            LogBytes("Unknown Packet", &byte, 1);
            started = 0;
            continue;
        }
      }

      packet[offset] = byte;
    } else {
      if (offset == size) LogBytes("Truncated Packet", packet, offset);
      LogBytes("Discarded Byte", &byte, 1);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        started = 0;
        continue;
      }

      if (logInputPackets) LogBytes("Input Packet", packet, offset);
      return length;
    }
  }
}

static int
getBaumPacket (BaumResponsePacket *packet) {
  return readBaumPacket(packet->bytes, sizeof(*packet));
}

static int
writeBaumPacket (BrailleDisplay *brl, const unsigned char *packet, int length) {
  unsigned char buffer[1 + (length * 2)];
  unsigned char *byte = buffer;
  *byte++ = ESCAPE;

  {
    int index = 0;
    while (index < length)
      if ((*byte++ = packet[index++]) == ESCAPE)
        *byte++ = ESCAPE;
  }

  {
    int count = byte - buffer;
    if (logOutputPackets) LogBytes("Output Packet", buffer, count);

    {
      int ok = io->writeBytes(buffer, count) != -1;
      if (ok) adjustWriteDelay(brl, count);
      return ok;
    }
  }
}

static int
identifyBaumDisplay (BrailleDisplay *brl) {
  int tries = 0;
  static const unsigned char request[] = {BAUM_REQ_GetDeviceIdentity};
  while (writeBaumPacket(brl, request, sizeof(request))) {
    while (io->awaitInput(500)) {
      BaumResponsePacket response;
      if (getBaumPacket(&response)) {
        if (response.data.code == BAUM_RSP_DeviceIdentity) {
          cellCount = logBaumDeviceIdentity(&response);

          {
            static const char request[] = {BAUM_REQ_DisplayData};
            if (!writeBaumPacket(brl, request, sizeof(request))) goto error;
            if (!writeBaumPacket(brl, request, sizeof(request))) goto error;

            while (io->awaitInput(100)) {
              BaumResponsePacket response;
              int size = getBaumPacket(&response);

              if (size) {
                if (response.data.code == BAUM_RSP_CellCount) {
                  cellCount = response.data.values.cellCount;
                  break;
                }

                LogBytes("unexpected packet", response.bytes, size);
              } else if (errno != EAGAIN) {
                goto error;
              }
            }
          }

          if (cellCount) return 1;
          LogPrint(LOG_DEBUG, "unknown cell count.");
        }
      }
    }
    if (errno != EAGAIN) break;
    if (++tries == 5) break;
  }

error:
  return 0;
}

static int
updateBaumKeys (BrailleDisplay *brl, int *keyPressed) {
  BaumResponsePacket packet;
  int size;

  while ((size = getBaumPacket(&packet))) {
    switch (packet.data.code) {
      case BAUM_RSP_CellCount:
        changeCellCount(brl, packet.data.values.cellCount);
        continue;

      case BAUM_RSP_DeviceIdentity:
        logBaumDeviceIdentity(&packet);
        continue;

      case BAUM_RSP_SerialNumber:
        logBaumSerialNumber(&packet);
        continue;

      case BAUM_RSP_CommunicationChannel:
        continue;

      case BAUM_RSP_PowerdownSignal:
        if (logBaumPowerdownReason(packet.data.values.powerdownReason)) {
        }
        continue;

      {
        unsigned int keys;
        unsigned int shift;

      case BAUM_RSP_TopKeys:
        keys = packet.data.values.topKeys;
        shift = 0;
        goto doKeys;

      case BAUM_RSP_FrontKeys:
        keys = packet.data.values.frontKeys;
        shift = 8;
        goto doKeys;

      case BAUM_RSP_CommandKeys:
        keys = packet.data.values.commandKeys;
        shift = 16;
        goto doKeys;

      case BAUM_RSP_BackKeys:
        keys = packet.data.values.backKeys;
        shift = 24;
        goto doKeys;

      doKeys:
        if (!setFunctionKeys((0XFF << shift), (keys << shift), keyPressed)) continue;
        return 1;
      }

      case BAUM_RSP_RoutingKeys: {
        int changed = 0;
        int number = 0;
        int index;
        for (index=0; index<MAXIMUM_ROUTING_BYTES; ++index) {
          unsigned char byte = packet.data.values.routingKeys[index];
          unsigned char bit;
          for (bit=0X01; bit; bit<<=1) {
            if (setRoutingKey(number, (byte & bit), keyPressed)) changed = 1;
            if (++number == cellCount) goto doneRoutingKeys;
          }
        }

      doneRoutingKeys:
        if (!changed) continue;
        return 1;
      }

      default:
        LogBytes("unexpected packet", packet.bytes, size);
        continue;
    }
  }

  return 0;
}

static int
writeBaumCells (BrailleDisplay *brl) {
  unsigned char packet[1 + cellCount];
  unsigned char *byte = packet;

  *byte++ = BAUM_REQ_DisplayData;

  memcpy(byte, externalCells, cellCount);
  byte += cellCount;

  return writeBaumPacket(brl, packet, byte-packet);
}

static const ProtocolOperations baumOperations = {
  19200, SERIAL_PARITY_NONE,
  readBaumPacket, writeBaumPacket,
  identifyBaumDisplay, updateBaumKeys, writeBaumCells
};

/* HandyTech Protocol */

typedef enum {
  HT_REQ_WRITE = 0X01,
  HT_REQ_RESET = 0XFF
} HandyTechRequestCode;

typedef enum {
  HT_RSP_KEY_TL1   = 0X04, /* UP */
  HT_RSP_KEY_TL2   = 0X03, /* B1 */
  HT_RSP_KEY_TL3   = 0X08, /* DN */
  HT_RSP_KEY_TR1   = 0X07, /* B2 */
  HT_RSP_KEY_TR2   = 0X0B, /* B3 */
  HT_RSP_KEY_TR3   = 0X0F, /* B4 */
  HT_RSP_KEY_CR1   = 0X20,
  HT_RSP_WRITE_ACK = 0X7E,
  HT_RSP_RELEASE   = 0X80,
  HT_RSP_IDENTITY  = 0XFE
} HandyTechResponseCode;
#define HT_IS_ROUTING_KEY(code) (((code) >= HT_RSP_KEY_CR1) && ((code) < (HT_RSP_KEY_CR1 + cellCount)))

typedef union {
  unsigned char bytes[2];

  struct {
    unsigned char code;

    union {
      unsigned char identity;
    } values;
  } data;
} HandyTechResponsePacket;

typedef struct {
  const char *name;
  unsigned char identity;
  unsigned char textCount;
  unsigned char statusCount;
} HandyTechModelEntry;

static const HandyTechModelEntry handyTechModelTable[] = {
  { "Modular 40",
    0X89, 40, 4
  }
  ,
  {NULL}        
};
static const HandyTechModelEntry *ht;

static int
readHandyTechPacket (unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!readByte(&byte, offset>0)) {
      if (offset > 0) LogBytes("Partial Packet", packet, offset);
      return 0;
    }

    if (offset < size) {
      if (offset == 0) {
        switch (byte) {
          case HT_RSP_IDENTITY:
            length = 2;
            break;

          case HT_RSP_WRITE_ACK:
            length = 1;
            break;

          default: {
            unsigned char key = byte & ~HT_RSP_RELEASE;
            switch (key) {
              default:
                if (!HT_IS_ROUTING_KEY(key)) {
                  LogBytes("Unknown Packet", &byte, 1);
                  continue;
                }

              case HT_RSP_KEY_TL1:
              case HT_RSP_KEY_TL2:
              case HT_RSP_KEY_TL3:
              case HT_RSP_KEY_TR1:
              case HT_RSP_KEY_TR2:
              case HT_RSP_KEY_TR3:
                length = 1;
                break;
            }
            break;
          }
        }
      }

      packet[offset] = byte;
    } else {
      if (offset == size) LogBytes("Truncated Packet", packet, offset);
      LogBytes("Discarded Byte", &byte, 1);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      if (logInputPackets) LogBytes("Input Packet", packet, offset);
      return length;
    }
  }
}

static int
getHandyTechPacket (HandyTechResponsePacket *packet) {
  return readHandyTechPacket(packet->bytes, sizeof(*packet));
}

static int
writeHandyTechPacket (BrailleDisplay *brl, const unsigned char *packet, int length) {
  if (logOutputPackets) LogBytes("Output Packet", packet, length);

  {
    int ok = io->writeBytes(packet, length) != -1;
    if (ok) adjustWriteDelay(brl, length);
    return ok;
  }
}

static const HandyTechModelEntry *
findHandyTechModel (unsigned char identity) {
  const HandyTechModelEntry *model;

  for (model=handyTechModelTable; model->name; ++model) {
    if (identity == model->identity) {
      LogPrint(LOG_INFO, "Baum emulation: HandyTech Model: %02X -> %s", identity, model->name);
      return model;
    }
  }

  LogPrint(LOG_WARNING, "Baum emulation: unknown HandyTech identity code: %02X", identity);
  return NULL;
}

static int
identifyHandyTechDisplay (BrailleDisplay *brl) {
  int tries = 0;
  static const unsigned char request[] = {HT_REQ_RESET};
  while (writeHandyTechPacket(brl, request, sizeof(request))) {
    while (io->awaitInput(500)) {
      HandyTechResponsePacket response;
      if (getHandyTechPacket(&response)) {
        if (response.data.code == HT_RSP_IDENTITY) {
          if (!(ht = findHandyTechModel(response.data.values.identity))) return 0;
          cellCount = ht->textCount;
          return 1;
        }
      }
    }
    if (errno != EAGAIN) break;
    if (++tries == 5) break;
  }

  return 0;
}

static int
updateHandyTechKeys (BrailleDisplay *brl, int *keyPressed) {
  HandyTechResponsePacket packet;
  int size;

  while ((size = getHandyTechPacket(&packet))) {
    unsigned char code = packet.data.code;

    switch (code) {
      case HT_RSP_IDENTITY: {
        const HandyTechModelEntry *model = findHandyTechModel(packet.data.values.identity);
        if (model && (model != ht)) {
          ht = model;
          changeCellCount(brl, ht->textCount);
        }
        continue;
      }

      case HT_RSP_WRITE_ACK:
        continue;
    }

    {
      unsigned char key = code & ~HT_RSP_RELEASE;
      int press = (code & HT_RSP_RELEASE) == 0;

      if (HT_IS_ROUTING_KEY(key)) {
        if (!setRoutingKey((key - HT_RSP_KEY_CR1), press, keyPressed)) continue;
      } else {
        unsigned int bit;
        switch (key) {
#define KEY(name) case HT_RSP_KEY_##name: bit = BAUM_KEY_##name; break;
          KEY(TL1);
          KEY(TL2);
          KEY(TL3);
          KEY(TR1);
          KEY(TR2);
          KEY(TR3);
#undef KEY

          default:
            LogBytes("unexpected packet", packet.bytes, size);
            continue;
        }
        if (!setFunctionKeys(bit, (press? bit: 0), keyPressed)) continue;
      }
      return 1;
    }
  }

  return 0;
}

static int
writeHandyTechCells (BrailleDisplay *brl) {
  unsigned char packet[1 + ht->statusCount + cellCount];
  unsigned char *byte = packet;

  *byte++ = HT_REQ_WRITE;

  {
    int count = ht->statusCount;
    while (count-- > 0) *byte++ = 0;
  }

  memcpy(byte, externalCells, cellCount);
  byte += cellCount;

  return writeHandyTechPacket(brl, packet, byte-packet);
}

static const ProtocolOperations handyTechOperations = {
  19200, SERIAL_PARITY_ODD,
  readHandyTechPacket, writeHandyTechPacket,
  identifyHandyTechDisplay, updateHandyTechKeys, writeHandyTechCells
};

/* PowerBraille Protocol */

#define PB_BUTTONS0_MARKER 0X60
#define PB1_BUTTONS0_TR3   0X08
#define PB1_BUTTONS0_TR2   0X04
#define PB1_BUTTONS0_TR1   0X02
#define PB1_BUTTONS0_TL2   0X01
#define PB2_BUTTONS0_TL3   0X08
#define PB2_BUTTONS0_TR2   0X04
#define PB2_BUTTONS0_TL1   0X02
#define PB2_BUTTONS0_TL2   0X01

#define PB_BUTTONS1_MARKER 0XE0
#define PB1_BUTTONS1_TL3   0X08
#define PB1_BUTTONS1_TL1   0X02
#define PB2_BUTTONS1_TR3   0X04
#define PB2_BUTTONS1_TR1   0X02

typedef enum {
  PB_REQ_WRITE = 0X04,
  PB_REQ_RESET = 0X0A
} PowerBrailleRequestCode;

typedef enum {
  PB_RSP_IDENTITY = 0X05,
  PB_RSP_SENSORS  = 0X08
} PowerBrailleResponseCode;

typedef union {
  unsigned char bytes[11];

  unsigned char buttons[2];

  struct {
    unsigned char zero;
    unsigned char code;

    union {
      struct {
        unsigned char cells;
        unsigned char dots;
        unsigned char version[4];
        unsigned char checksum[4];
      } identity;

      struct {
        unsigned char count;
        unsigned char vertical[4];
        unsigned char horizontal[10];
      } sensors;
    } values;
  } data;
} PowerBrailleResponsePacket;

static int
readPowerBraillePacket (unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!readByte(&byte, offset>0)) {
      if (offset > 0) LogBytes("Partial Packet", packet, offset);
      return 0;
    }
  haveByte:

    if (offset == 0) {
      if (!byte) {
        length = 2;
      } else if ((byte & PB_BUTTONS0_MARKER) == PB_BUTTONS0_MARKER) {
        length = 2;
      } else {
        LogBytes("Ignored Byte", &byte, 1);
        continue;
      }
    } else if (packet[0]) {
      if ((byte & PB_BUTTONS1_MARKER) != PB_BUTTONS1_MARKER) {
        LogBytes("Short Packet", packet, offset);
        offset = 0;
        length = 0;
        goto haveByte;
      }
    } else {
      if (offset == 1) {
        switch (byte) {
          case PB_RSP_IDENTITY:
            length = 12;
            break;

          case PB_RSP_SENSORS:
            length = 3;
            break;

          default:
            LogBytes("Unknown Packet", &byte, 1);
            offset = 0;
            length = 0;
            continue;
        }
      } else if ((offset == 2) && (packet[1] == PB_RSP_SENSORS)) {
        length += byte;
      }
    }

    if (offset < length) {
      packet[offset] = byte;
    } else {
      if (offset == size) LogBytes("Truncated Packet", packet, offset);
      LogBytes("Discarded Byte", &byte, 1);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      if (logInputPackets) LogBytes("Input Packet", packet, offset);
      return length;
    }
  }
}

static int
getPowerBraillePacket (PowerBrailleResponsePacket *packet) {
  return readPowerBraillePacket(packet->bytes, sizeof(*packet));
}

static int
writePowerBraillePacket (BrailleDisplay *brl, const unsigned char *packet, int length) {
  unsigned char buffer[2 + length];
  unsigned char *byte = buffer;

  *byte++ = 0XFF;
  *byte++ = 0XFF;

  memcpy(byte, packet, length);
  byte += length;

  {
    int count = byte - buffer;
    if (logOutputPackets) LogBytes("Output Packet", buffer, count);

    {
      int ok = io->writeBytes(buffer, count) != -1;
      if (ok) adjustWriteDelay(brl, count);
      return ok;
    }
  }
}

static int
identifyPowerBrailleDisplay (BrailleDisplay *brl) {
  int tries = 0;
  static const unsigned char request[] = {PB_REQ_RESET};
  while (writePowerBraillePacket(brl, request, sizeof(request))) {
    while (io->awaitInput(500)) {
      PowerBrailleResponsePacket response;
      if (getPowerBraillePacket(&response)) {
        if (response.data.code == PB_RSP_IDENTITY) {
          const unsigned char *version = response.data.values.identity.version;
          LogPrint(LOG_INFO, "Baum emulation: PowerBraille Version: %c%c%c%c",
                   version[0], version[1], version[2], version[3]);
          cellCount = response.data.values.identity.cells;
          return 1;
        }
      }
    }
    if (errno != EAGAIN) break;
    if (++tries == 5) break;
  }

  return 0;
}

static int
updatePowerBrailleKeys (BrailleDisplay *brl, int *keyPressed) {
  PowerBrailleResponsePacket packet;
  int size;

  while ((size = getPowerBraillePacket(&packet))) {
    if (!packet.data.zero) {
      switch (packet.data.code) {
        case PB_RSP_IDENTITY:
          changeCellCount(brl, packet.data.values.identity.cells);
          continue;

        case PB_RSP_SENSORS: {
          int changed = 0;
          int number = 0;
          int index;
          for (index=0; index<MAXIMUM_ROUTING_BYTES; ++index) {
            unsigned char byte = packet.data.values.sensors.horizontal[index];
            unsigned char bit;
            for (bit=0X01; bit; bit<<=1) {
              if (setRoutingKey(number, (byte & bit), keyPressed)) changed = 1;
              if (++number == cellCount) goto doneRoutingKeys;
            }
          }

        doneRoutingKeys:
          if (!changed) continue;
          return 1;
        }

        default:
          LogBytes("unexpected packet", packet.bytes, size);
          continue;
      }
    } else {
      unsigned int keys = 0;
      if (packet.buttons[0] & PB2_BUTTONS0_TL1) keys |= BAUM_KEY_TL1;
      if (packet.buttons[0] & PB2_BUTTONS0_TL2) keys |= BAUM_KEY_TL2;
      if (packet.buttons[0] & PB2_BUTTONS0_TL3) keys |= BAUM_KEY_TL3;
      if (packet.buttons[1] & PB2_BUTTONS1_TR1) keys |= BAUM_KEY_TR1;
      if (packet.buttons[0] & PB2_BUTTONS0_TR2) keys |= BAUM_KEY_TR2;
      if (packet.buttons[1] & PB2_BUTTONS1_TR3) keys |= BAUM_KEY_TR3;

      if (!setFunctionKeys(0XFF, keys, keyPressed)) continue;
      return 1;
    }
  }

  return 0;
}

static int
writePowerBrailleCells (BrailleDisplay *brl) {
  unsigned char packet[6 + (cellCount * 2)];
  unsigned char *byte = packet;

  *byte++ = PB_REQ_WRITE;
  *byte++ = 0; /* cursor mode: disabled */
  *byte++ = 0; /* cursor position: nowhere */
  *byte++ = 1; /* cursor type: command */
  *byte++ = cellCount * 2; /* attribute-data pairs */
  *byte++ = 0; /* start */

  {
    int i;
    for (i=0; i<cellCount; ++i) {
      *byte++ = 0; /* attributes */
      *byte++ = externalCells[i]; /* data */
    }
  }

  return writePowerBraillePacket(brl, packet, byte-packet);
}

static const ProtocolOperations powerBrailleOperations = {
  9600, SERIAL_PARITY_NONE,
  readPowerBraillePacket, writePowerBraillePacket,
  identifyPowerBrailleDisplay, updatePowerBrailleKeys, writePowerBrailleCells
};

/* Driver Handlers */

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "Baum Driver");
  LogPrint(LOG_INFO,   "   Copyright (C) 2005 by Dave Mielke <dave@mielke.cc>");
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
  }
  
  if (isSerialDevice(&device)) {
    io = &serialOperations;

#ifdef ENABLE_USB_SUPPORT
  } else if (isUsbDevice(&device)) {
    io = &usbOperations;
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
  } else if (isBluetoothDevice(&device)) {
    io = &bluezOperations;
#endif /* ENABLE_BLUETOOTH_SUPPORT */

  } else {
    unsupportedDevice(device);
    return 0;
  }

  {
    static const ProtocolOperations *const protocolTable[] = {
      &baumOperations,
      &handyTechOperations,
      &powerBrailleOperations,
      NULL
    };
    const ProtocolOperations *const *protocolAddress = protocolTable;

    while ((protocol = *protocolAddress)) {
      if (io->openPort(parameters, device)) {
        if (!flushInput()) break;

        if (protocol->identifyDisplay(brl)) {
          logCellCount();

          memset(&activeKeys, 0, sizeof(activeKeys));
          memset(&pressedKeys, 0, sizeof(pressedKeys));

          clearCells(0, cellCount);
          if (!updateCells(brl)) break;

          brl->x = cellCount;
          brl->y = 1;
          brl->helpPage = 0;
          return 1;
        }

        io->closePort();
      }

      ++protocolAddress;
    }
  }

  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  io->closePort();
}

static ssize_t
brl_readPacket (BrailleDisplay *brl, unsigned char *buffer, size_t size) {
  int count = protocol->readPacket(buffer, size);
  if (!count) count = -1;
  return count;
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const unsigned char *packet, size_t length) {
  return protocol->writePacket(brl, packet, length)? length: -1;
}

static int
brl_reset (BrailleDisplay *brl) {
  return 0;
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  int start = 0;
  int count = cellCount;

  while (count > 0) {
    if (brl->buffer[count-1] != internalCells[count-1]) break;
    --count;
  }

  while (start < count) {
    if (brl->buffer[start] != internalCells[start]) break;
    ++start;
  }

  memcpy(&internalCells[start], &brl->buffer[start], count);
  translateCells(start, count);
  updateCells(brl);
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  int command;
  int keyPressed = 0;
  unsigned char routingKeys[cellCount];
  int routingCount = 0;

  if (!protocol->updateKeys(brl, &keyPressed)) {
    if (errno == EAGAIN) return EOF;
    return BRL_CMD_RESTARTBRL;
  }

  if (keyPressed) activeKeys = pressedKeys;
  command = BRL_CMD_NOOP;

  {
    int key;
    for (key=0; key<cellCount; ++key)
      if (activeKeys.routing[key])
        routingKeys[routingCount++] = key;
  }

#define KEY(key,cmd) case (key): command = (cmd); break;
  if (routingCount == 0) {
    switch (activeKeys.function) {
      KEY(BAUM_KEY_TL2, BRL_CMD_FWINLT);
      KEY(BAUM_KEY_TR2, BRL_CMD_FWINRT);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TL3, BRL_CMD_CHRLT);
      KEY(BAUM_KEY_TR1|BAUM_KEY_TR3, BRL_CMD_CHRRT);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TL3, BRL_CMD_LNBEG);
      KEY(BAUM_KEY_TR1|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_LNEND);

      KEY(BAUM_KEY_TR1, BRL_CMD_LNUP);
      KEY(BAUM_KEY_TR3, BRL_CMD_LNDN);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TR1, BRL_CMD_TOP);
      KEY(BAUM_KEY_TL3|BAUM_KEY_TR3, BRL_CMD_BOT);

      KEY(BAUM_KEY_TR2|BAUM_KEY_TR1, BRL_CMD_PRDIFLN);
      KEY(BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_NXDIFLN);

      KEY(BAUM_KEY_TL2|BAUM_KEY_TL1, BRL_CMD_ATTRUP);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TL3, BRL_CMD_ATTRDN);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TR1|BAUM_KEY_TR2, BRL_CMD_PRPROMPT);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_NXPROMPT);

      KEY(BAUM_KEY_TL1, BRL_CMD_HOME);
      KEY(BAUM_KEY_TL3, BRL_CMD_INFO);

      KEY(BAUM_KEY_TL2|BAUM_KEY_TR2, BRL_CMD_CSRTRK);
      KEY(BAUM_KEY_TL1|BAUM_KEY_TL3|BAUM_KEY_TR1|BAUM_KEY_TR3, BRL_CMD_CSRJMP_VERT);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TR1, BRL_CMD_FREEZE);
      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TR2, BRL_CMD_HELP);
      KEY(BAUM_KEY_TL1|BAUM_KEY_TL3|BAUM_KEY_TR1, BRL_CMD_PREFMENU);
      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR1, BRL_CMD_PASTE);
      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR2, BRL_CMD_PREFLOAD);
      KEY(BAUM_KEY_TL1|BAUM_KEY_TL3|BAUM_KEY_TR3, BRL_CMD_BACK);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TR1|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_PREFSAVE);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR2, BRL_CMD_SIXDOTS|BRL_FLG_TOGGLE_ON);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR3, BRL_CMD_SIXDOTS|BRL_FLG_TOGGLE_OFF);

      KEY(BAUM_KEY_TL3|BAUM_KEY_TR1, BRL_CMD_MUTE);
      KEY(BAUM_KEY_TL3|BAUM_KEY_TR2, BRL_CMD_SAY_LINE);
      KEY(BAUM_KEY_TL3|BAUM_KEY_TR1|BAUM_KEY_TR2, BRL_CMD_SAY_ABOVE);
      KEY(BAUM_KEY_TL3|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_SAY_BELOW);
      KEY(BAUM_KEY_TL3|BAUM_KEY_TR1|BAUM_KEY_TR3, BRL_CMD_SPKHOME);
      KEY(BAUM_KEY_TL3|BAUM_KEY_TR1|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_AUTOSPEAK);

      default:
        break;
    }
  } else if (routingCount == 1) {
    unsigned char key = routingKeys[0];
    switch (activeKeys.function) {
      KEY(0, BRL_BLK_ROUTE+key);

      KEY(BAUM_KEY_TL1, BRL_BLK_CUTBEGIN+key);
      KEY(BAUM_KEY_TL2, BRL_BLK_CUTAPPEND+key);
      KEY(BAUM_KEY_TR1, BRL_BLK_CUTLINE+key);
      KEY(BAUM_KEY_TR2, BRL_BLK_CUTRECT+key);

      KEY(BAUM_KEY_TL3, BRL_BLK_DESCCHAR+key);
      KEY(BAUM_KEY_TR3, BRL_BLK_SETLEFT+key);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2, BRL_BLK_SETMARK+key);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TL3, BRL_BLK_GOTOMARK+key);

      KEY(BAUM_KEY_TR1|BAUM_KEY_TR2, BRL_BLK_PRINDENT+key);
      KEY(BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_BLK_NXINDENT+key);

      default:
        break;
    }
  }
#undef KEY

  if (keyPressed) {
    command |= BRL_FLG_REPEAT_DELAY;
  } else {
    memset(&activeKeys, 0, sizeof(activeKeys));
  }

  return command;
}
