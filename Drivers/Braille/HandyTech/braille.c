/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "misc.h"

typedef enum {
  PARM_INPUTMODE
} DriverParameter;
#define BRLPARMS "inputmode"

#define BRLSTAT ST_AlvaStyle
#define BRL_HAVE_STATUS_CELLS
#define BRL_HAVE_PACKET_IO
#define BRL_HAVE_SENSITIVITY
#include "brl_driver.h"
#include "touch.h"
#include "brldefs-ht.h"

typedef enum {
  HT_KEY_None = 0,

  HT_KEY_B1 = 0X03,
  HT_KEY_B2 = 0X07,
  HT_KEY_B3 = 0X0B,
  HT_KEY_B4 = 0X0F,

  HT_KEY_B5 = 0X13,
  HT_KEY_B6 = 0X17,
  HT_KEY_B7 = 0X1B,
  HT_KEY_B8 = 0X1F,

  HT_KEY_Up = 0X04,
  HT_KEY_Down = 0X08,

  /* Keypad keys (star80 and modular) */
  HT_KEY_B12 = 0X01,
  HT_KEY_Zero = 0X05,
  HT_KEY_B13 = 0X09,
  HT_KEY_B14 = 0X0D,

  HT_KEY_B11 = 0X11,
  HT_KEY_One = 0X15,
  HT_KEY_Two = 0X19,
  HT_KEY_Three = 0X1D,

  HT_KEY_B10 = 0X02,
  HT_KEY_Four = 0X06,
  HT_KEY_Five = 0X0A,
  HT_KEY_Six = 0X0E,

  HT_KEY_B9 = 0X12,
  HT_KEY_Seven = 0X16,
  HT_KEY_Eight = 0X1A,
  HT_KEY_Nine = 0X1E,

  /* Braille wave/star keys */
  HT_KEY_Escape = 0X0C,
  HT_KEY_Space = 0X10,
  HT_KEY_Return = 0X14,

  /* Braille star keys */
  HT_KEY_SpaceRight = 0X18
} HT_NavigationKey;

typedef enum {
  HT_SET_NavigationKeys = 0,
  HT_SET_RoutingKeys,
  HT_SET_StatusKeys
} HT_KeySet;

