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

#include <errno.h>

#include <winsock2.h>
#include <ws2bth.h>

#include "io_bluetooth.h"
#include "bluetooth_internal.h"
#include "log.h"

struct BluetoothConnectionExtensionStruct {
  SOCKET socket;
  SOCKADDR_BTH local;
  SOCKADDR_BTH remote;
};

static void
bthSetErrno (DWORD error, const char *action, const DWORD *exceptions) {
  if (exceptions) {
    const DWORD *exception = exceptions;

    while (*exception != NO_ERROR)
      if (error == *exception++)
        goto isException;
  }

  logWindowsError(error, action);
isException:
  setErrno(error);
}

static DWORD
bthSocketError (const char *action, const DWORD *exceptions) {
  DWORD error = WSAGetLastError();

  bthSetErrno(error, action, exceptions);
  return error;
}

BluetoothConnectionExtension *
bthConnect (uint64_t bda, uint8_t channel) {
  int result;
  WSADATA wsa;

  if ((result = WSAStartup(MAKEWORD(2, 2), &wsa)) == NO_ERROR) {
    BluetoothConnectionExtension *bcx;

    if ((bcx = malloc(sizeof(*bcx)))) {
      memset(bcx, 0, sizeof(*bcx));

      if ((bcx->socket = socket(PF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)) != INVALID_SOCKET) {
        bcx->local.addressFamily = AF_BTH;
        bcx->local.btAddr = BTH_ADDR_NULL;
        bcx->local.port = BT_PORT_ANY;

        if (bind(bcx->socket, (SOCKADDR *)&bcx->local, sizeof(bcx->local)) != SOCKET_ERROR) {
          bcx->remote.addressFamily = AF_BTH;
          bcx->remote.btAddr = bda;
          bcx->remote.port = channel;

          if (connect(bcx->socket, (SOCKADDR *)&bcx->remote, sizeof(bcx->remote)) != SOCKET_ERROR) {
            unsigned long nonblocking = 1;

            if (ioctlsocket(bcx->socket, FIONBIO, &nonblocking) != SOCKET_ERROR) {
              return bcx;
            } else {
              bthSocketError("RFCOMM nonblocking", NULL);
            }
          } else {
            static const DWORD exceptions[] = {
              ERROR_HOST_DOWN,
              ERROR_HOST_UNREACHABLE,
              NO_ERROR
            };
            bthSocketError("RFCOMM connect", exceptions);
          }
        } else {
          bthSocketError("RFCOMM bind", NULL);
        }

        closesocket(bcx->socket);
      } else {
        bthSocketError("RFCOMM socket", NULL);
      }

      free(bcx);
    } else {
      logMallocError();
    }

    WSACleanup();
  } else {
    bthSetErrno(result, "WSA startup", NULL);
  }

  return NULL;
}

void
bthDisconnect (BluetoothConnectionExtension *bcx) {
  if (closesocket(bcx->socket) == SOCKET_ERROR) bthSocketError("RFCOMM close", NULL);
  if (WSACleanup() == SOCKET_ERROR) bthSocketError("WSA cleanup", NULL);
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
    case SOCKET_ERROR:
      bthSocketError("RFCOMM wait", NULL);
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
  BluetoothConnectionExtension *bcx = connection->extension;
  size_t count = size;

  if (count) {
    char *to = buffer;

    while (1) {
      int result = recv(bcx->socket, to, count, 0);

      if (result != SOCKET_ERROR) {
        to += result;
        if (!(count -= result)) break;
      } else {
        static const DWORD exceptions[] = {
          WSAEWOULDBLOCK,
          NO_ERROR
        };
        DWORD error = bthSocketError("RFCOMM read", exceptions);

        if (error != WSAEWOULDBLOCK) return -1;
      }

      {
        int timeout = (to == buffer)? initialTimeout: subsequentTimeout;

        if (!timeout) {
          errno = EAGAIN;
          return -1;
        }

        if (!bthAwaitInput(connection, timeout)) return -1;
      }
    }
  }

  return size - count;
}

ssize_t
bthWriteData (BluetoothConnection *connection, const void *buffer, size_t size) {
  BluetoothConnectionExtension *bcx = connection->extension;
  const char *from = buffer;
  size_t count = size;

  while (count) {
    int result = send(bcx->socket, from, count, 0);

    if (result == SOCKET_ERROR) {
      bthSocketError("RFCOMM write", NULL);
      return -1;
    }

    from += result;
    count -= result;
  }

  return size;
}
