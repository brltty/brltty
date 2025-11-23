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

/* scr.cc - The screen reading library
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "unicode.h"
#include "scr.h"
#include "scr_utils.h"
#include "scr_real.h"
#include "driver.h"
#include "color.h"

MainScreen mainScreen;
BaseScreen *currentScreen = NULL;

int
isMainScreen (void) {
  return currentScreen == &mainScreen.base;
}

const char *const *
getScreenParameters (const ScreenDriver *driver) {
  return driver->parameters;
}

static void
initializeScreen (void) {
  screen->initialize(&mainScreen);
  currentScreen = &mainScreen.base;
  currentScreen->onForeground();
}

void
setNoScreen (void) {
  screen = &noScreen;
  initializeScreen();
}

int
constructScreenDriver (char **parameters) {
  initializeScreen();

  if (mainScreen.processParameters(parameters)) {
    if (mainScreen.construct()) {
      return 1;
    } else {
      logMessage(LOG_DEBUG, "screen driver initialization failed: %s",
                 screen->definition.code);
    }

    mainScreen.releaseParameters();
  }

  return 0;
}

void
destructScreenDriver (void) {
  mainScreen.destruct();
  mainScreen.releaseParameters();
}


int
pollScreen (void) {
  return currentScreen->poll();
}

int
refreshScreen (void) {
  return currentScreen->refresh();
}

void
describeScreen (ScreenDescription *description) {
  describeScreenObject(description, currentScreen);
  if (description->unreadable) description->quality = SCQ_NONE;
}

int
readScreen (short left, short top, short width, short height, ScreenCharacter *buffer) {
  const ScreenBox box = {
    .left = left,
    .top = top,
    .width = width,
    .height = height,
  };

  unsigned int count = box.width * box.height;
  clearScreenCharacters(buffer, count);
  if (!currentScreen->readCharacters(&box, buffer)) return 0;

  ScreenCharacter *character = buffer;
  const ScreenCharacter *end = character + count;

  while (character < end) {
    uint32_t text = character->text;

    if (!text || (text > UNICODE_LAST_CHARACTER)) {
      // This is not a valid Unicode character - return the replacement character.

      size_t index = character - buffer;
      unsigned int column = box.left + (index % box.width);
      unsigned int row = box.top + (index / box.width);

      logMessage(LOG_ERR,
        "invalid character U+%04"PRIX32 " on screen at [%u,%u]",
        text, column, row
      );

      character->text = UNICODE_REPLACEMENT_CHARACTER;
    }

    character += 1;
  }

  return 1;
}

int
readScreenText (short left, short top, short width, short height, wchar_t *buffer) {
  unsigned int count = width * height;
  ScreenCharacter characters[count];
  if (!readScreen(left, top, width, height, characters)) return 0;

  for (int i=0; i<count; i+=1) {
    buffer[i] = characters[i].text;
  }

  return 1;
}

unsigned char
getScreenColorAttributes (const ScreenColor *color) {
  ScreenColor c = *color;
  toVGAScreenColor(&c);
  return c.vgaAttributes;
}

int
insertScreenKey (ScreenKey key) {
  logMessage(LOG_CATEGORY(SCREEN_DRIVER), "insert key: 0X%04X", key);
  return currentScreen->insertKey(key);
}

ScreenPasteMode
getScreenPasteMode (void) {
  return currentScreen->getPasteMode();
}

int
routeScreenCursor (int column, int row, int screen) {
  return currentScreen->routeCursor(column, row, screen);
}

int
highlightScreenRegion (int left, int right, int top, int bottom) {
  return currentScreen->highlightRegion(left, right, top, bottom);
}

int
unhighlightScreenRegion (void) {
  return currentScreen->unhighlightRegion();
}

int
getScreenPointer (int *column, int *row) {
  return currentScreen->getPointer(column, row);
}

int
clearScreenTextSelection (void) {
  return currentScreen->clearSelection();
}

int
setScreenTextSelection (int startColumn, int startRow, int endColumn, int endRow) {
  if ((endRow < startRow) || ((endRow == startRow) && (endColumn < startColumn))) {
    int temp;

    temp = endColumn;
    endColumn = startColumn;
    startColumn = temp;

    temp = endRow;
    endRow = startRow;
    startRow = temp;
  }

  return currentScreen->setSelection(startColumn, startRow, endColumn, endRow);
}

int
currentVirtualTerminal (void) {
  return currentScreen->currentVirtualTerminal();
}

int
selectScreenVirtualTerminal (int vt) {
  return currentScreen->selectVirtualTerminal(vt);
}

int
switchScreenVirtualTerminal (int vt) {
  return currentScreen->switchVirtualTerminal(vt);
}

int
nextScreenVirtualTerminal (void) {
  return currentScreen->nextVirtualTerminal();
}

int
previousScreenVirtualTerminal (void) {
  return currentScreen->previousVirtualTerminal();
}

int
userVirtualTerminal (int number) {
  return mainScreen.userVirtualTerminal(number);
}

int
handleScreenCommands (int command, void *data) {
  return currentScreen->handleCommand(command);
}

KeyTableCommandContext
getScreenCommandContext (void) {
  return currentScreen->getCommandContext();
}
