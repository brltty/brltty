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
hidInitializeUSBFilter (HidUSBFilter *filter) {
  memset(filter, 0, sizeof(*filter));
}

void
hidInitializeBluetoothFilter (HidBluetoothFilter *filter) {
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
  const HidHandleMethods *handleMethods;
  const char *communicationMethod;
  HidItemsDescriptor *items;
};

static void
hidDestroyHandle (HidHandle *handle) {
  HidDestroyHandleMethod *method = hidPackageDescriptor.handleMethods->destroyHandle;

  if (method) {
    method(handle);
  } else {
    logUnsupportedOperation("hidCloseDevice");
    errno = ENOSYS;
  }
}

static HidDevice *
hidNewDevice (HidHandle *handle, const char *communicationMethod) {
  if (handle) {
    HidDevice *device;

    if ((device = malloc(sizeof(*device)))) {
      memset(device, 0, sizeof(*device));
      device->handle = handle;
      device->handleMethods = hidPackageDescriptor.handleMethods;
      device->communicationMethod = communicationMethod;
      return device;
    } else {
      logMallocError();
    }

    hidDestroyHandle(handle);
  }

  return NULL;
}

HidDevice *
hidOpenUSBDevice (const HidUSBFilter *filter) {
  HidNewUSBHandleMethod *method = hidPackageDescriptor.newUSBHandle;

  if (!method) {
    logUnsupportedOperation("hidOpenUSBDevice");
    errno = ENOSYS;
    return NULL;
  }

  return hidNewDevice(method(filter), "USB");
}

HidDevice *
hidOpenBluetoothDevice (const HidBluetoothFilter *filter) {
  HidNewBluetoothHandleMethod *method = hidPackageDescriptor.newBluetoothHandle;

  if (!method) {
    logUnsupportedOperation("hidOpenBluetoothDevice");
    errno = ENOSYS;
    return NULL;
  }

  return hidNewDevice(method(filter), "Bluetooth");
}

void
hidCloseDevice (HidDevice *device) {
  hidDestroyHandle(device->handle);
  if (device->items) free(device->items);
  free(device);
}

const HidItemsDescriptor *
hidGetItems (HidDevice *device) {
  if (!device->items) {
    HidGetItemsMethod *method = device->handleMethods->getItems;

    if (!method) {
      logUnsupportedOperation("hidGetItems");
      errno = ENOSYS;
      return NULL;
    }

    device->items = method(device->handle);
    if (!device->items) logMessage(LOG_ERR, "HID items descriptor not available");
  }

  return device->items;
}

int
hidGetIdentifiers (HidDevice *device, uint16_t *vendor, uint16_t *product) {
  HidGetIdentifiersMethod *method = device->handleMethods->getIdentifiers;

  if (!method) {
    logUnsupportedOperation("hidGetIdentifiers");
    errno = ENOSYS;
    return 0;
  }

  return method(device->handle, vendor, product);
}

static void
hidLogDataTransfer (const char *action, const unsigned char *data, size_t size, uint8_t identifier) {
  logBytes(LOG_CATEGORY(HUMAN_INTERFACE),
    "%s: %02X", data, size, action, identifier
  );
}

int
hidGetReport (HidDevice *device, unsigned char *buffer, size_t size) {
  HidGetReportMethod *method = device->handleMethods->getReport;

  if (!method) {
    logUnsupportedOperation("hidGetReport");
    errno = ENOSYS;
    return 0;
  }

  uint8_t identifier = *buffer;
  if (!method(device->handle, buffer, size)) return 0;

  hidLogDataTransfer("get report", buffer, size-1, identifier);
  return 1;
}

int
hidSetReport (HidDevice *device, const unsigned char *report, size_t size) {
  HidSetReportMethod *method = device->handleMethods->setReport;

  if (!method) {
    logUnsupportedOperation("hidSetReport");
    errno = ENOSYS;
    return 0;
  }

  hidLogDataTransfer("set report", report+1, size-1, *report);
  return method(device->handle, report, size);
}

