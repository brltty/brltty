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

/* scrtest.c - Test program for the screen reading library
 * $Id: scrtest.c,v 1.3 1996/09/24 01:04:27 nn201 Exp $
 */

#include <stdio.h>

#include "scr.h"

int
main (void)
{
  scrstat test;
  unsigned char buffer[2048], *res;
  short i, j, c;

  if (initscr ("brltty-al.hlp"))
    {
      fprintf (stderr, "scrtest: can't initialise screen reading\n");
      exit (1);
    }
  getstat (&test);
  res = getscr ((winpos)
		{
		5, 5, 70, 15
		}
		,buffer, SCR_TEXT);
  closescr ();
  printf ("rows == %d, cols == %d\n", test.rows, test.cols);
  printf ("posx == %d, posy == %d\n\n", test.posx, test.posy);
  if (res == NULL)
    {
      puts ("getscr failed");
      exit (1);
    }
  for (i = 0; i < 15; i++)
    {
      for (j = 0; j < 70; j++)
	{
	  if ((c = buffer[i * 70 + j]) > 32 && c < 127)
	    putchar (c);
	  else
	    putchar (' ');
	}
      printf ("\n");
    }
  return 0;
}
