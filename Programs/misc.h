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

#ifndef BRLTTY_INCLUDED_MISC
#define BRLTTY_INCLUDED_MISC

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "prologue.h"

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

/* These macros are meant for internal use only. */
#define BITMASK_ELEMENT_SIZE(element) (sizeof(element) * 8)
#define BITMASK_INDEX(bit,size) ((bit) / (size))
#define BITMASK_SHIFT(bit,size) ((bit) % (size))
#define BITMASK_ELEMENT_COUNT(bits,size) (BITMASK_INDEX((bits)-1, (size)) + 1)
#define BITMASK_ELEMENT(name,bit) ((name)[BITMASK_INDEX((bit), BITMASK_ELEMENT_SIZE((name)[0]))])
#define BITMASK_BIT(name,bit) (1 << BITMASK_SHIFT((bit), BITMASK_ELEMENT_SIZE((name)[0])))

/* These macros are for public use. */
#define BITMASK(name,bits,type) unsigned type name[BITMASK_ELEMENT_COUNT((bits), BITMASK_ELEMENT_SIZE(type))]
#define BITMASK_SIZE(name) BITMASK_ELEMENT_SIZE((name))
#define BITMASK_SET(name,bit) (BITMASK_ELEMENT((name), (bit)) |= BITMASK_BIT((name), (bit)))
#define BITMASK_CLEAR(name,bit) (BITMASK_ELEMENT((name), (bit)) &= ~BITMASK_BIT((name), (bit)))
#define BITMASK_TEST(name,bit) (BITMASK_ELEMENT((name), (bit)) & BITMASK_BIT((name), (bit)))

#ifndef MIN
#define MIN(a, b)  (((a) < (b))? (a): (b)) 
#endif /* MIN */

#ifndef MAX
#define MAX(a, b)  (((a) > (b))? (a): (b)) 
#endif /* MAX */

#define DEFINE_POINTER_TO(name,prefix) typeof(name) *prefix##name

#define HIGH_NIBBLE(byte) ((byte) & 0XF0)
#define LOW_NIBBLE(byte) ((byte) & 0XF)

#define getSameEndian(from) (from)
#define getOtherEndian(from) ((((from) & 0XFF) << 8) | (((from) >> 8) & 0XFF))
#define putSameEndian(to, from) (*(to) = getSameEndian((from)))
#define putOtherEndian(to, from) putSameEndian((to), getOtherEndian((from)))
#ifdef WORDS_BIGENDIAN
#  define getLittleEndian getOtherEndian
#  define putLittleEndian putOtherEndian
#  define getBigEndian getSameEndian
#  define putBigEndian putSameEndian
#else /* WORDS_BIGENDIAN */
#  define getLittleEndian getSameEndian
#  define putLittleEndian putSameEndian
#  define getBigEndian getOtherEndian
#  define putBigEndian putOtherEndian
#endif /* WORDS_BIGENDIAN */

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

extern char **splitString (const char *string, char delimiter, int *count);
extern void deallocateStrings (char **array);
extern char *joinStrings (const char *const *strings, int count);

extern FILE *openFile (const char *path, const char *mode);
extern FILE *openDataFile (const char *path, const char *mode);

extern int processLines (
  FILE *file,
  int (*handler) (char *line, void *data),
  void *data
);
extern int readLine (
  FILE *file,
  char **buffer,
  size_t *size
);
extern void formatInputError (
  char *buffer,
  int size,
  const char *file,
  const int *line,
  const char *format,
  va_list argp
);

#ifdef __MINGW32__
extern int gettimeofday (struct timeval *tvp, void *tzp);
extern void usleep (int usec);
#endif /* __MINGW32__ */

extern void approximateDelay (int milliseconds);		/* sleep for `msec' milliseconds */
extern void accurateDelay (int milliseconds);
extern long int millisecondsBetween (const struct timeval *from, const struct timeval *to);
extern long int millisecondsSince (const struct timeval *from);
extern int hasTimedOut (int milliseconds);	/* test timeout condition */

#if defined(HAVE_SYSLOG_H)
#include <syslog.h>
#else /* no system log */
typedef enum {
  LOG_EMERG,
  LOG_ALERT,
  LOG_CRIT,
  LOG_ERR,
  LOG_WARNING,
  LOG_NOTICE,
  LOG_INFO,
  LOG_DEBUG
} SyslogLevel;
#endif /* system log external definitions */

extern void LogOpen(int toConsole);
extern void LogClose(void);
extern void LogPrint
       (int level, const char *format, ...) PRINTF(2, 3);
extern void LogError (const char *action);
#ifdef WINDOWS
extern void LogWindowsCodeError (DWORD code, const char *action);
extern void LogWindowsError (const char *action);
#ifdef __MINGW32__
extern void LogWindowsSocketError (const char *action);
#endif /* __MINGW32__ */
#endif /* WINDOWS */
extern void LogBytes (int level, const char *description, const unsigned char *data, unsigned int length);
extern int setLogLevel (int level);
extern const char *setPrintPrefix (const char *prefix);
extern int setPrintLevel (int level);
extern int setPrintOff (void);

extern int getConsole (void);
extern int writeConsole (const unsigned char *address, size_t count);
extern int ringBell (void);

extern void *mallocWrapper (size_t size);
extern void *reallocWrapper (void *address, size_t size);
extern char *strdupWrapper (const char *string);

extern int isPathDelimiter (const char character);
extern int isAbsolutePath (const char *path);
extern char *getPathDirectory (const char *path);
extern const char *locatePathName (const char *path);
extern int isExplicitPath (const char *path);

extern char *makePath (const char *directory, const char *file);
extern void fixPath (char **path, const char *extension, const char *prefix);
extern int makeDirectory (const char *path);

extern const char *writableDirectory;
extern const char *getWritableDirectory (void);
extern char *makeWritablePath (const char *file);

extern char *getWorkingDirectory (void);
extern int setWorkingDirectory (const char *directory);

extern char *getHomeDirectory (void);
extern char *getUserDirectory (void);

extern const char *getDeviceDirectory (void);
extern char *getDevicePath (const char *device);
extern const char *resolveDeviceName (const char *const *names, const char *description, int mode);

extern int isQualifiedDevice (const char **path, const char *qualifier);
extern void unsupportedDevice (const char *path);

#undef ALLOW_DOS_DEVICE_NAMES
#if defined(__MSDOS__) || (defined(WINDOWS) && !defined(__CYGWIN32__))
#define ALLOW_DOS_DEVICE_NAMES 1
extern int isDosDevice (const char *path, const char *prefix);
#endif /* DOS or Windows (but not Cygwin) */

extern int rescaleInteger (int value, int from, int to);

extern int isInteger (int *value, const char *string);
extern int isFloat (float *value, const char *string);

extern int validateInteger (int *value, const char *string, const int *minimum, const int *maximum);
extern int validateFloat (float *value, const char *string, const float *minimum, const float *maximum);
extern int validateChoice (unsigned int *value, const char *string, const char *const *choices);
extern int validateFlag (unsigned int *value, const char *string, const char *on, const char *off);
extern int validateOnOff (unsigned int *value, const char *string);
extern int validateYesNo (unsigned int *value, const char *string);

extern char **getParameters (const char *const *names, const char *qualifier, const char *parameters);
extern void logParameters (const char *const *names, char **values, char *description);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_MISC */
