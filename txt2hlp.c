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

/* txt2hlp.c - Make BRLTTY format help files from plain text
 * $Id: txt2hlp.c,v 1.3 1996/09/24 01:04:28 nn201 Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "help.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int
main (int argc, char *argv[])
{
  int l;			/* length of input file */
  char buffer[242];
  char *p;
  FILE *inp, *outp;
  short f;			/* current input filename = argv[f] */
  pageinfo *psz;

  if (argc < 3)
    {
      fprintf (stderr, "Usage: txt2hlp dest source1 [source2 ...]\n");
      exit (1);
    }
  if (!(psz = (pageinfo *) malloc ((argc - 2) * sizeof (pageinfo))))
    {
      fprintf (stderr, "txt2hlp: out of memory");
      exit (2);
    }
  if (!(outp = fopen (argv[1], "w+")))
    {
      fprintf (stderr, "txt2hlp: cannot open %s\n", argv[1]);
      exit (3);
    }
  setvbuf (outp, NULL, _IOFBF, 0x7ff0);

  /* Leave space for header: */
  fseek (outp, sizeof (short) + (argc - 2) * sizeof (pageinfo), SEEK_SET);

  for (f = 2; f < argc; f++)
    {
      /* New file ... */
      if (!(inp = fopen (argv[f], "r")))
	{
	  fprintf (stderr, "txt2hlp: cannot open %s\n", argv[f]);
	  fclose (outp);
	  exit (f + 2);
	}
      setvbuf (inp, NULL, _IOFBF, 0x7ff0);
      fseek (inp, 0, SEEK_END);
      l = ftell (inp);
      fseek (inp, 0, SEEK_SET);
      psz[f - 2].rows = 0;
      psz[f - 2].cols = 0;
      while (ftell (inp) < l)
	{
	  fgets (buffer, 242, inp);
	  if (strlen (buffer))
	    {
	      p = strchr (buffer, '\n');
	      if (p)
		*p = '\0';
	      fputc (strlen (buffer), outp);
	      fwrite (buffer, 1, strlen (buffer), outp);
	      psz[f - 2].cols = MAX (psz[f - 2].cols, strlen (buffer));
	    }
	  else
	    fputc (0, outp);
	  psz[f - 2].rows++;
	}
      fclose (inp);
      psz[f - 2].cols = (((int) ((psz[f - 2].cols - 1) / 40)) + 1) * 40;
    }
  fseek (outp, 0, SEEK_SET);
  f = argc - 2;
  fwrite (&f, sizeof (short), 1, outp);
  fwrite (psz, sizeof (pageinfo), f, outp);
  fclose (outp);
  free (psz);
  return 0;
}
