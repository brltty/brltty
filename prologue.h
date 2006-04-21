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

#define _CONCATENATE(a,b) a##b
#define CONCATENATE(a,b) _CONCATENATE(a,b)

#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)

#define ARRAY_COUNT(array) (sizeof((array)) / sizeof((array)[0]))
#define ARRAY_SIZE(pointer, count) ((count) * sizeof(*(pointer)))

#if defined(__CYGWIN32__) || defined(__MINGW32__)
#define WINDOWS

#include <w32api.h>
#define WINVER WindowsXP

#define __USE_W32_SOCKETS
#include <windows.h>

#endif /* WINDOWS */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef WINDOWS
typedef HANDLE FileDescriptor;
#else /* WINDOWS */
typedef int FileDescriptor;
#endif /* WINDOWS */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_ATTRIBUTE_PACKED
#define PACKED __attribute__((packed))
#else /* HAVE_ATTRIBUTE_PACKED */
#define PACKED
#endif /* HAVE_ATTRIBUTE_PACKED */

#ifdef HAVE_ATTRIBUTE_FORMAT_PRINTF
#define PRINTF(fmt,var) __attribute__((format(printf, fmt, var)))
#else /* HAVE_ATTRIBUTE_FORMAT_PRINTF */
#define PRINTF(fmt,var)
#endif /* HAVE_ATTRIBUTE_FORMAT_PRINTF */

#ifdef HAVE_ATTRIBUTE_UNUSED
#define UNUSED __attribute__((unused))
#else /* HAVE_ATTRIBUTE_UNUSED */
#define UNUSED
#endif /* HAVE_ATTRIBUTE_UNUSED */

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
