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

#ifndef BRLTTY_INCLUDED_NOTES
#define BRLTTY_INCLUDED_NOTES

#include "note_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NOTES_PER_OCTAVE 12
#define NOTE_MIDDLE_C 60

extern unsigned char getLowestNote (void);
extern unsigned char getHighestNote (void);
extern uint32_t getIntegerNoteFrequency (unsigned char note);
extern unsigned char getNearestNote (NOTE_FREQUENCY_TYPE frequency);

#ifndef NO_FLOAT
extern float getRealNoteFrequency (unsigned char note);
#endif /* NO_FLOAT */

#ifdef NO_FLOAT
#define GET_NOTE_FREQUENCY getIntegerNoteFrequency
#else /* NO_FLOAT */
#define GET_NOTE_FREQUENCY getRealNoteFrequency
#endif /* NO_FLOAT */

typedef struct NoteDeviceStruct NoteDevice;

typedef struct {
  NoteDevice * (*construct) (int errorLevel);
  void (*destruct) (NoteDevice *device);

  int (*frequency) (NoteDevice *device, NOTE_FREQUENCY_TYPE frequency, unsigned int duration);
  int (*note) (NoteDevice *device, unsigned char note, unsigned int duration);
  int (*flush) (NoteDevice *device);
} NoteMethods;

extern const NoteMethods beepNoteMethods;
extern const NoteMethods pcmNoteMethods;
extern const NoteMethods midiNoteMethods;
extern const NoteMethods fmNoteMethods;

extern char *opt_pcmDevice;
extern char *opt_midiDevice;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_NOTES */
