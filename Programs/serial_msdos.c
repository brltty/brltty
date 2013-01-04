/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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
#include <sys/stat.h>
#include <fcntl.h>

#include "log.h"
#include "io_misc.h"
#include "system.h"

#include "serial_msdos.h"
#include "serial_internal.h"

#include <dos.h>
#include <dpmi.h>
#include <bios.h>
#include <go32.h>
#include <sys/farptr.h>

#define SERIAL_DIVISOR_BASE 115200
#define SERIAL_DIVISOR(baud) (SERIAL_DIVISOR_BASE / (baud))
#define SERIAL_SPEED(baud, bios) {SERIAL_DIVISOR((baud)), (bios)}
#define SERIAL_BAUD_ENTRY(baud, bios) {(baud), SERIAL_SPEED((baud),  (bios))}

BEGIN_SERIAL_BAUD_TABLE
  SERIAL_BAUD_ENTRY(   110,  0),
  SERIAL_BAUD_ENTRY(   150,  1),
  SERIAL_BAUD_ENTRY(   300,  2),
  SERIAL_BAUD_ENTRY(   600,  3),
  SERIAL_BAUD_ENTRY(  1200,  4),
  SERIAL_BAUD_ENTRY(  2400,  5),
  SERIAL_BAUD_ENTRY(  4800,  6),
  SERIAL_BAUD_ENTRY(  9600,  7),
  SERIAL_BAUD_ENTRY( 19200,  8),
  SERIAL_BAUD_ENTRY( 38400,  9),
  SERIAL_BAUD_ENTRY( 57600, 10),
  SERIAL_BAUD_ENTRY(115200, 11),
END_SERIAL_BAUD_TABLE

static unsigned short
serialPortBase (SerialDevice *serial) {
  return _farpeekw(_dos_ds, (0X0400 + (2 * serial->package.deviceIndex)));
}

static unsigned char
serialReadPort (SerialDevice *serial, unsigned char port) {
  return readPort1(serialPortBase(serial)+port);
}

static void
serialWritePort (SerialDevice *serial, unsigned char port, unsigned char value) {
  writePort1(serialPortBase(serial)+port, value);
}

void
serialPutInitialAttributes (SerialAttributes *attributes) {
}

int
serialPutSpeed (SerialAttributes *attributes, SerialSpeed speed) {
  attributes->speed = speed;
  attributes->bios.fields.bps = attributes->speed.biosBPS;
  return 1;
}

int
serialPutDataBits (SerialAttributes *attributes, unsigned int bits) {
  if ((bits < 5) || (bits > 8)) return 0;
  attributes->bios.fields.bits = bits - 5;
  return 1;
}

int
serialPutStopBits (SerialAttributes *attributes, SerialStopBits bits) {
  if (bits == SERIAL_STOP_1) {
    attributes->bios.fields.stop = 0;
  } else if (bits == 15 || bits == SERIAL_STOP_2) {
    attributes->bios.fields.stop = 1;
  } else {
    return 0;
  }

  return 1;
}

int
serialPutParity (SerialAttributes *attributes, SerialParity parity) {
  switch (parity) {
    case SERIAL_PARITY_NONE:
      attributes->bios.fields.parity = 0;
      break;

    case SERIAL_PARITY_ODD:
      attributes->bios.fields.parity = 1;
      break;

    case SERIAL_PARITY_EVEN:
      attributes->bios.fields.parity = 2;
      break;

    default:
      return 0;
  }

  return 1;
}

SerialFlowControl
serialPutFlowControl (SerialAttributes *attributes, SerialFlowControl flow) {
  return flow;
}

int
serialPutModemState (SerialAttributes *attributes, int enabled) {
  return !enabled;
}

unsigned int
serialGetDataBits (const SerialAttributes *attributes) {
  return attributes->bios.fields.bits + 5;
}

unsigned int
serialGetStopBits (const SerialAttributes *attributes) {
  return attributes->bios.fields.stop? 2: 1;
}

unsigned int
serialGetParityBits (const SerialAttributes *attributes) {
  return attributes->bios.fields.parity? 1: 0;
}

