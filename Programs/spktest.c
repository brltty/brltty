/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include "program.h"
#include "options.h"
#include "spk.h"
#include "misc.h"

static SpeechSynthesizer spk;

char *opt_pcmDevice;
static char *opt_speechRate;
static char *opt_textString;
static char *opt_speechVolume;
static char *opt_libraryDirectory;
static char *opt_dataDirectory;

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'D',
    .word = "data-directory",
    .flags = OPT_Hidden,
    .argument = "directory",
    .setting.string = &opt_dataDirectory,
    .defaultSetting = DATA_DIRECTORY,
    .description = "Path to directory for configuration files."
  },

  { .letter = 'L',
    .word = "library-directory",
    .flags = OPT_Hidden,
    .argument = "directory",
    .setting.string = &opt_libraryDirectory,
    .defaultSetting = LIBRARY_DIRECTORY,
    .description = "Path to directory for loading drivers."
  },

  { .letter = 't',
    .word = "text-string",
    .argument = "string",
    .setting.string = &opt_textString,
    .description = "Text to be spoken."
  },

  { .letter = 'v',
    .word = "volume",
    .argument = "loudness",
    .setting.string = &opt_speechVolume,
    .description = "Floating-point speech volume multiplier."
  },

  { .letter = 'r',
    .word = "rate",
    .argument = "speed",
    .setting.string = &opt_speechRate,
    .description = "Floating-point speech rate multiplier."
  },

  { .letter = 'd',
    .word = "device",
    .argument = "device",
    .setting.string = &opt_pcmDevice,
    .description = "Digital audio soundcard device specifier."
  },
END_OPTION_TABLE

static int
sayLine (char *line, void *data) {
  sayString(&spk, line, 0);
  return 1;
}

int
main (int argc, char *argv[]) {
  int status;
  const char *driver = NULL;
  void *object;
  float speechRate;
  float speechVolume;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "spktest",
      .argumentsSummary = "[driver [parameter=value ...]]"
    };
    processOptions(&descriptor, &argc, &argv);
  }

  {
    char **const paths[] = {
      &opt_libraryDirectory,
      &opt_dataDirectory,
      NULL
    };
    fixInstallPaths(paths);
  }

  speechRate = 1.0;
  if (opt_speechRate && *opt_speechRate) {
    static const float minimum = 0.1;
    static const float maximum = 10.0;
    if (!validateFloat(&speechRate, opt_speechRate, &minimum, &maximum)) {
      LogPrint(LOG_ERR, "%s: %s", "invalid rate multiplier", opt_speechRate);
      exit(2);
    }
  }

  speechVolume = 1.0;
  if (opt_speechVolume && *opt_speechVolume) {
    static const float minimum = 0.0;
    static const float maximum = 2.0;
    if (!validateFloat(&speechVolume, opt_speechVolume, &minimum, &maximum)) {
      LogPrint(LOG_ERR, "%s: %s", "invalid volume multiplier", opt_speechVolume);
      exit(2);
    }
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
      initializeSpeechSynthesizer(&spk);
      identifySpeechDriver(speech, 0);		/* start-up messages */
      if (speech->construct(&spk, parameterSettings)) {
        if (speech->rate) speech->rate(&spk, speechRate);
        if (speech->volume) speech->volume(&spk, speechVolume);

        if (opt_textString) {
          sayString(&spk, opt_textString, 0);
        } else {
          processLines(stdin, sayLine, NULL);
        }
        speech->destruct(&spk);		/* finish with the display */
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
