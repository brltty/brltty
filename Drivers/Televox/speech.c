/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

/* Televox/speech.c - for Televox-AR interface driver
 * $Id: speech.c,v 1.2 1996/09/24 01:04:31 nn201 Exp $
 *
 * Note: This is an old synthesizer I have so I made an interface using
 * the synthesizer TSR running under dosemu. Ask me <nico@cam.org> for the
 * interface code if you're interested.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "speech.h"
#include "Programs/spk.h"
#include "Programs/spk_driver.h"
#include "Programs/misc.h"


static char DevPath[] = "/dev/tlvx";	/* full path for the tlvx fifo */
static int dev_fd;
static unsigned char ShutUp[] = "\033S";	/* stop string */

static void
spk_identify (void)
{
  LogPrint(LOG_NOTICE, "Using the Televox speech interface.");
}

static void
spk_open (char **parameters)
{
  dev_fd = open (DevPath, O_WRONLY | O_NOCTTY | O_NONBLOCK);
}

static void
spk_say (const unsigned char *buffer, int len)
{
  if (dev_fd >= 0)
    {
      write (dev_fd, buffer, len);
      write (dev_fd, "\0", 1);
    }
}

static void
spk_mute (void)
{
  spk_say (ShutUp, strlen (ShutUp));
}

static void
spk_close (void)
{
  if (dev_fd >= 0)
    close (dev_fd);
}
