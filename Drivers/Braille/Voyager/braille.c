/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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

/* Voyager/braille.c - Braille display driver for Tieman Voyager displays.
 *
 * Written by Stéphane Doyon  <s.doyon@videotron.ca>
 *
 * It is being tested on Voyager 44, should also support Voyager 70.
 * It is designed to be compiled in BRLTTY version 4.1.
 *
 * History:
 * 0.21, January 2005:
 *       Remove gcc4 signedness/unsignedness incompatibilities.
 * 0.20, June 2004:
 *       Add statuscells parameter.
 *       Rename brlinput parameter to inputmode.
 *       Change default inputmode to no.
 *       Chorded functions work without chording when inputmode is no.
 *       Move complex routing key combinations to front/dot keys.
 *       Duplicate status key bindings on front/dot keys.
 *       Execute on first release rather than on all released.
 *       Add support for the part232 serial adapter.
 * 0.10, March 2004: Use BRLTTY core repeat functions. Add brlinput parameter
 *   and toggle to disallow braille typing.
 * 0.01, January 2004: fork from the original driver which relied on an
 *   in-kernel USB driver.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"
#include "timing.h"
#include "ascii.h"

#define BRLSTAT ST_VoyagerStyle
#include "brl_driver.h"
#include "brldefs-vo.h"


BEGIN_KEY_NAME_TABLE(all)
  KEY_SET_ENTRY(VO_SET_RoutingKeys, "RoutingKey"),

  KEY_NAME_ENTRY(VO_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(VO_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(VO_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(VO_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(VO_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(VO_KEY_Dot6, "Dot6"),
  KEY_NAME_ENTRY(VO_KEY_Dot7, "Dot7"),
  KEY_NAME_ENTRY(VO_KEY_Dot8, "Dot8"),

  KEY_NAME_ENTRY(VO_KEY_Thumb1, "Thumb1"),
  KEY_NAME_ENTRY(VO_KEY_Thumb2, "Thumb2"),
  KEY_NAME_ENTRY(VO_KEY_Left, "Left"),
  KEY_NAME_ENTRY(VO_KEY_Up, "Up"),
  KEY_NAME_ENTRY(VO_KEY_Down, "Down"),
  KEY_NAME_ENTRY(VO_KEY_Right, "Right"),
  KEY_NAME_ENTRY(VO_KEY_Thumb3, "Thumb3"),
  KEY_NAME_ENTRY(VO_KEY_Thumb4, "Thumb4"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(bp)
  KEY_NAME_ENTRY(BP_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(BP_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(BP_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(BP_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(BP_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(BP_KEY_Dot6, "Dot6"),

  KEY_NAME_ENTRY(BP_KEY_Shift, "Shift"),
  KEY_NAME_ENTRY(BP_KEY_Space, "Space"),
  KEY_NAME_ENTRY(BP_KEY_Control, "Control"),

  KEY_NAME_ENTRY(BP_KEY_JoystickEnter, "JoystickEnter"),
  KEY_NAME_ENTRY(BP_KEY_JoystickLeft, "JoystickLeft"),
  KEY_NAME_ENTRY(BP_KEY_JoystickRight, "JoystickRight"),
  KEY_NAME_ENTRY(BP_KEY_JoystickUp, "JoystickUp"),
  KEY_NAME_ENTRY(BP_KEY_JoystickDown, "JoystickDown"),

  KEY_NAME_ENTRY(BP_KEY_ScrollLeft, "ScrollLeft"),
  KEY_NAME_ENTRY(BP_KEY_ScrollRight, "ScrollRight"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(all),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(bp)
  KEY_NAME_TABLE(bp),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)
DEFINE_KEY_TABLE(bp)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
  &KEY_TABLE_DEFINITION(bp),
END_KEY_TABLE_LIST


typedef struct {
  const char *name;
  const KeyTableDefinition *keyTableDefinition;
} DeviceType;

static const DeviceType deviceType_Voyager = {
  .name = "Voyager",
  .keyTableDefinition = &KEY_TABLE_DEFINITION(all)
};

static const DeviceType deviceType_BraillePen = {
  .name = "Braille Pen",
  .keyTableDefinition = &KEY_TABLE_DEFINITION(bp)
};

typedef struct {
  const DeviceType *deviceType;
  unsigned char reportedCellCount;
  unsigned char actualCellCount;
} DeviceModel;

static const DeviceModel deviceModels[] = {
  { .reportedCellCount = 48,
    .actualCellCount = 44,
    .deviceType = &deviceType_Voyager
  }
  ,
  { .reportedCellCount = 72,
    .actualCellCount = 70,
    .deviceType = &deviceType_Voyager
  }
  ,
  { .reportedCellCount = 12,
    .actualCellCount = 12,
    .deviceType = &deviceType_BraillePen
  }
  ,
  { .reportedCellCount = 0 }
};

static const DeviceModel *deviceModel;


#define MAXIMUM_CELLS 70 /* arbitrary max for allocations */
static unsigned char cellCount;
#define IS_TEXT_RANGE(key1,key2) (((key1) <= (key2)) && ((key2) < cellCount))
#define IS_TEXT_KEY(key) IS_TEXT_RANGE((key), (key))

