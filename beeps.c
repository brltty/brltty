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

/* beeps.c - console beeps used by the system
 * $Id: beeps.c,v 1.3 1996/09/24 01:04:24 nn201 Exp $
 */

#ifdef linux

#include <sys/ioctl.h>
#include <linux/kd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#endif

#include "misc.h"

#include "beeps-songs.h"

short sound;


void
soundstat (short stat)
{
  sound = stat;
}


#ifdef linux

void
snd (int freq, int del, int consolefd)
{
  (void) ioctl (consolefd, KIOCSOUND, freq);
  shortdelay (del);
}

void
nosnd (int consolefd)
{
  (void) ioctl (consolefd, KIOCSOUND, 0);
}

void
play (int song[])
{
  if (sound)
    {
      int consolefd;
      consolefd = open ("/dev/console", O_WRONLY);
      if (consolefd < 0)
	return;
      while (*song != 0)
	{
	  int freq, del;
	  freq = *(song++);
	  del = *(song++);
	  snd (freq, del, consolefd);
	}
      nosnd (consolefd);
      close (consolefd);
    }
}

#else

void
play (int song[])
{
}

#endif
