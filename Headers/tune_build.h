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

#ifndef BRLTTY_INCLUDED_TUNE_BUILD
#define BRLTTY_INCLUDED_TUNE_BUILD

#include "tune.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NOTES_PER_SCALE 7

typedef enum {
  TUNE_BUILD_OK,
  TUNE_BUILD_SYNTAX,
  TUNE_BUILD_FATAL
} TuneBuildStatus;

typedef unsigned int TuneNumber;

typedef struct {
  const char *name;
  TuneNumber minimum;
  TuneNumber maximum;
  TuneNumber current;
} TuneParameter;

typedef struct {
  TuneBuildStatus status;

  struct {
    ToneElement *array;
    unsigned int size;
    unsigned int count;
  } tones;

  signed char accidentals[NOTES_PER_SCALE];
  TuneParameter duration;
  TuneParameter note;
  TuneParameter octave;
  TuneParameter percentage;
  TuneParameter tempo;

  struct {
    const wchar_t *text;
    const char *name;
    unsigned int index;
  } source;
} TuneBuilder;

extern void initializeTuneBuilder (TuneBuilder *tune);
extern void resetTuneBuilder (TuneBuilder *tune);

extern int parseTuneString (TuneBuilder *tune, const char *string);
extern int parseTuneText (TuneBuilder *tune, const wchar_t *text);
extern int endTune (TuneBuilder *tune);

extern int addTone (TuneBuilder *tune, const ToneElement *tone);
extern int addNote (TuneBuilder *tune, unsigned char note, int duration);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_TUNE_BUILD */
