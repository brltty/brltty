/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

/* FreedomScientific/braille.c - Braille display library
 * Freedom Scientific's Focus and PacMate series
 * Author: Dave Mielke <dave@mielke.cc>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "Programs/misc.h"

#include "Programs/brl_driver.h"

#define DEBUG_PACKETS

typedef struct {
  int (*openPort) (char **parameters, const char *device);
  void (*closePort) (void);
  int (*readBytes) (void *buffer, int length);
  int (*writePacket) (const void *buffer, int length, int *delay);
} InputOutputOperations;

#include "Programs/serial.h"
static int serialDevice = -1;
static struct termios oldSerialSettings;
static int serialCharactersPerSecond;			/* file descriptor for Braille display */

static int
openSerialPort (char **parameters, const char *device) {
  if (openSerialDevice(device, &serialDevice, &oldSerialSettings)) {
    speed_t baud = B57600;
    struct termios newSerialSettings;

    memset(&newSerialSettings, 0, sizeof(newSerialSettings));
    newSerialSettings.c_cflag = CS8 | CLOCAL | CREAD;
    newSerialSettings.c_iflag = IGNPAR;
    newSerialSettings.c_oflag = 0;		/* raw output */
    newSerialSettings.c_lflag = 0;		/* don't echo or generate signals */
    newSerialSettings.c_cc[VMIN] = 0;	/* set nonblocking read */
    newSerialSettings.c_cc[VTIME] = 0;

    if (resetSerialDevice(serialDevice, &newSerialSettings, baud)) {
      serialCharactersPerSecond = baud2integer(baud) / 10;
      return 1;
    }

    close(serialDevice);
    serialDevice = -1;
  }
  return 0;
}

static void
closeSerialPort (void) {
  if (serialDevice != -1) {
    tcsetattr(serialDevice, TCSADRAIN, &oldSerialSettings);		/* restore terminal settings */
    close(serialDevice);
    serialDevice = -1;
  }
}

static int
readSerialBytes (void *buffer, int length) {
  return read(serialDevice, buffer, length);
}

static int
writeSerialPacket (const void *buffer, int length, int *delay) {
  if (delay) *delay += length * 1000 / serialCharactersPerSecond;
  return safe_write(serialDevice, buffer, length);
}

static const InputOutputOperations serialOperations = {
  openSerialPort, closeSerialPort,
  readSerialBytes, writeSerialPacket
};

#ifdef ENABLE_USB
#include "Programs/usb.h"

static UsbDevice *usbDevice = NULL;
static unsigned char usbInterface;
static unsigned char usbOutputEndpoint;
static unsigned char usbInputEndpoint;

static int
chooseUsbDevice (UsbDevice *device, void *data) {
  const char *serialNumber = data;
  const UsbDeviceDescriptor *descriptor = usbDeviceDescriptor(device);
  if ((descriptor->idVendor == 0XF4E) &&
      ((descriptor->idProduct == 0X100) ||
       (descriptor->idProduct == 0X111))) {
    if (!usbVerifySerialNumber(device, serialNumber)) return 0;

    usbInterface = 0;
    if (usbClaimInterface(device, usbInterface)) {
      if (usbSetConfiguration(device, 1)) {
        if (usbSetAlternative(device, usbInterface, 0)) {
          usbOutputEndpoint = 1;
          usbInputEndpoint = 2;
          return 1;
        }
      }
      usbReleaseInterface(device, usbInterface);
    }
  }
  return 0;
}

static int
openUsbPort (char **parameters, const char *device) {
  if ((usbDevice = usbFindDevice(chooseUsbDevice, (void *)device))) {
    if (usbBeginInput(usbDevice, usbInputEndpoint, USB_ENDPOINT_TRANSFER_BULK, 0X40, 8)) {
      return 1;
    }

    usbCloseDevice(usbDevice);
    usbDevice = NULL;
  } else {
    LogPrint(LOG_DEBUG, "USB device not found%s%s",
             (*device? ": ": "."),
             device);
  }
  return 0;
}

