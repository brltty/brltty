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

#include <errno.h>

#include "io_bluetooth.h"
#include "bluetooth_internal.h"
#include "log.h"

BluetoothConnectionExtension *
bthConnect (uint64_t bda, uint8_t channel) {
  errno = ENOSYS;
  logSystemError("Bluetooth connect");
  return NULL;
}

void
bthDisconnect (BluetoothConnectionExtension *bcx) {
}

int
bthAwaitInput (BluetoothConnection *connection, int milliseconds) {
  errno = ENOSYS;
  logSystemError("Bluetooth wait");
  return 0;
}

ssize_t
bthReadData (
  BluetoothConnection *connection, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  errno = ENOSYS;
  logSystemError("Bluetooth read");
  return -1;
}

ssize_t
bthWriteData (BluetoothConnection *connection, const void *buffer, size_t size) {
  errno = ENOSYS;
  logSystemError("Bluetooth write");
  return -1;
}