static KEY_NAME_TABLE(keyNames_routing) = {
  KEY_SET_ENTRY(HT_SET_RoutingKeys, "RoutingKey"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_dots) = {
  KEY_NAME_ENTRY(HT_KEY_B1, "B1"),
  KEY_NAME_ENTRY(HT_KEY_B2, "B2"),
  KEY_NAME_ENTRY(HT_KEY_B3, "B3"),
  KEY_NAME_ENTRY(HT_KEY_B4, "B4"),

  KEY_NAME_ENTRY(HT_KEY_B5, "B5"),
  KEY_NAME_ENTRY(HT_KEY_B6, "B6"),
  KEY_NAME_ENTRY(HT_KEY_B7, "B7"),
  KEY_NAME_ENTRY(HT_KEY_B8, "B8"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_keypad) = {
  KEY_NAME_ENTRY(HT_KEY_B12, "B12"),
  KEY_NAME_ENTRY(HT_KEY_Zero, "Zero"),
  KEY_NAME_ENTRY(HT_KEY_B13, "B13"),
  KEY_NAME_ENTRY(HT_KEY_B14, "B14"),

  KEY_NAME_ENTRY(HT_KEY_B11, "B11"),
  KEY_NAME_ENTRY(HT_KEY_One, "One"),
  KEY_NAME_ENTRY(HT_KEY_Two, "Two"),
  KEY_NAME_ENTRY(HT_KEY_Three, "Three"),

  KEY_NAME_ENTRY(HT_KEY_B10, "B10"),
  KEY_NAME_ENTRY(HT_KEY_Four, "Four"),
  KEY_NAME_ENTRY(HT_KEY_Five, "Five"),
  KEY_NAME_ENTRY(HT_KEY_Six, "Six"),

  KEY_NAME_ENTRY(HT_KEY_B9, "B9"),
  KEY_NAME_ENTRY(HT_KEY_Seven, "Seven"),
  KEY_NAME_ENTRY(HT_KEY_Eight, "Eight"),
  KEY_NAME_ENTRY(HT_KEY_Nine, "Nine"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_rockers) = {
  KEY_NAME_ENTRY(HT_KEY_Escape, "RockerLeftTop"),
  KEY_NAME_ENTRY(HT_KEY_Return, "RockerLeftBottom"),

  KEY_NAME_ENTRY(HT_KEY_Up, "RockerRightTop"),
  KEY_NAME_ENTRY(HT_KEY_Down, "RockerRightBottom"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_modular) = {
  KEY_NAME_ENTRY(HT_KEY_Up, "Up"),
  KEY_NAME_ENTRY(HT_KEY_Down, "Down"),

  LAST_KEY_NAME_ENTRY
};

static const char keyBindings_modular[] = "mdlr";
static KEY_NAME_TABLE_LIST(keyNameTables_modular) = {
  keyNames_routing,
  keyNames_dots,
  keyNames_keypad,
  keyNames_modular,
  NULL
};

static KEY_NAME_TABLE(keyNames_modularEvolution) = {
  KEY_NAME_ENTRY(HT_KEY_Space, "ThumbLeft"),
  KEY_NAME_ENTRY(HT_KEY_SpaceRight, "ThumbRight"),

  LAST_KEY_NAME_ENTRY
};

static const char keyBindings_modularEvolution64[] = "me64";
static KEY_NAME_TABLE_LIST(keyNameTables_modularEvolution64) = {
  keyNames_routing,
  keyNames_dots,
  keyNames_rockers,
  keyNames_modularEvolution,
  NULL
};

static const char keyBindings_modularEvolution88[] = "me88";
static KEY_NAME_TABLE_LIST(keyNameTables_modularEvolution88) = {
  keyNames_routing,
  keyNames_dots,
  keyNames_rockers,
  keyNames_keypad,
  keyNames_modularEvolution,
  NULL
};

static KEY_NAME_TABLE(keyNames_brailleStar) = {
  KEY_NAME_ENTRY(HT_KEY_Space, "SpaceLeft"),
  KEY_NAME_ENTRY(HT_KEY_SpaceRight, "SpaceRight"),

  LAST_KEY_NAME_ENTRY
};

static const char keyBindings_brailleStar40[] = "bs40";
static KEY_NAME_TABLE_LIST(keyNameTables_brailleStar40) = {
  keyNames_routing,
  keyNames_dots,
  keyNames_rockers,
  keyNames_brailleStar,
  NULL
};

static const char keyBindings_brailleStar80[] = "bs80";
static KEY_NAME_TABLE_LIST(keyNameTables_brailleStar80) = {
  keyNames_routing,
  keyNames_dots,
  keyNames_rockers,
  keyNames_keypad,
  keyNames_brailleStar,
  NULL
};

static KEY_NAME_TABLE(keyNames_brailleWave) = {
  KEY_NAME_ENTRY(HT_KEY_Escape, "Escape"),
  KEY_NAME_ENTRY(HT_KEY_Space, "Space"),
  KEY_NAME_ENTRY(HT_KEY_Return, "Return"),

  LAST_KEY_NAME_ENTRY
};

static const char keyBindings_brailleWave[] = "wave";
static KEY_NAME_TABLE_LIST(keyNameTables_brailleWave) = {
  keyNames_routing,
  keyNames_dots,
  keyNames_modular,
  keyNames_brailleWave,
  NULL
};

typedef enum {
  HT_BWK_Backward = 0X01,
  HT_BWK_Forward = 0X08,

  HT_BWK_Escape = 0X02,
  HT_BWK_Enter = 0X04
} HT_BookwormKey;

static KEY_NAME_TABLE(keyNames_bookworm) = {
  KEY_NAME_ENTRY(HT_BWK_Backward, "Backward"),
  KEY_NAME_ENTRY(HT_BWK_Forward, "Forward"),

  KEY_NAME_ENTRY(HT_BWK_Escape, "Escape"),
  KEY_NAME_ENTRY(HT_BWK_Enter, "Enter"),

  LAST_KEY_NAME_ENTRY
};

static const char keyBindings_bookworm[] = "bkwm";
static KEY_NAME_TABLE_LIST(keyNameTables_bookworm) = {
  keyNames_bookworm,
  NULL
};

typedef struct {
  uint32_t front;
  signed char column;
  signed char status;
} Keys;
static Keys currentKeys, pressedKeys;
static const Keys nullKeys = {.front=0, .column=-1, .status=-1};
static unsigned int inputMode;

static const unsigned char BookwormSessionEnd[] = {0X05, 0X07};	/* bookworm trailer to display braille */

typedef int (ByteInterpreter) (BRL_DriverCommandContext context, unsigned char byte, int *command);
static ByteInterpreter interpretKeyByte;
static ByteInterpreter handleKeyByte;
static ByteInterpreter interpretBookwormByte;

typedef int (KeysInterpreter) (BRL_DriverCommandContext context, const Keys *keys, int *command);
static KeysInterpreter interpretModularKeys;
static KeysInterpreter interpretBrailleWaveKeys;
static KeysInterpreter interpretBrailleStarKeys;

typedef int (CellWriter) (BrailleDisplay *brl);
static CellWriter writeStatusAndTextCells;
static CellWriter writeBookwormCells;
static CellWriter writeEvolutionCells;

typedef struct {
  const char *name;
  const char *keyBindings;
  const KeyNameEntry *const *keyNameTables;

  ByteInterpreter *interpretByte;
  KeysInterpreter *interpretKeys;
  CellWriter *writeCells;

  const unsigned char *sessionEndAddress;

  HT_ModelIdentifier identifier:8;
  unsigned char textCells;
  unsigned char statusCells;

  unsigned char sessionEndLength;

  unsigned hasATC:1; /* Active Tactile Control */
  unsigned isBookworm:1;
} ModelEntry;

#define HT_BYTE_SEQUENCE(name,bytes) .name##Address = bytes, .name##Length = sizeof(bytes)
static const ModelEntry modelTable[] = {
  { .identifier = HT_Model_Modular20,
    .name = "Modular 20+4",
    .textCells = 20,
    .statusCells = 4,
    .keyBindings = keyBindings_modular,
  //.keyNameTables = keyNameTables_modular,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretModularKeys,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_Modular40,
    .name = "Modular 40+4",
    .textCells = 40,
    .statusCells = 4,
    .keyBindings = keyBindings_modular,
  //.keyNameTables = keyNameTables_modular,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretModularKeys,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_Modular80,
    .name = "Modular 80+4",
    .textCells = 80,
    .statusCells = 4,
    .keyBindings = keyBindings_modular,
  //.keyNameTables = keyNameTables_modular,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretModularKeys,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_ModularEvolution64,
    .name = "Modular Evolution 64",
    .textCells = 64,
    .statusCells = 0,
    .keyBindings = keyBindings_modularEvolution64,
  //.keyNameTables = keyNameTables_modularEvolution64,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleStarKeys,
    .writeCells = writeEvolutionCells,
    .hasATC = 1
  }
  ,
  { .identifier = HT_Model_ModularEvolution88,
    .name = "Modular Evolution 88",
    .textCells = 88,
    .statusCells = 0,
    .keyBindings = keyBindings_modularEvolution88,
    .keyNameTables = keyNameTables_modularEvolution88,
    .interpretByte = handleKeyByte,
    .writeCells = writeEvolutionCells,
    .hasATC = 1
  }
  ,
  { .identifier = HT_Model_BrailleWave,
    .name = "Braille Wave 40",
    .textCells = 40,
    .statusCells = 0,
    .keyBindings = keyBindings_brailleWave,
  //.keyNameTables = keyNameTables_brailleWave,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleWaveKeys,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_Bookworm,
    .name = "Bookworm",
    .isBookworm = 1,
    .textCells = 8,
    .statusCells = 0,
    .keyBindings = keyBindings_bookworm,
    .keyNameTables = keyNameTables_bookworm,
    .interpretByte = interpretBookwormByte,
    .writeCells = writeBookwormCells,
    HT_BYTE_SEQUENCE(sessionEnd, BookwormSessionEnd)
  }
  ,
  { .identifier = HT_Model_Braillino,
    .name = "Braillino 20",
    .textCells = 20,
    .statusCells = 0,
    .keyBindings = keyBindings_brailleStar40,
  //.keyNameTables = keyNameTables_brailleStar40,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleStarKeys,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_BrailleStar40,
    .name = "Braille Star 40",
    .textCells = 40,
    .statusCells = 0,
    .keyBindings = keyBindings_brailleStar40,
  //.keyNameTables = keyNameTables_brailleStar40,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleStarKeys,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_BrailleStar80,
    .name = "Braille Star 80",
    .textCells = 80,
    .statusCells = 0,
    .keyBindings = keyBindings_brailleStar80,
  //.keyNameTables = keyNameTables_brailleStar80,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretBrailleStarKeys,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_EasyBraille,
    .name = "Easy Braille",
    .textCells = 40,
    .statusCells = 0,
    .keyBindings = keyBindings_modular,
  //.keyNameTables = keyNameTables_modular,
    .interpretByte = interpretKeyByte,
    .interpretKeys = interpretModularKeys,
    .writeCells = writeStatusAndTextCells
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
#include "io_serial.h"

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
#include "io_usb.h"

static UsbChannel *usb = NULL;

static int
openUsbPort (char **parameters, const char *device) {
  const SerialParameters serial = {
    .baud = baud,
    .flow = SERIAL_FLOW_NONE,
    .data = 8,
    .stop = 1,
    .parity = SERIAL_PARITY_ODD
  };

  const UsbChannelDefinition definitions[] = {
    { /* GoHubs chip */
      .vendor=0X0921, .product=0X1200,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=1,
      .serial = &serial
    }
    ,
    { /* FTDI chip */
      .vendor=0X0403, .product=0X6001,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .serial = &serial
    }
    ,
    { .vendor=0 }
  };

  if ((usb = usbFindChannel(definitions, (void *)device))) {
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
#include "io_bluetooth.h"
#include "io_misc.h"

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

#define SYN 0X16

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
#define KEY(code)   (UINT32_C(1) << (code))

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

/* keypad keys */
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
    return 0;
  }

  LogPrint(LOG_INFO, "Detected %s: %d data %s, %d status %s.",
           model->name,
           model->textCells, (model->textCells == 1)? "cell": "cells",
           model->statusCells, (model->statusCells == 1)? "cell": "cells");

  brl->textColumns = model->textCells;			/* initialise size of display */
  brl->textRows = BRLROWS;
  brl->statusColumns = model->statusCells;
  brl->statusRows = 1;
  brl->keyBindings = model->keyBindings;		/* position in the model list */
  brl->keyNameTables = model->keyNameTables;		/* position in the model list */

  if (!reallocateBuffer(&rawData, brl->textColumns*brl->textRows)) return 0;
  if (!reallocateBuffer(&prevData, brl->textColumns*brl->textRows)) return 0;

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

static int
writeExtendedPacket (
  BrailleDisplay *brl, HT_ExtendedPacketType type,
  const unsigned char *data, unsigned char size
) {
  HT_Packet packet;
  packet.fields.type = HT_PKT_Extended;
  packet.fields.data.extended.model = model->identifier;
  packet.fields.data.extended.length = size + 1; /* type byte is included */
  packet.fields.data.extended.type = type;
  memcpy(packet.fields.data.extended.data.bytes, data, size);
  packet.fields.data.extended.data.bytes[size] = SYN;
  size += 5; /* EXT, ID, LEN, TYPE, ..., SYN */
  return io->writeBytes((unsigned char *)&packet, size, &brl->writeDelay) == size;
}

static int
setAtcMode (BrailleDisplay *brl, unsigned char value) {
  const unsigned char data[] = {value};
  return writeExtendedPacket(brl, HT_EXTPKT_SetAtcMode, data, sizeof(data));
}

static int
setAtcSensitivity (BrailleDisplay *brl, unsigned char value) {
  const unsigned char data[] = {value};
  return writeExtendedPacket(brl, HT_EXTPKT_SetAtcSensitivity, data, sizeof(data));
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
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
                setAtcSensitivity(brl, 50);

                touchAnalyzeCells(brl, NULL);
                brl->touchEnabled = 1;
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
brl_destruct (BrailleDisplay *brl) {
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
writeCells (BrailleDisplay *brl) {
  if (!model->writeCells(brl)) return 0;
  setState(BDS_WRITING);
  return 1;
}

static int
writeStatusAndTextCells (BrailleDisplay *brl) {
  unsigned char buffer[1 + model->statusCells + model->textCells];

  buffer[0] = 0X01;
  memcpy(buffer+1, rawStatus, model->statusCells);
  memcpy(buffer+model->statusCells+1, rawData, model->textCells);

  return io->writeBytes(buffer, sizeof(buffer), &brl->writeDelay) != -1;
}

static int
writeBookwormCells (BrailleDisplay *brl) {
  unsigned char buffer[1 + model->statusCells + model->textCells + 1];

  buffer[0] = 0X01;
  memcpy(buffer+1, rawData, model->textCells);
  buffer[sizeof(buffer)-1] = SYN;
  return io->writeBytes(buffer, sizeof(buffer), &brl->writeDelay) != -1;
}

static int
writeEvolutionCells (BrailleDisplay *brl) {
  unsigned char buffer[model->textCells];

  memcpy(buffer, rawData, model->textCells);

  return writeExtendedPacket(brl, HT_EXTPKT_Braille, buffer, sizeof(buffer));
}

static int
updateCells (BrailleDisplay *brl) {
  if (!updateRequired) return 1;
  if (currentState != BDS_READY) return 1;

  if (!writeCells(brl)) {
    setState(BDS_OFF);
    return 0;
  }

  updateRequired = 0;
  return 1;
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (memcmp(brl->buffer, prevData, model->textCells) != 0) {
    int i;
    for (i=0; i<model->textCells; ++i) {
      rawData[i] = outputTable[(prevData[i] = brl->buffer[i])];
    }
    updateRequired = 1;
  }
  updateCells(brl);
  return 1;
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *st) {
  if (model->statusCells &&
      (memcmp(st, prevStatus, model->statusCells) != 0)) {
    int i;
    for (i=0; i<model->statusCells; ++i) {
      rawStatus[i] = outputTable[(prevStatus[i] = st[i])];
    }
    updateRequired = 1;
  }
  return 1;
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
    uint32_t key = KEY(byte);
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
handleKeyByte (BRL_DriverCommandContext context, unsigned char byte, int *command) {
  int release = (byte & KEY_RELEASE) != 0;
  if (release) byte &= ~KEY_RELEASE;

  if ((byte >= KEY_ROUTING) &&
      (byte < (KEY_ROUTING + model->textCells))) {
    *command = EOF;
    return enqueueKeyEvent(HT_SET_RoutingKeys, byte-KEY_ROUTING, !release);
  }

  if ((byte >= KEY_STATUS) &&
      (byte < (KEY_STATUS + model->statusCells))) {
    *command = EOF;
    return enqueueKeyEvent(HT_SET_StatusKeys, byte-KEY_STATUS, !release);
  }

  if ((byte > 0) && (byte < 0X20)) {
    *command = EOF;
    return enqueueKeyEvent(HT_SET_NavigationKeys, byte, !release);
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
      const uint32_t dots = KEY_B1 | KEY_B2 | KEY_B3 | KEY_B4 | KEY_B5 | KEY_B6 | KEY_B7 | KEY_B8;
      if (keys->front & dots) {
        uint32_t modifiers = keys->front & ~dots;
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
      case (KEY_B2 | KEY_B4 | KEY_B5 | KEY_SPACE_LEFT):
        setState(BDS_OFF);
        *command = EOF;
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
  static const unsigned char keys[] = {
    HT_BWK_Backward,
    HT_BWK_Forward,
    HT_BWK_Escape,
    HT_BWK_Enter,
    0
  };

  const unsigned char *key = keys;
  const HT_KeySet set = HT_SET_NavigationKeys;

  if (!byte) return 0;
  {
    unsigned char bits = byte;
    while (*key) bits &= ~*key++;
    if (bits) return 0;
    key = keys;
  }

  while (*key) {
    if ((byte & *key) && !enqueueKeyEvent(set, *key, 1)) return 0;
    key += 1;
  }

  do {
    key -= 1;
    if ((byte & *key) && !enqueueKeyEvent(set, *key, 0)) return 0;
  } while (key != keys);

  *command = EOF;
  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  int noInput = 1;

  if (at2Count) {
    unsigned char code = at2Buffer[0];
    memcpy(at2Buffer, at2Buffer+1, --at2Count);
    return BRL_BLK_PASSAT + code;
  }

  while (1) {
    HT_Packet packet;
    int size = brl_readPacket(brl, &packet, sizeof(packet));

    if (size == -1) return BRL_CMD_RESTARTBRL;
    if (size == 0) break;
    noInput = 0;

    /* a kludge to handle the Bookworm going offline */
    if (model->isBookworm) {
      if (packet.fields.type == 0X06) {
        if (currentState != BDS_OFF) {
          /* if we get another byte right away then the device
           * has gone offline and is echoing its display
           */
          if (io->awaitInput(10)) {
            setState(BDS_OFF);
            continue;
          }

          /* if an input error occurred then restart the driver */
          if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;

          /* no additional input so fall through and interpret the packet as keys */
        }
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
                if (model->hasATC) touchAnalyzeCells(brl, prevData);
                setState(BDS_READY);
                continue;

              case HT_PKT_Extended: {
                unsigned char length UNUSED = packet.fields.data.extended.length - 1;
                const unsigned char *bytes = &packet.fields.data.extended.data.bytes[0];

                switch (packet.fields.data.extended.type) {
                  case HT_EXTPKT_Confirmation:
                    switch (bytes[0]) {
                      case HT_PKT_NAK:
                        updateRequired = 1;
                      case HT_PKT_ACK:
                        if (model->hasATC) touchAnalyzeCells(brl, prevData);
                        setState(BDS_READY);
                        continue;

                      default:
                        break;
                    }
                    break;

                  default:
                    break;
                }
                break;
              }

              default:
                break;
            }

          case BDS_READY:
            switch (packet.fields.type) {
              case HT_PKT_Extended: {
                unsigned char length = packet.fields.data.extended.length - 1;
                const unsigned char *bytes = &packet.fields.data.extended.data.bytes[0];

                switch (packet.fields.data.extended.type) {
                  case HT_EXTPKT_Key: {
                    int command;
                    if (model->interpretByte(context, bytes[0], &command)) {
                      updateCells(brl);
                      return command;
                    }
                    break;
                  }

                  case HT_EXTPKT_Scancode: {
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
                      return BRL_BLK_PASSAT + code;
                    }
                    break;
                  }

                  case HT_EXTPKT_AtcInfo: {
                    unsigned int cellCount = model->textCells + model->statusCells;
                    unsigned char pressureValues[cellCount];
                    const unsigned char *pressure;

                    if (bytes[0]) {
                      int cellIndex = bytes[0] - 1;
                      int dataIndex;

                      memset(pressureValues, 0, cellCount);
                      for (dataIndex=1; dataIndex<length; dataIndex++) {
                        unsigned char byte = bytes[dataIndex];
                        unsigned char nibble;

                        nibble = byte & 0XF0;
                        pressureValues[cellIndex++] = nibble | (nibble >> 4);

                        nibble = byte & 0X0F;
                        pressureValues[cellIndex++] = nibble | (nibble << 4);
                      }

                      pressure = &pressureValues[0];
                    } else {
                      pressure = NULL;
                    }

                    {
                      int command = touchAnalyzePressure(brl, pressure);
                      if (command != EOF) return command;
                    }

                    continue;
                  }

                  default:
                    break;
                }
                break;
              }

              default: {
                int command;
                if (model->interpretByte(context, packet.fields.type, &command)) {
                  updateCells(brl);
                  return command;
                }
                break;
              }
            }
            break;
        }
        break;
    }

    logUnexpectedPacket(packet.bytes, size);
    LogPrint(LOG_WARNING, "state %d", currentState);
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
          if (!writeCells(brl)) return BRL_CMD_RESTARTBRL;
        }
        break;
    }
  }
  updateCells(brl);

  return EOF;
}

static ssize_t
brl_readPacket (BrailleDisplay *brl, void *buffer, size_t size) {
  unsigned char *packet = buffer;
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;
      int count = io->readBytes(&byte, 1, started);

      if (count != 1) {
        if (!count && started) logPartialPacket(packet, offset);
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
      if (offset == size) logTruncatedPacket(packet, offset);
      logDiscardedByte(byte);
    }

    if (++offset == length) {
      if (offset <= size) {
        int ok = 0;

        switch (packet[0]) {
          case HT_PKT_Extended:
            if (packet[length-1] == SYN) ok = 1;
            break;

          default:
            ok = 1;
            break;
        }

        if (ok) {
          logInputPacket(packet, offset);
          return length;
        }

        logCorruptPacket(packet, offset);
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

static void
brl_sensitivity (BrailleDisplay *brl, BrailleSensitivity setting) {
  if (model->hasATC) {
    unsigned char value = 0XFF - (setting * 0XF0 / BRL_SENSITIVITY_MAXIMUM);
    setAtcSensitivity(brl, value);
  }
}