static void
closeUsbPort (void) {
  if (usbDevice) {
    usbReleaseInterface(usbDevice, usbInterface);
    usbCloseDevice(usbDevice);
    usbDevice = NULL;
  }
}

static int
readUsbBytes (void *buffer, int length) {
  int count = usbReapInput(usbDevice, buffer, length, 0);
  if (count == -1)
    if (errno == EAGAIN)
      count = 0;
  return count;
}

static int
writeUsbPacket (const void *buffer, int length, int *delay) {
  return usbBulkWrite(usbDevice, usbOutputEndpoint, buffer, length, 1000);
}

static const InputOutputOperations usbOperations = {
  openUsbPort, closeUsbPort,
  readUsbBytes, writeUsbPacket
};
#endif /* ENABLE_USB */

typedef enum {
  PKT_QUERY  = 0X00, /* host->unit: request device information */
  PKT_ACK    = 0X01, /* unit->host: acknowledge packet receipt */
  PKT_NAK    = 0X02, /* unit->host: negative acknowledge, report error */
  PKT_KEY    = 0X03, /* unit->host: key event */
  PKT_BUTTON = 0X04, /* unit->host: routing button event */
  PKT_WHEEL  = 0X05, /* unit->host: whiz wheel event */
  PKT_HVADJ  = 0X08, /* host->unit: set braille display voltage */
  PKT_BEEP   = 0X09, /* host->unit: sound short beep */
  PKT_UPDATE = 0X7E, /* host->unit: enter firmware update mode */
  PKT_DIAG   = 0X7F, /* host->unit: enter diagnostic mode */
  PKT_INFO   = 0X80, /* unit->host: response to query packet */
  PKT_WRITE  = 0X81  /* host->unit: write to braille display */
} PacketType;

typedef enum {
  PKT_ERR_TIMEOUT   = 0X30, /* no request received from host for a while */
  PKT_ERR_CHECKSUM  = 0X31, /* incorrect checksum */
  PKT_ERR_TYPE      = 0X32, /* unsupported packet type */
  PKT_ERR_PARAMETER = 0X33, /* invalid parameter */
  PKT_ERR_SIZE      = 0X34, /* write size too large */
  PKT_ERR_POSITION  = 0X35, /* write position too large */
  PKT_ERR_OVERRUN   = 0X36, /* message queue overflow */
  PKT_ERR_POWER     = 0X37, /* insufficient USB power */
  PKT_ERR_SPI       = 0X38  /* timeout on SPI bus */
} PacketError;

typedef enum {
  PKT_EXT_HVADJ    = 0X08, /* error in varibraille packet */
  PKT_EXT_BEEP     = 0X09, /* error in beep packet */
  PKT_EXT_CLEAR    = 0X31, /* error in ClearMsgBuf function */
  PKT_EXT_LOOP     = 0X32, /* timing loop in ParseCommands function */
  PKT_EXT_TYPE     = 0X33, /* unknown packet type in ParseCommands function */
  PKT_EXT_CMDWRITE = 0X34, /* error in CmdWrite function */
  PKT_EXT_UPDATE   = 0X7E, /* error in update packet */
  PKT_EXT_DIAG     = 0X7F, /* error in diag packet */
  PKT_EXT_QUERY    = 0X80, /* error in query packet */
  PKT_EXT_WRITE    = 0X81  /* error in write packet */
} PacketExtended;

typedef struct {
  unsigned char type;
  unsigned char arg1;
  unsigned char arg2;
  unsigned char arg3;
} PacketHeader;

typedef struct {
  PacketHeader header;
  union {
    unsigned char bytes[0X100];
    struct {
      char manufacturer[24];
      char model[16];
      char firmware[8];
    } info;
  } payload;
} Packet;

typedef struct {
  const char *identifier;
  unsigned char totalCells;
  unsigned char textCells;
  unsigned char statusCells;
} ModelEntry;
static const ModelEntry modelTable[] = {
  {"Focus 44"     , 44, 40, 3},
  {"Focus 70"     , 70, 66, 3},
  {"Focus 84"     , 84, 80, 3},
  {"pm display 20", 20, 20, 0},
  {"pm display 40", 40, 40, 0},
  {NULL           ,  0,  0, 0}
};
static const ModelEntry *model;

