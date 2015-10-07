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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  unsigned char note;     /* standard MIDI values (0 means silence) */
                          /* 1 through 127 are semitones, 60 is middle C */
  unsigned char duration; /* milliseconds (0 means stop) */
} TuneElement;

#define TUNE_NOTE(duration,note) {note, duration}
#define TUNE_REST(duration) TUNE_NOTE(duration, 0)
#define TUNE_STOP() TUNE_REST(0)

extern void suppressTuneDeviceOpenErrors (void);

extern void tunePlay (const TuneElement *tune);
extern void tuneWait (int time);
extern void tuneSync (void);
extern int tuneDevice (TuneDevice device);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_TUNE */
