/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* cut-n-paste.c - cut & paste functionality
 */

#include <string.h>
#include <stdlib.h>
#include "scr.h"
#include "tunes.h"


/* prototypes */
static void cut (void);


/* Global state variables */
unsigned char *cut_buffer = NULL;
static short cut_begx = 0, cut_begy = 0, cut_endx = 0, cut_endy = 0;



void cut_begin (int x, int y)
{
  if (cut_buffer)
    {
      free (cut_buffer);
      cut_buffer = NULL;
    }
  cut_begx = x;
  cut_begy = y;
  playTune (&tune_cut_begin);
}

void cut_end (int x, int y)
{
  cut_endx = x;
  cut_endy = y;
  cut ();
}

void cut_paste ()
{
  if (cut_buffer)
    insertString(cut_buffer);
}

static void cut (void)
{
  short cols, rows;

  cols = cut_endx - cut_begx + 1;
  rows = cut_endy - cut_begy + 1;
  if (cols >= 1 && rows >= 1)
    {
      unsigned char *srcbuf, *dstbuf, *srcptr, *dstptr;
      short r, c, spaces;
      srcbuf = (char *) malloc (rows * cols);
      dstbuf = (char *) malloc (rows * cols + rows);
      if (srcbuf && dstbuf)
	{
	  /* grab it */
	  readScreen((ScreenBox){cut_begx, cut_begy, cols, rows},
		     srcbuf, SCR_TEXT);
	  srcptr = srcbuf;
	  dstptr = dstbuf;
	  /* remove spaces at end of line, add return (except to last line),
	     and possibly remove non-printables... if any */
	  for (r = cut_begy; r <= cut_endy; r++)
	    {
	      for (spaces = 0, c = cut_begx; c <= cut_endx; c++, srcptr++)
		if (*srcptr <= 32)
		  spaces++;
		else
		  {
		    while (spaces)
		      {
			*(dstptr++) = ' ';
			spaces--;
		      }
		    *(dstptr++) = *srcptr;
		  }
	      if (r != cut_endy)
		*(dstptr++) = '\r';
	    }
	  *dstptr = 0;
	  /* throw away old buffer... we should consider a command to append... */
	  if (cut_buffer)
	    free (cut_buffer);
/* Should we just keep dstptr? We would save a malloc... */
	  /* make a new permanent buffer of just the right size */
	  cut_buffer = (char *) malloc (strlen (dstbuf) + 1);
	  strcpy (cut_buffer, dstbuf);
	  free (srcbuf);
	  free (dstbuf);
	  playTune (&tune_cut_end);
	}
      else
	{
	  if (srcbuf)
	    free (srcbuf);
	  if (dstbuf)
	    free (dstbuf);
	}
    }
}
