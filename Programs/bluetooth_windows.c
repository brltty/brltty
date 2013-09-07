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
#define WINVER WindowsVista
#define __USE_W32_SOCKETS

#pragma pack(push,2)
#include <winsock2.h>
#include <ws2bth.h>
#pragma pack(pop)

#include "prologue.h"

#include <errno.h>

#if defined(AF_BTH)
#ifndef PF_BTH
#define PF_BTH AF_BTH
#endif /* PF_BTH */

#elif defined(PF_BTH)
#ifndef AF_BTH
#define AF_BTH PF_BTH
#endif /* AF_BTH */

#else /* bluetooth address/protocol family definitions */
#define AF_BTH 32
#define PF_BTH AF_BTH
#warning using hard-coded value for Bluetooth socket address and protocol family constants
#endif /* bluetooth address/protocol family definitions */

#ifndef NS_BTH
#define NS_BTH 16
#warning using hard-coded value for Bluetooth namespace constant
#endif /* NS_BTH */

#include "log.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"

struct BluetoothConnectionExtensionStruct {
  SOCKET socket;
  SOCKADDR_BTH local;
  SOCKADDR_BTH remote;
};

static void
bthSetErrno (DWORD error, const char *action, const DWORD *exceptions) {
  if (exceptions) {
    const DWORD *exception = exceptions;

    while (*exception != NO_ERROR) {
      if (error == *exception) goto isException;
      exception += 1;
    }
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
bthNewConnectionExtension (uint64_t bda) {
  int result;
  WSADATA wsa;

  if ((result = WSAStartup(MAKEWORD(2, 2), &wsa)) == NO_ERROR) {
    BluetoothConnectionExtension *bcx;

    if ((bcx = malloc(sizeof(*bcx)))) {
      memset(bcx, 0, sizeof(*bcx));

      bcx->local.addressFamily = AF_BTH;
      bcx->local.btAddr = BTH_ADDR_NULL;
      bcx->local.port = 0;

      bcx->remote.addressFamily = AF_BTH;
      bcx->remote.btAddr = bda;
      bcx->remote.port = 0;

      bcx->socket = INVALID_SOCKET;
      return bcx;
    } else {
      logMallocError();
    }
  } else {
    bthSetErrno(result, "WSA startup", NULL);
  }

  return NULL;
}

void
bthReleaseConnectionExtension (BluetoothConnectionExtension *bcx) {
  if (bcx->socket != INVALID_SOCKET) {
    closesocket(bcx->socket);
    bcx->socket = INVALID_SOCKET;
  }

  free(bcx);
}

int
bthOpenChannel (BluetoothConnectionExtension *bcx, uint8_t channel, int timeout) {
  bcx->remote.port = channel;

  if ((bcx->socket = socket(PF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)) != INVALID_SOCKET) {
    if (bind(bcx->socket, (SOCKADDR *)&bcx->local, sizeof(bcx->local)) != SOCKET_ERROR) {
      if (connect(bcx->socket, (SOCKADDR *)&bcx->remote, sizeof(bcx->remote)) != SOCKET_ERROR) {
        unsigned long nonblocking = 1;

        if (ioctlsocket(bcx->socket, FIONBIO, &nonblocking) != SOCKET_ERROR) {
          return 1;
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
    bcx->socket = INVALID_SOCKET;
  } else {
    bthSocketError("RFCOMM socket", NULL);
  }

  return 0;
}

int
bthDiscoverChannel (
  uint8_t *channel, BluetoothConnectionExtension *bcx,
  const void *uuidBytes, size_t uuidLength
) {
  char addressString[0X100];
  DWORD addressLength = sizeof(addressString);

  if (WSAAddressToString((SOCKADDR *)&bcx->remote, sizeof(bcx->remote),
                          NULL,
                          addressString, &addressLength) != SOCKET_ERROR) {
    HANDLE handle;

    WSAQUERYSET restrictions = {
      .dwNameSpace = NS_BTH,
      .lpszContext = addressString,
      .lpServiceClassId = (GUID *)uuidBytes,
      .dwNumberOfCsAddrs = 0,
      .dwSize = sizeof(restrictions)
    };

    if (WSALookupServiceBegin(&restrictions, LUP_FLUSHCACHE, &handle) != SOCKET_ERROR) {
      WSAQUERYSET result;
      DWORD resultLength = sizeof(result);

      while (WSALookupServiceNext(handle, LUP_RETURN_ADDR, &resultLength, &result) != SOCKET_ERROR) {
        SOCKADDR_BTH *address = (SOCKADDR_BTH *)&result.lpcsaBuffer->RemoteAddr;
        logMessage(LOG_NOTICE, "windows sdp: %lu", address->port);
      }
      bthSocketError("WSALookupServiceNext", NULL);

      if (WSALookupServiceEnd(handle) == SOCKET_ERROR) {
        bthSocketError("WSALookupServiceEnd", NULL);
      }
    } else {
      bthSocketError("WSALookupServiceBegin", NULL);
    }
  } else {
    bthSocketError("WSAAddressToString", NULL);
  }

  return 0;
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

char *
bthObtainDeviceName (uint64_t bda, int timeout) {
  errno = ENOSYS;
  return NULL;
}
