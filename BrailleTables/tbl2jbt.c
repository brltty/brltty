/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

/* tbl2jbt - translate Braille dot table file to the JBT (JAWS for Windows
 * Braille Table) format
 * Warning: JFW expects tables for IBM437 charset, while BRLTTY uses
 * tables for ISO-8859-1. Use isotoibm437 before passing through tbl2jbt.
 * Adapted from James Bowden's tbl2txt
 * Stéphane Doyon
 * September 1999
 * Version 1.0
 */

#include <stdio.h>
#include <stdlib.h>

unsigned char dot[8] =
{1, 4, 16, 2, 8, 32, 64, 128};	/* Dot values */

int
main (int argc, char *argv[])
{
  FILE *fi, *fo;		/* Input and output file structures */
  int chr;			/* Current character being processed */
  int c;			/* Character read from input */
  int error = 0;		/* error flag */
  int res = 0;			/* Return result, 0=success */
  int i;			/* for loops */

  if (argc != 3)
    {
      fprintf (stderr, "tbl2jbt - Braille dot table to JAWS table\n");
      fprintf (stderr, "  warning: JFW expects a table for IBM 437 table charset, while BRLTTY uses tables for ISO-8859-1. Use isotoibm437 before passing through tbl2jbt.\n");
      fprintf (stderr, "Usage: tbl2txt input_file output_file\n");
      exit (2);
    }

  /* Attempt to open input file */
  if (!(fi = fopen (argv[1], "rb")))
    {
      fprintf (stderr, "tbl2jbt: Cannot open input file\n");
      exit (1);
    }

  /* Attempt to open output file */
  if (!(fo = fopen (argv[2], "w")))
    {
      fprintf (stderr, "tbl2jbt: Cannot open output file\n");
      fclose (fi);
      exit (1);
    }

  fprintf (fo, "[OEM]\r\n");
  for (chr = 0; chr < 256 && !error; chr++)
    {
      c = fgetc (fi);
      if (c == EOF)
	{
	  error = 1;
	  continue;
	}
      fprintf (fo, "\\%d=", chr);
      for (i = 0; i < 8; i++)
	if(c & dot[i])
	  fputc (i + '1', fo);
      fputs ("\r\n", fo);

    }

  if (error == 1)
    {
      fprintf (stderr, "tbl2jbt: Table not complete\n");
      res = 1;
    }

  fclose (fi);
  fclose (fo);
  return res;
}
