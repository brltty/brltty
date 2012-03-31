/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

#include <dos.h>
#include <dpmi.h>
#include <bios.h>
#include <go32.h>
#include <sys/farptr.h>

#include "serial_internal.h"
#include "io_misc.h"
#include "system.h"

#define SERIAL_DIVISOR_BASE 115200
#define SERIAL_DIVISOR(baud) (SERIAL_DIVISOR_BASE / (baud))
#define SERIAL_SPEED(baud, bios) (SerialSpeed){SERIAL_DIVISOR((baud)), (bios)}
#define SERIAL_BAUD_ENTRY(baud,, bios) {(baud), SERIAL_SPEED((baud),  (bios))}

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

#define SERIAL_PORT_RBR 0 /* receive buffered register */
#define SERIAL_PORT_THR 0 /* transmit holding register */
#define SERIAL_PORT_DLL 0 /* divisor latch low */
#define SERIAL_PORT_IER 1 /* interrupt enable register */
#define SERIAL_PORT_DLH 1 /* divisor latch high */
#define SERIAL_PORT_IIR 2 /* interrupt id register */
#define SERIAL_PORT_LCR 3 /* line control register */
#define SERIAL_PORT_MCR 4 /* modem control register */
#define SERIAL_PORT_MSR 6 /* modem status register */

#define SERIAL_FLAG_LCR_DLAB 0X80 /* divisor latch access bit */

static unsigned short
serialPortBase (SerialDevice *serial) {
  return _farpeekw(_dos_ds, 0X0400 + 2*serial->port);
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
  unsigned char lcr = serialReadPort(serial, SERIAL_PORT_LCR);
  int divisor;

  serialWritePort(serial, SERIAL_PORT_LCR,
                  lcr | SERIAL_FLAG_LCR_DLAB);
  divisor = (serialReadPort(serial, SERIAL_PORT_DLH) << 8) |
            serialReadPort(serial, SERIAL_PORT_DLL);
  serialWritePort(serial, SERIAL_PORT_LCR, lcr);
  if (interruptsWereEnabled) enable();

  attributes->bios.byte = lcr;
  {
    const SerialBaudEntry *baud = serialGetBaudEntry(SERIAL_DIVISOR_BASE/divisor);
    if (baud) {
      attributes->speed = baud->speed;
    } else {
      memset(attributes.speed, 0,
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
      if (bioscom(0, attributes->bios.byte, serial->port) & 0X0700) {
        logMessage(LOG_ERR, "bioscom failed");
        return 0;
      }
    } else {
      int interruptsWereEnabled = disable();
      SerialBiosConfiguration lcr = attributes->bios;
      lcr.fields.bps = 0;

      serialWritePort(serial, SERIAL_PORT_LCR,
                      lcr.byte | SERIAL_FLAG_LCR_DLAB);
      serialWritePort(serial, SERIAL_PORT_DLL,
                      attributes->speed.divisor & 0XFF);
      serialWritePort(serial, SERIAL_PORT_DLH,
                      attributes->speed.divisor >> 8);
      serialWritePort(serial, SERIAL_PORT_LCR, lcr.byte);
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

int
serialGetChunk (
  SerialDevice *serial,
  void *buffer, size_t *offset, size_t count,
  int initialTimeout, int subsequentTimeout
) {
  return readChunk(serial->fileDescriptor, buffer, offset, count, initialTimeout, subsequentTimeout);
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
serialGetLines (SerialDevice *serial, SerialLines *lines) {
  *lines = serialReadPort(serial, SERIAL_PORT_MSR) & 0XF0;
  return 1;
}

int
serialPutLines (SerialDevice *serial, SerialLines high, SerialLines low) {
  int interruptsWereEnabled = disable();
  unsigned char oldMCR = serialReadPort(serial, SERIAL_PORT_MCR);

  serialWritePort(serial, SERIAL_PORT_MCR,
                  (oldMCR | high) & ~low);
  if (interruptsWereEnabled) enable();
  return 1;
}

