/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

typedef struct {
   unsigned int frequency;
   unsigned int duration;
} ToneDefinition;

typedef struct {
   char *message;
   unsigned int tactile;
   ToneDefinition *tones;
} TuneDefinition;

extern TuneDefinition tune_detected;
extern TuneDefinition tune_braille_off;
extern TuneDefinition tune_link;
extern TuneDefinition tune_unlink;
extern TuneDefinition tune_wrap_down;
extern TuneDefinition tune_wrap_up;
extern TuneDefinition tune_bounce;
extern TuneDefinition tune_freeze;
extern TuneDefinition tune_unfreeze;
extern TuneDefinition tune_cut_begin;
extern TuneDefinition tune_cut_end;
extern TuneDefinition tune_toggle_on;
extern TuneDefinition tune_toggle_off;
extern TuneDefinition tune_done;
extern TuneDefinition tune_skip_first;
extern TuneDefinition tune_skip;
extern TuneDefinition tune_skip_more;

extern void setTuneDevice (unsigned char device);
extern void closeTuneDevice (void);
extern void playTune (TuneDefinition *tune);
