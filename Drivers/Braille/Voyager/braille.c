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

#include "misc.h"

#define BRLSTAT ST_VoyagerStyle
#define BRL_HAVE_FIRMNESS
#include "brl_driver.h"
#include "brldefs-vo.h"

typedef struct {
  int (*openPort) (char **parameters, const char *device);
  int (*preparePort) (void);
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
  int (*getKeys) (unsigned char *packet, int size);
  int (*soundBeep) (unsigned char duration);
} InputOutputOperations;
static const InputOutputOperations *io;


#include "io_serial.h"

static SerialDevice *serialDevice = NULL;
static const char *serialDeviceNames[] = {"Adapter", "Base"};

static int
writeSerialPacket (unsigned char code, unsigned char *data, unsigned char count) {
  unsigned char buffer[2 + (count * 2)];
  unsigned char size = 0;
  unsigned char index;

  buffer[size++] = 0X1B;
  buffer[size++] = code;

  for (index=0; index<count; ++index)
    if ((buffer[size++] = data[index]) == buffer[0])
      buffer[size++] = buffer[0];

  logOutputPacket(buffer, size);
  return serialWriteData(serialDevice, buffer, size) != -1;
}

static int
readSerialPacket (unsigned char *buffer, int size) {
  size_t offset = 0;
  int escape = 0;
  int length = -1;

  while ((offset < 1) || (offset < length)) {
    if (offset == size) {
      logTruncatedPacket(buffer, offset);
      offset = 0;
    }

    if (!serialReadChunk(serialDevice, buffer, &offset, 1, 0, 100)) {
      logPartialPacket(buffer, offset);
      return 0;
    }

    {
      unsigned char byte = buffer[offset - 1];

      if (byte == 0X1B) {
        if ((escape = !escape)) {
          offset--;
          continue;
        }
      }

      if (!escape) {
        if (offset == 1) {
          logIgnoredByte(byte);
          offset = 0;
        }
        continue;
      }
      escape = 0;

      if (offset > 1) {
        logTruncatedPacket(buffer, offset-1);
        buffer[0] = byte;
        offset = 1;
      }

      switch (byte) {
        case 0X43:
        case 0X47:
          length = 2;
          continue;

        case 0X4C:
          length = 3;
          continue;

        case 0X46:
        case 0X48:
          length = 5;
          continue;

        case 0X4B:
          length = 9;
          continue;

        case 0X53:
          length = 18;
          continue;

        default:
          logUnknownPacket(byte);
          offset = 0;
          continue;
      }
    }
  }

  logInputPacket(buffer, offset);
  return offset;
}

static int
nextSerialPacket (unsigned char code, unsigned char *buffer, int size) {
  int length;
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
      return 1;
    }

    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
  return 0;
}

static int
prepareSerialPort (void) {
  return serialSetFlowControl(serialDevice, SERIAL_FLOW_HARDWARE);
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
    if (nextSerialPacket(code, buffer, sizeof(buffer))) {
      *count = buffer[2];
      return 1;
    }
  }
  return 0;
}

static int
logSerialSerialNumber (void) {
  unsigned char device;
  for (device=0; device<=1; ++device) {
    const unsigned char code = 0X53;
    unsigned char buffer[18];
    if (!writeSerialPacket(code, &device, 1)) return 0;
    if (!nextSerialPacket(code, buffer, sizeof(buffer))) return 0;
    if (buffer[1] != device) continue;
    LogPrint(LOG_INFO, "Voyager %s Serial Number: %.*s", 
             serialDeviceNames[device], 16, buffer+2);
  }
  return 1;
}

static int
logSerialHardwareVersion (void) {
  unsigned char device;
  for (device=0; device<=1; ++device) {
    const unsigned char code = 0X48;
    unsigned char buffer[5];
    if (!writeSerialPacket(code, &device, 1)) return 0;
    if (!nextSerialPacket(code, buffer, sizeof(buffer))) return 0;
    if (buffer[1] != device) continue;
    LogPrint(LOG_INFO, "Voyager %s Hardware Version: %u.%u.%u", 
             serialDeviceNames[device], buffer[2], buffer[3], buffer[4]);
  }
  return 1;
}

