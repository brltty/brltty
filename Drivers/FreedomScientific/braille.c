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

typedef enum {
  PARM_DEBUGPACKETS,
  PARM_STATUSCELLS
} DriverParameter;
#define BRLPARMS "debugpackets", "statuscells"

#define BRLSTAT ST_AlvaStyle
#define BRL_HAVE_PACKET_IO
#define BRL_HAVE_FIRMNESS
#include "Programs/brl_driver.h"

typedef struct {
  int (*openPort) (char **parameters, const char *device);
  void (*closePort) (void);
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (void *buffer, int length);
  int (*writePacket) (const void *buffer, int length, int *delay);
} InputOutputOperations;

static const InputOutputOperations *io;
static int debugPackets = 0;
static int outputPayloadLimit;

#include "Programs/serial.h"
static int serialDevice = -1;
static struct termios oldSerialSettings;
static int serialCharactersPerSecond;

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
awaitSerialInput (int milliseconds) {
  return awaitInput(serialDevice, milliseconds);
}

static int
readSerialBytes (void *buffer, int length) {
  return read(serialDevice, buffer, length);
}

static int
writeSerialPacket (const void *buffer, int length, int *delay) {
  int written = safe_write(serialDevice, buffer, length);
  if (written == -1) LogError("Serial write");
  if (delay) *delay += length * 1000 / serialCharactersPerSecond;
  return written;
}

