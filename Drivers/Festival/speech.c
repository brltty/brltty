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

/* Festival/speech.c - Speech library
 * For the Festival text to speech package
 * Maintained by Nikhil Nair <nn201@cus.cam.ac.uk>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>

#include "speech.h"		/* for speech definitions */
#include "Programs/spk.h"
#include "Programs/spk_driver.h"
#include "Programs/misc.h"


static FILE *festival;


static void
spk_identify (void)
{
  LogPrint(LOG_NOTICE, "Using the Festival text to speech package.");
}


static void
spk_open (char **parameters)
{
  unsigned char init_speech[] = { INIT_SPEECH };

  festival = popen ("festival", "w");
  if (festival && init_speech[0])
    {
      fwrite (init_speech + 1, init_speech[0], 1, festival);
      fflush (festival);
    }
}


static void
spk_say (const unsigned char *buffer, int len)
{
  unsigned char pre_speech[] = { PRE_SPEECH};
  unsigned char post_speech[] = { POST_SPEECH };
  unsigned char tempbuff[40];
  unsigned char c;
  int i;

  if (!festival)
    return;

  if (pre_speech[0])
    fwrite (pre_speech + 1, pre_speech[0], 1, festival);
  for (i = 0; i < len; i++)
    {
      c = buffer[i];
      if (c < 33)	/* space or control character */
	{
	  tempbuff[0] = ' ';
	  fwrite (tempbuff, 1, 1, festival);
	}
      else if (c > MAX_TRANS)
	fwrite (&c, 1, 1, festival);
      else
	{
	  memcpy (tempbuff, vocab[c - 33], strlen (vocab[c - 33]));
	  fwrite (tempbuff, strlen (vocab[c - 33]), 1, festival);
	}
    }
  if (post_speech[0])
    fwrite (post_speech + 1, post_speech[0], 1, festival);
  fflush (festival);
}


static void
spk_mute (void)
{
  unsigned char mute_seq[] = { MUTE_SEQ };

  if (!festival)
    return;
  fwrite (mute_seq + 1, mute_seq[0], 1, festival);
  fflush (festival);
}


static void
spk_close (void)
{
  if (festival)
    pclose (festival);
}
