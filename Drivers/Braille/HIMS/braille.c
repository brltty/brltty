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

#include "brl_driver.h"

typedef enum {
  /* braille dot keys */
  KEY_DOT1  = 0X0001,
  KEY_DOT2  = 0X0002,
  KEY_DOT3  = 0X0004,
  KEY_DOT4  = 0X0008,
  KEY_DOT5  = 0X0010,
  KEY_DOT6  = 0X0020,
  KEY_DOT7  = 0X0040,
  KEY_DOT8  = 0X0080,
  KEY_SPACE = 0X0100,

  /* Braille Sense keys */
  KEY_BS_F1 = 0X0200,
  KEY_BS_F2 = 0X0400,
  KEY_BS_F3 = 0X0800,
  KEY_BS_F4 = 0X1000,
  KEY_BS_SL = 0X2000,
  KEY_BS_SR = 0X4000,

  /* Sync Braille keys */
  KEY_SB_LU = 0X1000,
  KEY_SB_RU = 0X2000,
  KEY_SB_RD = 0X4000,
  KEY_SB_LD = 0X8000,
} BrailleKeys;

typedef struct {
  int (*openPort) (const char *device);
  int (*configurePort) (void);
  void (*closePort) ();
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, int length, int wait);
  int (*writeBytes) (const unsigned char *buffer, int length);
} InputOutputOperations;
static const InputOutputOperations *io;

static const int logInputPackets = 0;
static const int logOutputPackets = 0;
static int charactersPerSecond;

typedef enum {
  IPT_CURSOR  = 0X00,
  IPT_KEYS    = 0X01,
  IPT_CELLS   = 0X02
} InputPacketType;

typedef union {
  unsigned char bytes[10];

  struct {
    unsigned char start;
    unsigned char type;
    unsigned char count;
    unsigned char data;
    unsigned char reserved[4];
    unsigned char checksum;
    unsigned char end;
  } PACKED data;
} PACKED InputPacket;

static int
readByte (unsigned char *byte, int wait) {
  int count = io->readBytes(byte, 1, wait);
  if (count > 0) return 1;

  if (count == 0) errno = EAGAIN;
  return 0;
}

static int
readPacket (BrailleDisplay *brl, InputPacket *packet) {
  const int length = 10;
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

    if (!offset) {
      if (byte != 0XFA) {
        LogBytes(LOG_WARNING, "Ignored Byte", &byte, 1);
        continue;
      }
    }

    packet->bytes[offset++] = byte;
    if (offset == length) {
      if (byte == 0XFB) {
        {
          int checksum = -packet->data.checksum;

          {
            int i;
            for (i=0; i<offset; i+=1) checksum += packet->bytes[i];
          }

          if ((checksum & 0XFF) != packet->data.checksum) {
            LogBytes(LOG_WARNING, "Incorrect Input Checksum", packet->bytes, offset);
          }
        }

        if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", packet->bytes, offset);
        return length;
      }

      {
        const unsigned char *start = memchr(packet->bytes+1, packet->bytes[0], offset-1);
        const unsigned char *end = packet->bytes + offset;
        if (!start) start = end;
        LogBytes(LOG_WARNING, "Discarded Bytes", packet->bytes, start-packet->bytes);
        memmove(packet->bytes, start, (offset = end - start));
      }
    }
  }
}

static int
writePacket (
  BrailleDisplay *brl,
  unsigned char type, unsigned char mode,
  const unsigned char *data1, size_t length1,
  const unsigned char *data2, size_t length2
) {
  unsigned char packet[2 + 1 + 1 + 2 + length1 + 1 + 1 + 2 + length2 + 1 + 4 + 1 + 2];
  unsigned char *byte = packet;
  unsigned char *checksum;

  /* DS */
  *byte++ = type;
  *byte++ = type;

  /* M */
  *byte++ = mode;

  /* DS1 */
  *byte++ = 0XF0;

  /* Cnt1 */
  *byte++ = (length1 >> 0) & 0XFF;
  *byte++ = (length1 >> 8) & 0XFF;

  /* D1 */
  memcpy(byte, data1, length1);
  byte += length1;

  /* DE1 */
  *byte++ = 0XF1;

  /* DS2 */
  *byte++ = 0XF2;

  /* Cnt2 */
  *byte++ = (length2 >> 0) & 0XFF;
  *byte++ = (length2 >> 8) & 0XFF;

  /* D2 */
  memcpy(byte, data2, length2);
  byte += length2;

  /* DE2 */
  *byte++ = 0XF3;

  /* Reserved */
  {
    int count = 4;
    while (count--) *byte++ = 0;
  }

  /* Chk */
  *(checksum = byte++) = 0;

  /* DE */
  *byte++ = 0XFD;
  *byte++ = 0XFD;

  {
    unsigned char sum = 0;
    const unsigned char *ptr = packet;
    while (ptr != byte) sum += *ptr++;
    *checksum = sum;
  }

  {
    int size = byte - packet;
    if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", packet, size);
    if (io->writeBytes(packet, size) == -1) return 0;
    brl->writeDelay += (size * 1000 / charactersPerSecond) + 1;
  }

  return 1;
}

