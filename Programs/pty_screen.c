/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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

#include "prologue.h"

#include "get_term.h"
#include "log.h"
#include "pty_screen.h"
#include "scr_emulator.h"
#include "msg_queue.h"
#include "utf8.h"

#define ENABLE_ROW_ARRAY 1

static unsigned char screenLogLevel = LOG_DEBUG;

void
ptySetScreenLogLevel (unsigned char level) {
  screenLogLevel = level;
}

static unsigned char hasColors = 0;
static unsigned char colorPairMap[1 << (4 + 4)];

static unsigned char currentForegroundColor;
static unsigned char currentBackgroundColor;

static unsigned char defaultForegroundColor;
static unsigned char defaultBackgroundColor;

static unsigned char colorBits;
static unsigned char colorCount;
static unsigned char colorMask;
static unsigned int pairCount;

static inline unsigned char
toColorPair (unsigned char foreground, unsigned char background) {
  return colorPairMap[
    ((background & colorMask) << colorBits) | (foreground & colorMask)
  ];
}

static void
initializeColors (unsigned char foreground, unsigned char background) {
  currentForegroundColor = defaultForegroundColor = foreground;
  currentBackgroundColor = defaultBackgroundColor = background;
}

static void
initializeColorPairMap (void) {
  for (unsigned int pair=0; pair<ARRAY_COUNT(colorPairMap); pair+=1) {
    colorPairMap[pair] = pair;
  }
}

static void
initializeColorPairs (void) {
  {
    short foreground, background;
    pair_content(0, &foreground, &background);
    initializeColors(foreground, background);

    unsigned char pair = toColorPair(foreground, background);
    colorPairMap[pair] = 0;
    colorPairMap[0] = pair;
  }

  for (unsigned char foreground=0; foreground<colorCount; foreground+=1) {
    for (unsigned char background=0; background<colorCount; background+=1) {
      unsigned char pair = toColorPair(foreground, background);
      if (!pair) continue;
      init_pair(pair, foreground, background);
    }
  }
}

static int haveTerminalMessageQueue = 0;
static int terminalMessageQueue;
static int haveInputTextHandler = 0;

static int
sendTerminalMessage (MessageType type, const void *content, size_t length) {
  if (!haveTerminalMessageQueue) return 0;
  return sendMessage(terminalMessageQueue, type, content, length, 0);
}

static int
startTerminalMessageReceiver (const char *name, MessageType type, size_t size, MessageHandler *handler, void *data) {
  if (!haveTerminalMessageQueue) return 0;
  return startMessageReceiver(name, terminalMessageQueue, type, size, handler, data);
}

static void
messageHandler_InputText (const MessageHandlerParameters *parameters) {
  PtyObject *pty = parameters->data;
  const char *content = parameters->content;
  size_t length = parameters->length;

  while (length) {
    wint_t character = convertUtf8ToWchar(&content, &length);
    if (character == WEOF) break;
    if (!ptyWriteInputCharacter(pty, character, 0)) break;
  }
}

static void
enableMessages (key_t key) {
  haveTerminalMessageQueue = createMessageQueue(&terminalMessageQueue, key);
}

static int segmentIdentifier = 0;
static ScreenSegmentHeader *segmentHeader = NULL;

static int
destroySegment (void) {
  if (haveTerminalMessageQueue) {
    destroyMessageQueue(terminalMessageQueue);
    haveTerminalMessageQueue = 0;
  }

  return destroyScreenSegment(segmentIdentifier);
}

static int
createSegment (const char *path, int driverDirectives) {
  key_t key;

  if (makeTerminalKey(&key, path)) {
    segmentHeader = createScreenSegment(&segmentIdentifier, key, LINES, COLS, ENABLE_ROW_ARRAY);

    if (segmentHeader) {
      if (driverDirectives) enableMessages(key);
      return 1;
    }
  }

  return 0;
}

static void
storeCursorPosition (void) {
  segmentHeader->cursorRow = getcury(stdscr);
  segmentHeader->cursorColumn = getcurx(stdscr);
}

