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
#include "timing.h"
#include "io_generic.h"
#include "io_serial.h"
#include "io_usb.h"
#include "io_bluetooth.h"

typedef union {
  struct {
    SerialDevice *device;
  } serial;

  struct {
    UsbChannel *channel;
  } usb;

  struct {
    BluetoothConnection *connection;
  } bluetooth;
} GioHandle;

typedef struct {
  void *address;
  size_t size;
} HidReportItemsData;

typedef int DisconnectResourceMethod (GioHandle *handle);

typedef ssize_t WriteDataMethod (GioHandle *handle, const void *data, size_t size, int timeout);

typedef int AwaitInputMethod (GioHandle *handle, int timeout);

typedef ssize_t ReadDataMethod (
  GioHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

typedef int ReconfigureResourceMethod (GioHandle *handle, const SerialParameters *parameters);

typedef int TellResourceMethod (
  GioHandle *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size, int timeout
);

typedef int AskResourceMethod (
  GioHandle *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size, int timeout
);

typedef int GetHidReportItemsMethod (GioHandle *handle, HidReportItemsData *items, int timeout);

typedef size_t GetHidReportSizeMethod (const HidReportItemsData *items, unsigned char report);

typedef ssize_t SetHidReportMethod (
  GioHandle *handle, unsigned char report,
  const void *data, uint16_t size, int timeout
);

typedef ssize_t GetHidReportMethod (
  GioHandle *handle, unsigned char report,
  void *buffer, uint16_t size, int timeout
);

typedef ssize_t SetHidFeatureMethod (
  GioHandle *handle, unsigned char report,
  const void *data, uint16_t size, int timeout
);

typedef ssize_t GetHidFeatureMethod (
  GioHandle *handle, unsigned char report,
  void *buffer, uint16_t size, int timeout
);

typedef struct {
  DisconnectResourceMethod *disconnectResource;

  WriteDataMethod *writeData;
  AwaitInputMethod *awaitInput;
  ReadDataMethod *readData;

  ReconfigureResourceMethod *reconfigureResource;

  TellResourceMethod *tellResource;
  AskResourceMethod *askResource;

  GetHidReportItemsMethod *getHidReportItems;
  GetHidReportSizeMethod *getHidReportSize;

  SetHidReportMethod *setHidReport;
  GetHidReportMethod *getHidReport;

  SetHidFeatureMethod *setHidFeature;
  GetHidFeatureMethod *getHidFeature;
} InputOutputMethods;

struct GioEndpointStruct {
  GioHandle handle;
  const InputOutputMethods *methods;
  GioOptions options;
  unsigned int bytesPerSecond;
  HidReportItemsData hidReportItems;

  struct {
    int error;
    unsigned int from;
    unsigned int to;
    unsigned char buffer[0X40];
  } input;
};

static void
initializeOptions (GioOptions *options) {
  options->applicationData = NULL;
  options->readyDelay = 0;
  options->inputTimeout = 0;
  options->outputTimeout = 0;
}

void
gioInitializeDescriptor (GioDescriptor *descriptor) {
  descriptor->serial.parameters = NULL;
  initializeOptions(&descriptor->serial.options);
  descriptor->serial.options.inputTimeout = 100;

  descriptor->usb.channelDefinitions = NULL;
  initializeOptions(&descriptor->usb.options);
  descriptor->usb.options.inputTimeout = 1000;
  descriptor->usb.options.outputTimeout = 1000;

  descriptor->bluetooth.channelNumber = 0;
  initializeOptions(&descriptor->bluetooth.options);
  descriptor->bluetooth.options.inputTimeout = 100;
}

void
gioInitializeSerialParameters (SerialParameters *parameters) {
  parameters->baud = SERIAL_DEFAULT_BAUD;
  parameters->dataBits = SERIAL_DEFAULT_DATA_BITS;
  parameters->stopBits = SERIAL_DEFAULT_STOP_BITS;
  parameters->parity = SERIAL_DEFAULT_PARITY;
  parameters->flowControl = SERIAL_DEFAULT_FLOW_CONTROL;
}

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

static const InputOutputMethods serialMethods = {
  .disconnectResource = disconnectSerialResource,

  .writeData = writeSerialData,
  .awaitInput = awaitSerialInput,
  .readData = readSerialData,

  .reconfigureResource = reconfigureSerialResource
};

static int
disconnectUsbResource (GioHandle *handle) {
  usbCloseChannel(handle->usb.channel);
  return 1;
}

static ssize_t
writeUsbData (GioHandle *handle, const void *data, size_t size, int timeout) {
  UsbChannel *channel = handle->usb.channel;

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
  UsbChannel *channel = handle->usb.channel;

  return usbAwaitInput(channel->device,
                       channel->definition.inputEndpoint,
                       timeout);
}

static ssize_t
readUsbData (
  GioHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  UsbChannel *channel = handle->usb.channel;

  return usbReapInput(channel->device, channel->definition.inputEndpoint,
                      buffer, size, initialTimeout, subsequentTimeout);
}

static int
reconfigureUsbResource (GioHandle *handle, const SerialParameters *parameters) {
  UsbChannel *channel = handle->usb.channel;

  return usbSetSerialParameters(channel->device, parameters);
}

static int
tellUsbResource (
  GioHandle *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->usb.channel;

  return usbControlWrite(channel->device, recipient, type,
                         request, value, index, data, size, timeout);
}

static int
askUsbResource (
  GioHandle *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->usb.channel;

  return usbControlRead(channel->device, recipient, type,
                        request, value, index, buffer, size, timeout);
}

static int
getUsbHidReportItems (GioHandle *handle, HidReportItemsData *items, int timeout) {
  UsbChannel *channel = handle->usb.channel;
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
getUsbHidReportSize (const HidReportItemsData *items, unsigned char report) {
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
  UsbChannel *channel = handle->usb.channel;

  return usbHidSetReport(channel->device, channel->definition.interface,
                         report, data, size, timeout);
}

static ssize_t
getUsbHidReport (
  GioHandle *handle, unsigned char report,
  void *buffer, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->usb.channel;

  return usbHidGetReport(channel->device, channel->definition.interface,
                         report, buffer, size, timeout);
}

static ssize_t
setUsbHidFeature (
  GioHandle *handle, unsigned char report,
  const void *data, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->usb.channel;

  return usbHidSetFeature(channel->device, channel->definition.interface,
                          report, data, size, timeout);
}

static ssize_t
getUsbHidFeature (
  GioHandle *handle, unsigned char report,
  void *buffer, uint16_t size, int timeout
) {
  UsbChannel *channel = handle->usb.channel;

  return usbHidGetFeature(channel->device, channel->definition.interface,
                          report, buffer, size, timeout);
}

static const InputOutputMethods usbMethods = {
  .disconnectResource = disconnectUsbResource,

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
disconnectBluetoothResource (GioHandle *handle) {
  bthCloseConnection(handle->bluetooth.connection);
  return 1;
}

static ssize_t
writeBluetoothData (GioHandle *handle, const void *data, size_t size, int timeout) {
  return bthWriteData(handle->bluetooth.connection, data, size);
}

static int
awaitBluetoothInput (GioHandle *handle, int timeout) {
  return bthAwaitInput(handle->bluetooth.connection, timeout);
}

static ssize_t
readBluetoothData (
  GioHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return bthReadData(handle->bluetooth.connection, buffer, size,
                     initialTimeout, subsequentTimeout);
}

static const InputOutputMethods bluetoothMethods = {
  .disconnectResource = disconnectBluetoothResource,

  .writeData = writeBluetoothData,
  .awaitInput = awaitBluetoothInput,
  .readData = readBluetoothData
};

static int
logUnsupportedOperation (const char *name) {
  errno = ENOSYS;
  logSystemError(name);
  return -1;
}

static void
setBytesPerSecond (GioEndpoint *endpoint, const SerialParameters *parameters) {
  endpoint->bytesPerSecond = parameters->baud / serialGetCharacterSize(parameters);
}

GioEndpoint *
gioConnectResource (
  const char *identifier,
  const GioDescriptor *descriptor
) {
  GioEndpoint *endpoint;

  if ((endpoint = malloc(sizeof(*endpoint)))) {
    endpoint->bytesPerSecond = 0;

    endpoint->input.error = 0;
    endpoint->input.from = 0;
    endpoint->input.to = 0;

    endpoint->hidReportItems.address = NULL;
    endpoint->hidReportItems.size = 0;

    if (descriptor->serial.parameters) {
      if (isSerialDevice(&identifier)) {
        if ((endpoint->handle.serial.device = serialOpenDevice(identifier))) {
          if (serialSetParameters(endpoint->handle.serial.device, descriptor->serial.parameters)) {
            endpoint->methods = &serialMethods;
            endpoint->options = descriptor->serial.options;
            setBytesPerSecond(endpoint, descriptor->serial.parameters);
            goto connectSucceeded;
          }

          serialCloseDevice(endpoint->handle.serial.device);
        }

        goto connectFailed;
      }
    }

    if (descriptor->usb.channelDefinitions) {
      if (isUsbDevice(&identifier)) {
        if ((endpoint->handle.usb.channel = usbFindChannel(descriptor->usb.channelDefinitions, identifier))) {
          endpoint->methods = &usbMethods;
          endpoint->options = descriptor->usb.options;

          if (!endpoint->options.applicationData) {
            endpoint->options.applicationData = endpoint->handle.usb.channel->definition.data;
          }

          {
            UsbChannel *channel = endpoint->handle.usb.channel;
            const SerialParameters *parameters = channel->definition.serial;
            if (parameters) setBytesPerSecond(endpoint, parameters);
          }

          goto connectSucceeded;
        }

        goto connectFailed;
      }
    }

    if (descriptor->bluetooth.channelNumber) {
      if (isBluetoothDevice(&identifier)) {
        if ((endpoint->handle.bluetooth.connection = bthOpenConnection(identifier, descriptor->bluetooth.channelNumber, 0))) {
          endpoint->methods = &bluetoothMethods;
          endpoint->options = descriptor->bluetooth.options;
          goto connectSucceeded;
        }

        goto connectFailed;
      }
    }

    errno = ENOSYS;
    logMessage(LOG_WARNING, "unsupported input/output resource identifier: %s", identifier);

  connectFailed:
    free(endpoint);
  } else {
    logMallocError();
  }

  return NULL;

connectSucceeded:
  {
    int delay = endpoint->options.readyDelay;
    if (delay) approximateDelay(delay);
  }

  if (!gioDiscardInput(endpoint)) {
    int originalErrno = errno;
    gioDisconnectResource(endpoint);
    errno = originalErrno;
    return NULL;
  }

  return endpoint;
}

int
gioDisconnectResource (GioEndpoint *endpoint) {
  int ok = 0;
  DisconnectResourceMethod *method = endpoint->methods->disconnectResource;

  if (!method) {
    logUnsupportedOperation("disconnectResource");
  } else if (method(&endpoint->handle)) {
    ok = 1;
  }

  if (endpoint->hidReportItems.address) free(endpoint->hidReportItems.address);
  free(endpoint);
  return ok;
}

const void *
gioGetApplicationData (GioEndpoint *endpoint) {
  return endpoint->options.applicationData;
}

ssize_t
gioWriteData (GioEndpoint *endpoint, const void *data, size_t size) {
  WriteDataMethod *method = endpoint->methods->writeData;
  if (!method) return logUnsupportedOperation("writeData");
  return method(&endpoint->handle, data, size,
                endpoint->options.outputTimeout);
}

int
gioAwaitInput (GioEndpoint *endpoint, int timeout) {
  AwaitInputMethod *method = endpoint->methods->awaitInput;
  if (!method) return logUnsupportedOperation("awaitInput");
  if (endpoint->input.to - endpoint->input.from) return 1;
  return method(&endpoint->handle, timeout);
}

ssize_t
gioReadData (GioEndpoint *endpoint, void *buffer, size_t size, int wait) {
  ReadDataMethod *method = endpoint->methods->readData;
  if (!method) return logUnsupportedOperation("readData");

  {
    unsigned char *start = buffer;
    unsigned char *next = start;

    while (size) {
      {
        unsigned int count = endpoint->input.to - endpoint->input.from;

        if (count) {
          if (count > size) count = size;
          memcpy(next, &endpoint->input.buffer[endpoint->input.from], count);

          endpoint->input.from += count;
          next += count;
          size -= count;
          continue;
        }

        endpoint->input.from = endpoint->input.to = 0;
      }

      if (endpoint->input.error) {
        if (next != start) break;
        errno = endpoint->input.error;
        endpoint->input.error = 0;
        return -1;
      }

      {
        ssize_t result = method(&endpoint->handle,
                                &endpoint->input.buffer[endpoint->input.to],
                                sizeof(endpoint->input.buffer) - endpoint->input.to,
                                (wait? endpoint->options.inputTimeout: 0), 0);

        if (result > 0) {
          if (LOG_CATEGORY_FLAG(GENERIC_INPUT)) {
            logBytes(categoryLogLevel, "generic input", &endpoint->input.buffer[endpoint->input.to], result);
          }

          endpoint->input.to += result;
          wait = 1;
        } else {
          if (!result) break;
          if (errno == EAGAIN) break;
          endpoint->input.error = errno;
        }
      }
    }

    if (next == start) errno = EAGAIN;
    return next - start;
  }
}

int
gioReadByte (GioEndpoint *endpoint, unsigned char *byte, int wait) {
  ssize_t result = gioReadData(endpoint, byte, 1, wait);
  if (result > 0) return 1;
  if (result == 0) errno = EAGAIN;
  return 0;
}

int
gioDiscardInput (GioEndpoint *endpoint) {
  unsigned char byte;
  while (gioReadByte(endpoint, &byte, 0));
  return errno == EAGAIN;
}

int
gioReconfigureResource (
  GioEndpoint *endpoint,
  const SerialParameters *parameters
) {
  int ok = 0;
  ReconfigureResourceMethod *method = endpoint->methods->reconfigureResource;

  if (!method) {
    logUnsupportedOperation("reconfigureResource");
  } else if (method(&endpoint->handle, parameters)) {
    setBytesPerSecond(endpoint, parameters);
    ok = 1;
  }

  return ok;
}

unsigned int
gioGetBytesPerSecond (GioEndpoint *endpoint) {
  return endpoint->bytesPerSecond;
}

unsigned int
gioGetMillisecondsToTransfer (GioEndpoint *endpoint, size_t bytes) {
  return endpoint->bytesPerSecond? (((bytes * 1000) / endpoint->bytesPerSecond) + 1): 0;
}

ssize_t
gioTellResource (
  GioEndpoint *endpoint,
  uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size
) {
  TellResourceMethod *method = endpoint->methods->tellResource;
  if (!method) return logUnsupportedOperation("tellResource");
  return method(&endpoint->handle, recipient, type,
                request, value, index, data, size,
                endpoint->options.outputTimeout);
}

ssize_t
gioAskResource (
  GioEndpoint *endpoint,
  uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size
) {
  AskResourceMethod *method = endpoint->methods->askResource;
  if (!method) return logUnsupportedOperation("askResource");
  return method(&endpoint->handle, recipient, type,
                request, value, index, buffer, size,
                endpoint->options.inputTimeout);
}

size_t
gioGetHidReportSize (GioEndpoint *endpoint, unsigned char report) {
  if (!endpoint->hidReportItems.address) {
    GetHidReportItemsMethod *method = endpoint->methods->getHidReportItems;

    if (!method) {
      logUnsupportedOperation("getHidReportItems");
      return 0;
    }

    if (!method(&endpoint->handle, &endpoint->hidReportItems,
                endpoint->options.inputTimeout)) {
      return 0;
    }
  }

  {
    GetHidReportSizeMethod *method = endpoint->methods->getHidReportSize;
    if (!method) return logUnsupportedOperation("getHidReportSize");
    return method(&endpoint->hidReportItems, report);
  }
}

ssize_t
gioSetHidReport (
  GioEndpoint *endpoint, unsigned char report,
  const void *data, uint16_t size
) {
  SetHidReportMethod *method = endpoint->methods->setHidReport;
  if (!method) return logUnsupportedOperation("setHidReport");
  return method(&endpoint->handle, report,
                data, size, endpoint->options.outputTimeout);
}

ssize_t
gioGetHidReport (
  GioEndpoint *endpoint, unsigned char report,
  void *buffer, uint16_t size
) {
  GetHidReportMethod *method = endpoint->methods->getHidReport;
  if (!method) return logUnsupportedOperation("getHidReport");
  return method(&endpoint->handle, report,
                buffer, size, endpoint->options.inputTimeout);
}

ssize_t
gioSetHidFeature (
  GioEndpoint *endpoint, unsigned char report,
  const void *data, uint16_t size
) {
  SetHidFeatureMethod *method = endpoint->methods->setHidFeature;
  if (!method) return logUnsupportedOperation("setHidFeature");
  return method(&endpoint->handle, report,
                data, size, endpoint->options.outputTimeout);
}

ssize_t
gioGetHidFeature (
  GioEndpoint *endpoint, unsigned char report,
  void *buffer, uint16_t size
) {
  GetHidFeatureMethod *method = endpoint->methods->getHidFeature;
  if (!method) return logUnsupportedOperation("getHidFeature");
  return method(&endpoint->handle, report,
                buffer, size, endpoint->options.inputTimeout);
}
