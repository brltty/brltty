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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "Programs/misc.h"

typedef enum {
  PARM_INPUTMODE
} DriverParameter;
#define BRLPARMS "inputmode"

#define BRLSTAT ST_AlvaStyle
#define BRL_HAVE_PACKET_IO
#include "Programs/brl_driver.h"
#include "braille.h"

/* packets */
static const unsigned char HandyBrailleBegin[] = {0X01};	/* general header to display braille */
static const unsigned char HandyME6Begin[] = {0X79, 0X36, 0X41, 0X01};	/* general header to display braille */
static const unsigned char HandyME8Begin[] = {0X79, 0X38, 0X59, 0X01};	/* general header to display braille */
static const unsigned char BookwormBrailleEnd[] = {0X16};	/* bookworm trailer to display braille */
static const unsigned char BookwormSessionEnd[] = {0X05, 0X07};	/* bookworm trailer to display braille */

typedef struct {
  unsigned long int front;
  signed char column;
  signed char status;
} Keys;
static Keys currentKeys, pressedKeys;
static const Keys nullKeys = {.front=0, .column=-1, .status=-1};
static unsigned int inputMode;

/* Braille display parameters */
typedef int (ByteInterpreter) (BRL_DriverCommandContext context, unsigned char byte, int *command);
static ByteInterpreter interpretKeyByte;
static ByteInterpreter interpretBookwormByte;
typedef int (KeysInterpreter) (BRL_DriverCommandContext context, const Keys *keys, int *command);
static KeysInterpreter interpretModularKeys;
static KeysInterpreter interpretBrailleWaveKeys;
static KeysInterpreter interpretBrailleStarKeys;
typedef struct {
  const char *name;
  ByteInterpreter *interpretByte;
  KeysInterpreter *interpretKeys;

  const unsigned char *brailleBeginAddress;
  const unsigned char *brailleEndAddress;
  const unsigned char *sessionEndAddress;

  unsigned char identifier;
  unsigned char textCells;
  unsigned char statusCells;
  unsigned char helpPage;

  unsigned char brailleBeginLength;
  unsigned char brailleEndLength;
  unsigned char sessionEndLength;

  unsigned hasATC:1; /* Active Tactile Control */
} ModelEntry;

#define HT_BYTE_SEQUENCE(name,bytes) .name##Address = bytes, .name##Length = sizeof(bytes)
static const ModelEntry modelTable[] = {
  { .identifier = 0X80,
    .name = "Modular 20+4",
    .textCells = 20,
    .statusCells = 4,
    .helpPage = 0,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretModularKeys,
    HT_BYTE_SEQUENCE(brailleBegin, HandyBrailleBegin)
  }
  ,
  { .identifier = 0X89,
    .name = "Modular 40+4",
    .textCells = 40,
    .statusCells = 4,
    .helpPage = 0,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretModularKeys,
    HT_BYTE_SEQUENCE(brailleBegin, HandyBrailleBegin)
  }
  ,
  { .identifier = 0X88,
    .name = "Modular 80+4",
    .textCells = 80,
    .statusCells = 4,
    .helpPage = 0,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretModularKeys,
    HT_BYTE_SEQUENCE(brailleBegin, HandyBrailleBegin)
  }
  ,
  { .identifier = 0X36,
    .name = "Modular Evolution 64",
    .textCells = 60,
    .statusCells = 4,
    .helpPage = 0,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleStarKeys,
    .hasATC = 1,
    HT_BYTE_SEQUENCE(brailleBegin, HandyME6Begin),
    HT_BYTE_SEQUENCE(brailleEnd, BookwormBrailleEnd)
  }
  ,
  { .identifier = 0X38,
    .name = "Modular Evolution 88",
    .textCells = 80,
    .statusCells = 8,
    .helpPage = 0,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleStarKeys,
    .hasATC = 1,
    HT_BYTE_SEQUENCE(brailleBegin, HandyME8Begin),
    HT_BYTE_SEQUENCE(brailleEnd, BookwormBrailleEnd)
  }
  ,
  { .identifier = 0X05,
    .name = "Braille Wave 40",
    .textCells = 40,
    .statusCells = 0,
    .helpPage = 0,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleWaveKeys,
    HT_BYTE_SEQUENCE(brailleBegin, HandyBrailleBegin)
  }
  ,
  { .identifier = 0X90,
    .name = "Bookworm",
    .textCells = 8,
    .statusCells = 0,
    .helpPage = 1,
    .interpretByte = interpretBookwormByte,
    HT_BYTE_SEQUENCE(brailleBegin, HandyBrailleBegin),
    HT_BYTE_SEQUENCE(brailleEnd, BookwormBrailleEnd),
    HT_BYTE_SEQUENCE(sessionEnd, BookwormSessionEnd)
  }
  ,
  { .identifier = 0X72,
    .name = "Braillino 20",
    .textCells = 20,
    .statusCells = 0,
    .helpPage = 2,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleStarKeys,
    HT_BYTE_SEQUENCE(brailleBegin, HandyBrailleBegin)
  }
  ,
  { .identifier = 0X74,
    .name = "Braille Star 40",
    .textCells = 40,
    .statusCells = 0,
    .helpPage = 2,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleStarKeys,
    HT_BYTE_SEQUENCE(brailleBegin, HandyBrailleBegin)
  }
  ,
  { .identifier = 0X78,
    .name = "Braille Star 80",
    .textCells = 80,
    .statusCells = 0,
    .helpPage = 3,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleStarKeys,
    HT_BYTE_SEQUENCE(brailleBegin, HandyBrailleBegin)
  }
  ,
  { /* end of table */
    .name = NULL
  }
};
#undef HT_BYTE_SEQUENCE

#define BRLROWS		1
#define MAX_STCELLS	8	/* highest number of status cells */



/* This is the brltty braille mapping standard to Handy's mapping table.
 */
static TranslationTable outputTable;

