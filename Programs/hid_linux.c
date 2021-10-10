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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <libudev.h>
#include <linux/hidraw.h>
#include <linux/input.h>

#include "log.h"
#include "io_hid.h"
#include "hid_internal.h"
#include "hid_linux.h"
#include "io_misc.h"

struct HidHandleStruct {
  char *sysfsPath;
  char *devicePath;

  int fileDescriptor;
  AsyncHandle inputMonitor;
  struct hidraw_devinfo deviceInformation;

  char strings[];
};

static void
hidLinuxCancelInputMonitor (HidHandle *handle) {
  if (handle->inputMonitor) {
    asyncCancelRequest(handle->inputMonitor);
    handle->inputMonitor = NULL;
  }
}

static void
hidLinuxDestroyHandle (HidHandle *handle) {
  hidLinuxCancelInputMonitor(handle);
  close(handle->fileDescriptor);
  free(handle);
}

ssize_t
hidLinuxGetDeviceName (HidHandle *handle, char *buffer, size_t size) {
  ssize_t length = ioctl(handle->fileDescriptor, HIDIOCGRAWNAME(size), buffer);
  if (length == -1) logSystemError("ioctl[HIDIOCGRAWNAME]");
  return length;
}

ssize_t
hidLinuxGetPhysicalAddress (HidHandle *handle, char *buffer, size_t size) {
  ssize_t length = ioctl(handle->fileDescriptor, HIDIOCGRAWPHYS(size), buffer);
  if (length == -1) logSystemError("ioctl[HIDIOCGRAWPHYS]");
  return length;
}

ssize_t
hidLinuxGetUniqueIdentifier (HidHandle *handle, char *buffer, size_t size) {
  ssize_t length = ioctl(handle->fileDescriptor, HIDIOCGRAWUNIQ(size), buffer);
  if (length == -1) logSystemError("ioctl[HIDIOCGRAWUNIQ]");
  return length;
}

typedef int HidLinuxAttributeTester (
  struct udev_device *device,
  const char *name,
  const void *value
);

static int
hidLinuxTestString (
  struct udev_device *device,
  const char *name,
  const void *value
) {
  const char *testString = value;
  if (!testString) return 1;
  if (!*testString) return 1;

  const char *actualString = udev_device_get_sysattr_value(device, name);
  if (!actualString) return 0;
  if (!*actualString) return 0;

  return hidMatchString(actualString, testString);
}

static int
hidLinuxTestIdentifier (
  struct udev_device *device,
  const char *name,
  const void *value
) {
  const uint16_t *testIdentifier = value;
  if (!*testIdentifier) return 1;

  uint16_t actualIdentifier;
  if (!hidParseIdentifier(&actualIdentifier, udev_device_get_sysattr_value(device, name))) return 0;

  return actualIdentifier == *testIdentifier;
}

typedef struct {
  const char *name;
  const void *value;
  HidLinuxAttributeTester *function;
} HidLinuxAttributeTest;

static int
hidLinuxTestAttributes (
  struct udev_device *device,
  const HidLinuxAttributeTest *tests,
  size_t testCount
) {
  const HidLinuxAttributeTest *test = tests;
  const HidLinuxAttributeTest *end = test + testCount;

  while (test < end) {
    if (!test->function(device, test->name, test->value)) return 0;
    test += 1;
  }

  return 1;
}

static HidHandle *
hidLinuxNewHandle (struct udev_device *device) {
  const char *sysPath = udev_device_get_syspath(device);
  const char *devPath = udev_device_get_devnode(device);

  size_t sysSize = strlen(sysPath) + 1;
  size_t devSize = strlen(devPath) + 1;
  HidHandle *handle = malloc(sizeof(*handle) + sysSize + devSize);

  if (handle) {
    memset(handle, 0, sizeof(*handle));

    {
      char *string = handle->strings;
      string = mempcpy((handle->sysfsPath = string), sysPath, sysSize);
      string = mempcpy((handle->devicePath = string), devPath, devSize);
    }

    if ((handle->fileDescriptor = open(devPath, (O_RDWR | O_NONBLOCK))) != -1) {
      if (ioctl(handle->fileDescriptor, HIDIOCGRAWINFO, &handle->deviceInformation) != -1) {
        return handle;
      } else {
        logSystemError("ioctl[HIDIOCGRAWINFO]");
      }

      close(handle->fileDescriptor);
    } else {
      logMessage(LOG_ERR, "device open error: %s: %s", devPath, strerror(errno));
    }

    free(handle);
  } else {
    logMallocError();
  }

  return NULL;
}

