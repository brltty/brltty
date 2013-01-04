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
#include <time.h>

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#endif /* HAVE_GETTIMEOFDAY */

#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif /* HAVE_SYS_POLL_H */

#ifdef HAVE_SELECT
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */
#endif /* HAVE_SELECT */

#include "log.h"
#include "timing.h"

#ifdef __MSDOS__
#include "sys_msdos.h"
#endif /* __MSDOS__ */

#if !HAVE_DECL_LOCALTIME_R
static inline struct tm *
localtime_r (const time_t *timep, struct tm *result) {
  *result = *localtime(timep);
  return result;
}
#endif /* HAVE_DECL_LOCALTIME_R */

void
getCurrentTime (TimeValue *now) {
#if defined(GRUB_RUNTIME)
  static time_t baseSeconds = 0;
  static uint64_t baseMilliseconds;

  if (!baseSeconds) {
    baseSeconds = time(NULL);
    baseMilliseconds = grub_get_time_ms();
  }

  {
    uint64_t milliseconds = grub_get_time_ms() - baseMilliseconds;

    now->seconds = baseSeconds + (milliseconds / MSECS_PER_SEC);
    now->nanoseconds = (milliseconds % MSECS_PER_SEC) * NSECS_PER_MSEC;
  }

#elif defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_REALTIME)
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  now->seconds = ts.tv_sec;
  now->nanoseconds = ts.tv_nsec;

#elif defined(HAVE_GETTIMEOFDAY)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  now->seconds = tv.tv_sec;
  now->nanoseconds = tv.tv_usec * USECS_PER_MSEC;

#elif defined(HAVE_TIME)
  now->seconds = time(NULL);
  now->nanoseconds = 0;

#else /* get current time */
  now->seconds = 0;
  now->nanoseconds = 0;
#endif /* get current time */
}

void
makeTimeValue (TimeValue *value, const TimeComponents *components) {
  value->nanoseconds = components->nanosecond;

#if defined(GRUB_RUNTIME)
  value->seconds = 0;

#else /* make seconds */
  struct tm time = {
    .tm_year = components->year - 1900,
    .tm_mon = components->month,
    .tm_mday = components->day + 1,
    .tm_hour = components->hour,
    .tm_min = components->minute,
    .tm_sec = components->second,
    .tm_isdst = -1
  };

  value->seconds = mktime(&time);
#endif /* make seconds */
}

void
expandTimeValue (const TimeValue *value, TimeComponents *components) {
  time_t seconds = value->seconds;
  struct tm time;

  localtime_r(&seconds, &time);
  components->nanosecond = value->nanoseconds;

#if defined(GRUB_RUNTIME)
  components->year = time.tm.year;
  components->month = time.tm.month - 1;
  components->day = time.tm.day - 1;
  components->hour = time.tm.hour;
  components->minute = time.tm.minute;
  components->second = time.tm.second;

#else /* expand seconds */
  components->year = time.tm_year + 1900;
  components->month = time.tm_mon;
  components->day = time.tm_mday - 1;
  components->hour = time.tm_hour;
  components->minute = time.tm_min;
  components->second = time.tm_sec;
#endif /* expand seconds */
}

size_t
formatSeconds (char *buffer, size_t size, const char *format, int32_t seconds) {
  time_t time = seconds;
  struct tm description;

  localtime_r(&time, &description);
  return strftime(buffer, size, format, &description);
}

void
normalizeTimeValue (TimeValue *time) {
  while (time->nanoseconds < 0) {
    time->seconds -= 1;
    time->nanoseconds += NSECS_PER_SEC;
  }

  while (time->nanoseconds >= NSECS_PER_SEC) {
    time->seconds += 1;
    time->nanoseconds -= NSECS_PER_SEC;
  }
}

