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

#ifndef BRLTTY_INCLUDED_IOMISC
#define BRLTTY_INCLUDED_IOMISC

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int awaitInput (int fileDescriptor, int milliseconds);
extern int readChunk (
  int fileDescriptor,
  void *buffer, size_t *offset, size_t count,
  int initialTimeout, int subsequentTimeout
);
extern ssize_t readData (
  int fileDescriptor, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

extern int awaitOutput (int fileDescriptor, int milliseconds);
extern ssize_t writeData (int fileDescriptor, const void *buffer, size_t size);

#ifndef __MINGW32__
extern int changeOpenFlags (int fileDescriptor, int flagsToClear, int flagsToSet);
extern int setOpenFlags (int fileDescriptor, int state, int flags);
extern int setBlockingIo (int fileDescriptor, int state);
extern int setCloseOnExec (int fileDescriptor, int state);
#endif /* __MINGW32__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IOMISC */
