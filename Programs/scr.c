/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* scr.cc - The screen reading library
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#define SCR_C 1

#include "prologue.h"

#include <string.h>

#include "misc.h"
#include "system.h"
#include "scr.h"
#include "scr_frozen.h"
#include "scr_help.h"
#include "scr_real.h"

static BaseScreen *currentScreen;
static HelpScreen helpScreen;
static FrozenScreen frozenScreen;                
static MainScreen mainScreen;
static const ScreenDriver *screenDriver = NULL;
static void *screenObject;

void
initializeAllScreens (const char *identifier, const char *driverDirectory) {
  initializeHelpScreen(&helpScreen);
  initializeFrozenScreen(&frozenScreen);

  {
    if (!(screenDriver = loadScreenDriver(identifier, &screenObject, driverDirectory))) {
      screenDriver = &noScreen;
      screenObject = NULL;
    }

    identifyScreenDriver(screenDriver, 0);
    screenDriver->initialize(&mainScreen);
  }
}

void
closeAllScreens (void) {
  frozenScreen.close();
  helpScreen.close();
  mainScreen.close();

  if (screenDriver) {
    screenDriver = NULL;

    if (screenObject) {
      unloadSharedObject(screenObject);
      screenObject = NULL;
    }
  }
}


typedef enum {
  SCR_HELP   = 0X01,
  SCR_FROZEN = 0X02
} ActiveScreen;
static ActiveScreen activeScreens = 0;

static void
selectScreen (void) {
  typedef struct {
    ActiveScreen which;
    BaseScreen *screen;
  } ScreenEntry;
  static const ScreenEntry screenEntries[] = {
    {SCR_HELP  , &helpScreen.base},
    {SCR_FROZEN, &frozenScreen.base},
    {0         , &mainScreen.base}
  };
  const ScreenEntry *entry = screenEntries;
  while (entry->which) {
    if (entry->which & activeScreens) break;
    ++entry;
  }
  currentScreen = entry->screen;
}

static void
activateScreen (ActiveScreen which) {
  activeScreens |= which;
  selectScreen();
}

static void
deactivateScreen (ActiveScreen which) {
  activeScreens &= ~which;
  selectScreen();
}

int
isLiveScreen (void) {
  return currentScreen == &mainScreen.base;
}


int
validateScreenBox (const ScreenBox *box, int columns, int rows) {
  if ((box->left >= 0))
    if ((box->width > 0))
      if (((box->left + box->width) <= columns))
        if ((box->top >= 0))
          if ((box->height > 0))
            if (((box->top + box->height) <= rows))
              return 1;
  return 0;
}

void
setScreenMessage (const ScreenBox *box, unsigned char *buffer, ScreenMode mode, const char *message) {
  int count = box->width * box->height;

  if (mode == SCR_TEXT) {
    int length = strlen(message) - box->left;
    if (length < 0) length = 0;
    if (length > box->width) length = box->width;

    memset(buffer, ' ', count);
    if (length) memcpy(buffer, message+box->left, length);
  } else {
    memset(buffer, 0X07, count);
  }
}

void
describeScreen (ScreenDescription *description) {
  description->cols = description->rows = 1;
  description->posx = description->posy = 0;
  description->number = 0;
  description->cursor = 1;
  description->unreadable = NULL;
  currentScreen->describe(description);

  if (description->unreadable) {
    description->cursor = 0;
  } else if (description->number == -1) {
    description->unreadable = "unreadable screen";
  }
}

int
readScreen (short left, short top, short width, short height, unsigned char *buffer, ScreenMode mode) {
  ScreenBox box;
  box.left = left;
  box.top = top;
  box.width = width;
  box.height = height;
  return currentScreen->read(box, buffer, mode);
}

int
insertKey (ScreenKey key) {
  return currentScreen->insertKey(key);
}

int
insertCharacters (const char *characters, int count) {
  while (count-- > 0)
    if (!insertKey((unsigned char)*characters++))
      return 0;
  return 1;
}

int
insertString (const char *string) {
  return insertCharacters(string, strlen(string));
}

int
routeCursor (int column, int row, int screen) {
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
selectVirtualTerminal (int vt) {
  return currentScreen->selectVirtualTerminal(vt);
}

int
switchVirtualTerminal (int vt) {
  return currentScreen->switchVirtualTerminal(vt);
}

int
currentVirtualTerminal (void) {
  return currentScreen->currentVirtualTerminal();
}

int
userVirtualTerminal (int number) {
  return mainScreen.userVirtualTerminal(number);
}

int
executeScreenCommand (int command) {
  return currentScreen->executeCommand(command);
}


const char *const *
getScreenParameters (void) {
  return screenDriver->parameters;
}

const char *
getScreenDriverCode (void) {
  return screenDriver->definition.code;
}

int
openMainScreen (char **parameters) {
  if (mainScreen.processParameters(parameters)) {
    if (mainScreen.open()) {
      currentScreen = &mainScreen.base;
      return 1;
      mainScreen.close();
    }
  }
  return 0;
}


int
openRoutingScreen (void) {
  /* This function should be used in a forked process. Though we want to
   * have a separate file descriptor for the main screen from the one used
   * in the main thread.  So we close and reopen the device.
   */
  mainScreen.close();
  return mainScreen.open();
}

void
closeRoutingScreen (void) {
  mainScreen.close();
}


static int helpOpened = 0;

int
isHelpScreen (void) {
  return currentScreen == &helpScreen.base;
}

int
haveHelpScreen (void) {
  return (activeScreens & SCR_HELP) != 0;
}

int
activateHelpScreen (void) {
  if (!helpOpened) return 0;
  activateScreen(SCR_HELP);
  return 1;
}

void
deactivateHelpScreen (void) {
  deactivateScreen(SCR_HELP);
}

int
openHelpScreen (const char *file) {
  return helpOpened = helpScreen.open(file);
}

void
closeHelpScreen (void) {
  if (helpOpened) {
    helpScreen.close();
    helpOpened = 0;
  }
}

void
setHelpPageNumber (short page) {
  helpScreen.setPageNumber(page);
}

short
getHelpPageNumber (void) {
  return helpScreen.getPageNumber();
}

short
getHelpPageCount (void) {
  return helpScreen.getPageCount();
}


int
isFrozenScreen (void) {
  return currentScreen == &frozenScreen.base;
}

int
haveFrozenScreen (void) {
  return (activeScreens & SCR_FROZEN) != 0;
}

int
activateFrozenScreen (void) {
  if (haveFrozenScreen() || !frozenScreen.open(&mainScreen.base)) return 0;
  activateScreen(SCR_FROZEN);
  return 1;
}

void
deactivateFrozenScreen (void) {
  if (haveFrozenScreen()) {
    frozenScreen.close();
    deactivateScreen(SCR_FROZEN);
  }
}
