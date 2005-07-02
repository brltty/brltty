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

#define BRL_HAVE_PACKET_IO
#include "Programs/brl_driver.h"

/* Global Definitions */

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
  unsigned int keys;
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
  int (*identifyDisplay) (BrailleDisplay *brl);
  int (*updateKeys) (BrailleDisplay *brl, int *keyPressed);
  int (*writeCells) (BrailleDisplay *brl);
} ProtocolOperations;
static const ProtocolOperations *protocol;

/* Serial IO */
#include "Programs/serial.h"

static SerialDevice *serialDevice = NULL;

static int
openSerialPort (char **parameters, const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, BITS_PER_SECOND)) {
      return 1;
    }

    serialCloseDevice(serialDevice);
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
  const UsbChannelDefinition definitions[] = {
    {0X0403, 0XFE72, 1, 0, 0, 1, 2, 0},
    {0}
  };

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

/* Internal Routines */

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
logCellCount (const char *source) {
  LogPrint(LOG_INFO, "Cell Count: %d (%s)", cellCount, source);
}

/* Baum Protocol */

#define ESCAPE 0X1B

typedef enum {
  REQ_DisplayData             = 0X01,
  REQ_GetVersionNumber        = 0X05,
  REQ_GetKeys                 = 0X08,
  REQ_GetMode                 = 0X11,
  REQ_SetMode                 = 0X12,
  REQ_SetProtocolState        = 0X15,
  REQ_SetCommunicationChannel = 0X16,
  REQ_CausePowerdown          = 0X17,
  REQ_GetDeviceIdentity       = 0X84,
  REQ_GetSerialNumber         = 0X8A,
  REQ_GetBluetoothName        = 0X8C,
  REQ_SetBluetoothName        = 0X8D,
  REQ_SetBluetoothPin         = 0X8E
} BaumRequestCode;

