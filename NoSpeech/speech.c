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

/* NoSpeech/speech.c - Dummy speech driver for speechless BRLTTY.
 * $Id: speech.c,v 1.2 1996/09/24 01:04:30 nn201 Exp $
 */

#define SPEECH_C 1

#include <stdio.h>

#include "speech.h"
#include "../spk.h"
#include "../spk_driver.h"
#include "../misc.h"


static void
identspk (void)
{
  LogAndStderr(LOG_NOTICE, "No speech support.");
}

static void
initspk (char *parm)
{
}

static void
say (unsigned char *buffer, int len)
{
}

static void
mutespk (void)
{
}

static void
closespk (void)
{
}
