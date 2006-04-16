/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#include "prologue.h"

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
  int (*writePacket) (const void *buffer, int length, unsigned int *delay);
} InputOutputOperations;

static const InputOutputOperations *io;
static unsigned int debugPackets = 0;
static int outputPayloadLimit;

#include "Programs/io_serial.h"
static SerialDevice *serialDevice = NULL;
static int serialCharactersPerSecond;

static int
openSerialPort (char **parameters, const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    int baud = 57600;

    if (serialRestartDevice(serialDevice, baud)) {
      serialCharactersPerSecond = baud / 10;
      return 1;
    }

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
readSerialBytes (void *buffer, int length) {
  return serialReadData(serialDevice, buffer, length, 0, 0);
}

static int
writeSerialPacket (const void *buffer, int length, unsigned int *delay) {
  int written = serialWriteData(serialDevice, buffer, length);
  if (delay && (written != -1)) *delay += (length * 1000 / serialCharactersPerSecond) + 1;
  return written;
}

static const InputOutputOperations serialOperations = {
  openSerialPort, closeSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialPacket
};

#ifdef ENABLE_USB_SUPPORT
#include "Programs/io_usb.h"

static UsbChannel *usb = NULL;

static int
openUsbPort (char **parameters, const char *device) {
  static const UsbChannelDefinition definitions[] = {
    {0X0F4E, 0X0100, 1, 0, 0, 2, 1, 0}, /* Focus */
    {0X0F4E, 0X0111, 1, 0, 0, 2, 1, 0}, /* PAC Mate */
    {0X0F4E, 0X0112, 1, 0, 0, 2, 1, 0}, /* Focus 2 */
    {0}
  };

  if ((usb = usbFindChannel(definitions, (void *)device))) {
    usbBeginInput(usb->device, usb->definition.inputEndpoint, 8);
    return 1;
  }
  return 0;
}

static void
closeUsbPort (void) {
  if (usb) {
    usbCloseChannel(usb);
    usb = NULL;
  }
}

static int
awaitUsbInput (int milliseconds) {
  return usbAwaitInput(usb->device, usb->definition.inputEndpoint, milliseconds);
}

static int
readUsbBytes (void *buffer, int length) {
  int count = usbReapInput(usb->device, usb->definition.inputEndpoint, buffer, length, 0, 0);
  if (count == -1)
    if (errno == EAGAIN)
      count = 0;
  return count;
}

static int
writeUsbPacket (const void *buffer, int length, unsigned int *delay) {
  return usbWriteEndpoint(usb->device, usb->definition.outputEndpoint, buffer, length, 1000);
}

static const InputOutputOperations usbOperations = {
  openUsbPort, closeUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbPacket
};
#endif /* ENABLE_USB_SUPPORT */

typedef enum {
  PKT_QUERY  = 0X00, /* host->unit: request device information */
  PKT_ACK    = 0X01, /* unit->host: acknowledge packet receipt */
  PKT_NAK    = 0X02, /* unit->host: negative acknowledge, report error */
  PKT_KEY    = 0X03, /* unit->host: key event */
  PKT_BUTTON = 0X04, /* unit->host: routing button event */
  PKT_WHEEL  = 0X05, /* unit->host: whiz wheel event */
  PKT_HVADJ  = 0X08, /* host->unit: set braille display voltage */
  PKT_BEEP   = 0X09, /* host->unit: sound short beep */
  PKT_CONFIG = 0X0F, /* host->unit: configure device options */
  PKT_INFO   = 0X80, /* unit->host: response to query packet */
  PKT_WRITE  = 0X81, /* host->unit: write to braille display */
  PKT_EXTKEY = 0X82  /* unit->host: extended keys event */
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

typedef enum {
  OPT_EXTKEY = 0X01  /* send extended key events */
} UnitOption;

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
    struct {
      unsigned char bytes[4];
    } extkey;
  } payload;
} Packet;

typedef struct {
  const char *identifier;
  const DotsTable *dotsTable;
  unsigned char totalCells;
  unsigned char statusCells;
  signed char hotkeysRow;
} ModelEntry;

static const DotsTable dots12345678 = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
static const DotsTable dots12374568 = {0X01, 0X02, 0X04, 0X10, 0X20, 0X40, 0X08, 0X80};

