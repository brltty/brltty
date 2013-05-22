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
#include "device.h"
#include "queue.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"

static Queue *bluetoothDeviceQueue = NULL;
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

static int
bthInitializeDeviceQueue (void) {
  if (!bluetoothDeviceQueue) {
    if (!(bluetoothDeviceQueue = newQueue(bthDeallocateDeviceEntry, NULL))) {
      return 0;
    }
  }

  return 1;
}

static int
bthTestDeviceEntry (const void *item, const void *data) {
  const BluetoothDeviceEntry *entry = item;
  const uint64_t *bda = data;

  return entry->bda == *bda;
}

static BluetoothDeviceEntry *
bthGetDeviceEntry (uint64_t bda, int add) {
  if (bthInitializeDeviceQueue()) {
    BluetoothDeviceEntry *entry = findItem(bluetoothDeviceQueue, bthTestDeviceEntry, &bda);
    if (entry) return entry;

    if (add) {
      if ((entry = malloc(sizeof(*entry)))) {
        entry->bda = bda;
        entry->connectError = 0;
        entry->deviceName = NULL;

        if (enqueueItem(bluetoothDeviceQueue, entry)) return entry;
        free(entry);
      } else {
        logMallocError();
      }
    }
  }

  return NULL;
}

void
bthClearCache (void) {
  if (bthInitializeDeviceQueue()) deleteElements(bluetoothDeviceQueue);
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

  *value = entry->connectError;
  return 1;
}

static void
bthForgetConnectError (uint64_t bda) {
  BluetoothDeviceEntry *entry = bthGetDeviceEntry(bda, 0);
  if (entry) entry->connectError = 0;
}

static int
bthParseAddress (uint64_t *bda, const char *address) {
  const char *character = address;
  const unsigned int width = 8;
  const unsigned long mask = (1UL << width) - 1UL;
  int counter = BDA_SIZE;
  *bda = 0;

  do {
    *bda <<= width;

    if (*character) {
      char *end;
      long int value = strtol(character, &end, 0X10);

      if (end == character) goto error;
      if (value < 0) goto error;
      if (value > mask) goto error;
      *bda |= value;

      if (!*(character = end)) continue;
      if (*character != ':') goto error;
      if (!*++character) goto error;
    }
  } while (--counter);

  if (!*character) return 1;

error:
  logMessage(LOG_ERR, "invalid Bluetooth device address: %s", address);
  errno = EINVAL;
  return 0;
}

BluetoothConnection *
bthOpenConnection (const char *address, uint8_t channel, int force) {
  BluetoothConnection *connection;

  if ((connection = malloc(sizeof(*connection)))) {
    memset(connection, 0, sizeof(*connection));
    connection->channel = channel;

    if (bthParseAddress(&connection->address, address)) {
      int alreadyTried = 0;

      if (force) {
        bthForgetConnectError(connection->address);
      } else {
        int value;

        if (bthRecallConnectError(connection->address, &value)) {
          errno = value;
          alreadyTried = 1;
        }
      }

      if (!alreadyTried) {
        TimePeriod period;
        startTimePeriod(&period, 2000);

	while (1) {
          if ((connection->extension = bthConnect(connection->address, connection->channel))) return connection;
	  if (afterTimePeriod(&period, NULL)) break;
	  if (errno != EBUSY) break;
	  approximateDelay(100);
	}

        bthRememberConnectError(connection->address, errno);
      }
    }

    free(connection);
  } else {
    logMallocError();
  }

  return NULL;
}

void
bthCloseConnection (BluetoothConnection *connection) {
  bthDisconnect(connection->extension);
  free(connection);
}

const char *
bthGetDeviceName (const char *address) {
  uint64_t bda;

  if (bthParseAddress(&bda, address)) {
    BluetoothDeviceEntry *entry = bthGetDeviceEntry(bda, 1);

    if (entry) {
      if (!entry->deviceName) {
        entry->deviceName = bthQueryDeviceName(bda);
      }

      return entry->deviceName;
    }
  }

  return NULL;
}

int
isBluetoothDevice (const char **identifier) {
  if (isQualifiedDevice(identifier, "bluetooth")) return 1;
  if (isQualifiedDevice(identifier, "bt")) return 1;
  if (isQualifiedDevice(identifier, "bluez")) return 1;
  return 0;
}

typedef struct {
  const char *namePrefix;
  const char *const *driverCodes;
} BluetoothNameEntry;

#define BLUETOOTH_NAME_DRIVERS(name, ...) static const char *const bluetoothNameDrivers_##name[] = {__VA_ARGS__, NULL}
BLUETOOTH_NAME_DRIVERS(ActiveBraille, "ht");
BLUETOOTH_NAME_DRIVERS(AlvaBC, "al");
BLUETOOTH_NAME_DRIVERS(BasicBraille, "ht");
BLUETOOTH_NAME_DRIVERS(BrailleEdge, "hm");
BLUETOOTH_NAME_DRIVERS(BrailleSense, "hm");
BLUETOOTH_NAME_DRIVERS(BrailleStar, "ht");
BLUETOOTH_NAME_DRIVERS(BrailliantBI, "hw");
BLUETOOTH_NAME_DRIVERS(Focus, "fs");
BLUETOOTH_NAME_DRIVERS(TSM, "sk");

static const BluetoothNameEntry bluetoothNameTable[] = {
  { .namePrefix = "Active Braille",
    .driverCodes = bluetoothNameDrivers_ActiveBraille
  },

  { .namePrefix = "ALVA BC",
    .driverCodes = bluetoothNameDrivers_AlvaBC
  },

  { .namePrefix = "Basic Braille",
    .driverCodes = bluetoothNameDrivers_BasicBraille
  },

  { .namePrefix = "BrailleEDGE",
    .driverCodes = bluetoothNameDrivers_BrailleEdge
  },

  { .namePrefix = "BrailleSense",
    .driverCodes = bluetoothNameDrivers_BrailleSense
  },

  { .namePrefix = "Braille Star",
    .driverCodes = bluetoothNameDrivers_BrailleStar
  },

  { .namePrefix = "Brailliant BI",
    .driverCodes = bluetoothNameDrivers_BrailliantBI
  },

  { .namePrefix = "Focus",
    .driverCodes = bluetoothNameDrivers_Focus
  },

  { .namePrefix = "TSM",
    .driverCodes = bluetoothNameDrivers_TSM
  },

  { .namePrefix = NULL }
};