/* Structure to remember which keys are pressed */
typedef struct {
  uint16_t navigation;
  unsigned char routing[MAXIMUM_CELLS];
} Keys;

/* Remember which keys are pressed.
 * Updated immediately whenever a key is pressed.
 * Cleared after analysis whenever a key is released.
 */
static Keys pressedKeys;

/* Flag to reinitialize brl_readCommand() function state. */
static char keysInitialized;

static void
initializeKeys (void) {
  if (!keysInitialized) {
    memset(&pressedKeys, 0, sizeof(pressedKeys));
    keysInitialized = 1;
  }
}

static void
updateKeys (const unsigned char *packet) {
  Keys currentKeys;

  unsigned char navigationPresses[0X10];
  int navigationPressCount = 0;

  unsigned char routingPresses[6];
  int routingPressCount = 0;

  initializeKeys();
  memset(&currentKeys, 0, sizeof(currentKeys));
  currentKeys.navigation = (packet[1] << 8) | packet[0];

  {
    unsigned char key = 0;
    uint16_t bit = 0X1;

    while (key < 0X10) {
      if ((pressedKeys.navigation & bit) && !(currentKeys.navigation & bit)) {
        enqueueKeyEvent(VO_SET_NavigationKeys, key, 0);
      } else if (!(pressedKeys.navigation & bit) && (currentKeys.navigation & bit)) {
        navigationPresses[navigationPressCount++] = key;
      }

      bit <<= 1;
      key += 1;
    }
  }
  
  {
    int i;

    for (i=2; i<8; i+=1) {
      unsigned char key = packet[i];
      if (!key) break;

      if ((key < 1) || (key > cellCount)) {
        logMessage(LOG_NOTICE, "Invalid routing key number: %u", key);
        continue;
      }
      key -= 1;

      currentKeys.routing[key] = 1;
      if (!pressedKeys.routing[key]) routingPresses[routingPressCount++] = key;
    }
  }

  {
    unsigned char key;

    for (key=0; key<cellCount; key+=1)
      if (pressedKeys.routing[key] && !currentKeys.routing[key])
        enqueueKeyEvent(VO_SET_RoutingKeys, key, 0);
  }

  while (navigationPressCount)
    enqueueKeyEvent(VO_SET_NavigationKeys, navigationPresses[--navigationPressCount], 1);

  while (routingPressCount)
    enqueueKeyEvent(VO_SET_RoutingKeys, routingPresses[--routingPressCount], 1);

  pressedKeys = currentKeys;
}


typedef struct {
  int (*openPort) (char **parameters, const char *device);
  void (*closePort) (void);
  int (*getCellCount) (unsigned char *length);
  int (*logSerialNumber) (void);
  int (*logHardwareVersion) (void);
  int (*logFirmwareVersion) (void);
  int (*setDisplayVoltage) (unsigned char voltage);
  int (*getDisplayVoltage) (unsigned char *voltage);
  int (*getDisplayCurrent) (unsigned char *current);
  int (*setDisplayState) (unsigned char state);
  int (*writeBraille) (unsigned char *cells, unsigned char count, unsigned char start);
  int (*updateKeys) (void);
  int (*soundBeep) (unsigned char duration);
} InputOutputOperations;
static const InputOutputOperations *io;


