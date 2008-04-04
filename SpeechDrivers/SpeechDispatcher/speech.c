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

#include "prologue.h"

#include <stdio.h>

#include "misc.h"

typedef enum {
  PARM_PORT,
  PARM_MODULE,
  PARM_LANGUAGE,
  PARM_VOICE
} DriverParameter;
#define SPKPARMS "port", "module", "language", "voice"

#define SPK_HAVE_RATE
#define SPK_HAVE_VOLUME
#define SPK_HAVE_PUNCTUATION
#include "spk_driver.h"

#include <libspeechd.h>

static SPDConnection *connectionHandle = NULL;
static const char *moduleName;
static const char *languageName;
static SPDVoiceType voiceType;
static signed int relativeRate;
static signed int relativeVolume;
static SPDPunctuation punctuationVerbosity;

static void
clearSettings (void) {
  moduleName = NULL;
  languageName = NULL;
  voiceType = -1;
  relativeRate = 0;
  relativeVolume = 0;
  punctuationVerbosity = -1;
}

static void
closeConnection (void) {
  if (connectionHandle) {
    spd_close(connectionHandle);
    connectionHandle = NULL;
  }
}

typedef void (*SpeechdAction) (const void *data);

static void
speechdAction (SpeechdAction action, const void *data) {
  if (connectionHandle) {
    action(data);
    if (!connectionHandle->stream) closeConnection();
  }
}

static void
setModule (const void *data) {
  if (moduleName) spd_set_output_module(connectionHandle, moduleName);
}

static void
setLanguage (const void *data) {
  if (languageName) spd_set_language(connectionHandle, languageName);
}

static void
setVoice (const void *data) {
  if (voiceType != -1) spd_set_voice_type(connectionHandle, voiceType);
}

static void
setRate (const void *data) {
  spd_set_voice_rate(connectionHandle, relativeRate);
}

static void
setVolume (const void *data) {
  spd_set_volume(connectionHandle, relativeVolume);
}

static void
setPunctuation (const void *data) {
  if (punctuationVerbosity != -1) spd_set_punctuation(connectionHandle, punctuationVerbosity);
}

static void
cancelSpeech (const void *data) {
  spd_cancel(connectionHandle);
}

static int
openConnection (void) {
  if (!connectionHandle) {
    if (!(connectionHandle = spd_open("brltty", "main", NULL, SPD_MODE_THREADED))) {
      LogPrint(LOG_ERR, "speech dispatcher open failure");
      return 0;
    }

    speechdAction(setModule, NULL);
    speechdAction(setLanguage, NULL);
    speechdAction(setVoice, NULL);
    speechdAction(setRate, NULL);
    speechdAction(setVolume, NULL);
    speechdAction(setPunctuation, NULL);
  }

  return 1;
}

static int
spk_construct (SpeechSynthesizer *spk, char **parameters) {
  clearSettings();

  if (parameters[PARM_PORT] && *parameters[PARM_PORT]) {
    static const int minimumPort = 0X1;
    static const int maximumPort = 0XFFFF;
    int port = 0;

    if (validateInteger(&port, parameters[PARM_PORT], &minimumPort, &maximumPort)) {
      char number[0X10];
      snprintf(number, sizeof(number), "%d", port);
      setenv("SPEECHD_PORT", number, 1);
    } else {
      LogPrint(LOG_WARNING, "%s: %s", "invalid port number", parameters[PARM_PORT]);
    }
  }

  if (parameters[PARM_MODULE] && *parameters[PARM_MODULE]) {
    moduleName = parameters[PARM_MODULE];
  }

  if (parameters[PARM_LANGUAGE] && *parameters[PARM_LANGUAGE]) {
    languageName = parameters[PARM_LANGUAGE];
  }

  if (parameters[PARM_VOICE] && *parameters[PARM_VOICE]) {
    static const SPDVoiceType voices[] = {
      SPD_MALE1, SPD_FEMALE1,
      SPD_MALE2, SPD_FEMALE2,
      SPD_MALE3, SPD_FEMALE3,
      SPD_CHILD_MALE, SPD_CHILD_FEMALE
    };

    static const char *choices[] = {
      "male1", "female1",
      "male2", "female2",
      "male3", "female3",
      "child_male", "child_female",
      NULL
    };

    unsigned int choice = 0;

    if (validateChoice(&choice, parameters[PARM_VOICE], choices)) {
      voiceType = voices[choice];
    } else {
      LogPrint(LOG_WARNING, "%s: %s", "invalid voice type", parameters[PARM_VOICE]);
    }
  }

  return openConnection();
}

static void
spk_destruct (SpeechSynthesizer *spk) {
  closeConnection();
  clearSettings();
}

typedef struct {
  const unsigned char *text;
  size_t length;
  size_t count;
  SPDPriority priority;
} SayData;

static void
sayText (const void *data) {
  const SayData *say = data;

  if (say->count == 1) {
    spd_char(connectionHandle, say->priority, (const char *)say->text);
  } else {
    spd_sayf(connectionHandle, say->priority, "%.*s", say->length, say->text);
  }
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *text, size_t length, size_t count, const unsigned char *attributes) {
  const SayData say = {
    .text = text,
    .length = length,
    .count = count,
    .priority = SPD_TEXT
  };

  speechdAction(sayText, &say);
  if (!connectionHandle) {
    if (openConnection()) {
      speechdAction(sayText, &say);
    }
  }
}

static void
spk_mute (SpeechSynthesizer *spk) {
  speechdAction(cancelSpeech, NULL);
}

static void
spk_rate (SpeechSynthesizer *spk, unsigned char setting) {
  relativeRate = getIntegerSpeechRate(setting, 100) - 100;
  speechdAction(setRate, NULL);
  LogPrint(LOG_DEBUG, "set rate: %u -> %d", setting, relativeRate);
}

static void
spk_volume (SpeechSynthesizer *spk, unsigned char setting) {
  relativeVolume = getIntegerSpeechVolume(setting, 100) - 100;
  speechdAction(setVolume, NULL);
  LogPrint(LOG_DEBUG, "set volume: %u -> %d", setting, relativeVolume);
}

static void
spk_punctuation (SpeechSynthesizer *spk, SpeechPunctuation setting) {
  punctuationVerbosity = (setting <= SPK_PUNCTUATION_NONE)? SPD_PUNCT_NONE: 
                         (setting >= SPK_PUNCTUATION_ALL)? SPD_PUNCT_ALL: 
                         SPD_PUNCT_SOME;
  speechdAction(setPunctuation, NULL);
  LogPrint(LOG_DEBUG, "set punctuation: %u -> %d", setting, punctuationVerbosity);
}
