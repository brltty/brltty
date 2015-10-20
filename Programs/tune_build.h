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

typedef struct {
  const char *name;
  unsigned char minimum;
  unsigned char maximum;
  unsigned char current;
} TuneParameter;

typedef struct {
  struct {
    FrequencyElement *array;
    unsigned int size;
    unsigned int count;
  } tones;

  TuneParameter tempo;

  struct {
    TuneParameter numerator;
    TuneParameter denominator;
  } meter;

  struct {
    const char *name;
    unsigned int index;
  } source;
} TuneBuilder;

extern void constructTuneBuilder (TuneBuilder *tune);
extern void destructTuneBuilder (TuneBuilder *tune);

extern int parseTuneLine (TuneBuilder *tune, const char *line);
extern int endTune (TuneBuilder *tune);

extern int addTone (TuneBuilder *tune, const FrequencyElement *tone);
extern int addNote (TuneBuilder *tune, unsigned char note, int duration);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_TUNE_BUILD */
