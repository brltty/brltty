/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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
#include "device.h"
#include "queue.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"

static Queue *bluetoothConnectErrors = NULL;
typedef struct {
  BluetoothDeviceAddress bda;
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
  const BluetoothDeviceAddress *bda = data;
  return memcmp(error->bda.bytes, bda->bytes, sizeof(bda->bytes)) == 0;
}

static BluetoothConnectError *
bthGetConnectError (const BluetoothDeviceAddress *bda, int add) {
  BluetoothConnectError *error = findItem(bluetoothConnectErrors, bthTestConnectError, bda);
  if (error) return error;

  if (add) {
    if ((error = malloc(sizeof(*error)))) {
      error->bda = *bda;
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
bthRememberConnectError (const BluetoothDeviceAddress *bda, int value) {
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
bthRecallConnectError (const BluetoothDeviceAddress *bda, int *value) {
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
bthForgetConnectError (const BluetoothDeviceAddress *bda) {
  if (bthInitializeConnectErrors()) {
    Element *element = findElement(bluetoothConnectErrors, bthTestConnectError, bda);
    if (element) deleteElement(element);
  }
}

static int
bthParseAddress (BluetoothDeviceAddress *bda, const char *address) {
  const char *start = address;
  int index = sizeof(bda->bytes);

  while (--index >= 0) {
    char *end;
    long int value = strtol(start, &end, 0X10);

    if (end == start) return 0;
    if (value < 0) return 0;
    if (value > 0XFF) return 0;

    bda->bytes[index] = value;
    if (!*end) break;
    if (*end != ':') return 0;
    start = end + 1;
  }

  if (index < 0) return 0;
  while (--index >= 0) bda->bytes[index] = 0;

  return 1;
}

BluetoothConnection *
bthOpenConnection (const char *address, unsigned char channel, int force) {
  BluetoothConnection *connection;

  if ((connection = malloc(sizeof(*connection)))) {
    memset(connection, 0, sizeof(*connection));
    connection->channel = channel;

    if (bthParseAddress(&connection->address, address)) {
      int alreadyTried = 0;

      if (force) {
        bthForgetConnectError(&connection->address);
      } else {
        int value;

        if (bthRecallConnectError(&connection->address, &value)) {
          errno = value;
          alreadyTried = 1;
        }
      }

      if (!alreadyTried) {
        if ((connection->extension = bthConnect(&connection->address, connection->channel))) return connection;
        bthRememberConnectError(&connection->address, errno);
      }
    } else {
      LogPrint(LOG_ERR, "invalid Bluetooth device address: %s", address);
      errno = EINVAL;
    }

    free(connection);
  } else {
    LogError("malloc");
  }

  return NULL;
}

void
bthCloseConnection (BluetoothConnection *connection) {
  bthDisconnect(connection->extension);
  free(connection);
}

int
isBluetoothDevice (const char **path) {
  if (isQualifiedDevice(path, "bluetooth:")) return 1;
  if (isQualifiedDevice(path, "bt:")) return 1;
  if (isQualifiedDevice(path, "bluez:")) return 1;
  return 0;
}