/* Global variables */
static unsigned char *rawData = NULL;		/* translated data to send to Braille */
static unsigned char *prevData = NULL;	/* previously sent raw data */
static unsigned char rawStatus[MAX_STCELLS];		/* to hold status info */
static unsigned char prevStatus[MAX_STCELLS];	/* to hold previous status */
static const ModelEntry *model;		/* points to terminal model config struct */

static unsigned char *at2Buffer;
static int at2Size;
static int at2Count;

typedef struct {
  int (*openPort) (char **parameters, const char *device);
  void (*closePort) ();
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, int length, int wait);
  int (*writeBytes) (const unsigned char *buffer, int length, unsigned int *delay);
} InputOutputOperations;

static const InputOutputOperations *io;
static const int baud = 19200;
static int charactersPerSecond;

/* Serial IO */
#include "Programs/io_serial.h"

static SerialDevice *serialDevice = NULL;			/* file descriptor for Braille display */

static int
openSerialPort (char **parameters, const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    serialSetParity(serialDevice, SERIAL_PARITY_ODD);

    if (serialRestartDevice(serialDevice, baud)) {
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
writeSerialBytes (const unsigned char *buffer, int length, unsigned int *delay) {
  int count = serialWriteData(serialDevice, buffer, length);
  if (delay && (count != -1)) *delay += (length * 1000 / charactersPerSecond) + 1;
  return count;
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
#include "Programs/io_usb.h"

static UsbChannel *usb = NULL;

static int
openUsbPort (char **parameters, const char *device) {
  const UsbChannelDefinition definitions[] = {
    {0X0921, 0X1200, 1, 0, 0, 1, 1, baud, 0, 8, 1, SERIAL_PARITY_ODD}, /* GoHubs chip */
    {0X0403, 0X6001, 1, 0, 0, 1, 2, baud, 0, 8, 1, SERIAL_PARITY_ODD}, /* FTDI chip */
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
writeUsbBytes (const unsigned char *buffer, int length, unsigned int *delay) {
  if (delay) *delay += (length * 1000 / charactersPerSecond) + 1;
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
#include "Programs/io_bluetooth.h"
#include "Programs/io_misc.h"

static int bluetoothConnection = -1;

static int
openBluetoothPort (char **parameters, const char *device) {
  return (bluetoothConnection = btOpenConnection(device, 1, 0)) != -1;
}

static int
awaitBluetoothInput (int milliseconds) {
  return awaitInput(bluetoothConnection, milliseconds);
}

static int
readBluetoothBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  if (!awaitInput(bluetoothConnection, (wait? timeout: 0)))
    return (errno == EAGAIN)? 0: -1;
  return readData(bluetoothConnection, buffer, length, 0, timeout);
}

static int
writeBluetoothBytes (const unsigned char *buffer, int length, unsigned int *delay) {
  int count = writeData(bluetoothConnection, buffer, length);
  if (delay) *delay += (length * 1000 / charactersPerSecond) + 1;
  if (count != length) {
    if (count == -1) {
      LogError("HandyTech Bluetooth write");
    } else {
      LogPrint(LOG_WARNING, "Trunccated bluetooth write: %d < %d", count, length);
    }
  }
  return count;
}

static void
closeBluetoothPort (void) {
  close(bluetoothConnection);
  bluetoothConnection = -1;
}

static const InputOutputOperations bluetoothOperations = {
  openBluetoothPort, closeBluetoothPort,
  awaitBluetoothInput, readBluetoothBytes, writeBluetoothBytes
};
#endif /* ENABLE_BLUETOOTH_SUPPORT */

typedef union {
  unsigned char bytes[4 + 0XFF];

  struct {
    unsigned char type;

    union {
      struct {
        unsigned char model;
      } PACKED ok;

      struct {
        unsigned char model;
        unsigned char length;
        unsigned char type;

        union {
          unsigned char bytes[0XFF];
        } data;
      } PACKED extended;
    } data;
  } PACKED fields;
} HT_Packet;

typedef enum {
  HT_PKT_Extended = 0X79,
  HT_PKT_NAK      = 0X7D,
  HT_PKT_ACK      = 0X7E,
  HT_PKT_OK       = 0XFE,
  HT_PKT_Reset    = 0XFF
} HT_PacketType;

typedef enum {
  BDS_OFF,
  BDS_READY,
  BDS_WRITING
} BrailleDisplayState;
static BrailleDisplayState currentState = BDS_OFF;
static struct timeval stateTime;
static unsigned int retryCount = 0;
static unsigned char updateRequired = 0;

/* common key constants */
#define KEY_RELEASE 0X80
#define KEY_ROUTING 0X20
#define KEY_STATUS  0X70
#define KEY(code)   (1U << (code))

/* modular front keys */
#define KEY_B1              KEY(0X03)
#define KEY_B2              KEY(0X07)
#define KEY_B3              KEY(0X0B)
#define KEY_B4              KEY(0X0F)
#define KEY_B5              KEY(0X13)
#define KEY_B6              KEY(0X17)
#define KEY_B7              KEY(0X1B)
#define KEY_B8              KEY(0X1F)
#define KEY_UP              KEY(0X04)
#define KEY_DOWN            KEY(0X08)

/* modular keypad keys */
#define KEY_B12             KEY(0X01)
#define KEY_ZERO            KEY(0X05)
#define KEY_B13             KEY(0X09)
#define KEY_B14             KEY(0X0D)
#define KEY_B11             KEY(0X11)
#define KEY_ONE             KEY(0X15)
#define KEY_TWO             KEY(0X19)
#define KEY_THREE           KEY(0X1D)
#define KEY_B10             KEY(0X02)
#define KEY_FOUR            KEY(0X06)
#define KEY_FIVE            KEY(0X0A)
#define KEY_SIX             KEY(0X0E)
#define KEY_B9              KEY(0X12)
#define KEY_SEVEN           KEY(0X16)
#define KEY_EIGHT           KEY(0X1A)
#define KEY_NINE            KEY(0X1E)

/* braille wave keys */
#define KEY_ESCAPE          KEY(0X0C)
#define KEY_SPACE           KEY(0X10)
#define KEY_RETURN          KEY(0X14)

/* braille star keys */
#define KEY_SPACE_LEFT      KEY_SPACE
#define KEY_SPACE_RIGHT     KEY(0X18)
#define ROCKER_LEFT_TOP     KEY_ESCAPE
#define ROCKER_LEFT_BOTTOM  KEY_RETURN
#define ROCKER_LEFT_MIDDLE  (ROCKER_LEFT_TOP | ROCKER_LEFT_BOTTOM)
#define ROCKER_RIGHT_TOP    KEY_UP
#define ROCKER_RIGHT_BOTTOM KEY_DOWN
#define ROCKER_RIGHT_MIDDLE (ROCKER_RIGHT_TOP | ROCKER_RIGHT_BOTTOM)

/* bookworm keys */
#define BWK_BACKWARD 0X01
#define BWK_ESCAPE   0X02
#define BWK_ENTER    0X04
#define BWK_FORWARD  0X08

static void
setState (BrailleDisplayState state) {
  if (state == currentState) {
    ++retryCount;
  } else {
    retryCount = 0;
    currentState = state;
  }
  gettimeofday(&stateTime, NULL);
  // LogPrint(LOG_DEBUG, "State: %d+%d", currentState, retryCount);
}

static int
brl_reset (BrailleDisplay *brl) {
  static const unsigned char packet[] = {HT_PKT_Reset};
  return io->writeBytes(packet, sizeof(packet), &brl->writeDelay) != -1;
}

static void
deallocateBuffers (void) {
  if (rawData) {
    free(rawData);
    rawData = NULL;
  }

  if (prevData) {
    free(prevData);
    prevData = NULL;
  }
}

static int
reallocateBuffer (unsigned char **buffer, size_t size) {
  void *address = realloc(*buffer, size);
  int allocated = address != NULL;
  if (allocated) {
    *buffer = address;
  } else {
    LogError("buffer allocation");
  }
  return allocated;
}

static int
identifyModel (BrailleDisplay *brl, unsigned char identifier) {
  for (
    model = modelTable;
    model->name && (model->identifier != identifier);
    model++
  );

  if (!model->name) {
    LogPrint(LOG_ERR, "Detected unknown HandyTech model with ID %02X.",
             identifier);
    LogPrint(LOG_WARNING, "Please fix modelTable[] in HandyTech/braille.c and notify the maintainer.");
    return 0;
  }

  LogPrint(LOG_INFO, "Detected %s: %d data %s, %d status %s.",
           model->name,
           model->textCells, (model->textCells == 1)? "cell": "cells",
           model->statusCells, (model->statusCells == 1)? "cell": "cells");

  brl->helpPage = model->helpPage;		/* position in the model list */
  brl->x = model->textCells;			/* initialise size of display */
  brl->y = BRLROWS;

  if (!reallocateBuffer(&rawData, brl->x*brl->y)) return 0;
  if (!reallocateBuffer(&prevData, brl->x*brl->y)) return 0;

  currentKeys = pressedKeys = nullKeys;
  inputMode = 0;

  memset(rawStatus, 0, model->statusCells);
  memset(rawData, 0, model->textCells);

  retryCount = 0;
  updateRequired = 0;
  currentState = BDS_OFF;
  setState(BDS_READY);

  return 1;
}

static void
setAtcMode (BrailleDisplay *brl, unsigned char value) {
  const unsigned char packet[] = {
    HT_PKT_Extended, model->identifier, 0X02, 0X50, value, 0X16
  };
  brl_writePacket(brl, packet, sizeof(packet));
}

static void
setAtcSensitivity (BrailleDisplay *brl, unsigned char value) {
  const unsigned char packet[] = {
    HT_PKT_Extended, model->identifier, 0X02, 0X51, value, 0X16
  };
  brl_writePacket(brl, packet, sizeof(packet));
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  at2Buffer = NULL;
  at2Size = 0;
  at2Count = 0;

  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
  }

  if (isSerialDevice(&device)) {
    io = &serialOperations;
  } else

#ifdef ENABLE_USB_SUPPORT
  if (isUsbDevice(&device)) {
    io = &usbOperations;
  } else
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
  if (isBluetoothDevice(&device)) {
    io = &bluetoothOperations;
  } else
#endif /* ENABLE_BLUETOOTH_SUPPORT */

  {
    unsupportedDevice(device);
    return 0;
  }

  rawData = prevData = NULL;		/* clear pointers */
  charactersPerSecond = baud / 11;

  if (io->openPort(parameters, device)) {
    int tries = 0;
    while (brl_reset(brl)) {
      while (io->awaitInput(100)) {
        HT_Packet response;
        if (brl_readPacket(brl, &response, sizeof(response)) > 0) {
          if (response.fields.type == HT_PKT_OK) {
            if (identifyModel(brl, response.fields.data.ok.model)) {
              if (model->hasATC) {
		setAtcMode(brl, 1);
              }

              if (*parameters[PARM_INPUTMODE])
                if (!validateYesNo(&inputMode, parameters[PARM_INPUTMODE]))
                  LogPrint(LOG_WARNING, "%s: %s", "invalid input setting", parameters[PARM_INPUTMODE]);

              return 1;
            }
            deallocateBuffers();
          }
        }
      }
      if (errno != EAGAIN) break;

      if (++tries == 3) break;
    }

    io->closePort();
  }
  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  if (model->sessionEndLength) {
    io->writeBytes(model->sessionEndAddress, model->sessionEndLength, NULL);
  }
  io->closePort();

  if (at2Buffer) {
    free(at2Buffer);
    at2Buffer = NULL;
  }

  deallocateBuffers();
}

static int
writeBrailleCells (BrailleDisplay *brl) {
  unsigned char buffer[model->brailleBeginLength + model->statusCells + model->textCells + model->brailleEndLength];
  int count = 0;

  if (model->brailleBeginLength) {
    memcpy(buffer+count, model->brailleBeginAddress, model->brailleBeginLength);
    count += model->brailleBeginLength;
  }

  memcpy(buffer+count, rawStatus, model->statusCells);
  count += model->statusCells;

  memcpy(buffer+count, rawData, model->textCells);
  count += model->textCells;

  if (model->brailleEndLength) {
    memcpy(buffer+count, model->brailleEndAddress, model->brailleEndLength);
    count += model->brailleEndLength;
  }

  // LogBytes("Write", buffer, count);
  return (io->writeBytes(buffer, count, &brl->writeDelay) != -1);
}

static int
updateBrailleCells (BrailleDisplay *brl) {
  if (!updateRequired) return 1;
  if (currentState != BDS_READY) return 1;

  if (!writeBrailleCells(brl)) {
    setState(BDS_OFF);
    return 0;
  }

  setState(BDS_WRITING);
  updateRequired = 0;
  return 1;
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  if (memcmp(brl->buffer, prevData, model->textCells) != 0) {
    int i;
    for (i=0; i<model->textCells; ++i) {
      rawData[i] = outputTable[(prevData[i] = brl->buffer[i])];
    }
    updateRequired = 1;
  }
  updateBrailleCells(brl);
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *st) {
  if (memcmp(st, prevStatus, model->statusCells) != 0) {	/* only if it changed */
    int i;
    for (i=0; i<model->statusCells; ++i) {
      rawStatus[i] = outputTable[(prevStatus[i] = st[i])];
    }
    updateRequired = 1;
  }
}

static int
interpretKeyByte (BRL_DriverCommandContext context, unsigned char byte, int *command) {
  int release = (byte & KEY_RELEASE) != 0;
  if (release) byte &= ~KEY_RELEASE;

  currentKeys.column = -1;
  currentKeys.status = -1;

  if ((byte >= KEY_ROUTING) &&
      (byte < (KEY_ROUTING + model->textCells))) {
    *command = BRL_CMD_NOOP;
    if (!release) {
      currentKeys.column = byte - KEY_ROUTING;
      if (model->interpretKeys(context, &currentKeys, command)) {
        pressedKeys = nullKeys;
      }
    }
    return 1;
  }

  if ((byte >= KEY_STATUS) &&
      (byte < (KEY_STATUS + model->statusCells))) {
    *command = BRL_CMD_NOOP;
    if (!release) {
      currentKeys.status = byte - KEY_STATUS;
      if (model->interpretKeys(context, &currentKeys, command)) {
        pressedKeys = nullKeys;
      }
    }
    return 1;
  }

  if (byte < 0X20) {
    unsigned long int key = KEY(byte);
    *command = BRL_CMD_NOOP;
    if (release) {
      currentKeys.front &= ~key;
      if (pressedKeys.front) {
        model->interpretKeys(context, &pressedKeys, command);
        pressedKeys = nullKeys;
      }
    } else {
      currentKeys.front |= key;
      pressedKeys = currentKeys;
      if (model->interpretKeys(context, &currentKeys, command)) {
        *command |= BRL_FLG_REPEAT_DELAY;
      }
    }
    return 1;
  }

  return 0;
}

static int
interpretModularKeys (BRL_DriverCommandContext context, const Keys *keys, int *command) {
  if (keys->column >= 0) {
    switch (keys->front) {
      default:
        break;
      case 0:
        *command = BRL_BLK_ROUTE + keys->column;
        return 1;
      case (KEY_B1):
        *command = BRL_BLK_SETLEFT + keys->column;
        return 1;
      case (KEY_B2):
        *command = BRL_BLK_DESCCHAR + keys->column;
        return 1;
      case (KEY_B3):
        *command = BRL_BLK_CUTAPPEND + keys->column;
        return 1;
      case (KEY_B4):
        *command = BRL_BLK_CUTBEGIN + keys->column;
        return 1;
      case (KEY_UP):
        *command = BRL_BLK_PRINDENT + keys->column;
        return 1;
      case (KEY_DOWN):
        *command = BRL_BLK_NXINDENT + keys->column;
        return 1;
      case (KEY_B5):
        *command = BRL_BLK_CUTRECT + keys->column;
        return 1;
      case (KEY_B6):
        *command = BRL_BLK_CUTLINE + keys->column;
        return 1;
      case (KEY_B7):
        *command = BRL_BLK_SETMARK + keys->column;
        return 1;
      case (KEY_B8):
        *command = BRL_BLK_GOTOMARK + keys->column;
        return 1;
    }
  } else if (keys->status >= 0) {
    switch (keys->status) {
      default:
        break;
      case 0:
        *command = BRL_CMD_HELP;
        return 1;
      case 1:
        *command = BRL_CMD_PREFMENU;
        return 1;
      case 2:
        *command = BRL_CMD_INFO;
        return 1;
      case 3:
        *command = BRL_CMD_FREEZE;
        return 1;
    }
  } else {
    switch (keys->front) {
      default:
        break;
      case (KEY_B1 | KEY_B8 | KEY_UP):
        inputMode = 0;
        *command = EOF;
        return 1;
      case (KEY_B1 | KEY_B8 | KEY_DOWN):
        inputMode = 1;
        *command = EOF;
        return 1;
      case (KEY_B9):
        *command = BRL_CMD_SAY_ABOVE;
        return 1;
      case (KEY_B10):
        *command = BRL_CMD_SAY_LINE;
        return 1;
      case (KEY_B11):
        *command = BRL_CMD_SAY_BELOW;
        return 1;
      case (KEY_B12):
        *command = BRL_CMD_MUTE;
        return 1;
      case (KEY_ZERO):
        *command = BRL_CMD_SPKHOME;
        return 1;
      case (KEY_B13):
        *command = BRL_CMD_SWITCHVT_PREV;
        return 1;
      case (KEY_B14):
        *command = BRL_CMD_SWITCHVT_NEXT;
        return 1;
      case (KEY_SEVEN):
        *command = BRL_CMD_LEARN;
        return 1;
      case (KEY_EIGHT):
        *command = BRL_CMD_MENU_PREV_ITEM;
        return 1;
      case (KEY_NINE):
        *command = BRL_CMD_MENU_FIRST_ITEM;
        return 1;
      case (KEY_FOUR):
        *command = BRL_CMD_MENU_PREV_SETTING;
        return 1;
      case (KEY_FIVE):
        *command = BRL_CMD_PREFSAVE;
        return 1;
      case (KEY_SIX):
        *command = BRL_CMD_MENU_NEXT_SETTING;
        return 1;
      case (KEY_ONE):
        *command = BRL_CMD_PREFMENU;
        return 1;
      case (KEY_TWO):
        *command = BRL_CMD_MENU_NEXT_ITEM;
        return 1;
      case (KEY_THREE):
        *command = BRL_CMD_MENU_LAST_ITEM;
        return 1;
      case (KEY_ZERO | KEY_SEVEN):
        *command = BRL_BLK_PASSKEY + BRL_KEY_HOME;
        return 1;
      case (KEY_ZERO | KEY_EIGHT):
        *command = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP;
        return 1;
      case (KEY_ZERO | KEY_NINE):
        *command = BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP;
        return 1;
      case (KEY_ZERO | KEY_FOUR):
        *command = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT;
        return 1;
      case (KEY_ZERO | KEY_SIX):
        *command = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT;
        return 1;
      case (KEY_ZERO | KEY_ONE):
        *command = BRL_BLK_PASSKEY + BRL_KEY_END;
        return 1;
      case (KEY_ZERO | KEY_TWO):
        *command = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN;
        return 1;
      case (KEY_ZERO | KEY_THREE):
        *command = BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN;
        return 1;
      case (KEY_ZERO | KEY_B13):
        *command = BRL_BLK_PASSKEY + BRL_KEY_INSERT;
        return 1;
      case (KEY_ZERO | KEY_B14):
        *command = BRL_BLK_PASSKEY + BRL_KEY_DELETE;
        return 1;
    }

    {
      int functionKeys = KEY_B9 | KEY_B10 | KEY_B11 | KEY_B12;
      switch (keys->front & functionKeys) {
        default:
          break;
        case (KEY_B9):
          *command = BRL_BLK_SETMARK;
          goto addOffset;
        case (KEY_B10):
          *command = BRL_BLK_GOTOMARK;
          goto addOffset;
        case (KEY_B11):
          *command = BRL_BLK_SWITCHVT;
          goto addOffset;
        case (KEY_B12):
          *command = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION;
          goto addOffset;
        addOffset:
          switch (keys->front & ~functionKeys) {
            default:
              break;
            case (KEY_ONE):
              *command += 0;
              return 1;
            case (KEY_TWO):
              *command += 1;
              return 1;
            case (KEY_THREE):
              *command += 2;
              return 1;
            case (KEY_FOUR):
              *command += 3;
              return 1;
            case (KEY_FIVE):
              *command += 4;
              return 1;
            case (KEY_SIX):
              *command += 5;
              return 1;
            case (KEY_SEVEN):
              *command += 6;
              return 1;
            case (KEY_EIGHT):
              *command += 7;
              return 1;
            case (KEY_NINE):
              *command += 8;
              return 1;
            case (KEY_ZERO):
              *command += 9;
              return 1;
            case (KEY_B13):
              *command += 10;
              return 1;
            case (KEY_B14):
              *command += 11;
              return 1;
          }
      }
    }

    if (inputMode) {
      const unsigned long int dots = KEY_B1 | KEY_B2 | KEY_B3 | KEY_B4 | KEY_B5 | KEY_B6 | KEY_B7 | KEY_B8;
      if (keys->front & dots) {
        unsigned long int modifiers = keys->front & ~dots;
        *command = BRL_BLK_PASSDOTS;

        if (keys->front & KEY_B1) *command |= BRL_DOT7;
        if (keys->front & KEY_B2) *command |= BRL_DOT3;
        if (keys->front & KEY_B3) *command |= BRL_DOT2;
        if (keys->front & KEY_B4) *command |= BRL_DOT1;
        if (keys->front & KEY_B5) *command |= BRL_DOT4;
        if (keys->front & KEY_B6) *command |= BRL_DOT5;
        if (keys->front & KEY_B7) *command |= BRL_DOT6;
        if (keys->front & KEY_B8) *command |= BRL_DOT8;

        if (modifiers & KEY_UP) {
          modifiers &= ~KEY_UP;
          *command |= BRL_FLG_CHAR_CONTROL;
        }
        if (modifiers & KEY_DOWN) {
          modifiers &= ~KEY_DOWN;
          *command |= BRL_FLG_CHAR_META;
        }
        if (!modifiers) return 1;
      }
      switch (keys->front) {
        default:
          break;
        case (KEY_UP):
          *command = BRL_BLK_PASSDOTS;
          return 1;
        case (KEY_DOWN):
          *command = BRL_BLK_PASSKEY + BRL_KEY_ENTER;
          return 1;
      }
    }
    switch (keys->front) {
      default:
        break;
      case (KEY_UP):
        *command = BRL_CMD_FWINLT;
        return 1;
      case (KEY_DOWN):
        *command = BRL_CMD_FWINRT;
        return 1;
      case (KEY_B1):
        *command = BRL_CMD_HOME;
        return 1;
      case (KEY_B1 | KEY_UP):
        *command = BRL_CMD_LNBEG;
        return 1;
      case (KEY_B1 | KEY_DOWN):
        *command = BRL_CMD_LNEND;
        return 1;
      case (KEY_B2):
        *command = BRL_CMD_TOP_LEFT;
        return 1;
      case (KEY_B2 | KEY_UP):
        *command = BRL_CMD_TOP;
        return 1;
      case (KEY_B2 | KEY_DOWN):
        *command = BRL_CMD_BOT;
        return 1;
      case (KEY_B3):
        *command = BRL_CMD_BACK;
        return 1;
      case (KEY_B3 | KEY_UP):
        *command = BRL_CMD_HWINLT;
        return 1;
      case (KEY_B3 | KEY_DOWN):
        *command = BRL_CMD_HWINRT;
        return 1;
      case (KEY_B6 | KEY_UP):
        *command = BRL_CMD_CHRLT;
        return 1;
      case (KEY_B6 | KEY_DOWN):
        *command = BRL_CMD_CHRRT;
        return 1;
      case (KEY_B4):
        *command = BRL_CMD_LNUP;
        return 1;
      case (KEY_B5):
        *command = BRL_CMD_LNDN;
        return 1;
      case (KEY_B1 | KEY_B4):
        *command = BRL_CMD_PRPGRPH;
        return 1;
      case (KEY_B1 |  KEY_B5):
        *command = BRL_CMD_NXPGRPH;
        return 1;
      case (KEY_B2 | KEY_B4):
        *command = BRL_CMD_PRPROMPT;
        return 1;
      case (KEY_B2 |  KEY_B5):
        *command = BRL_CMD_NXPROMPT;
        return 1;
      case (KEY_B3 | KEY_B4):
        *command = BRL_CMD_PRSEARCH;
        return 1;
      case (KEY_B3 |  KEY_B5):
        *command = BRL_CMD_NXSEARCH;
        return 1;
      case (KEY_B6 | KEY_B4):
        *command = BRL_CMD_ATTRUP;
        return 1;
      case (KEY_B6 |  KEY_B5):
        *command = BRL_CMD_ATTRDN;
        return 1;
      case (KEY_B7 | KEY_B4):
        *command = BRL_CMD_WINUP;
        return 1;
      case (KEY_B7 |  KEY_B5):
        *command = BRL_CMD_WINDN;
        return 1;
      case (KEY_B8 | KEY_B4):
        *command = BRL_CMD_PRDIFLN;
        return 1;
      case (KEY_B8 | KEY_B5):
        *command = BRL_CMD_NXDIFLN;
        return 1;
      case (KEY_B8):
        *command = BRL_CMD_HELP;
        return 1;
      case (KEY_B8 | KEY_B1):
        *command = BRL_CMD_CSRTRK;
        return 1;
      case (KEY_B8 | KEY_B2):
        *command = BRL_CMD_CSRVIS;
        return 1;
      case (KEY_B8 | KEY_B3):
        *command = BRL_CMD_ATTRVIS;
        return 1;
      case (KEY_B8 | KEY_B6):
        *command = BRL_CMD_FREEZE;
        return 1;
      case (KEY_B8 | KEY_B7):
        *command = BRL_CMD_TUNES;
        return 1;
      case (KEY_B7):
        *command = BRL_CMD_SIXDOTS;
        return 1;
      case (KEY_B7 | KEY_B1):
        *command = BRL_CMD_PREFMENU;
        return 1;
      case (KEY_B7 | KEY_B2):
        *command = BRL_CMD_PREFLOAD;
        return 1;
      case (KEY_B7 | KEY_B3):
        *command = BRL_CMD_PREFSAVE;
        return 1;
      case (KEY_B7 | KEY_B6):
        *command = BRL_CMD_INFO;
        return 1;
      case (KEY_B6):
        *command = BRL_CMD_DISPMD;
        return 1;
      case (KEY_B6 | KEY_B1):
        *command = BRL_CMD_SKPIDLNS;
        return 1;
      case (KEY_B6 | KEY_B2):
        *command = BRL_CMD_SKPBLNKWINS;
        return 1;
      case (KEY_B6 | KEY_B3):
        *command = BRL_CMD_SLIDEWIN;
        return 1;
      case (KEY_B2 | KEY_B3 | KEY_UP):
        *command = BRL_CMD_MUTE;
        return 1;
      case (KEY_B2 | KEY_B3 | KEY_DOWN):
        *command = BRL_CMD_SAY_LINE;
        return 1;
      case (KEY_UP | KEY_DOWN):
        *command = BRL_CMD_PASTE;
        return 1;
    }
  }
  return 0;
}

static int
interpretBrailleWaveKeys (BRL_DriverCommandContext context, const Keys *keys, int *command) {
  return interpretModularKeys(context, keys, command);
}

static int
interpretBrailleStarKeys (BRL_DriverCommandContext context, const Keys *keys, int *command) {
  if (keys->column >= 0) {
    switch (keys->front) {
      default:
        break;
      case (ROCKER_LEFT_TOP):
        *command = BRL_BLK_CUTBEGIN + keys->column;
        return 1;
      case (ROCKER_LEFT_MIDDLE):
        *command = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + keys->column;
        return 1;
      case (ROCKER_LEFT_BOTTOM):
        *command = BRL_BLK_CUTAPPEND + keys->column;
        return 1;
      case (ROCKER_RIGHT_TOP):
        *command = BRL_BLK_CUTLINE + keys->column;
        return 1;
      case (ROCKER_RIGHT_MIDDLE):
        *command = BRL_BLK_SWITCHVT + keys->column;
        return 1;
      case (ROCKER_RIGHT_BOTTOM):
        *command = BRL_BLK_CUTRECT + keys->column;
        return 1;
    }
  } else if (keys->status >= 0) {
    switch (keys->front) {
      default:
        break;
    }
  } else {
    switch (keys->front) {
      default:
        break;
      case (ROCKER_LEFT_TOP):
        *command = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP;
        return 1;
      case (ROCKER_RIGHT_TOP):
        *command = BRL_CMD_LNUP;
        return 1;
      case (ROCKER_LEFT_BOTTOM):
        *command = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN;
        return 1;
      case (ROCKER_RIGHT_BOTTOM):
        *command = BRL_CMD_LNDN;
        return 1;
      case (ROCKER_LEFT_MIDDLE):
        *command = BRL_CMD_FWINLT;
        return 1;
      case (ROCKER_RIGHT_MIDDLE):
        *command = BRL_CMD_FWINRT;
        return 1;
      case (ROCKER_LEFT_MIDDLE | ROCKER_RIGHT_MIDDLE):
      case (ROCKER_LEFT_MIDDLE | KEY_B3):
      case (KEY_B6 | ROCKER_RIGHT_MIDDLE):
        *command = BRL_CMD_HOME;
        return 1;
      case (ROCKER_RIGHT_MIDDLE | ROCKER_LEFT_TOP):
      case (ROCKER_RIGHT_MIDDLE | KEY_B5):
      case (KEY_B3 | ROCKER_LEFT_TOP):
        *command = BRL_CMD_TOP_LEFT;
        return 1;
      case (ROCKER_RIGHT_MIDDLE | ROCKER_LEFT_BOTTOM):
      case (ROCKER_RIGHT_MIDDLE | KEY_B7):
      case (KEY_B3 | ROCKER_LEFT_BOTTOM):
        *command = BRL_CMD_BOT_LEFT;
        return 1;
      case (ROCKER_LEFT_MIDDLE | ROCKER_RIGHT_TOP):
      case (ROCKER_LEFT_MIDDLE | KEY_B4):
      case (KEY_B6 | ROCKER_RIGHT_TOP):
        *command = BRL_CMD_TOP;
        return 1;
      case (ROCKER_LEFT_MIDDLE | ROCKER_RIGHT_BOTTOM):
      case (ROCKER_LEFT_MIDDLE | KEY_B2):
      case (KEY_B6 | ROCKER_RIGHT_BOTTOM):
        *command = BRL_CMD_BOT;
        return 1;
      case (ROCKER_LEFT_TOP | ROCKER_RIGHT_TOP):
      case (ROCKER_LEFT_TOP | KEY_B4):
      case (KEY_B5 | ROCKER_RIGHT_TOP):
        *command = BRL_CMD_PRDIFLN;
        return 1;
      case (ROCKER_LEFT_TOP | ROCKER_RIGHT_BOTTOM):
      case (ROCKER_LEFT_TOP | KEY_B2):
      case (KEY_B5 | ROCKER_RIGHT_BOTTOM):
        *command = BRL_CMD_NXDIFLN;
        return 1;
      case (ROCKER_LEFT_BOTTOM | ROCKER_RIGHT_TOP):
      case (ROCKER_LEFT_BOTTOM | KEY_B4):
      case (KEY_B7 | ROCKER_RIGHT_TOP):
        *command = BRL_CMD_ATTRUP;
        return 1;
      case (ROCKER_LEFT_BOTTOM | ROCKER_RIGHT_BOTTOM):
      case (ROCKER_LEFT_BOTTOM | KEY_B2):
      case (KEY_B7 | ROCKER_RIGHT_BOTTOM):
        *command = BRL_CMD_ATTRDN;
        return 1;
    }
  }
  if (!(keys->front & ~(KEY_B1 | KEY_B2 | KEY_B3 | KEY_B4 |
                        KEY_B5 | KEY_B6 | KEY_B7 | KEY_B8 |
                        KEY_SPACE_LEFT | KEY_SPACE_RIGHT |
                        KEY_B9 | KEY_SEVEN | KEY_EIGHT | KEY_NINE |
                        KEY_B10 | KEY_FOUR | KEY_FIVE | KEY_SIX |
                        KEY_B11 | KEY_ONE | KEY_TWO | KEY_THREE |
                        KEY_B12 | KEY_ZERO | KEY_B13 | KEY_B14))) {
    Keys modularKeys = *keys;
    if (modularKeys.front & KEY_SPACE_LEFT) {
      modularKeys.front &= ~KEY_SPACE_LEFT;
      modularKeys.front |= KEY_UP;
    }
    if (modularKeys.front & KEY_SPACE_RIGHT) {
      modularKeys.front &= ~KEY_SPACE_RIGHT;
      modularKeys.front |= KEY_DOWN;
    }
    if (interpretModularKeys(context, &modularKeys, command)) return 1;
  }
  return 0;
}

static int
interpretBookwormByte (BRL_DriverCommandContext context, unsigned char byte, int *command) {
  switch (context) {
    case BRL_CTX_PREFS:
      switch (byte) {
        case (BWK_BACKWARD):
          *command = BRL_CMD_FWINLT;
          return 1;
        case (BWK_FORWARD):
          *command = BRL_CMD_FWINRT;
          return 1;
        case (BWK_ESCAPE):
          *command = BRL_CMD_PREFLOAD;
          return 1;
        case (BWK_ESCAPE | BWK_BACKWARD):
          *command = BRL_CMD_MENU_PREV_SETTING;
          return 1;
        case (BWK_ESCAPE | BWK_FORWARD):
          *command = BRL_CMD_MENU_NEXT_SETTING;
          return 1;
        case (BWK_ENTER):
          *command = BRL_CMD_PREFMENU;
          return 1;
        case (BWK_ENTER | BWK_BACKWARD):
          *command = BRL_CMD_MENU_PREV_ITEM;
          return 1;
        case (BWK_ENTER | BWK_FORWARD):
          *command = BRL_CMD_MENU_NEXT_ITEM;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER):
          *command = BRL_CMD_PREFSAVE;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER | BWK_BACKWARD):
          *command = BRL_CMD_MENU_FIRST_ITEM;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER | BWK_FORWARD):
          *command = BRL_CMD_MENU_LAST_ITEM;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD):
          *command = BRL_CMD_NOOP;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD | BWK_ESCAPE):
          *command = BRL_CMD_NOOP;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD | BWK_ENTER):
          *command = BRL_CMD_NOOP;
          return 1;
        default:
          break;
      }
      break;
    default:
      switch (byte) {
        case (BWK_BACKWARD):
          *command = BRL_CMD_FWINLT;
          return 1;
        case (BWK_FORWARD):
          *command = BRL_CMD_FWINRT;
          return 1;
        case (BWK_ESCAPE):
          *command = BRL_CMD_CSRTRK;
          return 1;
        case (BWK_ESCAPE | BWK_BACKWARD):
          *command = BRL_CMD_BACK;
          return 1;
        case (BWK_ESCAPE | BWK_FORWARD):
          *command = BRL_CMD_DISPMD;
          return 1;
        case (BWK_ENTER):
          *command = BRL_BLK_ROUTE;
          return 1;
        case (BWK_ENTER | BWK_BACKWARD):
          *command = BRL_CMD_LNUP;
          return 1;
        case (BWK_ENTER | BWK_FORWARD):
          *command = BRL_CMD_LNDN;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER):
          *command = BRL_CMD_PREFMENU;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER | BWK_BACKWARD):
          *command = BRL_CMD_LNBEG;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER | BWK_FORWARD):
          *command = BRL_CMD_LNEND;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD):
          *command = BRL_CMD_HELP;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD | BWK_ESCAPE):
          *command = BRL_CMD_CSRSIZE;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD | BWK_ENTER):
          *command = BRL_CMD_FREEZE;
          return 1;
        default:
          break;
      }
      break;
  }
  return 0;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  int noInput = 1;

  if (at2Count) {
    unsigned char code = at2Buffer[0];
    memcpy(at2Buffer, at2Buffer+1, --at2Count);
    return BRL_BLK_PASSAT2 + code;
  }

  while (1) {
    HT_Packet packet;

    {
      static const int logInputPackets = 0;
      int size = brl_readPacket(brl, &packet, sizeof(packet));
      if (size == -1) return BRL_CMD_RESTARTBRL;
      if (size == 0) break;
      if (logInputPackets) LogBytes("Input Packet", packet.bytes, size);
    }
    noInput = 0;

    if (packet.fields.type == 0X06) {
      if (currentState != BDS_OFF) {
        if (io->awaitInput(10)) {
          setState(BDS_OFF);
          continue;
        }
        if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;
      }
    }

    switch (packet.fields.type) {
      case HT_PKT_OK:
        if (packet.fields.data.ok.model == model->identifier) {
          setState(BDS_READY);
          updateRequired = 1;
          currentKeys = pressedKeys = nullKeys;
          continue;
        }
        break;

      default:
        switch (currentState) {
          case BDS_OFF:
            continue;

          case BDS_WRITING:
            switch (packet.fields.type) {
              case HT_PKT_NAK:
                updateRequired = 1;
              case HT_PKT_ACK:
                setState(BDS_READY);
                continue;
            }

          case BDS_READY:
            switch (packet.fields.type) {
              case HT_PKT_Extended: {
                unsigned char length = packet.fields.data.extended.length - 1;
                const unsigned char *bytes = &packet.fields.data.extended.data.bytes[0];

                switch (packet.fields.data.extended.type) {
                  case 0X04: {
                    int command;
                    if (model->interpretByte(context, bytes[0], &command)) {
                      updateBrailleCells(brl);
                      return command;
                    }
                    break;
                  }

                  case 0X07:
                    switch (bytes[0]) {
                      case HT_PKT_NAK:
                        updateRequired = 1;
                      case HT_PKT_ACK:
                        setState(BDS_READY);
                        continue;
                    }
                    break;

                  case 0X09: {
                    if (length) {
                      unsigned char code = bytes++[0];
                      if (--length) {
                        int newCount = at2Count + length;
                        if (newCount > at2Size) {
                          int newSize = (newCount | 0XF) + 1;
                          at2Buffer = reallocWrapper(at2Buffer, newSize);
                          at2Size = newSize;
                        }
                        memcpy(at2Buffer+at2Count, bytes, length);
                        at2Count = newCount;
                      }
                      return BRL_BLK_PASSAT2 + code;
                    }
                    break;
                  }

                  case 0X52:
                    LogBytes("ATC", bytes, length);			break;
                    continue;
                }
                break;
              }

              default: {
                int command;
                if (model->interpretByte(context, packet.fields.type, &command)) {
                  updateBrailleCells(brl);
                  return command;
                }
                break;
              }
            }
            break;
        }
        break;
    }

    LogPrint(LOG_WARNING, "Unexpected Packet: %02X (state %d)", packet.fields.type, currentState);
  }

  if (noInput) {
    switch (currentState) {
      case BDS_OFF:
        break;

      case BDS_READY:
        break;

      case BDS_WRITING:
        if (millisecondsSince(&stateTime) > 1000) {
          if (retryCount > 3) return BRL_CMD_RESTARTBRL;
          if (!writeBrailleCells(brl)) return BRL_CMD_RESTARTBRL;
          setState(BDS_WRITING);
        }
        break;
    }
  }
  updateBrailleCells(brl);

  return EOF;
}

