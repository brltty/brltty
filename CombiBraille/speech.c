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

/* CombiBraille/speech.c - Speech library
 * For Tieman B.V.'s CombiBraille (serial interface only)
 * $Id: speech.c,v 1.2 1996/09/24 01:04:29 nn201 Exp $
 */

#define SPEECH_C 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "../speech.h"
#include "brlconf.h"
#include "speech.h"		/* for speech definitions */

/* These are shared with CombiBraille/brl.c: */
extern int brl_fd;
extern unsigned char *rawdata;


void
identspk (void)
{
  puts ("Using the CombiBraille's in-built speech.");
}


void
initspk (void)
{
}


void
say (unsigned char *buffer, int len)
{
  unsigned char *pre_speech = PRE_SPEECH;
  unsigned char *post_speech = POST_SPEECH;
  int i;

  if (pre_speech[0])
    {
      memcpy (rawdata, pre_speech + 1, pre_speech[0]);
      write (brl_fd, rawdata, pre_speech[0]);
    }
  for (i = 0; i < len; i++)
    {
      if (buffer[i] < 33)	/* space or control character */
	{
	  rawdata[0] = ' ';
	  write (brl_fd, rawdata, 1);
	}
      else if (buffer[i] > MAX_TRANS)
	write (brl_fd, buffer + i, 1);
      else
	{
	  memcpy (rawdata, vocab[buffer[i] - 33], \
		  strlen (vocab[buffer[i] - 33]));
	  write (brl_fd, rawdata, strlen (vocab[buffer[i] - 33]));
	}
    }
  if (post_speech[0])
    {
      memcpy (rawdata, post_speech + 1, post_speech[0]);
      write (brl_fd, rawdata, post_speech[0]);
    }
}


void
mutespk (void)
{
  unsigned char *mute_seq = MUTE_SEQ;

  memcpy (rawdata, mute_seq + 1, mute_seq[0]);
  write (brl_fd, rawdata, mute_seq[0]);
}


void
closespk (void)
{
}
