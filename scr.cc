/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <doyons@jsp.umontreal.ca>
 *
 * Version 1.0.2, 17 September 1996
 *
 * Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
 */

/* scr.cc - The screen reading library
 * $Id: scr.cc,v 1.3 1996/09/24 01:04:27 nn201 Exp $
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#define SCR_C 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "scrdev.h"
#include "config.h"

LiveScreen live, constlive;	// the physical screen - two `threads'
FrozenScreen frozen;		// a frozen screen image
HelpScreen help;		// the (possibly multi-page) online help
Screen *current;		// a pointer to the current screen object


int
initscr (void)
{
  if (live.open ())
    return 1;
  help.open ();
  current = &live;
  return 0;
}


int
initscr_phys (void)
{
  if (constlive.open ())
    return 1;
  return 0;
}


scrstat
getstat (void)
{
  scrstat stat;

  current->getstat (stat);
  return stat;
}


scrstat
getstat_phys (void)
{
  scrstat stat;

  constlive.getstat (stat);
  return stat;
}


unsigned char *
getscr (winpos pos, unsigned char *buffer, short mode)
{
  return current->getscr (pos, buffer, mode);
}


void
closescr (void)
{
  live.close ();
  frozen.close ();
  help.close ();
}


void
closescr_phys (void)
{
  constlive.close ();
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
	  if (!help.open ())
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
	      (current = &live, curscrn = LIVE_SCRN);
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
	      current = &live;
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
