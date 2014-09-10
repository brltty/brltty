/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#define PACKAGE_COPYRIGHT "Copyright (C) 1995-2014 by The BRLTTY Developers."

#define _CONCATENATE(a,b) a##b
#define CONCATENATE(a,b) _CONCATENATE(a,b)

#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)

#define MIN(a, b)  (((a) < (b))? (a): (b))
#define MAX(a, b)  (((a) > (b))? (a): (b))

#define ARRAY_COUNT(array) (sizeof((array)) / sizeof((array)[0]))
#define ARRAY_SIZE(pointer, count) ((count) * sizeof(*(pointer)))

#define SYMBOL_TYPE(name) name ## _t
#define SYMBOL_POINTER(name) static SYMBOL_TYPE(name) *name##_p = NULL;

#define VARIABLE_DECLARATION(variableName, variableType) \
  variableType variableName
#define VARIABLE_TYPEDEF(variableName, variableType) \
  typedef VARIABLE_DECLARATION(SYMBOL_TYPE(variableName), variableType)
#define VARIABLE_DECLARE(variableName, variableType) \
  VARIABLE_TYPEDEF(variableName, variableType); \
  extern SYMBOL_TYPE(variableName) variableName

#define FUNCTION_DECLARATION(functionName, returnType, argumentList) \
  returnType functionName argumentList
#define FUNCTION_TYPEDEF(functionName, returnType, argumentList) \
  typedef FUNCTION_DECLARATION(SYMBOL_TYPE(functionName), returnType, argumentList)
#define FUNCTION_DECLARE(functionName, returnType, argumentList) \
  FUNCTION_TYPEDEF(functionName, returnType, argumentList); \
  extern SYMBOL_TYPE(functionName) functionName

#define STR_BEGIN(buffer, size) { \
  char *strNext = (buffer); \
  char *strStart = strNext; \
  char *strEnd = strStart + (size); \
  *strNext = 0;
#define STR_END }
#define STR_LENGTH (size_t)(strNext - strStart)
#define STR_NEXT strNext
#define STR_LEFT (size_t)(strEnd - strNext)
#define STR_ADJUST(length) if ((strNext += (length)) > strEnd) strNext = strEnd
#define STR_PRINTF(format, ...) { \
  size_t strLength = snprintf(STR_NEXT, STR_LEFT, format, ## __VA_ARGS__); \
  STR_ADJUST(strLength); \
}
#define STR_VPRINTF(format, arguments) { \
  size_t strLength = vsnprintf(STR_NEXT, STR_LEFT, format, arguments); \
  STR_ADJUST(strLength); \
}

#ifdef HAVE_CONFIG_H
#ifdef FOR_BUILD
#include "forbuild.h"
#else /* FOR_BUILD */
#include "config.h"
#endif /* FOR_BUILD */
#endif /* HAVE_CONFIG_H */

#if defined(__CYGWIN__) || defined(__MINGW32__)
#define WINDOWS

#ifndef HAVE_SDKDDKVER_H
#include <w32api.h>
#define _WIN32_WINNT_NT4 WindowsNT4
#define _WIN32_WINNT_WIN95 Windows95
#define _WIN32_WINNT_WIN98 Windows98
#define _WIN32_WINNT_WINME WindowsME
#define _WIN32_WINNT_WIN2K Windows2000
#define _WIN32_WINNT_WINXP WindowsXP
#define _WIN32_WINNT_WS03 Windows2003
#define _WIN32_WINNT_VISTA WindowsVista
#endif /* HAVE_SDKDDKVER_H */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#endif /* _WIN32_WINNT */

#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif /* WINVER */

#ifdef __MINGW32__
#ifndef __USE_W32_SOCKETS
#define __USE_W32_SOCKETS
#endif /* __USE_W32_SOCKETS */

#include <ws2tcpip.h>
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
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#ifdef __MINGW32__
#if (__MINGW32_MAJOR_VERSION < 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION < 10))
extern int gettimeofday (struct timeval *tvp, void *tzp);
#endif /* gettimeofday */

