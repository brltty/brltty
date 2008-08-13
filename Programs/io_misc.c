/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#if defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#elif defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */

#include "io_misc.h"
#include "misc.h"

#ifdef __MSDOS__
#include "sys_msdos.h"
#endif /* __MSDOS__ */

static int
awaitFileDescriptor (int fileDescriptor, int milliseconds, int output) {
#ifdef __MSDOS__
  int left = milliseconds * 1000;
#endif /* __MSDOS__ */

  while (1) {
    {
#if defined(HAVE_SYS_POLL_H)
      struct pollfd pfd = {
        .fd = fileDescriptor,
        .events = output? POLLOUT: POLLIN
      };
      int timeout;

#ifdef __MSDOS__
      timeout = 0;
#else /* __MSDOS__ */
      timeout = milliseconds;
#endif /* __MSDOS__ */

      {
        int result = poll(&pfd, 1, timeout);

        if (result == -1) {
          if (errno == EINTR) continue;
          LogError("poll");
          return 0;
        }

        if (result) return 1;
      }
#else /* select */
      fd_set mask;
      struct timeval timeout;

      FD_ZERO(&mask);
      FD_SET(fileDescriptor, &mask);

      memset(&timeout, 0, sizeof(timeout));
#ifndef __MSDOS__
      timeout.tv_sec = milliseconds / 1000;
      timeout.tv_usec = (milliseconds % 1000) * 1000;
#endif /* __MSDOS__ */

      {
        int result = select(fileDescriptor+1,
                            (output? NULL: &mask),
                            (output? &mask: NULL),
                            NULL, &timeout);

        if (result == -1) {
          if (errno == EINTR) continue;
          LogError("select");
          return 0;
        }

        if (result) return 1;
      }
#endif /* await file descriptor */
    }

#ifdef __MSDOS__
    if (left > 0) {
      left -= tsr_usleep(1000);
      continue;
    }
#endif /* __MSDOS__ */

    if (milliseconds > 0)
      LogPrint(LOG_DEBUG, "timeout after %d %s",
               milliseconds, ((milliseconds == 1)? "millisecond": "milliseconds"));

    errno = EAGAIN;
    return 0;
  }
}

int
awaitInput (int fileDescriptor, int milliseconds) {
  return awaitFileDescriptor(fileDescriptor, milliseconds, 0);
}

int
readChunk (
  int fileDescriptor,
  void *buffer, size_t *offset, size_t count,
  int initialTimeout, int subsequentTimeout
) {
#ifdef __MSDOS__
  int tried = 0;
  goto noInput;
#endif /* __MSDOS__ */

  while (count > 0) {
    ssize_t amount;

    {
      unsigned char *address = buffer;
      address += *offset;
      amount = read(fileDescriptor, address, count);
    }

#ifdef __MSDOS__
    tried = 1;
#endif /* __MSDOS__ */

    if (amount == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) goto noInput;

#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK) goto noInput;
#endif /* EWOULDBLOCK */

      LogError("read");
      return 0;
    }

    if (amount == 0) {
      int timeout;
    noInput:
      if ((timeout = *offset? subsequentTimeout: initialTimeout)) {
        if (awaitInput(fileDescriptor, timeout)) continue;
        LogPrint(LOG_WARNING, "input byte missing at offset %d", (int)*offset);
#ifdef __MSDOS__
      } else if (!tried) {
        if (awaitInput(fileDescriptor, 0)) continue;
#endif /* __MSDOS__ */
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

ssize_t
readData (
  int fileDescriptor, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  size_t length = 0;
  if (readChunk(fileDescriptor, buffer, &length, size, initialTimeout, subsequentTimeout)) return size;
  if (errno == EAGAIN) return length;
  return -1;
}

int
awaitOutput (int fileDescriptor, int milliseconds) {
  return awaitFileDescriptor(fileDescriptor, milliseconds, 1);
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
      LogError("Write");
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
      LogError("F_SETFL");
    }
  } else {
    LogError("F_GETFL");
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
