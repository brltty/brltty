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

#include "log.h"
#include "pty_screen.h"

static int scrollRegionTop;
static int scrollRegionBottom;

static unsigned char hasColors = 0;
static unsigned char foregroundColor;
static unsigned char backgroundColor;

void
ptyBeginScreen (void) {
  initscr();
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
}

void
ptyEndScreen (void) {
  endwin();
}

void
ptyRefreshScreen (void) {
  refresh();
}

void
ptyInsertLines (int count) {
  while (count-- > 0) insertln();
}

void
ptyDeleteLines (int count) {
  while (count-- > 0) deleteln();
}

void
ptyInsertCharacters (int count) {
  while (count-- > 0) insch(' ');
}

void
ptyDeleteCharacters (int count) {
  while (count-- > 0) delch();
}

void
ptyAddCharacter (unsigned char character) {
  addch(character);
}

void
ptySetCursorVisibility (int visibility) {
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
}

void
ptyClearToEndOfLine (void) {
  clrtoeol();
}

int
ptyGetCursorRow () {
  return getcury(stdscr);
}

int
ptyGetCursorColumn () {
  return getcurx(stdscr);
}

void
ptySetCursorPosition (int row, int column) {
  move(row, column);
}

void
ptySetCursorRow (int row) {
  ptySetCursorPosition(row, ptyGetCursorColumn());
}

void
ptySetCursorColumn (int column) {
  ptySetCursorPosition(ptyGetCursorRow(), column);
}

void
ptySetScrollRegion (int top, int bottom) {
  scrollRegionTop = top;
  scrollRegionBottom = bottom;
  setscrreg(top, bottom);
}

static void
scrollLines (int amount) {
  scrl(amount);
}

static int
isWithinScrollRegion (int row) {
  if (row < scrollRegionTop) return 0;
  if (row > scrollRegionBottom) return 0;
  return 1;
}

void
ptyMoveCursorUp (int amount) {
  int oldRow = ptyGetCursorRow();
  int newRow = oldRow - amount;

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
ptyMoveCursorDown (int amount) {
  int oldRow = ptyGetCursorRow();
  int newRow = oldRow + amount;

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
ptyMoveCursorLeft (int amount) {
  ptySetCursorColumn(ptyGetCursorColumn()-amount);
}

void
ptyMoveCursorRight (int amount) {
  ptySetCursorColumn(ptyGetCursorColumn()+amount);
}
