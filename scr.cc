/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
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

/* scr.cc - The screen reading library
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#define SCR_C 1

#include "scrdev.h"
#include "config.h"


/* The Live Screen type is instanciated elsewhere and choosen at link time
 * from all available screen source drivers.
 * It is defined as extern RealScreen *live;
 */

// a frozen screen image
FrozenScreen frozen;		

// the (possibly multi-page) online help
HelpScreen help;

// a pointer to the current screen object
Screen *current;


int
initscr (char *helpfile)
{
  if (live->open ())
    return 1;
  help.open (helpfile);
  current = live;
  return 0;
}


int
initscr_phys (void)
{
  /* This function should be used in a forked process. Though we want to
   * have a separate file descriptor for the live screen from the one used
   * in the main thread.  So we close and reopen the device.
   */
  live->close();
  if (live->open ())
    return 1;
  return 0;
}


void
getstat (scrstat *stat)
{
  current->getstat (*stat);
}


void
getstat_phys (scrstat *stat)
{
  live->getstat (*stat);
}


unsigned char *
getscr (winpos pos, unsigned char *buffer, short mode)
{
  return current->getscr (pos, buffer, mode);
}


void
closescr (void)
{
  live->close ();
  frozen.close ();
  help.close ();
}


void
closescr_phys (void)
{
  live->close ();
}


int
selectdisp (int disp)
{
  static int dismd = LIVE_SCRN;	/* current mode */
  static int curscrn = LIVE_SCRN;	/* current display screen */

  if ((disp & HELP_SCRN) ^ (dismd & HELP_SCRN))
    {
      if (disp & HELP_SCRN)
	/* set help mode: */
	{
	  if (!help.open ("??????"))
	    {
	      current = &help;
	      curscrn = HELP_SCRN;
	      return (dismd |= HELP_SCRN);
	    }
	  else
	    return dismd;
	}
      else
	/* clear help mode: */
	{
	  if (curscrn == HELP_SCRN)
	    dismd & FROZ_SCRN ? (current = &frozen, curscrn = FROZ_SCRN) : \
	      (current = live, curscrn = LIVE_SCRN);
	  return (dismd &= ~HELP_SCRN);
	}
    }
  if ((disp & FROZ_SCRN) ^ (dismd & FROZ_SCRN))
    {
      if (disp & FROZ_SCRN)
	{
	  if (!frozen.open (live))
	    {
	      if (curscrn == LIVE_SCRN)
		{
		  current = &frozen;
		  curscrn = FROZ_SCRN;
		}
	      return (dismd |= FROZ_SCRN);
	    }
	  else
	    return dismd;
	}
      else
	{
	  if (curscrn == FROZ_SCRN)
	    {
	      frozen.close ();
	      current = live;
	      curscrn = LIVE_SCRN;
	    }
	  return (dismd &= ~FROZ_SCRN);
	}
    }
  return dismd;
}


void
sethlpscr (short x)
{
  help.setscrno (x);
}


short
numhlpscr (void)
{
  return help.numscreens ();
}
