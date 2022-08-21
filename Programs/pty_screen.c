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

#include "prologue.h"

#include <string.h>
#include <sys/stat.h>

#include "log.h"
#include "pty_screen.h"
#include "pty_shared.h"

static unsigned char screenLogLevel = LOG_DEBUG;

void
ptySetScreenLogLevel (unsigned char level) {
  screenLogLevel = level;
}

static unsigned char hasColors = 0;
static unsigned char currentForegroundColor;
static unsigned char currentBackgroundColor;
static unsigned char defaultForegroundColor;
static unsigned char defaultBackgroundColor;
static unsigned char colorPairMap[0100];

static unsigned char
toColorPair (unsigned char foreground, unsigned char background) {
  return colorPairMap[(background << 3) | foreground];
}

static void
initializeColors (unsigned char foreground, unsigned char background) {
  currentForegroundColor = defaultForegroundColor = foreground;
  currentBackgroundColor = defaultBackgroundColor = background;
}

static void
initializeColorPairs (void) {
  for (unsigned int pair=0; pair<ARRAY_COUNT(colorPairMap); pair+=1) {
    colorPairMap[pair] = pair;
  }

  {
    short foreground, background;
    pair_content(0, &foreground, &background);
    initializeColors(foreground, background);

    unsigned char pair = toColorPair(foreground, background);
    colorPairMap[pair] = 0;
    colorPairMap[0] = pair;
  }

  for (unsigned char foreground=COLOR_BLACK; foreground<=COLOR_WHITE; foreground+=1) {
    for (unsigned char background=COLOR_BLACK; background<=COLOR_WHITE; background+=1) {
      unsigned char pair = toColorPair(foreground, background);
      if (!pair) continue;
      init_pair(pair, foreground, background);
    }
  }
}

static unsigned int segmentSize = 0;
static int segmentIdentifier = 0;
static PtyHeader *segmentHeader = NULL;

void
ptyLogSegment (const char *label) {
  logBytes(screenLogLevel, "pty segment: %s", segmentHeader, segmentSize, label);
}

static int
releaseSegment (void) {
  if (shmctl(segmentIdentifier, IPC_RMID, NULL) != -1) return 1;
  logSystemError("shmctl[IPC_RMID]");
  return 0;;
}

static int
allocateSegment (const char *tty) {
  segmentSize = (sizeof(PtyCharacter) * COLS * LINES) + sizeof(PtyHeader);
  key_t key = ptyMakeSegmentKey(tty);

  int found = ptyGetSegmentIdentifier(key, &segmentIdentifier);
  if (found) releaseSegment();

  {
    int flags = IPC_CREAT | S_IRUSR | S_IWUSR;
    found = (segmentIdentifier = shmget(key, segmentSize, flags)) != -1;
  }

  if (found) {
    segmentHeader = ptyAttachSegment(segmentIdentifier);
    if (segmentHeader) return 1;
    releaseSegment();
  } else {
    logSystemError("shmget");
  }

  return 0;
}

static void
storeCursorPosition (void) {
  segmentHeader->cursorRow = getcury(stdscr);
  segmentHeader->cursorColumn = getcurx(stdscr);
}

static PtyCharacter *
getCurrentCharacter (PtyCharacter **end) {
  return ptyGetCharacter(segmentHeader, segmentHeader->cursorRow, segmentHeader->cursorColumn, end);
}

static PtyCharacter *
moveCharacters (PtyCharacter *to, const PtyCharacter *from, unsigned int count) {
  if (count) memmove(to, from, (count * sizeof(*from)));
  return to;
}

static void
setCharacters (PtyCharacter *from, const PtyCharacter *to, const PtyCharacter *character) {
  while (from < to) *from++ = *character;
}

static void
propagateCharacter (PtyCharacter *from, const PtyCharacter *to) {
  setCharacters(from+1, to, from);
}

static void
initializeCharacters (PtyCharacter *from, const PtyCharacter *to) {
  const PtyCharacter initializer = {
    .text = ' ',
    .foreground = defaultForegroundColor,
    .background = defaultBackgroundColor,
  };

  setCharacters(from, to, &initializer);
}

static void
initializeHeader (void) {
  segmentHeader->headerSize = sizeof(*segmentHeader);
  segmentHeader->segmentSize = segmentSize;

  segmentHeader->characterSize = sizeof(PtyCharacter);
  segmentHeader->charactersOffset = segmentHeader->headerSize;

  segmentHeader->screenHeight = LINES;
  segmentHeader->screenWidth = COLS;
  storeCursorPosition();;

  {
    PtyCharacter *from = ptyGetScreenStart(segmentHeader);
    const PtyCharacter *to = ptyGetScreenEnd(segmentHeader);
    initializeCharacters(from, to);
  }
}

static unsigned int scrollRegionTop;
static unsigned int scrollRegionBottom;

static unsigned int savedCursorRow = 0;
static unsigned int savedCursorColumn = 0;

