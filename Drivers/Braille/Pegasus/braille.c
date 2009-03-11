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

#include <string.h>
#include <errno.h>

#include "misc.h"

//#define BRL_STATUS_FIELDS sf...
#define BRL_HAVE_STATUS_CELLS
#include "brl_driver.h"

static const int logInputPackets = 0;
static const int logOutputPackets = 0;

static const char productPrefix[] = "PBC";
static const unsigned char productPrefixLength = sizeof(productPrefix) - 1;

static unsigned char textCells[80];
static unsigned char statusCells[2];
static int rewriteRequired;

static TranslationTable outputTable;

typedef struct {
  int (*identifyModel) (BrailleDisplay *brl);
  int (*writeCells) (BrailleDisplay *brl, const unsigned char *cells, unsigned int count);
} InputOutputMethods;

typedef struct {
  int (*openPort) (const char *device);
  int (*configurePort) (void);
  void (*closePort) ();
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, int length, int wait);
  int (*writeBytes) (const unsigned char *buffer, int length);
  const InputOutputMethods *methods;
} InputOutputOperations;
static const InputOutputOperations *io;

typedef enum {
  IPT_KEY_NAVIGATION = 0X13,
  IPT_KEY_SIMULATION = 0XFE,
  IPT_KEY_ROUTING    = 0XFF
} InputPacketType;

typedef union {
  unsigned char bytes[1];

  char product[44 + 1];

  struct {
    unsigned char type;

    union {
      struct {
        unsigned char type;
        unsigned char value;
        unsigned char release;
      } PACKED key;
    } fields;
  } PACKED data;
} PACKED InputPacket;

static int
setCellCounts (BrailleDisplay *brl, int size) {
  static const unsigned char sizes[] = {22, 29, 42, 82};
  if (!memchr(sizes, size, sizeof(sizes))) return 0;

  brl->statusColumns = ARRAY_COUNT(statusCells);
  brl->statusRows = 1;
  brl->textColumns = size - brl->statusColumns;
  brl->textRows = 1;
  return 1;
}

static int
getCellCounts (BrailleDisplay *brl, char *product) {
  unsigned int length = strlen(product);

  {
    static const unsigned char indexes[] = {3, 42, 0};
    const unsigned char *index = indexes;

    while (*index) {
      if (*index < length)
        if (setCellCounts(brl, product[*index]))
          return 1;

      index += 1;
    }
  }

  {
    static const char delimiters[] = " ";
    char *next = product;
    char *word;

    if ((word = strsep(&next, delimiters))) {
      if (strncmp(word, productPrefix, productPrefixLength) == 0) {
        if ((word = strsep(&next, delimiters))) {
          int size;

          if (*word && isInteger(&size, word)) {
            if (setCellCounts(brl, size)) {
              while (strsep(&next, delimiters));
              return 1;
            }
          }
        }
      }
    }
  }

  return 0;
}

static int
readByte (unsigned char *byte, int wait) {
  int count = io->readBytes(byte, 1, wait);
  if (count > 0) return 1;

  if (count == 0) errno = EAGAIN;
  return 0;
}

static int
readPacket (BrailleDisplay *brl, InputPacket *packet) {
  typedef enum {
    IPG_PRODUCT,
    IPG_KEY,
    IPG_DEFAULT
  } InputPacketGroup;
  InputPacketGroup group = IPG_DEFAULT;

  int length = 1;
  int offset = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;
      if (!readByte(&byte, started)) {
        if (started) LogBytes(LOG_WARNING, "Partial Packet", packet->bytes, offset);
        return 0;
      }
    }

  gotByte:
    if (!offset) {
      switch (byte) {
        case IPT_KEY_NAVIGATION:
        case IPT_KEY_SIMULATION:
        case IPT_KEY_ROUTING:
          group = IPG_KEY;
          length = 4;
          break;

        default:
          if (byte == productPrefix[0]) {
            group = IPG_PRODUCT;
            length = sizeof(packet->product) - 1;
          } else {
            LogBytes(LOG_WARNING, "Ignored Byte", &byte, 1);
            continue;
          }
          break;
      }
    } else {
      int unexpected = 0;

      switch (group) {
        case IPG_PRODUCT:
          if (offset < productPrefixLength) {
            if (byte != productPrefix[offset]) unexpected = 1;
          } else if (byte == '@') {
            length = offset + 1;
          }
          break;

        case IPG_KEY:
          if (offset == 1) {
            if (byte != packet->bytes[0]) unexpected = 1;
          } else if (offset == 3) {
            if (byte != 0X19) unexpected = 1;
          }
          break;

        default:
          break;
      }

      if (unexpected) {
        LogBytes(LOG_WARNING, "Short Packet", packet->bytes, offset);
        group = IPG_DEFAULT;
        offset = 0;
        length = 1;
        goto gotByte;
      }
    }

    packet->bytes[offset++] = byte;
    if (offset == length) {
      if (group == IPG_PRODUCT) {
        packet->bytes[length] = 0;
      }

      if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", packet->bytes, offset);
      return length;
    }
  }
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *buffer, int count) {
  if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", buffer, count);
  if (io->writeBytes(buffer, count) != -1) return 1;
  return 0;
}

