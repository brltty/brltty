/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-1999 by The BRLTTY Team, All rights reserved.
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

/* misc.c - Miscellaneous all-purpose routines
 */

#define __EXTENSIONS__	/* for time.h */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>

#include "misc.h"

#ifdef USE_SYSLOG
#include <syslog.h>
#include <stdarg.h>
#endif

unsigned
elapsed_msec (struct timeval *t1, struct timeval *t2)
{
  unsigned diff, error = 0xFFFFFFFF;
  if (t1->tv_sec > t2->tv_sec)
    return (error);
  diff = (t2->tv_sec - t1->tv_sec) * 1000L;
  if (diff == 0 && t1->tv_usec > t2->tv_usec)
    return (error);
  diff += (t2->tv_usec - t1->tv_usec) / 1000L;
  return (diff);
}


void
shortdelay (unsigned msec)
{
  struct timeval start, now;
  struct timezone tz;
  gettimeofday (&start, &tz);
  do
    {
      gettimeofday (&now, &tz);
    }
  while (elapsed_msec (&start, &now) < msec);
}


void
delay (int msec)
{
  struct timeval del;

  del.tv_sec = 0;
  del.tv_usec = msec * 1000;
  select (0, NULL, NULL, NULL, &del);
}


int
timeout_yet (int msec)
{
  static struct timeval tstart =
  {0, 0};
  struct timeval tnow;

  if (msec == 0)		/* initialiseation */
    {
      gettimeofday (&tstart, NULL);
      return 0;
    }
  gettimeofday (&tnow, NULL);
  return ((tnow.tv_sec * 1e6 + tnow.tv_usec) - \
	  (tstart.tv_sec * 1e6 + tstart.tv_usec) >= msec * 1e3);
}

#ifdef USE_SYSLOG

static int LogPrio, LogOpened = 0;

void LogOpen(int prio)
{
  openlog("BRLTTY",LOG_CONS,LOG_DAEMON);
  LogPrio = prio;
  LogOpened = 1;
}

void LogClose()
{
  closelog();
  LogOpened = 0;
}

void LogPrint(int prio, char *fmt, ...)
{
  va_list argp;

  if(LogOpened && (prio & LOG_PRIMASK) <= LogPrio){
    va_start(argp, fmt);
    vsyslog(prio, fmt, argp);
    va_end(argp);
  }
}

void LogAndStderr(int prio, char *fmt, ...)
{
  va_list argp;

  va_start(argp, fmt);

  vfprintf(stderr, fmt, argp);
  fprintf(stderr,"\n");
  if(LogOpened && (prio & LOG_PRIMASK) <= LogPrio)
    vsyslog(prio, fmt, argp);

  va_end(argp);
}

#else /* don't USE_SYSLOG */

void LogOpen(int prio) {}
void LogClose() {}
void LogPrint(int prio, char *fmt, ...) {}
void LogAndStderr(int prio, char *fmt, ...) {}
#endif


/* helper funktion for papenmeier status */
/* special table for papenmeier */
#define B1 1
#define B2 2
#define B3 4
#define B4 8
#define B5 16
#define B6 32
#define B7 64
#define B8 128

const unsigned char pm_ones[11] = { B1+B5+B4, B2, B2+B5, 
				    B2+B1, B2+B1+B4, B2+B4, 
				    B2+B5+B1, B2+B5+B4+B1, B2+B5+B4, 
				    B5+B1, B1+B2+B4+B5 };
const unsigned char pm_tens[11] = { B8+B6+B3, B7, B7+B8, 
				    B7+B3, B7+B3+B6, B7+B6, 
				    B7+B8+B3, B7+B8+B3+B6, B7+B8+B6,
				    B8+B3, B3+B6+B7+B8};

/* create bits for number 0..99 - special for papenmeier */
int pm_num(int x)
{
  return pm_tens[(x / 10) % 10] | pm_ones[x % 10];  
}

/* status cell   tens: line number    ones: no/all bits set */
int pm_stat(int line, int on)
{
  if (on)
    return pm_tens[line%10] | pm_ones[10];
  else
    return pm_tens[line];
}
