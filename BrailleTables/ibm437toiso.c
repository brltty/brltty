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

/* ibm437toiso - translate Braille dot table file defined for IBM CP437 charset
 * to ISO-8859-1 charset (as used by Unix consoles).
 * $Id:$
 * Nicolas Pitre, May 1997
 * Version 1.0
 */

#include <stdio.h>
#include <stdlib.h>


static int convtable[128] =
{199, 252, 233, 226, 228, 224, 229, 231, 234, 235, 232, 239, 238, 236,
 196, 197, 201, 230, 198, 244, 246, 242, 251, 249, 255, 214, 220, 162,
 163, 165, 158, 159, 225, 237, 243, 250, 241, 209, 170, 186, 191, 169,
 172, 189, 188, 161, 171, 187, 248, 164, 253, 179, 180, 145, 20, 156,
 184, 185, 255, 175, 166, 174, 190, 168, 192, 193, 194, 195, 142, 143,
 146, 128, 200, 144, 202, 203, 204, 205, 206, 207, 208, 157, 210, 211,
 212, 213, 153, 215, 216, 217, 218, 219, 154, 221, 222, 152, 133, 223,
 131, 227, 132, 134, 181, 135, 138, 130, 136, 137, 141, 173, 140, 139,
 240, 177, 149, 155, 147, 245, 247, 148, 176, 151, 183, 150, 129, 178,
 254, 160};

int
main (int argc, char *argv[])
{
  FILE *fi, *fo;		/* Input and output file structures */
  int chr;			/* Current character being processed */
  int c;			/* Character read from input */
  int newtable[128];		/* to build converted table */
  int error = 0;		/* error flag */
  int res = 0;			/* Return result, 0=success */

  if (argc != 3)
    {
      fprintf (stderr, "ibm437toiso - Convert a braille table file from IBM CP437 charset to ISO\n");
      fprintf (stderr, "Usage: ibm437toiso input_file output_file\n");
      exit (2);
    }

  /* Attempt to open input file */
  if (!(fi = fopen (argv[1], "rb")))
    {
      fprintf (stderr, "tbl2txt: Cannot open input file\n");
      exit (1);
    }

  /* Attempt to open output file */
  if (!(fo = fopen (argv[2], "w")))
    {
      fprintf (stderr, "tbl2txt: Cannot open output file\n");
      fclose (fi);
      exit (1);
    }

  /* the first 128 characters are the same */
  for (chr = 0; chr < 128 && !error; chr++)
    {
      c = fgetc (fi);
      if (c == EOF)
	{
	  error = 1;
	  continue;
	}
      fputc (c, fo);
    }
  /* initialize newtable with FF so unassigned characters will raise all
   * 8 braille dots.
   */
  for (chr = 0; chr < 128; chr++)
    newtable[chr] = 0xFF;
  /* relocate the 128 last characters */
  for (chr = 0; chr < 128 && !error; chr++)
    {
      c = fgetc (fi);
      if (c == EOF)
	{
	  error = 1;
	  continue;
	}
      newtable[convtable[chr] - 128] = c;
    }
  /* write the 128 last characters */
  for (chr = 0; chr < 128 && !error; chr++)
    {
      fputc (newtable[chr], fo);
    }
  if (error == 1)
    {
      fprintf (stderr, "ibm437toiso: Table not complete\n");
      res = 1;
    }

  fclose (fi);
  fclose (fo);
  return res;
}
