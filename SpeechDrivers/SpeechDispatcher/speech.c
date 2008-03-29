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

static SPDConnection *connection = NULL;

static int
spk_construct (SpeechSynthesizer *spk, char **parameters) {
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

  if ((connection = spd_open("brltty", "driver", NULL, SPD_MODE_THREADED))) {
    if (parameters[PARM_MODULE] && *parameters[PARM_MODULE]) {
      spd_set_output_module(connection, parameters[PARM_MODULE]);
    }

    if (parameters[PARM_LANGUAGE] && *parameters[PARM_LANGUAGE]) {
      spd_set_language(connection, parameters[PARM_LANGUAGE]);
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
        spd_set_voice_type(connection, voices[choice]);
      } else {
        LogPrint(LOG_WARNING, "%s: %s", "invalid voice type", parameters[PARM_VOICE]);
      }
    }

    return 1;
  } else {
    LogPrint(LOG_ERR, "speech dispatcher open failure");
  }

  return 0;
}

static void
spk_destruct (SpeechSynthesizer *spk) {
  if (connection) {
    spd_close(connection);
    connection = NULL;
  }
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
  spd_sayf(connection, SPD_TEXT, "%.*s", length, buffer);
}

static void
spk_mute (SpeechSynthesizer *spk) {
  spd_cancel(connection);
}

static void
spk_rate (SpeechSynthesizer *spk, unsigned char setting) {
  int value = getIntegerSpeechRate(setting, 100) - 100;
  spd_set_voice_rate(connection, value);
  LogPrint(LOG_DEBUG, "set rate: %u -> %d", setting, value);
}

static void
spk_volume (SpeechSynthesizer *spk, unsigned char setting) {
  int value = getIntegerSpeechVolume(setting, 100) - 100;
  spd_set_volume(connection, value);
  LogPrint(LOG_DEBUG, "set volume: %u -> %d", setting, value);
}

static void
spk_punctuation (SpeechSynthesizer *spk, SpeechPunctuation setting) {
  SPDPunctuation value = (setting <= SPK_PUNCTUATION_NONE)? SPD_PUNCT_NONE: 
                         (setting >= SPK_PUNCTUATION_ALL)? SPD_PUNCT_ALL: 
                         SPD_PUNCT_SOME;
  spd_set_punctuation(connection, value);
  LogPrint(LOG_NOTICE, "set punctuation: %u -> %d", setting, value);
}