static int
writeCells (BrailleDisplay *brl) {
  unsigned int textCount = brl->textColumns;
  unsigned int statusCount = brl->statusColumns;
  unsigned char cells[textCount + statusCount];
  unsigned char *cell = cells;

  while (textCount) *cell++ = outputTable[textCells[--textCount]];
  while (statusCount) *cell++ = outputTable[statusCells[--statusCount]];

  return io->methods->writeCells(brl, cells, cell-cells);
}

static void
updateCells (unsigned char *target, const unsigned char *source, int count) {
  if (memcmp(target, source, count) != 0) {
    memcpy(target, source, count);
    rewriteRequired = 1;
  }
}

/* Serial IO */
#include "io_serial.h"

static SerialDevice *serialDevice = NULL;
#define SERIAL_BAUD 9600

static int
openSerialPort (const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, SERIAL_BAUD)) {
      return 1;
    }

    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }

  return 0;
}

static int
configureSerialPort (void) {
  if (!serialSetFlowControl(serialDevice, SERIAL_FLOW_HARDWARE)) return 0;
  return 1;
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
readSerialBytes (unsigned char *buffer, int count, int wait) {
  const int timeout = 100;
  return serialReadData(serialDevice, buffer, count,
                        (wait? timeout: 0), timeout);
}

static int
writeSerialBytes (const unsigned char *buffer, int length) {
  return serialWriteData(serialDevice, buffer, length);
}

static int
identifySerialModel (BrailleDisplay *brl) {
  static const unsigned char request[] = {0X40, 0X50, 0X53};

  if (writeBytes(brl, request, sizeof(request))) {
    while (io->awaitInput(1000)) {
      InputPacket response;

      while (readPacket(brl, &response)) {
        if (response.data.type == productPrefix[0]) {
          if (getCellCounts(brl, response.product)) {
            brl->helpPage = 0;
            return 1;
          }
        }
      }
    }
  }

  return 0;
}

static int
writeSerialCells (BrailleDisplay *brl, const unsigned char *cells, unsigned int count) {
  static const unsigned char header[] = {0X40, 0X50, 0X4F};
  static const unsigned char trailer[] = {0X18, 0X20, 0X20};

  unsigned char buffer[sizeof(header) + count + sizeof(trailer)];
  unsigned char *byte = buffer;

  memcpy(byte, header, sizeof(header));
  byte += sizeof(header);

  memcpy(byte, cells, count);
  byte += count;

  memcpy(byte, trailer, sizeof(trailer));
  byte += sizeof(trailer);

  return writeBytes(brl, buffer, byte-buffer);
}

static const InputOutputMethods serialMethods = {
  identifySerialModel, writeSerialCells
};

static const InputOutputOperations serialOperations = {
  openSerialPort, configureSerialPort, closeSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialBytes,
  &serialMethods
};

#ifdef ENABLE_USB_SUPPORT
/* USB IO */
#include "io_usb.h"

static UsbChannel *usbChannel = NULL;
static const UsbSerialOperations *usbSerial = NULL;

static int
openUsbPort (const char *device) {
  static const UsbChannelDefinition definitions[] = {
    {
      .vendor=0X4242, .product=0X0001,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2
    }
    ,
    { .vendor=0 }
  };

  if ((usbChannel = usbFindChannel(definitions, (void *)device))) {
    usbSerial = usbGetSerialOperations(usbChannel->device);
    return 1;
  }
  return 0;
}

static int
configureUsbPort (void) {
  return 1;
}

static void
closeUsbPort (void) {
  if (usbChannel) {
    usbCloseChannel(usbChannel);
    usbSerial = NULL;
    usbChannel = NULL;
  }
}

static int
awaitUsbInput (int milliseconds) {
  return usbAwaitInput(usbChannel->device,
                       usbChannel->definition.inputEndpoint,
                       milliseconds);
}

static int
readUsbBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  int count = usbReapInput(usbChannel->device,
                           usbChannel->definition.inputEndpoint,
                           buffer, length,
                           (wait? timeout: 0), timeout);
  if (count != -1) return count;
  if (errno == EAGAIN) return 0;
  return -1;
}

