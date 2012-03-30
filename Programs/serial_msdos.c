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
#include "system.h"

#define SERIAL_DIVISOR_BASE 115200
#define SERIAL_DIVISOR(baud) (SERIAL_DIVISOR_BASE / (baud))
#define SERIAL_SPEED(baud, bios) (SerialSpeed){SERIAL_DIVISOR((baud)), (bios)}

BEGIN_SERIAL_BAUD_TABLE
  {   110, SERIAL_SPEED(   110,  0)},
  {   150, SERIAL_SPEED(   150,  1)},
  {   300, SERIAL_SPEED(   300,  2)},
  {   600, SERIAL_SPEED(   600,  3)},
  {  1200, SERIAL_SPEED(  1200,  4)},
  {  2400, SERIAL_SPEED(  2400,  5)},
  {  4800, SERIAL_SPEED(  4800,  6)},
  {  9600, SERIAL_SPEED(  9600,  7)},
  { 19200, SERIAL_SPEED( 19200,  8)},
  { 38400, SERIAL_SPEED( 38400,  9)},
  { 57600, SERIAL_SPEED( 57600, 10)},
  {115200, SERIAL_SPEED(115200, 11)},
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
  attributes->speed = serialGetBaudEntry(9600)->speed;
  attributes->bios.fields.bps = attributes->speed.biosBPS;
  attributes->bios.fields.bits = 8 - 5;
}

int
serialPutSpeed (SerialDevice *serial, SerialSpeed speed) {
  serial->pendingAttributes.speed = speed;
  serial->pendingAttributes.bios.fields.bps = serial->pendingAttributes.speed.biosBPS;
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

void
serialPutModemState (SerialAttributes *attributes, int enabled) {
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

