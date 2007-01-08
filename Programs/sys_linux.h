/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SYS_LINUX
#define BRLTTY_INCLUDED_SYS_LINUX

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int installKernelModule (const char *name, int *status);
extern int openCharacterDevice (const char *path, int flags, int major, int minor);
extern int getUinputDevice (void);
extern int writeKeyEvent (int key, int press);
extern void releaseAllKeys (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SYS_LINUX */
