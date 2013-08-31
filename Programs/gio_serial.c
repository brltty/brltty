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
disconnectSerialResource (GioHandle *handle) {
  serialCloseDevice(handle->serial.device);
  return 1;
}

static ssize_t
writeSerialData (GioHandle *handle, const void *data, size_t size, int timeout) {
  return serialWriteData(handle->serial.device, data, size);
}

static int
awaitSerialInput (GioHandle *handle, int timeout) {
  return serialAwaitInput(handle->serial.device, timeout);
}

static ssize_t
readSerialData (
  GioHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return serialReadData(handle->serial.device, buffer, size,
                        initialTimeout, subsequentTimeout);
}

static int
reconfigureSerialResource (GioHandle *handle, const SerialParameters *parameters) {
  return serialSetParameters(handle->serial.device, parameters);
}

const GioMethods gioSerialMethods = {
  .disconnectResource = disconnectSerialResource,

  .writeData = writeSerialData,
  .awaitInput = awaitSerialInput,
  .readData = readSerialData,

  .reconfigureResource = reconfigureSerialResource
};
