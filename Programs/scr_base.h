/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SCR_BASE
#define BRLTTY_INCLUDED_SCR_BASE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int isSpecialKey (ScreenKey key);
extern void setKeyModifiers (ScreenKey *key, ScreenKey which);

typedef struct {
  void (*describe) (ScreenDescription *);
  int (*readCharacters) (const ScreenBox *box, ScreenCharacter *buffer);
  int (*insertKey) (ScreenKey key);
  int (*routeCursor) (int column, int row, int screen);
  int (*highlightRegion) (int left, int right, int top, int bottom);
  int (*unhighlightRegion) (void);
  int (*getPointer) (int *column, int *row);
  int (*selectVirtualTerminal) (int vt);
  int (*switchVirtualTerminal) (int vt);
  int (*currentVirtualTerminal) (void);
  int (*executeCommand) (int command);
} BaseScreen;

extern void initializeBaseScreen (BaseScreen *);
extern void describeBaseScreen (BaseScreen *, ScreenDescription *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR_BASE */
