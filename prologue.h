/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_PROLOGUE
#define BRLTTY_INCLUDED_PROLOGUE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PACKAGE_COPYRIGHT "Copyright (C) 1995-2011 by The BRLTTY Developers."
#define PACKAGE_URL "http://mielke.cc/brltty/"

#define _CONCATENATE(a,b) a##b
#define CONCATENATE(a,b) _CONCATENATE(a,b)

#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)

#define MIN(a, b)  (((a) < (b))? (a): (b)) 
#define MAX(a, b)  (((a) > (b))? (a): (b)) 

#define ARRAY_COUNT(array) (sizeof((array)) / sizeof((array)[0]))
#define ARRAY_SIZE(pointer, count) ((count) * sizeof(*(pointer)))

#define STR_BEGIN(buffer, size) { \
  char *strNext = (buffer); \
  char *strStart = strNext; \
  char *strEnd = strStart + (size);
#define STR_END }
#define STR_LENGTH (strNext - strStart)
#define STR_NEXT strNext
#define STR_LEFT (strEnd - strNext)
#define STR_ADJUST(length) if ((strNext += (length)) > strEnd) strNext = strEnd
#define STR_PRINTF(format, ...) { \
  int strLength; \
  snprintf(STR_NEXT, STR_LEFT, format "%n", ## __VA_ARGS__, &strLength); \
  STR_ADJUST(strLength); \
}

#if defined(__CYGWIN32__) || defined(__MINGW32__)
#define WINDOWS

#include <w32api.h>
#define WINVER WindowsXP

#ifdef __MINGW32__
#ifndef __USE_W32_SOCKETS
#define __USE_W32_SOCKETS
#endif /* __USE_W32_SOCKETS */
#endif /* __MINGW32__ */

#include <windows.h>
#endif /* WINDOWS */

#ifdef __MINGW32__
#include <_mingw.h>
#endif /* __MINGW32__ */

/*
 * The (poorly named) macro "interface" is unfortunately defined within
 * Windows headers. Fortunately, though, it's also only ever used within
 * them so it's safe to undefine it here.
 */
#ifdef interface
#undef interface
#endif /* interface */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __MINGW32__
#if (__MINGW32_MAJOR_VERSION < 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION < 10))
extern int gettimeofday (struct timeval *tvp, void *tzp);
#endif /* gettimeofday */

#if (__MINGW32_MAJOR_VERSION < 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION < 15))
extern void usleep (int usec);
#endif /* usleep */
#endif /* __MINGW32__ */

#ifdef __MSDOS__
#include <stdarg.h>

extern int snprintf (char *str, size_t size, const char *format, ...);
extern int vsnprintf (char *str, size_t size, const char *format, va_list ap);

#define lstat(file_name, buf) stat(file_name, buf)

typedef signed char		int8_t;
typedef signed short		int16_t;
typedef signed long		int32_t;
typedef signed long long	int64_t;
typedef int64_t	                intmax_t;

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long		uint32_t;
typedef unsigned long long	uint64_t;
typedef uint64_t        	uintmax_t;

#define INT8_C(c)	c
#define INT16_C(c)	c
#define INT32_C(c)	c ## L
#define INT64_C(c)	c ## LL
#define INTMAX_C(c)	INT64_C(c)

#define UINT8_C(c)	c ## U
#define UINT16_C(c)	c ## U
#define UINT32_C(c)	c ## UL
#define UINT64_C(c)	c ## ULL
#define UINTMAX_C(c)	UINT64_C(c)

#define INT8_MIN     INT8_C(0X80)
#define INT16_MIN    INT16_C(0X8000)
#define INT32_MIN    INT32_C(0X80000000)
#define INT64_MIN    INT64_C(0X8000000000000000)

#define INT8_MAX     INT8_C(0X7F)
#define INT16_MAX    INT16_C(0X7FFF)
#define INT32_MAX    INT32_C(0X7FFFFFFF)
#define INT64_MAX    INT64_C(0X7FFFFFFFFFFFFFFF)

#define UINT8_MIN     UINT8_C(0)
#define UINT16_MIN    UINT16_C(0)
#define UINT32_MIN    UINT32_C(0)
#define UINT64_MIN    UINT64_C(0)

