/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifndef _MISC_H
#define _MISC_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>

#ifndef MIN
#define MIN(a, b)  (((a) < (b))? (a): (b)) 
#endif /* MIN */

#ifndef MAX
#define MAX(a, b)  (((a) > (b))? (a): (b)) 
#endif /* MAX */

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

#define _CONCATENATE(a,b) a##b
#define CONCATENATE(a,b) _CONCATENATE(a,b)

#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)

extern char **splitString (const char *string, char delimiter);
extern void deallocateStrings (char **array);

/* Process each line of an input text file safely.
 * This routine handles the actual reading of the file,
 * insuring that the input buffer is always big enough,
 * and calls a caller-supplied handler once for each line in the file.
 * The caller-supplied data pointer is passed straight through to the handler.
 */
extern int processLines (FILE *file, /* The input file. */
                         int (*handler) (char *line, void *data), /* The input line handler. */
		         void *data); /* A pointer to caller-specific data. */
extern int readLine (FILE *file, char **buffer, size_t *size);

/* Safe wrappers for system calls which handle the various scenarios
 * which can occur when signals interrupt them.
 * These wrappers, rather than the system calls they wrap, should be
 * called from within all drivers since certain tasks, i.e. cursor routing,
 * are performed by forked processes which generate a SIGCHLD when they terminate.
 * This approach also makes it safe for internal events to be scheduled
 * via signals like SIGALRM and SIGIO.
 */
size_t safe_write (int fd, const unsigned char *buffer, size_t length);

extern int changeOpenFlags (int fileDescriptor, int flagsToClear, int flagsToSet);
extern int setOpenFlags (int fileDescriptor, int state, int flags);
extern int setBlockingIo (int fileDescriptor, int state);
extern int setCloseOnExec (int fileDescriptor);

extern void delay (int milliseconds);		/* sleep for `msec' milliseconds */
extern void shortdelay (int milliseconds);
extern long int millisecondsBetween (const struct timeval *from, const struct timeval *to);
extern long int millisecondsSince (const struct timeval *from);
extern int timeout_yet (int milliseconds);	/* test timeout condition */

#ifdef HAVE_SYSLOG_H
#  include <syslog.h>
#else
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
#endif /* HAVE_SYSLOG_H */
extern void LogOpen(int toConsole);
extern void LogClose(void);
extern void LogPrint
       (int level, char *format, ...)
#ifdef HAVE_ATTRIBUTE_FORMAT_PRINTF
       __attribute__((format(printf, 2, 3)))
#endif /* HAVE_ATTRIBUTE_FORMAT_PRINTF */
       ;
extern void LogError (const char *action);
extern void LogBytes (const char *description, const unsigned char *data, unsigned int length);
extern int setLogLevel (int level);
extern int setPrintLevel (int level);
extern int setPrintOff (void);
extern int loggedProblemCount;

extern int getConsole (void);
extern int writeConsole (const unsigned char *address, size_t count);
extern int ringBell (void);

extern void *mallocWrapper (size_t size);
extern void *reallocWrapper (void *address, size_t size);
extern char *strdupWrapper (const char *string);
extern char *makePath (const char *directory, const char *file);
extern int makeDirectory (const char *path);

extern char *getDevicePath (const char *path);
extern int isQualifiedDevice (const char **path, const char *qualifier);
extern void unsupportedDevice (const char *path);

extern int awaitInput (int descriptor, int milliseconds);
extern int readChunk (
  int descriptor,
  unsigned char *buffer, int *offset, int count,
  int initialTimeout, int subsequentTimeout
);

extern int isInteger (int *value, const char *word);
extern int isFloat (float *value, const char *word);
extern int validateInteger (int *value, const char *description, const char *word, const int *minimum, const int *maximum);
extern int validateFloat (float *value, const char *description, const char *word, const float *minimum, const float *maximum);
extern int validateChoice (unsigned int *value, const char *description, const char *word, const char *const *choices);
extern int validateFlag (unsigned int *value, const char *description, const char *word, const char *on, const char *off);
extern int validateOnOff (unsigned int *value, const char *description, const char *word);
extern int validateYesNo (unsigned int *value, const char *description, const char *word);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MISC_H */
