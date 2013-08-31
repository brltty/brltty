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
#include <errno.h>

#include "log.h"
#include "io_generic.h"
#include "gio_internal.h"
#include "io_usb.h"

struct GioHandleStruct {
  UsbChannel *channel;
};

static int
disconnectUsbResource (GioHandle *handle) {
  usbCloseChannel(handle->channel);
  free(handle);
  return 1;
}

static char *
getUsbResourceName (GioHandle *handle, int timeout) {
  UsbChannel *channel = handle->channel;

  return usbGetProduct(channel->device, timeout);
}

static ssize_t
writeUsbData (GioHandle *handle, const void *data, size_t size, int timeout) {
  UsbChannel *channel = handle->channel;

  if (channel->definition.outputEndpoint) {
    return usbWriteEndpoint(channel->device,
                            channel->definition.outputEndpoint,
                            data, size, timeout);
  }

  {
    const UsbSerialOperations *serial = usbGetSerialOperations(channel->device);

    if (serial) {
      if (serial->writeData) {
        return serial->writeData(channel->device, data, size);
      }
    }
  }

  errno = ENOSYS;
  return -1;
}

static int
awaitUsbInput (GioHandle *handle, int timeout) {
  UsbChannel *channel = handle->channel;

  return usbAwaitInput(channel->device,
                       channel->definition.inputEndpoint,
                       timeout);
}

static ssize_t
readUsbData (
  GioHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  UsbChannel *channel = handle->channel;

  return usbReadData(channel->device, channel->definition.inputEndpoint,
                     buffer, size, initialTimeout, subsequentTimeout);
}

static int
reconfigureUsbResource (GioHandle *handle, const SerialParameters *parameters) {
  UsbChannel *channel = handle->channel;

  return usbSetSerialParameters(channel->device, parameters);
}

static ssize_t
tellUsbResource (
  GioHandle *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->channel;

  return usbControlWrite(channel->device, recipient, type,
                         request, value, index, data, size, timeout);
}

static ssize_t
askUsbResource (
  GioHandle *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->channel;

  return usbControlRead(channel->device, recipient, type,
                        request, value, index, buffer, size, timeout);
}

static int
getUsbHidReportItems (GioHandle *handle, GioHidReportItemsData *items, int timeout) {
  UsbChannel *channel = handle->channel;
  unsigned char *address;
  ssize_t result = usbHidGetItems(channel->device,
                                  channel->definition.interface, 0,
                                  &address, timeout);

  if (!address) return 0;
  items->address = address;
  items->size = result;
  return 1;
}

static size_t
getUsbHidReportSize (const GioHidReportItemsData *items, unsigned char report) {
  size_t size;
  if (usbHidGetReportSize(items->address, items->size, report, &size)) return size;
  errno = ENOSYS;
  return 0;
}

static ssize_t
setUsbHidReport (
  GioHandle *handle, unsigned char report,
  const void *data, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->channel;

  return usbHidSetReport(channel->device, channel->definition.interface,
                         report, data, size, timeout);
}

static ssize_t
getUsbHidReport (
  GioHandle *handle, unsigned char report,
  void *buffer, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->channel;

  return usbHidGetReport(channel->device, channel->definition.interface,
                         report, buffer, size, timeout);
}

static ssize_t
setUsbHidFeature (
  GioHandle *handle, unsigned char report,
  const void *data, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->channel;

  return usbHidSetFeature(channel->device, channel->definition.interface,
                          report, data, size, timeout);
}

static ssize_t
getUsbHidFeature (
  GioHandle *handle, unsigned char report,
  void *buffer, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->channel;

  return usbHidGetFeature(channel->device, channel->definition.interface,
                          report, buffer, size, timeout);
}

static const GioEndpointMethods gioUsbEndpointMethods = {
  .disconnectResource = disconnectUsbResource,
  .getResourceName = getUsbResourceName,

  .writeData = writeUsbData,
  .awaitInput = awaitUsbInput,
  .readData = readUsbData,

  .reconfigureResource = reconfigureUsbResource,

  .tellResource = tellUsbResource,
  .askResource = askUsbResource,

  .getHidReportItems = getUsbHidReportItems,
  .getHidReportSize = getUsbHidReportSize,

  .setHidReport = setUsbHidReport,
  .getHidReport = getUsbHidReport,

  .setHidFeature = setUsbHidFeature,
  .getHidFeature = getUsbHidFeature
};

static int
isUsbSupported (const GioDescriptor *descriptor) {
  return descriptor->usb.channelDefinitions != NULL;
}

static int
testUsbIdentifier (const char **identifier) {
  return isUsbDevice(identifier);
}

static int
connectUsbResource (
  const char *identifier,
  const GioDescriptor *descriptor,
  GioEndpoint *endpoint
) {
  GioHandle *handle = malloc(sizeof(*handle));

  if (handle) {
    memset(handle, 0,sizeof(*handle));

    if ((handle->channel = usbFindChannel(descriptor->usb.channelDefinitions, identifier))) {
      endpoint->handle = handle;
      endpoint->methods = &gioUsbEndpointMethods;
      endpoint->options = descriptor->usb.options;

      if (!endpoint->options.applicationData) {
        endpoint->options.applicationData = endpoint->handle->channel->definition.data;
      }

      {
        UsbChannel *channel = endpoint->handle->channel;
        const SerialParameters *parameters = channel->definition.serial;

        if (parameters) {
          gioSetBytesPerSecond(endpoint, parameters);
        }
      }

      return 1;
    }

    free(handle);
  } else {
    logMallocError();
  }

  return 0;
}

const GioResourceEntry gioUsbResourceEntry = {
  .isSupported = isUsbSupported,
  .testIdentifier = testUsbIdentifier,
  .connectResource = connectUsbResource
};
