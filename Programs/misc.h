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

#include <stdio.h>
#include <termios.h>
#include <sys/time.h>

/* misc.h - Header file for miscellaneous all-purpose routines
 */

#ifndef MIN
#define MIN(a, b)  (((a) < (b))? (a): (b)) 
#endif /* MIN */

#ifndef MAX
#define MAX(a, b)  (((a) > (b))? (a): (b)) 
#endif /* MAX */

#define _CONCATENATE(a,b) a##b
#define CONCATENATE(a,b) _CONCATENATE(a,b)

#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)

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

extern void setCloseOnExec (int fileDescriptor);

extern void delay (int milliseconds);		/* sleep for `msec' milliseconds */
extern void shortdelay (int milliseconds);
extern long int elapsedMilliseconds (const struct timeval *from, const struct timeval *to);
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
       __attribute__((format(printf, 2, 3)));
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

extern void unsupportedDevice (const char *path);
extern int isSerialDevice (const char **path);
extern int openSerialDevice (const char *path, int *descriptor, struct termios *attributes);
extern void rawSerialDevice (struct termios *attributes);
extern int setSerialDevice (int descriptor, struct termios *attributes, speed_t baud);
extern int resetSerialDevice (int descriptor, struct termios *attributes, speed_t baud);
extern int awaitInput (int descriptor, int milliseconds);
extern int readChunk (int descriptor, unsigned char *buffer, int *offset, int count, int timeout);

extern int isInteger (int *value, const char *word);
extern int isFloat (double *value, const char *word);
extern int validateInteger (int *value, const char *description, const char *word, const int *minimum, const int *maximum);
extern int validateFloat (double *value, const char *description, const char *word, const double *minimum, const double *maximum);
extern int validateBaud (speed_t *value, const char *description, const char *word, const unsigned int *choices);
extern int baud2integer (speed_t baud);
extern int validateChoice (unsigned int *value, const char *description, const char *word, const char *const *choices);
extern int validateFlag (unsigned int *value, const char *description, const char *word, const char *on, const char *off);
extern int validateOnOff (unsigned int *value, const char *description, const char *word);
extern int validateYesNo (unsigned int *value, const char *description, const char *word);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MISC_H */
