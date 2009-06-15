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

#define BRLSTAT ST_AlvaStyle
#define BRL_HAVE_STATUS_CELLS
#define BRL_HAVE_PACKET_IO
#define BRL_HAVE_SENSITIVITY
#include "brl_driver.h"
#include "touch.h"
#include "brldefs-ht.h"

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
  KEY_NAME_ENTRY(HT_KEY_Escape, "LeftRockerTop"),
  KEY_NAME_ENTRY(HT_KEY_Return, "LeftRockerBottom"),

  KEY_NAME_ENTRY(HT_KEY_Up, "RightRockerTop"),
  KEY_NAME_ENTRY(HT_KEY_Down, "RightRockerBottom"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_modular) = {
  KEY_NAME_ENTRY(HT_KEY_Up, "Up"),
  KEY_NAME_ENTRY(HT_KEY_Down, "Down"),

  KEY_NAME_ENTRY(HT_KEY_STATUS+0, "Status1"),
  KEY_NAME_ENTRY(HT_KEY_STATUS+1, "Status2"),
  KEY_NAME_ENTRY(HT_KEY_STATUS+2, "Status3"),
  KEY_NAME_ENTRY(HT_KEY_STATUS+3, "Status4"),

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

static const unsigned char BookwormSessionEnd[] = {0X05, 0X07};	/* bookworm trailer to display braille */

typedef int (ByteInterpreter) (unsigned char byte);
static ByteInterpreter interpretKeyByte;
static ByteInterpreter interpretBookwormByte;

typedef int (CellWriter) (BrailleDisplay *brl);
static CellWriter writeStatusAndTextCells;
static CellWriter writeBookwormCells;
static CellWriter writeEvolutionCells;

typedef struct {
  const char *name;
  const char *keyBindings;
  const KeyNameEntry *const *keyNameTables;

  ByteInterpreter *interpretByte;
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
    .keyNameTables = keyNameTables_modular,
    .interpretByte = interpretKeyByte,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_Modular40,
    .name = "Modular 40+4",
    .textCells = 40,
    .statusCells = 4,
    .keyBindings = keyBindings_modular,
    .keyNameTables = keyNameTables_modular,
    .interpretByte = interpretKeyByte,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_Modular80,
    .name = "Modular 80+4",
    .textCells = 80,
    .statusCells = 4,
    .keyBindings = keyBindings_modular,
    .keyNameTables = keyNameTables_modular,
    .interpretByte = interpretKeyByte,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_ModularEvolution64,
    .name = "Modular Evolution 64",
    .textCells = 64,
    .statusCells = 0,
    .keyBindings = keyBindings_modularEvolution64,
    .keyNameTables = keyNameTables_modularEvolution64,
    .interpretByte = interpretKeyByte,
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
    .interpretByte = interpretKeyByte,
    .writeCells = writeEvolutionCells,
    .hasATC = 1
  }
  ,
  { .identifier = HT_Model_BrailleWave,
    .name = "Braille Wave 40",
    .textCells = 40,
    .statusCells = 0,
    .keyBindings = keyBindings_brailleWave,
    .keyNameTables = keyNameTables_brailleWave,
    .interpretByte = interpretKeyByte,
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
    .keyNameTables = keyNameTables_brailleStar40,
    .interpretByte = interpretKeyByte,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_BrailleStar40,
    .name = "Braille Star 40",
    .textCells = 40,
    .statusCells = 0,
    .keyBindings = keyBindings_brailleStar40,
    .keyNameTables = keyNameTables_brailleStar40,
    .interpretByte = interpretKeyByte,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_BrailleStar80,
    .name = "Braille Star 80",
    .textCells = 80,
    .statusCells = 0,
    .keyBindings = keyBindings_brailleStar80,
    .keyNameTables = keyNameTables_brailleStar80,
    .interpretByte = interpretKeyByte,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { .identifier = HT_Model_EasyBraille,
    .name = "Easy Braille",
    .textCells = 40,
    .statusCells = 0,
    .keyBindings = keyBindings_modular,
    .keyNameTables = keyNameTables_modular,
    .interpretByte = interpretKeyByte,
    .writeCells = writeStatusAndTextCells
  }
  ,
  { /* end of table */
    .name = NULL
  }
};
#undef HT_BYTE_SEQUENCE

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
interpretKeyByte (unsigned char byte) {
  int release = (byte & HT_KEY_RELEASE) != 0;
  if (release) byte &= ~HT_KEY_RELEASE;

  if ((byte >= HT_KEY_ROUTING) &&
      (byte < (HT_KEY_ROUTING + model->textCells))) {
    return enqueueKeyEvent(HT_SET_RoutingKeys, byte-HT_KEY_ROUTING, !release);
  }

  if ((byte >= HT_KEY_STATUS) &&
      (byte < (HT_KEY_STATUS + model->statusCells))) {
    return enqueueKeyEvent(HT_SET_NavigationKeys, byte, !release);
  }

  if ((byte > 0) && (byte < 0X20)) {
    return enqueueKeyEvent(HT_SET_NavigationKeys, byte, !release);
  }

  return 0;
}

static int
interpretBookwormByte (unsigned char byte) {
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
                  case HT_EXTPKT_Key:
                    if (model->interpretByte(bytes[0])) {
                      updateCells(brl);
                      return EOF;
                    }
                    break;

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

              default:
                if (model->interpretByte(packet.fields.type)) {
                  updateCells(brl);
                  return EOF;
                }
                break;
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
