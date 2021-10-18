/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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
#include "io_generic.h"
#include "gio_internal.h"
#include "io_hid.h"

struct GioHandleStruct {
  HidDevice *device;
};

static int
disconnectHidResource (GioHandle *handle) {
  hidCloseDevice(handle->device);
  free(handle);
  return 1;
}

static const char *
makeHidResourceIdentifier (GioHandle *handle, char *buffer, size_t size) {
  return hidMakeDeviceIdentifier(handle->device, buffer, size);
}

static char *
getHidResourceName (GioHandle *handle, int timeout) {
  const char *name = hidGetDeviceName(handle->device);

  if (name) {
    char *copy = strdup(name);
    if (copy) return copy;
    logMallocError();
  }

  return NULL;
}

static void *
getHidResourceObject (GioHandle *handle) {
  return handle->device;
}

static ssize_t
writeHidData (GioHandle *handle, const void *data, size_t size, int timeout) {
  return hidWriteData(handle->device, data, size);
}

static int
awaitHidInput (GioHandle *handle, int timeout) {
  return hidAwaitInput(handle->device, timeout);
}

static ssize_t
readHidData (
  GioHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return hidReadData(handle->device, buffer, size,
                     initialTimeout, subsequentTimeout);
}

static int
monitorHidInput (GioHandle *handle, AsyncMonitorCallback *callback, void *data) {
  return hidMonitorInput(handle->device, callback, data);
}

static HidItemsDescriptor *
getHidItems (GioHandle *handle, int timeout) {
  return hidGetItems(handle->device);
}

static ssize_t
setHidReport (
  GioHandle *handle, HidReportIdentifier identifier,
  const unsigned char *data, size_t size, int timeout
) {
  return hidSetReport(handle->device, data, size);
}

static ssize_t
getHidReport (
  GioHandle *handle, unsigned char identifier,
  void *buffer, uint16_t size, int timeout
) {
  return hidGetReport(handle->device, buffer, size);
}

static ssize_t
setHidFeature (
  GioHandle *handle, HidReportIdentifier identifier,
  const unsigned char *data, size_t size, int timeout
) {
  return hidSetFeature(handle->device, data, size);
}

static ssize_t
getHidFeature (
  GioHandle *handle, unsigned char identifier,
  void *buffer, uint16_t size, int timeout
) {
  return hidGetFeature(handle->device, buffer, size);
}

static const GioMethods gioHidMethods = {
  .disconnectResource = disconnectHidResource,

  .makeResourceIdentifier = makeHidResourceIdentifier,
  .getResourceName = getHidResourceName,
  .getResourceObject = getHidResourceObject,

  .writeData = writeHidData,
  .awaitInput = awaitHidInput,
  .readData = readHidData,
  .monitorInput = monitorHidInput,

  .getHidItems = getHidItems,
  .setHidReport = setHidReport,
  .getHidReport = getHidReport,
  .setHidFeature = setHidFeature,
  .getHidFeature = getHidFeature,
};

static int
testHidIdentifier (const char **identifier) {
  return isHidDeviceIdentifier(identifier);
}

static const GioPublicProperties gioPublicProperties_hid = {
  .testIdentifier = testHidIdentifier,

  .type = {
    .name = "HID",
    .identifier = GIO_TYPE_HID
  }
};

static int
isHidSupported (const GioDescriptor *descriptor) {
  return 1;
}

static const GioOptions *
getHidOptions (const GioDescriptor *descriptor) {
  return &descriptor->hid.options;
}

static const GioMethods *
getHidMethods (void) {
  return &gioHidMethods;
}

static GioHandle *
connectHidResource (
  const char *identifier,
  const GioDescriptor *descriptor
) {
  GioHandle *handle = malloc(sizeof(*handle));

  if (handle) {
    memset(handle, 0, sizeof(*handle));

    if (hidOpenDeviceWithParameters(&handle->device, identifier)) {
      if (handle->device) {
        return handle;
      }
    }

    free(handle);
  } else {
    logMallocError();
  }

  return NULL;
}

static const GioPrivateProperties gioPrivateProperties_hid = {
  .isSupported = isHidSupported,

  .getOptions = getHidOptions,
  .getMethods = getHidMethods,

  .connectResource = connectHidResource
};

const GioProperties gioProperties_hid = {
  .public = &gioPublicProperties_hid,
  .private = &gioPrivateProperties_hid
};