int
serialGetAttributes (SerialDevice *serial, SerialAttributes *attributes) {
  int interruptsWereEnabled = disable();
  unsigned char lcr = serialReadPort(serial, UART_PORT_LCR);
  int divisor;

  serialWritePort(serial, UART_PORT_LCR,
                  lcr | UART_FLAG_LCR_DLAB);
  divisor = (serialReadPort(serial, UART_PORT_DLH) << 8) |
            serialReadPort(serial, UART_PORT_DLL);
  serialWritePort(serial, UART_PORT_LCR, lcr);
  if (interruptsWereEnabled) enable();

  attributes->bios.byte = lcr;
  {
    const SerialBaudEntry *baud = serialGetBaudEntry(SERIAL_DIVISOR_BASE/divisor);
    if (baud) {
      attributes->speed = baud->speed;
    } else {
      memset(&attributes->speed, 0,
             sizeof(attributes->speed));
    }
  }
  attributes->bios.fields.bps = attributes->speed.biosBPS;

  return 1;
}

int
serialPutAttributes (SerialDevice *serial, const SerialAttributes *attributes) {
  {
    if (attributes->speed.biosBPS <= 7) {
      if (bioscom(0, attributes->bios.byte, serial->package.deviceIndex) & 0X0700) {
        logMessage(LOG_ERR, "bioscom failed");
        return 0;
      }
    } else {
      int interruptsWereEnabled = disable();
      SerialBiosConfiguration lcr = attributes->bios;
      lcr.fields.bps = 0;

      serialWritePort(serial, UART_PORT_LCR,
                      lcr.byte | UART_FLAG_LCR_DLAB);
      serialWritePort(serial, UART_PORT_DLL,
                      attributes->speed.divisor & 0XFF);
      serialWritePort(serial, UART_PORT_DLH,
                      attributes->speed.divisor >> 8);
      serialWritePort(serial, UART_PORT_LCR, lcr.byte);

      if (interruptsWereEnabled) enable();
    }
  }

  return 1;
}

int
serialCancelInput (SerialDevice *serial) {
  return 1;
}

int
serialCancelOutput (SerialDevice *serial) {
  return 1;
}

int
serialPollInput (SerialDevice *serial, int timeout) {
  return awaitInput(serial->fileDescriptor, timeout);
}

int
serialDrainOutput (SerialDevice *serial) {
  return 1;
}

ssize_t
serialGetData (
  SerialDevice *serial,
  void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return readData(serial->fileDescriptor, buffer, size, initialTimeout, subsequentTimeout);
}

ssize_t
serialPutData (
  SerialDevice *serial,
  const void *data, size_t size
) {
  return writeData(serial->fileDescriptor, data, size);
}

int
serialGetLines (SerialDevice *serial) {
  serial->linesState = serialReadPort(serial, UART_PORT_MSR) & 0XF0;
  return 1;
}

int
serialPutLines (SerialDevice *serial, SerialLines high, SerialLines low) {
  int interruptsWereEnabled = disable();
  unsigned char oldMCR = serialReadPort(serial, UART_PORT_MCR);

  serialWritePort(serial, UART_PORT_MCR,
                  (oldMCR | high) & ~low);
  if (interruptsWereEnabled) enable();
  return 1;
}

int
serialRegisterWaitLines (SerialDevice *serial, SerialLines lines) {
  return 1;
}

int
serialMonitorWaitLines (SerialDevice *serial) {
  return 0;
}

int
serialConnectDevice (SerialDevice *serial, const char *device) {
  if ((serial->fileDescriptor = open(device, O_RDWR|O_NOCTTY|O_NONBLOCK)) != -1) {
    serial->package.deviceIndex = -1;

    {
      char *truePath;

      if ((truePath = _truename(device, NULL))) {
        char *com;

        if ((com = strstr(truePath, "COM"))) {
          serial->package.deviceIndex = atoi(com+3) - 1;
        }

        free(truePath);
      }
    }

    if (serial->package.deviceIndex >= 0) {
      if (serialPrepareDevice(serial)) {
        logMessage(LOG_DEBUG, "serial device opened: %s: fd=%d",
                   device, serial->fileDescriptor);
        return 1;
      }
    } else {
      logMessage(LOG_ERR, "could not determine serial device number: %s", device);
    }

    close(serial->fileDescriptor);
  } else {
    logMessage(LOG_ERR, "cannot open serial device: %s: %s", device, strerror(errno));
  }

  return 0;
}

void
serialDisconnectDevice (SerialDevice *serial) {
}

int
serialEnsureFileDescriptor (SerialDevice *serial) {
  return 1;
}

void
serialClearError (SerialDevice *serial) {
}

