/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#define BRLTTY_COPYRIGHT "Copyright (C) 1995-2006 by The BRLTTY Developers."
#define BRLTTY_URL "http://mielke.cc/brltty/"

#if defined(__CYGWIN32__) || defined(__MINGW32__)
#define WINDOWS

#define __USE_W32_SOCKETS
#include <windows.h>
#include <w32api.h>

#ifndef Windows95
#define Windows95 0x0400
#endif /* Windows95 */

#ifndef WindowsNT4
#define WindowsNT4 0x0400
#endif /* WindowsNT4 */

#ifndef Windows98
#define Windows98 0x0410
#endif /* Windows98 */

#ifndef WindowsME
#define WindowsME 0x0500
#endif /* WindowsME */

#endif /* WINDOWS */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef ENABLE_I18N_SUPPORT
#include <libintl.h>
#else /* ENABLE_I18N_SUPPORT */
#define gettext(string) (string)
#define ngettext(singular, plural, count) (((count) == 1)? (singular): (plural))
#endif /* ENABLE_I18N_SUPPORT */
#define strtext(string) string

#ifdef HAVE_SHMGET
#if SIZEOF_KEY_T == 4
#define PRIX_KEY_T PRIX32
#elif SIZEOF_KEY_T == 8
#define PRIX_KEY_T PRIX64
#else /* SIZEOF_KEY_T */
#error unsupported size for type key_t
#endif /* SIZEOF_KEY_T */
#endif /* HAVE_SHMGET */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PROLOGUE */