static void
setColor (ScreenSegmentColor *ssc, unsigned char color, unsigned char bold, int dim) {
  int bright = !!(color & 0X8);
  unsigned char on  = bright? SCI_MAX: SCI_REG;
  unsigned char off = bright? SCI_DIM: SCI_OFF;

  if (bold) on = UINT8_MAX;
  if (dim) on >>= 1, off >>= 1;

  ssc->red = (color & COLOR_RED)? on: off;
  ssc->green = (color & COLOR_GREEN)? on: off;
  ssc->blue = (color & COLOR_BLUE)? on: off;

  if ((ssc->red == SCI_REG) && (ssc->green == SCI_REG) && (ssc->blue == SCI_OFF)) {
    ssc->green = SCI_DIM;
  }
}

static ScreenSegmentCharacter *
setCharacter (unsigned int row, unsigned int column, const ScreenSegmentCharacter **end) {
  wchar_t text;
  attr_t attributes;
  int colorPair;

  {
    unsigned int oldRow = segmentHeader->cursorRow;
    unsigned int oldColumn = segmentHeader->cursorColumn;
    int move = (row != oldRow) || (column != oldColumn);
    if (move) ptySetCursorPosition(row, column);

    {
    #ifdef GOT_CURSES_WCH
      cchar_t character;
      in_wch(&character);

      text = character.chars[0];
      attributes = character.attr;
      colorPair = character.ext_color;
    #else /* GOT_CURSES_WCH */
      chtype character = inch();
      text = character & A_CHARTEXT;
      attributes = character & A_ATTRIBUTES;
      colorPair = PAIR_NUMBER(character);
    #endif /* GOT_CURSES_WCH */
    }

    if (move) ptySetCursorPosition(oldRow, oldColumn);
  }

  ScreenSegmentCharacter character = {
    .text = text,
    .alpha = UINT8_MAX,
  };

  {
    short foreground, background;
    pair_content(colorPair, &foreground, &background);

    int bold = !!(attributes & A_BOLD);
    int dim = !!(attributes & A_DIM);

    {
      ScreenSegmentColor *cfg, *cbg;

      if (attributes & (A_REVERSE | A_STANDOUT | A_ITALIC)) {
        cfg = &character.background;
        cbg = &character.foreground;
      } else {
        cfg = &character.foreground;
        cbg = &character.background;
      }

      setColor(cfg, foreground, bold, dim);
      setColor(cbg, background, 0, dim);
    }
  }

  if (attributes & A_BLINK) character.blink = 1;
  if (attributes & A_UNDERLINE) character.underline = 1;

  {
    ScreenSegmentCharacter *location = getScreenCharacter(segmentHeader, row, column, end);
    *location = character;
    return location;
  }
}

static ScreenSegmentCharacter *
setCurrentCharacter (const ScreenSegmentCharacter **end) {
  return setCharacter(segmentHeader->cursorRow, segmentHeader->cursorColumn, end);
}

static ScreenSegmentCharacter *
getCurrentCharacter (const ScreenSegmentCharacter **end) {
  return getScreenCharacter(segmentHeader, segmentHeader->cursorRow, segmentHeader->cursorColumn, end);
}

static void
fillCharacters (unsigned int row, unsigned int column, unsigned int count) {
  ScreenSegmentCharacter *from = setCharacter(row, column, NULL);
  propagateScreenCharacter(from, (from + count));
}

static void
fillRows (unsigned int row, unsigned int count) {
  const ScreenSegmentCharacter *character = setCharacter(row, 0, NULL);
  fillScreenRows(segmentHeader, row, count, character);
}

static unsigned int scrollRegionTop;
static unsigned int scrollRegionBottom;

static unsigned int savedCursorRow = 0;
static unsigned int savedCursorColumn = 0;

int
ptyBeginScreen (PtyObject *pty, int driverDirectives) {
  haveTerminalMessageQueue = 0;
  haveInputTextHandler = 0;

  if (initscr()) {
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    raw();
    noecho();

    scrollok(stdscr, TRUE);
    idlok(stdscr, TRUE);
    idcok(stdscr, TRUE);

    scrollRegionTop = getbegy(stdscr);
    scrollRegionBottom = getmaxy(stdscr) - 1;

    savedCursorRow = 0;
    savedCursorColumn = 0;

    hasColors = has_colors();
    initializeColorPairMap();
    colorBits = 3;

    if (hasColors) {
      start_color();
      if (COLORS > 8) colorBits += 1;
    }

    colorCount = 1 << colorBits;
    colorMask = colorCount - 1;
    pairCount = colorCount << colorBits;

    if (hasColors) {
      initializeColorPairs();
    } else {
      initializeColors(COLOR_WHITE, COLOR_BLACK);
    }

    if (createSegment(ptyGetPath(pty), driverDirectives)) {
      segmentHeader->screenNumber = 1;
      storeCursorPosition();

      haveInputTextHandler = startTerminalMessageReceiver(
        "terminal-input-text-receiver", TERM_MSG_INPUT_TEXT,
        0X200, messageHandler_InputText, pty
      );

      return 1;
    }

    endwin();
  }

  return 0;
}