#include "io_serial.h"
#define SERIAL_OPEN_DELAY 400
#define SERIAL_INITIAL_TIMEOUT 200
#define SERIAL_SUBSEQUENT_TIMEOUT 100

static SerialDevice *serialDevice = NULL;
static const char *serialDeviceNames[] = {"Adapter", "Base"};

static int
writeSerialPacket (unsigned char code, unsigned char *data, unsigned char count) {
  unsigned char buffer[2 + (count * 2)];
  unsigned char size = 0;
  unsigned char index;

  buffer[size++] = ESC;
  buffer[size++] = code;

  for (index=0; index<count; ++index)
    if ((buffer[size++] = data[index]) == buffer[0])
      buffer[size++] = buffer[0];

  logOutputPacket(buffer, size);
  return serialWriteData(serialDevice, buffer, size) != -1;
}

static int
readSerialPacket (unsigned char *packet, int size) {
  int started = 0;
  int escape = 0;
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    {
      int timeout = SERIAL_SUBSEQUENT_TIMEOUT;
      ssize_t result = serialReadData(serialDevice, &byte, 1,
                                      ((started || escape)? timeout: 0), timeout);

      if (result == 0) {
        errno = EAGAIN;
        result = -1;
      }

      if (result == -1) {
        if (started) logPartialPacket(packet, offset);
        return 0;
      }
    }

    if (byte == ESC) {
      if ((escape = !escape)) continue;
    } else if (escape) {
      escape = 0;

      if (offset > 0) {
        logShortPacket(packet, offset);
        offset = 0;
        length = 0;
      } else {
        started = 1;
      }
    }

    if (!started) {
      logIgnoredByte(byte);
      continue;
    }

    if (offset < size) {
      if (offset == 0) {
        switch (byte) {
          case 0X43:
          case 0X47:
            length = 2;
            break;

          case 0X4C:
            length = 3;
            break;

          case 0X46:
          case 0X48:
            length = 5;
            break;

          case 0X4B:
            length = 9;
            break;

          case 0X53:
            length = 10;
            break;

          default:
            logUnknownPacket(byte);
            started = 0;
            continue;
        }
      }

      packet[offset] = byte;
    } else {
      if (offset == size) logTruncatedPacket(packet, offset);
      logDiscardedByte(byte);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        started = 0;
        continue;
      }

      logInputPacket(packet, offset);
      return length;
    }
  }
}

static int
nextSerialPacket (unsigned char code, unsigned char *buffer, int size, int wait) {
  int length;

  if (wait)
    if (!serialAwaitInput(serialDevice, SERIAL_INITIAL_TIMEOUT))
      return 0;

  while ((length = readSerialPacket(buffer, size))) {
    if (buffer[0] == code) return length;
    logUnexpectedPacket(buffer, length);
  }
  return 0;
}

