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

static unsigned int scrollRegionTop;
static unsigned int scrollRegionBottom;

static unsigned char hasColors = 0;
static unsigned char foregroundColor;
static unsigned char backgroundColor;

static unsigned int sharedSegmentSize = 0;
static int sharedSegmentIdentifier = 0;
static void *sharedSegmentAddress = NULL;

static int
releaseSharedSegment (void) {
  if (shmctl(sharedSegmentIdentifier, IPC_RMID, NULL) != -1) return 1;
  logSystemError("shmctl[IPC_RMID]");
  return 0;;
}

static int
allocateSharedSegment (const char *tty) {
  sharedSegmentSize = (sizeof(PtySharedSegmentCharacter) * COLS * LINES) + sizeof(PtySharedSegmentHeader);
  key_t key = ptyMakeSharedSegmentKey(tty);

  int found = ptyGetSharedSegmentIdentifier(key, &sharedSegmentIdentifier);
  if (found) releaseSharedSegment();

  {
    int flags = IPC_CREAT | S_IRUSR | S_IWUSR;
    found = (sharedSegmentIdentifier = shmget(key, sharedSegmentSize, flags)) != -1;
  }

  if (found) {
    sharedSegmentAddress = ptyAttachSharedSegment(sharedSegmentIdentifier);
    if (sharedSegmentAddress) return 1;
    releaseSharedSegment();
  } else {
    logSystemError("shmget");
  }

  return 0;
}

static void
setSharedSegmentCharacters (const PtySharedSegmentCharacter *character, PtySharedSegmentCharacter *from, const PtySharedSegmentCharacter *to) {
  while (from < to) *from++ = *character;
}

static void
initializeSharedSegmentHeader (void) {
  PtySharedSegmentHeader *header = sharedSegmentAddress;

  header->headerSize = sizeof(*header);
  header->segmentSize = sharedSegmentSize;

  header->characterSize = sizeof(PtySharedSegmentCharacter);
  header->charactersOffset = header->headerSize;

  header->screenHeight = LINES;
  header->screenWidth = COLS;

  header->cursorRow = ptyGetCursorRow();
  header->cursorColumn = ptyGetCursorColumn();

  {
    static const PtySharedSegmentCharacter initializer = {
      .text = ' ',
    };

    PtySharedSegmentCharacter *from = ptyGetSharedSegmentScreenStart(header);
    const PtySharedSegmentCharacter *to = ptyGetSharedSegmentScreenEnd(header);
    setSharedSegmentCharacters(&initializer, from, to);
  }
}

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

    hasColors = has_colors();
    foregroundColor = COLOR_WHITE;
    backgroundColor = COLOR_BLACK;

    if (hasColors) {
      start_color();
    }

    if (allocateSharedSegment(tty)) {
      initializeSharedSegmentHeader();
      return 1;
    }

    endwin();
  }

  return 0;
}

void
ptyEndScreen (void) {
  endwin();
  ptyDetachSharedSegment(sharedSegmentAddress);
  releaseSharedSegment();
}

void
ptyRefreshScreen (void) {
  refresh();
}

unsigned int
ptyGetCursorRow () {
  return getcury(stdscr);
}

unsigned int
ptyGetCursorColumn () {
  return getcurx(stdscr);
}

void
ptySetCursorPosition (unsigned int row, unsigned int column) {
  move(row, column);
}

void
ptySetCursorRow (unsigned int row) {
  ptySetCursorPosition(row, ptyGetCursorColumn());
}

void
ptySetCursorColumn (unsigned int column) {
  ptySetCursorPosition(ptyGetCursorRow(), column);
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
  unsigned int oldRow = ptyGetCursorRow();
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
  unsigned int oldRow = ptyGetCursorRow();
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
  ptySetCursorColumn(ptyGetCursorColumn()-amount);
}

void
ptyMoveCursorRight (unsigned int amount) {
  ptySetCursorColumn(ptyGetCursorColumn()+amount);
}

static PtySharedSegmentCharacter *
setSharedSegmentCharacter (unsigned int row, unsigned int column, PtySharedSegmentCharacter **end) {
  cchar_t wch;

  {
    unsigned int oldRow = ptyGetCursorRow();
    unsigned int oldColumn = ptyGetCursorColumn();
    int move = (row != oldRow) || (column != oldColumn);

    if (move) ptySetCursorPosition(row, column);
    in_wch(&wch);
    if (move) ptySetCursorPosition(oldRow, oldColumn);
  }

  PtySharedSegmentCharacter *character = ptyGetSharedSegmentCharacter(sharedSegmentAddress, row, column, end);
  character->text = wch.chars[0];
  character->blink = wch.attr & A_BLINK;
  character->bold = wch.attr & A_BOLD;
  character->underline = wch.attr & A_UNDERLINE;
  character->reverse = wch.attr & A_REVERSE;
  character->standout = wch.attr & A_STANDOUT;
  character->dim = wch.attr & A_DIM;

  return character;
}

static PtySharedSegmentCharacter *
setCurrentSharedSegmentCharacter (PtySharedSegmentCharacter **end) {
  return setSharedSegmentCharacter(ptyGetCursorRow(), ptyGetCursorColumn(), end);
}

void
ptyInsertLines (unsigned int count) {
  while (count-- > 0) insertln();
}

void
ptyDeleteLines (unsigned int count) {
  while (count-- > 0) deleteln();
}

void
ptyInsertCharacters (unsigned int count) {
  while (count-- > 0) insch(' ');

  PtySharedSegmentCharacter *to;
  PtySharedSegmentCharacter *from = setCurrentSharedSegmentCharacter(&to);

  if (from < to) {
    memmove(from+1, from, ((to - from - 1) * sizeof(*from)));
  }
}

void
ptyDeleteCharacters (unsigned int count) {
  while (count-- > 0) delch();
}

void
ptyAddCharacter (unsigned char character) {
  unsigned int row = ptyGetCursorRow();
  unsigned int column = ptyGetCursorColumn();

  addch(character);
  setSharedSegmentCharacter(row, column, NULL);
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

void
ptySetForegroundColor (unsigned char color) {
  foregroundColor = color;
  // FIXME
}

void
ptySetBackgroundColor (unsigned char color) {
  backgroundColor = color;
  // FIXME
}

void
ptyClearToEndOfScreen (void) {
  clrtobot();

  PtySharedSegmentCharacter *from = setCurrentSharedSegmentCharacter(NULL);
  const PtySharedSegmentCharacter *to = ptyGetSharedSegmentScreenEnd(sharedSegmentAddress);
  setSharedSegmentCharacters(from, from+1, to);
}

void
ptyClearToEndOfLine (void) {
  clrtoeol();

  PtySharedSegmentCharacter *to;
  PtySharedSegmentCharacter *from = setCurrentSharedSegmentCharacter(&to);
  setSharedSegmentCharacters(from, from+1, to);
}