void
ptyEndScreen (void) {
  endwin();
  sendTerminalMessage(TERM_MSG_EMULATOR_EXITING, NULL, 0);
  detachScreenSegment(segmentHeader);
  destroySegment();
}

void
ptyResizeScreen (unsigned int height, unsigned int width) {
  if (is_term_resized(height, width)) {
    resize_term(height, width);
  }
}

void
ptyRefreshScreen (void) {
  sendTerminalMessage(TERM_MSG_SEGMENT_UPDATED, NULL, 0);
  refresh();
}

void
ptySetCursorPosition (unsigned int row, unsigned int column) {
  move(row, column);
  storeCursorPosition();
}

void
ptySetCursorRow (unsigned int row) {
  ptySetCursorPosition(row, segmentHeader->cursorColumn);
}

void
ptySetCursorColumn (unsigned int column) {
  ptySetCursorPosition(segmentHeader->cursorRow, column);
}

void
ptySaveCursorPosition (void) {
  savedCursorRow = segmentHeader->cursorRow;
  savedCursorColumn = segmentHeader->cursorColumn;
}

void
ptyRestoreCursorPosition (void) {
  ptySetCursorPosition(savedCursorRow, savedCursorColumn);
}

void
ptySetScrollRegion (unsigned int top, unsigned int bottom) {
  scrollRegionTop = top;
  scrollRegionBottom = bottom;
  setscrreg(top, bottom);
}

static int
isWithinScrollRegion (unsigned int row) {
  if (row < scrollRegionTop) return 0;
  if (row > scrollRegionBottom) return 0;
  return 1;
}

int
ptyAmWithinScrollRegion (void) {
  return isWithinScrollRegion(segmentHeader->cursorRow);
}

static void
scrollRows (unsigned int count, int down) {
  unsigned int top = scrollRegionTop;
  unsigned int bottom = scrollRegionBottom + 1;
  unsigned int size = bottom - top;
  if (count > size) count = size;
  unsigned int clear;

  if (down) {
    scrl(-count);
    clear = top;
  } else {
    scrl(count);
    clear = bottom - count;
  }

  scrollScreenRows(segmentHeader, top, size, count, down);
  fillRows(clear, count);
}

void
ptyScrollDown (unsigned int count) {
  scrollRows(count, true);
}

void
ptyScrollUp (unsigned int count) {
  scrollRows(count, false);
}

void
ptyMoveCursorUp (unsigned int amount) {
  unsigned int row = segmentHeader->cursorRow;
  if (amount > row) amount = row;
  if (amount > 0) ptySetCursorRow(row-amount);
}

void
ptyMoveCursorDown (unsigned int amount) {
  unsigned int oldRow = segmentHeader->cursorRow;
  unsigned int newRow = MIN(oldRow+amount, LINES-1);
  if (newRow != oldRow) ptySetCursorRow(newRow);
}

void
ptyMoveCursorLeft (unsigned int amount) {
  unsigned int column = segmentHeader->cursorColumn;
  if (amount > column) amount = column;
  if (amount > 0) ptySetCursorColumn(column-amount);
}

void
ptyMoveCursorRight (unsigned int amount) {
  unsigned int oldColumn = segmentHeader->cursorColumn;
  unsigned int newColumn = MIN(oldColumn+amount, COLS-1);
  if (newColumn != oldColumn) ptySetCursorColumn(newColumn);
}

void
ptyMoveUp1 (void) {
  if (segmentHeader->cursorRow == scrollRegionTop) {
    ptyScrollDown(1);
  } else {
    ptyMoveCursorUp(1);
  }
}

void
ptyMoveDown1 (void) {
  if (segmentHeader->cursorRow == scrollRegionBottom) {
    ptyScrollUp(1);
  } else {
    ptyMoveCursorDown(1);
  }
}

void
ptyTabBackward (void) {
  ptySetCursorColumn(((segmentHeader->cursorColumn - 1) / TABSIZE) * TABSIZE);
}

