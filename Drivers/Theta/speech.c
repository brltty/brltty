/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 2003 by The BRLTTY Team. All rights reserved.
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

/* FestivalLite/speech.c - Speech library
 * For the Festival Lite text to speech package
 * Maintained by Mario Lang <mlang@delysid.org>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "Programs/spk.h"
#include "Programs/misc.h"

typedef enum {
  PARM_AGE,
  PARM_GENDER,
  PARM_LANGUAGE,
  PARM_NAME,
  PARM_PITCH,
  PARM_RATE,
  PARM_VOLUME
} DriverParameter;
#define SPKPARMS "age", "gender", "language", "name", "pitch", "rate", "volume"

#include "Programs/spk_driver.h"
#include <theta.h>

static cst_voice *voice = NULL;
static pid_t child = -1;
static int pipeDescriptors[2];
static const int *pipeOutput = &pipeDescriptors[0];
static const int *pipeInput = &pipeDescriptors[1];

static void
initializeTheta (void) {
  static int initialized = 0;
  if (!initialized) {
    {
      const char *directory = THETA_ROOT "/voices";
      setenv("THETA_VOXPATH", directory, 0);
    }

    setenv("THETA_HOME", THETA_ROOT, 0);
    theta_init(NULL);
    initialized = 1;
  }
}

static void
spk_identify (void) {
  initializeTheta();
  LogPrint(LOG_NOTICE, "Theta [%s] text to speech engine.", theta_version);
}

static void
loadVoice (theta_voice_desc *descriptor) {
  if ((voice = theta_load_voice(descriptor))) {
    LogPrint(LOG_INFO, "Voice: %s(%s,%d)",
             theta_voice_human(voice),
             theta_voice_gender(voice),
             theta_voice_age(voice));
  } else {
    LogPrint(LOG_WARNING, "Voice load error: %s [%s]",
             descriptor->human, descriptor->voxname);
  }
}

static int
spk_open (char **parameters) {
  theta_voice_search criteria;
  memset(&criteria, 0, sizeof(criteria));
  initializeTheta();

  if (*parameters[PARM_GENDER]) {
    const char *const choices[] = {"male", "female", "neuter", NULL};
    unsigned int choice;
    if (validateChoice(&choice, "gender", parameters[PARM_GENDER], choices))
      criteria.gender = (char *)choices[choice];
  }

  if (*parameters[PARM_LANGUAGE]) criteria.lang = parameters[PARM_LANGUAGE];

  if (*parameters[PARM_AGE]) {
    const char *word = parameters[PARM_AGE];
    int value;
    int younger;
    static const int minimumAge = 1;
    static const int maximumAge = 99;
    if ((younger = word[0] == '-') && word[1]) ++word;
    if (validateInteger(&value, "age", word, &minimumAge, &maximumAge)) {
      if (younger) value = -value;
      criteria.age = value;
    }
  }

  {
    const char *name = parameters[PARM_NAME];
    if (name && (*name == '/')) {
      theta_voice_desc *descriptor = theta_try_voxdir(name, &criteria);
      if (descriptor) {
        loadVoice(descriptor);
        theta_free_voice_desc(descriptor);
      }
    } else {
      theta_voice_desc *descriptors = theta_enum_voices(theta_voxpath, &criteria);
      if (descriptors) {
        theta_voice_desc *descriptor;
        for (descriptor=descriptors; descriptor; descriptor=descriptor->next) {
          if (*name)
            if (strcasecmp(name, descriptor->human) != 0)
              continue;
          loadVoice(descriptor);
          if (voice) break;
        }
        theta_free_voicelist(descriptors);
      }
    }
  }

  if (voice) {
    {
      double pitch = 0.0;
      static const double minimumPitch = -2.0;
      static const double maximumPitch = 2.0;
      if (validateFloat(&pitch, "pitch shift", parameters[PARM_PITCH],
                        &minimumPitch, &maximumPitch))
        theta_set_pitch_shift(voice, pitch, NULL);
    }

    {
      double rate = 1.0;
      static const double minimumRate = 0.1;
      static const double maximumRate = 10.0;
      if (validateFloat(&rate, "rate adjustment", parameters[PARM_RATE],
                        &minimumRate, &maximumRate))
        theta_set_rate_stretch(voice, 1.0/rate, NULL);
    }

    {
      double volume = 1.0;
      static const double minimumVolume = 0.0;
      static const double maximumVolume = 10.0;
      if (validateFloat(&volume, "volume adjustment", parameters[PARM_VOLUME],
                        &minimumVolume, &maximumVolume))
        theta_set_rescale(voice, volume, NULL);
    }

    return 1;
  }

  LogPrint(LOG_WARNING, "No voices found.");
  return 0;
}

static int
doChild (void) {
  FILE *stream = fdopen(*pipeOutput, "r");
  char buffer[0X400];
  char *line;
  while ((line = fgets(buffer, sizeof(buffer), stream))) {
    theta_tts(line, voice);
  }
  return 0;
}

static void
spk_say (const unsigned char *buffer, int length) {
  if (voice) {
    if (child != -1) goto ready;

    if (pipe(pipeDescriptors) != -1) {
      if ((child = fork()) == -1) {
        LogError("fork");
      } else if (child == 0) {
        _exit(doChild());
      } else
      ready: {
        unsigned char text[length + 1];
        memcpy(text, buffer, length);
        text[length] = '\n';
        write(*pipeInput, text, sizeof(text));
        return;
      }

      close(*pipeInput);
      close(*pipeOutput);
    } else {
      LogError("pipe");
    }
  }
}

static void
spk_mute (void) {
  if (child != -1) {
    close(*pipeInput);
    close(*pipeOutput);

    kill(child, SIGKILL);
    child = -1;
  }
}

static void
spk_close (void) {
  spk_mute();

  if (voice) {
    theta_unload_voice(voice);
    voice = NULL;
  }
}
