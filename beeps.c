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

/* beeps.c - console beeps used by the system
 */

#ifdef linux

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kd.h>

#endif

#include "misc.h"


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

#if 0
void
snd_ndelay (int freq, int del, int consolefd)
{
/* KDMKTONE is non-blocking. Appropriate for last beep of a sound. Should
   work around the kernel 2.0.35 beep bug */
  (void) ioctl (consolefd, KDMKTONE, (del<<16) | freq );
}
#endif

void
nosnd (int consolefd)
{
/*
  (void) ioctl (consolefd, KIOCSOUND, 0);
*/
  /* alternatively?? */
  (void) ioctl (consolefd, KDMKTONE, 0);
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
	  /*
	  if(*song != 0)
	    snd (freq, del, consolefd);
	  else{
	    if(del >= 10)
	      snd_ndelay (freq, del, consolefd);
	    else{
	      snd(freq, del, consolefd);
	      snd_ndelay(1, 10, consolefd);
	    }
	  }
	  */
	}
      nosnd(consolefd);
      close (consolefd);
    }
}

#else

void
play (int song[])
{
}
#endif

/* Definitions of beep "tunes" for various events */

int snd_detected[] =
{3600, 60, 2700, 100, 0};
int snd_brloff[] =
{3600, 60, 5400, 60, 0};

int snd_link[] =
{1400, 7, 1500, 7, 1800, 12, 0};

int snd_unlink[] =
{1600, 7, 1500, 7, 1200, 20, 0};

/*int snd_wrap_down[] = {8000, 5, 4000, 7, 2000, 9, 1000, 9, 500, 5,0}; */

int snd_wrap_down[] =
{8000, 10, 4000, 7, 2000, 8, 1000, 6, 0};

/*int snd_wrap_up[] = {1000, 4, 2000, 25, 4000, 25, 8000, 20,0}; */

int snd_wrap_up[] =
{8000, 10, 4000, 7, 2000, 8, 1000, 6, 0};

/*int snd_bounce[] = {3000, 30, 2260, 20, 4520, 18,0}; */

int snd_bounce[] =
{500, 5, 1000, 6, 2000, 8, 4000, 7, 8000, 10, 0};

int snd_freeze[] =
{5000, 5, 4800, 5, 4600, 5, 4400, 5, 4200, 5, 4000, 5, 3800, 5,
 3600, 5, 3400, 5, 3200, 5, 3000, 5, 2800, 5, 2600, 5, 2400, 5,
 2200, 5, 2000, 5, 1800, 5, 1600, 5, 1400, 5, 1200, 5, 1000, 5,
 800, 5, 600, 5, 0};

int snd_unfreeze[] =
{800, 5, 1000, 5, 1200, 5, 1400, 5, 1600, 5, 1800, 5, 2000, 5,
 2200, 5, 2400, 5, 2600, 5, 2800, 5, 3000, 5, 3200, 5, 3400, 5,
 3600, 5, 3800, 5, 4000, 5, 4200, 5, 4400, 5, 4600, 5, 4800, 5,
 5000, 5, 0};

/*int snd_cut_beg[] = {4000, 10, 3000, 10, 4000, 10, 3000, 10, 4000, 10, 3000,
   10, 4000, 10, 3000, 10, 2000, 120, 1000, 10, 0}; */

/*int snd_cut_end[] = {4000, 10, 3000, 10, 4000, 10, 3000, 10, 4000, 10, 3000,
   10, 4000, 10, 3000, 10, 1000, 70, 2000, 30, 0}; */

int snd_cut_beg[] =
{2000, 40, 1000, 20, 0};

int snd_cut_end[] =
{1000, 50, 2000, 30, 0};

int snd_toggleon[] =
{2000, 30, 1, 30, 1500, 30, 1, 30, 1000, 40, 0};

int snd_toggleoff[] =
{1000, 20, 1, 30, 1500, 20, 1, 30, 2200, 20, 0};

int snd_done[] =
{2000, 40, 1, 30, 2000, 40, 1, 40, 2000, 140, 1, 20, 1500, 50, 0};

int snd_skip_first[] =
{1, 40, 4000, 4, 3000, 6, 2000, 8, 1, 25, 0};

int snd_skip[] =
{2000, 10, 1, 18, 0};

int snd_skipmore[] =
{2100, 20, 1, 1, 0};

