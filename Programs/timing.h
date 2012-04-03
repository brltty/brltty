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

#ifndef BRLTTY_INCLUDED_TIMING
#define BRLTTY_INCLUDED_TIMING

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  int32_t seconds;
  uint32_t nanoseconds;
} TimeValue;

extern void getCurrentTime (TimeValue *time);
extern size_t formatSeconds (char *buffer, size_t size, const char *format, time_t seconds);

extern void normalizeTimeValue (TimeValue *time);
extern void adjustTimeValue (TimeValue *time, int amount);
extern int compareTimeValues (const TimeValue *first, const TimeValue *second);

extern void approximateDelay (int milliseconds);		/* sleep for `msec' milliseconds */
extern void accurateDelay (int milliseconds);

extern long int millisecondsBetween (const TimeValue *from, const TimeValue *to);
extern long int millisecondsSince (const TimeValue *from);

extern int hasTimedOut (int milliseconds);	/* test timeout condition */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_TIMING */
