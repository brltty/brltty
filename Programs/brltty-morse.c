/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>

#include "log.h"
#include "options.h"
#include "prefs.h"
#include "tune_utils.h"
#include "notes.h"
#include "datafile.h"
#include "morse.h"

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

static
DATA_OPERANDS_PROCESSOR(processMorseLine) {
  MorseObject *morse = data;

  DataOperand text;
  getTextRemaining(file, &text);

  if (!addMorseCharacters(morse, text.characters, text.length)) return 0;
  if (!addMorseCharacter(morse, WC_C(' '))) return 0;
  return 1;
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-morse",
      .argumentsSummary = "text... | -f [{file | -}...]"
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
  ProgramExitStatus exitStatus = PROG_EXIT_FATAL;
  MorseObject *morse;

  if ((morse = newMorseObject())) {
    if (opt_fromFiles) {
      const InputFilesProcessingParameters parameters = {
        .dataFileParameters = {
          .processOperands = processMorseLine,
          .data = &morse
        }
      };

      exitStatus = processInputFiles(argv, argc, &parameters);
    } else if (argc) {
      exitStatus = PROG_EXIT_SUCCESS;

      do {
        if (!(addMorseString(morse, *argv) && addMorseCharacter(morse, WC_C(' ')))) {
          exitStatus = PROG_EXIT_FATAL;
          break;
        }

        argv += 1;
      } while (argc -= 1);
    } else {
      logMessage(LOG_ERR, "missing text");
      exitStatus = PROG_EXIT_SYNTAX;
    }

    if (exitStatus == PROG_EXIT_SUCCESS) {
      if (!playMorsePatterns(morse)) {
        exitStatus = PROG_EXIT_FATAL;
      }
    }

    destroyMorseObject(morse);
    morse = NULL;
  }

  return exitStatus;
}

#include "alert.h"

void
alert (AlertIdentifier identifier) {
}
