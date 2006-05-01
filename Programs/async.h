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
  int error;
  size_t count;
} InputResult;

typedef size_t (*InputCallback) (const InputResult *result);


typedef struct {
  void *data;
  const void *buffer;
  size_t size;
  int error;
  size_t count;
} OutputResult;

typedef void (*OutputCallback) (const OutputResult *result);


extern int asyncRead (
  FileDescriptor fileDescriptor,
  size_t size,
  InputCallback callback, void *data
);

extern int asyncWrite (
  FileDescriptor fileDescriptor,
  const void *buffer, size_t size,
  OutputCallback callback, void *data
);


extern void asyncWait (int timeout);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_ASYNC */
