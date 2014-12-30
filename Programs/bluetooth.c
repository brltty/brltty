/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include "parameters.h"
#include "timing.h"
#include "async_wait.h"
#include "parse.h"
#include "device.h"
#include "queue.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"

static int
bthDiscoverSerialPortChannel (uint8_t *channel, BluetoothConnectionExtension *bcx, int timeout) {
  int discovered;

  static const uint8_t uuid[] = {
    0X00, 0X00, 0X11, 0X01,
    0X00, 0X00,
    0X10, 0X00,
    0X80, 0X00,
    0X00, 0X80, 0X5F, 0X9B, 0X34, 0XFB
  };

  logMessage(LOG_CATEGORY(BLUETOOTH_IO), "discovering serial port channel");
  discovered = bthDiscoverChannel(channel, bcx, uuid, sizeof(uuid), timeout);

  if (discovered) {
    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "serial port channel discovered: %u", *channel);
  } else {
    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "serial port channel not discovered");
  }

  return discovered;
}

static void
bthLogChannel (uint8_t channel) {
  logMessage(LOG_CATEGORY(BLUETOOTH_IO), "RFCOMM channel: %u", channel);
}

typedef struct {
  uint64_t bda;
  int connectError;
  char *deviceName;
} BluetoothDeviceEntry;

static void
bthDeallocateDeviceEntry (void *item, void *data) {
  BluetoothDeviceEntry *entry = item;

  if (entry->deviceName) free(entry->deviceName);
  free(entry);
}

static Queue *
bthCreateDeviceQueue (void *data) {
  return newQueue(bthDeallocateDeviceEntry, NULL);
}

static Queue *
bthGetDeviceQueue (int create) {
  static Queue *devices = NULL;

  return getProgramQueue(&devices, "bluetooth-device-queue", create,
                         bthCreateDeviceQueue, NULL);
}

static int
bthTestDeviceEntry (const void *item, void *data) {
  const BluetoothDeviceEntry *entry = item;
  const uint64_t *bda = data;

  return entry->bda == *bda;
}

static BluetoothDeviceEntry *
bthGetDeviceEntry (uint64_t bda, int add) {
  Queue *devices = bthGetDeviceQueue(add);

  if (devices) {
    BluetoothDeviceEntry *entry = findItem(devices, bthTestDeviceEntry, &bda);
    if (entry) return entry;

    if (add) {
      if ((entry = malloc(sizeof(*entry)))) {
        entry->bda = bda;
        entry->connectError = 0;
        entry->deviceName = NULL;

        if (enqueueItem(devices, entry)) return entry;
        free(entry);
      } else {
        logMallocError();
      }
    }
  }

  return NULL;
}

void
bthForgetDevices (void) {
  Queue *devices = bthGetDeviceQueue(0);

  if (devices) deleteElements(devices);
}

static int
bthRememberConnectError (uint64_t bda, int value) {
  BluetoothDeviceEntry *entry = bthGetDeviceEntry(bda, 1);
  if (!entry) return 0;

  entry->connectError = value;
  return 1;
}

static int
bthRecallConnectError (uint64_t bda, int *value) {
  BluetoothDeviceEntry *entry = bthGetDeviceEntry(bda, 0);
  if (!entry) return 0;
  if (!entry->connectError) return 0;

  *value = entry->connectError;
  return 1;
}

