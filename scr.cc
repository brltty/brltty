/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

#include "scr_base.h"
#include "config.h"


/* The Live Screen type is instanciated elsewhere and chosen at link time
 * from all available screen source drivers.
 * It is defined as extern RealScreen *live;
 */

// a frozen screen image
FrozenScreen frozen;                

// the (possibly multi-page) online help
HelpScreen help;

// a pointer to the current screen object
Screen *current;


void
closeAllScreens (void) {
  live->close();
  frozen.close();
  help.close();
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
          current = &help;
          curscrn = HELP_SCRN;
          return (dismd |= HELP_SCRN);
        }
      else
        /* clear help mode: */
        {
          if (curscrn == HELP_SCRN)
            dismd & FROZ_SCRN ? (current = &frozen, curscrn = FROZ_SCRN) : \
              (current = live, curscrn = LIVE_SCRN);
          return (dismd &= ~HELP_SCRN);
        }
    }
  if ((disp & FROZ_SCRN) ^ (dismd & FROZ_SCRN))
    {
      if (disp & FROZ_SCRN)
        {
          if (frozen.open(live))
            {
              if (curscrn == LIVE_SCRN)
                {
                  current = &frozen;
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
              frozen.close();
              current = live;
              curscrn = LIVE_SCRN;
            }
          return (dismd &= ~FROZ_SCRN);
        }
    }
  return dismd;
}


void
describeScreen (ScreenDescription *description) {
  current->describe(*description);
}


unsigned char *
readScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  return current->read(box, buffer, mode);
}


int
insertKey (unsigned short key) {
  return current->insert(key);
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
  return current->route(column, row, screen);
}


int
selectVirtualTerminal (int vt) {
  return current->selectvt(vt);
}


int
switchVirtualTerminal (int vt) {
  return current->switchvt(vt);
}


int
executeScreenCommand (int cmd) {
  return current->execute(cmd);
}


const char *const *
getScreenParameters (void) {
  return live->parameters();
}


int
initializeLiveScreen (char **parameters) {
  if (live->prepare(parameters)) {
    if (live->open()) {
      if (live->setup()) {
        current = live;
        return 1;
      }
      live->close();
    }
  }
  return 0;
}


int
initializeRoutingScreen (void) {
  /* This function should be used in a forked process. Though we want to
   * have a separate file descriptor for the live screen from the one used
   * in the main thread.  So we close and reopen the device.
   */
  live->close();
  return live->open();
}


void
describeRoutingScreen (ScreenDescription *desscription) {
  live->describe(*desscription);
}


void
closeRoutingScreen (void) {
  live->close();
}


int
initializeHelpScreen (const char *file) {
  return helpOpened = help.open(file);
}


void
setHelpPageNumber (short page) {
  help.setPageNumber(page);
}


short
getHelpPageNumber (void) {
  return help.getPageNumber();
}


short
getHelpPageCount (void) {
  return help.getPageCount();
}
