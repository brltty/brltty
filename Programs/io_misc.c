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
} FileDescriptorMonitorData;

static int
monitorFileDescriptor (const AsyncMonitorResult *result) {
  FileDescriptorMonitorData *fdm = result->data;
  fdm->ready = 1;
  return 0;
}

static int
testFileDescriptor (void *data) {
  FileDescriptorMonitorData *fdm = data;

  return fdm->ready;
}

static int
awaitFileDescriptor (int fileDescriptor, int timeout, int output) {
  FileDescriptorMonitorData fdm = {
    .ready = 0
  };

  AsyncHandle monitor;
  int monitoring;

  if (output) {
    monitoring = asyncMonitorFileOutput(&monitor, fileDescriptor, monitorFileDescriptor, &fdm);
  } else {
    monitoring = asyncMonitorFileInput(&monitor, fileDescriptor, monitorFileDescriptor, &fdm);
  }

  if (monitoring) {
    asyncAwait(timeout, testFileDescriptor, &fdm);
    asyncCancel(monitor);

    if (fdm.ready) return 1;
#ifdef ETIMEDOUT
    errno = ETIMEDOUT;
#else /* ETIMEDOUT */
    errno = EAGAIN;
#endif /* ETIMEDOUT */
  }

  return 0;
}

int
awaitInput (int fileDescriptor, int timeout) {
  return awaitFileDescriptor(fileDescriptor, timeout, 0);
}

int
awaitOutput (int fileDescriptor, int timeout) {
  return awaitFileDescriptor(fileDescriptor, timeout, 1);
}

ssize_t
readData (
  int fileDescriptor, void *buffer, size_t size,
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
        if (awaitInput(fileDescriptor, timeout)) continue;
        logMessage(LOG_WARNING, "input byte missing at offset %u", offset);
      } else

#ifdef __MSDOS__
      if (!tried) {
        if (awaitInput(fileDescriptor, 0)) continue;
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
writeData (int fileDescriptor, const void *buffer, size_t size) {
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
        if (awaitOutput(fileDescriptor, 15000)) goto canWrite;
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
connectSocket (
  int socketDescriptor,
  const struct sockaddr *address,
  size_t addressLength,
  int timeout
) {
  int result = connect(socketDescriptor, address, addressLength);

  if (result == -1) {
    if (errno == EINPROGRESS) {
      if (awaitOutput(socketDescriptor, timeout)) {
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
