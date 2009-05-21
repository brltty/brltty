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

static const char productPrefix[] = "PBC";
static const unsigned char productPrefixLength = sizeof(productPrefix) - 1;

static int routingCommand;
static int rewriteRequired;
static unsigned char textCells[80];
static unsigned char statusCells[2];
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

static void
setCellCounts (BrailleDisplay *brl, int size) {
  brl->statusColumns = ARRAY_COUNT(statusCells);
  brl->statusRows = 1;
  brl->textColumns = size - brl->statusColumns;
  brl->textRows = 1;
}

static int
getCellCounts (BrailleDisplay *brl, char *product) {
  unsigned int length = strlen(product);

  {
    static const unsigned char indexes[] = {3, 42, 0};
    const unsigned char *index = indexes;

    while (*index) {
      if (*index < length) {
        unsigned char size = product[*index];
        static const unsigned char sizes[] = {22, 29, 42, 82};

        if (memchr(sizes, size, sizeof(sizes))) {
          setCellCounts(brl, size);
          return 1;
        }
      }

      index += 1;
    }
  }

  {
    static const char delimiters[] = " ";
    char *word;

    if ((word = strtok(product, delimiters))) {
      if (strncmp(word, productPrefix, productPrefixLength) == 0) {
        if ((word = strtok(NULL, delimiters))) {
          int size;

          if (!(*word && isInteger(&size, word))) size = 0;
          while (strtok(NULL, delimiters));

          if ((size > ARRAY_COUNT(statusCells)) &&
              (size <= (ARRAY_COUNT(statusCells) + ARRAY_COUNT(textCells)))) {
            setCellCounts(brl, size);
            return 1;
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

      logInputPacket(packet->bytes, offset);
      return length;
    }
  }
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *buffer, int count) {
  logOutputPacket(buffer, count);
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
  unsigned char buffer[1 + count];
  unsigned char *byte = buffer;

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
      routingCommand = BRL_BLK_ROUTE;
      rewriteRequired = 1;
      memset(textCells, 0, sizeof(textCells));
      memset(statusCells, 0, sizeof(statusCells));

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
interpretNavigationKey (unsigned char key) {
#define CMD(keys,cmd) case (keys): return (cmd)
#define BLK(keys,blk) case (keys): routingCommand = (blk); return BRL_CMD_NOOP
  switch (key) {
    CMD(0X15, BRL_CMD_FWINLT); /* left */
    CMD(0X4D, BRL_CMD_FWINRT); /* right */
    CMD(0X3D, BRL_CMD_LNUP); /* up */
    CMD(0X54, BRL_CMD_LNDN); /* down */

    CMD(0X16, BRL_CMD_TOP_LEFT); /* home */
    CMD(0X1C, BRL_CMD_BOT_LEFT); /* enter */
    CMD(0X36, BRL_CMD_RETURN); /* end */
    CMD(0X2C, BRL_CMD_CSRTRK); /* escape */

    CMD(0X27, BRL_CMD_ATTRUP); /* left-control + left */
    CMD(0X28, BRL_CMD_ATTRDN); /* left-control + right */
    CMD(0X21, BRL_CMD_PRDIFLN); /* left-control + up */
    CMD(0X22, BRL_CMD_NXDIFLN); /* left-control + down */

    CMD(0X3F, BRL_CMD_FREEZE); /* left-control + enter */
    CMD(0X2F, BRL_CMD_PREFMENU); /* left-control + end */
    CMD(0X56, BRL_CMD_INFO); /* left-control + escape */

    CMD(0X1F, BRL_CMD_DISPMD); /* left-shift + left */
    CMD(0X20, BRL_CMD_SIXDOTS); /* left-shift + right */
    CMD(0X5B, BRL_CMD_CSRJMP_VERT); /* left-shift + down */

    CMD(0X17, BRL_CMD_PRPROMPT); /* left-shift + home */
    CMD(0X3A, BRL_CMD_NXPROMPT); /* left-shift + enter */
    CMD(0X3B, BRL_CMD_PRPGRPH); /* left-shift + end */
    CMD(0X18, BRL_CMD_NXPGRPH); /* left-shift + escape */

    BLK(0X37, BRL_BLK_SETLEFT); /* right-shift + left */
    CMD(0X33, BRL_CMD_PASTE); /* right-shift + right */
    BLK(0X38, BRL_BLK_DESCCHAR); /* right-shift + down */

    BLK(0X2A, BRL_BLK_CUTBEGIN); /* right-shift + home */
    BLK(0X31, BRL_BLK_CUTAPPEND); /* right-shift + enter */
    BLK(0X32, BRL_BLK_CUTLINE); /* right-shift + end */
    BLK(0X30, BRL_BLK_CUTRECT); /* right-shift + escape */

    default:
      break;
  }
#undef BLK
#undef CMD

  return EOF;
}

static int
interpretSimulationKey (unsigned char key) {
  switch (key) {
    default:
      break;
  }

  return interpretNavigationKey(key);
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  InputPacket packet;
  int length;

  while ((length = readPacket(brl, &packet))) {
    int routing = routingCommand;

    routingCommand = BRL_BLK_ROUTE;

    switch (packet.data.type) {
      case IPT_KEY_NAVIGATION: {
        int command = interpretNavigationKey(packet.data.fields.key.value);
        if (command != EOF) return command;
        break;
      }

      case IPT_KEY_SIMULATION: {
        int command = interpretSimulationKey(packet.data.fields.key.value);
        if (command != EOF) return command;
        break;
      }

      case IPT_KEY_ROUTING: {
        unsigned char key = packet.data.fields.key.value;

        switch (key) {
          case 81: /* status 1 */
            return BRL_CMD_HELP;
          case 82: /* status 2 */
            return BRL_CMD_LEARN;

          default:
            if ((key > 0) && (key <= brl->textColumns)) return routing + key - 1;
            break;
        }
        break;
      }
    }

    LogBytes(LOG_WARNING, "Unexpected Packet", packet.bytes, length);
  }

  if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;
  return EOF;
}
