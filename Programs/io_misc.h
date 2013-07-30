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

#ifndef BRLTTY_INCLUDED_IO_MISC
#define BRLTTY_INCLUDED_IO_MISC

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int awaitFileInput (FileDescriptor fileDescriptor, int timeout);
extern int awaitFileOutput (FileDescriptor fileDescriptor, int timeout);

extern ssize_t readFile (
  FileDescriptor fileDescriptor, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

extern ssize_t writeFile (FileDescriptor fileDescriptor, const void *buffer, size_t size);

#ifdef HAVE_SYS_SOCKET_H
extern int awaitSocketInput (SocketDescriptor socketDescriptor, int timeout);
extern int awaitSocketOutput (SocketDescriptor socketDescriptor, int timeout);

extern ssize_t readSocket (
  SocketDescriptor socketDescriptor, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

extern ssize_t writeSocket (SocketDescriptor socketDescriptor, const void *buffer, size_t size);

extern int connectSocket (
  SocketDescriptor socketDescriptor,
  const struct sockaddr *address,
  size_t addressLength,
  int timeout
);
#endif /* HAVE_SYS_SOCKET_H */

#ifndef __MINGW32__
extern int changeOpenFlags (int fileDescriptor, int flagsToClear, int flagsToSet);
extern int setOpenFlags (int fileDescriptor, int state, int flags);
extern int setBlockingIo (int fileDescriptor, int state);
extern int setCloseOnExec (int fileDescriptor, int state);
#endif /* __MINGW32__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IO_MISC */
