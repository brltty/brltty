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
  const unsigned int width = 8;
  const unsigned long mask = (1UL << width) - 1UL;
  int counter = BDA_SIZE;
  *bda = 0;

  do {
    *bda <<= width;

    if (*address) {
      char *end;
      long int value = strtol(address, &end, 0X10);

      if (end == address) return 0;
      if (value < 0) return 0;
      if (value > mask) return 0;
      *bda |= value;

      if (!*(address = end)) continue;
      if (*address != ':') return 0;
      if (!*++address) return 0;
    }
  } while (--counter);

  return !*address;
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
    } else {
      logMessage(LOG_ERR, "invalid Bluetooth device address: %s", address);
      errno = EINVAL;
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

int
isBluetoothDevice (const char **identifier) {
  if (isQualifiedDevice(identifier, "bluetooth")) return 1;
  if (isQualifiedDevice(identifier, "bt")) return 1;
  if (isQualifiedDevice(identifier, "bluez")) return 1;
  return 0;
}