int
ptyBeginScreen (const char *tty) {
  if (initscr()) {
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    raw();
    noecho();
    scrollok(stdscr, TRUE);

    scrollRegionTop = getbegy(stdscr);
    scrollRegionBottom = getmaxy(stdscr) - 1;

    savedCursorRow = 0;
    savedCursorColumn = 0;

    hasColors = has_colors();
    initializeColors(COLOR_WHITE, COLOR_BLACK);

    if (hasColors) {
      start_color();
      initializeColorPairs();
    }

    if (allocateSegment(tty)) {
      initializeHeader();
      return 1;
    }

    endwin();
  }

  return 0;
}

void
ptyEndScreen (void) {
  endwin();
  ptyDetachSegment(segmentHeader);
  releaseSegment();
}

void
ptyRefreshScreen (void) {
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
ptySetScrollRegion (unsigned int top, unsigned int bottom) {
  scrollRegionTop = top;
  scrollRegionBottom = bottom;
  setscrreg(top, bottom);
}

static void
scrollLines (int amount) {
  scrl(amount);
}

static int
isWithinScrollRegion (unsigned int row) {
  if (row < scrollRegionTop) return 0;
  if (row > scrollRegionBottom) return 0;
  return 1;
}

void
ptyMoveCursorUp (unsigned int amount) {
  unsigned int oldRow = segmentHeader->cursorRow;
  unsigned int newRow = oldRow - amount;

  if (isWithinScrollRegion(oldRow)) {
    int delta = newRow - scrollRegionTop;

    if (delta < 0) {
      scrollLines(delta);
      newRow = scrollRegionTop;
    }
  }

  if (newRow != oldRow) ptySetCursorRow(newRow);
}

void
ptyMoveCursorDown (unsigned int amount) {
  unsigned int oldRow = segmentHeader->cursorRow;
  unsigned int newRow = oldRow + amount;

  if (isWithinScrollRegion(oldRow)) {
    int delta = newRow - scrollRegionBottom;

    if (delta > 0) {
      scrollLines(delta);
      newRow = scrollRegionBottom;
    }
  }

  if (newRow != oldRow) ptySetCursorRow(newRow);
}

void
ptyMoveCursorLeft (unsigned int amount) {
  ptySetCursorColumn(segmentHeader->cursorColumn-amount);
}

void
ptyMoveCursorRight (unsigned int amount) {
  ptySetCursorColumn(segmentHeader->cursorColumn+amount);
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

static PtyCharacter *
setCharacter (unsigned int row, unsigned int column, PtyCharacter **end) {
  cchar_t wch;

  {
    unsigned int oldRow = segmentHeader->cursorRow;
    unsigned int oldColumn = segmentHeader->cursorColumn;
    int move = (row != oldRow) || (column != oldColumn);

    if (move) ptySetCursorPosition(row, column);
    in_wch(&wch);
    if (move) ptySetCursorPosition(oldRow, oldColumn);
  }

  PtyCharacter *character = ptyGetCharacter(segmentHeader, row, column, end);
  character->text = wch.chars[0];

  character->blink = wch.attr & A_BLINK;
  character->bold = wch.attr & A_BOLD;
  character->underline = wch.attr & A_UNDERLINE;
  character->reverse = wch.attr & A_REVERSE;
  character->standout = wch.attr & A_STANDOUT;
  character->dim = wch.attr & A_DIM;

  short foreground, background;
  pair_content(wch.ext_color, &foreground, &background);
  character->foreground = foreground;
  character->background = background;

  return character;
}

static PtyCharacter *
setCurrentCharacter (PtyCharacter **end) {
  return setCharacter(segmentHeader->cursorRow, segmentHeader->cursorColumn, end);
}

void
ptyInsertLines (unsigned int count) {
  {
    unsigned int counter = count;
    while (counter-- > 0) insertln();
  }
}

void
ptyDeleteLines (unsigned int count) {
  {
    unsigned int counter = count;
    while (counter-- > 0) deleteln();
  }
}

void
ptyInsertCharacters (unsigned int count) {
  PtyCharacter *end;
  PtyCharacter *from = getCurrentCharacter(&end);

  PtyCharacter *to = from + count;
  if (to > end) to = end;
  moveCharacters(to, from, (end - to));

  {
    unsigned int counter = count;
    while (counter-- > 0) insch(' ');
  }

  setCurrentCharacter(NULL);
  propagateCharacter(from, to);
}

void
ptyDeleteCharacters (unsigned int count) {
  {
    unsigned int counter = count;
    while (counter-- > 0) delch();
  }
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
setColor (void) {
  attroff(A_COLOR);
  attron(COLOR_PAIR(toColorPair(currentForegroundColor, currentBackgroundColor)));
}

void
ptySetForegroundColor (int color) {
  if (color == -1) color = defaultForegroundColor;
  currentForegroundColor = color;
  setColor();
}

void
ptySetBackgroundColor (int color) {
  if (color == -1) color = defaultBackgroundColor;
  currentBackgroundColor = color;
  setColor();
}

void
ptyClearToEndOfScreen (void) {
  clrtobot();

  PtyCharacter *from = setCurrentCharacter(NULL);
  const PtyCharacter *to = ptyGetScreenEnd(segmentHeader);
  propagateCharacter(from, to);
}

void
ptyClearToEndOfLine (void) {
  clrtoeol();

  PtyCharacter *to;
  PtyCharacter *from = setCurrentCharacter(&to);
  propagateCharacter(from, to);
}
