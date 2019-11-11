/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "strfmt.h"
#include "parse.h"
#include "device.h"
#include "io_generic.h"
#include "gio_internal.h"
#include "io_bluetooth.h"
#include "brl.h"

struct GioHandleStruct {
  BluetoothConnection *connection;
};

static int
disconnectBluetoothResource (GioHandle *handle) {
  bthCloseConnection(handle->connection);
  free(handle);
  return 1;
}

static const char *
makeBluetoothResourceIdentifier (GioHandle *handle, char *buffer, size_t size) {
  size_t length;
  STR_BEGIN(buffer, size);
  STR_PRINTF("%s%c", BLUETOOTH_DEVICE_QUALIFIER, PARAMETER_QUALIFIER_CHARACTER);

  {
    uint64_t address = bthGetAddress(handle->connection);
    STR_PRINTF("address%c", PARAMETER_ASSIGNMENT_CHARACTER);
    STR_FORMAT(bthFormatAddress, address);
    STR_PRINTF("%c", DEVICE_PARAMETER_SEPARATOR);
  }

  {
    uint8_t channel = bthGetChannel(handle->connection);

    if (channel) {
      STR_PRINTF(
        "channel%c%u%c",
        PARAMETER_ASSIGNMENT_CHARACTER, channel, DEVICE_PARAMETER_SEPARATOR
      );
    }
  }

  length = STR_LENGTH;
  STR_END;

  {
    char *last = &buffer[length] - 1;
    if (*last == DEVICE_PARAMETER_SEPARATOR) *last = 0;
  }

  return buffer;
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

static int
monitorBluetoothInput (GioHandle *handle, AsyncMonitorCallback *callback, void *data) {
  return bthMonitorInput(handle->connection, callback, data);
}

static void *
getBluetoothResourceObject (GioHandle *handle) {
  return handle->connection;
}

static const GioMethods gioBluetoothMethods = {
  .disconnectResource = disconnectBluetoothResource,

  .makeResourceIdentifier = makeBluetoothResourceIdentifier,
  .getResourceName = getBluetoothResourceName,

  .writeData = writeBluetoothData,
  .awaitInput = awaitBluetoothInput,
  .readData = readBluetoothData,

  .monitorInput = monitorBluetoothInput,

  .getResourceObject = getBluetoothResourceObject
};

static int
testBluetoothIdentifier (const char **identifier) {
  return isBluetoothDeviceIdentifier(identifier);
}

static const GioPublicProperties gioPublicProperties_bluetooth = {
  .testIdentifier = testBluetoothIdentifier,

  .type = {
    .name = "Bluetooth",
    .identifier = GIO_TYPE_BLUETOOTH
  }
};

static int
isBluetoothSupported (const GioDescriptor *descriptor) {
  return descriptor->bluetooth.channelNumber || descriptor->bluetooth.discoverChannel;
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
    BluetoothConnectionRequest request;

    bthInitializeConnectionRequest(&request);
    request.driver = braille->definition.code;
    request.identifier = identifier;
    request.channel = descriptor->bluetooth.channelNumber;
    request.discover = descriptor->bluetooth.discoverChannel;

    memset(handle, 0, sizeof(*handle));

    if ((handle->connection = bthOpenConnection(&request))) {
      return handle;
    }

    free(handle);
  } else {
    logMallocError();
  }

  return NULL;
}

static const GioPrivateProperties gioPrivateProperties_bluetooth = {
  .isSupported = isBluetoothSupported,

  .getOptions = getBluetoothOptions,
  .getMethods = getBluetoothMethods,

  .connectResource = connectBluetoothResource
};

const GioProperties gioProperties_bluetooth = {
  .public = &gioPublicProperties_bluetooth,
  .private = &gioPrivateProperties_bluetooth
};
