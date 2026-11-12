/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
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
#include "bitmask.h"
#include "io_generic.h"
#include "gio_internal.h"
#include "io_hid.h"
#include "hid_items.h"
#include "hid_defs.h"

struct GioHandleStruct {
  HidDevice *device;
  const HidModelEntry *model;

  // The input reports of the device's keyboard (if it has one).
  BITMASK(keyboardReports, (UINT8_MAX + 1), char);
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
  while (1) {
    ssize_t result = hidReadData(handle->device, buffer, size,
                                 initialTimeout, subsequentTimeout);
    if (result <= 0) return result;

    // Each read returns one whole report. A display with a keyboard (e.g. an
    // APH Mantis Q40 connected via Bluetooth) also sends that keyboard's
    // reports this way. The operating system already handles them, and a
    // braille driver would only mistake the key codes within them for its
    // own reports, so drop them here.
    const unsigned char *report = buffer;
    if (!BITMASK_TEST(handle->keyboardReports, report[0])) return result;
  }
}

static int
monitorHidInput (GioHandle *handle, AsyncMonitorCallback *callback, void *data) {
  return hidMonitorInput(handle->device, callback, data);
}

int
getHidReportSize (
  GioHandle *handle, HidReportIdentifier identifier,
  HidReportSize *size, int timeout
) {
  return hidGetReportSize(handle->device, identifier, size);
}

static ssize_t
getHidReport (
  GioHandle *handle, HidReportIdentifier identifier,
  unsigned char *buffer, size_t size, int timeout
) {
  buffer[0] = identifier;
  return hidGetReport(handle->device, buffer, size);
}

static ssize_t
setHidReport (
  GioHandle *handle, HidReportIdentifier identifier,
  const unsigned char *data, size_t size, int timeout
) {
  unsigned char buffer[1 + size];

  if (!identifier) {
    buffer[0] = identifier;
    memcpy(&buffer[1], data, size);

    data = buffer;
    size += 1;
  }

  return hidSetReport(handle->device, data, size);
}

static ssize_t
getHidFeature (
  GioHandle *handle, HidReportIdentifier identifier,
  unsigned char *buffer, size_t size, int timeout
) {
  buffer[0] = identifier;
  return hidGetFeature(handle->device, buffer, size);
}

static ssize_t
setHidFeature (
  GioHandle *handle, HidReportIdentifier identifier,
  const unsigned char *data, size_t size, int timeout
) {
  unsigned char buffer[1 + size];

  if (!identifier) {
    buffer[0] = identifier;
    memcpy(&buffer[1], data, size);

    data = buffer;
    size += 1;
  }

  return hidSetFeature(handle->device, data, size);
}

static const GioHandleMethods gioHidMethods = {
  .disconnectResource = disconnectHidResource,

  .makeResourceIdentifier = makeHidResourceIdentifier,
  .getResourceName = getHidResourceName,
  .getResourceObject = getHidResourceObject,

  .writeData = writeHidData,
  .awaitInput = awaitHidInput,
  .readData = readHidData,
  .monitorInput = monitorHidInput,

  .getHidReportSize = getHidReportSize,
  .getHidReport = getHidReport,
  .setHidReport = setHidReport,
  .getHidFeature = getHidFeature,
  .setHidFeature = setHidFeature,
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
  return !!descriptor->hid.modelTable;
}

static const GioOptions *
getHidOptions (const GioDescriptor *descriptor) {
  return &descriptor->hid.options;
}

static const GioHandleMethods *
getHidMethods (void) {
  return &gioHidMethods;
}

static const HidModelEntry *
getHidModelEntry (HidDevice *device, const HidModelEntry *model) {
  if (model) {
    HidDeviceIdentifier vendor;
    HidDeviceIdentifier product;

    if (hidGetDeviceIdentifiers(device, &vendor, &product)) {
      for (; (model->vendor || model->product || model->name); model+=1) {
        if (model->vendor) {
          if (model->vendor != vendor) {
            continue;
          }
        }

        if (model->product) {
          if (model->product != product) {
            continue;
          }
        }

        if (model->name) {
          const char *name = hidGetDeviceName(device);
          if (!name) continue;

          size_t length = strlen(model->name);
          if (length > strlen(name)) continue;
          if (strncasecmp(name, model->name, length) != 0) continue;
        }

        logMessage(LOG_CATEGORY(HID_IO), "model found: %s", model->name);
        return model;
      }
    }
  }

  return NULL;
}

static void
findKeyboardReports (GioHandle *handle) {
  const HidItemsDescriptor *items = hidGetItems(handle->device);
  if (!items) return;

  const unsigned char *nextByte = items->bytes;
  size_t bytesLeft = items->count;

  HidUnsignedValue usagePage = 0;
  HidUnsignedValue usage = 0;
  HidUnsignedValue reportIdentifier = 0;
  unsigned int collectionDepth = 0;
  int isKeyboard = 0;

  while (bytesLeft) {
    HidItem item;
    if (!hidNextItem(&item, &nextByte, &bytesLeft)) break;

    switch (item.tag) {
      case HID_ITM_UsagePage:
        usagePage = item.value.u;
        break;

      case HID_ITM_Usage:
        usage = item.value.u;

        // A four-byte usage includes its own usage page.
        if (item.valueSize < 4) usage |= usagePage << 16;
        break;

      case HID_ITM_ReportID:
        reportIdentifier = item.value.u;
        break;

      case HID_ITM_Collection:
        if (!collectionDepth++) {
          isKeyboard =
            (usage == ((HID_UPG_GenericDesktop << 16) | HID_USG_GDT_Keyboard)) ||
            (usage == ((HID_UPG_GenericDesktop << 16) | HID_USG_GDT_Keypad));
        }

        usage = 0;
        break;

      case HID_ITM_EndCollection:
        if (collectionDepth) collectionDepth -= 1;
        usage = 0;
        break;

      case HID_ITM_Input:
        // Only numbered reports can be told apart from the others.
        if (isKeyboard && collectionDepth && reportIdentifier && (reportIdentifier <= UINT8_MAX)) {
          if (!BITMASK_TEST(handle->keyboardReports, reportIdentifier)) {
            BITMASK_SET(handle->keyboardReports, reportIdentifier);
            logMessage(LOG_CATEGORY(HID_IO), "keyboard report: %02X", (unsigned int)reportIdentifier);
          }
        }
        /* fall through */

      case HID_ITM_Output:
      case HID_ITM_Feature:
        usage = 0;
        break;

      default:
        break;
    }
  }
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
        handle->model = getHidModelEntry(
          handle->device,
          descriptor->hid.modelTable
        );

        if (handle->model) {
          findKeyboardReports(handle);
          return handle;
        }
      }
    }

    free(handle);
  } else {
    logMallocError();
  }

  return NULL;
}

static int
prepareHidEndpoint (GioEndpoint *endpoint) {
  gioSetApplicationData(endpoint, endpoint->handle->model->data);
  return 1;
}

static const GioPrivateProperties gioPrivateProperties_hid = {
  .isSupported = isHidSupported,

  .getOptions = getHidOptions,
  .getHandleMethods = getHidMethods,

  .connectResource = connectHidResource,
  .prepareEndpoint = prepareHidEndpoint,
};

const GioProperties gioProperties_hid = {
  .public = &gioPublicProperties_hid,
  .private = &gioPrivateProperties_hid
};
