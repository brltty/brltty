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
#include "strfmt.h"
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
  static const uint8_t uuid[] = {
    0X00, 0X00, 0X11, 0X01,
    0X00, 0X00,
    0X10, 0X00,
    0X80, 0X00,
    0X00, 0X80, 0X5F, 0X9B, 0X34, 0XFB
  };

  logMessage(LOG_CATEGORY(BLUETOOTH_IO), "discovering serial port channel");
  int discovered = bthDiscoverChannel(channel, bcx, uuid, sizeof(uuid), timeout);

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
  uint64_t address;
  char *name;
  int error;
  unsigned paired:1;
} BluetoothDeviceEntry;

static void
bthDeallocateDeviceEntry (void *item, void *data) {
  BluetoothDeviceEntry *device = item;

  if (device->name) free(device->name);
  free(device);
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
bthTestDeviceAddress (const void *item, void *data) {
  const BluetoothDeviceEntry *device = item;
  const uint64_t *address = data;

  return device->address == *address;
}

static BluetoothDeviceEntry *
bthGetDeviceEntry (uint64_t address, int add) {
  Queue *devices = bthGetDeviceQueue(add);

  if (devices) {
    BluetoothDeviceEntry *device = findItem(devices, bthTestDeviceAddress, &address);
    if (device) return device;

    if (add) {
      if ((device = malloc(sizeof(*device)))) {
        device->address = address;
        device->name = NULL;
        device->error = 0;
        device->paired = 0;

        if (enqueueItem(devices, device)) return device;
        free(device);
      } else {
        logMallocError();
      }
    }
  }

  return NULL;
}

static int
bthSetDeviceName (BluetoothDeviceEntry *device, const char *name) {
  if (name && *name) {
    char *copy = strdup(name);

    if (copy) {
      if (device->name) free(device->name);
      device->name = copy;
      return 1;
    } else {
      logMallocError();
    }
  }

  return 0;
}

static inline const char *
bthGetPairedKeyword (int state) {
  return getFlagKeywordYesNo(state);
}

static int
bthRememberDiscoveredDevice (const DiscoveredBluetoothDevice *device, void *data) {
  logMessage(LOG_CATEGORY(BLUETOOTH_IO),
             "remember discovered device: "
             "Addr:%012" PRIX64 " Paired:%s Name:%s",
             device->address, bthGetPairedKeyword(device->paired), device->name);

  BluetoothDeviceEntry *entry = bthGetDeviceEntry(device->address, 1);

  if (entry) {
    bthSetDeviceName(entry, device->name);
    entry->paired = device->paired;
  }

  return 0;
}

static int bluetoothDevicesDiscovered = 0;

static void
bthDiscoverDevices (void) {
  if (!bluetoothDevicesDiscovered) {
    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "begin device discovery");
    bthProcessDiscoveredDevices(bthRememberDiscoveredDevice, NULL);
    bluetoothDevicesDiscovered = 1;
    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "end device discovery");
  }
}

void
bthForgetDevices (void) {
  Queue *devices = bthGetDeviceQueue(0);

  if (devices) deleteElements(devices);
  bluetoothDevicesDiscovered = 0;
}

static int
bthRememberConnectError (uint64_t address, int value) {
  BluetoothDeviceEntry *device = bthGetDeviceEntry(address, 1);
  if (!device) return 0;

  device->error = value;
  return 1;
}

static int
bthRecallConnectError (uint64_t address, int *value) {
  BluetoothDeviceEntry *device = bthGetDeviceEntry(address, 0);
  if (!device) return 0;
  if (!device->error) return 0;

  *value = device->error;
  return 1;
}

static BluetoothConnection *
bthNewConnection (uint64_t address, uint8_t channel, int discover, int timeout) {
  BluetoothConnection *connection;

  if ((connection = malloc(sizeof(*connection)))) {
    memset(connection, 0, sizeof(*connection));
    connection->address = address;
    connection->channel = channel;

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

    free(connection);
  } else {
    logMallocError();
  }

  return NULL;
}

void
bthInitializeConnectionRequest (BluetoothConnectionRequest *request) {
  memset(request, 0, sizeof(*request));
  request->driver = NULL;
  request->identifier = NULL;
  request->timeout = BLUETOOTH_CHANNEL_CONNECT_TIMEOUT;
  request->channel = 0;
  request->discover = 0;
}

typedef enum {
  BTH_CONN_ADDRESS,
  BTH_CONN_NAME,
  BTH_CONN_CHANNEL,
  BTH_CONN_DISCOVER,
  BTH_CONN_TIMEOUT
} BluetoothConnectionParameter;

static const char *const bthConnectionParameters[] = {
  "address",
  "name",
  "channel",
  "discover",
  "timeout",
  NULL
};

