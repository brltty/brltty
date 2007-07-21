/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

/* GenericSay/speech.c - To use a generic 'say' command, like for the
 * rsynth package.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "misc.h"

typedef enum {
   PARM_COMMAND
} DriverParameter;
#define SPKPARMS "command"

#include "spk_driver.h"
#include "speech.h"

static const char *commandPath;	/* default full path for the say command */
static FILE *commandStream = NULL;

static int
spk_open (char **parameters)
{
  const char *command = parameters[PARM_COMMAND];
  commandPath = *command? command: SAY_CMD;
  LogPrint(LOG_INFO, "Speech Command: %s", commandPath);
  return 1;
}

static void
spk_say (const unsigned char *buffer, int length)
{
  if (!commandStream)
    commandStream = popen(commandPath, "w");
  if (commandStream)
    {
      const char *trailer = "\n";
      fwrite(buffer, length, 1, commandStream);
      fwrite(trailer, strlen(trailer), 1, commandStream);
      fflush(commandStream);
    }
}

static void
spk_mute (void)
{
  if (commandStream)
    {
       pclose(commandStream);
       commandStream = NULL;
    }
}

static void
spk_close (void)
{
   spk_mute();
}
