/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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

#include "log.h"
#include "scr_special.h"
#include "update.h"
#include "message.h"

#include "scr_frozen.h"
static FrozenScreen frozenScreen;

#include "scr_menu.h"
#include "menu_prefs.h"
static MenuScreen menuScreen;

#include "scr_help.h"
static HelpScreen helpScreen;

typedef enum {
  SCR_FROZEN,
  SCR_MENU,
  SCR_HELP,
} SpecialScreenType;

typedef struct {
  const char *name;
  BaseScreen *base;
  void (*destruct) (void);
  unsigned autoDestruct:1; \
  unsigned isActive:1;
} SpecialScreenEntry;

#define SPECIAL_SCREEN_INITIALIZER(type) \
  .name = #type, \
  .base = &type ## Screen.base

static SpecialScreenEntry specialScreenTable[] = {
  [SCR_FROZEN] = {
    SPECIAL_SCREEN_INITIALIZER(frozen),
    .autoDestruct = 1
  },

  [SCR_MENU] = {
    SPECIAL_SCREEN_INITIALIZER(menu)
  },

  [SCR_HELP] = {
    SPECIAL_SCREEN_INITIALIZER(help)
  },
};

static const unsigned char specialScreenCount = ARRAY_COUNT(specialScreenTable);

static SpecialScreenEntry *
getSpecialScreenEntry (SpecialScreenType type) {
  return &specialScreenTable[type];
}

static void
destructSpecialScreen (SpecialScreenEntry *sse) {
  if (sse->destruct) {
    logMessage(LOG_DEBUG, "destruct special screen: %s", sse->name);
    sse->destruct();
    sse->destruct = NULL;
  }
}

static void
announceCurrentScreen (void) {
  char buffer[0X80];
  size_t length = currentScreen->formatTitle(buffer, sizeof(buffer));

  if (length) message(NULL, buffer, 0);
}

static void
setCurrentScreen (BaseScreen *screen) {
  currentScreen = screen;
  scheduleUpdate("new screen selected");
  announceCurrentScreen();
}

static void
setSpecialScreen (const SpecialScreenEntry *sse) {
  setCurrentScreen(sse->base);
}

static void
selectCurrentScreen (void) {
  const SpecialScreenEntry *sse = specialScreenTable;
  const SpecialScreenEntry *end = sse + specialScreenCount;

  while (sse < end) {
    if (sse->isActive) break;
    sse += 1;
  }

  if (sse == end) {
    setCurrentScreen(&mainScreen.base);
  } else {
    setSpecialScreen(sse);
  }
}

static int
haveSpecialScreen (SpecialScreenType type) {
  return getSpecialScreenEntry(type)->isActive;
}

static void
activateSpecialScreen (SpecialScreenType type) {
  SpecialScreenEntry *sse = getSpecialScreenEntry(type);

  sse->isActive = 1;
  setSpecialScreen(sse);
}

static void
deactivateSpecialScreen (SpecialScreenType type) {
  SpecialScreenEntry *sse = getSpecialScreenEntry(type);

  sse->isActive = 0;
  if (sse->autoDestruct) destructSpecialScreen(sse);
  selectCurrentScreen();
}

int
isFrozenScreen (void) {
  return currentScreen == &frozenScreen.base;
}

int
haveFrozenScreen (void) {
  return haveSpecialScreen(SCR_FROZEN);
}

int
activateFrozenScreen (void) {
  SpecialScreenEntry *sse = getSpecialScreenEntry(SCR_FROZEN);

  if (sse->destruct) return 0;
  if (!frozenScreen.construct(&mainScreen.base)) return 0;
  sse->destruct = frozenScreen.destruct;

  activateSpecialScreen(SCR_FROZEN);
  return 1;
}

void
deactivateFrozenScreen (void) {
  deactivateSpecialScreen(SCR_FROZEN);
}

int
isMenuScreen (void) {
  return currentScreen == &menuScreen.base;
}

int
haveMenuScreen (void) {
  return haveSpecialScreen(SCR_MENU);
}

int
activateMenuScreen (void) {
  SpecialScreenEntry *sse = getSpecialScreenEntry(SCR_MENU);

  if (!sse->destruct) {
    Menu *menu = getPreferencesMenu();

    if (!menu) return 0;
    if (!menuScreen.construct(menu)) return 0;
    sse->destruct = menuScreen.destruct;
  }

  activateSpecialScreen(SCR_MENU);
  return 1;
}

void
deactivateMenuScreen (void) {
  deactivateSpecialScreen(SCR_MENU);
}

int
isHelpScreen (void) {
  return currentScreen == &helpScreen.base;
}

int
haveHelpScreen (void) {
  return haveSpecialScreen(SCR_HELP);
}

int
activateHelpScreen (void) {
  if (!constructHelpScreen()) return 0;
  activateSpecialScreen(SCR_HELP);
  return 1;
}

void
deactivateHelpScreen (void) {
  deactivateSpecialScreen(SCR_HELP);
}

int
constructHelpScreen (void) {
  SpecialScreenEntry *sse = getSpecialScreenEntry(SCR_HELP);

  if (!sse->destruct) {
    if (!helpScreen.construct()) return 0;
    sse->destruct = helpScreen.destruct;
  }

  return 1;
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

void
beginSpecialScreens (void) {
  initializeFrozenScreen(&frozenScreen);
  initializeMenuScreen(&menuScreen);
  initializeHelpScreen(&helpScreen);
}

void
endSpecialScreens (void) {
  SpecialScreenEntry *sse = specialScreenTable;
  SpecialScreenEntry *end = sse + specialScreenCount;

  while (sse < end) {
    destructSpecialScreen(sse);
    sse += 1;
  }
}