static const InputOutputOperations *io;

static TranslationTable outputTable;
static unsigned char outputBuffer[84];
static unsigned char textOffset;
static unsigned char statusOffset;
static int writeFrom;
static int writeTo;
static int writing;
static int writingFrom;
static int writingTo;

static union {
  Packet packet;
  unsigned char bytes[sizeof(Packet)];
} inputBuffer;
static int inputCount;

static int pressedKeys;
static int activeKeys;
#define KEY_DOT1          0X000001
#define KEY_DOT2          0X000002
#define KEY_DOT3          0X000004
#define KEY_DOT4          0X000008
#define KEY_DOT5          0X000010
#define KEY_DOT6          0X000020
#define KEY_DOT7          0X000040
#define KEY_DOT8          0X000080
#define KEY_WHEEL_LEFT    0X000100
#define KEY_WHEEL_RIGHT   0X000200
#define KEY_SHIFT_LEFT    0X000400
#define KEY_SHIFT_RIGHT   0X000800
#define KEY_ADVANCE_LEFT  0X001000
#define KEY_ADVANCE_RIGHT 0X002000
#define KEY_SPACE         0X008000
#define KEY_GDF_LEFT      0X010000
#define KEY_GDF_RIGHT     0X020000

static int wheelCommand;
static int wheelCounter;
#define WHEEL_COUNT 0X07
#define WHEEL_DOWN  0X08
#define WHEEL_UNIT  0X30
#define WHEEL_LEFT  0X00
#define WHEEL_RIGHT 0X10

static void
negativeAcknowledgement (const Packet *packet) {
  LogPrint(LOG_WARNING, "Negative Acknowledgement: %02X %02X",
           packet->header.arg1,
           packet->header.arg2);
}

static int
readPacket (BrailleDisplay *brl, Packet *packet) {
  int first = 1;
  while (1) {
    int count = sizeof(PacketHeader);
    int hasPayload = 0;

    if (inputCount >= sizeof(PacketHeader)) {
      if (inputBuffer.packet.header.type & 0X80) {
        hasPayload = 1;
        count += inputBuffer.packet.header.arg1 + 1;
      }
    }

    if (count <= inputCount) {
#ifdef DEBUG_PACKETS
      LogBytes("Input Packet", inputBuffer.bytes, count);
#endif /* DEBUG_PACKETS */

      if (hasPayload) {
        unsigned char checksum = 0;
        int index;
        for (index=0; index<count; ++index)
          checksum -= inputBuffer.bytes[index];
        if (checksum)
          LogPrint(LOG_WARNING, "Input packet checksum error.");
      }

      memcpy(packet, &inputBuffer, count);
      memcpy(&inputBuffer.bytes[0], &inputBuffer.bytes[count],
             inputCount -= count);
      return count;
    }

    if (first) {
      first = 0;
    } else {
      delay(1);
    }

    if ((count = io->readBytes(&inputBuffer.bytes[inputCount],
                               count - inputCount)) < 1) {
      return count;
    }
#ifdef DEBUG_PACKETS
    LogBytes("Input Bytes", &inputBuffer.bytes[inputCount], count);
#endif /* DEBUG_PACKETS */
    inputCount += count;
  }
}

static int
writePacket (
  BrailleDisplay *brl,
  unsigned char type,
  unsigned char arg1,
  unsigned char arg2,
  unsigned char arg3,
  const unsigned char *data
) {
  Packet packet;
  int size = sizeof(packet.header);
  unsigned char checksum = 0;

  checksum -= (packet.header.type = type);
  checksum -= (packet.header.arg1 = arg1);
  checksum -= (packet.header.arg2 = arg2);
  checksum -= (packet.header.arg3 = arg3);

  if (data) {
    unsigned char length = packet.header.arg1;
    int index;
    for (index=0; index<length; ++index)
      checksum -= (packet.payload.bytes[index] = data[index]);
    packet.payload.bytes[length] = checksum;
    size += length + 1;
  }

#ifdef DEBUG_PACKETS
  LogBytes("Output Packet", (unsigned char *)&packet, size);
#endif /* DEBUG_PACKETS */
  return io->writePacket(&packet, size, &brl->writeDelay) != -1;
}

