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
#include "io_endpoint.h"
#include "io_serial.h"
#include "io_usb.h"
#include "io_bluetooth.h"

typedef void CloseHandleMethod (void *handle);

typedef ssize_t WriteDataMethod (void *handle, const void *data, size_t size, int timeout);

typedef int AwaitInputMethod (void *handle, int timeout);

typedef ssize_t ReadDataMethod (
  void *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

typedef int TellDeviceMethod (
  void *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size, int timeout
);

typedef int AskDeviceMethod (
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
  CloseHandleMethod *closeHandle;

  WriteDataMethod *writeData;
  AwaitInputMethod *awaitInput;
  ReadDataMethod *readData;

  TellDeviceMethod *tellDevice;
  AskDeviceMethod *askDevice;

  SetHidReportMethod *setHidReport;
  GetHidReportMethod *getHidReport;
  SetHidFeatureMethod *setHidFeature;
  GetHidFeatureMethod *getHidFeature;
} InputOutputMethods;

struct InputOutputEndpointStruct {
  void *handle;
  const InputOutputMethods *methods;
  int inputTimeout;
  int outputTimeout;
  const void *applicationData;
};

void
ioInitializeEndpointSpecification (InputOutputEndpointSpecification *specification) {
  specification->serial.parameters = NULL;
  specification->serial.inputTimeout = 100;
  specification->serial.outputTimeout = 0;
  specification->serial.applicationData = NULL;

  specification->usb.channelDefinitions = NULL;
  specification->usb.inputTimeout = 1000;
  specification->usb.outputTimeout = 1000;
  specification->usb.applicationData = NULL;

  specification->bluetooth.channelNumber = 0;
  specification->bluetooth.inputTimeout = 100;
  specification->bluetooth.outputTimeout = 0;
  specification->bluetooth.applicationData = NULL;
}

void
ioInitializeSerialParameters (SerialParameters *parameters) {
  parameters->baud = 9600;
  parameters->flowControl = SERIAL_FLOW_NONE;
  parameters->dataBits = 8;
  parameters->stopBits = 1;
  parameters->parity = SERIAL_PARITY_NONE;
}

static void
closeSerialHandle (void *handle) {
  serialCloseDevice(handle);
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
  .closeHandle = closeSerialHandle,

  .writeData = writeSerialData,
  .awaitInput = awaitSerialInput,
  .readData = readSerialData
};

static void
closeUsbHandle (void *handle) {
  usbCloseChannel(handle);
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
  ssize_t result = usbReapInput(channel->device,
                                channel->definition.inputEndpoint,
                                buffer, size,
                                initialTimeout, subsequentTimeout);

  if (result != -1) return result;
  if (errno == EAGAIN) return 0;
  return -1;
}

static int
tellUsbDevice (
  void *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size, int timeout
) {
  UsbChannel *channel = handle;

  return usbControlWrite(channel->device, recipient, type,
                         request, value, index, data, size, timeout);
}

static int
askUsbDevice (
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
  .closeHandle = closeUsbHandle,

  .writeData = writeUsbData,
  .awaitInput = awaitUsbInput,
  .readData = readUsbData,

  .tellDevice = tellUsbDevice,
  .askDevice = askUsbDevice,

  .setHidReport = setUsbHidReport,
  .getHidReport = getUsbHidReport,
  .setHidFeature = setUsbHidFeature,
  .getHidFeature = getUsbHidFeature
};

static void
closeBluetoothHandle (void *handle) {
  bthCloseConnection(handle);
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
  .closeHandle = closeBluetoothHandle,

  .writeData = writeBluetoothData,
  .awaitInput = awaitBluetoothInput,
  .readData = readBluetoothData
};

InputOutputEndpoint *
ioOpenEndpoint (
  const char *identifier,
  const InputOutputEndpointSpecification *specification
) {
  InputOutputEndpoint *endpoint;

  if ((endpoint = malloc(sizeof(*endpoint)))) {
    if (specification->serial.parameters) {
      if (isSerialDevice(&identifier)) {
        if ((endpoint->handle = serialOpenDevice(identifier))) {
          if (serialSetParameters(endpoint->handle, specification->serial.parameters)) {
            if (serialRestartDevice(endpoint->handle, specification->serial.parameters->baud)) {
              endpoint->methods = &serialMethods;
              endpoint->inputTimeout = specification->serial.inputTimeout;
              endpoint->outputTimeout = specification->serial.outputTimeout;
              endpoint->applicationData = specification->serial.applicationData;
              return endpoint;
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
              endpoint->inputTimeout = specification->usb.inputTimeout;
              endpoint->outputTimeout = specification->usb.outputTimeout;
          endpoint->applicationData = specification->usb.applicationData;
          return endpoint;
        }

        goto openFailed;
      }
    }

    if (specification->bluetooth.channelNumber) {
      if (isBluetoothDevice(&identifier)) {
        if ((endpoint->handle = bthOpenConnection(identifier, specification->bluetooth.channelNumber, 1))) {
          endpoint->methods = &bluetoothMethods;
              endpoint->inputTimeout = specification->bluetooth.inputTimeout;
              endpoint->outputTimeout = specification->bluetooth.outputTimeout;
          endpoint->applicationData = specification->bluetooth.applicationData;
          return endpoint;
        }

        goto openFailed;
      }
    }

    logMessage(LOG_WARNING, "unsupported device: %s", identifier);
  openFailed:
    free(endpoint);
  }

  return NULL;
}

static int
logNoMethod (const char *name) {
  errno = ENOSYS;
  logSystemError(name);
  return -1;
}

void
ioCloseEndpoint (InputOutputEndpoint *endpoint) {
  CloseHandleMethod *method = endpoint->methods->closeHandle;

  if (method) {
    method(endpoint->handle);
  } else {
    logNoMethod("ioCloseEndpoint");
  }

  free(endpoint);
}

const void *
ioGetApplicationData (InputOutputEndpoint *endpoint) {
  return endpoint->applicationData;
}

ssize_t
ioWriteData (InputOutputEndpoint *endpoint, const void *data, size_t size) {
  WriteDataMethod *method = endpoint->methods->writeData;
  if (!method) return logNoMethod("ioWriteData");
  return method(endpoint->handle, data, size, endpoint->outputTimeout);
}

int
ioAwaitInput (InputOutputEndpoint *endpoint, int timeout) {
  AwaitInputMethod *method = endpoint->methods->awaitInput;
  if (!method) return logNoMethod("ioAwaitInput");
  return method(endpoint->handle, timeout);
}

ssize_t
ioReadData (InputOutputEndpoint *endpoint, void *buffer, size_t size, int wait) {
  ReadDataMethod *method = endpoint->methods->readData;
  int timeout = endpoint->inputTimeout;

  if (!method) return logNoMethod("ioReadData");
  return method(endpoint->handle, buffer, size,
                (wait? timeout: 0), timeout);
}

int
ioReadByte (InputOutputEndpoint *endpoint, unsigned char *byte, int wait) {
  ssize_t result = ioReadData(endpoint, byte, 1, wait);
  if (result > 0) return 1;
  if (result == 0) errno = EAGAIN;
  return 0;
}

ssize_t
ioTellDevice (
  InputOutputEndpoint *endpoint, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size
) {
  TellDeviceMethod *method = endpoint->methods->tellDevice;
  if (!method) return logNoMethod("ioTellDevice");
  return method(endpoint->handle, recipient, type,
                request, value, index, data, size,
                endpoint->outputTimeout);
}

ssize_t
ioAskDevice (
  InputOutputEndpoint *endpoint, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size
) {
  AskDeviceMethod *method = endpoint->methods->askDevice;
  if (!method) return logNoMethod("ioAskDevice");
  return method(endpoint->handle, recipient, type,
                request, value, index, buffer, size,
                endpoint->inputTimeout);
}

ssize_t
ioSetHidReport (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  const void *data, uint16_t size
) {
  SetHidReportMethod *method = endpoint->methods->setHidReport;
  if (!method) return logNoMethod("ioSetHidReport");
  return method(endpoint->handle, interface, report,
                data, size, endpoint->outputTimeout);
}

ssize_t
ioGetHidReport (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  void *buffer, uint16_t size
) {
  GetHidReportMethod *method = endpoint->methods->getHidReport;
  if (!method) return logNoMethod("ioGetHidReport");
  return method(endpoint->handle, interface, report,
                buffer, size, endpoint->inputTimeout);
}

ssize_t
ioSetHidFeature (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  const void *data, uint16_t size
) {
  SetHidFeatureMethod *method = endpoint->methods->setHidFeature;
  if (!method) return logNoMethod("ioSetHidFeature");
  return method(endpoint->handle, interface, report,
                data, size, endpoint->outputTimeout);
}

ssize_t
ioGetHidFeature (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  void *buffer, uint16_t size
) {
  GetHidFeatureMethod *method = endpoint->methods->getHidFeature;
  if (!method) return logNoMethod("ioGetHidFeature");
  return method(endpoint->handle, interface, report,
                buffer, size, endpoint->inputTimeout);
}
