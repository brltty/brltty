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

static Queue *bluetoothConnectErrors = NULL;
typedef struct {
  uint64_t bda;
  int value;
} BluetoothConnectError;

static void
bthDeallocateConnectError (void *item, void *data) {
  BluetoothConnectError *error = item;
  free(error);
}

static int
bthInitializeConnectErrors (void) {
  if (!bluetoothConnectErrors) {
    if (!(bluetoothConnectErrors = newQueue(bthDeallocateConnectError, NULL))) {
      return 0;
    }
  }

  return 1;
}

static int
bthTestConnectError (const void *item, const void *data) {
  const BluetoothConnectError *error = item;
  const uint64_t *bda = data;
  return error->bda == *bda;
}

static BluetoothConnectError *
bthGetConnectError (uint64_t bda, int add) {
  BluetoothConnectError *error = findItem(bluetoothConnectErrors, bthTestConnectError, &bda);
  if (error) return error;

  if (add) {
    if ((error = malloc(sizeof(*error)))) {
      error->bda = bda;
      error->value = 0;
      if (enqueueItem(bluetoothConnectErrors, error)) return error;

      free(error);
    }
  }

  return NULL;
}

void
bthForgetConnectErrors (void) {
  if (bthInitializeConnectErrors()) deleteElements(bluetoothConnectErrors);
}

static int
bthRememberConnectError (uint64_t bda, int value) {
  if (bthInitializeConnectErrors()) {
    BluetoothConnectError *error = bthGetConnectError(bda, 1);

    if (error) {
      error->value = value;
      return 1;
    }
  }

  return 0;
}

static int
bthRecallConnectError (uint64_t bda, int *value) {
  if (bthInitializeConnectErrors()) {
    BluetoothConnectError *error = bthGetConnectError(bda, 0);

    if (error) {
      *value = error->value;
      return 1;
    }
  }

  return 0;
}

static void
bthForgetConnectError (uint64_t bda) {
  if (bthInitializeConnectErrors()) {
    Element *element = findElement(bluetoothConnectErrors, bthTestConnectError, &bda);
    if (element) deleteElement(element);
  }
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

char *
bthGetDeviceName (const char *address) {
  uint64_t bda;

  if (bthParseAddress(&bda, address)) {
    char *name = bthQueryDeviceName(bda);

    if (name) return name;
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
