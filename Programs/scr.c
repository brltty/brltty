/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>

#include "scr.h"
#include "scr_frozen.h"
#include "scr_help.h"
#include "scr_main.h"

HelpScreen helpScreen;
FrozenScreen frozenScreen;                
MainScreen liveScreen;
BaseScreen *currentScreen;

void
initializeAllScreens (void) {
  initializeHelpScreen(&helpScreen);
  initializeLiveScreen(&liveScreen);
  initializeFrozenScreen(&frozenScreen);
}

void
closeAllScreens (void) {
  frozenScreen.close();
  liveScreen.close();
  helpScreen.close();
}


static int helpOpened;
int
selectDisplay (int disp) {
  static int dismd = LIVE_SCRN;        /* current mode */
  static int curscrn = LIVE_SCRN;        /* current display screen */

  if ((disp & HELP_SCRN) ^ (dismd & HELP_SCRN))
    {
      if (disp & HELP_SCRN)
        /* set help mode: */
        {
          if (!helpOpened) return dismd;
          currentScreen = &helpScreen.base;
          curscrn = HELP_SCRN;
          return (dismd |= HELP_SCRN);
        }
      else
        /* clear help mode: */
        {
          if (curscrn == HELP_SCRN)
            dismd & FROZ_SCRN ? (currentScreen = &frozenScreen.base, curscrn = FROZ_SCRN) : \
              (currentScreen = &liveScreen.base, curscrn = LIVE_SCRN);
          return (dismd &= ~HELP_SCRN);
        }
    }
  if ((disp & FROZ_SCRN) ^ (dismd & FROZ_SCRN))
    {
      if (disp & FROZ_SCRN)
        {
          if (frozenScreen.open(&liveScreen.base))
            {
              if (curscrn == LIVE_SCRN)
                {
                  currentScreen = &frozenScreen.base;
                  curscrn = FROZ_SCRN;
                }
              return (dismd |= FROZ_SCRN);
            }
          else
            return dismd;
        }
      else
        {
          if (curscrn == FROZ_SCRN)
            {
              frozenScreen.close();
              currentScreen = &liveScreen.base;
              curscrn = LIVE_SCRN;
            }
          return (dismd &= ~FROZ_SCRN);
        }
    }
  return dismd;
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
describeScreen (ScreenDescription *description) {
  currentScreen->describe(description);
}


unsigned char *
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
  return currentScreen->insert(key);
}


int
insertString (const unsigned char *string) {
  while (*string) {
    if (!insertKey(*string++)) return 0;
  }
  return 1;
}


int
routeCursor (int column, int row, int screen) {
  return currentScreen->route(column, row, screen);
}


int
setPointer (int column, int row) {
  return currentScreen->point(column, row);
}

int
getPointer (int *column, int *row) {
  return currentScreen->pointer(column, row);
}


int
selectVirtualTerminal (int vt) {
  return currentScreen->selectvt(vt);
}


int
switchVirtualTerminal (int vt) {
  return currentScreen->switchvt(vt);
}


int
currentVirtualTerminal (void) {
  return currentScreen->currentvt();
}


int
executeScreenCommand (int cmd) {
  return currentScreen->execute(cmd);
}


const char *const *
getScreenParameters (void) {
  return liveScreen.parameters();
}


int
openLiveScreen (char **parameters) {
  if (liveScreen.prepare(parameters)) {
    if (liveScreen.open()) {
      if (liveScreen.setup()) {
        currentScreen = &liveScreen.base;
        return 1;
      }
      liveScreen.close();
    }
  }
  return 0;
}


int
openRoutingScreen (void) {
  /* This function should be used in a forked process. Though we want to
   * have a separate file descriptor for the live screen from the one used
   * in the main thread.  So we close and reopen the device.
   */
  liveScreen.close();
  return liveScreen.open();
}


void
describeRoutingScreen (ScreenDescription *desscription) {
  liveScreen.base.describe(desscription);
}


void
closeRoutingScreen (void) {
  liveScreen.close();
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