static const ModelEntry modelTable[] = {
  {"Focus 40"     , &dots12345678, 40, 0, -1},
  {"Focus 44"     , &dots12374568, 44, 3, -1},
  {"Focus 70"     , &dots12374568, 70, 3, -1},
  {"Focus 80"     , &dots12345678, 80, 0, -1},
  {"Focus 84"     , &dots12374568, 84, 3, -1},
  {"pm display 20", &dots12345678, 20, 0,  1},
  {"pm display 40", &dots12345678, 40, 0,  1},
  {"pm display 80", &dots12345678, 80, 0,  1},
  {NULL           , NULL         ,  0, 0, -1}
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

static unsigned int realKeys;
static unsigned int virtualKeys;
static unsigned int pressedKeys;
static unsigned int activeKeys;
#define KEY_DOT1            0X00000001
#define KEY_DOT2            0X00000002
#define KEY_DOT3            0X00000004
#define KEY_DOT4            0X00000008
#define KEY_DOT5            0X00000010
#define KEY_DOT6            0X00000020
#define KEY_DOT7            0X00000040
#define KEY_DOT8            0X00000080
#define KEY_WHEEL_LEFT      0X00000100
#define KEY_WHEEL_RIGHT     0X00000200
#define KEY_SHIFT_LEFT      0X00000400
#define KEY_SHIFT_RIGHT     0X00000800
#define KEY_ADVANCE_LEFT    0X00001000
#define KEY_ADVANCE_RIGHT   0X00002000
#define KEY_SPACE           0X00008000
#define KEY_GDF_LEFT        0X00010000
#define KEY_GDF_RIGHT       0X00020000
#define KEY_BUMP_LEFT_UP    0X00100000
#define KEY_BUMP_LEFT_DOWN  0X00200000
#define KEY_BUMP_RIGHT_UP   0X00400000
#define KEY_BUMP_RIGHT_DOWN 0X00800000
#define KEY_HOT1            0X01000000
#define KEY_HOT2            0X02000000
#define KEY_HOT3            0X04000000
#define KEY_HOT4            0X08000000
#define KEY_HOT5            0X10000000
#define KEY_HOT6            0X20000000
#define KEY_HOT7            0X40000000
#define KEY_HOT8            0X80000000

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
static int acknowledgementsMissing;
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
      acknowledgementsMissing = 0;

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
      if (++acknowledgementsMissing < 5) {
        LogPrint(LOG_WARNING, "Missing ACK; assuming NAK.");
        goto handleNegativeAcknowledgement;
      }
      LogPrint(LOG_WARNING, "Too many missing ACKs.");
      count = -1;
    }
    return count;
  }
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  if (!validateYesNo(&debugPackets, parameters[PARM_DEBUGPACKETS]))
    LogPrint(LOG_WARNING, "%s: %s", "invalid debug packets setting", parameters[PARM_DEBUGPACKETS]);

  if (isSerialDevice(&device)) {
    io = &serialOperations;
  } else

#ifdef ENABLE_USB_SUPPORT
  if (isUsbDevice(&device)) {
    io = &usbOperations;
  } else
