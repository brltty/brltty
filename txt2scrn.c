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

/* txt2scrn.c - Make screen images in vcsa format
 * N. Nair, 28 July 1995
 *
 * Modified: 
 *      Nicolas Pitre, 18 July 1996
 *      - added support for multiple help screens in the output file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scr.h"		/* for INDEX type definition */

int
main (int argc, char *argv[])
{
  int rows = 0, cols = 80, posx = 0, posy = 0;
  int i;
  int l;			/* length of input file */
  char buffer[160];
  char *p;
  FILE *inp, *outp;
  int f;			/* current input filename = argv[f] */
  INDEX x;			/* current screen offset in output file */

  if (argc < 3)
    {
      fprintf (stderr, "Usage: txt2scrn dest source1 [source2 ...]\n");
      exit (1);
    }
  if (!(outp = fopen (argv[1], "w+")))
    {
      fprintf (stderr, "cannot open %s\n", argv[1]);
      exit (2);
    }
  setvbuf (outp, NULL, _IOFBF, 0x7ff0);

  /* create initial index table */
  fseek (outp, 0, SEEK_SET);
  x = (argc - 2) * sizeof (INDEX);	/* offset of first screen */
  for (i = 0; i < (argc - 2); i++)
    {
      fwrite (&x, sizeof (INDEX), 1, outp);
    }

  for (f = 2; f < argc; f++)
    {
      /* New file... */
      if (!(inp = fopen (argv[f], "r")))
	{
	  fprintf (stderr, "cannot open %s\n", argv[f]);
	  fclose (outp);
	  exit (f + 1);
	}
      setvbuf (inp, NULL, _IOFBF, 0x7ff0);
      fseek (inp, 0, SEEK_END);
      l = ftell (inp);
      fseek (inp, 0, SEEK_SET);
      rows = 0;
      fputc (rows, outp);
      fputc (cols, outp);
      fputc (posx, outp);
      fputc (posy, outp);
      while (ftell (inp) < l)
	{
	  fgets (buffer, 81, inp);
	  if (strlen (buffer))
	    {
	      p = strchr (buffer, '\n');
	      if (p)
		*p = '\0';
	      for (i = strlen (buffer); i < 80; buffer[i++] = ' ');
	      for (i = 80; i; buffer[2 * i] = buffer[--i], buffer[2 * i + 1] = 7);
	      fwrite (buffer, 2, 80, outp);
	      rows++;
	    }
	}
      fseek (outp, x, SEEK_SET);
      fputc (rows, outp);
      fseek (outp, (f - 2) * sizeof (INDEX), SEEK_SET);
      fwrite (&x, sizeof (INDEX), 1, outp);
      fseek (outp, 0, SEEK_END);
      x = ftell (outp);
      fclose (inp);
    }
  fclose (outp);
  return 0;
}
