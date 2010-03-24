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

/* Albatross/braille.c - Braille display library
 * Tivomatic's Albatross series
 * Author: Dave Mielke <dave@mielke.cc>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "log.h"
#include "timing.h"

#define BRL_HAVE_STATUS_CELLS
#include "brl_driver.h"
#include "brldefs-at.h"

BEGIN_KEY_NAME_TABLE(all)
  /* front left keys */
  KEY_NAME_ENTRY(AT_KEY_Home1, "Home1"),
  KEY_NAME_ENTRY(AT_KEY_End1, "End1"),
  KEY_NAME_ENTRY(AT_KEY_ExtraCursor1, "ExtraCursor1"),
  KEY_NAME_ENTRY(AT_KEY_Cursor1, "Cursor1"),
  KEY_NAME_ENTRY(AT_KEY_Up1, "Up1"),
  KEY_NAME_ENTRY(AT_KEY_Down1, "Down1"),
  KEY_NAME_ENTRY(AT_KEY_Left, "Left"),

  /* front right keys */
  KEY_NAME_ENTRY(AT_KEY_Home2, "Home2"),
  KEY_NAME_ENTRY(AT_KEY_End2, "End2"),
  KEY_NAME_ENTRY(AT_KEY_ExtraCursor2, "ExtraCursor2"),
  KEY_NAME_ENTRY(AT_KEY_Cursor2, "Cursor2"),
  KEY_NAME_ENTRY(AT_KEY_Up2, "Up2"),
  KEY_NAME_ENTRY(AT_KEY_Down2, "Down2"),
  KEY_NAME_ENTRY(AT_KEY_Right, "Right"),

  /* front middle keys */
  KEY_NAME_ENTRY(AT_KEY_Up3, "Up3"),
  KEY_NAME_ENTRY(AT_KEY_Down3, "Down3"),

  /* top left keys */
  KEY_NAME_ENTRY(AT_KEY_F1, "F1"),
  KEY_NAME_ENTRY(AT_KEY_F2, "F2"),
  KEY_NAME_ENTRY(AT_KEY_F3, "F3"),
  KEY_NAME_ENTRY(AT_KEY_F4, "F4"),
  KEY_NAME_ENTRY(AT_KEY_F5, "F5"),
  KEY_NAME_ENTRY(AT_KEY_F6, "F6"),
  KEY_NAME_ENTRY(AT_KEY_F7, "F7"),
  KEY_NAME_ENTRY(AT_KEY_F8, "F8"),

  /* top right keys */
  KEY_NAME_ENTRY(AT_KEY_F9, "F9"),
  KEY_NAME_ENTRY(AT_KEY_F10, "F10"),
  KEY_NAME_ENTRY(AT_KEY_F11, "F11"),
  KEY_NAME_ENTRY(AT_KEY_F12, "F12"),
  KEY_NAME_ENTRY(AT_KEY_F13, "F13"),
  KEY_NAME_ENTRY(AT_KEY_F14, "F14"),
  KEY_NAME_ENTRY(AT_KEY_F15, "F15"),
  KEY_NAME_ENTRY(AT_KEY_F16, "F16"),

  /* attribute keys */
  KEY_NAME_ENTRY(AT_KEY_Attribute1, "Attribute1"),
  KEY_NAME_ENTRY(AT_KEY_Attribute2, "Attribute2"),
  KEY_NAME_ENTRY(AT_KEY_Attribute3, "Attribute3"),
  KEY_NAME_ENTRY(AT_KEY_Attribute4, "Attribute4"),

  /* wheels */
  KEY_NAME_ENTRY(AT_KEY_LeftWheelRight, "LeftWheelRight"),
  KEY_NAME_ENTRY(AT_KEY_LeftWheelLeft, "LeftWheelLeft"),
  KEY_NAME_ENTRY(AT_KEY_LeftWheelUp, "LeftWheelUp"),
  KEY_NAME_ENTRY(AT_KEY_LeftWheelDown, "LeftWheelDown"),
  KEY_NAME_ENTRY(AT_KEY_RightWheelRight, "RightWheelRight"),
  KEY_NAME_ENTRY(AT_KEY_RightWheelLeft, "RightWheelLeft"),
  KEY_NAME_ENTRY(AT_KEY_RightWheelUp, "RightWheelUp"),
  KEY_NAME_ENTRY(AT_KEY_RightWheelDown, "RightWheelDown"),

  /* routing keys */
  KEY_SET_ENTRY(AT_SET_RoutingKeys1, "RoutingKey1"),
  KEY_SET_ENTRY(AT_SET_RoutingKeys2, "RoutingKey2"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(all),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

typedef struct {
  int (*openPort) (const char *device);
  int (*configurePort) (int baud);
  void (*closePort) (void);
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, size_t size, int wait);
  int (*writeBytes) (const unsigned char *buffer, size_t size);
} InputOutputOperations;

