/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_NOTES
#define BRLTTY_INCLUDED_NOTES

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
   int (*construct) (int errorLevel);
   int (*play) (int note, int duration);
   int (*flush) (void);
   void (*destruct) (void);
} NoteGenerator;

extern const NoteGenerator beeperNoteGenerator;
extern const NoteGenerator pcmNoteGenerator;
extern const NoteGenerator midiNoteGenerator;
extern const NoteGenerator fmNoteGenerator;

extern const float noteFrequencies[];
extern const unsigned int noteCount;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_NOTES */
