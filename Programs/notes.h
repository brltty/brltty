/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifndef _NOTES_H
#define _NOTES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
   int (*open) (int errorLevel);
   int (*play) (int note, int duration);
   int (*flush) (void);
   void (*close) (void);
} NoteGenerator;

extern const NoteGenerator beeperNoteGenerator;
extern const NoteGenerator pcmNoteGenerator;
extern const NoteGenerator midiNoteGenerator;
extern const NoteGenerator fmNoteGenerator;

extern const double noteFrequencies[];
extern const unsigned int noteCount;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NOTES_H */
