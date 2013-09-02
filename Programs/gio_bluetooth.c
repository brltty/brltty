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

#include "log.h"
#include "io_generic.h"
#include "gio_internal.h"
#include "io_bluetooth.h"

struct GioHandleStruct {
  BluetoothConnection *connection;
};

static int
disconnectBluetoothResource (GioHandle *handle) {
  bthCloseConnection(handle->connection);
  free(handle);
  return 1;
}

static char *
getBluetoothResourceName (GioHandle *handle, int timeout) {
  return bthGetNameOfDevice(handle->connection, timeout);
}

static ssize_t
writeBluetoothData (GioHandle *handle, const void *data, size_t size, int timeout) {
  return bthWriteData(handle->connection, data, size);
}

static int
awaitBluetoothInput (GioHandle *handle, int timeout) {
  return bthAwaitInput(handle->connection, timeout);
}

static ssize_t
readBluetoothData (
  GioHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return bthReadData(handle->connection, buffer, size,
                     initialTimeout, subsequentTimeout);
}

static const GioMethods gioBluetoothMethods = {
  .disconnectResource = disconnectBluetoothResource,

  .getResourceName = getBluetoothResourceName,

  .writeData = writeBluetoothData,
  .awaitInput = awaitBluetoothInput,
  .readData = readBluetoothData
};

static int
isBluetoothSupported (const GioDescriptor *descriptor) {
  return descriptor->bluetooth.channelNumber != 0;
}

static int
testBluetoothIdentifier (const char **identifier) {
  return isBluetoothDevice(identifier);
}

static const GioOptions *
getBluetoothOptions (const GioDescriptor *descriptor) {
  return &descriptor->bluetooth.options;
}

static const GioMethods *
getBluetoothMethods (void) {
  return &gioBluetoothMethods;
}

static GioHandle *
connectBluetoothResource (
  const char *identifier,
  const GioDescriptor *descriptor
) {
  GioHandle *handle = malloc(sizeof(*handle));

  if (handle) {
    memset(handle, 0, sizeof(*handle));

    if ((handle->connection = bthOpenConnection(identifier, descriptor->bluetooth.channelNumber))) {
      return handle;
    }

    free(handle);
  } else {
    logMallocError();
  }

  return NULL;
}

const GioClass gioBluetoothClass = {
  .isSupported = isBluetoothSupported,
  .testIdentifier = testBluetoothIdentifier,

  .getOptions = getBluetoothOptions,
  .getMethods = getBluetoothMethods,

  .connectResource = connectBluetoothResource
};
