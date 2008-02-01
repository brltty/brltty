/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#define PACKAGE_COPYRIGHT "Copyright (C) 1995-2008 by The BRLTTY Developers."
#define PACKAGE_URL "http://mielke.cc/brltty/"

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

#ifdef __MINGW32__
#define __USE_W32_SOCKETS
#endif /* __MINGW32__ */
#include <windows.h>

#endif /* WINDOWS */

#ifdef __MINGW32__
#include <_mingw.h>
#endif /* __MINGW32__ */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __MSDOS__
#include <stdarg.h>

extern int snprintf (char *str, size_t size, const char *format, ...);
extern int vsnprintf (char *str, size_t size, const char *format, va_list ap);

#define lstat(file_name, buf) stat(file_name, buf)

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long		uint32_t;
typedef unsigned long long	uint64_t;
typedef signed char		int8_t;
typedef signed short		int16_t;
typedef signed long		int32_t;
typedef signed long long	int64_t;

#define INT8_C(c)	c
#define INT16_C(c)	c
#define INT32_C(c)	c
#define INT64_C(c)	c ## LL

#define UINT8_C(c)	c ## U
#define UINT16_C(c)	c ## U
#define UINT32_C(c)	c ## U
#define UINT64_C(c)	c ## ULL

#define INTMAX_C(c)	c ## LL
#define UINTMAX_C(c)	c ## ULL

#else /* __MSDOS__ */
#include <inttypes.h>
#endif /* __MSDOS__ */

#if 1
#include <wchar.h>

#define WC_C(wc) L##wc
#define WS_C(ws) L##ws
#define PRIwc "lc"
#define PRIws "ls"
#else /* wchar */
#include <string.h>
#include <ctype.h>

#define wchar_t char
#define wint_t int
#define WEOF EOF

#define wmemchr memchr
#define wmemcmp memcmp
#define wmemcpy memcpy
#define wmemmove memmove
#define wmemset memset

#define wcscasecmp strcasecmp
#define wcsncasecmp strncasecmp
#define wcscat strcat
#define wcsncat strncat
#define wcscmp strcmp
#define wcsncmp strncmp
#define wcscpy strcpy
#define wcsncpy strncpy
#define wcslen strlen
#define wcsnlen strnlen

#define wcschr strchr
#define wcscoll strcoll
#define wcscspn strcspn
#define wcsdup strdup
#define wcspbrk strpbrk
#define wcsrchr strrchr
#define wcsspn strspn
#define wcsstr strstr
#define wcstok strtok
#define wcswcs strstr
#define wcsxfrm strxfrm

#define iswalnum isalnum
#define iswalpha isalpha
#define iswblank isblank
#define iswcntrl iscntrl
#define iswdigit isdigit
#define iswgraph isgraph
#define iswlower islower
#define iswprint isprint
#define iswpunct ispunct
#define iswspace isspace
#define iswupper isupper
#define iswxdigit isxdigit

#define towlower tolower
#define towupper toupper

#define swprintf snprintf

#define WC_C(wc) wc
#define WS_C(ws) ws
#define PRIwc "c"
#define PRIws "s"
#endif /* wchar */

#ifdef __MINGW32__
typedef HANDLE FileDescriptor;
#define INVALID_FILE_DESCRIPTOR INVALID_HANDLE_VALUE
#define PRIFD "p"
#define closeFileDescriptor(fd) CloseHandle(fd)

typedef SOCKET SocketDescriptor;
#define INVALID_SOCKET_DESCRIPTOR -1
#define PRISD "d"
#define closeSocketDescriptor(sd) closesocket(sd)
#else /* __MINGW32__ */
typedef int FileDescriptor;
#define INVALID_FILE_DESCRIPTOR -1
#define PRIFD "d"
#define closeFileDescriptor(fd) close(fd)

typedef int SocketDescriptor;
#define INVALID_SOCKET_DESCRIPTOR -1
#define PRISD "d"
#define closeSocketDescriptor(sd) close(sd)
#endif /* __MINGW32__ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef WRITABLE_DIRECTORY
#define WRITABLE_DIRECTORY ""
#endif /* WRITABLE_DIRECTORY */

#ifndef PACKED
#ifdef HAVE_ATTRIBUTE_PACKED
#define PACKED __attribute__((packed))
#else /* HAVE_ATTRIBUTE_PACKED */
#define PACKED
#endif /* HAVE_ATTRIBUTE_PACKED */
#endif /* PACKED */

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
