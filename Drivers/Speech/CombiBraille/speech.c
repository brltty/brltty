/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "io_generic.h"
#include "async_wait.h"

#include "spk_driver.h"
#include "speech.h"		/* for speech definitions */
#include "Drivers/Braille/CombiBraille/braille.h"

static unsigned char *speechBuffer;
static size_t speechSize;
static size_t speechLength;

static const char *const vocab[MAX_TRANS - 32] = {
  " exclamation ",
  " double quote ",
  " hash ",
  " dollar ",
  " percent ",
  " ampersand ",
  " quote ",
  " left paren ",
  " right paren ",
  " star ",
  " plus ",
  " comma ",
  " dash ",
  " dot ",
  " slash ",
  "0",
  "1",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  "9",
  " colon ",
  " semicolon ",
  " less than ",
  " equals ",
  " greater than ",
  " question ",
  " at ",
  "A",
  "B",
  "C",
  "D",
  "E",
  "F",
  "G",
  "H",
  "I",
  "J",
  "K",
  "L",
  "M",
  "N",
  "O",
  "P",
  "Q",
  "R",
  "S",
  "T",
  "U",
  "V",
  "W",
  "X",
  "Y",
  "Z",
  " left bracket ",
  " backslash ",
  " right bracket ",
  " uparrow ",
  " underline ",
  " accent ",
  "a",
  "b",
  "c",
  "d",
  "e",
  "f",
  "g",
  "h",
  "i",
  "j",
  "k",
  "l",
  "m",
  "n",
  "o",
  "p",
  "q",
  "r",
  "s",
  "t",
  "u",
  "v",
  "w",
  "x",
  "y",
  "z",
  " left brace ",
  " bar ",
  " right brace ",
  " tilde "
};

/* charset conversion table from iso latin-1 == iso 8859-1 to cp437==ibmpc
 * for chars >=128. 
 */
static unsigned char latin2CP437[0X80] = {
  199, 252, 233, 226, 228, 224, 229, 231,
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
  176, 151, 163, 150, 129, 178, 254, 152
};

static int
spk_construct (SpeechSynthesizer *spk, char **parameters) {
  speechBuffer = NULL;
  speechSize = 0;
  speechLength = 0;
  return 1;
}

static void
spk_destruct (SpeechSynthesizer *spk) {
  if (speechBuffer) {
    free(speechBuffer);
    speechBuffer = NULL;
  }
}

static int
addBytes (const unsigned char *bytes, size_t count) {
  size_t newLength = speechLength + count;

  if (newLength > speechSize) {
    size_t newSize = ((newLength | 0XFF) + 1) << 1;
    unsigned char *newBuffer = realloc(speechBuffer, newSize);

    if (!newBuffer) {
      logMallocError();
      return 0;
    }

    speechBuffer = newBuffer;
    speechSize = newSize;
  }

  memcpy(&speechBuffer[speechLength], bytes, count);
  speechLength += count;
  return 1;
}

static int
addSequence (const char *sequence) {
  return addBytes((const unsigned char *)&sequence[1], sequence[0]);
}

static void
flushSpeech (void) {
  if (speechLength) {
    if (cbBrailleDisplay) {
      GioEndpoint *endpoint = cbBrailleDisplay->gioEndpoint;

      if (endpoint) {
        gioWriteData(endpoint, speechBuffer, speechLength);
        asyncWait(speechLength * 1000 / gioGetBytesPerSecond(endpoint));
        speechLength = 0;
      }
    }
  }
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
  addSequence(PRE_SPEECH);

  for (unsigned int i=0; i<length; i+=1) {
    unsigned char byte = buffer[i];
    if (byte >= 0X80) byte = latin2CP437[byte-0X80];

    unsigned char *byteAddress = &byte;
    size_t byteCount = 1;

    if (byte < 33) {	/* space or control character */
      byte = ' ';
    } else if (byte <= MAX_TRANS) {
      const char *word = vocab[byte - 33];
      byteAddress = (unsigned char *)word;
      byteCount = strlen(word);
    }

    addBytes(byteAddress, byteCount);
  }

  addSequence(POST_SPEECH);
  flushSpeech();
}

static void
spk_mute (SpeechSynthesizer *spk) {
  addSequence(MUTE_SEQ);
  flushSpeech();
}
