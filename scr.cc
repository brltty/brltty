/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

#include "scrdev.h"
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


char **
getScreenParameters (void)
{
  return live->parameters();
}


int
initializeScreen (char **parameters)
{
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
initscr_phys (void)
{
  /* This function should be used in a forked process. Though we want to
   * have a separate file descriptor for the live screen from the one used
   * in the main thread.  So we close and reopen the device.
   */
  live->close();
  return live->open();
}


void
getScreenStatus (ScreenStatus *stat)
{
  current->getstat(*stat);
}


void
getstat_phys (ScreenStatus *stat)
{
  live->getstat(*stat);
}


unsigned char *
getscr (winpos pos, unsigned char *buffer, short mode)
{
  return current->getscr(pos, buffer, mode);
}


int
insertKey (unsigned short key)
{
  return live->insert(key);
}

int
insertString (const unsigned char *string)
{
  while (*string)
    if (!insertKey(*string++))
      return 0;
  return 1;
}


int
switchVirtualTerminal (int vt)
{
  return live->switchvt(vt);
}


void
closeScreen (void)
{
  live->close();
  frozen.close();
  help.close();
}


void
closescr_phys (void)
{
  live->close();
}


int
selectdisp (int disp)
{
  static int dismd = LIVE_SCRN;        /* current mode */
  static int curscrn = LIVE_SCRN;        /* current display screen */

  if ((disp & HELP_SCRN) ^ (dismd & HELP_SCRN))
    {
      if (disp & HELP_SCRN)
        /* set help mode: */
        {
          if (!help.open("??????"))
            {
              current = &help;
              curscrn = HELP_SCRN;
              return (dismd |= HELP_SCRN);
            }
          else
            return dismd;
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
          if (!frozen.open(live))
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


int
inithlpscr (char *helpfile)
{
  return help.open(helpfile);
}


void
sethlpscr (short x)
{
  help.setscrno(x);
}


short
numhlpscr (void)
{
  return help.numscreens();
}
