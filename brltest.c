/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-1999 by The BRLTTY Team, All rights reserved.
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

/* brltest.c - Test progrm for the Braille display library
 * $Id: brltest.c,v 1.3 1996/09/24 01:04:24 nn201 Exp $
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

/* Output translation tables - the files *.auto.h are generated at
 * compile-time:
 */
unsigned char texttrans[256] =
{
#include "text.auto.h"
};

#ifdef SAY_CMD
unsigned char say_buffer[140];
#endif

#if defined (Alva_ABT3)
unsigned char StatusCells[4];	/* status character buffer */
#elif defined (CombiBraille)
unsigned char statcells[5];	/* status cell buffer */
#endif

void message (char *s);


int
main (int argc, char *argv[])
{
  if (chdir (HOME_DIR))		/* change to directory containing data files */
    {
      fprintf (stderr, "Can't cd to %s, trying /etc\n", HOME_DIR);
      if (chdir ("/etc"))
	fprintf (stderr, "Can't cd to /etc, continuing anyway");
    }
  else
    printf ("Changed to directory %s\n", HOME_DIR);
  identbrl (argc > 1 ? argv[1] : NULL);		/* start-up messages */
  initbrl (&brl, argc > 1 ? argv[1] : NULL);	/* initialise display */
  if (brl.x == -1)
    {
      fprintf (stderr, "Initialisation error\n");
      exit (2);
    }
  printf ("Display initialized successfully, ");
  printf ("it is %d rows by %d cols\n", brl.y, brl.x);
  message ("Hello world, This is BRLTTY!");

  printf ("\nHit return to continue:\n");
  getchar ();
  closebrl (&brl);		/* finish with the display */
  return 0;
}


void
message (char *s)
{
  int i, j, l;

#ifdef SAY_CMD
  say_buffer[0] = strlen (s);
  strcpy (say_buffer + 1, s);
  say (say_buffer);
#endif
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

      writebrl (&brl);
      if (l)
	while (readbrl (TBL_ARG) == EOF)
	  delay (KEYDEL);
    }
}


/* dummy function to allow brl.o to link... */

void sethlpscr (short x)
{
}

void inskey( unsigned char *s, short f )
{
}

