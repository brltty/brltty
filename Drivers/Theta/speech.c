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

#define SPKNAME "Theta"

typedef enum {
  PARM_GENDER,
  PARM_LANGUAGE,
  PARM_NAME,
  PARM_AGE
} DriverParameter;
#define SPKPARMS "gender", "language", "name", "age"

#include "Programs/spk_driver.h"
#include <theta.h>

static theta_voice_desc *voiceList = NULL;
static theta_voice_desc *voiceEntry = NULL;
static cst_voice *voice = NULL;

static pid_t child = -1;
static int pipeDescriptors[2];
static const int *pipeOutput = &pipeDescriptors[0];
static const int *pipeInput = &pipeDescriptors[1];

static void
spk_identify (void) {
  LogPrint(LOG_NOTICE, "Using Theta");
}

static int
spk_open (char **parameters) {
  theta_voice_search criteria;
  memset(&criteria, 0, sizeof(criteria));
  theta_init(NULL);

  {
    const char *const choices[] = {"male", "female", "neuter", NULL};
    int choice;
    if (validateChoice(&choice, "gender", parameters[PARM_GENDER], choices))
      criteria.gender = (char *)choices[choice];
  }

  if (*parameters[PARM_LANGUAGE]) criteria.lang = parameters[PARM_LANGUAGE];

  if (*parameters[PARM_NAME]) criteria.name_regex = parameters[PARM_NAME];

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

  if ((voiceList = theta_enum_voices(theta_voxpath, &criteria))) {
    for (voiceEntry=voiceList; voiceEntry; voiceEntry=voiceEntry->next) {
      if ((voice = theta_load_voice(voiceEntry))) {
        LogPrint(LOG_INFO, "Using voice %s.", voiceEntry->voxname);
        return 1;
      } else {
        LogPrint(LOG_WARNING, "Voice load error: %s", voiceEntry->voxname);
      }
    }
  } else {
    LogPrint(LOG_WARNING, "No voices found.");
  }
  spk_close();
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

  if (voiceEntry) {
    voiceEntry = NULL;
  }

  if (voiceList) {
    theta_free_voicelist(voiceList);
    voiceList = NULL;
  }
}