typedef enum {
  RSP_CellCount            = 0X01,
  RSP_VersionNumber        = 0X05,
  RSP_ModeSetting          = 0X11,
  RSP_CommunicationChannel = 0X16,
  RSP_PowerdownSignal      = 0X17,
  RSP_RoutingKeys          = 0X22,
  RSP_TopKeys              = 0X24,
  RSP_FrontKeys            = 0X28,
  RSP_CommandKeys          = 0X2B,
  RSP_ErrorCode            = 0X40,
  RSP_DeviceIdentity       = 0X84,
  RSP_SerialNumber         = 0X8A,
  RSP_BluetoothName        = 0X8C
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
  BAUM_KEY_TL1 = 0X000001,
  BAUM_KEY_TL2 = 0X000002,
  BAUM_KEY_TL3 = 0X000004,
  BAUM_KEY_TR1 = 0X000008,
  BAUM_KEY_TR2 = 0X000010,
  BAUM_KEY_TR3 = 0X000020,
  BAUM_KEY_FLU = 0X000100,
  BAUM_KEY_FLD = 0X000200,
  BAUM_KEY_FMU = 0X000400,
  BAUM_KEY_FMD = 0X000800,
  BAUM_KEY_FRU = 0X001000,
  BAUM_KEY_FRD = 0X002000,
  BAUM_KEY_CK1 = 0X010000,
  BAUM_KEY_CK2 = 0X020000,
  BAUM_KEY_CK3 = 0X040000,
  BAUM_KEY_CK4 = 0X080000,
  BAUM_KEY_CK5 = 0X100000,
  BAUM_KEY_CK6 = 0X200000,
  BAUM_KEY_CK7 = 0X400000
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
  unsigned char bytes[0X100];

  struct {
    unsigned char code;

    union {
      unsigned char routingKeys[MAXIMUM_ROUTING_BYTES];
      unsigned char topKeys;
      unsigned char frontKeys;
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
readBaumPacket (BaumResponsePacket *packet) {
  int started = 0;
  int escape = 0;
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    {
      int count = io->readBytes(&byte, 1, started);
      if (count < 1) {
        if (count == 0) errno = EAGAIN;
        if (offset > 0) LogBytes("Partial Packet", packet->bytes, offset);
        return 0;
      }
    }

    if (byte == ESCAPE) {
      if ((escape = !escape)) continue;
    } else if (escape) {
      escape = 0;

      if (offset > 0) {
        LogBytes("Short Packet", packet->bytes, offset);
        offset = 0;
      } else {
        started = 1;
      }
    }

    if (!started) {
      LogBytes("Discarded Byte", &byte, 1);
      continue;
    }

    if (offset == 0) {
      switch (byte) {
        case RSP_CellCount:
        case RSP_VersionNumber:
        case RSP_CommunicationChannel:
        case RSP_PowerdownSignal:
        case RSP_TopKeys:
        case RSP_FrontKeys:
        case RSP_CommandKeys:
        case RSP_ErrorCode:
          length = 2;
          break;

        case RSP_ModeSetting:
          length = 3;
          break;

        case RSP_SerialNumber:
          length = 9;
          break;

        case RSP_BluetoothName:
          length = 15;
          break;

        case RSP_DeviceIdentity:
          length = 17;
          break;

        case RSP_RoutingKeys:
          length = (cellCount > 40)? 11: 6;
          break;

        default:
          LogBytes("Unknown Packet", &byte, 1);
          started = 0;
          continue;
      }
    }

    packet->bytes[offset++] = byte;
    if (offset == length) {
      LogBytes("Input Packet", packet->bytes, offset);
      return length;
    }
  }
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
    LogBytes("Output Packet", buffer, count);

    {
      int ok = io->writeBytes(buffer, count) != -1;
      if (ok) brl->writeDelay += count * 1000 / BYTES_PER_SECOND;
      return ok;
    }
  }
}

static int
identifyBaumDisplay (BrailleDisplay *brl) {
  int tries = 0;
  static const unsigned char request[] = {REQ_GetDeviceIdentity};
  while (writeBaumPacket(brl, request, sizeof(request))) {
    while (io->awaitInput(500)) {
      BaumResponsePacket response;
      if (readBaumPacket(&response)) {
        if (response.data.code == RSP_DeviceIdentity) {
          int length = BAUM_DEVICE_IDENTITY_LENGTH;
          char identity[length + 1];

          memcpy(identity, response.data.values.deviceIdentity, length);
          while (length) {
            const char byte = identity[--length];
            if ((byte != ' ') && (byte != 0)) {
              ++length;
              break;
            }
          }
          identity[length] = 0;
          LogPrint(LOG_INFO, "Model Name: %s", identity);

          {
            static const char request[] = {REQ_DisplayData};
            if (!writeBaumPacket(brl, request, sizeof(request))) goto error;
            if (!writeBaumPacket(brl, request, sizeof(request))) goto error;

            while (io->awaitInput(100)) {
              BaumResponsePacket response;
              int size = readBaumPacket(&response);
              if (size) {
                switch (response.data.code) {
                  case RSP_CellCount:
                    cellCount = response.data.values.cellCount;
                    logCellCount("explicit");
                    return 1;

                  default:
                    LogBytes("unexpected packet", response.bytes, size);
                    break;
                }
              } else if (errno != EAGAIN) {
                goto error;
              }
            }
          }

          {
            const char *number = strpbrk(identity, "0123456789");
            if (number) {
              cellCount = atoi(number);
              logCellCount("implicit");
              return 1;
            }
          }
          LogPrint(LOG_WARNING, "unknown cell count: %s", identity);
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
  while (1) {
    BaumResponsePacket packet;
    int size = readBaumPacket(&packet);
    if (!size) return 0;

    *keyPressed = 0;
    switch (packet.data.code) {
      case RSP_CellCount: {
        unsigned char count = packet.data.values.cellCount;
        if (count != cellCount) {
          if (count > cellCount) clearCells(cellCount, count-cellCount);
          cellCount = count;
          logCellCount("resize");

          brl->x = cellCount;
          brl->resizeRequired = 1;
        }
        continue;
      }

      {
        unsigned int keys;
        unsigned int shift;

      case RSP_TopKeys:
        keys = packet.data.values.topKeys;
        shift = 0;
        goto doKeys;

      case RSP_FrontKeys:
        keys = packet.data.values.frontKeys;
        shift = 8;
        goto doKeys;

      case RSP_CommandKeys:
        keys = packet.data.values.commandKeys;
        shift = 16;
        goto doKeys;

      doKeys:
        keys = (pressedKeys.keys & ~(0XFF << shift)) | (keys << shift);
        if (keys & ~pressedKeys.keys) *keyPressed = 1;
        pressedKeys.keys = keys;
        return 1;
      }

      case RSP_RoutingKeys: {
        int key = 0;
        int index;
        for (index=0; index<MAXIMUM_ROUTING_BYTES; ++index) {
          unsigned char byte = packet.data.values.routingKeys[index];
          unsigned char bit;
          for (bit=0X01; bit; bit<<=1) {
            unsigned char *pressed = &pressedKeys.routing[key];

            if (!(byte & bit)) {
              *pressed = 0;
            } else if (!*pressed) {
              *pressed = *keyPressed = 1;
            }

            if (++key == cellCount) goto doneRoutingKeys;
          }
        }
      doneRoutingKeys:

        return 1;
      }

      default:
        LogBytes("unexpected packet", packet.bytes, size);
        continue;
    }
  }
}

static int
writeBaumCells (BrailleDisplay *brl) {
  unsigned char packet[1 + cellCount];
  unsigned char *byte = packet;

  *byte++ = REQ_DisplayData;

  memcpy(byte, externalCells, cellCount);
  byte += cellCount;

  return writeBaumPacket(brl, packet, byte-packet);
}

static const ProtocolOperations baumOperations = {
  identifyBaumDisplay, updateBaumKeys, writeBaumCells
};

/* Driver Handlers */

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "BAUM Vario (Emul. 1) Driver");
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

  if (io->openPort(parameters, device)) {
    static const ProtocolOperations *protocolTable[] = {
      &baumOperations,
      NULL
    };
    const ProtocolOperations **protocolEntry = protocolTable;

    while (*protocolEntry) {
      if ((*protocolEntry)->identifyDisplay(brl)) {
        protocol = *protocolEntry;

        memset(&activeKeys, 0, sizeof(activeKeys));
        memset(&pressedKeys, 0, sizeof(pressedKeys));

        clearCells(0, cellCount);
        if (!updateCells(brl)) break;

        brl->x = cellCount;
        brl->y = 1;
        brl->helpPage = 0;
        return 1;
      }

      ++protocolEntry;
    }

    io->closePort();
  }

  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  io->closePort();
}

static ssize_t
brl_readPacket (BrailleDisplay *brl, unsigned char *buffer, size_t size) {
  BaumResponsePacket packet;
  int count = readBaumPacket(&packet);
  if (!count) return -1;
  memcpy(buffer, packet.bytes, count);
  return count;
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const unsigned char *packet, size_t length) {
  return writeBaumPacket(brl, packet, length)? length: -1;
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
  int keyPressed;
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
    switch (activeKeys.keys) {
      KEY(BAUM_KEY_TL2, BRL_CMD_FWINLT);
      KEY(BAUM_KEY_TR2, BRL_CMD_FWINRT);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TL3, BRL_CMD_CHRLT);
      KEY(BAUM_KEY_TR1|BAUM_KEY_TR3, BRL_CMD_CHRRT);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TL3, BRL_CMD_LNBEG);
      KEY(BAUM_KEY_TR1|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_LNEND);

      KEY(BAUM_KEY_TR1, BRL_CMD_LNUP);
      KEY(BAUM_KEY_TR3, BRL_CMD_LNDN);

      KEY(BAUM_KEY_TL1, BRL_CMD_TOP);
      KEY(BAUM_KEY_TL3, BRL_CMD_BOT);

      KEY(BAUM_KEY_TR2|BAUM_KEY_TR1, BRL_CMD_PRDIFLN);
      KEY(BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_NXDIFLN);

      KEY(BAUM_KEY_TL2|BAUM_KEY_TL1, BRL_CMD_ATTRUP);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TL3, BRL_CMD_ATTRDN);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TR1, BRL_CMD_HOME);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TR2, BRL_CMD_PASTE);
      KEY(BAUM_KEY_TL3|BAUM_KEY_TR3, BRL_CMD_CSRJMP_VERT);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TR1, BRL_CMD_FREEZE);
      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TR2, BRL_CMD_HELP);
      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR1, BRL_CMD_PREFMENU);
      KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR2, BRL_CMD_PREFLOAD);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR1, BRL_CMD_INFO);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR1|BAUM_KEY_TR2, BRL_CMD_CSRTRK);
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
    switch (activeKeys.keys) {
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