static void
updateCells (BrailleDisplay *brl) {
  if (!writing)
    if (writeTo != -1)
      if (writePacket(brl, PKT_WRITE,
                      writeTo-writeFrom+1, writeFrom,
                      0, &outputBuffer[writeFrom])) {
        writing = 1;
        writingFrom = writeFrom;
        writingTo = writeTo;
        writeFrom = -1;
        writeTo = -1;
      }
}

static void
writeCells (
  BrailleDisplay *brl,
  const unsigned char *cells,
  unsigned char count,
  unsigned char offset
) {
  int index;
  for (index=0; index<count; ++index) {
    unsigned char cell = outputTable[cells[index]];
    unsigned char position = offset + index;
    unsigned char *byte = &outputBuffer[position];
    if (cell != *byte) {
      if ((writeFrom == -1) || (position < writeFrom)) writeFrom = position;
      if (position > writeTo) writeTo = position;
      *byte = cell;
    }
  }
}

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "Freedom Scientific Driver");
  LogPrint(LOG_INFO, "   Copyright (C) 2004 by Dave Mielke <dave@mielke.cc>");
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(&dots, &outputTable);
  }

  if (isSerialDevice(&device)) {
    io = &serialOperations;

#ifdef ENABLE_USB
  } else if (isUsbDevice(&device)) {
    io = &usbOperations;
#endif /* ENABLE_USB */

  } else {
    unsupportedDevice(device);
    return 0;
  }
  inputCount = 0;

  if (!io->openPort(parameters, device)) goto failure;
  while (writePacket(brl, PKT_QUERY, 0, 0, 0, NULL)) {
    int acknowledged = 0;
    delay(1000);
    while (1) {
      Packet response;
      int count = readPacket(brl, &response);

      if (count == -1) goto failure;

      if (count == 0) {
        if (!(acknowledged && model)) break;

        brl->x = model->textCells;
        brl->y = 1;
        return 1;
      }

      switch (response.header.type) {
        case PKT_INFO:
          model = modelTable;
          while (model->identifier) {
            if (strcmp(response.payload.info.model, model->identifier) == 0) break;
            ++model;
          }

          if (!model->identifier) {
            LogPrint(LOG_WARNING, "Detected unknown model: %s", response.payload.info.model);
            model = NULL;

            {
              const char *word = strrchr(response.payload.info.model, ' ');
              if (word) {
                int size;
                if (isInteger(&size, ++word)) {
                  static ModelEntry entry;
                  static char identifier[sizeof(response.payload.info.model)];
                  memset(&entry, 0, sizeof(entry));
                  entry.totalCells = entry.textCells = size;
                  snprintf(identifier, sizeof(identifier), "Generic %d", size);
                  entry.identifier = identifier;
                  model = &entry;
                }
              }
            }
          }

          if (model) {
            LogPrint(LOG_INFO, "Detected %s: text=%d, status=%d, firmware=%s",
                     model->identifier,
                     model->textCells, model->statusCells,
                     response.payload.info.firmware);

            statusOffset = 0;
            textOffset =  (model->statusCells)? model->statusCells+1: 0;

            memset(outputBuffer, 0, model->totalCells);
            writeFrom = 0;
            writeTo = model->totalCells - 1;
            writing = 0;
            pressedKeys = 0;
            activeKeys = 0;
            wheelCounter = 0;
          }
          break;

        case PKT_ACK:
          acknowledged = 1;
          break;

        case PKT_NAK:
          negativeAcknowledgement(&response);
        default:
          acknowledged = 0;
          model = NULL;
          break;
      }

      delay(10);
    }
  }

failure:
  brl_close(brl);
  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  io->closePort();
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  writeCells(brl, brl->buffer, model->textCells, textOffset);
  updateCells(brl);
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  writeCells(brl, status, model->statusCells, statusOffset);
}

