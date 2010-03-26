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

#include <errno.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "log.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"
#include "io_misc.h"

struct BluetoothConnectionExtensionStruct {
  int socket;
  struct sockaddr_rc local;
  struct sockaddr_rc remote;
};

BluetoothConnectionExtension *
bthConnect (const BluetoothDeviceAddress *bda, unsigned char channel) {
  BluetoothConnectionExtension *bcx;

  if ((bcx = malloc(sizeof(*bcx)))) {
    memset(bcx, 0, sizeof(*bcx));

    if ((bcx->socket = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) != -1) {
      bcx->local.rc_family = AF_BLUETOOTH;
      bcx->local.rc_channel = 0;
      bacpy(&bcx->local.rc_bdaddr, BDADDR_ANY); /* Any HCI. No support for explicit
                                                 * interface specification yet.
                                                 */

      if (bind(bcx->socket, (struct sockaddr *)&bcx->local, sizeof(bcx->local)) != -1) {
        bcx->remote.rc_family = AF_BLUETOOTH;
        bcx->remote.rc_channel = channel;
        memcpy(bcx->remote.rc_bdaddr.b, bda->bytes, sizeof(bda->bytes));

        if (connect(bcx->socket, (struct sockaddr *)&bcx->remote, sizeof(bcx->remote)) != -1) {
          if (setBlockingIo(bcx->socket, 0)) {
            return bcx;
          }
        } else if ((errno != EHOSTDOWN) && (errno != EHOSTUNREACH)) {
          LogError("RFCOMM connect");
        }
      } else {
        LogError("RFCOMM bind");
      }

      close(bcx->socket);
    } else {
      LogError("RFCOMM socket");
    }

    free(bcx);
  } else {
    LogError("malloc");
  }

  return NULL;
}

void
bthDisconnect (BluetoothConnectionExtension *bcx) {
  close(bcx->socket);
  free(bcx);
}

int
bthAwaitInput (BluetoothConnection *connection, int milliseconds) {
  BluetoothConnectionExtension *bcx = connection->extension;

  return awaitInput(bcx->socket, milliseconds);
}

ssize_t
bthReadData (
  BluetoothConnection *connection, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  BluetoothConnectionExtension *bcx = connection->extension;

  return readData(bcx->socket, buffer, size, initialTimeout, subsequentTimeout);
}

ssize_t
bthWriteData (BluetoothConnection *connection, const void *buffer, size_t size) {
  BluetoothConnectionExtension *bcx = connection->extension;

  return writeData(bcx->socket, buffer, size);
}
