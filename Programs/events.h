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

#ifndef BRLTTY_INCLUDED_EVENTS
#define BRLTTY_INCLUDED_EVENTS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  void *data;
  const void *buffer;
  size_t size;
  ssize_t count;
  int error;
} InputOutputResponse;

typedef void (*InputOutputListener) (const InputOutputResponse *response);

extern int asyncRead (
  int fileDescriptor, size_t size,
  InputOutputListener respond, void *data
);

extern int asyncWrite (
  int fileDescriptor, const void *buffer, size_t size,
  InputOutputListener respond, void *data
);

extern void processEvents (int milliseconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_EVENTS */
