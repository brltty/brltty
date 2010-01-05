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

#include <strings.h>
#include <errno.h>

#include "misc.h"
#include "queue.h"
#include "io_misc.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"

static Queue *btConnectErrors = NULL;
typedef struct {
  bdaddr_t address;
  int value;
} BluetoothConnectError;

static void
btDeallocateConnectError (void *item, void *data) {
  BluetoothConnectError *error = item;
  free(error);
}

static int
btInitializeConnectErrors (void) {
  if (!btConnectErrors) {
    if (!(btConnectErrors = newQueue(btDeallocateConnectError, NULL))) {
      return 0;
    }
  }

  return 1;
}

static int
btTestConnectError (void *item, void *data) {
  const BluetoothConnectError *error = item;
  const bdaddr_t *address = data;
  return memcmp(&error->address, address, sizeof(*address)) == 0;
}

static BluetoothConnectError *
btGetConnectError (bdaddr_t address, int add) {
  BluetoothConnectError *error = findItem(btConnectErrors, btTestConnectError, &address);
  if (error) return error;

  if (add) {
    if ((error = malloc(sizeof(*error)))) {
      error->address = address;
      error->value = 0;
      if (enqueueItem(btConnectErrors, error)) return error;

      free(error);
    }
  }

  return NULL;
}

void
btForgetConnectErrors (void) {
  if (btInitializeConnectErrors()) deleteElements(btConnectErrors);
}

static int
btRememberConnectError (bdaddr_t address, int value) {
  if (btInitializeConnectErrors()) {
    BluetoothConnectError *error = btGetConnectError(address, 1);

    if (error) {
      error->value = value;
      return 1;
    }
  }

  return 0;
}

static int
btRecallConnectError (bdaddr_t address, int *value) {
  if (btInitializeConnectErrors()) {
    BluetoothConnectError *error = btGetConnectError(address, 0);

    if (error) {
      *value = error->value;
      return 1;
    }
  }

  return 0;
}

static void
btForgetConnectError (bdaddr_t address) {
  if (btInitializeConnectErrors()) {
    Element *element = processQueue(btConnectErrors, btTestConnectError, &address);
    if (element) deleteElement(element);
  }
}

static int
btParseAddress (bdaddr_t *address, const char *string) {
  const char *start = string;
  int index = sizeof(address->b);
  while (--index >= 0) {
    char *end;
    long int value = strtol(start, &end, 0X10);
    if (end == start) return 0;
    if (value < 0) return 0;
    if (value > 0XFF) return 0;
    address->b[index] = value;
    if (!*end) break;
    if (*end != ':') return 0;
    start = end + 1;
  }
  if (index < 0) return 0;
  while (--index >= 0) address->b[index] = 0;
  return 1;
}

int
btOpenConnection (const char *address, unsigned char channel, int force) {
  bdaddr_t bda;
  if (btParseAddress(&bda, address)) {
    if (force) {
      btForgetConnectError(bda);
    } else {
      int value;
      if (btRecallConnectError(bda, &value)) {
        errno = value;
        return -1;
      }
    }

    {
      int connection = btConnect(bda, channel);
      if (connection != -1) {
        if (setBlockingIo(connection, 0)) return connection;
        close(connection);
      }
      btRememberConnectError(bda, errno);
    }
  } else {
    LogPrint(LOG_ERR, "Invalid Bluetooth address: %s", address);
    errno = EINVAL;
  }
  return -1;
}

int
isBluetoothDevice (const char **path) {
  if (isQualifiedDevice(path, "bluetooth:")) return 1;
  if (isQualifiedDevice(path, "bt:")) return 1;
  if (isQualifiedDevice(path, "bluez:")) return 1;
  return 0;
}