static char **
bthGetConnectionParameters (const char *identifier) {
  if (!identifier) identifier = "";
  return getDeviceParameters(bthConnectionParameters, identifier);
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

STR_BEGIN_FORMATTER(bthFormatAddress, uint64_t address)
  uint8_t bytes[6];
  size_t count = ARRAY_COUNT(bytes);
  unsigned int index = count;

  while (index > 0) {
    bytes[--index] = address & 0XFF;
    address >>= 8;
  }

  while (index < count) {
    if (index > 0) STR_PRINTF("%c", ':');
    STR_PRINTF("%02X", bytes[index++]);
  }
STR_END_FORMATTER

static const BluetoothNameEntry *
bthGetNameEntry (const char *name) {
  if (name && *name) {
    const BluetoothNameEntry *entry = bluetoothNameTable;

    while (entry->namePrefix) {
      if (strncmp(name, entry->namePrefix, strlen(entry->namePrefix)) == 0) {
        return entry;
      }

      entry += 1;
    }
  }

  return NULL;
}

typedef struct {
  struct {
    const char *address;
    size_t length;
  } name;

  struct {
    const char *address;
    size_t length;
  } driver;
} GetDeviceAddressData;

static int
bthTestDeviceName (const void *item, void *data) {
  const BluetoothDeviceEntry *device = item;
  const GetDeviceAddressData *gda = data;

  logMessage(LOG_CATEGORY(BLUETOOTH_IO),
             "testing device: Addr:%012" PRIX64 " Paired:%s Name:%s",
             device->address, bthGetPairedKeyword(device->paired), device->name
  );

  if (!device->paired) {
    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "not paired");
    return 0;
  }

  if (gda->name.length) {
    if (strncmp(device->name, gda->name.address, gda->name.length) != 0) {
      logMessage(LOG_CATEGORY(BLUETOOTH_IO), "ineligible name");
      return 0;
    }
  }

  const BluetoothNameEntry *name = bthGetNameEntry(device->name);
  if (!name) {
    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "unrecognized name");
    return 0;
  }

  if (gda->driver.length) {
    const char *const *code = name->driverCodes;

    while (*code) {
      if (strncmp(*code, gda->driver.address, gda->driver.length) == 0) {
        logMessage(LOG_CATEGORY(BLUETOOTH_IO), "found");
        return 1;
      }

      code += 1;
    }

    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "ineligible driver");
    return 0;
  }

  logMessage(LOG_CATEGORY(BLUETOOTH_IO), "driver not specified");
  return 0;
}

