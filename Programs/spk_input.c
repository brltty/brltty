/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2024 by The BRLTTY Developers.
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
  const unsigned char *const buffer = parameters->buffer;
  const size_t bufferSize = parameters->length;
  const unsigned char *const bufferEnd = buffer + bufferSize;
  const unsigned char *from = buffer;

  while (from < bufferEnd) {
    SayOptions options = 0;
    unsigned char colour = SCR_COLOUR_DEFAULT;
    int dontSpeak = 0;
    int asTune = 0;

    while (from < bufferEnd) {
      if (*from != ASCII_ESC) break;
      if (++from == bufferEnd) break;

      switch (*from++) {
        case '!':
          options |= SAY_OPT_MUTE_FIRST;
          break;

        case 'b':
          if (!prefs.autospeakEmptyLine) dontSpeak = 1;
          break;

        case 'c':
          if (from < bufferEnd) colour = *from++;
          break;

        case 'd':
          if (!prefs.autospeakDeletedCharacters) dontSpeak = 1;
          break;

        case 'i':
          if (!prefs.autospeakInsertedCharacters) dontSpeak = 1;
          break;

        case 'l':
          if (!prefs.autospeakSelectedLine) dontSpeak = 1;
          break;

        case 'r':
          if (!prefs.autospeakReplacedCharacters) dontSpeak = 1;
          break;

        case 's':
          if (!prefs.autospeakSelectedCharacter) dontSpeak = 1;
          break;

        case 't':
          asTune = 1;
          break;

        case 'w':
          if (!prefs.autospeakCompletedWords) dontSpeak = 1;
          break;
      }
    }

    const unsigned char *to = memchr(from, ASCII_ESC, (bufferSize - (from - buffer)));
    if (!to) to = bufferEnd;

    size_t textLength = to - from;
    char text[textLength + 1];
    memcpy(text, from, textLength);
    text[textLength] = 0;
    logMessage(LOG_CATEGORY(SPEECH_EVENTS), "speech input: %s", text);

    if (options & SAY_OPT_MUTE_FIRST) {
      if (asTune || !textLength) {
        muteSpeech(&spk, "speech input");
      }
    }

    if (textLength) {
      if (asTune) {
        TuneBuilder *tb = newTuneBuilder();

        if (tb) {
          setTuneSourceName(tb, "speech-input");
          setTuneSourceIndex(tb, 0);

          if (parseTuneString(tb, "p100")) {
            if (parseTuneString(tb, text)) {
              ToneElement *tune = getTune(tb);
              if (tune) tunePlayTones(tune, TPO_FREE);
            }
          }

          destroyTuneBuilder(tb);
        }
      } else if (!dontSpeak) {
        size_t attributesCount = countUtf8Characters(text);
        unsigned char attributes[attributesCount + 1];
        memset(attributes, colour, attributesCount);

        sayUtf8Characters(
          &spk, text, attributes,
          textLength, attributesCount, options
        );
      }
    }

    from = to;
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