typedef int HidLinuxDeviceTester (
  struct udev_device *device,
  HidHandle *handle,
  const void *filter
);

static HidHandle *
hidLinuxFindDevice (HidLinuxDeviceTester *testDevice, const void *filter) {
  HidHandle *handle = NULL;
  struct udev *udev = udev_new();

  if (udev) {
    struct udev_enumerate *enumeration = udev_enumerate_new(udev);

    if (enumeration) {
      udev_enumerate_add_match_subsystem(enumeration, "hidraw");
      udev_enumerate_scan_devices(enumeration);

      struct udev_list_entry *deviceList = udev_enumerate_get_list_entry(enumeration);
      struct udev_list_entry *deviceEntry;

      udev_list_entry_foreach(deviceEntry, deviceList) {
        const char *sysPath = udev_list_entry_get_name(deviceEntry);
        struct udev_device *hidDevice = udev_device_new_from_syspath(udev, sysPath);

        if (hidDevice) {
          if ((handle = hidLinuxNewHandle(hidDevice))) {
            if (!testDevice(hidDevice, handle, filter)) {
              hidLinuxDestroyHandle(handle);
              handle = NULL;
            }
          }

          udev_device_unref(hidDevice);
        }

        if (handle) break;
      }

      udev_enumerate_unref(enumeration);
      enumeration = NULL;
    }

    udev_unref(udev);
    udev = NULL;
  }

  return handle;
}

static int
hidLinuxTestUSBDevice (struct udev_device *hidDevice, HidHandle *handle, const void *filter) {
  if (handle->deviceInformation.bustype != BUS_USB) return 0;
  const HidUSBFilter *huf = filter;

  struct udev_device *usbDevice = udev_device_get_parent_with_subsystem_devtype(hidDevice, "usb", "usb_device");
  if (!usbDevice) return 0;

  const HidLinuxAttributeTest tests[] = {
    { .name = "idVendor",
      .value = &huf->vendorIdentifier,
      .function = hidLinuxTestIdentifier
    },

    { .name = "idProduct",
      .value = &huf->productIdentifier,
      .function = hidLinuxTestIdentifier
    },

    { .name = "manufacturer",
      .value = huf->manufacturerName,
      .function = hidLinuxTestString
    },

    { .name = "product",
      .value = huf->productDescription,
      .function = hidLinuxTestString
    },

    { .name = "serial",
      .value = huf->serialNumber,
      .function = hidLinuxTestString
    },
  };

  return hidLinuxTestAttributes(usbDevice, tests, ARRAY_COUNT(tests));
}

static HidHandle *
hidLinuxNewUSBHandle (const HidUSBFilter *filter) {
  return hidLinuxFindDevice(hidLinuxTestUSBDevice, filter);
}

static int
hidLinuxTestBluetoothDevice (struct udev_device *hidDevice, HidHandle *handle, const void *filter) {
  if (handle->deviceInformation.bustype != BUS_BLUETOOTH) return 0;
  const HidBluetoothFilter *hbf = filter;

  if (hbf->vendorIdentifier) {
    if (handle->deviceInformation.vendor != hbf->vendorIdentifier) {
      return 0;
    }
  }

  if (hbf->productIdentifier) {
    if (handle->deviceInformation.product != hbf->productIdentifier) {
      return 0;
    }
  }

  {
    const char *address = hbf->macAddress;

    if (address && *address) {
      char buffer[0X100];
      hidLinuxGetUniqueIdentifier(handle, buffer, sizeof(buffer));
      if (strcasecmp(address, buffer) != 0) return 0;
    }
  }

  {
    const char *name = hbf->deviceName;

    if (name && *name) {
      char buffer[0X100];
      hidLinuxGetDeviceName(handle, buffer, sizeof(buffer));
      if (!hidMatchString(buffer, name)) return 0;
    }
  }

  return 1;
}

