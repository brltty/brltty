/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "log.h"
#include "parameters.h"
#include "program.h"
#include "file.h"
#include "parse.h"
#include "prefs.h"
#include "charset.h"
#include "spk.h"
#include "spk_thread.h"
#include "brltty.h"

void
initializeSpeechSynthesizer (SpeechSynthesizer *spk) {
  spk->canAutospeak = 1;

  spk->setVolume = NULL;
  spk->setRate = NULL;
  spk->setPitch = NULL;
  spk->setPunctuation = NULL;

  spk->data = NULL;
}

static SpeechDriverThread *speechDriverThread = NULL;

int speechTracking = 0;
int speechScreen = -1;
int speechLine = 0;
int speechIndex = SPK_INDEX_NONE;

int
startSpeechDriverThread (SpeechSynthesizer *spk, char **parameters) {
  if (!speechDriverThread) {
    if (!(speechDriverThread = newSpeechDriverThread(spk, parameters))) {
      return 0;
    }
  }

  return 1;
}

void
stopSpeechDriverThread (void) {
  if (speechDriverThread) {
    SpeechDriverThread *sdt = speechDriverThread;

    speechDriverThread = NULL;
    destroySpeechDriverThread(sdt);
  }
}

int
tellSpeechFinished (void) {
  if (!speechTracking) return 1;
  return speechMessage_speechFinished(speechDriverThread);
}

void
setSpeechFinished (void) {
  speechTracking = 0;
  speechIndex = SPK_INDEX_NONE;

  endAutospeakDelay();
}

int
tellSpeechIndex (int index) {
  if (!speechTracking) return 1;
  return speechMessage_speechLocation(speechDriverThread, index);
}

void
setSpeechIndex (int index) {
  if (speechTracking) {
    if (scr.number == speechScreen) {
      if (index != speechIndex) {
        speechIndex = index;
        if (ses->trackCursor) trackSpeech();
      }

      return;
    }

    setSpeechFinished();
  }
}

int
muteSpeech (const char *reason) {
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "mute: %s", reason);
  return speechRequest_muteSpeech(speechDriverThread);
}

int
sayUtf8Characters (
  const char *text, const unsigned char *attributes,
  size_t length, size_t count,
  int immediate
) {
  if (count) {
    if (immediate) {
      if (!muteSpeech("say immediate")) {
        return 0;
      }
    }

    logMessage(LOG_CATEGORY(SPEECH_EVENTS), "say: %s", text);
    if (!speechRequest_sayText(speechDriverThread, text, length, count, attributes)) return 0;
  }

  return 1;
}

void
sayString (const char *string, int immediate) {
  sayUtf8Characters(string, NULL, strlen(string), getTextLength(string), immediate);
}

static void
sayStringSetting (const char *name, const char *string) {
  char statement[0X40];
  snprintf(statement, sizeof(statement), "%s %s", name, string);
  sayString(statement, 1);
}

static void
sayIntegerSetting (const char *name, int integer) {
  char string[0X10];
  snprintf(string, sizeof(string), "%d", integer);
  sayStringSetting(name, string);
}

static unsigned int
getIntegerSetting (unsigned char setting, unsigned char internal, unsigned int external) {
  return rescaleInteger(setting, internal, external);
}

int
canSetSpeechVolume (void) {
  return spk.setVolume != NULL;
}

int
setSpeechVolume (int setting, int say) {
  if (!canSetSpeechVolume()) return 0;
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "set volume: %d", setting);
  speechRequest_setVolume(speechDriverThread, setting);
  if (say) sayIntegerSetting(gettext("volume"), setting);
  return 1;
}

unsigned int
getIntegerSpeechVolume (unsigned char setting, unsigned int normal) {
  return getIntegerSetting(setting, SPK_VOLUME_DEFAULT, normal);
}

#ifndef NO_FLOAT
float
getFloatSpeechVolume (unsigned char setting) {
  return (float)setting / (float)SPK_VOLUME_DEFAULT;
}
#endif /* NO_FLOAT */

int
canSetSpeechRate (void) {
  return spk.setRate != NULL;
}

int
setSpeechRate (int setting, int say) {
  if (!canSetSpeechRate()) return 0;
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "set rate: %d", setting);
  speechRequest_setRate(speechDriverThread, setting);
  if (say) sayIntegerSetting(gettext("rate"), setting);
  return 1;
}

unsigned int
getIntegerSpeechRate (unsigned char setting, unsigned int normal) {
  return getIntegerSetting(setting, SPK_RATE_DEFAULT, normal);
}

#ifndef NO_FLOAT
float
getFloatSpeechRate (unsigned char setting) {
  static const float spkRateTable[] = {
    0.3333,
    0.3720,
    0.4152,
    0.4635,
    0.5173,
    0.5774,
    0.6444,
    0.7192,
    0.8027,
    0.8960,
    1.0000,
    1.1161,
    1.2457,
    1.3904,
    1.5518,
    1.7320,
    1.9332,
    2.1577,
    2.4082,
    2.6879,
    3.0000
  };

  return spkRateTable[setting];
}
#endif /* NO_FLOAT */

int
canSetSpeechPitch (void) {
  return spk.setPitch != NULL;
}

int
setSpeechPitch (int setting, int say) {
  if (!canSetSpeechPitch()) return 0;
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "set pitch: %d", setting);
  speechRequest_setPitch(speechDriverThread, setting);
  if (say) sayIntegerSetting(gettext("pitch"), setting);
  return 1;
}

unsigned int
getIntegerSpeechPitch (unsigned char setting, unsigned int normal) {
  return getIntegerSetting(setting, SPK_PITCH_DEFAULT, normal);
}

#ifndef NO_FLOAT
float
getFloatSpeechPitch (unsigned char setting) {
  return (float)setting / (float)SPK_PITCH_DEFAULT;
}
#endif /* NO_FLOAT */

int
canSetSpeechPunctuation (void) {
  return spk.setPunctuation != NULL;
}

int
setSpeechPunctuation (SpeechPunctuation setting, int say) {
  if (!canSetSpeechPunctuation()) return 0;
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "set punctuation: %d", setting);
  speechRequest_setPunctuation(speechDriverThread, setting);
  return 1;
}
