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

#ifndef BRLTTY_INCLUDED_STATDEFS
#define BRLTTY_INCLUDED_STATDEFS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  sfEnd,
  sfWindowCoordinates,
  sfWindowColumn,
  sfWindowRow,
  sfCursorCoordinates,
  sfCursorColumn,
  sfCursorRow,
  sfCursorAndWindowColumn,
  sfCursorAndWindowRow,
  sfScreenNumber,
  sfStateDots,
  sfStateLetter,
  sfTime,
  sfAlphabeticWindowCoordinates,
  sfAlphabeticCursorCoordinates,
  sfGeneric
} StatusField;

#define GSC_MARKER 0XFF /* must be in GSC_FIRST */
typedef enum {
  GSC_FIRST = 0 /* must be first */,

  /* numbers */
  gscWindowColumn /* screen column where left of braille window is */,
  gscWindowRow /* screen row where top of braille window is */,
  gscCursorColumn /* screen column where cursor is */,
  gscCursorRow /* screen row where cursor is */,
  gscScreenNumber /* virtual screen number */,

  /* flags */
  gscFrozenScreen /* frozen screen */,
  gscDisplayMode /* attributes display */,
  gscTextStyle /* six-dot braille */,
  gscSlidingWindow /* sliding window */,
  gscSkipIdenticalLines /* skip identical lines */,
  gscSkipBlankWindows /* skip blank windows */,
  gscShowCursor /* visible cursor */,
  gscHideCursor /* hidden cursor */,
  gscTrackCursor /* cursor tracking */,
  gscCursorStyle /* block cursor */,
  gscBlinkingCursor /* blinking cursor */,
  gscShowAttributes /* visible attributes underline */,
  gscBlinkingAttributes /* blinking attributes underline */,
  gscBlinkingCapitals /* blinking capital letters */,
  gscAlertTunes /* alert tunes */,
  gscHelpScreen /* help mode */,
  gscInfoMode /* info mode */,
  gscAutorepeat /* autorepeat */,
  gscAutospeak /* autospeak */,

  GSC_COUNT /* must be last */
} BRL_GenericStatusCell;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_STATDEFS */