#if (__MINGW32_MAJOR_VERSION < 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION < 15))
extern void usleep (int usec);
#endif /* usleep */
#endif /* __MINGW32__ */

#ifdef GRUB_RUNTIME
#undef NESTED_FUNC_ATTR
#define NESTED_FUNC_ATTR __attribute__((__regparm__(1)))

/* missing needed standard integer definitions */
#define INT16_MAX 0X7FFF
#define INT32_MAX 0X7FFFFFFF
#define UINT32_C(i) (i)
#define PRIuGRUB_UINT16_T "u"
#define PRIuGRUB_UINT8_T "u"

/* to get gettext() declared */
#define GRUB_POSIX_GETTEXT_DOMAIN "brltty"

/* disable the use of floating-point operations */
#define NO_FLOAT
#define float NO_FLOAT
#define double NO_DOUBLE
#endif /* GRUB_RUNHTIME */

#ifdef __MSDOS__
#include <stdarg.h>

extern int snprintf (char *str, size_t size, const char *format, ...);
extern int vsnprintf (char *str, size_t size, const char *format, va_list ap);

#define lstat(file_name, buf) stat(file_name, buf)
#endif /* __MSDOS__ */

#ifdef __MINGW32__
typedef HANDLE FileDescriptor;
#define INVALID_FILE_DESCRIPTOR INVALID_HANDLE_VALUE
#define PRIfd "p"
#define closeFileDescriptor(fd) CloseHandle(fd)

typedef SOCKET SocketDescriptor;
#define INVALID_SOCKET_DESCRIPTOR INVALID_SOCKET
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

#ifdef __CYGWIN__
#include <sys/cygwin.h>

#define getSocketError() errno
#define setErrno(error) errno = cygwin_internal(CW_GET_ERRNO_FROM_WINERROR, (error))
#else /* __CYGWIN__ */
#define getSocketError() WSAGetLastError()
#define setErrno(error) errno = win_toErrno((error))

#ifndef WIN_ERRNO_STORAGE_CLASS
#define WIN_ERRNO_STORAGE_CLASS
extern
#endif /* WIN_ERRNO_STORAGE_CLASS */
WIN_ERRNO_STORAGE_CLASS int win_toErrno (DWORD error);
#endif /* __CYGWIN__ */

#else /* WINDOWS */
#define getSystemError() errno
#define getSocketError() errno

#define setErrno(error)
#endif /* WINDOWS */

#define setSystemErrno() setErrno(getSystemError())
#define setSocketErrno() setErrno(getSocketError())

#if defined(__MINGW32__)
#define PRIsize "u"
#define PRIssize "d"

#else /* format for size_t and ssize_t */
#define PRIsize "zu"
#define PRIssize "zd"
#endif /* format for size_t and ssize_t */

#if defined(__CYGWIN__)
#define PRIkey "llX"
#else /* format for key_t */
#define PRIkey PRIX32
#endif /* format for key_t */

#if defined(__MSDOS__)
#undef WCHAR_MAX

#elif defined(HAVE_WCHAR_H)
#include <wchar.h>
#include <wctype.h>

#endif /* HAVE_WCHAR_H */

#ifdef WCHAR_MAX
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

typedef unsigned char mbstate_t;

static inline size_t
mbrtowc (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps) {
  if (!s) return 0;
  if (!n) return 0;
  if (pwc) *pwc = *s & 0XFF;
  if (!*s) return 0;
  return 1;
}

static inline size_t
wcrtomb (char *s, wchar_t wc, mbstate_t *ps) {
  if (s) *s = wc;
  return 1;
}

static inline int
mbsinit (const mbstate_t *ps) {
  return 1;
}

#define WC_C(wc) (wchar_t)wc
#define WS_C(ws) (const wchar_t *)ws
#define PRIwc "c"
#define PRIws "s"
#define iswLatin1(wc) (1)
#endif /* HAVE_WCHAR_H */

