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
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_BEEP
#define BRLTTY_INCLUDED_BEEP

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int playBeep (unsigned short frequency, unsigned int duration);

extern int canBeep (void);
extern int synchronousBeep (unsigned short frequency, unsigned short milliseconds);
extern int asynchronousBeep (unsigned short frequency, unsigned short milliseconds);
extern int startBeep (unsigned short frequency);
extern int stopBeep (void);
extern void endBeep (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BEEP */
