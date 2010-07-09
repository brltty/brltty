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

#define BRL_STATUS_CELLS_GENERIC 0XFF /* must be in BRL_firstStatusCell */
typedef enum {
  BRL_firstStatusCell = 0,

  /* numbers */
  BRL_GSC_BRLCOL /* screen column where left of braille window is */,
  BRL_GSC_BRLROW /* screen row where top of braille window is */,
  BRL_GSC_CSRCOL /* screen column where cursor is */,
  BRL_GSC_CSRROW /* screen row where cursor is */,
  BRL_GSC_SCRNUM /* virtual screen number */,

  /* flags */
  BRL_GSC_FREEZE /* frozen screen */,
  BRL_GSC_DISPMD /* attributes display */,
  BRL_GSC_SIXDOTS /* six-dot braille */,
  BRL_GSC_SLIDEWIN /* sliding window */,
  BRL_GSC_SKPIDLNS /* skip identical lines */,
  BRL_GSC_SKPBLNKWINS /* skip blank windows */,
  BRL_GSC_CSRVIS /* visible cursor */,
  BRL_GSC_CSRHIDE /* hidden cursor */,
  BRL_GSC_CSRTRK /* cursor tracking */,
  BRL_GSC_CSRSIZE /* block cursor */,
  BRL_GSC_CSRBLINK /* blinking cursor */,
  BRL_GSC_ATTRVIS /* visible attributes underline */,
  BRL_GSC_ATTRBLINK /* blinking attributes underline */,
  BRL_GSC_CAPBLINK /* blinking capital letters */,
  BRL_GSC_TUNES /* alert tunes */,
  BRL_GSC_HELP /* help mode */,
  BRL_GSC_INFO /* info mode */,
  BRL_GSC_AUTOREPEAT /* autorepeat */,
  BRL_GSC_AUTOSPEAK /* autospeak */,

  BRL_genericStatusCellCount
} BRL_GenericStatusCell;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_STATDEFS */
