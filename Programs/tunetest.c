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
#include "options.h"
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

static void
beginTuneStream (const char *name, void *data) {
  TuneBuilder *tune = data;
  resetTuneBuilder(tune);
  tune->source.name = name;
}

static void
playTune (TuneBuilder *tune) {
  if (tune->status == TUNE_BUILD_OK) {
    if (endTune(tune)) {
      tunePlayFrequencies(tune->tones.array);
      tuneSynchronize();
    }
  }
}

static void
endTuneStream (int incomplete, void *data) {
  if (!incomplete) {
    TuneBuilder *tune = data;
    playTune(tune);
  }
}

static int
handleTuneLine (char *line, void *data) {
  TuneBuilder *tune = data;
  tune->source.index += 1;
  return parseTuneLine(tune, line);
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "tunetest",
      .argumentsSummary = "note... | -f [{file | -}...]"
    };
    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  resetPreferences();
  if (!parseTuneDevice(opt_tuneDevice)) return PROG_EXIT_SYNTAX;
  if (!parseTuneVolume(opt_outputVolume)) return PROG_EXIT_SYNTAX;

#ifdef HAVE_MIDI_SUPPORT
  if (!parseTuneInstrument(opt_midiInstrument)) return PROG_EXIT_SYNTAX;
#endif /* HAVE_MIDI_SUPPORT */

  if (!setTuneDevice()) return PROG_EXIT_SEMANTIC;
  TuneBuilder tune;
  initializeTuneBuilder(&tune);
  ProgramExitStatus exitStatus;

  if (opt_fromFiles) {
    const InputFilesProcessingParameters parameters = {
      .beginStream = beginTuneStream,
      .endStream = endTuneStream,
      .handleLine = handleTuneLine,
      .data = &tune
    };

    exitStatus = processInputFiles(argv, argc, &parameters);
  } else if (argc) {
    exitStatus = PROG_EXIT_SUCCESS;
    tune.source.name = "<command-line>";

    do {
      tune.source.index += 1;
      if (!parseTuneLine(&tune, *argv)) break;
      argv += 1;
    } while (argc -= 1);

    playTune(&tune);
  } else {
    logMessage(LOG_ERR, "missing tune");
    exitStatus = PROG_EXIT_SYNTAX;
  }

  if (exitStatus == PROG_EXIT_SUCCESS) {
    switch (tune.status) {
      case TUNE_BUILD_OK:
        exitStatus = PROG_EXIT_SUCCESS;
        break;

      case TUNE_BUILD_SYNTAX:
        exitStatus = PROG_EXIT_SYNTAX;
        break;

      case TUNE_BUILD_FATAL:
        exitStatus = PROG_EXIT_FATAL;
        break;
    }
  } else if (exitStatus == PROG_EXIT_FORCE) {
    exitStatus = PROG_EXIT_SUCCESS;
  }

  resetTuneBuilder(&tune);
  return exitStatus;
}
