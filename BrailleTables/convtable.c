/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* convtable.c - convert Braille tables between formats
 * $Id: convtable.c,v 1.3 1996/09/24 01:04:25 nn201 Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int standard[8] =
{0, 1, 2, 3, 4, 5, 6, 7};	/* BRLTTY standard mapping */
int Tieman[8] =
{0, 7, 1, 6, 2, 5, 3, 4};	/* Tieman standard */
int Alva_TSI[8] =
{0, 3, 1, 4, 2, 5, 6, 7};	/* Alva/TSI standard */

#define USAGE fputs ("Usage: convtable src dest < source-file > dest-file\n\
     `src' and `dest' can be `s' for standard (BRLTTY), `t' for Tieman\n\
     or `a' for Alva/TSI mappings.\n", \
		    stderr);

int
main (int argc, char *argv[])
{
  int *src, *dest;		/* source and destination tables */
  int i, c_in, c_out;

  if (argc != 3)
    {
      USAGE;
      exit (1);
    }

  switch (tolower (argv[1][0]))
    {
    case 's':			/* BRLTTY standard */
      src = standard;
      break;
    case 't':			/* Tieman standard */
      src = Tieman;
      break;
    case 'a':			/* Alva/TSI standard */
      src = Alva_TSI;
      break;
    default:			/* unrecognised */
      src = NULL;
      break;
    }
  switch (tolower (argv[2][0]))
    {
    case 's':			/* BRLTTY standard */
      dest = standard;
      break;
    case 't':			/* Tieman standard */
      dest = Tieman;
      break;
    case 'a':			/* Alva/TSI standard */
      dest = Alva_TSI;
      break;
    default:			/* unrecognised */
      dest = NULL;
      break;
    }
  if (src == NULL || dest == NULL || \
      tolower (argv[1][0]) == tolower (argv[2][0]))
    {
      USAGE;
      exit (1);
    }

  while ((c_in = getchar ()) != EOF)
    {
      for (c_out = 0, i = 0; i < 8; i++)
	if (c_in & 1 << src[i])
	  c_out |= 1 << dest[i];
      putchar (c_out);
    }
  return 0;
}
