/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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
#define VO_VERSION "0.21"
#define VO_DATE "January 2005"
#define VO_COPYRIGHT \
"   Copyright (C) 2005 by Stéphane Doyon  <s.doyon@videotron.ca>"

/* Voyager/braille.c - Braille display driver for Tieman Voyager displays.
 *
 * Written by Stéphane Doyon  <s.doyon@videotron.ca>
 *
 * It is being tested on Voyager 44, should also support Voyager 70.
 * It is designed to be compiled in BRLTTY version 3.5.
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
  PARM_INPUTMODE,
  PARM_STATUSCELLS
} DriverParameter;
#define BRLPARMS "inputmode", "statuscells"

#define BRLSTAT ST_VoyagerStyle
#define BRL_HAVE_FIRMNESS
#include "Programs/brl_driver.h"
#include "Programs/tbl.h"

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


#include "Programs/serial.h"

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

/*LogBytes("Output Packet", buffer, size);*/
  return serialWriteData(serialDevice, buffer, size) != -1;
}

static int
readSerialPacket (unsigned char *buffer, int size) {
  size_t offset = 0;
  int escape = 0;
  int length = -1;

  while ((offset < 1) || (offset < length)) {
    if (offset == size) {
      LogBytes("Large Packet", buffer, offset);
      offset = 0;
    }

    if (!serialReadChunk(serialDevice, buffer, &offset, 1, 0, 100)) {
      LogBytes("Partial Packet", buffer, offset);
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
          LogBytes("Discarded Byte", buffer, offset);
          offset = 0;
        }
        continue;
      }
      escape = 0;

      if (offset > 1) {
        LogBytes("Truncated Packet", buffer, offset-1);
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
          LogBytes("Unsupported Packet", buffer, offset);
          offset = 0;
          continue;
      }
    }
  }

/*LogBytes("Input Packet", buffer, offset);*/
  return offset;
}

