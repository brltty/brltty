/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
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

/* helphdr.h - describes the helpfile format
 * $Id: helphdr.h,v 1.3 1996/09/24 01:04:26 nn201 Exp $
 */

/* The compiled helpfile (brlttydev.hlp) has the following structure:
 *   1. number of pages (short int numpages)
 *   2. page info table: array of numpages pageinfo objects (described below)
 *   3. numpages blocks of help data, space padded to give a constant line
 *      length, no line termination character
 */

typedef struct
  {
    unsigned char rows;
    unsigned char cols;
  }
pageinfo;
