/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

#ifndef _TUNES_H
#define _TUNES_H

typedef struct {
   unsigned char note;     // standard MIDI values (0 means silence)
                           // 1 through 127 are semitones, 60 is middle C
   unsigned char duration; // milliseconds (0 means stop)
} ToneDefinition;
#define TONE_NOTE(duration,note) {note, duration}
#define TONE_WAIT(duration) TONE_NOTE(duration, 0)
#define TONE_STOP() TONE_WAIT(0)

typedef struct {
   char *message;
   unsigned int tactile;
   ToneDefinition *tones;
} TuneDefinition;
#define TUNE_TACTILE(duration,pattern) (((duration) << 8) | (pattern))

extern TuneDefinition tune_detected;
extern TuneDefinition tune_braille_off;
extern TuneDefinition tune_cut_begin;
extern TuneDefinition tune_cut_end;
extern TuneDefinition tune_toggle_on;
extern TuneDefinition tune_toggle_off;
extern TuneDefinition tune_link;
extern TuneDefinition tune_unlink;
extern TuneDefinition tune_freeze;
extern TuneDefinition tune_unfreeze;
extern TuneDefinition tune_skip_first;
extern TuneDefinition tune_skip;
extern TuneDefinition tune_skip_more;
extern TuneDefinition tune_wrap_down;
extern TuneDefinition tune_wrap_up;
extern TuneDefinition tune_bounce;
extern TuneDefinition tune_bad_command;
extern TuneDefinition tune_done;
extern TuneDefinition tune_mark_set;

extern void setTuneDevice (unsigned char device);
extern void closeTuneDevice (int force);
extern void playTune (TuneDefinition *tune);

extern const char *midiInstrumentTable[];
extern unsigned int midiInstrumentCount;

#endif /* !defined(_TUNES_H) */