static int
nextSerialPacket (unsigned char code, unsigned char *buffer, int size) {
  int length;
  while ((length = readSerialPacket(buffer, size))) {
    if (buffer[0] == code) return length;
    LogBytes("Ignored Packet", buffer, length);
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
#include "Programs/usb.h"

/* Workarounds for control transfer flakiness (at least in this demo model) */
#define USB_RETRIES 6

static UsbChannel *usb = NULL;

static int
writeUsbData (uint8_t request, uint16_t value, uint16_t index,
	      const unsigned char *buffer, uint16_t size) {
  int retry = 0;
  while (1) {
    int ret = usbControlWrite(usb->device, USB_RECIPIENT_ENDPOINT, USB_TYPE_VENDOR,
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
    int ret = usbControlRead(usb->device, USB_RECIPIENT_ENDPOINT, USB_TYPE_VENDOR,
                             request, value, index, buffer, size, 100);
    if ((ret != -1) || (errno != EPIPE) || (retry == USB_RETRIES)) return ret;
    LogPrint(LOG_WARNING, "Voyager request 0X%X retry #%d.", request, ++retry);
  }
}

static int
openUsbPort (char **parameters, const char *device) {
  static const UsbChannelDefinition definitions[] = {
    {0X0798, 0X0001, 1, 0, 0, 1, 0, 0},
    {0}
  };

  if (!(usb = usbFindChannel(definitions, (void *)device))) return 0;

  /* start the input packet monitor */
  {
    int retry = 0;
    while (1) {
      int ret = usbBeginInput(usb->device, usb->definition.inputEndpoint, 8);
      if ((ret != 0) || (errno != EPIPE) || (retry == USB_RETRIES)) break;
      LogPrint(LOG_WARNING, "begin input retry #%d.", ++retry);
    }
  }

  return 1;
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
static unsigned int inputMode;
static TranslationTable outputTable;
static unsigned char *currentCells = NULL; /* buffer to prepare new pattern */
static unsigned char *previousCells = NULL; /* previous pattern displayed */

#define MAXIMUM_CELLS 70 /* arbitrary max for allocations */
static unsigned char totalCells;
static unsigned char textOffset;
static unsigned char textCells;
static unsigned char statusOffset;
static unsigned char statusCells;
#define IS_TEXT_RANGE(key1,key2) (((key1) >= textOffset) && ((key2) < (textOffset + textCells)))
#define IS_TEXT_KEY(key) IS_TEXT_RANGE((key), (key))
#define IS_STATUS_KEY(key) (((key) >= statusOffset) && ((key) < (statusOffset + statusCells)))

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "Tieman Voyager User-space Driver: version " VO_VERSION " (" VO_DATE ")");
  LogPrint(LOG_INFO, VO_COPYRIGHT);
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  if (isSerialDevice(&device)) {
    io = &serialOperations;

#ifdef ENABLE_USB_SUPPORT
  } else if (isUsbDevice(&device)) {
    io = &usbOperations;
#endif /* ENABLE_USB_SUPPORT */

  } else {
    unsupportedDevice(device);
    return 0;
  }

  if (io->openPort(parameters, device)) {
    /* find out how big the display is */
    totalCells = 0;
    {
      unsigned char count;
      if (io->getCellCount(&count)) {
        switch (count) {
          case 48:
            totalCells = 44;
            brl->helpPage = 0;
            break;

          case 72:
            totalCells = 70;
            brl->helpPage = 1;
            break;

          default:
            LogPrint(LOG_ERR, "Unsupported Voyager cell count: %u", count);
            break;
        }
      }
    }

    if (totalCells) {
      LogPrint(LOG_INFO, "Voyager Cell Count: %u", totalCells);

      /* position the text and status cells */
      textCells = totalCells;
      textOffset = statusOffset = 0;
      {
        int cells = 3;
        const char *word = parameters[PARM_STATUSCELLS];

        {
          int maximum = textCells / 2;
          int minimum = -maximum;
          int value = cells;
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

      /* log information about the display */
      io->logSerialNumber();
      io->logHardwareVersion();
      io->logFirmwareVersion();

      /* currentCells holds the status cells and the text cells.
       * We export directly to BRLTTY only the text cells.
       */
      brl->x = textCells;		/* initialize size of display */
      brl->y = 1;		/* always 1 */

      if ((currentCells = malloc(totalCells))) {
        if ((previousCells = malloc(totalCells))) {
          /* Force rewrite of display */
          memset(currentCells, 0, totalCells); /* no dots */
          memset(previousCells, 0XFF, totalCells); /* all dots */

          if (io->setDisplayState(1)) {
            io->soundBeep(200);

            {
              static const DotsTable dots
                = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
              makeOutputTable(&dots, &outputTable);
            }

            inputMode = 0;
            if (*parameters[PARM_INPUTMODE])
              validateYesNo(&inputMode, "Allow braille input",
                            parameters[PARM_INPUTMODE]);

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
brl_close (BrailleDisplay *brl) {
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

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *cells) {
  memcpy(currentCells+statusOffset, cells, statusCells);
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  unsigned char buffer[totalCells];

  memcpy(currentCells+textOffset, brl->buffer, textCells);

  /* If content hasn't changed, do nothing. */
  if (memcmp(previousCells, currentCells, totalCells) == 0) return;

  /* remember current content */
  memcpy(previousCells, currentCells, totalCells);

  /* translate to voyager dot pattern coding */
  {
    int i;
    for (i=0; i<totalCells; i++)
      buffer[i] = outputTable[currentCells[i]];
  }

  /* The firmware supports multiples of 8 cells, so there are extra cells
   * in the firmware's imagination that don't actually exist physically.
   */
  switch (totalCells) {
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
}


/* Names and codes for display keys */

/* The top round keys behind the routing keys, numbered assuming they
 * are to be used for braille input.
 */
#define DOT1 0X01
#define DOT2 0X02
#define DOT3 0X04
#define DOT4 0X08
#define DOT5 0X10
#define DOT6 0X20
#define DOT7 0X40
#define DOT8 0X80
#define DOT_KEYS (DOT1 | DOT2 | DOT3 | DOT4 | DOT5 | DOT6 | DOT7 | DOT8)

/* The front keys. Codes are shifted by 8 bits so they can be combined
 * with the codes for the top keys.
 */
#define K_A    0X0100 /* Leftmost */
#define K_B    0X0200 /* Second from the left */
#define K_RL   0X0400 /* The round key to the left of the central pad */
#define K_UP   0X0800 /* Up position of central pad */
#define K_DOWN 0X1000 /* Down position of central pad */
#define K_RR   0X2000 /* The round key to the right of the central pad */
#define K_C    0X4000 /* Second from the right */
#define K_D    0X8000 /* Rightmost */
#define FRONT_KEYS (K_A | K_B | K_RL | K_UP | K_DOWN | K_RR | K_C | K_D)
#define SPACE_BAR (K_B | K_C)

/* OK what follows is pretty hairy. I got tired of individually maintaining
 * the sources and help files so here's my first attempt at "automatic"
 * generation of help files. This is my first shot at it, so be kind with
 * me.
 */

/* These macros include an ordering hint for the help file and the help
 * text. GENHLP is not defined during compilation, so at compilation the
 * macros are expanded in a way that just drops the help-related
 * information.
 */
#define KEY(keys, command) case keys: cmd = command; break;
#ifdef GENHLP
/* To generate the help files we do gcc -DGENHLP -E (and heavily post-process
 * the result). So these macros expand to something that is easily
 * searched/grepped for and "easily" post-processed.
 */
#define HLP(where, keys, description) <HLP> where: keys : description </HLP>
#else /* GENHLP */
#define HLP(where, keys, description)
#endif /* GENHLP */

#define HKEY(where, keys, command, description) \
  HLP(where, #keys, description) \
  KEY(keys, command)

#define PHKEY(where, prefix, keys, command, description) \
  HLP(where, prefix "+" #keys, description) \
  KEY(keys, command)

#define CKEY(where, dots, command, description) \
  HLP(where, "Chord-" #dots, description) \
  KEY(dots, command)

#define HKEY2(where, keys1, keys2, command1, command2, description) \
  HLP(where, #keys1 "/" #keys2, description) \
  KEY(keys1, command1); \
  KEY(keys2, command2)

#define PHKEY2(where, prefix, keys1, keys2, command1, command2, description) \
  HLP(where, prefix "+ " #keys1 "/" #keys2, description) \
  KEY(keys1, command1); \
  KEY(keys2, command2)

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  /* Structure to remember which keys are pressed */
  typedef struct {
    unsigned int control; /* Front and dot keys */
    unsigned char routing[MAXIMUM_CELLS];
  } Keys;

  /* Static variables to remember the state between calls. */
  /* Remember which keys are pressed.
   * Updated immediately whenever a key is pressed.
   * Cleared after analysis whenever a key is released.
   */
  static Keys activeKeys;
  static Keys pressedKeys;
  /* For a key binding that triggers two commands */
  static int pendingCommand;

  /* Ordered list of pressed routing keys by number */
  unsigned char routingKeys[MAXIMUM_CELLS];
  /* number of entries in routingKeys */
  int routingCount = 0;

  /* recognized command */
  int cmd = BRL_CMD_NOOP;
  int keyPressed = 0;

  /* read buffer: packet[0] for DOT keys, packet[1] for keys A B C D UP DOWN
   * RL RR, packet[2]-packet[7] list pressed routing keys by number, maximum
   * 6 keys, list ends with 0.
   * All 0s is sent when all keys released.
   */
  unsigned char packet[8];

  if (firstRead) {
    /* initialize state */
    firstRead = 0;
    memset(&activeKeys, 0, sizeof(activeKeys));
    memset(&pressedKeys, 0, sizeof(pressedKeys));
    pendingCommand = EOF;
  }

  if (pendingCommand != EOF) {
    cmd = pendingCommand;
    pendingCommand = EOF;
    return cmd;
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
      return EOF;
    }
  }

  /* one or more keys were pressed or released */
  {
    Keys keys;
    int i;
    memset(&keys, 0, sizeof(keys));

    /* We combine dot and front key info in keystate */
    keys.control = (packet[1] << 8) | packet[0];
    if (keys.control & ~pressedKeys.control) keyPressed = 1;
    
    for (i=2; i<8; i++) {
      unsigned char key = packet[i];
      if (!key) break;

      if ((key < 1) || (key > totalCells)) {
        LogPrint(LOG_NOTICE, "Invalid routing key number: %u", key);
        continue;
      }
      key--;

      keys.routing[key] = 1;
      if (!pressedKeys.routing[key]) keyPressed = 1;
    }

    pressedKeys = keys;
    if (keyPressed) activeKeys = keys;
  }

  {
    int key;
    for (key=0; key<totalCells; key++)
      if (activeKeys.routing[key])
        routingKeys[routingCount++] = key;
  }

  if (routingCount == 0) {
    /* No routing keys */

    if (!(activeKeys.control & ~FRONT_KEYS)) {
      /* Just front keys */

      if (context == BRL_CTX_PREFS) {
	switch (activeKeys.control) {
          HKEY2(891, K_DOWN, K_UP,
                BRL_CMD_MENU_PREV_SETTING, BRL_CMD_MENU_NEXT_SETTING,
                "Select previous/next setting");
          HKEY(892, K_RL|K_RR, BRL_CMD_PREFMENU, "Exit menu");
          HKEY(892, K_RL|K_UP, BRL_CMD_PREFLOAD, "Discard changes");
          HKEY(892, K_RL|K_DOWN, BRL_CMD_PREFSAVE, "Save changes and exit menu");
	}
      }

      if (cmd == BRL_CMD_NOOP) {
	switch (activeKeys.control) {
	  HKEY2(101, K_A, K_D,
                BRL_CMD_FWINLT, BRL_CMD_FWINRT,
                "Go backward/forward one window");
	  HKEY2(501, K_RL|K_A, K_RL|K_D,
                BRL_CMD_LNBEG, BRL_CMD_LNEND,
                "Go to beginning/end of line");
	  HKEY2(501, K_RR|K_A, K_RR|K_D,
                BRL_CMD_CHRLT, BRL_CMD_CHRRT,
                "Go left/right one character");
	  HKEY2(501, K_UP|K_A, K_UP|K_D,
                BRL_CMD_FWINLTSKIP, BRL_CMD_FWINRTSKIP,
                "Go to previous/next non-blank window");
	  HKEY2(501, K_DOWN|K_A, K_DOWN|K_D,
                BRL_CMD_PRSEARCH, BRL_CMD_NXSEARCH,
                "Search screen backward/forward for cut text");

	  HKEY2(101, K_B, K_C,
                BRL_CMD_LNUP, BRL_CMD_LNDN,
                "Go up/down one line");
	  HKEY2(101, K_A|K_B, K_A|K_C,
                BRL_CMD_TOP_LEFT, BRL_CMD_BOT_LEFT,
                "Go to top-left/bottom-left corner");
	  HKEY2(101, K_D|K_B, K_D|K_C,
                BRL_CMD_TOP, BRL_CMD_BOT,
                "Go to top/bottom line");
	  HKEY2(501, K_RL|K_B, K_RL|K_C,
                BRL_CMD_ATTRUP, BRL_CMD_ATTRDN,
		"Go to previous/next line with different highlighting");
	  HKEY2(501, K_RR|K_B, K_RR|K_C,
                BRL_CMD_PRDIFLN, BRL_CMD_NXDIFLN,
                "Go to previous/next line with different content");
	  HKEY2(501, K_UP|K_B, K_UP|K_C,
                BRL_CMD_PRPGRPH, BRL_CMD_NXPGRPH,
                "Go to previous/next paragraph (blank line separation)");
	  HKEY2(501, K_DOWN|K_B, K_DOWN|K_C,
                BRL_CMD_PRPROMPT, BRL_CMD_NXPROMPT,
                "Go to previous/next prompt (same prompt as current line)");

	  HKEY(101, K_RL, BRL_CMD_BACK, 
               "Go back (undo unexpected cursor tracking motion)");
	  HKEY(101, K_RR, BRL_CMD_HOME, "Go to cursor");
	  HKEY(101, K_RL|K_RR, BRL_CMD_CSRTRK, "Cursor tracking (toggle)");

	  HKEY2(101, K_UP, K_DOWN,
                BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP,
                BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN,
                "Move cursor up/down (arrow keys)");
	  HKEY(210, K_RL|K_UP, BRL_CMD_DISPMD, "Show attributes (toggle)");
	  HKEY(210, K_RL|K_DOWN, BRL_CMD_SIXDOTS, "Six dots (toggle)");
	  HKEY(210, K_RR|K_UP, BRL_CMD_AUTOREPEAT, "Autorepeat (toggle)");
	  HKEY(210, K_RR|K_DOWN, BRL_CMD_AUTOSPEAK, "Autospeak (toggle)");

	  HKEY(602, K_B|K_C, BRL_BLK_PASSDOTS+0, "Space bar")
	  HKEY(302, K_A|K_D, BRL_CMD_CSRJMP_VERT,
	       "Route cursor to current line");
	}
      }
    } else if ((!inputMode || (activeKeys.control & SPACE_BAR)) &&
               !(activeKeys.control & FRONT_KEYS & ~SPACE_BAR)) {
      /* Dots combined with B or C or both but no other front keys.
       * This is a chorded character typed in braille.
       */
      switch (activeKeys.control & DOT_KEYS) {
        HLP(601, "Chord-1478", "Input mode (toggle)")
        case DOT1|DOT4|DOT7|DOT8:
          if (!keyPressed) cmd = BRL_CMD_NOOP | ((inputMode = !inputMode)? BRL_FLG_TOGGLE_ON: BRL_FLG_TOGGLE_OFF);
          break;

	CKEY(210, DOT1, BRL_CMD_ATTRVIS, "Attribute underlining (toggle)");
	CKEY(610, DOT1|DOT2, BRL_BLK_PASSKEY + BRL_KEY_BACKSPACE, "Backspace key");
	CKEY(210, DOT1|DOT4, BRL_CMD_CSRVIS, "Cursor visibility (toggle)");
	CKEY(610, DOT1|DOT4|DOT5, BRL_BLK_PASSKEY + BRL_KEY_DELETE, "Delete key");
	CKEY(610, DOT1|DOT5, BRL_BLK_PASSKEY + BRL_KEY_ESCAPE, "Escape key");
	CKEY(210, DOT1|DOT2|DOT4, BRL_CMD_FREEZE, "Freeze screen (toggle)");
	CKEY(201, DOT1|DOT2|DOT5, BRL_CMD_HELP, "Help screen (toggle)");
	CKEY(610, DOT2|DOT4, BRL_BLK_PASSKEY + BRL_KEY_INSERT, "Insert key");
	CKEY(201, DOT1|DOT2|DOT3, BRL_CMD_LEARN, "Learn mode (toggle)");
	case DOT1|DOT2|DOT3|DOT4|DOT5|DOT6|DOT7|DOT8:
	CKEY(205, DOT1|DOT3|DOT4, BRL_CMD_PREFMENU, "Preferences menu (toggle)");
	CKEY(408, DOT1|DOT2|DOT3|DOT4, BRL_CMD_PASTE, "Paste cut text");
	CKEY(206, DOT1|DOT2|DOT3|DOT5, BRL_CMD_PREFLOAD, "Reload preferences from disk");
	CKEY(201, DOT2|DOT3|DOT4, BRL_CMD_INFO, "Status line (toggle)");
	CKEY(610, DOT2|DOT3|DOT4|DOT5, BRL_BLK_PASSKEY + BRL_KEY_TAB, "Tab key");
	CKEY(206, DOT2|DOT4|DOT5|DOT6, BRL_CMD_PREFSAVE, "Write preferences to disk");
	CKEY(610, DOT4|DOT6, BRL_BLK_PASSKEY + BRL_KEY_ENTER, "Enter key");
	CKEY(610, DOT2, BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP, "Page up");
	CKEY(610, DOT5, BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN, "Page down");
	CKEY(610, DOT3, BRL_BLK_PASSKEY+BRL_KEY_HOME, "Home key");
	CKEY(610, DOT6, BRL_BLK_PASSKEY+BRL_KEY_END, "End key");
	CKEY(610, DOT7, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT, "Left arrow");
	CKEY(610, DOT8, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, "Right arrow");
      }
    } else if (!(activeKeys.control & ~DOT_KEYS)) {
      /* Just dot keys */
      /* This is a character typed in braille */
      cmd = BRL_BLK_PASSDOTS;
      if (activeKeys.control & DOT1) cmd |= BRL_DOT1;
      if (activeKeys.control & DOT2) cmd |= BRL_DOT2;
      if (activeKeys.control & DOT3) cmd |= BRL_DOT3;
      if (activeKeys.control & DOT4) cmd |= BRL_DOT4;
      if (activeKeys.control & DOT5) cmd |= BRL_DOT5;
      if (activeKeys.control & DOT6) cmd |= BRL_DOT6;
      if (activeKeys.control & DOT7) cmd |= BRL_DOT7;
      if (activeKeys.control & DOT8) cmd |= BRL_DOT8;
    }
  } else if (!activeKeys.control) {
    /* Just routing keys */
    if (routingCount == 1) {
      if (IS_TEXT_KEY(routingKeys[0])) {
        HLP(301,"CRt#", "Route cursor to cell")
        cmd = BRL_BLK_ROUTE + routingKeys[0] - textOffset;
      } else {
        int key = statusOffset? totalCells - 1 - routingKeys[0]:
                                routingKeys[0] - statusOffset;
        switch (key) {
          case 0:
            HLP(881, "CRs1", "Help screen (toggle)")
            cmd = BRL_CMD_HELP;
            break;

          case 1:
            HLP(882, "CRs2", "Preferences menu (toggle)")
            cmd = BRL_CMD_PREFMENU;
            break;

          case 2:
            HLP(883, "CRs3", "Learn mode (toggle)")
            cmd = BRL_CMD_LEARN;
            break;

          case 3:
            HLP(884, "CRs4", "Route cursor to current line")
            cmd = BRL_CMD_CSRJMP_VERT;
            break;
        }
      }
    } else if ((routingCount == 2) &&
               IS_TEXT_RANGE(routingKeys[0], routingKeys[1])) {
      HLP(405, "CRtx+CRty", "Cut text from x through y")
      cmd = BRL_BLK_CUTBEGIN + routingKeys[0] - textOffset;
      pendingCommand = BRL_BLK_CUTLINE + routingKeys[1] - textOffset;
    }
  } else if (activeKeys.control & (K_UP|K_RL|K_RR)) {
    /* Some routing keys combined with UP RL or RR (actually any key
     * combination that has at least one of those).
     * Treated special because we use absolute routing key numbers
     * (counting the status cell keys).
     */
    if (routingCount == 1) {
      switch (activeKeys.control) {
        case K_UP:
          HLP(692, "UP+ CRa<CELLS-1>/CRa<CELLS>",
              "Switch to previous/next virtual console")
          if (routingKeys[0] == totalCells-1) {
            cmd = BRL_CMD_SWITCHVT_NEXT;
          } else if (routingKeys[0] == totalCells-2) {
            cmd = BRL_CMD_SWITCHVT_PREV;
          } else {
            HLP(691, "UP+CRa#", "Switch to virtual console #")
            cmd = BRL_BLK_SWITCHVT + routingKeys[0];
          }
          break;

        PHKEY(501,"CRa#", K_RL,
              BRL_BLK_SETMARK + routingKeys[0],
              "Remember current position as mark #");
        PHKEY(501,"CRa#", K_RR,
              BRL_BLK_GOTOMARK + routingKeys[0],
              "Go to mark #");
      }
    }
  } else if ((routingCount == 1) && IS_TEXT_KEY(routingKeys[0])) {
    /* One text routing key with some other keys */
    switch (activeKeys.control) {
      PHKEY(501, "CRt#", K_DOWN,
            BRL_BLK_SETLEFT + routingKeys[0] - textOffset,
            "Go right # cells");
      PHKEY(401, "CRt#", K_A,
            BRL_BLK_CUTBEGIN + routingKeys[0] - textOffset,
            "Mark beginning of region to cut");
      PHKEY(401, "CRt#", K_A|K_B,
            BRL_BLK_CUTAPPEND + routingKeys[0] - textOffset,
            "Mark beginning of cut region for append");
      PHKEY(401, "CRt#", K_D,
            BRL_BLK_CUTRECT + routingKeys[0] - textOffset,
            "Mark bottom-right of rectangular region and cut");
      PHKEY(401, "CRt#", K_D|K_C,
            BRL_BLK_CUTLINE + routingKeys[0] - textOffset,
            "Mark end of linear region and cut");
      PHKEY2(501, "CRt#", K_B, K_C,
             BRL_BLK_PRINDENT + routingKeys[0] - textOffset,
             BRL_BLK_NXINDENT + routingKeys[0] - textOffset,
             "Go to previous/next line indented no more than #");
    }
  }

  if (keyPressed) {
    /* key was pressed, start the autorepeat delay */
    cmd |= BRL_FLG_REPEAT_DELAY;
  } else {
    /* key was released, clear state */
    memset(&activeKeys, 0, sizeof(activeKeys));
  }

  return cmd;
}

/* Voltage: from 0->300V to 255->200V.
 * Presumably this is voltage for dot firmness.
 * Presumably 0 makes dots hardest, 255 makes them softest.
 * We are told 265V is normal operating voltage but we don't know the scale.
 */
static void
brl_firmness (BrailleDisplay *brl, BrailleFirmness setting) {
  unsigned char voltage = 0XFF - (setting * 0XFF / BF_MAXIMUM);
  LogPrint(LOG_DEBUG, "Setting voltage: %02X", voltage);
  io->setDisplayVoltage(voltage);
}
