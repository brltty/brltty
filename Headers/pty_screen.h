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

#ifndef BRLTTY_INCLUDED_PTY_SCREEN
#define BRLTTY_INCLUDED_PTY_SCREEN

#include "get_curses.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void ptyBeginScreen (void);
extern void ptyEndScreen (void);
extern void ptyRefreshScreen (void);

extern void ptyInsertLines (int count);
extern void ptyDeleteLines (int count);

extern void ptyInsertCharacters (int count);
extern void ptyDeleteCharacters (int count);
extern void ptyAddCharacter (unsigned char character);

extern void ptySetCursorVisibility (int visibility);
extern void ptySetAttributes (attr_t attributes);
extern void ptyAddAttributes (attr_t attributes);
extern void ptyRemoveAttributes (attr_t attributes);
extern void ptySetForegroundColor (unsigned char color);
extern void ptySetBackgroundColor (unsigned char color);

extern void ptyClearToEndOfScreen (void);
extern void ptyClearToEndOfLine (void);

extern int ptyGetCursorRow ();
extern int ptyGetCursorColumn ();

extern void ptySetCursorPosition (int row, int column);
extern void ptySetCursorRow (int row);
extern void ptySetCursorColumn (int column);

extern void ptySetScrollRegion (int top, int bottom);
extern void ptyMoveCursorUp (int amount);
extern void ptyMoveCursorDown (int amount);
extern void ptyMoveCursorLeft (int amount);
extern void ptyMoveCursorRight (int amount);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PTY_SCREEN */
