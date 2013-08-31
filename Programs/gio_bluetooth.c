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

#include "io_generic.h"
#include "gio_internal.h"

static int
disconnectBluetoothResource (GioHandle *handle) {
  bthCloseConnection(handle->bluetooth.connection);
  return 1;
}

static char *
getBluetoothResourceName (GioHandle *handle, int timeout) {
  return bthGetNameOfDevice(handle->bluetooth.connection, timeout);
}

static ssize_t
writeBluetoothData (GioHandle *handle, const void *data, size_t size, int timeout) {
  return bthWriteData(handle->bluetooth.connection, data, size);
}

static int
awaitBluetoothInput (GioHandle *handle, int timeout) {
  return bthAwaitInput(handle->bluetooth.connection, timeout);
}

static ssize_t
readBluetoothData (
  GioHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return bthReadData(handle->bluetooth.connection, buffer, size,
                     initialTimeout, subsequentTimeout);
}

const GioMethods gioBluetoothMethods = {
  .disconnectResource = disconnectBluetoothResource,
  .getResourceName = getBluetoothResourceName,

  .writeData = writeBluetoothData,
  .awaitInput = awaitBluetoothInput,
  .readData = readBluetoothData
};
