/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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
#include "drivers.h"
#include "scr.h"
#include "scr_frozen.h"
#include "scr_help.h"
#include "scr_real.h"
#include "scr.auto.h"

static HelpScreen helpScreen;
static FrozenScreen frozenScreen;                
static MainScreen mainScreen;
static BaseScreen *currentScreen = &mainScreen.base;

#define SCRSYMBOL noScreen
#define DRIVER_NAME NoScreen
#define DRIVER_CODE no
#define DRIVER_COMMENT "no screen support"
#define DRIVER_VERSION ""
#define DRIVER_DEVELOPERS ""
#include "scr_driver.h"

static void
scr_initialize (MainScreen *main) {
  initializeMainScreen(main);
}

const ScreenDriver *screen = &noScreen;

const char *const *
getScreenParameters (const ScreenDriver *driver) {
  return driver->parameters;
}

const DriverDefinition *
getScreenDriverDefinition (const ScreenDriver *driver) {
  return &driver->definition;
}

int
haveScreenDriver (const char *code) {
  return haveDriver(code, SCREEN_DRIVER_CODES, driverTable);
}

const char *
getDefaultScreenDriver (void) {
  return getDefaultDriver(driverTable);
}

const ScreenDriver *
loadScreenDriver (const char *code, void **driverObject, const char *driverDirectory) {
  return loadDriver(code, driverObject,
                    driverDirectory, driverTable,
                    "screen", 'x', "scr",
                    &noScreen, &noScreen.definition);
}

void
initializeScreen (void) {
  screen->initialize(&mainScreen);
}

int
constructScreenDriver (char **parameters) {
  initializeScreen();
  if (mainScreen.processParameters(parameters)) {
    if (mainScreen.construct()) {
      return 1;
    } else {
      LogPrint(LOG_DEBUG, "screen driver initialization failed: %s",
               screen->definition.code);
    }
  }

  return 0;
}

void
destructScreenDriver (void) {
  mainScreen.destruct();
}

void
identifyScreenDriver (const ScreenDriver *driver, int full) {
  identifyDriver("Screen", &driver->definition, full);
}

void
identifyScreenDrivers (int full) {
  const DriverEntry *entry = driverTable;
  while (entry->address) {
    const ScreenDriver *driver = entry++->address;
    identifyScreenDriver(driver, full);
  }
}

void
constructSpecialScreens (void) {
  initializeHelpScreen(&helpScreen);
  initializeFrozenScreen(&frozenScreen);
}

void
destructSpecialScreens (void) {
  frozenScreen.destruct();
  helpScreen.destruct();
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

  LogPrint(LOG_ERR, "invalid screen area: cols=%d left=%d width=%d rows=%d top=%d height=%d",
           columns, box->left, box->width,
           rows, box->top, box->height);
  return 0;
}

void
setScreenCharacterText (ScreenCharacter *characters, char text, size_t count) {
  while (count > 0) {
    characters[--count].text = text;
  }
}

void
copyScreenCharacterText (ScreenCharacter *characters, const char *text, size_t count) {
  while (count > 0) {
    --count;
    characters[count].text = text[count];
  }
}

void
setScreenCharacterAttributes (ScreenCharacter *characters, unsigned char attributes, size_t count) {
  while (count > 0) {
    characters[--count].attributes = attributes;
  }
}

void
clearScreenCharacters (ScreenCharacter *characters, size_t count) {
  setScreenCharacterText(characters, ' ', count);
  setScreenCharacterAttributes(characters, 0X07, count);
}

void
setScreenMessage (const ScreenBox *box, ScreenCharacter *buffer, const char *message) {
  int count = box->width * box->height;

  {
    size_t length = strlen(message) - box->left;
    if (length < 0) length = 0;
    if (length > box->width) length = box->width;

    clearScreenCharacters(buffer, count);
    if (length) copyScreenCharacterText(buffer, &message[box->left], length);
  }
}

void
describeScreen (ScreenDescription *description) {
  describeBaseScreen(currentScreen, description);
}

int
readScreen (short left, short top, short width, short height, ScreenCharacter *buffer) {
  ScreenBox box;
  box.left = left;
  box.top = top;
  box.width = width;
  box.height = height;
  return currentScreen->readCharacters(&box, buffer);
}

int
readScreenText (short left, short top, short width, short height, wchar_t *buffer) {
  unsigned int count = width * height;
  ScreenCharacter characters[count];
  if (!readScreen(left, top, width, height, characters)) return 0;

  {
    int i;
    for (i=0; i<count; ++i) buffer[i] = characters[i].text;
  }

  return 1;
}

int
insertScreenKey (ScreenKey key) {
  return currentScreen->insertKey(key);
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


int
constructRoutingScreen (void) {
  /* This function should be used in a forked process. Though we want to
   * have a separate file descriptor for the main screen from the one used
   * in the main thread.  So we close and reopen the device.
   */
  mainScreen.destruct();
  return mainScreen.construct();
}

void
destructRoutingScreen (void) {
  mainScreen.destruct();
}


static int helpConstructed = 0;

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
  if (!helpConstructed) return 0;
  activateScreen(SCR_HELP);
  return 1;
}

void
deactivateHelpScreen (void) {
  deactivateScreen(SCR_HELP);
}

int
constructHelpScreen (const char *file) {
  return helpConstructed = helpScreen.construct(file);
}

void
destructHelpScreen (void) {
  if (helpConstructed) {
    helpScreen.destruct();
    helpConstructed = 0;
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
  if (haveFrozenScreen() || !frozenScreen.construct(&mainScreen.base)) return 0;
  activateScreen(SCR_FROZEN);
  return 1;
}

void
deactivateFrozenScreen (void) {
  if (haveFrozenScreen()) {
    frozenScreen.destruct();
    deactivateScreen(SCR_FROZEN);
  }
}
