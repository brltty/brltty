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

/* brltest.c - Test progrm for the Braille display library
 * J. Bowden, 24 July 1995
 */

#define BRLTTY_C 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "brl.h"
#include "misc.h"
#include "config.h"

brldim brl;
unsigned char texttrans[256];	/* text translation table (output) */
#if defined (Alva_ABT3)
unsigned char StatusCells[4];	/* status character buffer */
#elif defined (CombiBraille)
unsigned char statcells[5];	/* status cell buffer */
#endif

void message (char *s);


int
main (int argc, char *argv[])
{
  int tbl_fd;

  if (chdir (HOME_DIR))		/* change to directory containing data files */
    {
      fprintf (stderr, "`%s': ", HOME_DIR);
      perror (NULL);
      exit (1);
    }
  printf ("Changed to directory %s\n", HOME_DIR);
  identbrl (argc > 1 ? argv[1] : NULL);		/* start-up messages */
  brl = initbrl (argc > 1 ? argv[1] : NULL);	/* initialise display */
  if (brl.x == -1)
    {
      fprintf (stderr, "Initialisation error\n");
      exit (2);
    }
  printf ("Display initialised successfully, ");
  printf ("it is %d rows by %d cols\n", brl.y, brl.x);
  tbl_fd = open (TEXTTRN_NAME, O_RDONLY);
  if (tbl_fd == -1)
    {
      perror (TEXTTRN_NAME);
      exit (3);
    }
  if (read (tbl_fd, texttrans, 256) != 256)
    {
      close (tbl_fd);
      perror (TEXTTRN_NAME);
      exit (4);
    }
  message ("Hello world, This is BRLTTY!");

  printf ("\nHit return to continue:\n");
  getchar ();
  closebrl (brl);		/* finish with the display */
  return 0;
}


void
message (char *s)
{
  int i, j, l;

#ifdef CombiBraille
  memset (statcells, 0, 5);
  setbrlstat (statcells);
#endif
  memset (brl.disp, ' ', brl.x * brl.y);
  l = strlen (s);
  while (l)
    {
      j = l <= brl.x * brl.y ? l : brl.x * brl.y - 1;
      for (i = 0; i < j; brl.disp[i++] = *s++);
      if (l -= j)
	brl.disp[brl.x * brl.y - 1] = '-';

      /* Do Braille translation using text table: */
      for (i = 0; i < brl.x * brl.y; brl.disp[i] = texttrans[brl.disp[i]], \
	   i++);

      writebrl (brl);
      if (l)
	while (readbrl (TBL_ARG) == EOF)
	  delay (KEYDEL);
    }
}


/* dummy function to allow brl.o to link... */
void
sethlpscr (short x)
{
}