void
adjustTimeValue (TimeValue *time, int milliseconds) {
  TimeValue amount = {
    .seconds = milliseconds / MSECS_PER_SEC,
    .nanoseconds = (milliseconds % MSECS_PER_SEC) * NSECS_PER_MSEC
  };

  normalizeTimeValue(time);
  normalizeTimeValue(&amount);
  time->seconds += amount.seconds;
  time->nanoseconds += amount.nanoseconds;
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
  TimeValue elapsed = {
    .seconds = to->seconds - from->seconds,
    .nanoseconds = to->nanoseconds - from->nanoseconds
  };

  normalizeTimeValue(&elapsed);
  return (elapsed.seconds * MSECS_PER_SEC)
       + (elapsed.nanoseconds / NSECS_PER_MSEC);
}

void
getMonotonicTime (TimeValue *now) {
#if defined(GRUB_RUNTIME)
  grub_uint64_t milliseconds = grub_get_time_ms();
  now->seconds = milliseconds / MSECS_PER_SEC;
  now->nanoseconds = (milliseconds % MSECS_PER_SEC) * NSECS_PER_MSEC;

#elif defined(CLOCK_MONOTONIC_HR)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_HR, &ts);
  now->seconds = ts.tv_sec;
  now->nanoseconds = ts.tv_nsec;

#elif defined(CLOCK_MONOTONIC)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  now->seconds = ts.tv_sec;
  now->nanoseconds = ts.tv_nsec;

#else /* fall back to clock time */
  getCurrentTime(now);
#endif /* get monotonic time */
}

long int
getMonotonicElapsed (const TimeValue *start) {
  TimeValue now;

  getMonotonicTime(&now);
  return millisecondsBetween(start, &now);
}

void
restartTimePeriod (TimePeriod *period) {
  getMonotonicTime(&period->start);
}

void
startTimePeriod (TimePeriod *period, long int length) {
  period->length = length;
  restartTimePeriod(period);
}

int
afterTimePeriod (const TimePeriod *period, long int *elapsed) {
  long int milliseconds = getMonotonicElapsed(&period->start);

  if (elapsed) *elapsed = milliseconds;
  return milliseconds >= period->length;
}

void
approximateDelay (int milliseconds) {
  if (milliseconds > 0) {
#if defined(__MINGW32__)
    Sleep(milliseconds);

#elif defined(__MSDOS__)
    tsr_usleep(milliseconds * USECS_PER_MSEC);

#elif defined (GRUB_RUNTIME)
    grub_millisleep(milliseconds);

#elif defined(HAVE_NANOSLEEP)
    const struct timespec timeout = {
      .tv_sec = milliseconds / MSECS_PER_SEC,
      .tv_nsec = (milliseconds % MSECS_PER_SEC) * NSECS_PER_MSEC
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
      .tv_sec = milliseconds / MSECS_PER_SEC,
      .tv_usec = (milliseconds % MSECS_PER_SEC) * USECS_PER_MSEC
    };

    if (select(0, NULL, NULL, NULL, &timeout) == -1) {
      if (errno != EINTR) logSystemError("select");
    }

#endif /* delay */
  }
}

static int
getTickLength (void) {
  static int tickLength = 0;

  if (!tickLength) {
#if defined(GRUB_TICKS_PER_SECOND)
    tickLength = MSECS_PER_SEC / GRUB_TICKS_PER_SECOND;
#elif defined(_SC_CLK_TCK)
    tickLength = MSECS_PER_SEC / sysconf(_SC_CLK_TCK);
#elif defined(CLK_TCK)
    tickLength = MSECS_PER_SEC / CLK_TCK;
#elif defined(HZ)
    tickLength = MSECS_PER_SEC / HZ;
#else /* tick length */
#error cannot determine tick length
#endif /* tick length */

    if (!tickLength) tickLength = 1;
  }

  return tickLength;
}

void
accurateDelay (int milliseconds) {
  TimePeriod period;
  int tickLength = getTickLength();

  startTimePeriod(&period, milliseconds);
  if (milliseconds >= tickLength) approximateDelay(milliseconds / tickLength * tickLength);

  while (!afterTimePeriod(&period, NULL)) {
#ifdef __MSDOS__
    /* We're executing as a system clock interrupt TSR so we need to allow
     * them to occur in order for gettimeofday() to change.
     */
    tsr_usleep(1);
#endif /* __MSDOS__ */
  }
}
