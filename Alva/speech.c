/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-1999 by The BRLTTY Team, All rights reserved.
 *
 * Nicolas Pitre <nico@cam.org>
 * Stéphane Doyon <s.doyon@videotron.ca>
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 */

/* Alva/speech.c - Speech library
 * For the Alva Delphi.
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


/* This is shared with brlmain.c */
int SendToAlva( char *data, int len );


/* charset conversion table from iso latin-1 == iso 8859-1 to cp437==ibmpc
 * for chars >=128. 
 */
static unsigned char latin2cp437[128] =
  {199, 252, 233, 226, 228, 224, 229, 231,
   234, 235, 232, 239, 238, 236, 196, 197,
   201, 181, 198, 244, 247, 242, 251, 249,
   223, 214, 220, 243, 183, 209, 158, 159,
   255, 173, 155, 156, 177, 157, 188, 21,
   191, 169, 166, 174, 170, 237, 189, 187,
   248, 241, 253, 179, 180, 230, 20, 250,
   184, 185, 167, 175, 172, 171, 190, 168,
   192, 193, 194, 195, 142, 143, 146, 128,
   200, 144, 202, 203, 204, 205, 206, 207,
   208, 165, 210, 211, 212, 213, 153, 215,
   216, 217, 218, 219, 154, 221, 222, 225,
   133, 160, 131, 227, 132, 134, 145, 135,
   138, 130, 136, 137, 141, 161, 140, 139,
   240, 164, 149, 162, 147, 245, 148, 246,
   176, 151, 163, 150, 129, 178, 254, 152};


void
identspk (void)
{
  puts ("  - Using the Alva Delphi's in-built speech.");
}


void
initspk (void)
{
}


void
say (unsigned char *buffer, int len)
{
  static unsigned char *pre_speech = PRE_SPEECH;
  static unsigned char *post_speech = POST_SPEECH;
  unsigned char buffer[256];
  unsigned char c;
  int i;

  if (pre_speech[0])
    {
      memcpy (buffer, pre_speech + 1, pre_speech[0]);
      SendToAlva (buffer, pre_speech[0]);
    }
  for (i = 0; i < len; i++)
    {
      c = buffer[i];
      if (c >= 128) c = latin2cp437[c];
      if (c < 33)	/* space or control character */
	{
	  buffer[0] = ' ';
	  SendToAlva (buffer, 1);
	}
      else if (c > MAX_TRANS)
	SendToAlva (&c, 1);
      else
	{
	  memcpy (buffer, vocab[c - 33], strlen (vocab[c - 33]));
	  SendToAlva (buffer, strlen (vocab[c - 33]));
	}
    }
  if (post_speech[0])
    {
      memcpy (buffer, post_speech + 1, post_speech[0]);
      SendToAlva (buffer, post_speech[0]);
    }
}


void
mutespk (void)
{
  static unsigned char *mute_seq = MUTE_SEQ;
  unsigned char buffer[32];

return;
  memcpy (buffer, mute_seq + 1, mute_seq[0]);
  SendToAlva (buffer, mute_seq[0]);
}


void
closespk (void)
{
}
