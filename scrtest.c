/*
 * BrlTty - A daemon providing access to the Linux console (when in text
 *          mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
 *
 * BrlTty comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* scrtest.c - Test program for the screen reading library
 * $Id: scrtest.c,v 1.3 1996/09/24 01:04:27 nn201 Exp $
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "scr.h"

int
main (void)
{
  scrstat test;
  unsigned char buffer[2048], *res;
  short i, j, c;

  if (initscr ())
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
