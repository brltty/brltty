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

#ifdef SAY_CMD
unsigned char say_buffer[140];
#endif

void message (char *s);


int
main (int argc, char *argv[])
{
  if(argc > 2)
    braille_libname = argv[2];
  else
    braille_libname = NULL;

  if (!load_braille_driver())
    {
      LogAndStderr(LOG_ERR, "Braille driver not specified.");
      exit(10);
    }

  if (chdir (HOME_DIR))		/* change to directory containing data files */
    {
      fprintf (stderr, "Can't cd to %s, trying /etc\n", HOME_DIR);
      if (chdir ("/etc"))
	fprintf (stderr, "Can't cd to /etc, continuing anyway");
    }
  else
    printf ("Changed to directory %s\n", HOME_DIR);
  braille->identify();		/* start-up messages */
  braille->initialize(&brl, argc > 1 ? argv[1] : BRLDEV);	/* initialise display */
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
  braille->close(&brl);		/* finish with the display */
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

  memset (statcells, 0, sizeof(statcells));
  braille->setstatus(statcells);

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

      braille->write(&brl);
      if (l)
	while (braille->read(TBL_ARG) == EOF)
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

