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

typedef struct {
  int (*openPort) (const char *device);
  void (*closePort) ();
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, int length, int wait);
  int (*writeBytes) (const unsigned char *buffer, int length);
} InputOutputOperations;
static const InputOutputOperations *io;

#define SERIAL_BAUD 9600
static const int charactersPerSecond = SERIAL_BAUD / 10;

/* Serial IO */
#include "io_serial.h"

static SerialDevice *serialDevice = NULL;

static int
openSerialPort (const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, SERIAL_BAUD)) return 1;

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
  openSerialPort, closeSerialPort,
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
      .vendor=0X10C4, .product=0XEA60,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=1
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
  openUsbPort, closeUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes
};
#endif /* ENABLE_USB_SUPPORT */

static const int logInputPackets = 1;
static const int logOutputPackets = 0;
static TranslationTable outputTable;
static unsigned char textCells[40];

static int
readByte (unsigned char *byte, int wait) {
  int count = io->readBytes(byte, 1, wait);
  if (count > 0) return 1;

  if (count == 0) errno = EAGAIN;
  return 0;
}

static int
readBytes (unsigned char *buffer, size_t length) {
  size_t count = 0;

  while (count < length) {
    int started = count > 0;

    if (!readByte(&buffer[count], started)) {
      if (started) LogBytes(LOG_WARNING, "Partial Packet", buffer, count);
      return 0;
    }

    count += 1;
  }

  if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", buffer, length);
  return 1;
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *buffer, size_t length) {
  if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", buffer, length);
  if (io->writeBytes(buffer, length) == -1) return 0;
  brl->writeDelay += (length * 1000 / charactersPerSecond) + 1;
  return 1;
}

static int
writeCells (BrailleDisplay *brl) {
  static const unsigned char header[] = {
    0XFF, 0XFF, 0X73, 0X65, 0X69, 0X6B, 0X61, 0X00
  };

  unsigned char packet[sizeof(header) + (brl->textColumns * 2)];
  unsigned char *byte = packet;

  memcpy(byte, header, sizeof(header));
  byte += sizeof(header);

  {
    int i;
    for (i=0; i<brl->textColumns; i+=1) {
      *byte++ = 0;
      *byte++ = outputTable[textCells[i]];
    }
  }

  return writeBytes(brl, packet, byte-packet);
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

  {
    unsupportedDevice(device);
    return 0;
  }

  if (io->openPort(device)) {
    int probeCount = 0;

    do {
      static const unsigned char request[] = {0XFF, 0XFF, 0X1C};

      if (writeBytes(brl, request, sizeof(request))) {
        if (io->awaitInput(1000)) {
          unsigned char response[12];

          if (readBytes(response, sizeof(response))) {
            brl->textColumns = sizeof(textCells);
            brl->textRows = 1;
            brl->helpPage = 0;

            memset(textCells, 0XFF, sizeof(textCells));
            return 1;
          }
        }
      }
    } while (++probeCount == 3);

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
  if (memcmp(textCells, brl->buffer, brl->textColumns) != 0) {
    memcpy(textCells, brl->buffer, brl->textColumns);
    if (!writeCells(brl)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return EOF;
}
