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
#include <errno.h>

#include "log.h"
#include "io_hid.h"
#include "hid_internal.h"

void
hidInitializeDeviceFilter_USB (HidDeviceFilter_USB *filter) {
  memset(filter, 0, sizeof(*filter));
}

void
hidInitializeDeviceFilter_Bluetooth (HidDeviceFilter_Bluetooth *filter) {
  memset(filter, 0, sizeof(*filter));
}

int
hidParseIdentifier (uint16_t *identifier, const char *string) {
  if (!string) return 0;
  if (!*string) return 0;
  if (strlen(string) > 4) return 0;

  char *end;
  long int value = strtol(string, &end, 0X10);
  if (*end) return 0;

  if (value < 0) return 0;
  if (value > UINT16_MAX) return 0;

  *identifier = value;
  return 1;
}

int
hidMatchString (const char *actualString, const char *testString) {
  size_t testLength = strlen(testString);
  if (testLength > strlen(actualString)) return 0;
  return strncasecmp(actualString, testString, testLength) == 0;
}

struct HidDeviceStruct {
  HidHandle *handle;
};

static HidDevice *
hidNewDevice (HidHandle *handle) {
  if (handle) {
    HidDevice *device;

    if ((device = malloc(sizeof(*device)))) {
      memset(device, 0, sizeof(*device));
      device->handle = handle;
      return device;
    } else {
      logMallocError();
    }

    hidPlatformMethods.destroy(handle);
  }

  return NULL;
}

HidDevice *
hidOpenDevice_USB (const HidDeviceFilter_USB *filter) {
  HidNewUSBMethod *method = hidPlatformMethods.newUSB;

  if (!method) {
    logUnsupportedOperation("hidOpenDevice_USB");
    errno = ENOSYS;
    return NULL;
  }

  return hidNewDevice(method(filter));
}

HidDevice *
hidOpenDevice_Bluetooth (const HidDeviceFilter_Bluetooth *filter) {
  HidNewBluetoothMethod *method = hidPlatformMethods.newBluetooth;

  if (!method) {
    logUnsupportedOperation("hidOpenDevice_Bluetooth");
    errno = ENOSYS;
    return NULL;
  }

  return hidNewDevice(method(filter));
}

void
hidCloseDevice (HidDevice *device) {
  HidDestroyMethod *method = hidPlatformMethods.destroy;

  if (method) {
    method(device->handle);
  } else {
    logUnsupportedOperation("hidCloseDevice");
    errno = ENOSYS;
  }
}

HidItemsDescriptor *
hidGetItems (HidDevice *device) {
  HidGetItemsMethod *method = hidPlatformMethods.getItems;

  if (!method) {
    logUnsupportedOperation("hidGetItems");
    errno = ENOSYS;
    return NULL;
  }

  return method(device->handle);
}

int
hidGetIdentifiers (HidDevice *device, uint16_t *vendor, uint16_t *product) {
  HidGetIdentifiersMethod *method = hidPlatformMethods.getIdentifiers;

  if (!method) {
    logUnsupportedOperation("hidGetIdentifiers");
    errno = ENOSYS;
    return 0;
  }

  return method(device->handle, vendor, product);
}

int
hidGetReport (HidDevice *device, char *buffer, size_t size) {
  HidGetReportMethod *method = hidPlatformMethods.getReport;

  if (!method) {
    logUnsupportedOperation("hidGetReport");
    errno = ENOSYS;
    return 0;
  }

  return method(device->handle, buffer, size);
}

int
hidSetReport (HidDevice *device, const char *report, size_t size) {
  HidSetReportMethod *method = hidPlatformMethods.setReport;

  if (!method) {
    logUnsupportedOperation("hidSetReport");
    errno = ENOSYS;
    return 0;
  }

  return method(device->handle, report, size);
}

int
hidGetFeature (HidDevice *device, char *buffer, size_t size) {
  HidGetFeatureMethod *method = hidPlatformMethods.getFeature;

  if (!method) {
    logUnsupportedOperation("hidGetFeature");
    errno = ENOSYS;
    return 0;
  }

  return method(device->handle, buffer, size);
}

int
hidSetFeature (HidDevice *device, const char *feature, size_t size) {
  HidSetFeatureMethod *method = hidPlatformMethods.setFeature;

  if (!method) {
    logUnsupportedOperation("hidSetFeature");
    errno = ENOSYS;
    return 0;
  }

  return method(device->handle, feature, size);
}

int
hidMonitorInput (HidDevice *device, AsyncMonitorCallback *callback, void *data) {
  HidMonitorInputMethod *method = hidPlatformMethods.monitorInput;

  if (!method) {
    logUnsupportedOperation("hidMonitorInput");
    errno = ENOSYS;
    return 0;
  }

  return method(device->handle, callback, data);
}

int
hidAwaitInput (HidDevice *device, int timeout) {
  HidAwaitInputMethod *method = hidPlatformMethods.awaitInput;

  if (!method) {
    logUnsupportedOperation("hidAwaitInput");
    errno = ENOSYS;
    return 0;
  }

  return method(device->handle, timeout);
}

ssize_t
hidReadData (
  HidDevice *device, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  HidReadDataMethod *method = hidPlatformMethods.readData;

  if (!method) {
    logUnsupportedOperation("hidReadData");
    errno = ENOSYS;
    return -1;
  }

  return method(device->handle, buffer, size, initialTimeout, subsequentTimeout);
}