static int
openSerialPort (char **parameters, const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, 38400)) {
      if (serialSetFlowControl(serialDevice, SERIAL_FLOW_HARDWARE)) {
        approximateDelay(SERIAL_OPEN_DELAY);
        return 1;
      }
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
getSerialCellCount (unsigned char *count) {
  const unsigned int code = 0X4C;
  if (writeSerialPacket(code, NULL, 0)) {
    unsigned char buffer[3];
    if (nextSerialPacket(code, buffer, sizeof(buffer), 1)) {
      *count = buffer[2];
      return 1;
    }
  }
  return 0;
}

static int
logSerialSerialNumber (void) {
  unsigned char device;

  for (device=0; device<ARRAY_COUNT(serialDeviceNames); ++device) {
    const unsigned char code = 0X53;
    unsigned char buffer[10];

    if (!writeSerialPacket(code, &device, 1)) return 0;
    if (!nextSerialPacket(code, buffer, sizeof(buffer), 1)) return 0;
    logMessage(LOG_INFO, "Voyager %s Serial Number: %02X%02X%02X%02X%02X%02X%02X%02X",
               serialDeviceNames[buffer[1]],
               buffer[2], buffer[3], buffer[4], buffer[5],
               buffer[6], buffer[7], buffer[8], buffer[9]);
  }

  return 1;
}

static int
logSerialHardwareVersion (void) {
  unsigned char device;

  for (device=0; device<ARRAY_COUNT(serialDeviceNames); ++device) {
    const unsigned char code = 0X48;
    unsigned char buffer[5];

    if (!writeSerialPacket(code, &device, 1)) return 0;
    if (!nextSerialPacket(code, buffer, sizeof(buffer), 1)) return 0;
    logMessage(LOG_INFO, "Voyager %s Hardware Version: %c.%c.%c", 
               serialDeviceNames[buffer[1]],
               buffer[2], buffer[3], buffer[4]);
  }

  return 1;
}

static int
logSerialFirmwareVersion (void) {
  unsigned char device;

  for (device=0; device<ARRAY_COUNT(serialDeviceNames); ++device) {
    const unsigned char code = 0X46;
    unsigned char buffer[5];

    if (!writeSerialPacket(code, &device, 1)) return 0;
    if (!nextSerialPacket(code, buffer, sizeof(buffer), 1)) return 0;
    logMessage(LOG_INFO, "Voyager %s Firmware Version: %c.%c.%c", 
               serialDeviceNames[buffer[1]],
               buffer[2], buffer[3], buffer[4]);
  }

  return 1;
}

static int
setSerialDisplayVoltage (unsigned char voltage) {
  return writeSerialPacket(0X56, &voltage, 1);
}

static int
getSerialDisplayVoltage (unsigned char *voltage) {
  const unsigned char code = 0X47;
  if (writeSerialPacket(code, NULL, 0)) {
    unsigned char buffer[2];
    if (nextSerialPacket(code, buffer, sizeof(buffer), 1)) {
      *voltage = buffer[1];
      return 1;
    }
  }
  return 0;
}

static int
getSerialDisplayCurrent (unsigned char *current) {
  const unsigned int code = 0X43;
  if (writeSerialPacket(code, NULL, 0)) {
    unsigned char buffer[2];
    if (nextSerialPacket(code, buffer, sizeof(buffer), 1)) {
      *current = buffer[1];
      return 1;
    }
  }
  return 0;
}

static int
setSerialDisplayState (unsigned char state) {
  return writeSerialPacket(0X44, &state, 1);
}

static int
writeSerialBraille (unsigned char *cells, unsigned char count, unsigned char start) {
  unsigned char buffer[2 + count];
  unsigned char size = 0;
  buffer[size++] = start;
  buffer[size++] = count;
  memcpy(&buffer[size], cells, count);
  size += count;
  return writeSerialPacket(0X42, buffer, size);
}

static int
updateSerialKeys (void) {
  const unsigned char code = 0X4B;
  unsigned char packet[9];

  while (nextSerialPacket(code, packet, sizeof(packet), 0)) {
    updateKeys(&packet[1]);
  }

  return 1;
}

static int
soundSerialBeep (unsigned char duration) {
  return writeSerialPacket(0X41, &duration, 1);
}

static const InputOutputOperations serialOperations = {
  openSerialPort, closeSerialPort, getSerialCellCount,
  logSerialSerialNumber, logSerialHardwareVersion, logSerialFirmwareVersion,
  setSerialDisplayVoltage, getSerialDisplayVoltage, getSerialDisplayCurrent,
  setSerialDisplayState, writeSerialBraille,
  updateSerialKeys, soundSerialBeep
};


#include "io_usb.h"

/* Workarounds for control transfer flakiness (at least in this demo model) */
#define USB_RETRIES 6

static UsbChannel *usb = NULL;

static int
writeUsbData (
  uint8_t request, uint16_t value, uint16_t index,
  const unsigned char *buffer, uint16_t size
) {
  int retry = 0;
  while (1) {
    int ret = usbControlWrite(usb->device, UsbControlRecipient_Endpoint, UsbControlType_Vendor,
                              request, value, index, buffer, size, 100);
    if (ret != -1) return 1;
    if ((errno != EPIPE) || (retry == USB_RETRIES)) return 0;
    logMessage(LOG_WARNING, "Voyager request 0X%X retry #%d.", request, ++retry);
  }
}

static int
readUsbData (uint8_t request, uint16_t value, uint16_t index,
	     unsigned char *buffer, uint16_t size) {
  int retry = 0;
  while (1) {
    int ret = usbControlRead(usb->device, UsbControlRecipient_Endpoint, UsbControlType_Vendor,
                             request, value, index, buffer, size, 100);
    if ((ret != -1) || (errno != EPIPE) || (retry == USB_RETRIES)) return ret;
    logMessage(LOG_WARNING, "Voyager request 0X%X retry #%d.", request, ++retry);
  }
}

static int
openUsbPort (char **parameters, const char *device) {
  static const UsbChannelDefinition definitions[] = {
    { .vendor=0X0798, .product=0X0001, 
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1
    }
    ,
    { .vendor=0 }
  };

  if ((usb = usbFindChannel(definitions, (void *)device))) {
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
getUsbCellCount (unsigned char *count) {
  unsigned char buffer[2];
  int size = readUsbData(0X06, 0, 0, buffer, sizeof(buffer));
  if (size == -1) return 0;

  *count = buffer[1];
  return 1;
}

static int
logUsbString (uint8_t request, const char *description) {
  UsbDescriptor descriptor;
  if (readUsbData(request, 0, 0, descriptor.bytes, sizeof(descriptor.bytes)) != -1) {
    char *string = usbDecodeString(&descriptor.string);
    if (string) {
      logMessage(LOG_INFO, "Voyager %s: %s", description, string);
      free(string);
      return 1;
    }
  }
  return 0;
}

static int
logUsbSerialNumber (void) {
  return logUsbString(0X03, "Serial Number");
}

static int
logUsbHardwareVersion (void) {
  unsigned char buffer[2];
  int size = readUsbData(0X04, 0, 0, buffer, sizeof(buffer));
  if (size == -1) return 0;

  logMessage(LOG_INFO, "Voyager Hardware Version: %u.%u",
             buffer[0], buffer[1]);
  return 1;
}

static int
logUsbFirmwareVersion (void) {
  return logUsbString(0X05, "Firmware Version");
}

static int
setUsbDisplayVoltage (unsigned char voltage) {
  return writeUsbData(0X01, voltage, 0, NULL, 0);
}

static int
getUsbDisplayVoltage (unsigned char *voltage) {
  unsigned char buffer[1];
  int size = readUsbData(0X02, 0, 0, buffer, sizeof(buffer));
  if (size == -1) return 0;

  *voltage = buffer[0];
  return 1;
}

static int
getUsbDisplayCurrent (unsigned char *current) {
  unsigned char buffer[1];
  int size = readUsbData(0X08, 0, 0, buffer, sizeof(buffer));
  if (size == -1) return 0;

  *current = buffer[0];
  return 1;
}

static int
setUsbDisplayState (unsigned char state) {
  return writeUsbData(0X00, state, 0, NULL, 0);
}

static int
writeUsbBraille (unsigned char *cells, unsigned char count, unsigned char start) {
  return writeUsbData(0X07, 0, start, cells, count);
}

static int
updateUsbKeys (void) {
  while (1) {
    unsigned char packet[8];

    {
      int size = usbReapInput(usb->device, usb->definition.inputEndpoint,
                              packet, sizeof(packet),
                              0, 0);

      if (size < 0) {
        if (errno == EAGAIN) {
          /* no input */
          return 1;
        }

        if (errno == ENODEV) {
          /* Display was disconnected */
          return 0;
        }

        logMessage(LOG_ERR, "Voyager read error: %s", strerror(errno));
        keysInitialized = 0;
        return 1;
      }

      if ((size > 0) && (size < sizeof(packet))) {
        /* The display handles read requests of only and exactly 8 bytes */
        logMessage(LOG_NOTICE, "Short read: %d", size);
        keysInitialized = 0;
        return 1;
      }

      if (size == 0) {
        /* no new key */
        return 1;
      }
    }

    updateKeys(packet);
  }
}

static int
soundUsbBeep (unsigned char duration) {
  return writeUsbData(0X09, duration, 0, NULL, 0);
}

static const InputOutputOperations usbOperations = {
  openUsbPort, closeUsbPort, getUsbCellCount,
  logUsbSerialNumber, logUsbHardwareVersion, logUsbFirmwareVersion,
  setUsbDisplayVoltage, getUsbDisplayVoltage, getUsbDisplayCurrent,
  setUsbDisplayState, writeUsbBraille,
  updateUsbKeys, soundUsbBeep
};


/* Global variables */
static unsigned char *currentCells = NULL; /* buffer to prepare new pattern */
static unsigned char *previousCells = NULL; /* previous pattern displayed */

/* Voltage: from 0->300V to 255->200V.
 * Presumably this is voltage for dot firmness.
 * Presumably 0 makes dots hardest, 255 makes them softest.
 * We are told 265V is normal operating voltage but we don't know the scale.
 */
static int
setFirmness (BrailleDisplay *brl, BrailleFirmness setting) {
  unsigned char voltage = 0XFF - (setting * 0XFF / BRL_FIRMNESS_MAXIMUM);
  logMessage(LOG_DEBUG, "Setting voltage: %02X", voltage);
  return io->setDisplayVoltage(voltage);
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if (isSerialDevice(&device)) {
    io = &serialOperations;
  } else if (isUsbDevice(&device)) {
    io = &usbOperations;
  } else {
    unsupportedDevice(device);
    return 0;
  }

  if (io->openPort(parameters, device)) {
    if (io->getCellCount(&cellCount)) {
      deviceModel = deviceModels;

      while (deviceModel->reportedCellCount) {
        if (deviceModel->reportedCellCount == cellCount) {
          const DeviceType *deviceType = deviceModel->deviceType;

          cellCount = deviceModel->actualCellCount;
          logMessage(LOG_INFO, "Voyager Cell Count: %u", cellCount);

          io->logSerialNumber();
          io->logHardwareVersion();
          io->logFirmwareVersion();

          /* currentCells holds the status cells and the text cells.
           * We export directly to BRLTTY only the text cells.
           */
          brl->textColumns = cellCount;		/* initialize size of display */
          brl->textRows = 1;		/* always 1 */

          {
            const KeyTableDefinition *ktd = deviceType->keyTableDefinition;
            brl->keyBindings = ktd->bindings;
            brl->keyNameTables = ktd->names;
          }

          brl->setFirmness = setFirmness;

          if ((currentCells = malloc(cellCount))) {
            if ((previousCells = malloc(cellCount))) {
              /* Force rewrite of display */
              memset(currentCells, 0, cellCount); /* no dots */
              memset(previousCells, 0XFF, cellCount); /* all dots */

              if (io->setDisplayState(1)) {
                makeOutputTable(dotsTable_ISO11548_1);
                keysInitialized = 0;

                io->soundBeep(200);
                return 1;
              }

              free(previousCells);
              previousCells = NULL;
            } else {
              logMallocError();
            }

            free(currentCells);
            currentCells = NULL;
          } else {
            logMallocError();
          }

          break;
        }

        deviceModel += 1;
      }

      if (!deviceModel->reportedCellCount) {
        logMessage(LOG_ERR, "Unsupported Voyager cell count: %u", cellCount);
        deviceModel = NULL;
      }
    }

    io->closePort();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  io->closePort();

  if (currentCells) {
    free(currentCells);
    currentCells = NULL;
  }

  if (previousCells) {
    free(previousCells);
    previousCells = NULL;
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(previousCells, brl->buffer, cellCount, NULL, NULL)) {
    translateOutputCells(currentCells, brl->buffer, cellCount);

    /* The firmware supports multiples of 8 cells, so there are extra cells
     * in the firmware's imagination that don't actually exist physically.
     */
    switch (cellCount) {
      case 44: {
        /* Two ghost cells at the beginning of the display,
         * plus two more after the sixth physical cell.
         */
        unsigned char buffer[46];
        memcpy(buffer, currentCells, 6);
        buffer[6] = buffer[7] = 0;
        memcpy(buffer+8, currentCells+6, 38);
        if (!io->writeBraille(buffer, 46, 2)) return 0;
        break;
      }

      case 70:
        /* Two ghost cells at the beginning of the display. */
        if (!io->writeBraille(currentCells, cellCount, 2)) return 0;
        break;

      default:
        /* No ghost cells. */
        if (!io->writeBraille(currentCells, cellCount, 0)) return 0;
        break;
    }
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return io->updateKeys()? EOF: BRL_CMD_RESTARTBRL;
}
