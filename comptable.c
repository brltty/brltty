/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <doyons@jsp.umontreal.ca>
 *
 * Version 1.0.2, 17 September 1996
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

/* comptable.c - filter to compile 256-byte table file into C code
 * $Id: comptable.c,v 1.3 1996/09/24 01:04:25 nn201 Exp $
 */

/* The output of this filter can be #include'ed into C code to initialise
 * a static array.  Braces are not included.
 */

#include <stdio.h>

int
main (void)
{
  unsigned char buffer[8];
  int i, j;

  for (i = 0; i < 32; i++)
    {
      fread (buffer, 1, 8, stdin);
      for (j = 0; j < 7; printf ("0x%02x, ", buffer[j++]));
      if (i < 31)
	printf ("0x%02x,\n", buffer[7]);
      else
	printf ("0x%02x\n", buffer[7]);
    }
  return 0;
}
