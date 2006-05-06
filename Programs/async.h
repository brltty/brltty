/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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

#ifndef BRLTTY_INCLUDED_ASYNC
#define BRLTTY_INCLUDED_ASYNC

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct {
  void *data;
  const void *buffer;
  size_t size;
  size_t length;
  int error;
  unsigned end:1;
} AsyncInputResult;

typedef size_t (*AsyncInputCallback) (const AsyncInputResult *result);


typedef struct {
  void *data;
  const void *buffer;
  size_t size;
  int error;
  size_t count;
} AsyncOutputResult;

typedef void (*AsyncOutputCallback) (const AsyncOutputResult *result);


extern int asyncRead (
  FileDescriptor fileDescriptor,
  size_t size,
  AsyncInputCallback callback, void *data
);

extern int asyncWrite (
  FileDescriptor fileDescriptor,
  const void *buffer, size_t size,
  AsyncOutputCallback callback, void *data
);


typedef void (*AsyncAlarmCallback) (void *data);

extern int asyncAbsoluteAlarm (
  const struct timeval *time,
  AsyncAlarmCallback callback,
  void *data
);

extern int asyncRelativeAlarm (
  int interval,
  AsyncAlarmCallback callback,
  void *data
);


extern void asyncWait (int duration);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_ASYNC */
