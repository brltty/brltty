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

#include "prologue.h"

#include <time.h>

#include "status.h"
#include "brldefs.h"
#include "scr.h"
#include "brltty.h"
#include "ttb.h"

static void
renderDigitUpper (unsigned char *cell, int digit) {
  *cell |= portraitDigits[digit];
}

static void
renderDigitLower (unsigned char *cell, int digit) {
  *cell |= lowerDigit(portraitDigits[digit]);
}

static void
renderNumberUpper (unsigned char *cells, int number) {
  renderDigitUpper(&cells[0], (number / 10) % 10);
  renderDigitUpper(&cells[1], number % 10);
}

static void
renderNumberLower (unsigned char *cells, int number) {
  renderDigitLower(&cells[0], (number / 10) % 10);
  renderDigitLower(&cells[1], number % 10);
}

static void
renderNumberVertical (unsigned char *cell, int number) {
  renderDigitUpper(cell, (number / 10) % 10);
  renderDigitLower(cell, number % 10);
}

static void
renderCoordinatesVertical (unsigned char *cells, int column, int row) {
  renderNumberUpper(&cells[0], row);
  renderNumberLower(&cells[0], column);
}

static void
renderCoordinatesAlphabetic (unsigned char *cell, int column, int row) {
  /* the coordinates are presented as an underlined letter as the Alva DOS TSR */
  *cell = !SCR_COORDINATES_OK(column, row)? convertCharacterToDots(textTable, WC_C('z')):
          ((updateIntervals / 16) % (row / 25 + 1))? 0:
          convertCharacterToDots(textTable, (row % 25 + WC_C('a'))) |
          ((column / textCount) << 6);
}

typedef void (*RenderStatusField) (unsigned char *cells);

static void
renderStatusField_windowCoordinates (unsigned char *cells) {
  renderCoordinatesVertical(cells, SCR_COLUMN_NUMBER(ses->winx), SCR_ROW_NUMBER(ses->winy));
}

static void
renderStatusField_windowColumn (unsigned char *cells) {
  renderNumberVertical(cells, SCR_COLUMN_NUMBER(ses->winx));
}

static void
renderStatusField_windowRow (unsigned char *cells) {
  renderNumberVertical(cells, SCR_ROW_NUMBER(ses->winy));
}

static void
renderStatusField_cursorCoordinates (unsigned char *cells) {
  renderCoordinatesVertical(cells, SCR_COLUMN_NUMBER(scr.posx), SCR_ROW_NUMBER(scr.posy));
}

static void
renderStatusField_cursorColumn (unsigned char *cells) {
  renderNumberVertical(cells, SCR_COLUMN_NUMBER(scr.posx));
}

static void
renderStatusField_cursorRow (unsigned char *cells) {
  renderNumberVertical(cells, SCR_ROW_NUMBER(scr.posy));
}

static void
renderStatusField_cursorAndWindowColumn (unsigned char *cells) {
  renderNumberUpper(cells, SCR_COLUMN_NUMBER(scr.posx));
  renderNumberLower(cells, SCR_COLUMN_NUMBER(ses->winx));
}

static void
renderStatusField_cursorAndWindowRow (unsigned char *cells) {
  renderNumberUpper(cells, SCR_ROW_NUMBER(scr.posy));
  renderNumberLower(cells, SCR_ROW_NUMBER(ses->winy));
}

static void
renderStatusField_screenNumber (unsigned char *cells) {
  renderNumberVertical(cells, scr.number);
}

static void
renderStatusField_stateDots (unsigned char *cells) {
  *cells = (isFrozenScreen()    ? BRL_DOT1: 0) |
           (prefs.showCursor    ? BRL_DOT4: 0) |
           (ses->showAttributes   ? BRL_DOT2: 0) |
           (prefs.cursorStyle   ? BRL_DOT5: 0) |
           (prefs.alertTunes    ? BRL_DOT3: 0) |
           (prefs.blinkingCursor? BRL_DOT6: 0) |
           (ses->trackCursor      ? BRL_DOT7: 0) |
           (prefs.slidingWindow ? BRL_DOT8: 0);
}

static void
renderStatusField_stateLetter (unsigned char *cells) {
  *cells = convertCharacterToDots(textTable,
                                  ses->showAttributes? WC_C('a'):
                                  isFrozenScreen()? WC_C('f'):
                                  ses->trackCursor? WC_C('t'):
                                  WC_C(' '));
}

