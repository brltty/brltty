/*
 * BrlTty - A daemon providing access to the Linux console (when in text
 *          mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
 *
 * BrlTty comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
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
