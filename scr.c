/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.0, 26 July 1996
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

/* scr.c - The screen reading library
 * N. Nair, 21 July 1995
 */

#define SCR_C 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "scr.h"
#include "config.h"

int scr_fd;			/* file descriptor for physical screen */
int frz_fd;			/* file descriptor for frozen screen image */
int hlp_fd;			/* file descriptor for help screen */
int fd;				/* file descriptor for screen reading */
short hlpscr_no = 0;		/* help screen index */
INDEX scr_offset = 0;		/* offset to desired screen */


void
sethlpscr (short x)
{
  hlpscr_no = x;
}

int
initscr (void)
{
  scr_fd = open (SCRDEV, O_RDONLY);	/* open vcsa device for read-only */
  if (scr_fd == -1)
    return 1;
  fd = scr_fd;
  frz_fd = open (FRZFILE, O_RDWR | O_CREAT | O_TRUNC);
  if (frz_fd != -1)
    if (fchmod (frz_fd, S_IRUSR | S_IWUSR))
      {
	close (frz_fd);
	frz_fd = -1;
      }
  hlp_fd = open (HLPFILE, O_RDONLY);
  if (hlp_fd == -1)
    {
      close (hlp_fd);
      hlp_fd = -1;
    }
  return 0;
}


scrstat
getstat (void)
{
  unsigned char buffer[4];
  scrstat stat;

  lseek (fd, scr_offset, SEEK_SET);	/* go to start of file */
  read (fd, buffer, 4);		/* get screen status bytes */
  stat.rows = buffer[0];
  stat.cols = buffer[1];
  stat.posx = buffer[2];
  stat.posy = buffer[3];
  return stat;
}

unsigned char *
getscr (short left, short top, short width, short height, \
	unsigned char *buffer, short mode)
{
  scrstat stat;			/* screen statistics */
  off_t start;			/* start offset */
  size_t linelen;		/* number of bytes to read for one */
  /* complete line */
  char linebuf[512];		/* line buffer; larger than is needed */
  int i, j;			/* loop counters */

  stat = getstat ();
  if (left < 0 || top < 0 || width < 1 || height < 1 \
      ||mode < 0 || mode > 1 || left + width > stat.cols \
      ||top + height > stat.rows)
    return NULL;
  start = scr_offset + 4 + (top * stat.cols + left) * 2 + mode;
  linelen = 2 * width - 1;
  for (i = 0; i < height; i++)
    {
      lseek (fd, start + i * stat.cols * 2, SEEK_SET);
      read (fd, linebuf, linelen);
      for (j = 1; j < width; linebuf[j] = linebuf[j * 2], j++);
      memcpy (buffer + i * width, linebuf, width);
    }
  return buffer;
}

void
closescr (void)
{
  close (scr_fd);		/* close physical screen reading */
  if (frz_fd != -1)
    {
      close (frz_fd);
      unlink (FRZFILE);
    }
  if (hlp_fd != -1)
    close (hlp_fd);
}


int
selectdisp (int disp)
{
  scrstat stat;
  char *buffer;
  int buflen, fdtemp;
  static int dismd = LIVE_SCRN;	/* current mode */
  static int curscrn = LIVE_SCRN;	/* current display screen */

  if ((disp & HELP_SCRN) ^ (dismd & HELP_SCRN))
    {
      if (disp & HELP_SCRN)	/* set help mode? */
	{
	  if (hlp_fd != -1)
	    {
	      fd = hlp_fd;
	      curscrn = HELP_SCRN;
	      lseek (fd, hlpscr_no * sizeof (INDEX), SEEK_SET);
	      read (fd, &scr_offset, sizeof (INDEX));
	      return (dismd |= HELP_SCRN);
	    }
	  else
	    return dismd;
	}
      else
	/* clear help mode? */
	{
	  if (curscrn == HELP_SCRN)
	    dismd & FROZ_SCRN ? (fd = frz_fd, curscrn = FROZ_SCRN) : \
	      (fd = scr_fd, curscrn = LIVE_SCRN);
	  scr_offset = 0;
	  return (dismd &= ~HELP_SCRN);
	}
    }
  if ((disp & FROZ_SCRN) ^ (dismd & FROZ_SCRN))
    {
      if (disp & FROZ_SCRN)
	{
	  if (frz_fd == -1)
	    {
	      frz_fd = open (FRZFILE, O_RDWR | O_CREAT | O_TRUNC);
	      if (frz_fd != -1)
		if (fchmod (frz_fd, S_IRUSR | S_IWUSR))
		  {
		    close (frz_fd);
		    frz_fd = -1;
		  }
	    }
	  if (frz_fd != -1)
	    {
	      fdtemp = fd;	/* store old fd */
	      fd = scr_fd;	/* use physical screen always */
	      stat = getstat ();
	      fd = fdtemp;	/* restore fd */
	      buflen = stat.cols * stat.rows * 2 + 4;
	      if (!(buffer = (char *) malloc (buflen)))
		return dismd;
	      if (lseek (scr_fd, 0, SEEK_SET) || \
		  read (scr_fd, buffer, buflen) != buflen || \
		  lseek (frz_fd, 0, SEEK_SET) || \
		  write (frz_fd, buffer, buflen) != buflen)
		{
		  free (buffer);
		  return dismd;
		}
	      free (buffer);
	      if (curscrn == LIVE_SCRN)
		{
		  fd = frz_fd;
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
	      fd = scr_fd;
	      curscrn = LIVE_SCRN;
	    }
	  return (dismd &= ~FROZ_SCRN);
	}
    }
  return dismd;
}
