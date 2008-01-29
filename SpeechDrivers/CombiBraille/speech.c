/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "misc.h"

#include "spk_driver.h"
#include "speech.h"		/* for speech definitions */
#include "BrailleDrivers/CombiBraille/braille.h"

static size_t spk_size = 0X1000;
static unsigned char *spk_buffer = NULL;
static unsigned int spk_written = 0;


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

static int
spk_construct (SpeechSynthesizer *spk, char **parameters)
{
  if ((spk_buffer = malloc(spk_size))) {
    return 1;
  } else {
    LogError("malloc");
  }
  return 0;
}


static void
spk_write (const unsigned char *address, unsigned int count)
{
  serialWriteData(CB_serialDevice, address, count);
  spk_written += count;
}

static void
spk_flush (void)
{
  approximateDelay(spk_written * 1000 / CB_charactersPerSecond);
  spk_written = 0;
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *buffer, size_t len)
{
  unsigned char *pre_speech = (unsigned char *)PRE_SPEECH;
  unsigned char *post_speech = (unsigned char *)POST_SPEECH;
  int i;

  if (pre_speech[0]) spk_write(pre_speech+1, pre_speech[0]);
  for (i = 0; i < len; i++) {
    unsigned char byte = buffer[i];
    unsigned char *byte_address = &byte;
    unsigned int byte_count = 1;
    if (byte >= 0X80) byte = latin2cp437[byte];
    if (byte < 33) {	/* space or control character */
      byte = ' ';
    } else if (byte <= MAX_TRANS) {
      const char *word = vocab[byte - 33];
      byte_address = (unsigned char *)word;
      byte_count = strlen(word);
    }
    spk_write(byte_address, byte_count);
  }
  if (post_speech[0]) spk_write(post_speech+1, post_speech[0]);
  spk_flush();
}


static void
spk_mute (SpeechSynthesizer *spk)
{
  unsigned char *mute_seq = (unsigned char *)MUTE_SEQ;

  spk_write(mute_seq+1, mute_seq[0]);
  spk_flush();
}


static void
spk_destruct (SpeechSynthesizer *spk)
{
  if (spk_buffer) {
    free(spk_buffer);
    spk_buffer = NULL;
  }
}
