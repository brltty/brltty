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

#ifndef _IO_H
#define _IO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int awaitInput (int descriptor, int milliseconds);
extern int readChunk (
  int descriptor,
  unsigned char *buffer, int *offset, int count,
  int initialTimeout, int subsequentTimeout
);
extern int timedRead (
  int descriptor, unsigned char *buffer, int count,
  int initialTimeout, int subsequentTimeout
);

extern ssize_t writeData (int fd, const void *buffer, size_t size);

extern int changeOpenFlags (int fileDescriptor, int flagsToClear, int flagsToSet);
extern int setOpenFlags (int fileDescriptor, int state, int flags);
extern int setBlockingIo (int fileDescriptor, int state);
extern int setCloseOnExec (int fileDescriptor);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _IO_H */