static const InputOutputOperations serialOperations = {
  openSerialPort, closeSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialPacket
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
    if (usbBeginInput(usbDevice, usbInputEndpoint, 8)) {
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
awaitUsbInput (int milliseconds) {
  return usbAwaitInput(usbDevice, usbInputEndpoint, milliseconds);
}

static int
readUsbBytes (void *buffer, int length) {
  int count = usbReapInput(usbDevice, usbInputEndpoint, buffer, length, 0);
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
  awaitUsbInput, readUsbBytes, writeUsbPacket
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
  PKT_ERR_TIMEOUT   = 0X30, /* no data received from host for a while */
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
  unsigned char statusCells;
  const DotsTable *dotsTable;
  int hotkeysRow;
} ModelEntry;

static const DotsTable dots12345678 = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
static const DotsTable dots12374568 = {0X01, 0X02, 0X04, 0X10, 0X20, 0X40, 0X08, 0X80};

static const ModelEntry modelTable[] = {
  {"Focus 44"     , 44, 3, &dots12374568, -1},
  {"Focus 70"     , 70, 3, &dots12374568, -1},
  {"Focus 84"     , 84, 3, &dots12374568, -1},
  {"pm display 20", 20, 0, &dots12345678,  1},
  {"pm display 40", 40, 0, &dots12345678,  1},
  {NULL           ,  0, 0, NULL         , -1}
};
static const ModelEntry *model;

static TranslationTable outputTable;
static unsigned char outputBuffer[84];

static unsigned char textOffset;
static unsigned char textCells;
static unsigned char statusOffset;
static unsigned char statusCells;

static int writeFrom;
static int writeTo;
static int writingFrom;
static int writingTo;

static int firmnessSetting;

static union {
  Packet packet;
  unsigned char bytes[sizeof(Packet)];
} inputBuffer;
static int inputCount;

static int realKeys;
static int virtualKeys;
static int pressedKeys;
static int activeKeys;
#define KEY_DOT1          0X00000001
#define KEY_DOT2          0X00000002
#define KEY_DOT3          0X00000004
#define KEY_DOT4          0X00000008
#define KEY_DOT5          0X00000010
#define KEY_DOT6          0X00000020
#define KEY_DOT7          0X00000040
#define KEY_DOT8          0X00000080
#define KEY_WHEEL_LEFT    0X00000100
#define KEY_WHEEL_RIGHT   0X00000200
#define KEY_SHIFT_LEFT    0X00000400
#define KEY_SHIFT_RIGHT   0X00000800
#define KEY_ADVANCE_LEFT  0X00001000
#define KEY_ADVANCE_RIGHT 0X00002000
#define KEY_SPACE         0X00008000
#define KEY_GDF_LEFT      0X00010000
#define KEY_GDF_RIGHT     0X00020000
#define KEY_HOT1          0X01000000
#define KEY_HOT2          0X02000000
#define KEY_HOT3          0X04000000
#define KEY_HOT4          0X08000000
#define KEY_HOT5          0X10000000
#define KEY_HOT6          0X20000000
#define KEY_HOT7          0X40000000
#define KEY_HOT8          0X80000000

#define DOT_KEYS (KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT4 | KEY_DOT5 | KEY_DOT6 | KEY_DOT7 | KEY_DOT8)
#define HOT_KEYS (KEY_HOT1 | KEY_HOT2 | KEY_HOT3 | KEY_HOT4 | KEY_HOT5 | KEY_HOT6 | KEY_HOT7 | KEY_HOT8)
#define SHIFT_KEYS (KEY_SHIFT_LEFT | KEY_SHIFT_RIGHT)

static int wheelCommand;
static int wheelCounter;
#define WHEEL_COUNT 0X07
#define WHEEL_DOWN  0X08
#define WHEEL_UNIT  0X30
#define WHEEL_LEFT  0X00
#define WHEEL_RIGHT 0X10

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

  if (debugPackets)
    LogBytes("Output Packet", (unsigned char *)&packet, size);
  return io->writePacket(&packet, size, &brl->writeDelay);
}

static void
logNegativeAcknowledgement (const Packet *packet) {
  const char *problem;
  const char *component;

  switch (packet->header.arg1) {
    default:
      problem = "unknown problem";
      break;
    case PKT_ERR_TIMEOUT:
      problem = "timeout during packet transmission";
      break;
    case PKT_ERR_CHECKSUM:
      problem = "incorrect checksum";
      break;
    case PKT_ERR_TYPE:
      problem = "unknown packet type";
      break;
    case PKT_ERR_PARAMETER:
      problem = "invalid parameter value";
      break;
    case PKT_ERR_SIZE:
      problem = "write size too large";
      break;
    case PKT_ERR_POSITION:
      problem = "write start too large";
      break;
    case PKT_ERR_OVERRUN:
      problem = "message FIFO overflow";
      break;
    case PKT_ERR_POWER:
      problem = "insufficient USB power";
      break;
    case PKT_ERR_SPI:
      problem = "SPI bus timeout";
      break;
  }

  switch (packet->header.arg2) {
    default:
      component = "unknown component";
      break;
    case PKT_EXT_HVADJ:
      component = "VariBraille packet";
      break;
    case PKT_EXT_BEEP:
      component = "beep packet";
      break;
    case PKT_EXT_CLEAR:
      component = "ClearMsgBuf function";
      break;
    case PKT_EXT_LOOP:
      component = "timing loop of ParseCommands function";
      break;
    case PKT_EXT_TYPE:
      component = "ParseCommands function";
      break;
    case PKT_EXT_CMDWRITE:
      component = "CmdWrite function";
      break;
    case PKT_EXT_UPDATE:
      component = "update packet";
      break;
    case PKT_EXT_DIAG:
      component = "diag packet";
      break;
    case PKT_EXT_QUERY:
      component = "query packet";
      break;
    case PKT_EXT_WRITE:
      component = "write packet";
      break;
  }

  LogPrint(LOG_WARNING, "Negative Acknowledgement: [%02X] %s in [%02X] %s",
           packet->header.arg1, problem,
           packet->header.arg2, component);
}

static void
handleFirmnessAcknowledgement (int ok) {
  firmnessSetting = -1;
}

static void
handleWriteAcknowledgement (int ok) {
  if (!ok) {
    if ((writeFrom == -1) || (writingFrom < writeFrom)) writeFrom = writingFrom;
    if ((writeTo == -1) || (writingTo > writeTo)) writeTo = writingTo;
  }
}

typedef void (*AcknowledgementHandler) (int ok);
static AcknowledgementHandler acknowledgementHandler;
static struct timeval acknowledgementTime;
static void
setAcknowledgementHandler (AcknowledgementHandler handler) {
  acknowledgementHandler = handler;
  gettimeofday(&acknowledgementTime, NULL);
}

static int
writeRequest (BrailleDisplay *brl) {
  if (!acknowledgementHandler) {
    if (firmnessSetting >= 0) {
      int size = writePacket(brl, PKT_HVADJ, firmnessSetting, 0, 0, NULL);
      if (size != -1) {
        setAcknowledgementHandler(handleFirmnessAcknowledgement);
      }
      return size;
    }

    if (writeTo != -1) {
      int size;
      int count = writeTo + 1 - writeFrom;
      int truncate = count > outputPayloadLimit;
      if (truncate) count = outputPayloadLimit;
      if ((size = writePacket(brl, PKT_WRITE, count, writeFrom, 0,
                              &outputBuffer[writeFrom])) != -1) {
        setAcknowledgementHandler(handleWriteAcknowledgement);
        writingFrom = writeFrom;
        if (truncate) {
          writingTo = (writeFrom += count) - 1;
        } else {
          writingTo = writeTo;
          writeFrom = -1;
          writeTo = -1;
        }
      }
      return size;
    }
  }
  return 0;
}

static void
updateCells (
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

static int
readPacket (BrailleDisplay *brl, Packet *packet) {
  while (1) {
    int size = sizeof(PacketHeader);
    int hasPayload = 0;

    if (inputCount >= sizeof(PacketHeader)) {
      if (inputBuffer.packet.header.type & 0X80) {
        hasPayload = 1;
        size += inputBuffer.packet.header.arg1 + 1;
      }
    }

    if (size <= inputCount) {
      if (debugPackets) LogBytes("Input Packet", inputBuffer.bytes, size);

      if (hasPayload) {
        unsigned char checksum = 0;
        int index;
        for (index=0; index<size; ++index)
          checksum -= inputBuffer.bytes[index];
        if (checksum)
          LogPrint(LOG_WARNING, "Input packet checksum error.");
      }

      memcpy(packet, &inputBuffer, size);
      memmove(&inputBuffer.bytes[0], &inputBuffer.bytes[size],
             inputCount -= size);
      return size;
    }

  retry:
    {
      int count = io->readBytes(&inputBuffer.bytes[inputCount], size-inputCount);
      if (count < 1) {
        if (count == -1) {
          LogError("read");
        } else if ((count == 0) && (inputCount > 0)) {
          if (io->awaitInput(1000)) goto retry;
          if (errno != EAGAIN) count = -1;
          LogBytes("Aborted Input", inputBuffer.bytes, inputCount);
          inputCount = 0;
        }
        return count;
      }

      if (!inputCount) {
        static const unsigned char packets[] = {
          PKT_ACK, PKT_NAK,
          PKT_KEY, PKT_BUTTON, PKT_WHEEL,
          PKT_INFO
        };
        int first;
        for (first=0; first<count; ++first)
          if (memchr(packets, inputBuffer.bytes[first], sizeof(packets)))
            break;
        if (first) {
          LogBytes("Discarded Input", inputBuffer.bytes, first);
          memmove(&inputBuffer.bytes[0], &inputBuffer.bytes[first], count-=first);
        }
      }

      if (debugPackets)
        LogBytes("Input Bytes", &inputBuffer.bytes[inputCount], count);
      inputCount += count;
    }
  }
}

static int
getPacket (BrailleDisplay *brl, Packet *packet) {
  while (1) {
    int count = readPacket(brl, packet);
    if (count > 0) {
      switch (packet->header.type) {
        {
          int ok;

        case PKT_NAK:
          logNegativeAcknowledgement(packet);
          if (!acknowledgementHandler) {
            LogPrint(LOG_WARNING, "Unexpected NAK.");
            continue;
          }

          switch (packet->header.arg1) {
            case PKT_ERR_TIMEOUT: {
              int originalLimit = outputPayloadLimit;
              if (outputPayloadLimit > model->totalCells)
                outputPayloadLimit = model->totalCells;
              if (outputPayloadLimit > 1)
                outputPayloadLimit--;
              if (outputPayloadLimit != originalLimit)
                LogPrint(LOG_WARNING, "Maximum payload length reduced from %d to %d.",
                         originalLimit, outputPayloadLimit);
              break;
            }
          }

        handleNegativeAcknowledgement:
          ok = 0;
          goto handleAcknowledgement;

        case PKT_ACK:
          if (!acknowledgementHandler) {
            LogPrint(LOG_WARNING, "Unexpected ACK.");
            continue;
          }

          ok = 1;
        handleAcknowledgement:
          acknowledgementHandler(ok);
          acknowledgementHandler = NULL;
          writeRequest(brl);
          continue;
        }
      }
    } else if ((count == 0) && acknowledgementHandler &&
               (millisecondsSince(&acknowledgementTime) > 500)) {
      LogPrint(LOG_WARNING, "Missing ACK; assuming NAK.");
      goto handleNegativeAcknowledgement;
    }
    return count;
  }
}

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "Freedom Scientific Driver");
  LogPrint(LOG_INFO, "   Copyright (C) 2004 by Dave Mielke <dave@mielke.cc>");
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  validateYesNo(&debugPackets, "debug packets", parameters[PARM_DEBUGPACKETS]);

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
  outputPayloadLimit = 0XFF;

  if (io->openPort(parameters, device)) {
    int tries = 0;

    while (io->awaitInput(10)) {
      Packet packet;
      int count = readPacket(brl, &packet);
      if (count == -1) goto failure;
    }
    if (errno != EAGAIN) goto failure;

    while (writePacket(brl, PKT_QUERY, 0, 0, 0, NULL) > 0) {
      int acknowledged = 0;
      model = NULL;

      while (io->awaitInput(100)) {
        Packet response;
        int count = readPacket(brl, &response);
        if (count == -1) goto failure;

        switch (response.header.type) {
          case PKT_INFO:
            model = modelTable;
            while (model->identifier) {
              if (strcmp(response.payload.info.model, model->identifier) == 0) break;
              ++model;
            }

            if (!model->identifier) {
              static ModelEntry generic;
              model = &generic;
              LogPrint(LOG_WARNING, "Detected unknown model: %s", response.payload.info.model);

              memset(&generic, 0, sizeof(generic));
              generic.identifier = "Generic";
              generic.totalCells = 20;
              generic.dotsTable = &dots12345678;
              generic.hotkeysRow = 1;

              {
                typedef struct {
                  const char *identifier;
                  const DotsTable *dotsTable;
                } ExceptionEntry;
                static const ExceptionEntry exceptionTable[] = {
                  {"Focus", &dots12374568},
                  {NULL   , NULL         }
                };
                const ExceptionEntry *exception = exceptionTable;
                while (exception->identifier) {
                  if (strncmp(response.payload.info.model, exception->identifier, strlen(exception->identifier)) == 0) {
                    generic.dotsTable = exception->dotsTable;
                    break;
                  }
                  exception++;
                }
              }

              {
                const char *word = strrchr(response.payload.info.model, ' ');
                if (word) {
                  int size;
                  if (isInteger(&size, ++word)) {
                    static char identifier[sizeof(response.payload.info.model)];
                    generic.totalCells = size;
                    snprintf(identifier, sizeof(identifier), "%s %d",
                             generic.identifier, generic.totalCells);
                    generic.identifier = identifier;
                  }
                }
              }
            }

            if (model) {
              makeOutputTable(model->dotsTable, &outputTable);
              textCells = model->totalCells;
              textOffset = statusOffset = 0;

              {
                int cells = model->statusCells;
                const char *word = parameters[PARM_STATUSCELLS];
                if (word && *word) {
                  int maximum = textCells / 2;
                  int minimum = -maximum;
                  int value;
                  if (validateInteger(&value, "status cells specification", word, &minimum, &maximum)) {
                    cells = value;
                  }
                }

                if (cells) {
                  if (cells < 0) {
                    statusOffset = textCells + cells;
                    cells = -cells;
                  } else {
                    textOffset = cells + 1;
                  }
                  textCells -= cells + 1;
                }
                statusCells = cells;
              }

              memset(outputBuffer, 0, model->totalCells);
              writeFrom = 0;
              writeTo = model->totalCells - 1;

              acknowledgementHandler = NULL;
              firmnessSetting = -1;

              realKeys = 0;
              virtualKeys = 0;
              pressedKeys = 0;
              activeKeys = 0;
              wheelCounter = 0;

              LogPrint(LOG_INFO, "Detected %s: text=%d, status=%d, firmware=%s",
                       model->identifier,
                       textCells, statusCells,
                       response.payload.info.firmware);
            }
            break;

          case PKT_ACK:
            acknowledged = 1;
            break;

          case PKT_NAK:
            logNegativeAcknowledgement(&response);
          default:
            acknowledged = 0;
            model = NULL;
            break;
        }

        if (acknowledged && model) {
          brl->x = textCells;
          brl->y = 1;
          return 1;
        }
      }
      if (errno != EAGAIN) break;

      if (++tries == 5) {
        LogPrint(LOG_WARNING, "No response from display.");
        break;
      }
    }

  failure:
    io->closePort();
  }
  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  while (io->awaitInput(100)) {
    Packet packet;
    int count = getPacket(brl, &packet);
    if (count == -1) break;
  }
  io->closePort();
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  updateCells(brl, brl->buffer, textCells, textOffset);
  writeRequest(brl);
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  updateCells(brl, status, statusCells, statusOffset);
}

