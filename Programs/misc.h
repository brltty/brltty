/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_MISC
#define BRLTTY_INCLUDED_MISC

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "prologue.h"

#include <sys/time.h>

#ifndef MIN
#define MIN(a, b)  (((a) < (b))? (a): (b)) 
#endif /* MIN */

#ifndef MAX
#define MAX(a, b)  (((a) > (b))? (a): (b)) 
#endif /* MAX */

#define DEFINE_POINTER_TO(name,prefix) typeof(name) *prefix##name

#ifdef WINDOWS
#define getSystemError() GetLastError()

#ifdef __CYGWIN32__
#include <sys/cygwin.h>

#define getSocketError() errno
#define setErrno(error) errno = cygwin_internal(CW_GET_ERRNO_FROM_WINERROR, (error))
#else /* __CYGWIN32__ */
#define getSocketError() WSAGetLastError()
#define setErrno(error) errno = EIO
#endif /* __CYGWIN32__ */

#else /* WINDOWS */
#define getSystemError() errno
#define getSocketError() errno

#define setErrno(error)
#endif /* WINDOWS */

#define setSystemErrno() setErrno(getSystemError())
#define setSocketErrno() setErrno(getSocketError())

#if defined(__MINGW32__)
typedef DWORD ProcessIdentifier;
#define PRIpid "lu"
#define SCNpid "lu"

#elif defined(__MSDOS__)
typedef int ProcessIdentifier;
#define PRIpid "d"
#define SCNpid "d"
#define DOS_PROCESS_ID 1

#else /* Unix */
typedef pid_t ProcessIdentifier;
#define PRIpid "d"
#define SCNpid "d"
#endif /* platform-dependent definitions */

extern ProcessIdentifier getProcessIdentifier (void);
extern int testProcessIdentifier (ProcessIdentifier pid);

extern void approximateDelay (int milliseconds);		/* sleep for `msec' milliseconds */
extern void accurateDelay (int milliseconds);
extern long int millisecondsBetween (const struct timeval *from, const struct timeval *to);
extern long int millisecondsSince (const struct timeval *from);
extern int hasTimedOut (int milliseconds);	/* test timeout condition */

extern void *mallocWrapper (size_t size);
extern void *reallocWrapper (void *address, size_t size);
extern char *strdupWrapper (const char *string);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_MISC */
