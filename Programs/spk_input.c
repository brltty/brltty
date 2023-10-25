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
#include "spk_input.h"
#include "spk.h"
#include "pipe.h"
#include "tune.h"
#include "tune_builder.h"
#include "ascii.h"
#include "utf8.h"
#include "scr_types.h"
#include "core.h"

#ifdef ENABLE_SPEECH_SUPPORT
struct SpeechInputObjectStruct {
  NamedPipeObject *pipe;
};

static
NAMED_PIPE_INPUT_CALLBACK(handleSpeechInput) {
  const unsigned char *buffer = parameters->buffer;
  size_t bufferSize = parameters->length;
  const unsigned char *end = buffer + bufferSize;

  SayOptions options = 0;
  unsigned char colour = SCR_COLOUR_DEFAULT;
  int asTones = 0;

  while (buffer < end) {
     if (*buffer != ASCII_ESC) break;
     if (++buffer == end) break;

     switch (*buffer++) {
       case '!':
         options |= SAY_OPT_MUTE_FIRST;
         break;

       case 'c':
         if (buffer < end) colour = *buffer++;
         break;

       case 't':
         asTones = 1;
         break;

     }
  }

  if (buffer < end) {
    size_t textLength = end - buffer;
    char text[textLength + 1];
    memcpy(text, buffer, textLength);
    text[textLength] = 0;

    if (asTones) {
      static TuneBuilder *tb = NULL;

      if (tb) {
        resetTuneBuilder(tb);
      } else if ((tb = newTuneBuilder())) {
        setTuneSourceName(tb, "speech-input");
        setTuneSourceIndex(tb, 0);
      }

      if (tb) {
        if (parseTuneString(tb, "p100")) {
          if (parseTuneString(tb, text)) {
            ToneElement *tones = getTune(tb);

            if (tones) {
              tunePlayTones(tones);
            }
          }
        }
      }
    } else {
      size_t attributesCount = countUtf8Characters(text);
      unsigned char attributes[attributesCount + 1];
      memset(attributes, colour, attributesCount);

      sayUtf8Characters(
        &spk, text, attributes,
        textLength, attributesCount, options
      );
    }
  }

  return bufferSize;
}

SpeechInputObject *
newSpeechInputObject (const char *name) {
  SpeechInputObject *obj;

  if ((obj = malloc(sizeof(*obj)))) {
    memset(obj, 0, sizeof(*obj));

    if ((obj->pipe = newNamedPipeObject(name, handleSpeechInput, obj))) {
      return obj;
    }

    free(obj);
  } else {
    logMallocError();
  }

  return NULL;
}

void
destroySpeechInputObject (SpeechInputObject *obj) {
  if (obj->pipe) destroyNamedPipeObject(obj->pipe);
  free(obj);
}
#endif /* ENABLE_SPEECH_SUPPORT */