static int
logSerialFirmwareVersion (void) {
  unsigned char device;
  for (device=0; device<=1; ++device) {
    const unsigned char code = 0X46;
    unsigned char buffer[5];
    if (!writeSerialPacket(code, &device, 1)) return 0;
    if (!nextSerialPacket(code, buffer, sizeof(buffer))) return 0;
    if (buffer[1] != device) continue;
    LogPrint(LOG_INFO, "Voyager %s Firmware Version: %u.%u.%u", 
             serialDeviceNames[device], buffer[2], buffer[3], buffer[4]);
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
    if (nextSerialPacket(code, buffer, sizeof(buffer))) {
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
    if (nextSerialPacket(code, buffer, sizeof(buffer))) {
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
getSerialKeys (unsigned char *packet, int size) {
  const int offset = 1;
  unsigned char buffer[offset + size];
  int length = nextSerialPacket(0X4B, buffer, sizeof(buffer));
  if (length) {
    memcpy(packet, buffer+offset, length-=offset);
    return length;
  }
  return -1;
}

static int
soundSerialBeep (unsigned char duration) {
  return writeSerialPacket(0X41, &duration, 1);
}

static const InputOutputOperations serialOperations = {
  openSerialPort, prepareSerialPort, closeSerialPort, getSerialCellCount,
  logSerialSerialNumber, logSerialHardwareVersion, logSerialFirmwareVersion,
  setSerialDisplayVoltage, getSerialDisplayVoltage, getSerialDisplayCurrent,
  setSerialDisplayState, writeSerialBraille,
  getSerialKeys, soundSerialBeep
};


#ifdef ENABLE_USB_SUPPORT
#include "io_usb.h"

/* Workarounds for control transfer flakiness (at least in this demo model) */
#define USB_RETRIES 6

static UsbChannel *usb = NULL;

static int
writeUsbData (uint8_t request, uint16_t value, uint16_t index,
	      const unsigned char *buffer, uint16_t size) {
  int retry = 0;
  while (1) {
    int ret = usbControlWrite(usb->device, UsbControlRecipient_Endpoint, UsbControlType_Vendor,
                              request, value, index, buffer, size, 100);
    if ((ret != -1) || (errno != EPIPE) || (retry == USB_RETRIES)) return ret;
    LogPrint(LOG_WARNING, "Voyager request 0X%X retry #%d.", request, ++retry);
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
    LogPrint(LOG_WARNING, "Voyager request 0X%X retry #%d.", request, ++retry);
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

static int
prepareUsbPort (void) {
  return 1;
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
      LogPrint(LOG_INFO, "Voyager %s: %s", description, string);
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

  LogPrint(LOG_INFO, "Voyager Hardware: %u.%u",
           buffer[0], buffer[1]);
  return 1;
}

static int
logUsbFirmwareVersion (void) {
  return logUsbString(0X05, "Firmware");
}

static int
setUsbDisplayVoltage (unsigned char voltage) {
  return writeUsbData(0X01, voltage, 0, NULL, 0) != -1;
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
  return writeUsbData(0X00, state, 0, NULL, 0) != -1;
}

static int
writeUsbBraille (unsigned char *cells, unsigned char count, unsigned char start) {
  return writeUsbData(0X07, 0, start, cells, count) != -1;
}

static int
getUsbKeys (unsigned char *packet, int size) {
  return usbReapInput(usb->device, usb->definition.inputEndpoint, packet, size, 0, 0);
}

static int
soundUsbBeep (unsigned char duration) {
  return writeUsbData(0X09, duration, 0, NULL, 0) != -1;
}

static const InputOutputOperations usbOperations = {
  openUsbPort, prepareUsbPort, closeUsbPort, getUsbCellCount,
  logUsbSerialNumber, logUsbHardwareVersion, logUsbFirmwareVersion,
  setUsbDisplayVoltage, getUsbDisplayVoltage, getUsbDisplayCurrent,
  setUsbDisplayState, writeUsbBraille,
  getUsbKeys, soundUsbBeep
};
#endif /* ENABLE_USB_SUPPORT */


/* Global variables */
static char firstRead; /* Flag to reinitialize brl_readCommand() function state. */
static TranslationTable outputTable;
static unsigned char *currentCells = NULL; /* buffer to prepare new pattern */
static unsigned char *previousCells = NULL; /* previous pattern displayed */

#define MAXIMUM_CELLS 70 /* arbitrary max for allocations */
static unsigned char cellCount;
#define IS_TEXT_RANGE(key1,key2) (((key1) <= (key2)) && ((key2) < cellCount))
#define IS_TEXT_KEY(key) IS_TEXT_RANGE((key), (key))

static KEY_NAME_TABLE(keyNames_all) = {
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

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE_LIST(keyNameTables_all) = {
  keyNames_all,
  NULL
};

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
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

  if (io->openPort(parameters, device)) {
    /* find out how big the display is */
    cellCount = 0;
    {
      unsigned char count;
      if (io->getCellCount(&count)) {
        switch (count) {
          case 48:
            cellCount = 44;
            break;

          case 72:
            cellCount = 70;
            break;

          default:
            LogPrint(LOG_ERR, "Unsupported Voyager cell count: %u", count);
            break;
        }
      }
    }

    if (cellCount) {
      LogPrint(LOG_INFO, "Voyager Cell Count: %u", cellCount);

      /* log information about the display */
      io->logSerialNumber();
      io->logHardwareVersion();
      io->logFirmwareVersion();

      /* currentCells holds the status cells and the text cells.
       * We export directly to BRLTTY only the text cells.
       */
      brl->textColumns = cellCount;		/* initialize size of display */
      brl->textRows = 1;		/* always 1 */
      brl->keyNameTables = keyNameTables_all;

      if ((currentCells = malloc(cellCount))) {
        if ((previousCells = malloc(cellCount))) {
          /* Force rewrite of display */
          memset(currentCells, 0, cellCount); /* no dots */
          memset(previousCells, 0XFF, cellCount); /* all dots */

          if (io->setDisplayState(1)) {
            io->soundBeep(200);

            {
              static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
              makeOutputTable(dots, outputTable);
            }

            firstRead = 1;
            if (io->preparePort()) return 1;
          }

          free(previousCells);
          previousCells = NULL;
        }

        free(currentCells);
        currentCells = NULL;
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
  unsigned char buffer[cellCount];

  memcpy(currentCells, brl->buffer, cellCount);

  /* If content hasn't changed, do nothing. */
  if (memcmp(previousCells, currentCells, cellCount) == 0) return 1;

  /* remember current content */
  memcpy(previousCells, currentCells, cellCount);

  /* translate to voyager dot pattern coding */
  {
    int i;
    for (i=0; i<cellCount; i++)
      buffer[i] = outputTable[currentCells[i]];
  }

  /* The firmware supports multiples of 8 cells, so there are extra cells
   * in the firmware's imagination that don't actually exist physically.
   */
  switch (cellCount) {
    case 44: {
      /* Two ghost cells at the beginning of the display,
       * plus two more after the sixth physical cell.
       */
      unsigned char hbuf[46];
      memcpy(hbuf, buffer, 6);
      hbuf[6] = hbuf[7] = 0;
      memcpy(hbuf+8, buffer+6, 38);
      io->writeBraille(hbuf, 46, 2);
      break;
    }

    case 70:
      /* Two ghost cells at the beginning of the display. */
      io->writeBraille(buffer, 70, 2);
      break;
  }
  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  /* Structure to remember which keys are pressed */
  typedef struct {
    uint16_t control; /* Front and dot keys */
    unsigned char routing[MAXIMUM_CELLS];
  } Keys;

  /* Static variables to remember the state between calls. */
  /* Remember which keys are pressed.
   * Updated immediately whenever a key is pressed.
   * Cleared after analysis whenever a key is released.
   */
  static Keys pressedKeys;

  while (1) {
    /* read buffer: packet[0] for DOT keys, packet[1] for keys A B C D UP DOWN
     * RL RR, packet[2]-packet[7] list pressed routing keys by number, maximum
     * 6 keys, list ends with 0.
     * All 0s is sent when all keys released.
     */
    unsigned char packet[8];

    /* one or more keys were pressed or released */
    Keys currentKeys;
    int i;

    unsigned char controlPresses[0X10];
    int controlPressCount = 0;

    unsigned char routingPresses[6];
    int routingPressCount = 0;

    if (firstRead) {
      /* initialize state */
      firstRead = 0;
      memset(&pressedKeys, 0, sizeof(pressedKeys));
    }

    {
      int size = io->getKeys(packet, sizeof(packet));

      if (size < 0) {
        if (errno == EAGAIN) {
          /* no input */
          size = 0;
        } else if (errno == ENODEV) {
          /* Display was disconnected */
          return BRL_CMD_RESTARTBRL;
        } else {
          LogPrint(LOG_ERR, "Voyager read error: %s", strerror(errno));
          firstRead = 1;
          return EOF;
        }
      } else if ((size > 0) && (size < sizeof(packet))) {
        /* The display handles read requests of only and exactly 8bytes */
        LogPrint(LOG_NOTICE, "Short read: %d", size);
        firstRead = 1;
        return EOF;
      }

      if (size == 0) {
        /* no new key */
        break;
      }
    }

    memset(&currentKeys, 0, sizeof(currentKeys));

    /* We combine dot and front key info in keystate */
    currentKeys.control = (packet[1] << 8) | packet[0];

    for (i=0; i<0X10; i+=1) {
      uint16_t bit = 1 << i;

      if ((pressedKeys.control & bit) && !(currentKeys.control & bit)) {
        enqueueKeyEvent(VO_SET_NavigationKeys, i+1, 0);
      } else if (!(pressedKeys.control & bit) && (currentKeys.control & bit)) {
        controlPresses[controlPressCount++] = i + 1;
      }
    }
    
    for (i=2; i<8; i+=1) {
      unsigned char key = packet[i];
      if (!key) break;

      if ((key < 1) || (key > cellCount)) {
        LogPrint(LOG_NOTICE, "Invalid routing key number: %u", key);
        continue;
      }
      key -= 1;

      currentKeys.routing[key] = 1;
      if (!pressedKeys.routing[key]) routingPresses[routingPressCount++] = key;
    }

    for (i=0; i<cellCount; i+=1)
      if (pressedKeys.routing[i] && !currentKeys.routing[i])
        enqueueKeyEvent(VO_SET_RoutingKeys, i, 0);

    while (controlPressCount)
      enqueueKeyEvent(VO_SET_NavigationKeys, controlPresses[--controlPressCount], 1);

    while (routingPressCount)
      enqueueKeyEvent(VO_SET_RoutingKeys, routingPresses[--routingPressCount], 1);

    pressedKeys = currentKeys;
  }

  return EOF;
}

/* Voltage: from 0->300V to 255->200V.
 * Presumably this is voltage for dot firmness.
 * Presumably 0 makes dots hardest, 255 makes them softest.
 * We are told 265V is normal operating voltage but we don't know the scale.
 */
static void
brl_firmness (BrailleDisplay *brl, BrailleFirmness setting) {
  unsigned char voltage = 0XFF - (setting * 0XFF / BRL_FIRMNESS_MAXIMUM);
  LogPrint(LOG_DEBUG, "Setting voltage: %02X", voltage);
  io->setDisplayVoltage(voltage);
}
