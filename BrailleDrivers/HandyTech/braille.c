/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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
#include "Programs/at2.h"

typedef enum {
  PARM_INPUTMODE
} DriverParameter;
#define BRLPARMS "inputmode"

#define BRLSTAT ST_AlvaStyle
#define BRL_HAVE_PACKET_IO
#include "Programs/brl_driver.h"
#include "braille.h"

/* Communication codes */
static unsigned char HandyDescribe[] = {0XFF};
static unsigned char HandyDescription[] = {0XFE};
static unsigned char HandyBrailleStart[] = {0X01};	/* general header to display braille */
static unsigned char BookwormBrailleEnd[] = {0X16};	/* bookworm trailer to display braille */
static unsigned char BookwormStop[] = {0X05, 0X07};	/* bookworm trailer to display braille */

typedef struct {
  unsigned long int front;
  int column;
  int status;
} Keys;
static Keys currentKeys, pressedKeys, nullKeys;
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
  unsigned char identifier;
  unsigned char columns;
  unsigned char statusCells;
  unsigned char helpPage;
  ByteInterpreter *interpretByte;
  KeysInterpreter *interpretKeys;
  unsigned char *brailleStartAddress;
  unsigned char *brailleEndAddress;
  unsigned char *stopAddress;
  unsigned char brailleStartLength;
  unsigned char brailleEndLength;
  unsigned char stopLength;
} ModelDescription;
static const ModelDescription Models[] = {
  {
    "Modular 20+4", 0X80,
    20, 4, 0,
    interpretKeyByte, interpretModularKeys,
    HandyBrailleStart,         NULL, NULL,
    sizeof(HandyBrailleStart), 0,    0
  }
  ,
  {
    "Modular 40+4", 0X89,
    40, 4, 0,
    interpretKeyByte, interpretModularKeys,
    HandyBrailleStart,         NULL, NULL,
    sizeof(HandyBrailleStart), 0,    0
  }
  ,
  {
    "Modular 80+4", 0X88,
    80, 4, 0,
    interpretKeyByte, interpretModularKeys,
    HandyBrailleStart,         NULL, NULL,
    sizeof(HandyBrailleStart), 0,    0
  }
  ,
  {
    "Braille Wave 40", 0X05,
    40, 0, 0,
    interpretKeyByte, interpretBrailleWaveKeys,
    HandyBrailleStart,         NULL, NULL,
    sizeof(HandyBrailleStart), 0,    0
  }
  ,
  {
    "Bookworm", 0X90,
    8, 0, 1,
    interpretBookwormByte, NULL,
    HandyBrailleStart,         BookwormBrailleEnd,         BookwormStop,
    sizeof(HandyBrailleStart), sizeof(BookwormBrailleEnd), sizeof(BookwormStop)
  }
  ,
  {
    "Braillino 20", 0X72,
    20, 0, 2,
    interpretKeyByte, interpretBrailleStarKeys,
    HandyBrailleStart,         NULL, NULL,
    sizeof(HandyBrailleStart), 0,    0 
  }
  ,
  {
    "Braille Star 40", 0X74,
    40, 0, 2,
    interpretKeyByte, interpretBrailleStarKeys,
    HandyBrailleStart,         NULL, NULL,
    sizeof(HandyBrailleStart), 0,    0
  }
  ,
  {
    "Braille Star 80", 0X78,
    80, 0, 3,
    interpretKeyByte, interpretBrailleStarKeys,
    HandyBrailleStart,         NULL, NULL,
    sizeof(HandyBrailleStart), 0,    0
  }
  ,
  { /* end of table */
    NULL, 0,
    0, 0, 0,
    NULL, NULL,
    NULL, NULL, NULL,
    0,    0,    0
  }
};

#define BRLROWS		1
#define MAX_STCELLS	4	/* highest number of status cells */



/* This is the brltty braille mapping standard to Handy's mapping table.
 */
static TranslationTable outputTable;

/* Global variables */
static unsigned char *rawData = NULL;		/* translated data to send to Braille */
static unsigned char *prevData = NULL;	/* previously sent raw data */
static unsigned char rawStatus[MAX_STCELLS];		/* to hold status info */
static unsigned char prevStatus[MAX_STCELLS];	/* to hold previous status */
static const ModelDescription *model;		/* points to terminal model config struct */

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