#endif /* ENABLE_USB_SUPPORT */

  {
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
              makeOutputTable(model->dotsTable[0], outputTable);
              textCells = model->totalCells;
              textOffset = statusOffset = 0;

              {
                int cells = model->statusCells;
                const char *word = parameters[PARM_STATUSCELLS];
                if (word && *word) {
                  int maximum = textCells / 2;
                  int minimum = -maximum;
                  int value;
                  if (validateInteger(&value, word, &minimum, &maximum)) {
                    cells = value;
                  } else {
                    LogPrint(LOG_WARNING, "%s: %s", "invalid status cells specification", word);
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
              acknowledgementsMissing = 0;
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

      if (++tries == 3) break;
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
  unsigned int keys = realKeys | virtualKeys;
  int press = (keys & pressedKeys) != keys;
  int command = BRL_CMD_NOOP;
  int flags = 0;

  pressedKeys = keys;
  if (press) {
    activeKeys = pressedKeys;
    flags |= BRL_FLG_REPEAT_DELAY;
  } else {
    keys = activeKeys;
    activeKeys = 0;
    if (!keys) return command;
  }

  {
    if ((keys & DOT_KEYS) && !(keys & ~(DOT_KEYS | SHIFT_KEYS))) {
      command = BRL_BLK_PASSDOTS | flags;
      if (keys & KEY_DOT1) command |= BRL_DOT1;
      if (keys & KEY_DOT2) command |= BRL_DOT2;
      if (keys & KEY_DOT3) command |= BRL_DOT3;
      if (keys & KEY_DOT4) command |= BRL_DOT4;
      if (keys & KEY_DOT5) command |= BRL_DOT5;
      if (keys & KEY_DOT6) command |= BRL_DOT6;
      if (keys & KEY_DOT7) command |= BRL_DOT7;
      if (keys & KEY_DOT8) command |= BRL_DOT8;
      if (keys & KEY_SHIFT_LEFT) command |= BRL_FLG_CHAR_UPPER;
      if (keys & KEY_SHIFT_RIGHT) command |= BRL_FLG_CHAR_CONTROL;
      return command;
    }
  }

  switch (keys) {
    default:
      break;

    case (KEY_WHEEL_LEFT):
      command = BRL_CMD_LNBEG;
      break;
    case (KEY_WHEEL_RIGHT):
      command = BRL_CMD_LNEND;
      break;

    case (KEY_GDF_LEFT):
      command = BRL_CMD_BACK;
      break;
    case (KEY_GDF_RIGHT):
      command = BRL_CMD_HOME;
      break;
    case (KEY_GDF_LEFT | KEY_GDF_RIGHT):
      command = BRL_CMD_PASTE;
      break;

    case (KEY_ADVANCE_LEFT):
      command = BRL_CMD_FWINLT;
      break;
    case (KEY_ADVANCE_RIGHT):
      command = BRL_CMD_FWINRT;
      break;
    case (KEY_GDF_LEFT | KEY_ADVANCE_LEFT):
      command = BRL_CMD_TOP_LEFT;
      break;
    case (KEY_GDF_LEFT | KEY_ADVANCE_RIGHT):
      command = BRL_CMD_BOT_LEFT;
      break;
    case (KEY_GDF_RIGHT | KEY_ADVANCE_LEFT):
      command = BRL_CMD_TOP;
      break;
    case (KEY_GDF_RIGHT | KEY_ADVANCE_RIGHT):
      command = BRL_CMD_BOT;
      break;

    case (KEY_SPACE | KEY_DOT2 | KEY_DOT4):
    case (KEY_HOT1):
      command = BRL_CMD_SKPIDLNS;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2):
    case (KEY_GDF_RIGHT | KEY_HOT1):
      command = BRL_CMD_SKPBLNKWINS;
      break;

    case (KEY_SPACE | KEY_DOT1):
    case (KEY_HOT2):
      command = BRL_CMD_DISPMD;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT3 | KEY_DOT6):
    case (KEY_GDF_RIGHT | KEY_HOT2):
      command = BRL_CMD_ATTRVIS;
      break;

    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT4 | KEY_DOT5):
    case (KEY_HOT3):
      command = BRL_CMD_CSRTRK;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT4):
    case (KEY_GDF_RIGHT | KEY_HOT3):
      command = BRL_CMD_CSRVIS;
      break;

    case (KEY_HOT4):
      command = BRL_CMD_SIXDOTS;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT5):
      command = BRL_CMD_SIXDOTS | BRL_FLG_TOGGLE_ON;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT6):
      command = BRL_CMD_SIXDOTS | BRL_FLG_TOGGLE_OFF;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT5):
    case (KEY_GDF_RIGHT | KEY_HOT4):
      command = BRL_CMD_AUTOREPEAT;
      break;

    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT5):
    case (KEY_HOT5):
      command = BRL_CMD_HELP;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT4):
    case (KEY_GDF_RIGHT | KEY_HOT5):
      command = BRL_CMD_FREEZE;
      break;

    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3):
    case (KEY_HOT6):
      command = BRL_CMD_LEARN;
      break;
    case (KEY_SPACE | KEY_SHIFT_LEFT | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT4):
    case (KEY_GDF_RIGHT | KEY_HOT6):
      command = BRL_CMD_PREFLOAD;
      break;

    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT4):
    case (KEY_HOT7):
      command = BRL_CMD_PREFMENU;
      break;
    case (KEY_SPACE | KEY_SHIFT_RIGHT | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT4):
    case (KEY_GDF_RIGHT | KEY_HOT7):
      command = BRL_CMD_PREFSAVE;
      break;

    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT4):
    case (KEY_HOT8):
      command = BRL_CMD_INFO;
      break;
    case (KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT6):
    case (KEY_GDF_RIGHT | KEY_HOT8):
      command = BRL_CMD_CSRJMP_VERT;
      break;

    case (KEY_SPACE):
      command = BRL_BLK_PASSDOTS;
      break;
    case (KEY_SPACE | KEY_SHIFT_LEFT):
      command = BRL_BLK_PASSKEY + BRL_KEY_BACKSPACE;
      break;
    case (KEY_SPACE | KEY_SHIFT_RIGHT):
      command = BRL_BLK_PASSKEY + BRL_KEY_ENTER;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT5 | KEY_DOT6):
      command = BRL_BLK_PASSKEY + BRL_KEY_TAB;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT3):
      command = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT;
      break;
    case (KEY_SPACE | KEY_DOT5 | KEY_DOT6):
      command = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT5):
      command = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP;
      break;
    case (KEY_SPACE | KEY_DOT3 | KEY_DOT6):
      command = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN;
      break;
    case (KEY_SPACE | KEY_DOT5):
      command = BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP;
      break;
    case (KEY_SPACE | KEY_DOT6):
      command = BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN;
      break;
    case (KEY_SPACE | KEY_DOT2):
      command = BRL_BLK_PASSKEY + BRL_KEY_HOME;
      break;
    case (KEY_SPACE | KEY_DOT3):
      command = BRL_BLK_PASSKEY + BRL_KEY_END;
      break;
    case (KEY_SPACE | KEY_DOT3 | KEY_DOT5):
      command = BRL_BLK_PASSKEY + BRL_KEY_INSERT;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT5 | KEY_DOT6):
      command = BRL_BLK_PASSKEY + BRL_KEY_DELETE;
      break;
    case (KEY_SPACE | KEY_DOT2 | KEY_DOT6):
      command = BRL_BLK_PASSKEY + BRL_KEY_ESCAPE;
      break;

    case (KEY_SPACE | KEY_SHIFT_LEFT | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT6):
      command = BRL_CMD_SWITCHVT_PREV;
      break;
    case (KEY_SPACE | KEY_SHIFT_RIGHT | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT6):
      command = BRL_CMD_SWITCHVT_NEXT;
      break;
  }

  if (command != BRL_CMD_NOOP) command |= flags;
  return command;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  if (wheelCounter) {
    --wheelCounter;
    return wheelCommand;
  }

  while (1) {
    Packet packet;
    int count = getPacket(brl, &packet);
    if (count == -1) return BRL_CMD_RESTARTBRL;
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
        int command = BRL_CMD_NOOP;

        if (row == model->hotkeysRow) {
          static unsigned int keys[] = {
            KEY_GDF_LEFT,
            KEY_HOT1, KEY_HOT2, KEY_HOT3, KEY_HOT4,
            KEY_HOT5, KEY_HOT6, KEY_HOT7, KEY_HOT8,
            KEY_GDF_RIGHT
          };
          static const int keyCount = ARRAY_COUNT(keys);

          unsigned int key;
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
                    command = BRL_BLK_ROUTE;
                    break;
                  case (KEY_ADVANCE_LEFT):
                    command = BRL_BLK_CUTBEGIN;
                    break;
                  case (KEY_ADVANCE_RIGHT):
                    command = BRL_BLK_CUTRECT;
                    break;
                  case (KEY_GDF_LEFT):
                    command = BRL_BLK_CUTAPPEND;
                    break;
                  case (KEY_GDF_RIGHT):
                    command = BRL_BLK_CUTLINE;
                    break;

                  case (KEY_SPACE):
                    command = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION;
                    break;
                  case (KEY_SHIFT_RIGHT):
                    command = BRL_BLK_SWITCHVT;
                    break;
                }
                break;

              case 1:
                switch (pressedKeys) {
                  default:
                    break;
                  case 0:
                    command = BRL_BLK_DESCCHAR;
                    break;
                  case (KEY_ADVANCE_LEFT):
                    command = BRL_BLK_PRINDENT;
                    break;
                  case (KEY_ADVANCE_RIGHT):
                    command = BRL_BLK_NXINDENT;
                    break;
                  case (KEY_GDF_LEFT):
                    command = BRL_BLK_SETLEFT;
                    break;
                }
                break;
            }
            if (command != BRL_CMD_NOOP) command += button;
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
        wheelCommand = BRL_CMD_NOOP;
        switch (motion) {
          default:
            break;

          case (WHEEL_LEFT):
            switch (pressedKeys) {
              default:
                break;
              case 0:
                wheelCommand = BRL_CMD_LNUP;
                break;
              case (KEY_WHEEL_LEFT):
                wheelCommand = BRL_CMD_PRDIFLN;
                break;
              case (KEY_ADVANCE_LEFT):
                wheelCommand = BRL_CMD_PRPROMPT;
                break;
              case (KEY_ADVANCE_RIGHT):
                wheelCommand = BRL_CMD_PRPGRPH;
                break;
              case (KEY_GDF_LEFT):
                wheelCommand = BRL_CMD_ATTRUP;
                break;
              case (KEY_GDF_RIGHT):
                wheelCommand = BRL_CMD_PRSEARCH;
                break;
            }
            break;

          case (WHEEL_LEFT | WHEEL_DOWN):
            switch (pressedKeys) {
              default:
                break;
              case 0:
                wheelCommand = BRL_CMD_LNDN;
                break;
              case (KEY_WHEEL_LEFT):
                wheelCommand = BRL_CMD_NXDIFLN;
                break;
              case (KEY_ADVANCE_LEFT):
                wheelCommand = BRL_CMD_NXPROMPT;
                break;
              case (KEY_ADVANCE_RIGHT):
                wheelCommand = BRL_CMD_NXPGRPH;
                break;
              case (KEY_GDF_LEFT):
                wheelCommand = BRL_CMD_ATTRDN;
                break;
              case (KEY_GDF_RIGHT):
                wheelCommand = BRL_CMD_NXSEARCH;
                break;
            }
            break;

          case (WHEEL_RIGHT):
            switch (pressedKeys) {
              default:
                break;
              case 0:
                wheelCommand = BRL_CMD_FWINLT;
                break;
              case (KEY_WHEEL_RIGHT):
                wheelCommand = BRL_CMD_CHRLT;
                break;
            }
            break;

          case (WHEEL_RIGHT | WHEEL_DOWN):
            switch (pressedKeys) {
              default:
                break;
              case 0:
                wheelCommand = BRL_CMD_FWINRT;
                break;
              case (KEY_WHEEL_RIGHT):
                wheelCommand = BRL_CMD_CHRRT;
                break;
            }
            break;
        }

        if (wheelCommand != BRL_CMD_NOOP) wheelCounter = (packet.header.arg1 & WHEEL_COUNT) - 1;
        return wheelCommand;
      }
    }
  }
}

