/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.9.0, 06 April 1998
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <s.doyon@videotron.ca>
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

#include <sys/time.h>
#include <sys/types.h>


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
