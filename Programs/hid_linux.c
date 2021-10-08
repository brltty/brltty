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

#include "log.h"
#include "io_hid.h"
#include "hid_linux.h"

struct HidDeviceStruct {
  char *sysPath;
  char *devPath;
  int fileDescriptor;
  char strings[];
};

typedef int HidAttributeTester (
  struct udev_device *device,
  const char *attributeName,
  const void *testValue
);

static int
hidTestString (
  struct udev_device *device,
  const char *attributeName,
  const void *testValue
) {
  const char *testString = testValue;
  if (!testString) return 1;
  if (!*testString) return 1;

  const char *actualString = udev_device_get_sysattr_value(device, attributeName);
  if (!actualString) return 0;
  if (!*actualString) return 0;

  return hidMatchString(actualString, testString);
}

static int
hidTestIdentifier (
  struct udev_device *device,
  const char *attributeName,
  const void *testValue
) {
  const uint16_t *testIdentifier = testValue;
  if (!*testIdentifier) return 1;

  uint16_t actualIdentifier;
  if (!hidParseIdentifier(&actualIdentifier, udev_device_get_sysattr_value(device, attributeName))) return 0;

  return actualIdentifier == *testIdentifier;
}

typedef struct {
  const char *name;
  const void *value;
  HidAttributeTester *function;
} HidAttributeTest;

static int
hidTestAttributes (
  struct udev_device *device,
  const HidAttributeTest *tests, size_t testCount
) {
  const HidAttributeTest *test = tests;
  const HidAttributeTest *end = test + testCount;

  while (test < end) {
    if (!test->function(device, test->name, test->value)) return 0;
    test += 1;
  }

  return 1;
}

static HidDevice *
hidNewDevice (struct udev_device *udevDevice) {
  const char *sysPath = udev_device_get_syspath(udevDevice);
  const char *devPath = udev_device_get_devnode(udevDevice);

  size_t sysSize = strlen(sysPath) + 1;
  size_t devSize = strlen(devPath) + 1;
  HidDevice *hidDevice = malloc(sizeof(*hidDevice) + sysSize + devSize);

  if (hidDevice) {
    memset(hidDevice, 0, sizeof(*hidDevice));

    {
      char *string = hidDevice->strings;
      string = mempcpy((hidDevice->sysPath = string), sysPath, sysSize);
      string = mempcpy((hidDevice->devPath = string), devPath, devSize);
    }

    if ((hidDevice->fileDescriptor = open(devPath, (O_RDWR | O_NONBLOCK))) != -1) {
      return hidDevice;
    } else {
      logMessage(LOG_ERR, "device open error: %s: %s", devPath, strerror(errno));
    }

    free(hidDevice);
  } else {
    logMallocError();
  }

  return NULL;
}

static HidDevice *
hidOpenDevice (
  const char *subsystem, const char *devtype,
  const HidAttributeTest *tests, size_t testCount
) {
  HidDevice *hidDevice = NULL;
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
        struct udev_device *sysDevice = udev_device_new_from_syspath(udev, sysPath);

        if (sysDevice) {
          struct udev_device *parent = udev_device_get_parent_with_subsystem_devtype(sysDevice, subsystem, devtype);

          if (parent) {
            if (hidTestAttributes(parent, tests, testCount)) {
              hidDevice = hidNewDevice(sysDevice);
            }

            udev_device_unref(sysDevice);
          }
        }

        if (hidDevice) break;
      }

      udev_enumerate_unref(enumeration);
      enumeration = NULL;
    }

    udev_unref(udev);
    udev = NULL;
  }

  return hidDevice;
}

HidDevice *
hidOpenDevice_USB (const HidDeviceFilter_USB *filter) {
  const HidAttributeTest tests[] = {
    { .name = "manufacturer",
      .value = filter->manufacturerName,
      .function = hidTestString
    },

    { .name = "product",
      .value = filter->productDescription,
      .function = hidTestString
    },

    { .name = "serial",
      .value = filter->serialNumber,
      .function = hidTestString
    },

    { .name = "idVendor",
      .value = &filter->vendorIdentifier,
      .function = hidTestIdentifier
    },

    { .name = "idProduct",
      .value = &filter->productIdentifier,
      .function = hidTestIdentifier
    },
  };

  return hidOpenDevice("usb", "usb_device", tests, ARRAY_COUNT(tests));
}

HidDevice *
hidOpenDevice_Bluetooth (const HidDeviceFilter_Bluetooth *filter) {
  const HidAttributeTest tests[] = {
    { .name = "address",
      .value = filter->deviceAddress,
      .function = hidTestString
    },
  };

  return hidOpenDevice("usb", "usb_device", tests, ARRAY_COUNT(tests));
}

void
hidCloseDevice (HidDevice *device) {
  close(device->fileDescriptor);
  free(device);
}

HidItemsDescriptor *
hidGetItems (HidDevice *device) {
  int size;

  if (ioctl(device->fileDescriptor, HIDIOCGRDESCSIZE, &size) != -1) {
    struct hidraw_report_descriptor descriptor = {
      .size = size
    };

    if (ioctl(device->fileDescriptor, HIDIOCGRDESC, &descriptor) != -1) {
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

int
hidGetIdentifiers (HidDevice *device, uint16_t *vendor, uint16_t *product) {
  struct hidraw_devinfo info;

  if (ioctl(device->fileDescriptor, HIDIOCGRAWINFO, &info) != -1) {
    if (vendor) *vendor = info.vendor;
    if (product) *product = info.product;
    return 1;
  } else {
    logSystemError("ioctl[HIDIOCGRAWINFO]");
  }

  return 0;
}

int
hidGetReport (HidDevice *device, char *buffer, size_t size) {
  if (ioctl(device->fileDescriptor, HIDIOCGINPUT(size), buffer) != -1) return 1;
  logSystemError("ioctl[HIDIOCGINPUT]");
  return 0;
}

int
hidSetReport (HidDevice *device, const char *report, size_t size) {
  if (ioctl(device->fileDescriptor, HIDIOCSOUTPUT(size), report) != -1) return 1;
  logSystemError("ioctl[HIDIOCSOUTPUT]");
  return 0;
}

int
hidGetFeature (HidDevice *device, char *buffer, size_t size) {
  if (ioctl(device->fileDescriptor, HIDIOCGFEATURE(size), buffer) != -1) return 1;
  logSystemError("ioctl[HIDIOCGFEATURE]");
  return 0;
}

int
hidSetFeature (HidDevice *device, const char *feature, size_t size) {
  if (ioctl(device->fileDescriptor, HIDIOCSFEATURE(size), feature) != -1) return 1;
  logSystemError("ioctl[HIDIOCSFEATURE]");
  return 0;
}

ssize_t
hidGetName (HidDevice *device, char *buffer, size_t size) {
  ssize_t length = ioctl(device->fileDescriptor, HIDIOCGRAWNAME(size), buffer);
  if (length == -1) logSystemError("ioctl[HIDIOCGRAWNAME]");
  return length;
}

ssize_t
hidGetPhysical (HidDevice *device, char *buffer, size_t size) {
  ssize_t length = ioctl(device->fileDescriptor, HIDIOCGRAWPHYS(size), buffer);
  if (length == -1) logSystemError("ioctl[HIDIOCGRAWPHYS]");
  return length;
}

ssize_t
hidGetUnique (HidDevice *device, char *buffer, size_t size) {
  ssize_t length = ioctl(device->fileDescriptor, HIDIOCGRAWUNIQ(size), buffer);
  if (length == -1) logSystemError("ioctl[HIDIOCGRAWUNIQ]");
  return length;
}