#define UINT8_MAX     UINT8_C(0XFF)
#define UINT16_MAX    UINT16_C(0XFFFF)
#define UINT32_MAX    UINT32_C(0XFFFFFFFF)
#define UINT64_MAX    UINT64_C(0XFFFFFFFFFFFFFFFF)

#define PRId8 "d"
#define PRIi8 "i"
#define PRIu8 "u"
#define PRIo8 "o"
#define PRIx8 "x"
#define PRIX8 "X"

#define PRId16 "d"
#define PRIi16 "i"
#define PRIu16 "u"
#define PRIo16 "o"
#define PRIx16 "x"
#define PRIX16 "X"

#define PRId32 "ld"
#define PRIi32 "li"
#define PRIu32 "lu"
#define PRIo32 "lo"
#define PRIx32 "lx"
#define PRIX32 "lX"

#define PRId64 "lld"
#define PRIi64 "lli"
#define PRIu64 "llu"
#define PRIo64 "llo"
#define PRIx64 "llx"
#define PRIX64 "llX"

#define PRIdMAX PRId64
#define PRIiMAX PRIi64
#define PRIuMAX PRIu64
#define PRIoMAX PRIo64
#define PRIxMAX PRIx64
#define PRIXMAX PRIX64

#else /* __MSDOS__ */
#include <inttypes.h>
#endif /* __MSDOS__ */

#ifdef __MINGW32__
typedef HANDLE FileDescriptor;
#define INVALID_FILE_DESCRIPTOR INVALID_HANDLE_VALUE
#define PRIfd "p"
#define closeFileDescriptor(fd) CloseHandle(fd)

typedef SOCKET SocketDescriptor;
#define INVALID_SOCKET_DESCRIPTOR -1
#define PRIsd "d"
#define closeSocketDescriptor(sd) closesocket(sd)
#else /* __MINGW32__ */
typedef int FileDescriptor;
#define INVALID_FILE_DESCRIPTOR -1
#define PRIfd "d"
#define closeFileDescriptor(fd) close(fd)

typedef int SocketDescriptor;
#define INVALID_SOCKET_DESCRIPTOR -1
#define PRIsd "d"
#define closeSocketDescriptor(sd) close(sd)
#endif /* __MINGW32__ */

#ifdef WINDOWS
#define getSystemError() GetLastError()

#ifdef __CYGWIN32__
#include <sys/cygwin.h>

#define getSocketError() errno
#define setErrno(error) errno = cygwin_internal(CW_GET_ERRNO_FROM_WINERROR, (error))
#else /* __CYGWIN32__ */
#define getSocketError() WSAGetLastError()
#define setErrno(error) errno = win_toErrno((error))

#ifndef WIN_ERRNO_STORAGE_CLASS
#define WIN_ERRNO_STORAGE_CLASS
extern
#endif /* WIN_ERRNO_STORAGE_CLASS */
WIN_ERRNO_STORAGE_CLASS int win_toErrno (DWORD error);
#endif /* __CYGWIN32__ */

#else /* WINDOWS */
#define getSystemError() errno
#define getSocketError() errno

#define setErrno(error)
#endif /* WINDOWS */

#define setSystemErrno() setErrno(getSystemError())
#define setSocketErrno() setErrno(getSocketError())

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_WCHAR_H 
#include <wchar.h>
#include <wctype.h>

#define convertTextToWchars mbstowcs

#define WC_C(wc) L##wc
#define WS_C(ws) L##ws
#define PRIwc "lc"
#define PRIws "ls"
#define iswLatin1(wc) ((wc) < 0X100)
#else /* HAVE_WCHAR_H */
#include <ctype.h>
#include <string.h>

#define wchar_t unsigned char
#define wint_t int
#define WEOF EOF
#define WCHAR_MAX UINT8_MAX

#define wmemchr(source,character,count) memchr((const char *)(source), (char)(character), (count))
#define wmemcmp(source1,source2,count) memcmp((const char *)(source1), (const char *)(source2), (count))
#define wmemcpy(target,source,count) memcpy((char *)(target), (const char *)(source), (count))
#define wmemmove(target,source,count) memmove((char *)(target), (const char *)(source), (count))
#define wmemset(target,character,count) memset((char *)(target), (char)(character), (count))