static int
interpretKeys (void) {
  int keys = realKeys | virtualKeys;
  int press = (keys & pressedKeys) != keys;
  int command = CMD_NOOP;
  int flags = 0;

  pressedKeys = keys;
  if (press) {
    activeKeys = pressedKeys;
    flags |= VAL_REPEAT_DELAY;
  } else {
    keys = activeKeys;
    activeKeys = 0;
    if (!keys) return command;
  }

  {
    if ((keys & DOT_KEYS) && !(keys & ~(DOT_KEYS | SHIFT_KEYS))) {
      command = VAL_PASSDOTS | flags;
      if (keys & KEY_DOT1) command |= B1;
      if (keys & KEY_DOT2) command |= B2;
      if (keys & KEY_DOT3) command |= B3;
      if (keys & KEY_DOT4) command |= B4;
      if (keys & KEY_DOT5) command |= B5;
      if (keys & KEY_DOT6) command |= B6;
      if (keys & KEY_DOT7) command |= B7;
      if (keys & KEY_DOT8) command |= B8;
      if (keys & KEY_SHIFT_LEFT) command |= VPC_UPPER;
      if (keys & KEY_SHIFT_RIGHT) command |= VPC_CONTROL;
      return command;
    }
  }

  switch (keys) {
    default:
      break;

    case (KEY_WHEEL_LEFT):
      command = CMD_LNBEG;
      break;
    case (KEY_WHEEL_RIGHT):
      command = CMD_LNEND;
      break;

    case (KEY_GDF_LEFT):
      command = CMD_BACK;
      break;
    case (KEY_GDF_RIGHT):
      command = CMD_HOME;
      break;
    case (KEY_GDF_LEFT | KEY_GDF_RIGHT):
      command = CMD_PASTE;
      break;

    case (KEY_ADVANCE_LEFT):
      command = CMD_FWINLT;
      break;
    case (KEY_ADVANCE_RIGHT):
      command = CMD_FWINRT;
      break;
    case (KEY_GDF_LEFT | KEY_ADVANCE_LEFT):
      command = CMD_TOP_LEFT;
      break;
    case (KEY_GDF_LEFT | KEY_ADVANCE_RIGHT):
      command = CMD_BOT_LEFT;
      break;
    case (KEY_GDF_RIGHT | KEY_ADVANCE_LEFT):
      command = CMD_TOP;
      break;
    case (KEY_GDF_RIGHT | KEY_ADVANCE_RIGHT):
      command = CMD_BOT;
      break;

    case (KEY_SPACE | KEY_DOT2 | KEY_DOT4):
    case (KEY_HOT1):
      command = CMD_SKPIDLNS;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2):
    case (KEY_GDF_RIGHT | KEY_HOT1):
      command = CMD_SKPBLNKWINS;
      break;

    case (KEY_SPACE | KEY_DOT1):
    case (KEY_HOT2):
      command = CMD_DISPMD;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT3 | KEY_DOT6):
    case (KEY_GDF_RIGHT | KEY_HOT2):
      command = CMD_ATTRVIS;
      break;

    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT4 | KEY_DOT5):
    case (KEY_HOT3):
      command = CMD_CSRTRK;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT4):
    case (KEY_GDF_RIGHT | KEY_HOT3):
      command = CMD_CSRVIS;
      break;

    case (KEY_HOT4):
      command = CMD_SIXDOTS;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT5):
      command = CMD_SIXDOTS | VAL_TOGGLE_ON;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT6):
      command = CMD_SIXDOTS | VAL_TOGGLE_OFF;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT5):
    case (KEY_GDF_RIGHT | KEY_HOT4):
      command = CMD_AUTOREPEAT;
      break;

    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT5):
    case (KEY_HOT5):
      command = CMD_HELP;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT4):
    case (KEY_GDF_RIGHT | KEY_HOT5):
      command = CMD_FREEZE;
      break;

    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3):
    case (KEY_HOT6):
      command = CMD_LEARN;
      break;
    case (KEY_SPACE | KEY_SHIFT_LEFT | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT4):
    case (KEY_GDF_RIGHT | KEY_HOT6):
      command = CMD_PREFLOAD;
      break;

    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT4):
    case (KEY_HOT7):
      command = CMD_PREFMENU;
      break;
    case (KEY_SPACE | KEY_SHIFT_RIGHT | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT4):
    case (KEY_GDF_RIGHT | KEY_HOT7):
      command = CMD_PREFSAVE;
      break;

    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT4):
    case (KEY_HOT8):
      command = CMD_INFO;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT6):
    case (KEY_GDF_RIGHT | KEY_HOT8):
      command = CMD_CSRJMP_VERT;
      break;

    case (KEY_SPACE):
      command = VAL_PASSDOTS;
      break;
    case (KEY_SPACE | KEY_SHIFT_LEFT):
      command = VAL_PASSKEY + VPK_BACKSPACE;
      break;
    case (KEY_SPACE | KEY_SHIFT_RIGHT):
      command = VAL_PASSKEY + VPK_RETURN;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT5 | KEY_DOT6):
      command = VAL_PASSKEY + VPK_TAB;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3):
      command = VAL_PASSKEY + VPK_CURSOR_LEFT;
      break;
    case (KEY_SPACE | KEY_DOT5 | KEY_DOT6):
      command = VAL_PASSKEY + VPK_CURSOR_RIGHT;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT5):
      command = VAL_PASSKEY + VPK_CURSOR_UP;
      break;
    case (KEY_SPACE | KEY_DOT3 | KEY_DOT6):
      command = VAL_PASSKEY + VPK_CURSOR_DOWN;
      break;
    case (KEY_SPACE | KEY_DOT5):
      command = VAL_PASSKEY + VPK_PAGE_UP;
      break;
    case (KEY_SPACE | KEY_DOT6):
      command = VAL_PASSKEY + VPK_PAGE_DOWN;
      break;
    case (KEY_SPACE | KEY_DOT2):
      command = VAL_PASSKEY + VPK_HOME;
      break;
    case (KEY_SPACE | KEY_DOT3):
      command = VAL_PASSKEY + VPK_END;
      break;
    case (KEY_SPACE | KEY_DOT3 | KEY_DOT5):
      command = VAL_PASSKEY + VPK_INSERT;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT5 | KEY_DOT6):
      command = VAL_PASSKEY + VPK_DELETE;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT6):
      command = VAL_PASSKEY + VPK_ESCAPE;
      break;

    case (KEY_SPACE | KEY_SHIFT_LEFT | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT6):
      command = CMD_SWITCHVT_PREV;
      break;
    case (KEY_SPACE | KEY_SHIFT_RIGHT | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT6):
      command = CMD_SWITCHVT_NEXT;
      break;
  }

  if (command != CMD_NOOP) command |= flags;
  return command;
}

