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

#ifndef BRLTTY_INCLUDED_TUNE
#define BRLTTY_INCLUDED_TUNE

#include "tune_types.h"
#include "note_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  unsigned char note;     /* standard MIDI values (0 means silence) */
                          /* 1 through 127 are semitones, 60 is middle C */
  unsigned char duration; /* milliseconds (0 means stop) */
} NoteElement;

#define NOTE_PLAY(duration,note) {note, duration}
#define NOTE_REST(duration) NOTE_PLAY(duration, 0)
#define NOTE_STOP() NOTE_REST(0)

typedef struct {
  NOTE_FREQUENCY_TYPE frequency; /* Herz (0 means silence) */
  int duration;        /* milliseconds (0 means stop) */
} FrequencyElement;

#define FREQ_PLAY(duration,frequency) {frequency, duration}
#define FREQ_REST(duration) FREQ_PLAY(duration, 0)
#define FREQ_STOP() FREQ_REST(0)

extern void suppressTuneDeviceOpenErrors (void);

extern void tunePlayNotes (const NoteElement *tune);
extern void tunePlayFrequencies (const FrequencyElement *tune);
extern void tuneWait (int time);
extern void tuneSync (void);
extern int tuneSetDevice (TuneDevice device);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_TUNE */
