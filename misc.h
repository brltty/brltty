/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

#include <stdio.h>

/* misc.h - Header file for miscellaneous all-purpose routines
 */

// Process each line of an input text file safely.
// This routine handles the actual reading of the file,
// insuring that the input buffer is always big enough,
// and calls a caller-supplied handler once for each line in the file.
// The caller-supplied data pointer is passed straight through to the handler.
void process_lines (FILE *file, // The input file.
                    void (*handler) (char *line, void *data), // The input line handler.
		    void *data); // A pointer to caller-specific data.

// Safe wrappers for system calls which handle the various scenarios
// which can occur when signals interrupt them.
// These wrappers, rather than the system calls they wrap, should be
// called from within all drivers since certain tasks, i.e. cursor routing,
// are performed by forked processes which generate a SIGCHLD when they terminate.
// This approach also makes it safe for internal events to be scheduled
// via signals like SIGALRM and SIGIO.
size_t safe_read (int fd, unsigned char *buffer, size_t length);
size_t safe_write (int fd, const unsigned char *buffer, size_t length);

#define __EXTENSIONS__
#include <sys/time.h>

/* should syslog be used? may not be portable... */
#define USE_SYSLOG 1
#ifdef USE_SYSLOG
#include <syslog.h>
#endif

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

unsigned elapsed_msec (struct timeval *t1, struct timeval *t2);
void shortdelay (unsigned msec);
void delay (int msec);		/* sleep for `msec' milliseconds */
int timeout_yet (int msec);	/* test timeout condition */

void LogOpen(void);
void LogClose(void);
void SetLogPrio(int prio);
void SetErrPrio(int prio);
void LogPrint(int prio, char *fmt, ...);
void LogAndStderr(int prio, char *fmt, ...);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#ifndef USE_SYSLOG
/* provide dummy definitions for log priorities so it will compile */
#define LOG_EMERG       0
#define LOG_ALERT       0
#define LOG_CRIT        0
#define LOG_ERR         0
#define LOG_WARNING     0
#define LOG_NOTICE      0
#define LOG_INFO        0
#define LOG_DEBUG       0
#endif

/* Formatting of status cells. */
extern int landscape_number(int x);
extern int landscape_flag(int number, int on);
extern int seascape_number(int x);
extern int seascape_flag(int number, int on);
