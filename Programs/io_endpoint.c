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

#include "io_endpoint.h"
#include "io_serial.h"
#include "io_usb.h"
#include "io_bluetooth.h"

typedef struct {
  void (*closeHandle) (void *handle);

  int (*awaitInput) (void *handle, int timeout);
  ssize_t (*readData) (
    void *handle, void *buffer, size_t size,
    int initialTimeout, int subsequentTimeout
  );
  ssize_t (*writeData) (void *handle, const void *data, size_t size, int timeout);

  int (*tellDevice) (
    void *handle, uint8_t recipient, uint8_t type,
    uint8_t request, uint16_t value, uint16_t index,
    const void *data, uint16_t size, int timeout
  );

  int (*askDevice) (
    void *handle, uint8_t recipient, uint8_t type,
    uint8_t request, uint16_t value, uint16_t index,
    void *buffer, uint16_t size, int timeout
  );
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
  specification->usb.inputTimeout = 100;
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
  parameters->flow = SERIAL_FLOW_NONE;
  parameters->data = 8;
  parameters->stop = 1;
  parameters->parity = SERIAL_PARITY_NONE;
}

static void
closeSerialHandle (void *handle) {
  serialCloseDevice(handle);
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

static ssize_t
writeSerialData (void *handle, const void *data, size_t size, int timeout) {
  return serialWriteData(handle, data, size);
}

static const InputOutputMethods serialMethods = {
  .closeHandle = closeSerialHandle,
  .awaitInput = awaitSerialInput,
  .readData = readSerialData,
  .writeData = writeSerialData
};

static void
closeUsbHandle (void *handle) {
  usbCloseChannel(handle);
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
writeUsbData (void *handle, const void *data, size_t size, int timeout) {
  UsbChannel *channel = handle;

  return usbWriteEndpoint(channel->device,
                          channel->definition.outputEndpoint,
                          data, size, timeout);
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

static const InputOutputMethods usbMethods = {
  .closeHandle = closeUsbHandle,
  .awaitInput = awaitUsbInput,
  .readData = readUsbData,
  .writeData = writeUsbData,
  .tellDevice = tellUsbDevice,
  .askDevice = askUsbDevice
};

static void
closeBluetoothHandle (void *handle) {
  bthCloseConnection(handle);
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

static ssize_t
writeBluetoothData (void *handle, const void *data, size_t size, int timeout) {
  return bthWriteData(handle, data, size);
}

static const InputOutputMethods bluetoothMethods = {
  .closeHandle = closeBluetoothHandle,
  .awaitInput = awaitBluetoothInput,
  .readData = readBluetoothData,
  .writeData = writeBluetoothData
};

InputOutputEndpoint *
ioOpenEndpoint (
  const char *path,
  const InputOutputEndpointSpecification *specification
) {
  InputOutputEndpoint *endpoint;

  if ((endpoint = malloc(sizeof(*endpoint)))) {
    if (specification->serial.parameters) {
      if (isSerialDevice(&path)) {
        if ((endpoint->handle = serialOpenDevice(path))) {
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
      if (isUsbDevice(&path)) {
        if ((endpoint->handle = usbFindChannel(specification->usb.channelDefinitions, path))) {
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
      if (isBluetoothDevice(&path)) {
        if ((endpoint->handle = bthOpenConnection(path, specification->bluetooth.channelNumber, 1))) {
          endpoint->methods = &bluetoothMethods;
              endpoint->inputTimeout = specification->bluetooth.inputTimeout;
              endpoint->outputTimeout = specification->bluetooth.outputTimeout;
          endpoint->applicationData = specification->bluetooth.applicationData;
          return endpoint;
        }

        goto openFailed;
      }
    }

  openFailed:
    free(endpoint);
  }

  return NULL;
}

void
ioCloseEndpoint (InputOutputEndpoint *endpoint) {
  if (endpoint->handle) endpoint->methods->closeHandle(endpoint->handle);
  free(endpoint);
}

const void *
ioGetApplicationData (InputOutputEndpoint *endpoint) {
  return endpoint->applicationData;
}

int
ioAwaitInput (InputOutputEndpoint *endpoint, int timeout) {
  return endpoint->methods->awaitInput(endpoint->handle, timeout);
}

ssize_t
ioReadData (InputOutputEndpoint *endpoint, void *buffer, size_t size, int wait) {
  int timeout = endpoint->inputTimeout;

  return endpoint->methods->readData(endpoint->handle, buffer, size,
                                     (wait? timeout: 0), timeout);
}

ssize_t
ioWriteData (InputOutputEndpoint *endpoint, const void *data, size_t size) {
  return endpoint->methods->writeData(endpoint->handle, data, size, endpoint->outputTimeout);
}

ssize_t
ioTellDevice (
  InputOutputEndpoint *endpoint, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size
) {
  return endpoint->methods->tellDevice(endpoint->handle, recipient, type,
                                       request, value, index, data, size,
                                       endpoint->outputTimeout);
}

ssize_t
ioAskDevice (
  InputOutputEndpoint *endpoint, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size
) {
  return endpoint->methods->askDevice(endpoint->handle, recipient, type,
                                      request, value, index, buffer, size,
                                      endpoint->outputTimeout);
}