static ssize_t
brl_readPacket (BrailleDisplay *brl, void *buffer, size_t length) {
  Packet packet;
  int count = readPacket(brl, &packet);
  if (count > 0) {
    if (count > sizeof(packet.header)) count--;
    if (length < count) {
      LogPrint(LOG_WARNING, "Input packet buffer too small: %d < %d", (int)length, count);
      count = length;
    }
    memcpy(buffer, &packet, count);
  }
  return count;
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const void *packet, size_t length) {
  const unsigned char *bytes = packet;
  int size = 4;
  if (length >= size) {
    int hasPayload = 0;
    if (bytes[0] & 0X80) {
      size += bytes[1];
      hasPayload = 1;
    }
    if (length >= size) {
      if (length > size)
        LogPrint(LOG_WARNING, "Output packet buffer larger than necessary: %d > %d",
                 (int)length, size);
      return writePacket(brl, bytes[0], bytes[1], bytes[2], bytes[3],
                         (hasPayload? &bytes[4]: NULL));
    }
  }
  LogPrint(LOG_WARNING, "Output packet buffer too small: %d < %d", (int)length, size);
  return 0;
}

static int
brl_reset (BrailleDisplay *brl) {
  return 0;
}

static void
brl_firmness (BrailleDisplay *brl, BrailleFirmness setting) {
  firmnessSetting = setting * 0XFF / BF_MAXIMUM;
  writeRequest(brl);
}
