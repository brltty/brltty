/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */

#include "io.h"
#include "misc.h"

int
awaitInput (int descriptor, int milliseconds) {
  fd_set mask;
  struct timeval timeout;

  FD_ZERO(&mask);
  FD_SET(descriptor, &mask);

  memset(&timeout, 0, sizeof(timeout));
  timeout.tv_sec = milliseconds / 1000;
  timeout.tv_usec = (milliseconds % 1000) * 1000;

  while (1) {
    switch (select(descriptor+1, &mask, NULL, NULL, &timeout)) {
      case -1:
        if (errno == EINTR) continue;
        LogError("Input wait");
        return 0;

      case 0:
        if (milliseconds > 0)
          LogPrint(LOG_DEBUG, "Input wait timed out after %d %s.",
                   milliseconds, ((milliseconds == 1)? "millisecond": "milliseconds"));
        errno = EAGAIN;
        return 0;

      default:
        return 1;
    }
  }
}

int
readChunk (
  int descriptor,
  unsigned char *buffer, int *offset, int count,
  int initialTimeout, int subsequentTimeout
) {
  while (count > 0) {
    int amount = read(descriptor, buffer+*offset, count);

    if (amount == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) goto noInput;
      if (errno == EWOULDBLOCK) goto noInput;
      LogError("Read");
      return 0;
    }

    if (amount == 0) {
      int timeout;
    noInput:
      if ((timeout = *offset? subsequentTimeout: initialTimeout)) {
        if (awaitInput(descriptor, timeout)) continue;
        LogPrint(LOG_WARNING, "Input byte missing at offset %d.", *offset);
      } else {
        errno = EAGAIN;
      }
      return 0;
    }

    *offset += amount;
    count -= amount;
  }
  return 1;
}

int
timedRead (
  int descriptor, unsigned char *buffer, int count,
  int initialTimeout, int subsequentTimeout
) {
  int length = 0;
  if (readChunk(descriptor, buffer, &length, count, initialTimeout, subsequentTimeout)) return count;
  if (errno == EAGAIN) return length;
  return -1;
}

ssize_t
writeData (int fd, const void *buffer, size_t size) {
  const unsigned char *address = buffer;

  while (size > 0) {
    ssize_t count = write(fd, address, size);

    if (count == -1) {
      if (errno == EINTR) continue;
      if (errno != EAGAIN) return count;
      count = 0;
    }

    if (count) {
      address += count;
      size -= count;
    } else {
      delay(100);
    }
  }

  {
    const unsigned char *start = buffer;
    return address - start;
  }
}

int
changeOpenFlags (int fileDescriptor, int flagsToClear, int flagsToSet) {
  int flags;
  if ((flags = fcntl(fileDescriptor, F_GETFL)) != -1) {
    flags &= ~flagsToClear;
    flags |= flagsToSet;
    if (fcntl(fileDescriptor, F_SETFL, flags) != -1) {
      return 1;
    } else {
      LogError("F_SETFL");
    }
  } else {
    LogError("F_GETFL");
  }
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
  return setOpenFlags(fileDescriptor, !state, O_NONBLOCK);
}

int
setCloseOnExec (int fileDescriptor) {
  return fcntl(fileDescriptor, F_SETFD, FD_CLOEXEC) != -1;
}
