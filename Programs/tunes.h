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

#ifndef _TUNES_H
#define _TUNES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
   unsigned char note;     /* standard MIDI values (0 means silence) */
                           /* 1 through 127 are semitones, 60 is middle C */
   unsigned char duration; /* milliseconds (0 means stop) */
} TuneElement;
#define TUNE_NOTE(duration,note) (TuneElement){note, duration}
#define TUNE_REST(duration) TUNE_NOTE(duration, 0)
#define TUNE_STOP() TUNE_REST(0)

typedef struct {
   char *message;
   unsigned int tactile;
   TuneElement *elements;
} TuneDefinition;
#define TUNE_TACTILE(duration,pattern) (((duration) << 8) | (pattern))

extern TuneDefinition tune_braille_on;
extern TuneDefinition tune_braille_off;
extern TuneDefinition tune_command_rejected;
extern TuneDefinition tune_command_done;
extern TuneDefinition tune_routing_succeeded;
extern TuneDefinition tune_routing_failed;
extern TuneDefinition tune_mark_set;
extern TuneDefinition tune_cut_begin;
extern TuneDefinition tune_cut_end;
extern TuneDefinition tune_toggle_on;
extern TuneDefinition tune_toggle_off;
extern TuneDefinition tune_cursor_linked;
extern TuneDefinition tune_cursor_unlinked;
extern TuneDefinition tune_screen_frozen;
extern TuneDefinition tune_screen_unfrozen;
extern TuneDefinition tune_wrap_down;
extern TuneDefinition tune_wrap_up;
extern TuneDefinition tune_skip_first;
extern TuneDefinition tune_skip;
extern TuneDefinition tune_skip_more;
extern TuneDefinition tune_bounce;

typedef enum {
  tdBeeper,
  tdPcm,
  tdMidi,
  tdFm
} TuneDevice;

extern TuneDevice getDefaultTuneDevice (void);
extern void suppressTuneDeviceOpenErrors (void);
extern int setTuneDevice (TuneDevice device);
extern void closeTuneDevice (int force);
extern void playTune (const TuneDefinition *tune);

extern const char *midiInstrumentTable[];
extern const unsigned int midiInstrumentCount;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _TUNES_H */