typedef struct {
  const char *modelName;
  unsigned int helpPage;
  int (*getCellCount) (BrailleDisplay *brl, unsigned int *count);
  int (*interpretKeys) (BrailleKeys keys);
} ProtocolOperations;
static const ProtocolOperations *protocol;

static int routingCommand;

static int
getBrailleSenseCellCount (BrailleDisplay *brl, unsigned int *count) {
  *count = 32;
  return 1;
}

static int
interpretBrailleSenseKeys (BrailleKeys keys) {
  {
    int command = BRL_BLK_PASSDOTS;
    BrailleKeys originalKeys = keys;

#define KEY(name,bit) if (keys & KEY_##name) { command |= (bit); keys &= ~KEY_##name; }
    KEY(DOT1, BRL_DOT1);
    KEY(DOT2, BRL_DOT2);
    KEY(DOT3, BRL_DOT3);
    KEY(DOT4, BRL_DOT4);
    KEY(DOT5, BRL_DOT5);
    KEY(DOT6, BRL_DOT6);
    KEY(DOT7, BRL_DOT7);
    KEY(DOT8, BRL_DOT8);
    if (keys == originalKeys) keys &= ~KEY_SPACE;

    if (keys != originalKeys) {
      KEY(BS_F2, BRL_FLG_CHAR_UPPER);
      KEY(BS_F3, BRL_FLG_CHAR_CONTROL);

      if (!keys) return command;
      keys = originalKeys;
    }
#undef KEY
  }

#define CMD(keys,cmd) case (keys): return (cmd)
#define BLK(keys,blk) case (keys): routingCommand = (blk); return BRL_CMD_NOOP
#define KEY(keys,key) CMD((keys), BRL_BLK_PASSKEY + (key))
#define TGL(keys,cmd) \
  CMD((keys) | KEY_DOT7, (cmd) | BRL_FLG_TOGGLE_OFF); \
  CMD((keys) | KEY_DOT8, (cmd) | BRL_FLG_TOGGLE_ON); \
  CMD((keys), (cmd))
  switch (keys) {
    CMD(KEY_BS_F4, BRL_CMD_HOME);
    CMD(KEY_BS_F1 | KEY_BS_F4, BRL_CMD_BACK);
    CMD(KEY_BS_F2 | KEY_BS_F3, BRL_CMD_CSRJMP_VERT);

    CMD(KEY_BS_SL, BRL_CMD_FWINLT);
    CMD(KEY_BS_SR, BRL_CMD_FWINRT);
    CMD(KEY_BS_SL | KEY_BS_SR, BRL_CMD_LNBEG);

    CMD(KEY_BS_F2, BRL_CMD_LNUP);
    CMD(KEY_BS_F3, BRL_CMD_LNDN);

    CMD(KEY_BS_F1 | KEY_BS_F2, BRL_CMD_FWINLTSKIP);
    CMD(KEY_BS_F3 | KEY_BS_F4, BRL_CMD_FWINRTSKIP);

    CMD(KEY_BS_F1 | KEY_BS_SL, BRL_CMD_PRPROMPT);
    CMD(KEY_BS_F1 | KEY_BS_SR, BRL_CMD_NXPROMPT);

    CMD(KEY_BS_F2 | KEY_BS_SL, BRL_CMD_PRDIFLN);
    CMD(KEY_BS_F2 | KEY_BS_SR, BRL_CMD_NXDIFLN);

    CMD(KEY_BS_F3 | KEY_BS_SL, BRL_CMD_ATTRUP);
    CMD(KEY_BS_F3 | KEY_BS_SR, BRL_CMD_ATTRDN);

    CMD(KEY_BS_F4 | KEY_BS_SL, BRL_CMD_PRPGRPH);
    CMD(KEY_BS_F4 | KEY_BS_SR, BRL_CMD_NXPGRPH);

    CMD(KEY_BS_F1 | KEY_BS_F2 | KEY_BS_SL, BRL_CMD_TOP_LEFT);
    CMD(KEY_BS_F1 | KEY_BS_F2 | KEY_BS_SR, BRL_CMD_BOT_LEFT);

    CMD(KEY_BS_F3 | KEY_BS_F4 | KEY_BS_SL, BRL_CMD_CHRLT);
    CMD(KEY_BS_F3 | KEY_BS_F4 | KEY_BS_SR, BRL_CMD_CHRRT);

    BLK(KEY_BS_F1 | KEY_BS_F3 | KEY_BS_F4, BRL_BLK_CUTBEGIN);
    BLK(KEY_BS_F2 | KEY_BS_F3 | KEY_BS_F4, BRL_BLK_CUTAPPEND);
    BLK(KEY_BS_F1 | KEY_BS_F2 | KEY_BS_F3, BRL_BLK_CUTLINE);
    BLK(KEY_BS_F1 | KEY_BS_F2 | KEY_BS_F4, BRL_BLK_CUTRECT);
    CMD(KEY_BS_F1 | KEY_BS_F2 | KEY_BS_F3 | KEY_BS_F4, BRL_CMD_PASTE);

    BLK(KEY_BS_F1 | KEY_BS_F3, BRL_BLK_SETLEFT);
    BLK(KEY_BS_F2 | KEY_BS_F4, BRL_BLK_DESCCHAR);

    TGL(KEY_SPACE | KEY_DOT1, BRL_CMD_DISPMD);
    TGL(KEY_SPACE | KEY_DOT1 | KEY_DOT2, BRL_CMD_SKPBLNKWINS);
    TGL(KEY_SPACE | KEY_DOT1 | KEY_DOT4, BRL_CMD_CSRVIS);
    CMD(KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT4, BRL_CMD_FREEZE);
    CMD(KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT5, BRL_CMD_HELP);
    TGL(KEY_SPACE | KEY_DOT2 | KEY_DOT4, BRL_CMD_SKPIDLNS);
    CMD(KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3, BRL_CMD_LEARN);
    CMD(KEY_SPACE | KEY_DOT1 | KEY_DOT3 | KEY_DOT4, BRL_CMD_PREFMENU);
    CMD(KEY_SPACE | KEY_DOT1 | KEY_DOT3 | KEY_DOT4 | KEY_DOT7, BRL_CMD_PREFLOAD);
    CMD(KEY_SPACE | KEY_DOT1 | KEY_DOT3 | KEY_DOT4 | KEY_DOT8, BRL_CMD_PREFSAVE);
    TGL(KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT4, BRL_CMD_INFO);
    TGL(KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT4 | KEY_DOT5, BRL_CMD_CSRTRK);
    TGL(KEY_SPACE | KEY_DOT1 | KEY_DOT3 | KEY_DOT6, BRL_CMD_ATTRVIS);
    BLK(KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT6, BRL_BLK_SWITCHVT);
    CMD(KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT6 | KEY_DOT7, BRL_CMD_SWITCHVT_PREV);
    CMD(KEY_SPACE | KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT6 | KEY_DOT8, BRL_CMD_SWITCHVT_NEXT);
    CMD(KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT5, BRL_CMD_SIXDOTS | BRL_FLG_TOGGLE_ON);
    CMD(KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT6, BRL_CMD_SIXDOTS | BRL_FLG_TOGGLE_OFF);

    KEY(KEY_SPACE | KEY_DOT7, BRL_KEY_BACKSPACE);
    KEY(KEY_SPACE | KEY_DOT8, BRL_KEY_ENTER);
    KEY(KEY_SPACE | KEY_DOT7 | KEY_DOT8, BRL_KEY_TAB);

    KEY(KEY_SPACE | KEY_DOT2 | KEY_DOT3, BRL_KEY_CURSOR_LEFT);
    KEY(KEY_SPACE | KEY_DOT5 | KEY_DOT6, BRL_KEY_CURSOR_RIGHT);
    KEY(KEY_SPACE | KEY_DOT2 | KEY_DOT5, BRL_KEY_CURSOR_UP);
    KEY(KEY_SPACE | KEY_DOT3 | KEY_DOT6, BRL_KEY_CURSOR_DOWN);

    KEY(KEY_SPACE | KEY_DOT5, BRL_KEY_PAGE_UP);
    KEY(KEY_SPACE | KEY_DOT6, BRL_KEY_PAGE_DOWN);
    KEY(KEY_SPACE | KEY_DOT2, BRL_KEY_HOME);
    KEY(KEY_SPACE | KEY_DOT3, BRL_KEY_END);

    KEY(KEY_SPACE | KEY_DOT2 | KEY_DOT6, BRL_KEY_ESCAPE);
    KEY(KEY_SPACE | KEY_DOT3 | KEY_DOT5, BRL_KEY_INSERT);
    KEY(KEY_SPACE | KEY_DOT2 | KEY_DOT5 | KEY_DOT6, BRL_KEY_DELETE);
    BLK(KEY_SPACE | KEY_DOT2 | KEY_DOT3 | KEY_DOT5 | KEY_DOT6, BRL_BLK_PASSKEY + BRL_KEY_FUNCTION);

    default:
      break;
  }
#undef TGL
#undef KEY
#undef BLK
#undef CMD

  return EOF;
}