int
hidGetFeature (HidDevice *device, unsigned char *buffer, size_t size) {
  HidGetFeatureMethod *method = device->handleMethods->getFeature;

  if (!method) {
    logUnsupportedOperation("hidGetFeature");
    errno = ENOSYS;
    return 0;
  }

  uint8_t identifier = *buffer;
  if (!method(device->handle, buffer, size)) return 0;

  hidLogDataTransfer("get feature", buffer, size-1, identifier);
  return 1;
}

int
hidSetFeature (HidDevice *device, const unsigned char *feature, size_t size) {
  HidSetFeatureMethod *method = device->handleMethods->setFeature;

  if (!method) {
    logUnsupportedOperation("hidSetFeature");
    errno = ENOSYS;
    return 0;
  }

  hidLogDataTransfer("set feature", feature+1, size-1, *feature);
  return method(device->handle, feature, size);
}

int
hidWriteData (HidDevice *device, const unsigned char *data, size_t size) {
  HidWriteDataMethod *method = device->handleMethods->writeData;

  if (!method) {
    logUnsupportedOperation("hidWriteData");
    errno = ENOSYS;
    return 0;
  }

  logBytes(LOG_CATEGORY(HUMAN_INTERFACE),
    "output", data, size
  );

  return method(device->handle, data, size);
}

int
hidMonitorInput (HidDevice *device, AsyncMonitorCallback *callback, void *data) {
  HidMonitorInputMethod *method = device->handleMethods->monitorInput;

  if (!method) {
    logUnsupportedOperation("hidMonitorInput");
    errno = ENOSYS;
    return 0;
  }

  return method(device->handle, callback, data);
}

int
hidAwaitInput (HidDevice *device, int timeout) {
  HidAwaitInputMethod *method = device->handleMethods->awaitInput;

  if (!method) {
    logUnsupportedOperation("hidAwaitInput");
    errno = ENOSYS;
    return 0;
  }

  return method(device->handle, timeout);
}

ssize_t
hidReadData (
  HidDevice *device, unsigned char *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  HidReadDataMethod *method = device->handleMethods->readData;

  if (!method) {
    logUnsupportedOperation("hidReadData");
    errno = ENOSYS;
    return -1;
  }

  ssize_t result = method(device->handle, buffer, size, initialTimeout, subsequentTimeout);

  if (result != -1) {
    logBytes(LOG_CATEGORY(HUMAN_INTERFACE),
      "input", buffer, result
    );
  }

  return result;
}

const char *
hidGetDeviceAddress (HidDevice *device) {
  HidGetDeviceAddressMethod *method = device->handleMethods->getDeviceAddress;

  if (!method) {
    logUnsupportedOperation("hidGetDeviceAddress");
    errno = ENOSYS;
    return NULL;
  }

  return method(device->handle);
}

const char *
hidGetDeviceName (HidDevice *device) {
  HidGetDeviceNameMethod *method = device->handleMethods->getDeviceName;

  if (!method) {
    logUnsupportedOperation("hidGetDeviceName");
    errno = ENOSYS;
    return NULL;
  }

  return method(device->handle);
}

const char *
hidGetHostPath (HidDevice *device) {
  HidGetHostPathMethod *method = device->handleMethods->getHostPath;

  if (!method) {
    logUnsupportedOperation("hidGetHostPath");
    errno = ENOSYS;
    return NULL;
  }

  return method(device->handle);
}

const char *
hidGetHostDevice (HidDevice *device) {
  HidGetHostDeviceMethod *method = device->handleMethods->getHostDevice;

  if (!method) {
    logUnsupportedOperation("hidGetHostDevice");
    errno = ENOSYS;
    return NULL;
  }

  return method(device->handle);
}

const char *
hidCacheString (
  HidHandle *handle, char **cachedValue,
  char *buffer, size_t size,
  HidGetStringMethod *getString, void *data
) {
  if (!*cachedValue) {
    if (!getString(handle, buffer, size, data)) return NULL;
    char *value = strdup(buffer);

    if (!value) {
      logMallocError();
      return NULL;
    }

    *cachedValue = value;
  }

  return *cachedValue;
}