static ssize_t
brl_readPacket (BrailleDisplay *brl, void *buffer, size_t size) {
  static const int logInputPackets = 0;
  unsigned char *packet = buffer;
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;
      int count = io->readBytes(&byte, 1, started);

      if (count != 1) {
        if (!count && started) LogBytes("Partial Packet", packet, offset);
        return count;
      }
    }

    if (offset == 0) {
      switch (byte) {
        default:
          length = 1;
          break;

        case HT_PKT_OK:
          length = 2;
          break;

        case HT_PKT_Extended:
          length = 4;
          break;
      }
    } else {
      switch (packet[0]) {
        case HT_PKT_Extended:
          if (offset == 2) length += byte;
          break;
      }
    }

    if (offset < size) {
      packet[offset] = byte;
    } else {
      if (offset == size) LogBytes("Truncated Packet", packet, offset);
      LogBytes("Discarded Byte", &byte, 1);
    }

    if (++offset == length) {
      if (offset <= size) {
        int ok = 0;

        switch (packet[0]) {
          case HT_PKT_Extended:
            if (packet[length-1] == 0X16) ok = 1;
            break;

          default:
            ok = 1;
            break;
        }

        if (ok) {
          if (logInputPackets) LogBytes("Input Packet", packet, offset);
          return length;
        }

        LogBytes("Malformed Packet", packet, offset);
      }

      offset = 0;
      length = 0;
    }
  }
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const void *packet, size_t length) {
  return io->writeBytes(packet, length, &brl->writeDelay);
}
