/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

/* scr.cc - The screen reading library
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#define SCR_C 1

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "message.h"
#include "system.h"
#include "drivers.h"
#include "menu_prefs.h"
#include "scr.h"
#include "scr_help.h"
#include "scr_menu.h"
#include "scr_frozen.h"
#include "scr_real.h"
#include "scr.auto.h"

static HelpScreen helpScreen;
static MenuScreen menuScreen;
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
      logMessage(LOG_DEBUG, "screen driver initialization failed: %s",
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
  initializeMenuScreen(&menuScreen);
  initializeFrozenScreen(&frozenScreen);
}

void
destructSpecialScreens (void) {
  helpScreen.destruct();
  menuScreen.destruct();
  frozenScreen.destruct();
}


typedef enum {
  SCR_HELP   = 0X01,
  SCR_MENU   = 0X02,
  SCR_FROZEN = 0X04
} ActiveScreen;
static ActiveScreen activeScreens = 0;

static void
setScreen (ActiveScreen which) {
  typedef struct {
    ActiveScreen which;
    BaseScreen *screen;
  } ScreenEntry;

  static const ScreenEntry screenEntries[] = {
    {SCR_HELP  , &helpScreen.base},
    {SCR_MENU  , &menuScreen.base},
    {SCR_FROZEN, &frozenScreen.base},
    {0         , &mainScreen.base}
  };
  const ScreenEntry *entry = screenEntries;

  while (entry->which) {
    if (which & entry->which) break;
    entry += 1;
  }
  currentScreen = entry->screen;

  {
    char buffer[0X80];
    size_t length = formatScreenTitle(buffer, sizeof(buffer));
    if (length) message(NULL, buffer, 0);
  }
}

static void
selectScreen (void) {
  setScreen(activeScreens);
}

int
haveScreen (ActiveScreen which) {
  return (activeScreens & which) != 0;
}

static void
activateScreen (ActiveScreen which) {
  activeScreens |= which;
  setScreen(which);
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

  logMessage(LOG_ERR, "invalid screen area: cols=%d left=%d width=%d rows=%d top=%d height=%d",
             columns, box->left, box->width,
             rows, box->top, box->height);
  return 0;
}

void
setScreenCharacterText (ScreenCharacter *characters, wchar_t text, size_t count) {
  while (count > 0) {
    characters[--count].text = text;
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
  setScreenCharacterText(characters, WC_C(' '), count);
  setScreenCharacterAttributes(characters, SCR_COLOUR_DEFAULT, count);
}

void
setScreenMessage (const ScreenBox *box, ScreenCharacter *buffer, const char *message) {
  const ScreenCharacter *end = buffer + box->width;
  unsigned int index = 0;
  size_t length = strlen(message);
  mbstate_t state;

  memset(&state, 0, sizeof(state));
  clearScreenCharacters(buffer, (box->width * box->height));

  while (length) {
    wchar_t wc;
    size_t result = mbrtowc(&wc, message, length, &state);
    if ((ssize_t)result < 1) break;

    message += result;
    length -= result;

    if (index++ >= box->left) {
      if (buffer == end) break;
      (buffer++)->text = wc;
    }
  }
}

size_t
formatScreenTitle (char *buffer, size_t size) {
  return currentScreen->formatTitle(buffer, size);
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
selectScreenVirtualTerminal (int vt) {
  return currentScreen->selectVirtualTerminal(vt);
}

int
switchScreenVirtualTerminal (int vt) {
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
executeScreenCommand (int *command) {
  return currentScreen->executeCommand(command);
}

KeyTableCommandContext
getScreenCommandContext (void) {
  return currentScreen->getCommandContext();
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


static int helpScreenConstructed = 0;

int
isHelpScreen (void) {
  return currentScreen == &helpScreen.base;
}

int
haveHelpScreen (void) {
  return haveScreen(SCR_HELP);
}

int
activateHelpScreen (void) {
  if (!constructHelpScreen()) return 0;
  activateScreen(SCR_HELP);
  return 1;
}

void
deactivateHelpScreen (void) {
  deactivateScreen(SCR_HELP);
}

int
constructHelpScreen (void) {
  if (!helpScreenConstructed) {
    if (!helpScreen.construct()) return 0;
    helpScreenConstructed = 1;
  }

  return 1;
}

void
destructHelpScreen (void) {
  if (helpScreenConstructed) {
    helpScreen.destruct();
    helpScreenConstructed = 0;
  }
}

int
addHelpPage (void) {
  return helpScreen.addPage();
}

unsigned int
getHelpPageCount (void) {
  return helpScreen.getPageCount();
}

unsigned int
getHelpPageNumber (void) {
  return helpScreen.getPageNumber();
}

int
setHelpPageNumber (unsigned int number) {
  return helpScreen.setPageNumber(number);
}

int
clearHelpPage (void) {
  return helpScreen.clearPage();
}

int
addHelpLine (const wchar_t *characters) {
  return helpScreen.addLine(characters);
}

unsigned int
getHelpLineCount (void) {
  return helpScreen.getLineCount();
}


static int menuScreenConstructed = 0;

int
isMenuScreen (void) {
  return currentScreen == &menuScreen.base;
}

int
haveMenuScreen (void) {
  return haveScreen(SCR_MENU);
}

int
activateMenuScreen (void) {
  if (!menuScreenConstructed) {
    Menu *menu = getPreferencesMenu();

    if (!menu) return 0;
    if (!menuScreen.construct(menu)) return 0;
    menuScreenConstructed = 1;
  }

  activateScreen(SCR_MENU);
  return 1;
}

void
deactivateMenuScreen (void) {
  deactivateScreen(SCR_MENU);
}


int
isFrozenScreen (void) {
  return currentScreen == &frozenScreen.base;
}

int
haveFrozenScreen (void) {
  return haveScreen(SCR_FROZEN);
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
