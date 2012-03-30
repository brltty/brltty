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

#include "serial_internal.h"

BEGIN_SERIAL_BAUD_TABLE
  {110, SERIAL_SPEED_110},
  {150, SERIAL_SPEED_150},
  {300, SERIAL_SPEED_300},
  {600, SERIAL_SPEED_600},
  {1200, SERIAL_SPEED_1200},
  {2400, SERIAL_SPEED_2400},
  {4800, SERIAL_SPEED_4800},
  {9600, SERIAL_SPEED_9600},
  {19200, SERIAL_SPEED_19200},
  {38400, SERIAL_SPEED_38400},
  {57600, SERIAL_SPEED_57600},
  {115200, SERIAL_SPEED_115200},
END_SERIAL_BAUD_TABLE

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
  attributes->speed = SERIAL_SPEED_9600;
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

