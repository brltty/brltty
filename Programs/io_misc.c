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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#include "io_misc.h"
#include "log.h"
#include "timing.h"
#include "async.h"

typedef struct {
  unsigned ready:1;
} InputOutputMonitorData;

static int
monitorInputOutput (const AsyncMonitorResult *result) {
  InputOutputMonitorData *iom = result->data;
  iom->ready = 1;
  return 0;
}

static int
testInputOutput (void *data) {
  InputOutputMonitorData *iom = data;

  return iom->ready;
}

static int
awaitFileDescriptor (FileDescriptor fileDescriptor, int timeout, int output) {
  InputOutputMonitorData iom = {
    .ready = 0
  };

  AsyncHandle monitor;
  int monitoring;

  if (output) {
    monitoring = asyncMonitorFileOutput(&monitor, fileDescriptor, monitorInputOutput, &iom);
  } else {
    monitoring = asyncMonitorFileInput(&monitor, fileDescriptor, monitorInputOutput, &iom);
  }

  if (monitoring) {
    asyncAwait(timeout, testInputOutput, &iom);
    asyncCancel(monitor);

    if (iom.ready) return 1;
#ifdef ETIMEDOUT
    errno = ETIMEDOUT;
#else /* ETIMEDOUT */
    errno = EAGAIN;
#endif /* ETIMEDOUT */
  }

  return 0;
}

int
awaitFileInput (FileDescriptor fileDescriptor, int timeout) {
  return awaitFileDescriptor(fileDescriptor, timeout, 0);
}

int
awaitFileOutput (FileDescriptor fileDescriptor, int timeout) {
  return awaitFileDescriptor(fileDescriptor, timeout, 1);
}

ssize_t
readFile (
  FileDescriptor fileDescriptor, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  unsigned char *address = buffer;

#ifdef __MSDOS__
  int tried = 0;
  goto noInput;
#endif /* __MSDOS__ */

  while (size > 0) {
    ssize_t count = read(fileDescriptor, address, size);

#ifdef __MSDOS__
    tried = 1;
#endif /* __MSDOS__ */

    if (count == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) goto noInput;

#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK) goto noInput;
#endif /* EWOULDBLOCK */

      logSystemError("read");
      return count;
    }

    if (!count) {
      unsigned char *start;
      unsigned int offset;
      int timeout;

    noInput:
      start = buffer;
      offset = address - start;
      timeout = offset? subsequentTimeout: initialTimeout;

      if (timeout) {
        if (awaitFileInput(fileDescriptor, timeout)) continue;
        logMessage(LOG_WARNING, "input byte missing at offset %u", offset);
      } else

#ifdef __MSDOS__
      if (!tried) {
        if (awaitFileInput(fileDescriptor, 0)) continue;
      } else
#endif /* __MSDOS__ */

      {
        errno = EAGAIN;
      }

      break;
    }

    address += count;
    size -= count;
  }

  {
    unsigned char *start = buffer;

    return address - start;
  }
}

ssize_t
writeFile (FileDescriptor fileDescriptor, const void *buffer, size_t size) {
  const unsigned char *address = buffer;

canWrite:
  while (size > 0) {
    ssize_t count = write(fileDescriptor, address, size);

    if (count == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) goto noOutput;

#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK) goto noOutput;
#endif /* EWOULDBLOCK */

      logSystemError("Write");
      return count;
    }

    if (!count) {
      errno = EAGAIN;

    noOutput:
      do {
        if (awaitFileOutput(fileDescriptor, 15000)) goto canWrite;
      } while (errno == EAGAIN);

      return -1;
    }

    address += count;
    size -= count;
  }

  {
    const unsigned char *start = buffer;
    return address - start;
  }
}

#ifdef HAVE_SYS_SOCKET_H
int
awaitSocketInput (SocketDescriptor socketDescriptor, int timeout) {
  return awaitFileInput(socketDescriptor, timeout);
}

int
awaitSocketOutput (SocketDescriptor socketDescriptor, int timeout) {
  return awaitFileOutput(socketDescriptor, timeout);
}

ssize_t
readSocket (
  SocketDescriptor socketDescriptor, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return readFile(socketDescriptor, buffer, size, initialTimeout, subsequentTimeout);
}

ssize_t
writeSocket (SocketDescriptor socketDescriptor, const void *buffer, size_t size) {
  return writeFile(socketDescriptor, buffer, size);
}

int
connectSocket (
  SocketDescriptor socketDescriptor,
  const struct sockaddr *address,
  size_t addressLength,
  int timeout
) {
  int result = connect(socketDescriptor, address, addressLength);

  if (result == -1) {
    if (errno == EINPROGRESS) {
      if (awaitSocketOutput(socketDescriptor, timeout)) {
        int error;
        socklen_t length = sizeof(error);

        if (getsockopt(socketDescriptor, SOL_SOCKET, SO_ERROR, &error, &length) != -1) {
          if (!error) return 0;
          errno = error;
        }
      }

      close(socketDescriptor);
    }
  }

  return result;
}
#endif /* HAVE_SYS_SOCKET_H */

int
changeOpenFlags (int fileDescriptor, int flagsToClear, int flagsToSet) {
#if defined(F_GETFL) && defined(F_SETFL)
  int flags;

  if ((flags = fcntl(fileDescriptor, F_GETFL)) != -1) {
    flags &= ~flagsToClear;
    flags |= flagsToSet;
    if (fcntl(fileDescriptor, F_SETFL, flags) != -1) {
      return 1;
    } else {
      logSystemError("F_SETFL");
    }
  } else {
    logSystemError("F_GETFL");
  }
#else /* defined(F_GETFL) && defined(F_SETFL) */
  errno = ENOSYS;
#endif /* defined(F_GETFL) && defined(F_SETFL) */

  return 0;
}

int
setOpenFlags (int fileDescriptor, int state, int flags) {
  if (state) {
    return changeOpenFlags(fileDescriptor, 0, flags);
  } else {
    return changeOpenFlags(fileDescriptor, flags, 0);
  }
}

int
setBlockingIo (int fileDescriptor, int state) {
#ifdef O_NONBLOCK
  if (setOpenFlags(fileDescriptor, !state, O_NONBLOCK)) return 1;
#else /* O_NONBLOCK */
  errno = ENOSYS;
#endif /* O_NONBLOCK */

  return 0;
}

int
setCloseOnExec (int fileDescriptor, int state) {
#if defined(F_SETFD) && defined(FD_CLOEXEC)
  if (fcntl(fileDescriptor, F_SETFD, (state? FD_CLOEXEC: 0)) != -1) return 1;
#else /* defined(F_SETFD) && defined(FD_CLOEXEC) */
  errno = ENOSYS;
#endif /* defined(F_SETFD) && defined(FD_CLOEXEC) */

  return 0;
}
