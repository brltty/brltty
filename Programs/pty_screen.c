/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
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

#include <wchar.h>
#include <stdlib.h>
#include <string.h>

#include "get_term.h"
#include "log.h"
#include "pty_screen.h"
#include "pty_clipboard.h"
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
initializeCharacterColors (unsigned char foreground, unsigned char background) {
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
    initializeCharacterColors(foreground, background);

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

/* BRLTTY clipboard -> host clipboard bridging, emulator side. The emulator runs
 * in the GUI session, so it owns the system clipboard; the driver owns BRLTTY's
 * clipboard and reports a change here whenever a braille copy updates it. */
static void
messageHandler_clipboardToHost (const MessageHandlerParameters *parameters) {
  ptyPublishClipboard(parameters->content, parameters->length);
}

static void
enableMessages (key_t key) {
  haveTerminalMessageQueue = createMessageQueue(&terminalMessageQueue, key);
}

static int segmentIdentifier = 0;
static ScreenSegmentHeader *segmentHeader = NULL;
static key_t segmentKey = 0;
static int haveSegmentKey = 0;

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
  if (makeTerminalKey(&segmentKey, path)) {
    haveSegmentKey = 1;
    segmentHeader = createScreenSegment(&segmentIdentifier, segmentKey, LINES, COLS, ENABLE_ROW_ARRAY);

    if (segmentHeader) {
      if (driverDirectives) enableMessages(segmentKey);
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

/* When a glyph is written into the last column the cursor lingers there with
 * this flag set, rather than wrapping immediately. The wrap is performed only
 * when the next glyph arrives. This is the VT "magic margin" / xenl behavior
 * that screen and tmux (which this emulator impersonates) advertise, and that
 * full-screen programs such as Claude Code rely on when drawing boxes. */
static int pendingWrap = 0;

/* Move the curses cursor and mirror the position into the segment, without
 * disturbing the pending-wrap state (used for internal repositioning). */
static void
moveCursor (unsigned int row, unsigned int column) {
  move(row, column);
  storeCursorPosition();
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
    if (move) moveCursor(row, column);

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

    if (move) moveCursor(oldRow, oldColumn);
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

  /* Make curses size the screen from the real terminal rather than from the
   * inherited LINES/COLUMNS environment variables. Those variables are often
   * stale (commonly left at 80x24) and, when honored, would cap the emulated
   * screen regardless of the actual terminal size. use_tioctl(TRUE) asks
   * curses to obtain the size from the operating system (TIOCGWINSZ).
   * use_tioctl is an ncurses extension (6.0+), so guard it. */
#ifdef NCURSES_VERSION
  use_env(FALSE);
  use_tioctl(TRUE);
#endif /* NCURSES_VERSION */

  if (initscr()) {
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    raw();
    noecho();
    nonl();

    scrollok(stdscr, TRUE);
    idlok(stdscr, TRUE);
    idcok(stdscr, TRUE);

    scrollRegionTop = getbegy(stdscr);
    scrollRegionBottom = getmaxy(stdscr) - 1;
    pendingWrap = 0;

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
      initializeCharacterColors(COLOR_WHITE, COLOR_BLACK);
    }

    if (createSegment(ptyGetPath(pty), driverDirectives)) {
      segmentHeader->screenNumber = 1;
      storeCursorPosition();

      haveInputTextHandler = startTerminalMessageReceiver(
        "terminal-input-text-receiver", TERM_MSG_INPUT_TEXT,
        0X200, messageHandler_InputText, pty
      );

      startTerminalMessageReceiver(
        "terminal-clipboard-receiver", TERM_MSG_CLIPBOARD_TO_HOST,
        TERM_CLIPBOARD_MESSAGE_SIZE, messageHandler_clipboardToHost, NULL
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
  if ((height == 0) || (width == 0)) return;
  if ((height == segmentHeader->screenHeight) &&
      (width == segmentHeader->screenWidth)) {
    return;
  }

  /* Resize the curses screen we render through. */
  if (is_term_resized(height, width)) {
    resize_term(height, width);
  }

  /* A System V shared-memory segment cannot be resized in place, so publish a
   * fresh one at the new size under the same key. The reader notices the new
   * segment (its identifier changes) and re-attaches; the old mapping stays
   * valid for anyone still reading it until they detach. */
  if (haveSegmentKey) {
    ScreenSegmentHeader *old = segmentHeader;

    /* createScreenSegment removes the prior segment under this key (it calls
     * getScreenSegment + destroyScreenSegment) before allocating the new one. */
    ScreenSegmentHeader *fresh = createScreenSegment(
      &segmentIdentifier, segmentKey, height, width, ENABLE_ROW_ARRAY
    );

    if (!fresh) {
      /* Allocation failed: keep the existing (smaller) segment intact rather
       * than writing the new, larger geometry into it. */
      logMessage(LOG_WARNING, "screen segment resize failed; keeping old size");
      return;
    }

    segmentHeader = fresh;
    segmentHeader->screenNumber = 1;
    if (old) detachScreenSegment(old);
  }

  /* A resize resets the scroll region to the whole screen and cancels any
   * deferred wrap; clamp the saved cursor to the new bounds. */
  scrollRegionTop = 0;
  scrollRegionBottom = height - 1;
  setscrreg(scrollRegionTop, scrollRegionBottom);
  pendingWrap = 0;

  if (savedCursorRow >= height) savedCursorRow = height - 1;
  if (savedCursorColumn >= width) savedCursorColumn = width - 1;

  /* Mirror the (resized) curses screen into the fresh segment so the new
   * dimensions are immediately populated; the child repaints on its own
   * SIGWINCH soon after. */
  storeCursorPosition();

  for (unsigned int row=0; row<height; row+=1) {
    for (unsigned int column=0; column<width; column+=1) {
      setCharacter(row, column, NULL);
    }
  }

  storeCursorPosition();
  ptyRefreshScreen();
}

void
ptyRefreshScreen (void) {
  sendTerminalMessage(TERM_MSG_SEGMENT_UPDATED, NULL, 0);
  refresh();
}

void
ptySetCursorPosition (unsigned int row, unsigned int column) {
  /* Any explicit cursor movement cancels a deferred wrap. */
  pendingWrap = 0;
  moveCursor(row, column);
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

/* Place a glyph at an explicit position without letting curses scroll at the
 * bottom-right corner; the caller sets the final cursor position afterwards. */
static void
writeGlyph (unsigned int row, unsigned int column, wchar_t character) {
  scrollok(stdscr, FALSE);
  move(row, column);

#ifdef GOT_CURSES_WCH
  {
    attr_t attributes;
    short colorPair;
    attr_get(&attributes, &colorPair, NULL);

    wchar_t string[] = {character, 0};
    cchar_t wch;

    if (setcchar(&wch, string, attributes, colorPair, NULL) == OK) {
      add_wch(&wch);
    } else {
      addch(character);
    }
  }
#else /* GOT_CURSES_WCH */
  addch(character);
#endif /* GOT_CURSES_WCH */

  scrollok(stdscr, TRUE);

  /* Resync the segment cursor with the position curses advanced to, so that
   * setCharacter() reads back the cells we just wrote rather than the next. */
  storeCursorPosition();
}

static void
wrapToNextLine (void) {
  ptySetCursorColumn(0);
  ptyMoveDown1();
}

void
ptyAddCharacter (wchar_t character) {
  int width = wcwidth(character);
  if (width < 1) width = 1;

  /* Resolve a wrap deferred from the previous glyph before placing this one. */
  if (pendingWrap) {
    pendingWrap = 0;
    wrapToNextLine();
  }

  /* A double-width glyph that no longer fits on the line wraps to the next. */
  if ((segmentHeader->cursorColumn + width) > segmentHeader->screenWidth) {
    wrapToNextLine();
  }

  unsigned int row = segmentHeader->cursorRow;
  unsigned int column = segmentHeader->cursorColumn;

  writeGlyph(row, column, character);

  /* Mirror every cell the glyph occupied (two for a double-width character). */
  for (unsigned int c=column; (c<(column+width)) && (c<segmentHeader->screenWidth); c+=1) {
    setCharacter(row, c, NULL);
  }

  unsigned int newColumn = column + width;
  if (newColumn < segmentHeader->screenWidth) {
    moveCursor(row, newColumn);
  } else {
    /* The glyph reached the right margin: keep the cursor on the last column
     * and defer the wrap until the next glyph (xenl behavior). */
    moveCursor(row, segmentHeader->screenWidth - 1);
    pendingWrap = 1;
  }
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
setColorAttribute (void) {
  attroff(A_COLOR);
  attron(COLOR_PAIR(toColorPair(currentForegroundColor, currentBackgroundColor)));
}

void
ptySetForegroundColor (int color) {
  if (color == -1) color = defaultForegroundColor;
  currentForegroundColor = color;
  setColorAttribute();
}

void
ptySetBackgroundColor (int color) {
  if (color == -1) color = defaultBackgroundColor;
  currentBackgroundColor = color;
  setColorAttribute();
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

void
ptyClearToBeginningOfDisplay (void) {
  unsigned int row = segmentHeader->cursorRow;

  /* Clear every row above the cursor in full. */
  if (row > 0) {
    unsigned int savedRow = segmentHeader->cursorRow;
    unsigned int savedColumn = segmentHeader->cursorColumn;

    for (unsigned int r=0; r<row; r+=1) {
      moveCursor(r, 0);
      clrtoeol();
    }

    fillRows(0, row);
    moveCursor(savedRow, savedColumn);
  }

  /* Clear the cursor's own row from its start up to the cursor. */
  ptyClearToBeginningOfLine();
}

/* Clear the cursor's entire row, leaving the cursor where it is. */
void
ptyClearLine (void) {
  unsigned int column = segmentHeader->cursorColumn;
  ptySetCursorColumn(0);
  ptyClearToEndOfLine();
  ptySetCursorColumn(column);
}

/* Clear the whole screen, leaving the cursor where it is (CSI 2J). */
void
ptyClearDisplay (void) {
  unsigned int row = segmentHeader->cursorRow;
  unsigned int column = segmentHeader->cursorColumn;
  ptySetCursorPosition(0, 0);
  ptyClearToEndOfDisplay();
  ptySetCursorPosition(row, column);
}

/* Erase count characters at the cursor (overwrite with blanks) without moving
 * the cursor or shifting the rest of the line (CSI X / ECH). */
void
ptyEraseCharacters (unsigned int count) {
  unsigned int row = segmentHeader->cursorRow;
  unsigned int column = segmentHeader->cursorColumn;

  unsigned int available = segmentHeader->screenWidth - column;
  if (count > available) count = available;

  for (unsigned int i=0; i<count; i+=1) {
    writeGlyph(row, (column + i), ' ');
    setCharacter(row, (column + i), NULL);
  }

  pendingWrap = 0;
  moveCursor(row, column);
}

void
ptyGetCursorPosition (unsigned int *row, unsigned int *column) {
  if (row) *row = segmentHeader->cursorRow;
  if (column) *column = segmentHeader->cursorColumn;
}
