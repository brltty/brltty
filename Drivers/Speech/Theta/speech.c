/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

/* Theta/speech.c - Speech library
 * For the Theta text to speech package
 * Maintained by Dave Mielke <dave@mielke.cc>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/wait.h>

#include "log.h"
#include "parse.h"

typedef enum {
  PARM_AGE,
  PARM_GENDER,
  PARM_LANGUAGE,
  PARM_NAME,
  PARM_PITCH
} DriverParameter;
#define SPKPARMS "age", "gender", "language", "name", "pitch"

#include "spk_driver.h"

struct theta_sfx_block_ops { /* used but not defined in header */
  char dummy;
};
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
loadVoice (theta_voice_desc *descriptor) {
  if ((voice = theta_load_voice(descriptor))) {
    logMessage(LOG_INFO, "Voice: %s(%s,%d)",
               theta_voice_human(voice),
               theta_voice_gender(voice),
               theta_voice_age(voice));
  } else {
    logMessage(LOG_WARNING, "Voice load error: %s [%s]",
               descriptor->human, descriptor->voxname);
  }
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
spk_say (volatile SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
  if (voice) {
    if (child != -1) goto ready;

    if (pipe(pipeDescriptors) != -1) {
      if ((child = fork()) == -1) {
        logSystemError("fork");
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
      logSystemError("pipe");
    }
  }
}

static void
spk_mute (volatile SpeechSynthesizer *spk) {
  if (child != -1) {
    close(*pipeInput);
    close(*pipeOutput);

    kill(child, SIGKILL);
    waitpid(child, NULL, 0);
    child = -1;
  }
}

static void
spk_setVolume (volatile SpeechSynthesizer *spk, unsigned char setting) {
  theta_set_rescale(voice, getFloatSpeechVolume(setting), NULL);
}

static void
spk_setRate (volatile SpeechSynthesizer *spk, unsigned char setting) {
  theta_set_rate_stretch(voice, 1.0/getFloatSpeechRate(setting), NULL);
}

static int
spk_construct (volatile SpeechSynthesizer *spk, char **parameters) {
  theta_voice_search criteria;

  memset(&criteria, 0, sizeof(criteria));
  initializeTheta();

  spk->setVolume = spk_setVolume;
  spk->setRate = spk_setRate;

  if (*parameters[PARM_GENDER]) {
    const char *const choices[] = {"male", "female", "neuter", NULL};
    unsigned int choice;
    if (validateChoice(&choice, parameters[PARM_GENDER], choices)) {
      criteria.gender = (char *)choices[choice];
    } else {
      logMessage(LOG_WARNING, "%s: %s", "invalid gender specification", parameters[PARM_GENDER]);
    }
  }

  if (*parameters[PARM_LANGUAGE]) criteria.lang = parameters[PARM_LANGUAGE];

  if (*parameters[PARM_AGE]) {
    const char *word = parameters[PARM_AGE];
    int value;
    int younger;
    static const int minimumAge = 1;
    static const int maximumAge = 99;
    if ((younger = word[0] == '-') && word[1]) ++word;
    if (validateInteger(&value, word, &minimumAge, &maximumAge)) {
      if (younger) value = -value;
      criteria.age = value;
    } else {
      logMessage(LOG_WARNING, "%s: %s", "invalid age specification", word);
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
      float pitch = 0.0;
      static const float minimumPitch = -2.0;
      static const float maximumPitch = 2.0;
      if (validateFloat(&pitch, parameters[PARM_PITCH], &minimumPitch, &maximumPitch)) {
        theta_set_pitch_shift(voice, pitch, NULL);
      } else {
        logMessage(LOG_WARNING, "%s: %s", "invalid pitch shift specification", parameters[PARM_PITCH]);
      }
    }

    logMessage(LOG_INFO, "Theta Engine: version %s", theta_version);
    return 1;
  }

  logMessage(LOG_WARNING, "No voices found.");
  return 0;
}

static void
spk_destruct (volatile SpeechSynthesizer *spk) {
  spk_mute(spk);

  if (voice) {
    theta_unload_voice(voice);
    voice = NULL;
  }
}
