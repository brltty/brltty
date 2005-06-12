/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#ifndef BRLTTY_INCLUDED_PROLOGUE
#define BRLTTY_INCLUDED_PROLOGUE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined(__CYGWIN32__) || defined(__MINGW32__)
#define WINDOWS

#define __USE_W32_SOCKETS
#include <windows.h>
#endif /* defined(__CYGWIN32__) || defined(__MINGW32__) */
#include <sys/types.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PROLOGUE */