static void
renderStatusField_time (unsigned char *cells) {
  time_t now = time(NULL);
  struct tm *local = localtime(&now);
  renderNumberUpper(cells, local->tm_hour);
  renderNumberLower(cells, local->tm_min);
}

static void
renderStatusField_alphabeticWindowCoordinates (unsigned char *cells) {
  renderCoordinatesAlphabetic(cells, ses->winx, ses->winy);
}

static void
renderStatusField_alphabeticCursorCoordinates (unsigned char *cells) {
  renderCoordinatesAlphabetic(cells, scr.posx, scr.posy);
}

static void
renderStatusField_generic (unsigned char *cells) {
  cells[BRL_firstStatusCell] = BRL_STATUS_CELLS_GENERIC;
  cells[BRL_GSC_BRLCOL] = SCR_COLUMN_NUMBER(ses->winx);
  cells[BRL_GSC_BRLROW] = SCR_ROW_NUMBER(ses->winy);
  cells[BRL_GSC_CSRCOL] = SCR_COLUMN_NUMBER(scr.posx);
  cells[BRL_GSC_CSRROW] = SCR_ROW_NUMBER(scr.posy);
  cells[BRL_GSC_SCRNUM] = scr.number;
  cells[BRL_GSC_FREEZE] = isFrozenScreen();
  cells[BRL_GSC_DISPMD] = ses->showAttributes;
  cells[BRL_GSC_SIXDOTS] = prefs.textStyle;
  cells[BRL_GSC_SLIDEWIN] = prefs.slidingWindow;
  cells[BRL_GSC_SKPIDLNS] = prefs.skipIdenticalLines;
  cells[BRL_GSC_SKPBLNKWINS] = prefs.skipBlankWindows;
  cells[BRL_GSC_CSRVIS] = prefs.showCursor;
  cells[BRL_GSC_CSRHIDE] = ses->hideCursor;
  cells[BRL_GSC_CSRTRK] = ses->trackCursor;
  cells[BRL_GSC_CSRSIZE] = prefs.cursorStyle;
  cells[BRL_GSC_CSRBLINK] = prefs.blinkingCursor;
  cells[BRL_GSC_ATTRVIS] = prefs.showAttributes;
  cells[BRL_GSC_ATTRBLINK] = prefs.blinkingAttributes;
  cells[BRL_GSC_CAPBLINK] = prefs.blinkingCapitals;
  cells[BRL_GSC_TUNES] = prefs.alertTunes;
  cells[BRL_GSC_HELP] = isHelpScreen();
  cells[BRL_GSC_INFO] = infoMode;
  cells[BRL_GSC_AUTOREPEAT] = prefs.autorepeat;
  cells[BRL_GSC_AUTOSPEAK] = prefs.autospeak;
}

typedef struct {
  RenderStatusField render;
  unsigned char length;
} StatusFieldEntry;

static const StatusFieldEntry statusFieldTable[] = {
  {NULL, 0},
  {renderStatusField_windowCoordinates, 2},
  {renderStatusField_windowColumn, 1},
  {renderStatusField_windowRow, 1},
  {renderStatusField_cursorCoordinates, 2},
  {renderStatusField_cursorColumn, 1},
  {renderStatusField_cursorRow, 1},
  {renderStatusField_cursorAndWindowColumn, 2},
  {renderStatusField_cursorAndWindowRow, 2},
  {renderStatusField_screenNumber, 1},
  {renderStatusField_stateDots, 1},
  {renderStatusField_stateLetter, 1},
  {renderStatusField_time, 2},
  {renderStatusField_alphabeticWindowCoordinates, 1},
  {renderStatusField_alphabeticCursorCoordinates, 1},
  {renderStatusField_generic, BRL_genericStatusCellCount}
};
static const unsigned int statusFieldCount = ARRAY_COUNT(statusFieldTable);

unsigned int
getStatusFieldsLength (const unsigned char *fields) {
  unsigned int length = 0;
  while (*fields != sfEnd) length += statusFieldTable[*fields++].length;
  return length;
}

void
renderStatusFields (const unsigned char *fields, unsigned char *cells) {
  while (*fields != sfEnd) {
    StatusField field = *fields++;

    if (field < statusFieldCount) {
      const StatusFieldEntry *sf = &statusFieldTable[field];
      sf->render(cells);
      cells += sf->length;
    }
  }
}