static int
writeUsbBytes (const unsigned char *buffer, int length) {
  return usbWriteEndpoint(usbChannel->device,
                          usbChannel->definition.outputEndpoint,
                          buffer, length, 1000);
}

static int
identifyUsbModel (BrailleDisplay *brl) {
  int ok = 0;
  char *product;

  if ((product = usbGetProduct(usbChannel->device, 1000))) {
    if (getCellCounts(brl, product)) {
      ok = 1;
    }

    free(product);
  }

  return ok;
}

static int
writeUsbCells (BrailleDisplay *brl, const unsigned char *cells, unsigned int count) {
  unsigned char buffer[2 + count];
  unsigned char *byte = buffer;

  *byte++ = 0X00;
  *byte++ = 0X43;

  memcpy(byte, cells, count);
  byte += count;

  return writeBytes(brl, buffer, byte-buffer);
}

static const InputOutputMethods usbMethods = {
  identifyUsbModel, writeUsbCells
};

static const InputOutputOperations usbOperations = {
  openUsbPort, configureUsbPort, closeUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes,
  &usbMethods
};
#endif /* ENABLE_USB_SUPPORT */

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
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

  {
    unsupportedDevice(device);
    return 0;
  }

  if (io->openPort(device)) {
    if (io->methods->identifyModel(brl)) {
      memset(textCells, 0, sizeof(textCells));
      memset(statusCells, 0, sizeof(statusCells));
      rewriteRequired = 1;

      if (io->configurePort()) return 1;
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
  updateCells(textCells, brl->buffer, brl->textColumns);

  if (rewriteRequired) {
    if (!writeCells(brl)) return 0;
    rewriteRequired = 0;
  }

  return 1;
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *cells) {
  updateCells(statusCells, cells, brl->statusColumns);
  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  InputPacket packet;
  int length;

  while ((length = readPacket(brl, &packet))) {
    switch (packet.data.type) {
      case IPT_KEY_NAVIGATION:
        switch (packet.data.fields.key.value) {
          /* navigation */
          case 0X3D: /* up */
            return BRL_CMD_LNUP;
          case 0X54: /* down */
            return BRL_CMD_LNDN;
          case 0X15: /* left */
            return BRL_CMD_FWINLT;
          case 0X4D: /* right */
            return BRL_CMD_FWINRT;

          case 0X16: /* home */
            return BRL_CMD_TOP_LEFT;
          case 0X36: /* end */
            return BRL_CMD_BOT_LEFT;
          case 0X1C: /* enter */
            break;
          case 0X2C: /* escape */
            return BRL_CMD_HOME;

          /* extended navigation */
          case 0X5B: /* left-shift + down */
            break;
          case 0X1F: /* left-shift + left */
            return BRL_CMD_HWINLT;
          case 0X20: /* left-shift + right */
            return BRL_CMD_HWINRT;

          case 0X17: /* left-shift + home */
            return BRL_CMD_LNBEG;
          case 0X18: /* left-shift + escape */
            break;

          /* switches */
          case 0X38: /* right-shift + down */
            return BRL_CMD_ATTRVIS;
          case 0X37: /* right-shift + left */
            break;
          case 0X33: /* right-shift + right */
            return BRL_CMD_TUNES;

          case 0X2A: /* right-shift + home */
            return BRL_CMD_SIXDOTS;
          case 0X32: /* right-shift + end */
            return BRL_CMD_SKPIDLNS;
          case 0X31: /* right-shift + enter */
            return BRL_CMD_CAPBLINK;
          case 0X30: /* right-shift + escape */
            return BRL_CMD_CSRVIS;

          /* toggles */
          case 0X21: /* left-control + up */
            return BRL_CMD_DISPMD;
          case 0X22: /* left-control + down */
            break;
          case 0X27: /* left-control + left */
            return BRL_CMD_INFO;
          case 0X28: /* left-control + right */
            return BRL_CMD_PREFMENU;

          case 0X2F: /* left-control + end */
            break;
          case 0X56: /* left-control + escape */
            return BRL_CMD_CSRSIZE;

          default:
            break;
        }
        break;

      case 0XFE: /* simulation key */
        switch (packet.data.fields.key.value) {

          default:
            break;
        }
        break;

      case IPT_KEY_ROUTING: { /* routing key */
        unsigned char value = packet.data.fields.key.value;

        if (value < brl->textColumns) {
          return BRL_BLK_ROUTE + value;
        }

        break;
      }
    }

    LogBytes(LOG_WARNING, "Unexpected Packet", packet.bytes, length);
  }

  if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;
  return EOF;
}
