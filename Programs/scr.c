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
#include "update.h"
#include "message.h"
#include "menu_prefs.h"
#include "scr.h"
#include "scr_help.h"
#include "scr_menu.h"
#include "scr_frozen.h"
#include "scr_real.h"
#include "driver.h"
#include "cmd_queue.h"

static HelpScreen helpScreen;
static MenuScreen menuScreen;
static FrozenScreen frozenScreen;                
static MainScreen mainScreen;

typedef enum {
  SCR_HELP   = 0X01,
  SCR_MENU   = 0X02,
  SCR_FROZEN = 0X04
} ActiveScreen;

static ActiveScreen activeScreens = 0;
static BaseScreen *currentScreen = &mainScreen.base;

static void
announceScreen (void) {
  char buffer[0X80];
  size_t length = formatScreenTitle(buffer, sizeof(buffer));

  if (length) message(NULL, buffer, 0);
}

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
  scheduleUpdate("new screen selected");
  announceScreen();
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
isMainScreen (void) {
  return currentScreen == &mainScreen.base;
}

const char *const *
getScreenParameters (const ScreenDriver *driver) {
  return driver->parameters;
}

const DriverDefinition *
getScreenDriverDefinition (const ScreenDriver *driver) {
  return &driver->definition;
}

void
initializeScreen (void) {
  screen->initialize(&mainScreen);
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


size_t
formatScreenTitle (char *buffer, size_t size) {
  return currentScreen->formatTitle(buffer, size);
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

static int
handleScreenCommands (int command, void *data) {
  return currentScreen->handleCommand(command);
}

int
addScreenCommands (void) {
  return pushCommandHandler("screen", KTB_CTX_DEFAULT,
                            handleScreenCommands, NULL, NULL);
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
  mainScreen.releaseParameters();
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
