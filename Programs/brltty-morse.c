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
#include <string.h>

#include "log.h"
#include "options.h"
#include "prefs.h"
#include "tune_utils.h"
#include "notes.h"
#include "tune.h"
#include "datafile.h"
#include "charset.h"

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

typedef uint8_t MorsePattern;

static const MorsePattern morsePatterns[] = {
  [WC_C('a')] = 0B101,
  [WC_C('b')] = 0B11110,
  [WC_C('c')] = 0B11010,
  [WC_C('d')] = 0B1110,
  [WC_C('e')] = 0B11,
  [WC_C('f')] = 0B11011,
  [WC_C('g')] = 0B1100,
  [WC_C('h')] = 0B11111,
  [WC_C('i')] = 0B111,
  [WC_C('j')] = 0B10001,
  [WC_C('k')] = 0B1010,
  [WC_C('l')] = 0B11101,
  [WC_C('m')] = 0B100,
  [WC_C('n')] = 0B110,
  [WC_C('o')] = 0B1000,
  [WC_C('p')] = 0B11001,
  [WC_C('q')] = 0B10100,
  [WC_C('r')] = 0B1101,
  [WC_C('s')] = 0B1111,
  [WC_C('t')] = 0B10,
  [WC_C('u')] = 0B1011,
  [WC_C('v')] = 0B10111,
  [WC_C('w')] = 0B1001,
  [WC_C('x')] = 0B10110,
  [WC_C('y')] = 0B10010,
  [WC_C('z')] = 0B11100,

  [WC_C('ä')] = 0B10101,
  [WC_C('á')] = 0B101001,
  [WC_C('å')] = 0B101001,
  [WC_C('é')] = 0B111011,
  [WC_C('ñ')] = 0B100100,
  [WC_C('ö')] = 0B11000,
  [WC_C('ü')] = 0B10011,

  [WC_C('0')] = 0B100000,
  [WC_C('1')] = 0B100001,
  [WC_C('2')] = 0B100011,
  [WC_C('3')] = 0B100111,
  [WC_C('4')] = 0B101111,
  [WC_C('5')] = 0B111111,
  [WC_C('6')] = 0B111110,
  [WC_C('7')] = 0B111100,
  [WC_C('8')] = 0B111000,
  [WC_C('9')] = 0B110000,

  [WC_C('.')] = 0B1010101,
  [WC_C(',')] = 0B1001100,
  [WC_C('?')] = 0B1110011,
  [WC_C('!')] = 0B1001010,
  [WC_C(':')] = 0B1111000,
  [WC_C('\'')] = 0B1100001,
  [WC_C('"')] = 0B1101101,
  [WC_C('(')] = 0B110010,
  [WC_C(')')] = 0B1010010,
  [WC_C('=')] = 0B101110,
  [WC_C('+')] = 0B110101,
  [WC_C('-')] = 0B1011110,
  [WC_C('/')] = 0B110110,
  [WC_C('&')] = 0B111101,
  [WC_C('@')] = 0B1101001,

  [0] = 0
};

typedef struct {
  struct {
    ToneElement *array;
    size_t size;
    size_t count;
  } elements;
} MorseData;

static int
addElement (const ToneElement *element, MorseData *morse) {
  if (morse->elements.count == morse->elements.size) {
    size_t newSize = morse->elements.size? (morse->elements.size << 1): 0X10;
    ToneElement *newArray = realloc(morse->elements.array, (newSize * sizeof(*newArray)));

    if (!newArray) {
      logMallocError();
      return 0;
    }

    morse->elements.array = newArray;
    morse->elements.size = newSize;
  }

  morse->elements.array[morse->elements.count++] = *element;
  return 1;
}

static int
addTone (unsigned int length, MorseData *morse) {
  ToneElement tone = TONE_PLAY((50 * length), 440);
  return addElement(&tone, morse);;
}

static int
addSilence (unsigned int length, MorseData *morse) {
  ToneElement tone = TONE_REST((50 * length));
  return addElement(&tone, morse);;
}

static int
addSpace (MorseData *morse) {
  return addSilence(2, morse);
}

static int
addMorse (MorsePattern pattern, MorseData *morse) {
  if (pattern) {
    while (pattern != 0B1) {
      unsigned int length = (pattern & 0B1)? 1: 3;
      if (!addTone(length, morse)) return 0;
      if (!addSilence(1, morse)) return 0;
      pattern >>= 1;
    }

    if (!addSilence(2, morse)) return 0;
  }

  return 1;
}

static int
addCharacter (wchar_t character, MorseData *morse) {
  character = towlower(character);
  MorsePattern pattern = (character < ARRAY_COUNT(morsePatterns))? morsePatterns[character]: 0;
  return addMorse(pattern, morse);
}

static int
addCharacters (const wchar_t *characters, size_t count, MorseData *morse) {
  const wchar_t *character = characters;
  const wchar_t *end = character + count;

  while (character < end) {
    if (!addCharacter(*character++, morse)) return 0;
  }

  return 1;
}

static int
addString (const char *string, MorseData *morse) {
  size_t size = strlen(string) + 1;
  wchar_t characters[size];

  const char *byte = string;
  wchar_t *end = characters;

  convertUtf8ToWchars(&byte, &end, size);
  return addCharacters(characters, (end - characters), morse);
}

static
DATA_OPERANDS_PROCESSOR(processMorseLine) {
  MorseData *morse = data;

  DataOperand text;
  getTextRemaining(file, &text);

  if (!addCharacters(text.characters, text.length, morse)) return 0;
  if (!addSpace(morse)) return 0;
  return 1;
}

static int
playMorse (MorseData *morse) {
  {
    ToneElement element = TONE_STOP();
    if (!addElement(&element, morse)) return 0;
  }

  tunePlayTones(morse->elements.array);
  tuneSynchronize();
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

  MorseData morse = {
    .elements = {
      .array = NULL,
      .size = 0,
      .count = 0
    }
  };

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
      if (!(addString(*argv, &morse) && addSpace(&morse))) {
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
    if (playMorse(&morse)) {
      exitStatus = PROG_EXIT_FATAL;
    }
  }

  return exitStatus;
}

#include "alert.h"

void
alert (AlertIdentifier identifier) {
}
