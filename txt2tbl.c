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

/* txt2tbl.c - Simple Braille table creator
 * James Bowden
 * March 1996
 * Version 1.0
 */

#include <stdio.h>
#include <stdlib.h>

unsigned char dotpos[8] =
{1, 4, 16, 2, 8, 32, 64, 128};	/* Value of each dot */

int
main (int argc, char *argv[])
{
  FILE *fi, *fo;		/* Input and output file structures */
  unsigned char dotarray[256];	/* Stores dot information */
  int chr;			/* Current character being processed */
  int c;			/* Character read from input */
  int flag;			/* Processed a char in this input line? */
  int error = 0;		/* Error flag */

  if (argc != 3)
    {
      fprintf (stderr, "txt2tbl - create Braille dot translation table\n");
      fprintf (stderr, "Usage: txt2tbl input_file output_file\n");
      exit (2);
    }

  /* Attempt to open input file */
  if (!(fi = fopen (argv[1], "r")))
    {
      fprintf (stderr, "txt2tbl: Cannot open input file\n");
      exit (1);
    }

  /* Attempt to open output file */
  if (!(fo = fopen (argv[2], "wb")))
    {
      fprintf (stderr, "txt2tbl: Cannot open output file\n");
      fclose (fi);
      exit (1);
    }

  for (chr = 0; chr < 256; dotarray[chr++] = 0);	/* Clear array */

  c = fgetc (fi);
  flag = chr = 0;
  while (c != EOF && !error)
    {
      switch (c)
	{
	case '\n':		/* New line */
	  if (flag)
	    {
	      flag = 0;		/* Reset for the new line */
	      chr++;		/* Next character to process */
	    }
	  break;
	case '(':		/* Start looking */
	  if (!flag)
	    flag = 1;
	  break;
	case ')':		/* Stop looking on this line */
	  if (flag)
	    flag = 2;
	  break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':		/* Braille dots */
	  if (flag == 1)	/* We are recording */
	    if (chr < 256)
	      dotarray[chr] |= dotpos[c - '1'];
	    else
	      error = 1;
	  break;
	}

      c = fgetc (fi);
    }

  if (error == 1)
    {
      fprintf (stderr, "txt2tbl: Too many characters\n");
      fclose (fi);
      fclose (fo);
      exit (1);
    }
  else
    for (chr = 0; chr < 256; fputc (dotarray[chr++], fo));

  fclose (fi);
  fclose (fo);
  return 0;
}
