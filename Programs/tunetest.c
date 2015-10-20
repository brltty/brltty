/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* tunetest.c - Test program for the tune playing library
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "program.h"
#include "options.h"
#include "file.h"
#include "prefs.h"
#include "tune_utils.h"
#include "tune_build.h"
#include "notes.h"

static int opt_fromFiles;
static char *opt_outputVolume;
static char *opt_tuneDevice;

#ifdef HAVE_MIDI_SUPPORT
static char *opt_midiInstrument;
#endif /* HAVE_MIDI_SUPPORT */

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'f',
    .word = "files",
    .setting.flag = &opt_fromFiles,
    .description = "Use files rather than command line arguments."
  },

  { .letter = 'v',
    .word = "volume",
    .argument = "loudness",
    .setting.string = &opt_outputVolume,
    .description = "Output volume (percentage)."
  },

  { .letter = 'd',
    .word = "device",
    .argument = "device",
    .setting.string = &opt_tuneDevice,
    .description = "Name of tune device."
  },

#ifdef HAVE_PCM_SUPPORT
  { .letter = 'p',
    .word = "pcm-device",
    .flags = OPT_Hidden,
    .argument = "device",
    .setting.string = &opt_pcmDevice,
    .description = "Device specifier for soundcard digital audio."
  },
#endif /* HAVE_PCM_SUPPORT */

#ifdef HAVE_MIDI_SUPPORT
  { .letter = 'm',
    .word = "midi-device",
    .flags = OPT_Hidden,
    .argument = "device",
    .setting.string = &opt_midiDevice,
    .description = "Device specifier for the Musical Instrument Digital Interface."
  },

  { .letter = 'i',
    .word = "instrument",
    .argument = "instrument",
    .setting.string = &opt_midiInstrument,
    .description = "Name of MIDI instrument."
  },
#endif /* HAVE_MIDI_SUPPORT */
END_OPTION_TABLE

static int
processLine (char *line, void *data) {
  TuneBuilder *tune = data;
  tune->source.index += 1;
  return parseTuneLine(tune, line);
}

static int
processStream (TuneBuilder *tune, FILE *stream) {
  return processLines(stream, processLine, tune);
}

static int
processStandardInput (TuneBuilder *tune) {
  tune->source.name = standardInputName;
  return processStream(tune, stdin);
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "tunetest",
      .argumentsSummary = "[note][/[duration]] ..."
    };
    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  resetPreferences();
  if (!parseTuneDevice(opt_tuneDevice)) return PROG_EXIT_SYNTAX;
  if (!parseTuneVolume(opt_outputVolume)) return PROG_EXIT_SYNTAX;

#ifdef HAVE_MIDI_SUPPORT
  if (!parseTuneInstrument(opt_midiInstrument)) return PROG_EXIT_SYNTAX;
#endif /* HAVE_MIDI_SUPPORT */

  TuneBuilder tune;
  initializeTuneBuilder(&tune);

  if (opt_fromFiles) {
    if (argc) {
      do {
        const char *argument = *argv++;

        if (strcmp(argument, standardStreamArgument) == 0) {
          if (!processStandardInput(&tune)) return PROG_EXIT_FATAL;
        } else {
          FILE *stream = fopen(argument, "r");

          if (stream) {
            tune.source.name = argument;
            tune.source.index = 0;
            processStream(&tune, stream);
            fclose(stream);
          }
        }
      } while (argc -= 1);
    } else {
      if (!processStandardInput(&tune)) return PROG_EXIT_FATAL;
    }
  } else {
    tune.source.name = "<command-line>";

    if (!argc) {
      logMessage(LOG_ERR, "missing tune");
      return PROG_EXIT_SYNTAX;
    }

    while (argc) {
      tune.source.index += 1;
      if (!parseTuneLine(&tune, *argv)) return PROG_EXIT_SYNTAX;
      argv += 1, argc -= 1;
    }
  }

  if (!endTune(&tune)) return PROG_EXIT_FATAL;
  if (!setTuneDevice()) return PROG_EXIT_SEMANTIC;
  tunePlayFrequencies(tune.tones.array);
  tuneSynchronize();
  resetTuneBuilder(&tune);

  return PROG_EXIT_SUCCESS;
}
