/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>

#include "drivers.h"
#include "scr.h"
#include "scr_main.h"
#include "scr.auto.h"

typedef enum {
  PARM_MESSAGE
} ScreenParameters;
#define SCRPARMS "message"

#define SCRSYMBOL noScreen
#define DRIVER_NAME NoScreen
#define DRIVER_CODE no
#define DRIVER_COMMENT "no screen support"
#define DRIVER_VERSION ""
#define DRIVER_DEVELOPERS ""
#include "scr_driver.h"

static const char defaultScreenMessage[] = strtext("no screen");
static const char *screenMessage = defaultScreenMessage;

static int
processParameters_NoScreen (char **parameters) {
  {
    const char *message = parameters[PARM_MESSAGE];

    screenMessage = (message && *message)? message: gettext(defaultScreenMessage);
  }

  return 1;
}

static int
currentVirtualTerminal_NoScreen (void) {
  return SCR_NO_VT;
}

static void
describe_NoScreen (ScreenDescription *description) {
  description->rows = 1;
  description->cols = strlen(screenMessage);
  description->posx = 0;
  description->posy = 0;
  description->number = currentVirtualTerminal_NoScreen();
}

static int
readCharacters_NoScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  ScreenDescription description;
  describe_NoScreen(&description);
  if (!validateScreenBox(box, description.cols, description.rows)) return 0;
  setScreenMessage(box, buffer, screenMessage);
  return 1;
}

static int
poll_NoScreen (void) {
  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeMainScreen(main);

  main->base.poll = poll_NoScreen;
  main->base.describe = describe_NoScreen;
  main->base.readCharacters = readCharacters_NoScreen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_NoScreen;

  main->processParameters = processParameters_NoScreen;
}

const ScreenDriver *screen = &noScreen;

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