static HidHandle *
hidLinuxNewBluetoothHandle (const HidBluetoothFilter *filter) {
  return hidLinuxFindDevice(hidLinuxTestBluetoothDevice, filter);
}

static HidItemsDescriptor *
hidLinuxGetItems (HidHandle *handle) {
  int size;

  if (ioctl(handle->fileDescriptor, HIDIOCGRDESCSIZE, &size) != -1) {
    struct hidraw_report_descriptor descriptor = {
      .size = size
    };

    if (ioctl(handle->fileDescriptor, HIDIOCGRDESC, &descriptor) != -1) {
      HidItemsDescriptor *items;

      if ((items = malloc(sizeof(*items) + size))) {
        memset(items, 0, sizeof(*items));
        items->count = size;
        memcpy(items->bytes, descriptor.value, size);
        return items;
      } else {
        logMallocError();
      }
    } else {
      logSystemError("ioctl[HIDIOCGRDESC]");
    }
  } else {
    logSystemError("ioctl[HIDIOCGRDESCSIZE]");
  }

  return NULL;
}

static int
hidLinuxGetIdentifiers (HidHandle *handle, uint16_t *vendor, uint16_t *product) {
  if (vendor) *vendor = handle->deviceInformation.vendor;
  if (product) *product = handle->deviceInformation.product;
  return 1;
}

static int
hidLinuxGetReport (HidHandle *handle, char *buffer, size_t size) {
  if (ioctl(handle->fileDescriptor, HIDIOCGINPUT(size), buffer) != -1) return 1;
  logSystemError("ioctl[HIDIOCGINPUT]");
  return 0;
}

static int
hidLinuxSetReport (HidHandle *handle, const char *report, size_t size) {
  if (ioctl(handle->fileDescriptor, HIDIOCSOUTPUT(size), report) != -1) return 1;
  logSystemError("ioctl[HIDIOCSOUTPUT]");
  return 0;
}

static int
hidLinuxGetFeature (HidHandle *handle, char *buffer, size_t size) {
  if (ioctl(handle->fileDescriptor, HIDIOCGFEATURE(size), buffer) != -1) return 1;
  logSystemError("ioctl[HIDIOCGFEATURE]");
  return 0;
}

static int
hidLinuxSetFeature (HidHandle *handle, const char *feature, size_t size) {
  if (ioctl(handle->fileDescriptor, HIDIOCSFEATURE(size), feature) != -1) return 1;
  logSystemError("ioctl[HIDIOCSFEATURE]");
  return 0;
}

static int
hidLinuxMonitorInput (HidHandle *handle, AsyncMonitorCallback *callback, void *data) {
  hidLinuxCancelInputMonitor(handle);
  if (!callback) return 1;
  return asyncMonitorFileInput(&handle->inputMonitor, handle->fileDescriptor, callback, data);
}

static int
hidLinuxAwaitInput (HidHandle *handle, int timeout) {
  return awaitFileInput(handle->fileDescriptor, timeout);
}

static ssize_t
hidLinuxReadData (
  HidHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return readFile(handle->fileDescriptor, buffer, size, initialTimeout, subsequentTimeout);
}

HidPlatformMethods hidPlatformMethods = {
  .newUSBHandle = hidLinuxNewUSBHandle,
  .newBluetoothHandle = hidLinuxNewBluetoothHandle,
  .destroyHandle = hidLinuxDestroyHandle,

  .getItems = hidLinuxGetItems,
  .getIdentifiers = hidLinuxGetIdentifiers,

  .getReport = hidLinuxGetReport,
  .setReport = hidLinuxSetReport,

  .getFeature = hidLinuxGetFeature,
  .setFeature = hidLinuxSetFeature,

  .monitorInput = hidLinuxMonitorInput,
  .awaitInput = hidLinuxAwaitInput,
  .readData = hidLinuxReadData,
};
