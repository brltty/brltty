/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "misc.h"
#include "scr.h"
#include "route.h"

/*
 * These control the performance of cursor routing.  The optimal settings
 * will depend heavily on system load, etc.  See the documentation for
 * further details.
 * NOTE: if you try to route the cursor to an invalid place, BRLTTY won't
 * give up until the timeout has elapsed!
 */
#define CURSOR_ROUTING_NICENESS 10	/* niceness of cursor routing subprocess */
#define CURSOR_ROUTING_TIMEOUT 2000	/* max wait for response to key press */
#define CURSOR_ROUTING_INTERVAL 0	/* how often to check for response */
#define CURSOR_ROUTING_SETTLE 400	/* grace period for temporary cursor relocation */

volatile pid_t routingProcess = 0;
volatile int routingStatus = -1;

static void
insertCursorKey (unsigned short key, const sigset_t *mask) {
  sigset_t old;
  sigprocmask(SIG_BLOCK, mask, &old);
  insertKey(key);
  sigprocmask(SIG_SETMASK, &old, NULL);
}

static int
doCursorRouting (int column, int row, int screen) {
  ScreenDescription scr;		/* for screen state infos */
  int oldx, oldy, dif;
  sigset_t mask;		/* for blocking of SIGUSR1 */

  /* Configure the cursor routing subprocess. */
  nice(CURSOR_ROUTING_NICENESS); /* reduce scheduling priority */

  /* Set up the signal mask. */
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_UNBLOCK, &mask, NULL);

  /* Initialise second thread of screen reading: */
  if (!openRoutingScreen()) return ROUTE_ERROR;
  describeRoutingScreen(&scr);

  /* Deal with vertical movement first, ignoring horizontal jumping ... */
  while ((dif = row - scr.posy) && (scr.no == screen)) {
    insertCursorKey((dif > 0)? SCR_KEY_CURSOR_DOWN: SCR_KEY_CURSOR_UP, &mask);
    timeout_yet(0);		/* initialise stop-watch */
    while (1) {
      delay(CURSOR_ROUTING_INTERVAL);	/* sleep a while ... */

      oldy = scr.posy;
      oldx = scr.posx;
      describeRoutingScreen(&scr);
      if ((scr.posy != oldy) || (scr.posx != oldx)) break;

      if (timeout_yet(CURSOR_ROUTING_TIMEOUT)) return ROUTE_WRONG_ROW;
    }

    if ((scr.posy == oldy && (scr.posx - oldx) * dif <= 0) ||
        (scr.posy != oldy && (row - scr.posy) * (row - scr.posy) >= dif * dif)) {
      delay(CURSOR_ROUTING_SETTLE);
      describeRoutingScreen(&scr);
      if ((scr.posy == oldy && (scr.posx - oldx) * dif <= 0) ||
          (scr.posy != oldy && (row - scr.posy) * (row - scr.posy) >= dif * dif)) {
        /* 
         * We are getting farther from our target... Let's try to 
         * go back to the previous position wich was obviously the 
         * nearest ever reached to date before giving up.
         */
        insertCursorKey((dif < 0)? SCR_KEY_CURSOR_DOWN: SCR_KEY_CURSOR_UP, &mask);
        break;
      }
    }
  }

  if (column >= 0) {	/* don't do this for vertical-only routing (column=-1) */
    /* Now horizontal movement, quitting if the vertical position is wrong: */
    while ((dif = column - scr.posx) && (scr.posy == row) && (scr.no == screen)) {
      insertCursorKey((dif > 0)? SCR_KEY_CURSOR_RIGHT: SCR_KEY_CURSOR_LEFT, &mask);
      timeout_yet(0);	/* initialise stop-watch */
      while (1) {
        delay(CURSOR_ROUTING_INTERVAL);	/* sleep a while ... */

        oldx = scr.posx;
        describeRoutingScreen(&scr);
        if ((scr.posx != oldx) || (scr.posy != row)) break;

        if (timeout_yet(CURSOR_ROUTING_TIMEOUT)) return ROUTE_WRONG_COLUMN;
      }

      if (scr.posy != row ||
          (column - scr.posx) * (column - scr.posx) >= dif * dif) {
        delay(CURSOR_ROUTING_SETTLE);
        describeRoutingScreen(&scr);
        if (scr.posy != row ||
            (column - scr.posx) * (column - scr.posx) >= dif * dif) {
          /* 
           * We probably wrapped on a short line... or are getting 
           * farther from our target. Try to get back to the previous
           * position which was obviously the nearest ever reached
           * to date before we exit.
           */
          insertCursorKey((dif > 0)? SCR_KEY_CURSOR_LEFT: SCR_KEY_CURSOR_RIGHT, &mask);
          break;
        }
      }
    }
  }

  closeRoutingScreen();		/* close second thread of screen reading */
  if (scr.posy != row) return ROUTE_WRONG_ROW;
  if ((column >= 0) && (scr.posx != column)) return ROUTE_WRONG_COLUMN;
  return ROUTE_OK;
}

int
startCursorRouting (int column, int row, int screen) {
  int started = 0;
  sigset_t newMask, oldMask;

  sigemptyset(&newMask);
  sigaddset(&newMask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &newMask, &oldMask);	/* block SIGUSR1 */

  /*
   * First, we must check if a subprocess is already running. 
   * If so, we send it a SIGUSR1 and wait for it to die. 
   * 
   * N.B. According to man 2 wait, setting SIGCHLD handler to SIG_IGN may mean
   * that wait can't catch the dying child.
   */
  if (routingProcess) {
    kill(routingProcess, SIGUSR1);
    do {
      sigsuspend(&oldMask);
    } while (routingProcess);
    routingStatus = -1;
  }

  switch (routingProcess = fork()) {
    case 0: /* child: cursor routing subprocess */
      _exit(doCursorRouting(column, row, screen));		/* terminate child process */

    case -1: /* error: fork() failed */
      LogError("fork");
      routingProcess = 0;
      break;

    default: /* parent: continue while cursor is being routed */
      started = 1;
      break;
  }

  sigprocmask(SIG_SETMASK, &oldMask, NULL); /* unblock SIGCHLD */
  return started;
}
