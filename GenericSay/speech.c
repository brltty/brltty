/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team, All rights reserved.
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

/* Generic_say/speech.c - To use a generic 'say' command, like for the
 * rsynth package.
 */

#define SPEECH_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "speech.h"
#include "../spk.h"
#include "../spk_driver.h"
#include "../misc.h"


static char say_path[] = SAY_CMD;	/* full path for the say command */
static FILE *say_fd = NULL;

static void
identspk (void)
{
  LogAndStderr(LOG_NOTICE, "Speech will be piped to \"%s\".", say_path);
}

static void
initspk (char *parm)
{
}

static void
say (unsigned char *buffer, int len)
{
  if (!say_fd)
    say_fd = popen (say_path, "w");
  if (say_fd)
    {
      fwrite (buffer, len, 1, say_fd);
//      fwrite (".\n", 2, 1, say_fd);
//      fflush (say_fd);
    }
}

static void
mutespk (void)
{
  if (say_fd)
    {
       pclose(say_fd);
       say_fd = NULL;
    }
}

static void
closespk (void)
{
   mutespk();
}