static int
bthGetDeviceAddress (uint64_t *address, char **parameters, const char *driver) {
  {
    const char *parameter = parameters[BTH_CONN_ADDRESS];

    if (parameter && *parameter) {
      return bthParseAddress(address, parameter);
    }
  }

  {
    bthDiscoverDevices();
    Queue *devices = bthGetDeviceQueue(0);

    if (devices) {
      const char *name = parameters[BTH_CONN_NAME];

      GetDeviceAddressData gda = {
        .name = {
          .address = name,
          .length = name? strlen(name): 0
        },

        .driver = {
          .address = driver,
          .length = driver? strlen(driver): 0
        }
      };

      if (gda.driver.length) {
        logMessage(LOG_CATEGORY(BLUETOOTH_IO), "begin device search");
        const BluetoothDeviceEntry *device = findItem(devices, bthTestDeviceName, &gda);
        logMessage(LOG_CATEGORY(BLUETOOTH_IO), "end device search");

        if (device) {
          *address = device->address;
          return 1;
        }
      }
    }
  }

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

BluetoothConnection *
bthOpenConnection (const BluetoothConnectionRequest *request) {
  BluetoothConnection *connection = NULL;
  BluetoothConnectionRequest req = *request;
  char **parameters = bthGetConnectionParameters(req.identifier);

  if (parameters) {
    {
      int ok = 1;

      if (!req.channel) req.discover = 1;
      if (!bthProcessChannelParameter(&req, parameters[BTH_CONN_CHANNEL])) ok = 0;
      if (!bthProcessDiscoverParameter(&req, parameters[BTH_CONN_DISCOVER])) ok = 0;
      if (!bthProcessTimeoutParameter(&req, parameters[BTH_CONN_TIMEOUT])) ok = 0;

      uint64_t address;
      if (!bthGetDeviceAddress(&address, parameters, req.driver)) ok = 0;

      if (ok) connection = bthNewConnection(address, req.channel, req.discover, req.timeout);
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

const char *
bthMakeConnectionIdentifier (BluetoothConnection *connection, char *buffer, size_t size) {
  size_t length;
  STR_BEGIN(buffer, size);
  STR_PRINTF("%s%c", BLUETOOTH_DEVICE_QUALIFIER, PARAMETER_QUALIFIER_CHARACTER);

  {
    uint64_t address = bthGetAddress(connection);
    STR_PRINTF("%s%c", bthConnectionParameters[BTH_CONN_ADDRESS], PARAMETER_ASSIGNMENT_CHARACTER);
    STR_FORMAT(bthFormatAddress, address);
    STR_PRINTF("%c", DEVICE_PARAMETER_SEPARATOR);
  }

  {
    uint8_t channel = bthGetChannel(connection);

    if (channel) {
      STR_PRINTF(
        "%s%c%u%c",
        bthConnectionParameters[BTH_CONN_CHANNEL],
        PARAMETER_ASSIGNMENT_CHARACTER,
        channel,
        DEVICE_PARAMETER_SEPARATOR
      );
    }
  }

  length = STR_LENGTH;
  STR_END;

  {
    char *last = &buffer[length] - 1;
    if (*last == DEVICE_PARAMETER_SEPARATOR) *last = 0;
  }

  return buffer;
}

uint64_t
bthGetAddress (BluetoothConnection *connection) {
  return connection->address;
}

uint8_t
bthGetChannel (BluetoothConnection *connection) {
  return connection->channel;
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
bthGetDeviceName (uint64_t address, int timeout) {
  bthDiscoverDevices();
  BluetoothDeviceEntry *device = bthGetDeviceEntry(address, 1);

  if (device) {
    if (!device->name) {
      logMessage(LOG_CATEGORY(BLUETOOTH_IO), "obtaining device name");

      if ((device->name = bthObtainDeviceName(address, timeout))) {
        logMessage(LOG_CATEGORY(BLUETOOTH_IO), "device name: %s", device->name);
      } else {
        logMessage(LOG_CATEGORY(BLUETOOTH_IO), "device name not obtained");
      }
    }

    return device->name;
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

static struct {
  const char **table;
  unsigned int count;
  unsigned int size;
} driverCodes = {
  .table = NULL,
  .count = 0,
  .size = 0
};

static int
bthFindDriverCode (unsigned int *position, const char *code) {
  int first = 0;
  int last = driverCodes.count - 1;

  while (first <= last) {
    unsigned int current = (first + last) / 2;
    int relation = strcmp(code, driverCodes.table[current]);

    if (relation < 0) {
      last = current - 1;
    } else if (relation > 0) {
      first = current + 1;
    } else {
      *position = current;
      return 1;
    }
  }

  *position = first;
  return 0;
}

static int
bthEnsureDriverCodeTableSize (void) {
  if (driverCodes.count < driverCodes.size) return 1;

  unsigned int newSize = driverCodes.size + 0X10;
  const char **newTable = realloc(driverCodes.table, ARRAY_SIZE(driverCodes.table, newSize));

  if (!newTable) {
    logMallocError();
    return 0;
  }

  driverCodes.table = newTable;
  driverCodes.size = newSize;
  return 1;
}

static int
bthAddDriverCode (const char *code) {
  unsigned int position;
  if (bthFindDriverCode(&position, code)) return 1;
  if (!bthEnsureDriverCodeTableSize()) return 0;

  memmove(&driverCodes.table[position+1], &driverCodes.table[position],
          ARRAY_SIZE(driverCodes.table, (driverCodes.count - position)));

  driverCodes.count += 1;
  driverCodes.table[position] = code;
  return 1;
}

static const char *const *
bthGetAllDriverCodes (void) {
  if (driverCodes.table) return driverCodes.table;
  const BluetoothNameEntry *entry = bluetoothNameTable;

  while (entry->namePrefix) {
    const char *const *code = entry->driverCodes;

    while (*code) {
      if (!bthAddDriverCode(*code)) goto failure;
      code += 1;
    }

    entry += 1;
  }

  if (bthEnsureDriverCodeTableSize()) {
    driverCodes.table[driverCodes.count] = NULL;
    return driverCodes.table;
  }

failure:
  if (driverCodes.table) {
    free(driverCodes.table);
    memset(&driverCodes, 0, sizeof(driverCodes));
  }

  static const char *const *noDriverCodes = {NULL};
  return noDriverCodes;
}

const char *const *
bthGetDriverCodes (const char *identifier, int timeout) {
  const char *const *codes = NULL;
  char **parameters = bthGetConnectionParameters(identifier);

  if (parameters) {
    uint64_t address;

    if (bthGetDeviceAddress(&address, parameters, NULL)) {
      const char *name = bthGetDeviceName(address, timeout);
      const BluetoothNameEntry *entry = bthGetNameEntry(name);
      if (entry) codes = entry->driverCodes;
    }

    deallocateStrings(parameters);
  }

  if (!codes) codes = bthGetAllDriverCodes();
  return codes;
}

int
isBluetoothDeviceIdentifier (const char **identifier) {
  return hasQualifier(identifier, BLUETOOTH_DEVICE_QUALIFIER);
}