#ifdef WORDS_BIGENDIAN
#define CHARSET_ENDIAN_SUFFIX "BE"
#else /* WORDS_BIGENDIAN */
#define CHARSET_ENDIAN_SUFFIX "LE"
#endif /* WORDS_BIGENDIAN */

#define WCHAR_CHARSET ("UCS-" SIZEOF_WCHAR_T_STR CHARSET_ENDIAN_SUFFIX)

#ifndef HAVE_MEMPCPY
#define mempcpy(dest,src,size) (void *)((char *)memcpy((dest), (src), (size)) + (size))
#endif /* HAVE_MEMPCPY */

#ifndef HAVE_WMEMPCPY
#define wmempcpy(dest,src,count) (wmemcpy((dest), (src), (count)) + (count))
#endif /* HAVE_WMEMPCPY */

#ifndef WRITABLE_DIRECTORY
#define WRITABLE_DIRECTORY ""
#endif /* WRITABLE_DIRECTORY */

#ifndef PRINTF
#ifdef HAVE_ATTRIBUTE_FORMAT_PRINTF
#define PRINTF(fmt,var) __attribute__((format(__printf__, fmt, var)))
#else /* HAVE_ATTRIBUTE_FORMAT_PRINTF */
#define PRINTF(fmt,var)
#endif /* HAVE_ATTRIBUTE_FORMAT_PRINTF */
#endif /* PRINTF */

#ifndef NORETURN
#ifdef HAVE_ATTRIBUTE_NORETURN
#define NORETURN __attribute__((noreturn))
#else /* HAVE_ATTRIBUTE_NORETURN */
#define NORETURN
#endif /* HAVE_ATTRIBUTE_NORETURN */
#endif /* NORETURN */

#ifndef PACKED
#ifdef HAVE_ATTRIBUTE_PACKED
#define PACKED __attribute__((packed))
#else /* HAVE_ATTRIBUTE_PACKED */
#define PACKED
#endif /* HAVE_ATTRIBUTE_PACKED */
#endif /* PACKED */

#ifndef UNUSED
#ifdef HAVE_ATTRIBUTE_UNUSED
#define UNUSED __attribute__((unused))
#else /* HAVE_ATTRIBUTE_UNUSED */
#define UNUSED
#endif /* HAVE_ATTRIBUTE_UNUSED */
#endif /* UNUSED */

#ifdef ENABLE_I18N_SUPPORT
#include <libintl.h>
#else /* ENABLE_I18N_SUPPORT */
#define gettext(string) (string)
#define ngettext(singular, plural, count) (((count) == 1)? (singular): (plural))
#endif /* ENABLE_I18N_SUPPORT */
#define strtext(string) string

#ifndef USE_PKG_BEEP_NONE
#define HAVE_BEEP_SUPPORT
#endif /* USE_PKG_BEEP_NONE */

#ifndef USE_PKG_PCM_NONE
#define HAVE_PCM_SUPPORT
#endif /* USE_PKG_PCM_NONE */

#ifndef USE_PKG_MIDI_NONE
#define HAVE_MIDI_SUPPORT
#endif /* USE_PKG_MIDI_NONE */

#ifndef USE_PKG_FM_NONE
#define HAVE_FM_SUPPORT
#endif /* USE_PKG_FM_NONE */

/* configure is still making a few mistakes with respect to the grub environment */
#ifdef GRUB_RUNTIME

/* some headers exist but probably shouldn't */
#undef HAVE_SIGNAL_H

/* including <time.h> on its own yields compile errors */
#undef HAVE_DECL_LOCALTIME_R
#define HAVE_DECL_LOCALTIME_R 1

/* AC_CHECK_FUNC() is checking local libraries - these are the errors that matter */
#undef HAVE_FCHDIR
#undef HAVE_SELECT
#endif /* GRUB_RUNTIME */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PROLOGUE */