static const ProtocolOperations brailleSenseOperations = {
  "Braille Sense",
  0, getBrailleSenseCellCount, interpretBrailleSenseKeys
};

static int
getSyncBrailleCellCount (BrailleDisplay *brl, unsigned int *count) {
  static const unsigned char data[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  if (writePacket(brl, 0XFB, 0X01, data, sizeof(data), NULL, 0)) {
    InputPacket packet;

    while (io->awaitInput(1000)) {
      if (readPacket(brl, &packet)) {
        if (packet.data.type == IPT_CELLS) {
          *count = packet.data.data;
          return 1;
        }
      }
    }
  }

  return 0;
}

static int
interpretSyncBrailleKeys (BrailleKeys keys) {
#define CMD(keys,cmd) case (keys): return (cmd)
  switch (keys) {
    CMD(KEY_SB_LU, BRL_CMD_LNUP);
    CMD(KEY_SB_LD, BRL_CMD_LNDN);
    CMD(KEY_SB_RU, BRL_CMD_FWINLT);
    CMD(KEY_SB_RD, BRL_CMD_FWINRT);
    CMD(KEY_SB_RU | KEY_SB_RD, BRL_CMD_RETURN);

    CMD(KEY_SB_LU | KEY_SB_LD, BRL_CMD_LNBEG);
    CMD(KEY_SB_LU | KEY_SB_LD | KEY_SB_RU, BRL_CMD_TOP_LEFT);
    CMD(KEY_SB_LU | KEY_SB_LD | KEY_SB_RD, BRL_CMD_BOT_LEFT);

    CMD(KEY_SB_LU | KEY_SB_RU, BRL_CMD_CSRTRK);
    CMD(KEY_SB_LU | KEY_SB_RD, BRL_CMD_SIXDOTS);
    CMD(KEY_SB_LD | KEY_SB_RU, BRL_CMD_FREEZE);
    CMD(KEY_SB_LD | KEY_SB_RD, BRL_CMD_DISPMD);

    CMD(KEY_SB_LU | KEY_SB_RU | KEY_SB_RD, BRL_CMD_INFO);
    CMD(KEY_SB_LD | KEY_SB_RU | KEY_SB_RD, BRL_CMD_PREFMENU);
    CMD(KEY_SB_LU | KEY_SB_LD | KEY_SB_RU | KEY_SB_RD, BRL_CMD_HELP);

    default:
      break;
  }
#undef CMD

  return EOF;
}

static const ProtocolOperations syncBrailleOperations = {
  "SyncBraille",
  1, getSyncBrailleCellCount, interpretSyncBrailleKeys
};

/* Serial IO */
#include "io_serial.h"

static SerialDevice *serialDevice = NULL;
#define SERIAL_BAUD 115200

static int
openSerialPort (const char *device) {
  if (!(serialDevice = serialOpenDevice(device))) return 0;
  protocol = &brailleSenseOperations;
  return 1;
}

static int
configureSerialPort (void) {
  if (!serialRestartDevice(serialDevice, SERIAL_BAUD)) return 0;
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

static const InputOutputOperations serialOperations = {
  openSerialPort, configureSerialPort, closeSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialBytes
};

#ifdef ENABLE_USB_SUPPORT
/* USB IO */
#include "io_usb.h"

static UsbChannel *usbChannel = NULL;
static const UsbSerialOperations *usbSerial = NULL;

static int
openUsbPort (const char *device) {
  static const UsbChannelDefinition definitions[] = {
    { /* Braille Sense */
      .vendor=0X045E, .product=0X930A,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1,
      .data=&brailleSenseOperations
    }
    ,
    { /* Sync Braille */
      .vendor=0X0403, .product=0X6001,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .data=&syncBrailleOperations
    }
    ,
    { .vendor=0 }
  };

  if ((usbChannel = usbFindChannel(definitions, (void *)device))) {
    usbSerial = usbGetSerialOperations(usbChannel->device);
    protocol = usbChannel->definition.data;
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

static const InputOutputOperations usbOperations = {
  openUsbPort, configureUsbPort, closeUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes
};
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
/* Bluetooth IO */
#include "io_bluetooth.h"
#include "io_misc.h"

static int bluetoothConnection = -1;

static int
openBluetoothPort (const char *device) {
  if ((bluetoothConnection = btOpenConnection(device, 4, 0)) == -1) return 0;
  protocol = &brailleSenseOperations;
  return 1;
}

static int
configureBluetoothPort (void) {
  return 1;
}

static void
closeBluetoothPort (void) {
  if (bluetoothConnection != -1) {
    close(bluetoothConnection);
    bluetoothConnection = -1;
  }
}

static int
awaitBluetoothInput (int milliseconds) {
  return awaitInput(bluetoothConnection, milliseconds);
}

static int
readBluetoothBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  size_t offset = 0;
  return readChunk(bluetoothConnection,
                   buffer, &offset, length,
                   (wait? timeout: 0), timeout);
}

static int
writeBluetoothBytes (const unsigned char *buffer, int length) {
  int count = writeData(bluetoothConnection, buffer, length);
  if (count != length) {
    if (count == -1) {
      LogError("Bluetooth write");
    } else {
      LogPrint(LOG_WARNING, "Trunccated bluetooth write: %d < %d", count, length);
    }
  }
  return count;
}

static const InputOutputOperations bluetoothOperations = {
  openBluetoothPort, configureBluetoothPort, closeBluetoothPort,
  awaitBluetoothInput, readBluetoothBytes, writeBluetoothBytes
};
#endif /* ENABLE_BLUETOOTH_SUPPORT */

static TranslationTable outputTable;
static unsigned char previousCells[40];

static int
writeCells (BrailleDisplay *brl) {
  size_t count = brl->textColumns * brl->textRows;
  unsigned char cells[count];

  {
    int i;
    for (i=0; i<count; i+=1) cells[i] = outputTable[previousCells[i]];
  }

  return writePacket(brl, 0XFC, 0X01, cells, count, NULL, 0);
}

static int
clearCells (BrailleDisplay *brl) {
  memset(previousCells, 0, brl->textColumns*brl->textRows);
  return writeCells(brl);
}

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

#ifdef ENABLE_BLUETOOTH_SUPPORT
  if (isBluetoothDevice(&device)) {
    io = &bluetoothOperations;
  } else
#endif /* ENABLE_BLUETOOTH_SUPPORT */

  {
    unsupportedDevice(device);
    return 0;
  }

  if (io->openPort(device)) {
    if (io->configurePort()) {
      charactersPerSecond = SERIAL_BAUD / 10;
      LogPrint(LOG_INFO, "detected: %s", protocol->modelName);

      if (protocol->getCellCount(brl, &brl->textColumns) ||
          protocol->getCellCount(brl, &brl->textColumns)) {
        brl->textRows = 1;
        brl->helpPage = protocol->helpPage;

        routingCommand = BRL_BLK_ROUTE;

        if (clearCells(brl)) return 1;
      }
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
  size_t count = brl->textColumns * brl->textRows;

  if (memcmp(brl->buffer, previousCells, count) != 0) {
    memcpy(previousCells, brl->buffer, count);
    if (!writeCells(brl)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  InputPacket packet;
  int routing;

  if (!readPacket(brl, &packet)) {
    if (errno == EAGAIN) return EOF;
    return BRL_CMD_RESTARTBRL;
  }

  routing = routingCommand;
  routingCommand = BRL_BLK_ROUTE;

  switch (packet.data.type) {
    case IPT_CURSOR:
      return routing + packet.data.data;

    case IPT_KEYS: {
      BrailleKeys keys = packet.data.reserved[0] | (packet.data.reserved[1] << 8);
      int command = protocol->interpretKeys(keys);

      if (command == EOF) command = BRL_CMD_NOOP;
      return command;
    }
  }

  return EOF;
}
