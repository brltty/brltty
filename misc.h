/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nicolas Pitre <nico@cam.org>
 * Stéphane Doyon <s.doyon@videotron.ca>
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* misc.h - Header file for miscellaneous all-purpose routines
 */

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

void LogOpen(int prio);
void LogClose();
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
