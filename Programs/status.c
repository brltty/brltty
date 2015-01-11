/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#include "status.h"
#include "timing.h"
#include "update.h"
#include "brl_dots.h"
#include "brl_utils.h"
#include "scr.h"
#include "brltty.h"
#include "prefs.h"
#include "ttb.h"

static void
renderDigitUpper (unsigned char *cell, int digit) {
  *cell |= portraitDigits[digit];
}

static void
renderDigitLower (unsigned char *cell, int digit) {
  *cell |= toLowerDigit(portraitDigits[digit]);
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
  if (!SCR_COORDINATES_OK(column, row)) {
    *cell = convertCharacterToDots(textTable, WC_C('z'));
  } else {
    const int32_t height = 25;
    const int32_t frequency = row / height;

    if (frequency) {
      const int32_t interval = NSECS_PER_SEC / (frequency * 2);
      TimeValue time;

      getMonotonicTime(&time);
      scheduleUpdateIn("alva status field",
                       (((interval - (time.nanoseconds % interval)) / NSECS_PER_MSEC) + 1));

      if (!((time.nanoseconds / interval) % 2)) {
        *cell = 0;
        return;
      }
    }

    *cell = convertCharacterToDots(textTable, ((row % height) + WC_C('a')))
          | ((column / textCount) << 6);
  }
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
  *cells = (isFrozenScreen()           ? BRL_DOT_1: 0) |
           (prefs.showSystemCursor     ? BRL_DOT_4: 0) |
           (ses->displayMode           ? BRL_DOT_2: 0) |
           (prefs.systemCursorStyle    ? BRL_DOT_5: 0) |
           (prefs.alertTunes           ? BRL_DOT_3: 0) |
           (prefs.blinkingSystemCursor ? BRL_DOT_6: 0) |
           (ses->trackSystemCursor     ? BRL_DOT_7: 0) |
           (prefs.slidingBrailleWindow ? BRL_DOT_8: 0);
}

static void
renderStatusField_stateLetter (unsigned char *cells) {
  *cells = convertCharacterToDots(textTable,
                                  ses->displayMode      ? WC_C('a'):
                                  isHelpScreen()        ? WC_C('h'):
                                  isMenuScreen()        ? WC_C('m'):
                                  isFrozenScreen()      ? WC_C('f'):
                                  ses->trackSystemCursor? WC_C('t'):
                                                          WC_C(' '));
}

static void
renderStatusField_time (unsigned char *cells) {
  TimeValue value;
  TimeComponents components;

  getCurrentTime(&value);
  scheduleUpdateIn("time status field", millisecondsTillNextMinute(&value));

  expandTimeValue(&value, &components);
  renderNumberUpper(cells, components.hour);
  renderNumberLower(cells, components.minute);
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
  cells[GSC_FIRST] = GSC_MARKER;
  cells[gscWindowColumn] = SCR_COLUMN_NUMBER(ses->winx);
  cells[gscWindowRow] = SCR_ROW_NUMBER(ses->winy);
  cells[gscCursorColumn] = SCR_COLUMN_NUMBER(scr.posx);
  cells[gscCursorRow] = SCR_ROW_NUMBER(scr.posy);
  cells[gscScreenNumber] = scr.number;
  cells[gscFrozenScreen] = isFrozenScreen();
  cells[gscDisplayMode] = ses->displayMode;
  cells[gscTextStyle] = prefs.textStyle;
  cells[gscSlidingWindow] = prefs.slidingBrailleWindow;
  cells[gscSkipIdenticalLines] = prefs.skipIdenticalLines;
  cells[gscSkipBlankWindows] = prefs.skipBlankBrailleWindows;
  cells[gscShowCursor] = prefs.showSystemCursor;
  cells[gscHideCursor] = ses->hideSystemCursor;
  cells[gscTrackCursor] = ses->trackSystemCursor;
  cells[gscCursorStyle] = prefs.systemCursorStyle;
  cells[gscBlinkingCursor] = prefs.blinkingSystemCursor;
  cells[gscShowAttributes] = prefs.showAttributes;
  cells[gscBlinkingAttributes] = prefs.blinkingAttributes;
  cells[gscBlinkingCapitals] = prefs.blinkingCapitals;
  cells[gscAlertTunes] = prefs.alertTunes;
  cells[gscAutorepeat] = prefs.autorepeat;
  cells[gscAutospeak] = prefs.autospeak;
  cells[gscBrailleInputMode] = prefs.brailleInputMode;
}

typedef struct {
  RenderStatusField render;
  unsigned char length;
} StatusFieldEntry;

static const StatusFieldEntry statusFieldTable[] = {
  [sfEnd] = {
    .render = NULL,
    .length = 0
  }
  ,
  [sfWindowCoordinates] = {
    .render = renderStatusField_windowCoordinates,
    .length = 2
  }
  ,
  [sfWindowColumn] = {
    .render = renderStatusField_windowColumn,
    .length = 1
  }
  ,
  [sfWindowRow] = {
    .render = renderStatusField_windowRow,
    .length = 1
  }
  ,
  [sfCursorCoordinates] = {
    .render = renderStatusField_cursorCoordinates,
    .length = 2
  }
  ,
  [sfCursorColumn] = {
    .render = renderStatusField_cursorColumn,
    .length = 1
  }
  ,
  [sfCursorRow] = {
    .render = renderStatusField_cursorRow,
    .length = 1
  }
  ,
  [sfCursorAndWindowColumn] = {
    .render = renderStatusField_cursorAndWindowColumn,
    .length = 2
  }
  ,
  [sfCursorAndWindowRow] = {
    .render = renderStatusField_cursorAndWindowRow,
    .length = 2
  }
  ,
  [sfScreenNumber] = {
    .render = renderStatusField_screenNumber,
    .length = 1
  }
  ,
  [sfStateDots] = {
    .render = renderStatusField_stateDots,
    .length = 1
  }
  ,
  [sfStateLetter] = {
    .render = renderStatusField_stateLetter,
    .length = 1
  }
  ,
  [sfTime] = {
    .render = renderStatusField_time,
    .length = 2
  }
  ,
  [sfAlphabeticWindowCoordinates] = {
    .render = renderStatusField_alphabeticWindowCoordinates,
    .length = 1
  }
  ,
  [sfAlphabeticCursorCoordinates] = {
    .render = renderStatusField_alphabeticCursorCoordinates,
    .length = 1
  }
  ,
  [sfGeneric] = {
    .render = renderStatusField_generic,
    .length = GSC_COUNT
  }
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
