/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

#ifdef HAVE_CLOCK_GETTIME
#include <time.h>
#endif /* HAVE_CLOCK_GETTIME */

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#endif /* HAVE_GETTIMEOFDAY */

#ifdef HAVE_TIME
#include <time.h>
#endif /* HAVE_TIME */

#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif /* HAVE_SYS_POLL_H */

#ifdef HAVE_SELECT
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */
#endif /* HAVE_SELECT */

#include "log.h"
#include "timing.h"

#ifdef __MSDOS__
#include "sys_msdos.h"
#endif /* __MSDOS__ */

void
getCurrentTime (TimeValue *now) {
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_REALTIME)
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  now->seconds = ts.tv_sec;
  now->nanoseconds = ts.tv_nsec;

#elif defined(HAVE_GETTIMEOFDAY)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  now->seconds = tv.tv_sec;
  now->nanoseconds = tv.tv_usec * 1000;

#elif defined(HAVE_TIME)
  now->seconds = time(NULL);
  now->nanoseconds = 0;

#else /* get current time */
  now->seconds = 0;
  now->nanoseconds = 0;
#endif /* get current time */
}

void
approximateDelay (int milliseconds) {
  if (milliseconds > 0) {
#if defined(__MINGW32__)
    Sleep(milliseconds);

#elif defined(__MSDOS__)
    tsr_usleep(milliseconds*1000);

#elif defined(HAVE_NANOSLEEP)
    const struct timespec timeout = {
      .tv_sec = milliseconds / 1000,
      .tv_nsec = (milliseconds % 1000) * 1000000
    };

    if (nanosleep(&timeout, NULL) == -1) {
      if (errno != EINTR) logSystemError("nanosleep");
    }

#elif defined(HAVE_SYS_POLL_H)
    if (poll(NULL, 0, milliseconds) == -1) {
      if (errno != EINTR) logSystemError("poll");
    }

#elif defined(HAVE_SELECT)
    struct timeval timeout = {
      .tv_sec = milliseconds / 1000,
      .tv_usec = (milliseconds % 1000) * 1000
    };

    if (select(0, NULL, NULL, NULL, &timeout) == -1) {
      if (errno != EINTR) logSystemError("select");
    }

#endif /* delay */
  }
}

void
normalizeTimeValue (TimeValue *time) {
  time->seconds += time->nanoseconds / 1000000000;
  time->nanoseconds = time->nanoseconds % 1000000000;
}

void
adjustTimeValue (TimeValue *time, int amount) {
  int quotient = amount / 1000;
  int remainder = amount % 1000;

  if (remainder < 0) remainder += 1000, quotient -= 1;
  time->seconds += quotient;
  time->nanoseconds += remainder * 1000000;
  normalizeTimeValue(time);
}

int
compareTimeValues (const TimeValue *first, const TimeValue *second) {
  if (first->seconds < second->seconds) return -1;
  if (first->seconds > second->seconds) return 1;

  if (first->nanoseconds < second->nanoseconds) return -1;
  if (first->nanoseconds > second->nanoseconds) return 1;

  return 0;
}

long int
millisecondsBetween (const TimeValue *from, const TimeValue *to) {
  return ((to->seconds - from->seconds) * 1000)
       + ((to->nanoseconds - from->nanoseconds) / 1000000);
}

long int
millisecondsSince (const TimeValue *from) {
  TimeValue now;
  getCurrentTime(&now);
  return millisecondsBetween(from, &now);
}

void
accurateDelay (int milliseconds) {
  static int tickLength = 0;

  TimeValue start;
  getCurrentTime(&start);

  if (!tickLength) {
#if defined(_SC_CLK_TCK)
    tickLength = 1000 / sysconf(_SC_CLK_TCK);
#elif defined(CLK_TCK)
    tickLength = 1000 / CLK_TCK;
#elif defined(HZ)
    tickLength = 1000 / HZ;
#else /* tick length */
#error cannot determine tick length
#endif /* tick length */

    if (!tickLength) tickLength = 1;
  }

  if (milliseconds >= tickLength) approximateDelay(milliseconds / tickLength * tickLength);

  while (millisecondsSince(&start) < milliseconds) {
#ifdef __MSDOS__
    /* We're executing as a system clock interrupt TSR so we need to allow
     * them to occur in order for gettimeofday() to change.
     */
    tsr_usleep(1);
#endif /* __MSDOS__ */
  }
}

int
hasTimedOut (int milliseconds) {
  static TimeValue start = {0, 0};

  if (milliseconds) return millisecondsSince(&start) >= milliseconds;

  getCurrentTime(&start);
  return 1;
}
