/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

/* tunetest.c - Test program for the tune playing library
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "options.h"
#include "tunes.h"
#include "misc.h"
#include "defaults.h"
#include "brltty.h"
#include "message.h"
#include "brl.h"

int updateInterval = DEFAULT_UPDATE_INTERVAL;
Preferences prefs;

static char *opt_tuneDevice;
static char *opt_outputVolume;

#ifdef ENABLE_PCM_SUPPORT
char *opt_pcmDevice;
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
char *opt_midiDevice;
static char *opt_midiInstrument;
#endif /* ENABLE_MIDI_SUPPORT */

BEGIN_OPTION_TABLE
  {"device", "device", 'd', 0, 0,
   &opt_tuneDevice, NULL,
   "Name of tune device."},

#ifdef ENABLE_MIDI_SUPPORT
  {"instrument", "instrument", 'i', 0, 0,
   &opt_midiInstrument, NULL,
   "Name of MIDI instrument."},
#endif /* ENABLE_MIDI_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
  {"midi-device", "device", 'm', 0, 0,
   &opt_midiDevice, NULL,
   "Device specifier for the Musical Instrument Digital Interface."},
#endif /* ENABLE_MIDI_SUPPORT */

#ifdef ENABLE_PCM_SUPPORT
  {"pcm-device", "device", 'p', 0, 0,
   &opt_pcmDevice, NULL,
   "Device specifier for soundcard digital audio."},
#endif /* ENABLE_PCM_SUPPORT */

  {"level", "volume", 'v', 0, 0,
   &opt_outputVolume, NULL,
   "Output volume (percentage)."},
END_OPTION_TABLE

static const char *deviceNames[] = {"beeper", "pcm", "midi", "fm", NULL};

#ifdef ENABLE_MIDI_SUPPORT
static unsigned char
instrumentArgument (const char *argument) {
  size_t argumentLength = strlen(argument);
  unsigned char instrument;
  for (instrument=0; instrument<midiInstrumentCount; ++instrument) {
    const char *component = midiInstrumentTable[instrument];
    size_t componentLeft = strlen(component);
    const char *word = argument;
    size_t wordLeft = argumentLength;
    {
      const char *delimiter = memchr(component, '(', componentLeft);
      if (delimiter) componentLeft = delimiter - component;
    }
    while (1) {
      while (*component == ' ') component++, componentLeft--;
      if ((componentLeft == 0) != (wordLeft == 0)) break; 
      if (!componentLeft) return instrument;
      {
        size_t wordLength = wordLeft;
        size_t componentLength = componentLeft;
        const char *delimiter;
        if ((delimiter = memchr(word, '-', wordLeft))) wordLength = delimiter - word;
        if ((delimiter = memchr(component, ' ', componentLeft))) componentLength = delimiter - component;
        if (strncasecmp(word, component, wordLength) != 0) break;
        word += wordLength; wordLeft -= wordLength;
        if (*word) word++, wordLeft--;
        component += componentLength; componentLeft -= componentLength;
      }
    }
  }
  fprintf(stderr, "%s: Invalid instrument: %s\n", programName, argument);
  exit(2);
}
#endif /* ENABLE_MIDI_SUPPORT */

int
main (int argc, char *argv[]) {
  TuneDevice tuneDevice;
  unsigned char outputVolume;

#ifdef ENABLE_MIDI_SUPPORT
  unsigned char midiInstrument;
#endif /* ENABLE_MIDI_SUPPORT */

  processOptions(optionTable, optionCount,
                 "tunetest", &argc, &argv,
                 NULL, NULL, NULL,
                 "{note duration} ...");

  if (opt_tuneDevice && *opt_tuneDevice) {
    tuneDevice = wordArgument(opt_tuneDevice, deviceNames, "device");
  } else {
    tuneDevice = getDefaultTuneDevice();
  }

#ifdef ENABLE_MIDI_SUPPORT
  if (opt_midiInstrument && *opt_midiInstrument) {
    midiInstrument = instrumentArgument(opt_midiInstrument);
  } else {
    midiInstrument = 0;
  }
#endif /* ENABLE_MIDI_SUPPORT */

  if (opt_outputVolume && *opt_outputVolume) {
    outputVolume = integerArgument(opt_outputVolume, 0, 100, "level");
  } else {
    outputVolume = 50;
  }

  if (!argc) {
    fprintf(stderr, "%s: Missing tune.\n", programName);
  } else if (argc % 2) {
    fprintf(stderr, "%s: Missing duration.\n", programName);
  } else {
    unsigned int count = argc / 2;
    TuneElement *elements = (TuneElement *)mallocWrapper((sizeof(*elements) * count) + 1);
    TuneElement *element = elements;

    while (argc) {
      short note = integerArgument(*argv++, 1, 127, "note");
      short duration = integerArgument(*argv++, 1, 255, "duration");
      argc -= 2;
      *(element++) = TUNE_NOTE(duration, note);
    }
    *element = TUNE_STOP();

    if (!setTuneDevice(tuneDevice)) {
      fprintf(stderr, "%s: Unsupported tune device: %s\n", programName, deviceNames[tuneDevice]);
      exit(3);
    }

    memset(&prefs, 0, sizeof(prefs));
    prefs.alertMessages = 0;
    prefs.alertDots = 0;
    prefs.alertTunes = 1;
    switch (tuneDevice) {
      default:
        break;
      case tdPcm:
        prefs.pcmVolume = outputVolume;
        break;
      case tdMidi:
        prefs.midiVolume = outputVolume;
        break;
      case tdFm:
        prefs.fmVolume = outputVolume;
        break;
    }
#ifdef ENABLE_MIDI_SUPPORT
    prefs.midiInstrument = midiInstrument;
#endif /* ENABLE_MIDI_SUPPORT */
    {
      TuneDefinition tune = {NULL, 0, elements};
      playTune(&tune);
      closeTuneDevice(1);
    }

    free(elements);
    return 0;
  }
  return 2;
}

void
message (const char *text, short flags) {
}
void
showDotPattern (unsigned char dots, unsigned char duration) {
}
