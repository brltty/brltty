/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2022 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_SCR_EMULATOR
#define BRLTTY_INCLUDED_SCR_EMULATOR

#include "scr_terminal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int createMessageQueue (int *queue, key_t key);
extern int destroyMessageQueue (int queue);

extern ScreenSegmentHeader *createScreenSegment (int *id, key_t key, int columns, int rows);
extern int destroyScreenSegment (int identifier);

extern void moveScreenCharacters (ScreenSegmentCharacter *to, const ScreenSegmentCharacter *from, size_t count);
extern void setScreenCharacters (ScreenSegmentCharacter *from, const ScreenSegmentCharacter *to, const ScreenSegmentCharacter *character);
extern void propagateScreenCharacter (ScreenSegmentCharacter *from, const ScreenSegmentCharacter *to);

#define SCREEN_SEGMENT_COLOR(r, g, b) {.red=r, .green=g, .blue=b}
#define SCREEN_SEGMENT_COLOR_LEVEL 0XAA
#define SCREEN_SEGMENT_COLOR_BLACK SCREEN_SEGMENT_COLOR(0, 0, 0)
#define SCREEN_SEGMENT_COLOR_WHITE SCREEN_SEGMENT_COLOR(SCREEN_SEGMENT_COLOR_LEVEL, SCREEN_SEGMENT_COLOR_LEVEL, SCREEN_SEGMENT_COLOR_LEVEL)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR_EMULATOR */