static int
brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds) {
  if (wheelCounter) {
    --wheelCounter;
    return wheelCommand;
  }

  while (1) {
    Packet packet;
    int count = getPacket(brl, &packet);
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

      case PKT_KEY:
        realKeys = packet.header.arg1 |
                   (packet.header.arg2 << 8) |
                   (packet.header.arg3 << 16);
        return interpretKeys();

      case PKT_BUTTON: {
        int button = packet.header.arg1;
        unsigned char press = packet.header.arg2 & 0X01;
        unsigned char row = packet.header.arg3;
        int command = CMD_NOOP;

        if (row == model->hotkeysRow) {
          static int keys[] = {
            KEY_GDF_LEFT,
            KEY_HOT1, KEY_HOT2, KEY_HOT3, KEY_HOT4,
            KEY_HOT5, KEY_HOT6, KEY_HOT7, KEY_HOT8,
            KEY_GDF_RIGHT
          };
          static const int keyCount = sizeof(keys) / sizeof(keys[0]);

          int key;
          button -= (model->totalCells - keyCount) / 2;

          if (button < 0) {
            key = KEY_ADVANCE_LEFT;
          } else if (button >= keyCount) {
            key = KEY_ADVANCE_RIGHT;
          } else {
            key = keys[button];
          }

          if (press) {
            virtualKeys |= key;
          } else {
            virtualKeys &= ~key;
          }
          return interpretKeys();
        }

        activeKeys = 0;
        if (press) {
          if ((button >= textOffset) && (button < (textOffset + textCells))) {
            button -= textOffset;
            switch (row) {
              default:
                break;

              case 0:
                switch (pressedKeys) {
                  default:
                    break;
                  case 0:
                    command = CR_ROUTE;
                    break;
                  case (KEY_ADVANCE_LEFT):
                    command = CR_CUTBEGIN;
                    break;
                  case (KEY_ADVANCE_RIGHT):
                    command = CR_CUTRECT;
                    break;
                  case (KEY_GDF_LEFT):
                    command = CR_CUTAPPEND;
                    break;
                  case (KEY_GDF_RIGHT):
                    command = CR_CUTLINE;
                    break;

                  case (KEY_SPACE):
                    command = VAL_PASSKEY + VPK_FUNCTION;
                    break;
                  case (KEY_SHIFT_RIGHT):
                    command = CR_SWITCHVT;
                    break;
                }
                break;

              case 1:
                switch (pressedKeys) {
                  default:
                    break;
                  case 0:
                    command = CR_DESCCHAR;
                    break;
                  case (KEY_ADVANCE_LEFT):
                    command = CR_PRINDENT;
                    break;
                  case (KEY_ADVANCE_RIGHT):
                    command = CR_NXINDENT;
                    break;
                  case (KEY_GDF_LEFT):
                    command = CR_SETLEFT;
                    break;
                }
                break;
            }
            if (command != CMD_NOOP) command += button;
          } else if ((button >= statusOffset) && (button < (statusOffset + statusCells))) {
            button -= statusOffset;
          }
        }

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
              case (KEY_ADVANCE_LEFT):
                wheelCommand = CMD_PRPROMPT;
                break;
              case (KEY_ADVANCE_RIGHT):
                wheelCommand = CMD_PRPGRPH;
                break;
              case (KEY_GDF_LEFT):
                wheelCommand = CMD_ATTRUP;
                break;
              case (KEY_GDF_RIGHT):
                wheelCommand = CMD_PRSEARCH;
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
              case (KEY_ADVANCE_LEFT):
                wheelCommand = CMD_NXPROMPT;
                break;
              case (KEY_ADVANCE_RIGHT):
                wheelCommand = CMD_NXPGRPH;
                break;
              case (KEY_GDF_LEFT):
                wheelCommand = CMD_ATTRDN;
                break;
              case (KEY_GDF_RIGHT):
                wheelCommand = CMD_NXSEARCH;
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

static ssize_t
brl_readPacket (BrailleDisplay *brl, unsigned char *buffer, size_t length) {
  Packet packet;
  int count = readPacket(brl, &packet);
  if (count > 0) {
    if (count > sizeof(packet.header)) count--;
    if (length < count) {
      LogPrint(LOG_WARNING, "Input packet buffer too small: %d < %d", length, count);
      count = length;
    }
    memcpy(buffer, &packet, count);
  }
  return count;
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const unsigned char *buffer, size_t length) {
  int size = 4;
  if (length >= size) {
    int hasPayload = 0;
    if (buffer[0] & 0X80) {
      size += buffer[1];
      hasPayload = 1;
    }
    if (length >= size) {
      if (length > size)
        LogPrint(LOG_WARNING, "Output packet buffer larger than necessary: %d > %d",
                 length, size);
      return writePacket(brl, buffer[0], buffer[1], buffer[2], buffer[3],
                         (hasPayload? &buffer[4]: NULL));
    }
  }
  LogPrint(LOG_WARNING, "Output packet buffer too small: %d < %d", length, size);
  return 0;
}

static int
brl_reset (BrailleDisplay *brl) {
  return 0;
}

static void
brl_firmness (BrailleDisplay *brl, int setting) {
  firmnessSetting = setting * 0XFF / BRL_MAXIMUM_FIRMNESS;
  writeRequest(brl);
}
