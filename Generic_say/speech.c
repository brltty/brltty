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

/* Generic_say/speech.c - To use a generic 'say' command, like for the
 * rsynth package.
 * $Id: speech.c,v 1.3 1996/09/26 09:48:56 nn201 Exp $
 */

#define SPEECH_C 1

#include <stdio.h>
#include <string.h>

#include "../speech.h"


char CmdPath[] = "/usr/bin/say";	/* full path for the say command */
FILE *cmd_fd;

void
identspk (void)
{
  printf ("  - Text to speech will be piped to the %s command\n", CmdPath);
}

void
initspk (void)
{
  cmd_fd = popen (CmdPath, "w");
}

void
say (unsigned char *buffer, int len)
{
  if (cmd_fd)
    {
      fwrite (buffer, len, 1, cmd_fd);
      fwrite (".\n", 2, 1, cmd_fd);
      fflush (cmd_fd);
    }
}

void
mutespk (void)
{
}

void
closespk (void)
{
  if (cmd_fd)
    pclose (cmd_fd);
}