void
ptyTabForward (void) {
  ptySetCursorColumn(((segmentHeader->cursorColumn / TABSIZE) + 1) * TABSIZE);
}

void
ptyInsertLines (unsigned int count) {
  if (ptyAmWithinScrollRegion()) {
    unsigned int row = segmentHeader->cursorRow;
    unsigned int oldTop = scrollRegionTop;
    unsigned int oldBottom = scrollRegionBottom;

    ptySetScrollRegion(row, scrollRegionBottom);
    ptyScrollDown(count);
    ptySetScrollRegion(oldTop, oldBottom);
  }
}

void
ptyDeleteLines (unsigned int count) {
  if (ptyAmWithinScrollRegion()) {
    unsigned int row = segmentHeader->cursorRow;
    unsigned int oldTop = scrollRegionTop;
    unsigned int oldBottom = scrollRegionBottom;

    ptySetScrollRegion(row, scrollRegionBottom);
    ptyScrollUp(count);
    ptySetScrollRegion(oldTop, oldBottom);
  }
}

void
ptyInsertCharacters (unsigned int count) {
  const ScreenSegmentCharacter *end;
  ScreenSegmentCharacter *from = getCurrentCharacter(&end);

  if ((from + count) > end) count = end - from;
  ScreenSegmentCharacter *to = from + count;
  moveScreenCharacters(to, from, (end - to));

  {
    unsigned int counter = count;
    while (counter-- > 0) insch(' ');
  }

  fillCharacters(segmentHeader->cursorRow, segmentHeader->cursorColumn, count);
}

void
ptyDeleteCharacters (unsigned int count) {
  const ScreenSegmentCharacter *end;
  ScreenSegmentCharacter *to = getCurrentCharacter(&end);

  if ((to + count) > end) count = end - to;
  ScreenSegmentCharacter *from = to + count;
  if (from < end) moveScreenCharacters(to, from, (end - from));

  {
    unsigned int counter = count;
    while (counter-- > 0) delch();
  }

  fillCharacters(segmentHeader->cursorRow, (COLS - count), count);
}

void
ptyAddCharacter (unsigned char character) {
  unsigned int row = segmentHeader->cursorRow;
  unsigned int column = segmentHeader->cursorColumn;

  addch(character);
  storeCursorPosition();

  setCharacter(row, column, NULL);
}

void
ptySetCursorVisibility (unsigned int visibility) {
  curs_set(visibility);
}

void
ptySetAttributes (attr_t attributes) {
  attrset(attributes);
}

void
ptyAddAttributes (attr_t attributes) {
  attron(attributes);
}

void
ptyRemoveAttributes (attr_t attributes) {
  attroff(attributes);
}

static void
setCharacterColors (void) {
  attroff(A_COLOR);
  attron(COLOR_PAIR(toColorPair(currentForegroundColor, currentBackgroundColor)));
}

void
ptySetForegroundColor (int color) {
  if (color == -1) color = defaultForegroundColor;
  currentForegroundColor = color;
  setCharacterColors();
}

void
ptySetBackgroundColor (int color) {
  if (color == -1) color = defaultBackgroundColor;
  currentBackgroundColor = color;
  setCharacterColors();
}

void
ptyClearToEndOfLine (void) {
  clrtoeol();

  const ScreenSegmentCharacter *to;
  ScreenSegmentCharacter *from = setCurrentCharacter(&to);
  propagateScreenCharacter(from, to);
}

void
ptyClearToBeginningOfLine (void) {
  unsigned int column = segmentHeader->cursorColumn;
  if (column > 0) ptySetCursorColumn(0);

  while (1) {
    ptyAddCharacter(' ');
    if (segmentHeader->cursorColumn > column) break;
  }

  ptySetCursorColumn(column);
}

void
ptyClearToEndOfDisplay (void) {
  clrtobot();

  if (haveScreenRowArray(segmentHeader)) {
    ptyClearToEndOfLine();

    unsigned int bottomRows = segmentHeader->screenHeight - segmentHeader->cursorRow - 1;
    if (bottomRows > 0) fillRows((segmentHeader->cursorRow + 1), bottomRows);
  } else {
    ScreenSegmentCharacter *from = setCurrentCharacter(NULL);
    const ScreenSegmentCharacter *to;
    getScreenCharacterArray(segmentHeader, &to);
    propagateScreenCharacter(from, to);
  }
}