#define wcscasecmp(source1,source2) strcasecmp((const char *)(source1), (const char *)(source2))
#define wcsncasecmp(source1,source2,count) strncasecmp((const char *)(source1), (const char *)(source2), (count))
#define wcscat(target,source) strcat((char *)(target), (const char *)(source))
#define wcsncat(target,source,count) strncat((char *)(target), (const char *)(source), (count))
#define wcscmp(source1,source2) strcmp((const char *)(source1), (const char *)(source2))
#define wcsncmp(source1,source2,count) strncmp((const char *)(source1), (const char *)(source2), (count))
#define wcscpy(target,source) strcpy((char *)(target), (const char *)(source))
#define wcsncpy(target,source,count) strncpy((char *)(target), (const char *)(source), (count))
#define wcslen(source) strlen((const char *)(source))
#define wcsnlen(source,count) strnlen((const char *)(source), (count))

#define wcschr(source,character) strchr((const char *)(source), (char)(character))
#define wcscoll(source1,source2) strcoll((const char *)(source1), (const char *)(source2))
#define wcscspn(source,reject) strcspn((const char *)(source), (const char *)(reject))
#define wcsdup(source) strdup((const char *)(source))
#define wcspbrk(source,accept) strpbrk((const char *)(source), (const char *)(accept))
#define wcsrchr(source,character) strrchr((const char *)(source), (char)(character))
#define wcsspn(source,accept) strspn((const char *)(source), (const char *)(accept))
#define wcsstr(source,substring) strstr((const char *)(source), (const char *)(substring))
#define wcstok(target,delimiters,end) strtok_r((char *)(target), (const char *)(delimiters), (char **)(end))
#define wcswcs(source,substring) strstr((const char *)(source), (const char *)(substring))
#define wcsxfrm(target,source,count) strxfrm((char *)(target), (const char *)(source), (count))

#define wcstol(source,end,base) strtol((const char *)(source), (char **)(end), (base))
#define wcstoll(source,end,base) strtoll((const char *)(source), (char **)(end), (base))

#define iswalnum(character) isalnum((int)(character))
#define iswalpha(character) isalpha((int)(character))
#define iswblank(character) isblank((int)(character))
#define iswcntrl(character) iscntrl((int)(character))
#define iswdigit(character) isdigit((int)(character))
#define iswgraph(character) isgraph((int)(character))
#define iswlower(character) islower((int)(character))
#define iswprint(character) isprint((int)(character))
#define iswpunct(character) ispunct((int)(character))
#define iswspace(character) isspace((int)(character))
#define iswupper(character) isupper((int)(character))
#define iswxdigit(character) isxdigit((int)(character))

#define towlower(character) tolower((int)(character))
#define towupper(character) toupper((int)(character))

#define swprintf(target,count,source,...) snprintf((char *)(target), (count), (const char *)(source), ## __VA_ARGS__)
#define vswprintf(target,count,source,args) vsnprintf((char *)(target), (count), (const char *)(source), (args))

static inline size_t
convertTextToWchars (wchar_t *characters, const char *text, size_t size) {
  size_t length = strlen(text);

  if (characters) {
    if (length > size) {
      length = size;
    } else if (length < size) {
      length += 1;
    }

    memcpy(characters, text, length);
  }

  return length;
}

#define WC_C(wc) (wchar_t)wc
#define WS_C(ws) (const wchar_t *)ws
#define PRIwc "c"
#define PRIws "s"
#define iswLatin1(wc) (1)
#endif /* HAVE_WCHAR_H */

#ifndef HAVE_MEMPCPY
#define mempcpy(dest,src,size) (void *)((char *)memcpy((dest), (src), (size)) + (size))
#endif /* HAVE_MEMPCPY */

#ifndef HAVE_WMEMPCPY
#define wmempcpy(dest,src,count) (wmemcpy((dest), (src), (count)) + (count))
#endif /* HAVE_WMEMPCPY */

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
#define PRINTF(fmt,var) __attribute__((format(__printf__, fmt, var)))
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
