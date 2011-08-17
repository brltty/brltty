/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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

#include <errno.h>

#include "log.h"
#include "timing.h"
#include "io_endpoint.h"
#include "io_serial.h"
#include "io_usb.h"
#include "io_bluetooth.h"

typedef int DisconnectResourceMethod (void *handle);

typedef ssize_t WriteDataMethod (void *handle, const void *data, size_t size, int timeout);

typedef int AwaitInputMethod (void *handle, int timeout);

typedef ssize_t ReadDataMethod (
  void *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

typedef int TellResourceMethod (
  void *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size, int timeout
);

typedef int AskResourceMethod (
  void *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size, int timeout
);

typedef ssize_t SetHidReportMethod (
  void *handle, unsigned char interface, unsigned char report,
  const void *data, uint16_t size, int timeout
);

typedef ssize_t GetHidReportMethod (
  void *handle, unsigned char interface, unsigned char report,
  void *buffer, uint16_t size, int timeout
);

typedef ssize_t SetHidFeatureMethod (
  void *handle, unsigned char interface, unsigned char report,
  const void *data, uint16_t size, int timeout
);

typedef ssize_t GetHidFeatureMethod (
  void *handle, unsigned char interface, unsigned char report,
  void *buffer, uint16_t size, int timeout
);

typedef struct {
  DisconnectResourceMethod *disconnectResource;

  WriteDataMethod *writeData;
  AwaitInputMethod *awaitInput;
  ReadDataMethod *readData;

  TellResourceMethod *tellResource;
  AskResourceMethod *askResource;

  SetHidReportMethod *setHidReport;
  GetHidReportMethod *getHidReport;
  SetHidFeatureMethod *setHidFeature;
  GetHidFeatureMethod *getHidFeature;
} InputOutputMethods;

struct InputOutputEndpointStruct {
  void *handle;
  const InputOutputMethods *methods;
  InputOutputEndpointAttributes attributes;
  int bytesPerSecond;
};

static void
initializeEndpointAttributes (InputOutputEndpointAttributes *attributes) {
  attributes->applicationData = NULL;
  attributes->readyDelay = 0;
  attributes->inputTimeout = 0;
  attributes->outputTimeout = 0;
}

void
ioInitializeEndpointSpecification (InputOutputEndpointSpecification *specification) {
  specification->serial.parameters = NULL;
  initializeEndpointAttributes(&specification->serial.attributes);
  specification->serial.attributes.inputTimeout = 100;

  specification->usb.channelDefinitions = NULL;
  initializeEndpointAttributes(&specification->usb.attributes);
  specification->usb.attributes.inputTimeout = 1000;
  specification->usb.attributes.outputTimeout = 1000;

  specification->bluetooth.channelNumber = 0;
  initializeEndpointAttributes(&specification->bluetooth.attributes);
  specification->bluetooth.attributes.inputTimeout = 100;
}

void
ioInitializeSerialParameters (SerialParameters *parameters) {
  parameters->baud = 9600;
  parameters->flowControl = SERIAL_FLOW_NONE;
  parameters->dataBits = 8;
  parameters->stopBits = 1;
  parameters->parity = SERIAL_PARITY_NONE;
}

static int
disconnectSerialResource (void *handle) {
  serialCloseDevice(handle);
  return 1;
}

static ssize_t
writeSerialData (void *handle, const void *data, size_t size, int timeout) {
  return serialWriteData(handle, data, size);
}

static int
awaitSerialInput (void *handle, int timeout) {
  return serialAwaitInput(handle, timeout);
}

static ssize_t
readSerialData (
  void *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return serialReadData(handle, buffer, size,
                        initialTimeout, subsequentTimeout);
}

static const InputOutputMethods serialMethods = {
  .disconnectResource = disconnectSerialResource,

  .writeData = writeSerialData,
  .awaitInput = awaitSerialInput,
  .readData = readSerialData
};

static int
disconnectUsbResource (void *handle) {
  usbCloseChannel(handle);
  return 1;
}

static int
writeUsbData (void *handle, const void *data, size_t size, int timeout) {
  UsbChannel *channel = handle;

  return usbWriteEndpoint(channel->device,
                          channel->definition.outputEndpoint,
                          data, size, timeout);
}

static int
awaitUsbInput (void *handle, int timeout) {
  UsbChannel *channel = handle;

  return usbAwaitInput(channel->device,
                       channel->definition.inputEndpoint,
                       timeout);
}

static ssize_t
readUsbData (
  void *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  UsbChannel *channel = handle;

  return usbReapInput(channel->device, channel->definition.inputEndpoint,
                      buffer, size, initialTimeout, subsequentTimeout);
}

static int
tellUsbResource (
  void *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size, int timeout
) {
  UsbChannel *channel = handle;

  return usbControlWrite(channel->device, recipient, type,
                         request, value, index, data, size, timeout);
}

static int
askUsbResource (
  void *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size, int timeout
) {
  UsbChannel *channel = handle;

  return usbControlRead(channel->device, recipient, type,
                        request, value, index, buffer, size, timeout);
}

static ssize_t
setUsbHidReport (
  void *handle, unsigned char interface, unsigned char report,
  const void *data, uint16_t size, int timeout
) {
  UsbChannel *channel = handle;

  return usbHidSetReport(channel->device, interface, report, data, size, timeout);
}

static ssize_t
getUsbHidReport (
  void *handle, unsigned char interface, unsigned char report,
  void *buffer, uint16_t size, int timeout
) {
  UsbChannel *channel = handle;

  return usbHidGetReport(channel->device, interface, report, buffer, size, timeout);
}

static ssize_t
setUsbHidFeature (
  void *handle, unsigned char interface, unsigned char report,
  const void *data, uint16_t size, int timeout
) {
  UsbChannel *channel = handle;

  return usbHidSetFeature(channel->device, interface, report, data, size, timeout);
}

static ssize_t
getUsbHidFeature (
  void *handle, unsigned char interface, unsigned char report,
  void *buffer, uint16_t size, int timeout
) {
  UsbChannel *channel = handle;

  return usbHidGetFeature(channel->device, interface, report, buffer, size, timeout);
}

static const InputOutputMethods usbMethods = {
  .disconnectResource = disconnectUsbResource,

  .writeData = writeUsbData,
  .awaitInput = awaitUsbInput,
  .readData = readUsbData,

  .tellResource = tellUsbResource,
  .askResource = askUsbResource,

  .setHidReport = setUsbHidReport,
  .getHidReport = getUsbHidReport,
  .setHidFeature = setUsbHidFeature,
  .getHidFeature = getUsbHidFeature
};

static int
disconnectBluetoothResource (void *handle) {
  bthCloseConnection(handle);
  return 1;
}

static ssize_t
writeBluetoothData (void *handle, const void *data, size_t size, int timeout) {
  return bthWriteData(handle, data, size);
}

static int
awaitBluetoothInput (void *handle, int timeout) {
  return bthAwaitInput(handle, timeout);
}

static ssize_t
readBluetoothData (
  void *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return bthReadData(handle, buffer, size,
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

InputOutputEndpoint *
ioConnectResource (
  const char *identifier,
  const InputOutputEndpointSpecification *specification
) {
  InputOutputEndpoint *endpoint;

  if ((endpoint = malloc(sizeof(*endpoint)))) {
    endpoint->bytesPerSecond = 0;

    if (specification->serial.parameters) {
      if (isSerialDevice(&identifier)) {
        if ((endpoint->handle = serialOpenDevice(identifier))) {
          if (serialSetParameters(endpoint->handle, specification->serial.parameters)) {
            if (serialRestartDevice(endpoint->handle, specification->serial.parameters->baud)) {
              endpoint->methods = &serialMethods;
              endpoint->attributes = specification->serial.attributes;
              endpoint->bytesPerSecond = specification->serial.parameters->baud / serialGetCharacterBits(endpoint->handle);
              goto opened;
            }
          }

          serialCloseDevice(endpoint->handle);
        }

        goto openFailed;
      }
    }

    if (specification->usb.channelDefinitions) {
      if (isUsbDevice(&identifier)) {
        if ((endpoint->handle = usbFindChannel(specification->usb.channelDefinitions, identifier))) {
          endpoint->methods = &usbMethods;
          endpoint->attributes = specification->usb.attributes;
          goto opened;
        }

        goto openFailed;
      }
    }

    if (specification->bluetooth.channelNumber) {
      if (isBluetoothDevice(&identifier)) {
        if ((endpoint->handle = bthOpenConnection(identifier, specification->bluetooth.channelNumber, 1))) {
          endpoint->methods = &bluetoothMethods;
          endpoint->attributes = specification->bluetooth.attributes;
          goto opened;
        }

        goto openFailed;
      }
    }

    errno = ENOSYS;
    logMessage(LOG_WARNING, "unsupported input/output resource identifier: %s", identifier);

  openFailed:
    free(endpoint);
  } else {
    logMallocError();
  }

  return NULL;

opened:
  {
    int delay = endpoint->attributes.readyDelay;
    if (delay) approximateDelay(delay);
  }

  return endpoint;
}

int
ioDisconnectResource (InputOutputEndpoint *endpoint) {
  int ok = 0;
  DisconnectResourceMethod *method = endpoint->methods->disconnectResource;

  if (!method) {
    logUnsupportedOperation("ioDisconnectResource");
  } else if (method(endpoint->handle)) {
    ok = 1;
  }

  free(endpoint);
  return ok;
}

const void *
ioGetApplicationData (InputOutputEndpoint *endpoint) {
  return endpoint->attributes.applicationData;
}

int
ioGetBytesPerSecond (InputOutputEndpoint *endpoint) {
  return endpoint->bytesPerSecond;
}

ssize_t
ioWriteData (InputOutputEndpoint *endpoint, const void *data, size_t size) {
  WriteDataMethod *method = endpoint->methods->writeData;
  if (!method) return logUnsupportedOperation("ioWriteData");
  return method(endpoint->handle, data, size,
                endpoint->attributes.outputTimeout);
}

int
ioAwaitInput (InputOutputEndpoint *endpoint, int timeout) {
  AwaitInputMethod *method = endpoint->methods->awaitInput;
  if (!method) return logUnsupportedOperation("ioAwaitInput");
  return method(endpoint->handle, timeout);
}

ssize_t
ioReadData (InputOutputEndpoint *endpoint, void *buffer, size_t size, int wait) {
  ReadDataMethod *method = endpoint->methods->readData;
  if (!method) return logUnsupportedOperation("ioReadData");

  {
    int timeout = endpoint->attributes.inputTimeout;
    ssize_t result = method(endpoint->handle, buffer, size,
                            (wait? timeout: 0), timeout);

    if (result != -1) return result;
    if (errno == EAGAIN) return 0;
  }

  return -1;
}

int
ioReadByte (InputOutputEndpoint *endpoint, unsigned char *byte, int wait) {
  ssize_t result = ioReadData(endpoint, byte, 1, wait);
  if (result > 0) return 1;
  if (result == 0) errno = EAGAIN;
  return 0;
}

ssize_t
ioTellResource (
  InputOutputEndpoint *endpoint,
  uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size
) {
  TellResourceMethod *method = endpoint->methods->tellResource;
  if (!method) return logUnsupportedOperation("ioTellResource");
  return method(endpoint->handle, recipient, type,
                request, value, index, data, size,
                endpoint->attributes.outputTimeout);
}

ssize_t
ioAskResource (
  InputOutputEndpoint *endpoint,
  uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size
) {
  AskResourceMethod *method = endpoint->methods->askResource;
  if (!method) return logUnsupportedOperation("ioAskResource");
  return method(endpoint->handle, recipient, type,
                request, value, index, buffer, size,
                endpoint->attributes.inputTimeout);
}

ssize_t
ioSetHidReport (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  const void *data, uint16_t size
) {
  SetHidReportMethod *method = endpoint->methods->setHidReport;
  if (!method) return logUnsupportedOperation("ioSetHidReport");
  return method(endpoint->handle, interface, report,
                data, size, endpoint->attributes.outputTimeout);
}

ssize_t
ioGetHidReport (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  void *buffer, uint16_t size
) {
  GetHidReportMethod *method = endpoint->methods->getHidReport;
  if (!method) return logUnsupportedOperation("ioGetHidReport");
  return method(endpoint->handle, interface, report,
                buffer, size, endpoint->attributes.inputTimeout);
}

ssize_t
ioSetHidFeature (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  const void *data, uint16_t size
) {
  SetHidFeatureMethod *method = endpoint->methods->setHidFeature;
  if (!method) return logUnsupportedOperation("ioSetHidFeature");
  return method(endpoint->handle, interface, report,
                data, size, endpoint->attributes.outputTimeout);
}

ssize_t
ioGetHidFeature (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  void *buffer, uint16_t size
) {
  GetHidFeatureMethod *method = endpoint->methods->getHidFeature;
  if (!method) return logUnsupportedOperation("ioGetHidFeature");
  return method(endpoint->handle, interface, report,
                buffer, size, endpoint->attributes.inputTimeout);
}
