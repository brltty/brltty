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

#ifndef _TONES_H
#define _TONES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
   int (*open) (void);
   int (*generate) (int note, int duration);
   int (*flush) (void);
   void (*close) (void);
} ToneGenerator;

extern const ToneGenerator speakerToneGenerator;
extern const ToneGenerator dacToneGenerator;
extern const ToneGenerator midiToneGenerator;
extern const ToneGenerator adlibToneGenerator;

extern const double noteFrequencies[];
extern const unsigned int noteCount;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _TONES_H */
