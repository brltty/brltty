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

/* Televox/speech.c - for Televox-AR interface driver
 * $Id: speech.c,v 1.2 1996/09/24 01:04:31 nn201 Exp $
 *
 * Note: This is an old synthesizer I have so I made an interface using
 * the synthesizer TSR running under dosemu. Ask me <nico@cam.org> for the
 * interface code if you're interested.
 */

#define SPEECH_C 1

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "speech.h"
#include "../spk.h"
#include "../spk_driver.h"
#include "../misc.h"


static char DevPath[] = "/dev/tlvx";	/* full path for the tlvx fifo */
static int dev_fd;
static unsigned char ShutUp[] = "\033S";	/* stop string */

static void
identspk (void)
{
  LogAndStderr(LOG_NOTICE, "Using the Televox speech interface.");
}

static void
initspk (char *parm)
{
  dev_fd = open (DevPath, O_WRONLY | O_NOCTTY | O_NONBLOCK);
}

static void
say (unsigned char *buffer, int len)
{
  if (dev_fd >= 0)
    {
      write (dev_fd, buffer, len);
      write (dev_fd, "\0", 1);
    }
}

static void
mutespk (void)
{
  say (ShutUp, strlen (ShutUp));
}

static void
closespk (void)
{
  if (dev_fd >= 0)
    close (dev_fd);
}
