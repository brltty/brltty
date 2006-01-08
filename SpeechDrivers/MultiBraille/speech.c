/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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

/* CombiBraille/speech.c - Speech library
 * For Tieman B.V.'s CombiBraille (serial interface only)
 * Maintained by Nikhil Nair <nn201@cus.cam.ac.uk>
 * $Id: speech.c,v 1.2 1996/09/24 01:04:29 nn201 Exp $
 */

#include "prologue.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "Programs/misc.h"

#include "Programs/spk_driver.h"
#include "speech.h"		/* for speech definitions */
#include "BrailleDrivers/MultiBraille/braille.h"

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


static void
spk_identify (void)
{
  puts ("Using the MultiBraille's in-built speech.\n");
}


static int
spk_open (char **parameters)
{
  return 1;
}


static void
spk_say (const unsigned char *buffer, int len)
{
  unsigned char *pre_speech = (unsigned char *)PRE_SPEECH;
  unsigned char *post_speech = (unsigned char *)POST_SPEECH;
  unsigned char c;
  int i;

  if (pre_speech[0])
    {
      serialWriteData (MB_serialDevice, pre_speech+1, pre_speech[0]);
    }
  for (i = 0; i < len; i++)
    {
      c = buffer[i];
      if (c >= 128) c = latin2cp437[c];
      if (c < 33)	/* space or control character */
	{
	  static const char blank = ' ';
	  serialWriteData (MB_serialDevice, &blank, 1);
	}
      else if (c > MAX_TRANS)
	serialWriteData (MB_serialDevice, &c, 1);
      else
	{
          const char *word = vocab[c - 33];
	  serialWriteData (MB_serialDevice, word, strlen (word));
	}
    }
  if (post_speech[0])
    {
      serialWriteData (MB_serialDevice, post_speech+1, post_speech[0]);
    }
}


static void
spk_mute (void)
{
  unsigned char *mute_seq = (unsigned char *)MUTE_SEQ;
  serialWriteData (MB_serialDevice, mute_seq+1, mute_seq[0]);
}


static void
spk_close (void)
{
}
