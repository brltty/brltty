/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.9.0, 06 April 1998
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <s.doyon@videotron.ca>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* NoSpeech/speech.c - Dummy speech driver for speechless BRLTTY.
 * $Id: speech.c,v 1.2 1996/09/24 01:04:30 nn201 Exp $
 */

#define SPEECH_C 1

#include <stdio.h>

#include "../speech.h"


void
identspk (void)
{
  puts ("  - BRLTTY compiled without speech support");
}

void
initspk (void)
{
}

void
say (unsigned char *buffer, int len)
{
}

void
mutespk (void)
{
}

void
closespk (void)
{
}
