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

#include <winsock2.h>
#include <ws2bth.h>

#include "io_bluetooth.h"
#include "bluetooth_internal.h"
#include "log.h"
#include "misc.h"

struct BluetoothConnectionExtensionStruct {
  SOCKET socket;
  SOCKADDR_BTH local;
  SOCKADDR_BTH remote;
};

static void
bthSocketError (const char *action) {
  DWORD error = WSAGetLastError();
  LogWindowsCodeError(error, action);
  setErrno(error);
}

BluetoothConnectionExtension *
bthConnect (const BluetoothDeviceAddress *bda, unsigned char channel) {
  BluetoothConnectionExtension *bcx;

  if ((bcx = malloc(sizeof(*bcx)))) {
    memset(bcx, 0, sizeof(*bcx));

    if ((bcx->socket = socket(PF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)) != -1) {
      bcx->local.addressFamily = AF_BTH;
      bcx->local.btAddr = BTH_ADDR_NULL;
      bcx->local.port = 0;

      if (bind(bcx->socket, (SOCKADDR *)&bcx->local, sizeof(bcx->local)) != -1) {
        bcx->remote.addressFamily = AF_BTH;
        memcpy(&bcx->remote.btAddr, bda->bytes, sizeof(bda->bytes));
        bcx->remote.port = channel;

        if (connect(bcx->socket, (SOCKADDR *)&bcx->remote, sizeof(bcx->remote)) != -1) {
          return bcx;
        } else {
          DWORD error = WSAGetLastError();

          if ((error != ERROR_HOST_DOWN) && (error != ERROR_HOST_UNREACHABLE)) {
            LogWindowsCodeError(error, "RFCOMM connect");
          }

          setErrno(error);
        }
      } else {
        bthSocketError("RFCOMM bind");
      }

      closesocket(bcx->socket);
    } else {
      bthSocketError("RFCOMM socket");
    }

    free(bcx);
  } else {
    LogError("malloc");
  }

  return NULL;
}

void
bthDisconnect (BluetoothConnectionExtension *bcx) {
  closesocket(bcx->socket);
  free(bcx);
}

int
bthAwaitInput (BluetoothConnection *connection, int milliseconds) {
  BluetoothConnectionExtension *bcx = connection->extension;
  fd_set input;
  struct timeval timeout;

  FD_ZERO(&input);
  FD_SET(bcx->socket, &input);

  memset(&timeout, 0, sizeof(timeout));
  timeout.tv_sec = milliseconds / 1000;
  timeout.tv_usec = (milliseconds % 1000) * 1000;

  switch (select(1, &input, NULL, NULL, &timeout)) {
    case -1:
      bthSocketError("RFCOMM wait");
      break;

    case 0:
      errno = EAGAIN;
      break;

    default:
      return 1;
  }

  return 0;
}

ssize_t
bthReadData (
  BluetoothConnection *connection, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  errno = ENOSYS;
  LogError("Bluetooth read");
  return -1;
}

ssize_t
bthWriteData (BluetoothConnection *connection, const void *buffer, size_t size) {
  BluetoothConnectionExtension *bcx = connection->extension;
  DWORD result;

  if (WriteFile(bcx->socket, buffer, size, &result, NULL)) return result;
  bthSocketError("RFCOMM write");
  return -1;
}
