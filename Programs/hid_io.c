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

    hidPlatformMethods.destroyHandle(handle);
  }

  return NULL;
}

HidDevice *
hidOpenUSBDevice (const HidUSBFilter *filter) {
  HidNewUSBHandleMethod *method = hidPlatformMethods.newUSBHandle;

  if (!method) {
    logUnsupportedOperation("hidOpenUSBDevice");
    errno = ENOSYS;
    return NULL;
  }

  return hidNewDevice(method(filter));
}

HidDevice *
hidOpenBluetoothDevice (const HidBluetoothFilter *filter) {
  HidNewBluetoothHandleMethod *method = hidPlatformMethods.newBluetoothHandle;

  if (!method) {
    logUnsupportedOperation("hidOpenBluetoothDevice");
    errno = ENOSYS;
    return NULL;
  }

  return hidNewDevice(method(filter));
}

void
hidCloseDevice (HidDevice *device) {
  HidDestroyHandleMethod *method = hidPlatformMethods.destroyHandle;

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

static void
hidLogReport (const char *action, const char *data, size_t size, uint8_t identifier) {
  logBytes(LOG_CATEGORY(HUMAN_INTERFACE),
    "%s: %02X", data, size, action, identifier
  );
}

int
hidGetReport (HidDevice *device, char *buffer, size_t size) {
  HidGetReportMethod *method = hidPlatformMethods.getReport;

  if (!method) {
    logUnsupportedOperation("hidGetReport");
    errno = ENOSYS;
    return 0;
  }

  uint8_t identifier = *buffer;
  if (!method(device->handle, buffer, size)) return 0;

  hidLogReport("get report", buffer, size-1, identifier);
  return 1;
}

int
hidSetReport (HidDevice *device, const char *report, size_t size) {
  HidSetReportMethod *method = hidPlatformMethods.setReport;

  if (!method) {
    logUnsupportedOperation("hidSetReport");
    errno = ENOSYS;
    return 0;
  }

  if (!method(device->handle, report, size)) return 0;
  hidLogReport("set report", report+1, size-1, *report);
  return 1;
}

int
hidGetFeature (HidDevice *device, char *buffer, size_t size) {
  HidGetFeatureMethod *method = hidPlatformMethods.getFeature;

  if (!method) {
    logUnsupportedOperation("hidGetFeature");
    errno = ENOSYS;
    return 0;
  }

  uint8_t identifier = *buffer;
  if (!method(device->handle, buffer, size)) return 0;

  hidLogReport("get feature", buffer, size-1, identifier);
  return 1;
}

int
hidSetFeature (HidDevice *device, const char *feature, size_t size) {
  HidSetFeatureMethod *method = hidPlatformMethods.setFeature;

  if (!method) {
    logUnsupportedOperation("hidSetFeature");
    errno = ENOSYS;
    return 0;
  }

  if (!method(device->handle, feature, size)) return 0;
  hidLogReport("set feature", feature+1, size-1, *feature);
  return 1;
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

  ssize_t result = method(device->handle, buffer, size, initialTimeout, subsequentTimeout);

  if (result != -1) {
    logBytes(LOG_CATEGORY(HUMAN_INTERFACE),
      "interrupt input", buffer, result
    );
  }

  return result;
}
