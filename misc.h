/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

#include <stdio.h>
#include <termios.h>

/* misc.h - Header file for miscellaneous all-purpose routines
 */

#define MIN(a, b)  (((a) < (b))? (a): (b)) 
#define MAX(a, b)  (((a) > (b))? (a): (b)) 
	
// Process each line of an input text file safely.
// This routine handles the actual reading of the file,
// insuring that the input buffer is always big enough,
// and calls a caller-supplied handler once for each line in the file.
// The caller-supplied data pointer is passed straight through to the handler.
extern int processLines (FILE *file, // The input file.
                    void (*handler) (char *line, void *data), // The input line handler.
		    void *data); // A pointer to caller-specific data.

// Safe wrappers for system calls which handle the various scenarios
// which can occur when signals interrupt them.
// These wrappers, rather than the system calls they wrap, should be
// called from within all drivers since certain tasks, i.e. cursor routing,
// are performed by forked processes which generate a SIGCHLD when they terminate.
// This approach also makes it safe for internal events to be scheduled
// via signals like SIGALRM and SIGIO.
size_t safe_write (int fd, const unsigned char *buffer, size_t length);

#define __EXTENSIONS__
#include <sys/time.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void setCloseOnExec (int fileDescriptor);

extern void delay (int msec);		/* sleep for `msec' milliseconds */
extern void shortdelay (unsigned msec);
extern unsigned long elapsed_msec (struct timeval *t1, struct timeval *t2);
extern int timeout_yet (int msec);	/* test timeout condition */

/* should syslog be used? may not be portable... */
#define USE_SYSLOG 1
#ifdef USE_SYSLOG
   #include <syslog.h>
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
#endif
extern void LogOpen(int toConsole);
extern void LogClose(void);
extern void LogPrint
       (int level, char *format, ...)
       __attribute__((format(printf, 2, 3)));
extern void LogError (const char *action);
extern void LogBytes (const char *description, const unsigned char *data, unsigned int length);
extern void SetLogLevel(int level);
extern void SetStderrLevel(int level);
extern void SetStderrOff(void);
extern int ProblemCount;

extern int getConsole (void);
extern int writeConsole (const unsigned char *address, size_t count);
extern int ringBell (void);
extern int timedBeep (unsigned short frequency, unsigned short milliseconds);

extern void *mallocWrapper (size_t size);
extern void *reallocWrapper (void *address, size_t size);
extern char *strdupWrapper (const char *string);

extern int openSerialDevice (const char *path, int *descriptor, struct termios *attributes);
extern int setSerialDevice (int descriptor, struct termios *attributes, speed_t baud);
extern int resetSerialDevice (int descriptor, struct termios *attributes, speed_t baud);
extern int awaitInput (int descriptor, int milliseconds);
extern int readChunk (int descriptor, unsigned char *buffer, int *offset, int count, int timeout);

extern int validateInteger (int *integer, const char *description, const char *value, const int *minimum, const int *maximum);
extern int validateBaud (speed_t *baud, const char *description, const char *value, const unsigned int *choices);
extern int baud2integer (speed_t baud);
extern int validateChoice (unsigned int *choice, const char *description, const char *value, const char *const *choices);
extern int validateFlag (unsigned int *flag, const char *description, const char *value, const char *on, const char *off);
extern int validateOnOff (unsigned int *flag, const char *description, const char *value);
extern int validateYesNo (unsigned int *flag, const char *description, const char *value);

/* Formatting of status cells. */
extern const unsigned char landscapeDigits[11];
extern int landscapeNumber (int x);
extern int landscapeFlag (int number, int on);
extern const unsigned char seascapeDigits[11];
extern int seascapeNumber (int x);
extern int seascapeFlag (int number, int on);
extern const unsigned char portraitDigits[11];
extern int portraitNumber (int x);
extern int portraitFlag (int number, int on);

extern void reverseTable(unsigned char *origtab, unsigned char *revtab);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !defined(_MISC_H) */