static int
brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds) {
  if (wheelCounter) {
    --wheelCounter;
    return wheelCommand;
  }

  while (1) {
    Packet packet;
    int count = readPacket(brl, &packet);
    if (count == -1) return CMD_RESTARTBRL;
    if (count == 0) return EOF;

    switch (packet.header.type) {
      default:
        LogPrint(LOG_WARNING, "Unsupported packet: %02X %02X %02X %02X",
                 packet.header.type,
                 packet.header.arg1,
                 packet.header.arg2,
                 packet.header.arg3);
        continue;

      case PKT_NAK:
        negativeAcknowledgement(&packet);
        if (writing) {
          if ((writeFrom == -1) || (writingFrom < writeFrom)) writeFrom = writingFrom;
          if ((writeTo == -1) || (writingTo > writeTo)) writeTo = writingTo;
      case PKT_ACK:
          writing = 0;
        }
        continue;

      case PKT_KEY: {
        int keys = packet.header.arg1 |
                   (packet.header.arg2 << 8) |
                   (packet.header.arg3 << 16);
        int command;

        pressedKeys = keys;
        if ((activeKeys & keys) != keys) {
          activeKeys = keys;
          command = VAL_REPEAT_DELAY;
        } else {
          keys = activeKeys;
          activeKeys = 0;
          command = 0;
        }

        switch (keys) {
          default:
            command |= CMD_NOOP;
            break;
          case (KEY_WHEEL_LEFT):
            command |= CMD_BACK;
            break;
          case (KEY_WHEEL_RIGHT):
            command |= CMD_HOME;
            break;
        }
        return command;
      }

      case PKT_BUTTON: {
        int command = CMD_NOOP;
        activeKeys = 0;
        if (packet.header.arg2 & 0X01) {
          switch (packet.header.arg3) {
            default:
              break;

            case 0:
              switch (pressedKeys) {
                default:
                  break;
              }
              break;

            case 1:
              switch (pressedKeys) {
                default:
                  break;

                case 0:
                  command = CR_ROUTE;
                  break;
              }
              break;
          }
        }

        if (command != CMD_NOOP) command += packet.header.arg1;
        return command;
      }

      case PKT_WHEEL: {
        int unit = packet.header.arg1 & WHEEL_UNIT;
        int motion = packet.header.arg1 & (WHEEL_UNIT | WHEEL_DOWN);
        if (unit == WHEEL_RIGHT) motion ^= WHEEL_DOWN;

        activeKeys = 0;
        wheelCommand = CMD_NOOP;
        switch (motion) {
          default:
            break;

          case (WHEEL_LEFT):
            switch (pressedKeys) {
              default:
                break;
              case 0:
                wheelCommand = CMD_LNUP;
                break;
              case (KEY_WHEEL_LEFT):
                wheelCommand = CMD_PRDIFLN;
                break;
            }
            break;

          case (WHEEL_LEFT | WHEEL_DOWN):
            switch (pressedKeys) {
              default:
                break;
              case 0:
                wheelCommand = CMD_LNDN;
                break;
              case (KEY_WHEEL_LEFT):
                wheelCommand = CMD_NXDIFLN;
                break;
            }
            break;

          case (WHEEL_RIGHT):
            switch (pressedKeys) {
              default:
                break;
              case 0:
                wheelCommand = CMD_FWINLT;
                break;
              case (KEY_WHEEL_RIGHT):
                wheelCommand = CMD_CHRLT;
                break;
            }
            break;

          case (WHEEL_RIGHT | WHEEL_DOWN):
            switch (pressedKeys) {
              default:
                break;
              case 0:
                wheelCommand = CMD_FWINRT;
                break;
              case (KEY_WHEEL_RIGHT):
                wheelCommand = CMD_CHRRT;
                break;
            }
            break;
        }

        if (wheelCommand != CMD_NOOP) wheelCounter = (packet.header.arg1 & WHEEL_COUNT) - 1;
        return wheelCommand;
      }
    }
  }
}
