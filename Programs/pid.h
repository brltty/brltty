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

#ifndef BRLTTY_INCLUDED_PID
#define BRLTTY_INCLUDED_PID

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PID */
