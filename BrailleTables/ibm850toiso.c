/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

/* ibm850toiso - translate Braille dot table file defined for IBM CP850 charset
 * to ISO-8859-1 charset (as used by Unix consoles).
 * $Id:$
 * Nicolas Pitre, May 1997
 * Version 1.0
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

static int convtable[128] =
{199, 252, 233, 226, 228, 224, 229, 231,        /* 128 - 135 */
 234, 235, 232, 239, 238, 236, 196, 197,        /* 136 - 143 */
 201, 230, 198, 244, 246, 242, 251, 249,        /* 144 - 151 */
 255, 214, 220, 248, 163, 216, 215, 159,        /* 152 - 159 */
 225, 237, 243, 250, 241, 209, 170, 186,        /* 160 - 167 */
 191, 174, 172, 189, 188, 161, 171, 187,        /* 168 - 175 */
 155, 157, 141, 129, 139, 193, 194, 192,        /* 176 - 183 */
 169, 150, 132, 175, 184, 162, 165, 151,        /* 184 - 191 */
 183, 181, 147, 128, 142, 143, 227, 195,        /* 192 - 199 */
 131, 144, 146, 133, 135, 153, 158, 164,        /* 200 - 207 */
 240, 208, 202, 203, 200, 134, 205, 206,        /* 208 - 215 */
 207, 137, 130, 136, 154, 166, 204, 152,        /* 216 - 223 */
 211, 223, 212, 210, 245, 213, 145, 222,        /* 224 - 231 */
 254, 218, 219, 217, 253, 221, 140, 180,        /* 232 - 239 */
 173, 177, 149, 190, 182, 167, 247, 148,        /* 240 - 247 */
 176, 168, 156, 185, 179, 178, 138, 160         /* 248 - 255 */
};

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
      fprintf (stderr, "ibm850toiso - Convert a braille table file from IBM CP850 charset to ISO\n");
      fprintf (stderr, "Usage: ibm850toiso input_file output_file\n");
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
      fprintf (stderr, "ibm850toiso: Table not complete\n");
      res = 1;
    }

  fclose (fi);
  fclose (fo);
  return res;
}