static BluetoothConnection *
bthNewConnection (const char *address, uint8_t channel, int discover, int timeout) {
  BluetoothConnection *connection;

  if ((connection = malloc(sizeof(*connection)))) {
    memset(connection, 0, sizeof(*connection));
    connection->channel = channel;

    if (bthParseAddress(&connection->address, address)) {
      if ((connection->extension = bthNewConnectionExtension(connection->address))) {
        int alreadyTried = 0;

        if (discover) bthDiscoverSerialPortChannel(&connection->channel, connection->extension, timeout);
        bthLogChannel(connection->channel);

        {
          int value;

          if (bthRecallConnectError(connection->address, &value)) {
            errno = value;
            alreadyTried = 1;
          }
        }

        if (!alreadyTried) {
          TimePeriod period;
          startTimePeriod(&period, BLUETOOTH_CHANNEL_BUSY_RETRY_TIMEOUT);

          while (1) {
            if (bthOpenChannel(connection->extension, connection->channel, timeout)) {
              return connection;
            }

            if (afterTimePeriod(&period, NULL)) break;
            if (errno != EBUSY) break;
            asyncWait(BLUETOOTH_CHANNEL_BUSY_RETRY_INTERVAL);
          }

          bthRememberConnectError(connection->address, errno);
        }

        bthReleaseConnectionExtension(connection->extension);
      }
    }

    free(connection);
  } else {
    logMallocError();
  }

  return NULL;
}

void
bthInitializeConnectionRequest (BluetoothConnectionRequest *request) {
  memset(request, 0, sizeof(*request));
  request->identifier = NULL;
  request->timeout = BLUETOOTH_CHANNEL_CONNECT_TIMEOUT;
  request->channel = 0;
  request->discover = 0;
}

int
bthParseAddress (uint64_t *address, const char *string) {
  const char *character = string;
  unsigned int counter = BDA_SIZE;
  char delimiter = 0;
  *address = 0;

  while (*character) {
    long unsigned int value;

    {
      const char *start = character;
      char *end;

      value = strtoul(character, &end, 0X10);
      if ((end - start) != 2) break;
      character = end;
    }

    if (value > UINT8_MAX) break;
    *address <<= 8;
    *address |= value;

    if (!--counter) {
      if (*character) break;
      return 1;
    }

    if (!*character) break;

    if (!delimiter) {
      delimiter = *character;
      if ((delimiter != ':') && (delimiter != '-')) break;
    } else if (*character != delimiter) {
      break;
    }

    character += 1;
  }

  logMessage(LOG_ERR, "invalid Bluetooth device address: %s", string);
  errno = EINVAL;
  return 0;
}

int
bthParseChannelNumber (uint8_t *channel, const char *string) {
  if (*string) {
    unsigned int value;

    if (isUnsignedInteger(&value, string)) {
      if ((value > 0) && (value < 0X1F)) {
        *channel = value;
        return 1;
      }
    }
  }

  logMessage(LOG_WARNING, "invalid RFCOMM channel number: %s", string);
  return 0;
}

static int
bthProcessTimeoutParameter (BluetoothConnectionRequest *request, const char *parameter) {
  int seconds;

  if (!parameter) return 1;
  if (!*parameter) return 1;

  if (isInteger(&seconds, parameter)) {
    if ((seconds > 0) && (seconds < 60)) {
      request->timeout = seconds * MSECS_PER_SEC;
      return 1;
    }
  }

  logMessage(LOG_ERR, "invalid Bluetooth connection timeout: %s", parameter);
  return 0;
}

static int
bthProcessChannelParameter (BluetoothConnectionRequest *request, const char *parameter) {
  uint8_t channel;

  if (!parameter) return 1;
  if (!*parameter) return 1;

  if (bthParseChannelNumber(&channel, parameter)) {
    request->channel = channel;
    request->discover = 0;
    return 1;
  }

  return 0;
}

static int
bthProcessDiscoverParameter (BluetoothConnectionRequest *request, const char *parameter) {
  unsigned int flag;

  if (!parameter) return 1;
  if (!*parameter) return 1;

  if (validateYesNo(&flag, parameter)) {
    request->discover = flag;
    return 1;
  }

  logMessage(LOG_ERR, "invalid discover option: %s", parameter);
  return 0;
}

typedef enum {
  BTH_CONN_ADDRESS,
  BTH_CONN_TIMEOUT,
  BTH_CONN_CHANNEL,
  BTH_CONN_DISCOVER
} BluetoothConnectionParameter;

