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

/* scrtest.c - Test program for the screen reading library
 * N. Nair, 20 September 1995
 */

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
  test = getstat ();
  res = getscr (5, 5, 70, 15, buffer, SCR_TEXT);
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