static const InputOutputOperations *io;
static int charactersPerSecond;

#include "io_serial.h"

static SerialDevice *serialDevice = NULL;

static int
openSerialPort (const char *device) {
  if ((serialDevice = serialOpenDevice(device))) return 1;
  return 0;
}

static int
configureSerialPort (int baud) {
  return serialRestartDevice(serialDevice, baud);
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
readSerialBytes (unsigned char *buffer, size_t size, int wait) {
  const int timeout = 100;
  return serialReadData(serialDevice, buffer, size,
                        (wait? timeout: 0), timeout);
}

static int
writeSerialBytes (const unsigned char *buffer, size_t size) {
  return serialWriteData(serialDevice, buffer, size);
}

static const InputOutputOperations serialOperations = {
  openSerialPort, configureSerialPort, closeSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialBytes
};

#include "io_usb.h"

static UsbChannel *usbChannel = NULL;

static int
openUsbPort (const char *device) {
  static const UsbChannelDefinition definitions[] = {
    { /* Albatross */
      .vendor=0X0403, .product=0X6001,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2
    }
    ,
    { .vendor=0 }
  };

  if ((usbChannel = usbFindChannel(definitions, (void *)device))) {
    return 1;
  }

  return 0;
}

static int
configureUsbPort (int baud) {
  const SerialParameters parameters = {
    .baud = baud,
    .flow = SERIAL_FLOW_NONE,
    .data = 8,
    .stop = 1,
    .parity = SERIAL_PARITY_NONE
  };

  return usbSetSerialParameters(usbChannel->device, &parameters);
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
  return usbAwaitInput(usbChannel->device, usbChannel->definition.inputEndpoint, milliseconds);
}

static int
readUsbBytes (unsigned char *buffer, size_t size, int wait) {
  const int timeout = 100;
  int count = usbReapInput(usbChannel->device,
                           usbChannel->definition.inputEndpoint,
                           buffer, size,
                           (wait? timeout: 0), timeout);
  if (count != -1) return count;
  if (errno == EAGAIN) return 0;
  return -1;
}

static int
writeUsbBytes (const unsigned char *buffer, size_t size) {
  return usbWriteEndpoint(usbChannel->device, usbChannel->definition.outputEndpoint, buffer, size, 1000);
}

static const InputOutputOperations usbOperations = {
  openUsbPort, configureUsbPort, closeUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes
};

static TranslationTable inputMap;
static const unsigned char topLeftKeys[]  = {
  AT_KEY_F1, AT_KEY_F2, AT_KEY_F3, AT_KEY_F4,
  AT_KEY_F5, AT_KEY_F6, AT_KEY_F7, AT_KEY_F8
};
static const unsigned char topRightKeys[] = {
  AT_KEY_F9, AT_KEY_F10, AT_KEY_F11, AT_KEY_F12,
  AT_KEY_F13, AT_KEY_F14, AT_KEY_F15, AT_KEY_F16
};

static unsigned char controlKey;
#define NO_CONTROL_KEY 0XFF

static TranslationTable outputTable;
static unsigned char displayContent[80];
static int displaySize;
static int windowWidth;
static int windowStart;
static int statusCount;
static int statusStart;

static int
readByte (unsigned char *byte) {
  int received = io->readBytes(byte, 1, 0);
  if (received == -1) LogError("Albatross read");
  return received == 1;
}

static void
discardInput (void) {
  unsigned char byte;
  while (readByte(&byte));
}

static int
awaitByte (unsigned char *byte) {
  if (readByte(byte)) return 1;

  if (io->awaitInput(1000))
    if (readByte(byte))
      return 1;

  return 0;
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, int count) {
  brl->writeDelay += (count * 1000 / charactersPerSecond) + 1;
  if (io->writeBytes(bytes, count) != -1) return 1;
  LogError("Albatross write");
  return 0;
}

static int
acknowledgeDisplay (BrailleDisplay *brl) {
  unsigned char description;
  if (!awaitByte(&description)) return 0;
  if (description == 0XFF) return 0;

  {
    unsigned char byte;

    if (!awaitByte(&byte)) return 0;
    if (byte != 0XFF) return 0;

    if (!awaitByte(&byte)) return 0;
    if (byte != description) return 0;
  }

  {
    static const unsigned char acknowledgement[] = {0XFE, 0XFF, 0XFE, 0XFF};
    if (!writeBytes(brl, acknowledgement, sizeof(acknowledgement))) return 0;

    discardInput();
    approximateDelay(100);
    discardInput();
  }
  LogPrint(LOG_DEBUG, "Albatross description byte: %02X", description);

  windowStart = statusStart = 0;
  displaySize = (description & 0X80)? 80: 46;
  if ((statusCount = description & 0X0F)) {
    windowWidth = displaySize - statusCount - 1;
    if (description & 0X20) {
      statusStart = windowWidth + 1;
      displayContent[statusStart - 1] = 0;
    } else {
      windowStart = statusCount + 1;
      displayContent[windowStart - 1] = 0;
    }
  } else {
    windowWidth = displaySize;
  }

  {
    int i;
    for (i=0; i<sizeof(inputMap); ++i) inputMap[i] = i;

    /* top keypad remapping */
    {
      const unsigned char *left = NULL;
      const unsigned char *right = NULL;

      switch (description & 0X50) {
        case 0X00: /* left right */
          break;

        case 0X10: /* right right */
          left = topRightKeys;
          break;

        case 0X50: /* left left */
          right = topLeftKeys;
          break;

        case 0X40: /* right left */
          left = topRightKeys;
          right = topLeftKeys;
          break;
      }

      if (left)
        for (i=0; i<8; ++i)
          inputMap[topLeftKeys[i]] = left[i];

      if (right)
        for (i=0; i<8; ++i)
          inputMap[topRightKeys[i]] = right[i];
    }
  }

  LogPrint(LOG_INFO, "Albatross: %d cells (%d text, %d%s status), top keypads [%s,%s].",
           displaySize, windowWidth, statusCount,
           !statusCount? "": statusStart? " right": " left",
           (inputMap[topLeftKeys[0]] == topLeftKeys[0])? "left": "right",
           (inputMap[topRightKeys[0]] == topRightKeys[0])? "right": "left");
  return 1;
}

static int
clearDisplay (BrailleDisplay *brl) {
  unsigned char bytes[] = {0XFA};
  int cleared = writeBytes(brl, bytes, sizeof(bytes));
  if (cleared) memset(displayContent, 0, displaySize);
  return cleared;
}

static int
updateDisplay (BrailleDisplay *brl, const unsigned char *cells, int count, int start) {
  static time_t lastUpdate = 0;
  unsigned char bytes[count * 2 + 2];
  unsigned char *byte = bytes;
  int index;
  *byte++ = 0XFB;
  for (index=0; index<count; ++index) {
    unsigned char cell;
    if (!cells) {
      cell = displayContent[start+index];
    } else if ((cell = outputTable[cells[index]]) != displayContent[start+index]) {
      displayContent[start+index] = cell;
    } else {
      continue;
    }
    *byte++ = start + index + 1;
    *byte++ = cell;
  }

  if (((byte - bytes) > 1) || (time(NULL) != lastUpdate)) {
    *byte++ = 0XFC;
    if (!writeBytes(brl, bytes, byte-bytes)) return 0;
    lastUpdate = time(NULL);
  }
  return 1;
}

static int
updateWindow (BrailleDisplay *brl, const unsigned char *cells) {
  return updateDisplay(brl, cells, windowWidth, windowStart);
}

static int
updateStatus (BrailleDisplay *brl, const unsigned char *cells) {
  return updateDisplay(brl, cells, statusCount, statusStart);
}

static int
refreshDisplay (BrailleDisplay *brl) {
  return updateDisplay(brl, NULL, displaySize, 0);
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X80, 0X40, 0X20, 0X10, 0X08, 0X04, 0X02, 0X01};
    makeOutputTable(dots, outputTable);
  }

  if (isSerialDevice(&device)) {
    io = &serialOperations;
  } else if (isUsbDevice(&device)) {
    io = &usbOperations;
  } else {
    unsupportedDevice(device);
    return 0;
  }

  if (io->openPort(device)) {
    int baudTable[] = {19200, 9600, 0};
    const int *baud = baudTable;

    while (io->configurePort(*baud)) {
      time_t start = time(NULL);
      int count = 0;
      unsigned char byte;

      charactersPerSecond = *baud / 10;
      controlKey = NO_CONTROL_KEY;

      LogPrint(LOG_DEBUG, "Trying Albatross at %d baud.", *baud);
      while (awaitByte(&byte)) {
        if (byte == 0XFF) {
          if (!acknowledgeDisplay(brl)) break;

          clearDisplay(brl);
          brl->textColumns = windowWidth;
          brl->textRows = 1;

          {
            const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

            brl->keyBindings = ktd->bindings;
            brl->keyNameTables = ktd->names;
          }

          return 1;
        }

        if (++count == 100) break;
        if (difftime(time(NULL), start) > 5.0) break;
      }

      if (!*++baud) baud = baudTable;
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
  updateWindow(brl, brl->buffer);
  return 1;
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  updateStatus(brl, status);
  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  unsigned char byte;

  while (readByte(&byte)) {
    if (byte == 0XFF) {
      if (acknowledgeDisplay(brl)) {
        refreshDisplay(brl);
        brl->textColumns = windowWidth;
        brl->textRows = 1;
        brl->resizeRequired = 1;
      }
      continue;
    }

    byte = inputMap[byte];
    {
      unsigned char set;
      unsigned char key;

      if ((byte >= 2) && (byte <= 41)) {
        set = AT_SET_RoutingKeys1;
        key = byte - 2;
      } else if ((byte >= 111) && (byte <= 150)) {
        set = AT_SET_RoutingKeys1;
        key = byte - 71;
      } else if ((byte >= 43) && (byte <= 82)) {
        set = AT_SET_RoutingKeys2;
        key = byte - 43;
      } else if ((byte >= 152) && (byte <= 191)) {
        set = AT_SET_RoutingKeys2;
        key = byte - 112;
      } else {
        goto notRouting;
      }

      if ((key >= windowStart) &&
          (key < (windowStart + windowWidth))) {
        key -= windowStart;
        enqueueKeyEvent(set, key, 1);
        enqueueKeyEvent(set, key, 0);
        continue;
      }
    }
  notRouting:

    switch (byte) {
      default:
        break;

      case 0XFB:
        refreshDisplay(brl);
        continue;

      case AT_KEY_Attribute1:
      case AT_KEY_Attribute2:
      case AT_KEY_Attribute3:
      case AT_KEY_Attribute4:
      case AT_KEY_F1:
      case AT_KEY_F2:
      case AT_KEY_F7:
      case AT_KEY_F8:
      case AT_KEY_F9:
      case AT_KEY_F10:
      case AT_KEY_F15:
      case AT_KEY_F16:
      case AT_KEY_Home1:
      case AT_KEY_Home2:
      case AT_KEY_End1:
      case AT_KEY_End2:
      case AT_KEY_ExtraCursor1:
      case AT_KEY_ExtraCursor2:
      case AT_KEY_Cursor1:
      case AT_KEY_Cursor2:
        if (byte == controlKey) {
          controlKey = NO_CONTROL_KEY;
          enqueueKeyEvent(AT_SET_NavigationKeys, byte, 0);
          continue;
        }

        if (controlKey == NO_CONTROL_KEY) {
          controlKey = byte;
          enqueueKeyEvent(AT_SET_NavigationKeys, byte, 1);
          continue;
        }

      case AT_KEY_Up1:
      case AT_KEY_Down1:
      case AT_KEY_Left:
      case AT_KEY_Up2:
      case AT_KEY_Down2:
      case AT_KEY_Right:
      case AT_KEY_Up3:
      case AT_KEY_Down3:
      case AT_KEY_F3:
      case AT_KEY_F4:
      case AT_KEY_F5:
      case AT_KEY_F6:
      case AT_KEY_F11:
      case AT_KEY_F12:
      case AT_KEY_F13:
      case AT_KEY_F14:
      case AT_KEY_LeftWheelRight:
      case AT_KEY_LeftWheelLeft:
      case AT_KEY_LeftWheelUp:
      case AT_KEY_LeftWheelDown:
      case AT_KEY_RightWheelRight:
      case AT_KEY_RightWheelLeft:
      case AT_KEY_RightWheelUp:
      case AT_KEY_RightWheelDown:
        enqueueKeyEvent(AT_SET_NavigationKeys, byte, 1);
        enqueueKeyEvent(AT_SET_NavigationKeys, byte, 0);
        continue;
    }

    logUnexpectedPacket(&byte, 1);
  }

  if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;
  return EOF;
}
