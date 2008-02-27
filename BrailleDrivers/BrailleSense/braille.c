/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#include <string.h>
#include <errno.h>

#include "misc.h"

#include "brl_driver.h"

typedef struct {
  int (*openPort) (const char *device);
  int (*configurePort) (void);
  void (*closePort) ();
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, int length, int wait);
  int (*writeBytes) (const unsigned char *buffer, int length);
} InputOutputOperations;

/* Serial IO */
#include "io_serial.h"

static SerialDevice *serialDevice = NULL;

static int
openSerialPort (const char *device) {
  if ((serialDevice = serialOpenDevice(device))) return 1;
  return 0;
}

static int
configureSerialPort (void) {
  if (!serialRestartDevice(serialDevice, 115200)) return 0;
  return 1;
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

static void
closeSerialPort (void) {
  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
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
      .vendor=0X045E, .product=0X00CE,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2
    }
    ,
    { .vendor=0 }
  };

  if ((usbChannel = usbFindChannel(definitions, (void *)device))) {
    usbSerial = usbGetSerialOperations(usbChannel->device);
    usbBeginInput(usbChannel->device,
                  usbChannel->definition.inputEndpoint,
                  8);
    return 1;
  }
  return 0;
}

static int
configureUsbPort (void) {
  return 1;
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

static void
closeUsbPort (void) {
  if (usbChannel) {
    usbCloseChannel(usbChannel);
    usbSerial = NULL;
    usbChannel = NULL;
  }
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
  return (bluetoothConnection = btOpenConnection(device, 1, 0)) != -1;
}

static int
configureBluetoothPort (void) {
  return 1;
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

static void
closeBluetoothPort (void) {
  if (bluetoothConnection != -1) {
    close(bluetoothConnection);
    bluetoothConnection = -1;
  }
}

static const InputOutputOperations bluetoothOperations = {
  openBluetoothPort, configureBluetoothPort, closeBluetoothPort,
  awaitBluetoothInput, readBluetoothBytes, writeBluetoothBytes
};
#endif /* ENABLE_BLUETOOTH_SUPPORT */

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

typedef enum {
  KEY_DOT1  = 0X0001,
  KEY_DOT2  = 0X0002,
  KEY_DOT3  = 0X0004,
  KEY_DOT4  = 0X0008,
  KEY_DOT5  = 0X0010,
  KEY_DOT6  = 0X0020,
  KEY_DOT7  = 0X0040,
  KEY_DOT8  = 0X0080,
  KEY_SPACE = 0X0100,
  KEY_F1    = 0X0200,
  KEY_F2    = 0X0400,
  KEY_F3    = 0X0800,
  KEY_F4    = 0X1000,
  KEY_LEFT  = 0X2000,
  KEY_RIGHT = 0X4000,
} BrailleKeys;

static const int logInputPackets = 0;
static const int logOutputPackets = 0;

static const InputOutputOperations *io = NULL;
static TranslationTable outputTable;
static unsigned char previousCells[40];

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
    io->writeBytes(packet, size);
  }

  return 1;
}

static int
writeCells (BrailleDisplay *brl) {
  size_t count = brl->x * brl->y;
  unsigned char cells[count];

  {
    int i;
    for (i=0; i<count; i+=1) cells[i] = outputTable[previousCells[i]];
  }

  return writePacket(0XFC, 0X01, cells, count, NULL, 0);
}

static int
clearCells (BrailleDisplay *brl) {
  memset(previousCells, 0, brl->x*brl->y);
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
      brl->x = 32;
      brl->y = 1;
      brl->helpPage = 0;

      if (clearCells(brl)) return 1;
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
  size_t count = brl->x * brl->y;

  if (memcmp(brl->buffer, previousCells, count) != 0) {
    memcpy(previousCells, brl->buffer, count);
    if (!writeCells(brl)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  InputPacket packet;

  if (!readPacket(brl, &packet)) {
    if (errno == EAGAIN) return EOF;
    return BRL_CMD_RESTARTBRL;
  }

  switch (packet.data.type) {
    case IPT_CURSOR:
      return BRL_BLK_ROUTE + packet.data.data;

    case IPT_KEYS: {
      BrailleKeys keys = packet.data.reserved[0] | (packet.data.reserved[1] << 8);

      {
        const BrailleKeys dots = KEY_DOT1 | KEY_DOT2 | KEY_DOT3 | KEY_DOT4 | KEY_DOT5 | KEY_DOT6 | KEY_DOT7 | KEY_DOT8;
        if ((keys & dots) && (keys == dots)) {
          int command = BRL_BLK_PASSDOTS;
          if (keys & KEY_DOT1) command |= BRL_DOT1;
          if (keys & KEY_DOT2) command |= BRL_DOT2;
          if (keys & KEY_DOT3) command |= BRL_DOT3;
          if (keys & KEY_DOT4) command |= BRL_DOT4;
          if (keys & KEY_DOT5) command |= BRL_DOT5;
          if (keys & KEY_DOT6) command |= BRL_DOT6;
          if (keys & KEY_DOT7) command |= BRL_DOT7;
          if (keys & KEY_DOT8) command |= BRL_DOT8;
          return command;
        }
      }

      switch (keys) {
        case KEY_SPACE:
          return BRL_BLK_PASSDOTS;

        case KEY_LEFT:
          return BRL_CMD_FWINLT;

        case KEY_RIGHT:
          return BRL_CMD_FWINRT;

        default:
          break;
      }

      return BRL_CMD_NOOP;
    }
  }

  return EOF;
}