static char **
bthGetConnectionParameters (const char *identifier) {
  static const char *const names[] = {
    "address",
    "timeout",
    "channel",
    "discover",
    NULL
  };

  if (!identifier) identifier = "";
  return getDeviceParameters(names, identifier);
}

BluetoothConnection *
bthOpenConnection (const BluetoothConnectionRequest *request) {
  BluetoothConnection *connection = NULL;
  BluetoothConnectionRequest req = *request;
  char **parameters = bthGetConnectionParameters(req.identifier);

  if (parameters) {
    {
      int ok = 1;

      if (!req.channel) req.discover = 1;
      if (!bthProcessTimeoutParameter(&req, parameters[BTH_CONN_TIMEOUT])) ok = 0;
      if (!bthProcessChannelParameter(&req, parameters[BTH_CONN_CHANNEL])) ok = 0;
      if (!bthProcessDiscoverParameter(&req, parameters[BTH_CONN_DISCOVER])) ok = 0;

      if (ok) connection = bthNewConnection(parameters[BTH_CONN_ADDRESS], req.channel, req.discover, req.timeout);
    }

    deallocateStrings(parameters);
  }

  return connection;
}

void
bthCloseConnection (BluetoothConnection *connection) {
  bthReleaseConnectionExtension(connection->extension);
  free(connection);
}

int
bthAwaitInput (BluetoothConnection *connection, int timeout) {
  return bthPollInput(connection->extension, timeout);
}

ssize_t
bthReadData (
  BluetoothConnection *connection, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  ssize_t result = bthGetData(connection->extension, buffer, size, initialTimeout, subsequentTimeout);

  if (result > 0) logBytes(LOG_CATEGORY(BLUETOOTH_IO), "input", buffer, result);
  return result;
}

ssize_t
bthWriteData (BluetoothConnection *connection, const void *buffer, size_t size) {
  if (size > 0) logBytes(LOG_CATEGORY(BLUETOOTH_IO), "output", buffer, size);
  return bthPutData(connection->extension, buffer, size);
}

static char *
bthGetDeviceName (uint64_t bda, int timeout) {
  BluetoothDeviceEntry *entry = bthGetDeviceEntry(bda, 1);

  if (entry) {
    if (!entry->deviceName) {
      logMessage(LOG_CATEGORY(BLUETOOTH_IO), "obtaining device name");

      if ((entry->deviceName = bthObtainDeviceName(bda, timeout))) {
        logMessage(LOG_CATEGORY(BLUETOOTH_IO), "device name: %s", entry->deviceName);
      } else {
        logMessage(LOG_CATEGORY(BLUETOOTH_IO), "device name not obtained");
      }
    }

    return entry->deviceName;
  }

  return NULL;
}

char *
bthGetNameOfDevice (BluetoothConnection *connection, int timeout) {
  return bthGetDeviceName(connection->address, timeout);
}

char *
bthGetNameAtAddress (const char *address, int timeout) {
  uint64_t bda;

  if (bthParseAddress(&bda, address)) {
    return bthGetDeviceName(bda, timeout);
  }

  return NULL;
}

const char *const *
bthGetDriverCodes (const char *identifier, int timeout) {
  const char *const *codes = NULL;
  char **parameters = bthGetConnectionParameters(identifier);

  if (parameters) {
    const char *name = bthGetNameAtAddress(parameters[BTH_CONN_ADDRESS], timeout);

    if (name) {
      const BluetoothNameEntry *entry = bluetoothNameTable;

      while (entry->namePrefix) {
        if (strncmp(name, entry->namePrefix, strlen(entry->namePrefix)) == 0) {
          codes = entry->driverCodes;
          break;
        }

        entry += 1;
      }
    }

    deallocateStrings(parameters);
  }

  return codes;
}

int
isBluetoothDevice (const char **identifier) {
  return isQualifiedDevice(identifier, "bluetooth");
}
