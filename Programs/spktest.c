/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

/* spktest.c - Test progrm for the speech synthesizer drivers.
 */

#include "prologue.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include "options.h"
#include "spk.h"
#include "misc.h"

char *opt_pcmDevice;
static char *opt_speechRate;
static char *opt_textString;
static char *opt_speechVolume;
static char *opt_libraryDirectory;
static char *opt_dataDirectory;

BEGIN_OPTION_TABLE
  {"pcm-device", "device", 'p', 0, 0,
   &opt_pcmDevice, NULL,
   "Device specifier for soundcard digital audio."},

  {"rate", "speed", 'r', 0, 0,
   &opt_speechRate, NULL,
   "Floating-point speech rate multiplier."},

  {"text-string", "string", 't', 0, 0,
   &opt_textString, NULL,
   "Text to be spoken."},

  {"volume", "loudness", 'v', 0, 0,
   &opt_speechVolume, NULL,
   "Floating-point speech volume multiplier."},

  {"data-directory", "directory", 'D', 0, 0,
   &opt_dataDirectory, DATA_DIRECTORY,
   "Path to directory for configuration files."},

  {"library-directory", "directory", 'L', 0, 0,
   &opt_libraryDirectory, LIBRARY_DIRECTORY,
   "Path to directory for loading drivers."},
END_OPTION_TABLE

static int
sayLine (char *line, void *data) {
  sayString(line);
  return 1;
}

int
main (int argc, char *argv[]) {
  int status;
  const char *driver = NULL;
  void *object;
  float speechRate;
  float speechVolume;

  processOptions(optionTable, optionCount,
                 "spktest", &argc, &argv,
                 NULL, NULL, NULL,
                 "[driver [parameter=value ...]]");

  if (opt_speechRate && *opt_speechRate) {
    speechRate = floatArgument(opt_speechRate, 0.1, 10.0, "multiplier");
  } else {
    speechRate = 1.0;
  }

  if (opt_speechVolume && *opt_speechVolume) {
    speechVolume = floatArgument(opt_speechVolume, 0.0, 200.0, "multiplier");
  } else {
    speechVolume = 1.0;
  }

  if (argc) {
    driver = *argv++;
    --argc;
  }

  if ((speech = loadSpeechDriver(driver, &object, opt_libraryDirectory))) {
    const char *const *parameterNames = speech->parameters;
    char **parameterSettings;
    if (!parameterNames) {
      static const char *const noNames[] = {NULL};
      parameterNames = noNames;
    }
    {
      const char *const *name = parameterNames;
      unsigned int count;
      char **setting;
      while (*name) ++name;
      count = name - parameterNames;
      if (!(parameterSettings = malloc((count + 1) * sizeof(*parameterSettings)))) {
        LogPrint(LOG_ERR, "insufficient memory.");
        exit(9);
      }
      setting = parameterSettings;
      while (count--) *setting++ = "";
      *setting = NULL;
    }
    while (argc) {
      char *assignment = *argv++;
      int ok = 0;
      char *delimiter = strchr(assignment, '=');
      if (!delimiter) {
        LogPrint(LOG_ERR, "missing speech driver parameter value: %s", assignment);
      } else if (delimiter == assignment) {
        LogPrint(LOG_ERR, "missing speech driver parameter name: %s", assignment);
      } else {
        size_t nameLength = delimiter - assignment;
        const char *const *name = parameterNames;
        while (*name) {
          if (strncasecmp(assignment, *name, nameLength) == 0) {
            parameterSettings[name - parameterNames] = delimiter + 1;
            ok = 1;
            break;
          }
          ++name;
        }
        if (!ok) LogPrint(LOG_ERR, "invalid speech driver parameter: %s", assignment);
      }
      if (!ok) exit(2);
      --argc;
    }

    if (chdir(opt_dataDirectory) != -1) {
      speech->identify();		/* start-up messages */
      if (speech->open(parameterSettings)) {
        if (speech->rate) speech->rate(speechRate);
        if (speech->volume) speech->volume(speechVolume);

        if (opt_textString) {
          sayString(opt_textString);
        } else {
          processLines(stdin, sayLine, NULL);
        }
        speech->close();		/* finish with the display */
        status = 0;
      } else {
        LogPrint(LOG_ERR, "can't initialize speech driver.");
        status = 5;
      }
    } else {
      LogPrint(LOG_ERR, "can't change directory to '%s': %s",
               opt_dataDirectory, strerror(errno));
      status = 4;
    }
  } else {
    LogPrint(LOG_ERR, "can't load speech driver.");
    status = 3;
  }
  return status;
}
