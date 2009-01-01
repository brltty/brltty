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

#warning I/O port access not available on this platform

int
enablePorts (int errorLevel, unsigned short int base, unsigned short int count) {
  LogPrint(errorLevel, "I/O ports not supported.");
  return 0;
}

int
disablePorts (unsigned short int base, unsigned short int count) {
  return 0;
}

unsigned char
readPort1 (unsigned short int port) {
  return 0;
}

void
writePort1 (unsigned short int port, unsigned char value) {
}
