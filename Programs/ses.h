/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SES
#define BRLTTY_INCLUDED_SES

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* 
 * Structure definition for volatile screen state variables.
 */

typedef struct {
  short column;
  short row;
} ScreenLocation;

typedef struct {
  int number;

  unsigned char trackCursor;		/* cursor tracking mode */
  unsigned char hideCursor;		/* for temporarily hiding the cursor */
  unsigned char displayMode;		/* text or attributes display */

  int winx, winy;	/* upper-left corner of braille window */
  int motx, moty;	/* last user motion of braille window */
  int trkx, trky;	/* tracked cursor position */
  int ptrx, ptry;	/* last known mouse/pointer position */
  int spkx, spky;	/* current speech position */

  ScreenLocation marks[0X100];
} SessionEntry;

extern SessionEntry *getSessionEntry (int number);
extern void deallocateSessionEntries (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SES */