typedef enum {
  BDS_OFF,
  BDS_RESETTING,
  BDS_IDENTIFYING,
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
readByte (BrailleDisplay *brl, unsigned char *byte, int wait) {
  return io->readBytes(byte, sizeof(*byte), wait);
}

static int
brl_reset (BrailleDisplay *brl) {
  return 0;
}

static int
writeDescribe (BrailleDisplay *brl) {
  return io->writeBytes(HandyDescribe, sizeof(HandyDescribe), &brl->writeDelay) != -1;
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
    model = Models;
    model->name && (model->identifier != identifier);
    model++
  );
  if (!model->name) {
    LogPrint(LOG_ERR, "Detected unknown HandyTech model with ID %02X.",
             identifier);
    LogPrint(LOG_WARNING, "Please fix Models[] in HandyTech/braille.c and notify the maintainer.");
    return 0;
  }
  LogPrint(LOG_INFO, "Detected %s: %d data %s, %d status %s.",
           model->name,
           model->columns, (model->columns == 1)? "cell": "cells",
           model->statusCells, (model->statusCells == 1)? "cell": "cells");

  brl->helpPage = model->helpPage;		/* position in the model list */
  brl->x = model->columns;			/* initialise size of display */
  brl->y = BRLROWS;

  if (!reallocateBuffer(&rawData, brl->x*brl->y)) return 0;
  if (!reallocateBuffer(&prevData, brl->x*brl->y)) return 0;

  nullKeys.front = 0;
  nullKeys.column = -1;
  nullKeys.status = -1;
  currentKeys = pressedKeys = nullKeys;
  inputMode = 0;

  memset(rawStatus, 0, model->statusCells);
  memset(rawData, 0, model->columns);

  retryCount = 0;
  updateRequired = 0;
  currentState = BDS_OFF;
  setState(BDS_READY);

  return 1;
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  AT2_resetState();
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
    while (writeDescribe(brl)) {
      while (io->awaitInput(100)) {
        unsigned char response[sizeof(HandyDescription) + 1];
        if (io->readBytes(response, sizeof(response), 0) == sizeof(response)) {
          if (memcmp(response, HandyDescription, sizeof(HandyDescription)) == 0) {
            if (identifyModel(brl, response[sizeof(HandyDescription)])) {
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
  if (model->stopLength) {
    io->writeBytes(model->stopAddress, model->stopLength, NULL);
  }
  io->closePort();

  if (at2Buffer) {
    free(at2Buffer);
    at2Buffer = NULL;
  }

  deallocateBuffers();
}

static int
updateBrailleCells (BrailleDisplay *brl) {
  if (updateRequired && (currentState == BDS_READY)) {
    unsigned char buffer[model->brailleStartLength + model->statusCells + model->columns + model->brailleEndLength];
    int count = 0;

    if (model->brailleStartLength) {
      memcpy(buffer+count, model->brailleStartAddress, model->brailleStartLength);
      count += model->brailleStartLength;
    }

    memcpy(buffer+count, rawStatus, model->statusCells);
    count += model->statusCells;

    memcpy(buffer+count, rawData, model->columns);
    count += model->columns;

    if (model->brailleEndLength) {
      memcpy(buffer+count, model->brailleEndAddress, model->brailleEndLength);
      count += model->brailleEndLength;
    }

    // LogBytes("Write", buffer, count);
    if (io->writeBytes(buffer, count, &brl->writeDelay) == -1) {
      setState(BDS_OFF);
      return 0;
    }
    setState(BDS_WRITING);
    updateRequired = 0;
  }
  return 1;
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  if (memcmp(brl->buffer, prevData, model->columns) != 0) {
    int i;
    for (i=0; i<model->columns; ++i) {
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
      (byte < (KEY_ROUTING + model->columns))) {
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
        *command = BRL_CMD_HOME;
        return 1;
      case (ROCKER_RIGHT_MIDDLE | ROCKER_LEFT_TOP):
        *command = BRL_CMD_TOP_LEFT;
        return 1;
      case (ROCKER_RIGHT_MIDDLE | ROCKER_LEFT_BOTTOM):
        *command = BRL_CMD_BOT_LEFT;
        return 1;
      case (ROCKER_LEFT_MIDDLE | ROCKER_RIGHT_TOP):
        *command = BRL_CMD_TOP;
        return 1;
      case (ROCKER_LEFT_MIDDLE | ROCKER_RIGHT_BOTTOM):
        *command = BRL_CMD_BOT;
        return 1;
      case (ROCKER_LEFT_TOP | ROCKER_RIGHT_TOP):
        *command = BRL_CMD_PRDIFLN;
        return 1;
      case (ROCKER_LEFT_TOP | ROCKER_RIGHT_BOTTOM):
        *command = BRL_CMD_NXDIFLN;
        return 1;
      case (ROCKER_LEFT_BOTTOM | ROCKER_RIGHT_TOP):
        *command = BRL_CMD_ATTRUP;
        return 1;
      case (ROCKER_LEFT_BOTTOM | ROCKER_RIGHT_BOTTOM):
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
  int timedOut = 1;

  if (at2Count) {
    unsigned char code = at2Buffer[0];
    memcpy(at2Buffer, at2Buffer+1, --at2Count);
    return BRL_BLK_PASSAT2 + code;
  }

  while (1) {
    unsigned char byte;
    {
      int count = readByte(brl, &byte, 0);
      if (count == -1) return BRL_CMD_RESTARTBRL;
      if (count == 0) break;
    }
    timedOut = 0;
    /* LogPrint(LOG_DEBUG, "Read: %02X", byte); */

    if (byte == 0X06) {
      if (currentState != BDS_OFF) {
        if (io->awaitInput(10)) {
          setState(BDS_OFF);
          continue;
        }
        if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;
      }
    }

    switch (byte) {
      case 0XFE:
        setState(BDS_IDENTIFYING);
        continue;
      default:
        switch (currentState) {
          case BDS_OFF:
            continue;
          case BDS_RESETTING:
            break;
          case BDS_IDENTIFYING:
            if (byte == model->identifier) {
              setState(BDS_READY);
              updateRequired = 1;
              currentKeys = pressedKeys = nullKeys;
              continue;
            }
            break;
          case BDS_WRITING:
            switch (byte) {
              case 0X7D:
                updateRequired = 1;
              case 0X7E:
                setState(BDS_READY);
                continue;
              default:
                break;
            }
          case BDS_READY:
            switch (byte) {
              case 0X79: {
                unsigned char header[2];
                io->readBytes(header, sizeof(header), 1);
                if (header[0] == model->identifier) {
                  unsigned char length = header[1];
                  unsigned char data[length+1];
                  io->readBytes(data, sizeof(data), 1);
                  if (data[length] == 0X16) {
                    const unsigned char *bytes = data + 1;
                    length--;

                    switch (data[0]) {
                      case 0X09: {
                        if (length) {
                          unsigned char code = *bytes++;
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
                    }
                  } else {
                    LogBytes("Malformed keycode packet", data, sizeof(data));
                  }
                } else {
                  LogError("Keycode packet ID mismatch");
                }
                continue;
              }

              default: {
                int command;
                if (model->interpretByte(context, byte, &command)) {
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

    LogPrint(LOG_WARNING, "Unexpected byte: %02X (state %d)", byte, currentState);
  }

  if (timedOut) {
    switch (currentState) {
      case BDS_OFF:
        break;
      case BDS_RESETTING:
        if (millisecondsSince(&stateTime) > 3000) {
          if (retryCount > 3) {
            setState(BDS_OFF);
          } else if (writeDescribe(brl)) {
            setState(BDS_RESETTING);
          } else {
            setState(BDS_OFF);
          }
        }
        break;
      case BDS_IDENTIFYING:
        if (millisecondsSince(&stateTime) > 1000) {
          if (writeDescribe(brl)) {
            setState(BDS_RESETTING);
          } else {
            setState(BDS_OFF);
          }
        }
        break;
      case BDS_READY:
        break;
      case BDS_WRITING:
        if (millisecondsSince(&stateTime) > 1000) {
          if (retryCount > 3) {
            if (writeDescribe(brl)) {
              setState(BDS_RESETTING);
            } else {
              setState(BDS_OFF);
            }
          } else {
            updateRequired = 1;
          }
        }
        break;
    }
  }
  updateBrailleCells(brl);

  return EOF;
}

static ssize_t
brl_readPacket (BrailleDisplay *brl, unsigned char *bytes, size_t count) {
  return io->readBytes(bytes, count, 0);
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const unsigned char *data, size_t length) {
  return io->writeBytes(data, length, &brl->writeDelay);
}
