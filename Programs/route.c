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
#define CSRJMP_NICENESS 10	/* niceness of cursor routing subprocess */
#define CSRJMP_TIMEOUT 2000	/* cursor routing idle timeout in ms */
#define CSRJMP_LOOP_DELAY 0	/* delay to use in csrjmp_sub() loops (ms) */
#define CSRJMP_SETTLE_DELAY 400	/* delay to use in csrjmp_sub() loops (ms) */

volatile pid_t routingProcess = 0;

static void
insertCursorKey (unsigned short key, const sigset_t *mask) {
  sigset_t old;
  sigprocmask(SIG_BLOCK, mask, &old);
  insertKey(key);
  sigprocmask(SIG_SETMASK, &old, NULL);
}

static void
doCursorRouting (int column, int row, int screen) {
  ScreenDescription scr;		/* for screen state infos */
  int oldx, oldy;		/* current cursor position */
  int dif, timedOut;
  sigset_t mask;		/* for blocking of SIGUSR1 */

  /* Set up signal mask: */
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_UNBLOCK, &mask, NULL);

  /* Initialise second thread of screen reading: */
  if (!openRoutingScreen()) return;
  describeRoutingScreen(&scr);

  /* Deal with vertical movement first, ignoring horizontal jumping ... */
  while ((dif = row - scr.posy) && (scr.no == screen)) {
    timeout_yet(0);		/* initialise stop-watch */
    insertCursorKey((dif > 0)? KEY_CURSOR_DOWN: KEY_CURSOR_UP, &mask);

    do {
#if CSRJMP_LOOP_DELAY > 0
      delay(CSRJMP_LOOP_DELAY);	/* sleep a while ... */
#endif /* CSRJMP_LOOP_DELAY > 0 */

      oldy = scr.posy;
      oldx = scr.posx;
      describeRoutingScreen(&scr);
      timedOut = timeout_yet(CSRJMP_TIMEOUT);
    } while ((scr.posy == oldy) && (scr.posx == oldx) && !timedOut);
    if (timedOut) break;

    if ((scr.posy == oldy && (scr.posx - oldx) * dif <= 0) ||
        (scr.posy != oldy && (row - scr.posy) * (row - scr.posy) >= dif * dif)) {
      delay(CSRJMP_SETTLE_DELAY);
      describeRoutingScreen(&scr);
      if ((scr.posy == oldy && (scr.posx - oldx) * dif <= 0) ||
          (scr.posy != oldy && (row - scr.posy) * (row - scr.posy) >= dif * dif)) {
        /* 
         * We are getting farther from our target... Let's try to 
         * go back to the previous position wich was obviously the 
         * nearest ever reached to date before giving up.
         */
        insertCursorKey((dif < 0)? KEY_CURSOR_DOWN: KEY_CURSOR_UP, &mask);
        break;
      }
    }
  }

  if (column >= 0) {	/* don't do this for vertical-only routing (column=-1) */
    /* Now horizontal movement, quitting if the vertical position is wrong: */
    while ((dif = column - scr.posx) && (scr.posy == row) && (scr.no == screen)) {
      timeout_yet(0);	/* initialise stop-watch */
      insertCursorKey((dif > 0)? KEY_CURSOR_RIGHT: KEY_CURSOR_LEFT, &mask);

      do {
#if CSRJMP_LOOP_DELAY > 0
        delay(CSRJMP_LOOP_DELAY);	/* sleep a while ... */
#endif /* CSRJMP_LOOP_DELAY > 0 */

        oldx = scr.posx;
        describeRoutingScreen(&scr);
        timedOut = timeout_yet(CSRJMP_TIMEOUT);
      } while ((scr.posx == oldx) && (scr.posy == row) && !timedOut);
      if (timedOut) break;

      if (scr.posy != row ||
          (column - scr.posx) * (column - scr.posx) >= dif * dif) {
        delay(CSRJMP_SETTLE_DELAY);
        describeRoutingScreen(&scr);
        if (scr.posy != row ||
            (column - scr.posx) * (column - scr.posx) >= dif * dif) {
          /* 
           * We probably wrapped on a short line... or are getting 
           * farther from our target. Try to get back to the previous
           * position which was obviously the nearest ever reached
           * to date before we exit.
           */
          insertCursorKey((dif > 0)? KEY_CURSOR_LEFT: KEY_CURSOR_RIGHT, &mask);
          break;
        }
      }
    }
  }

  closeRoutingScreen();		/* close second thread of screen reading */
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
  }

  switch (routingProcess = fork()) {
    case 0: /* child, cursor routing process */
      nice(CSRJMP_NICENESS); /* reduce scheduling priority */
      doCursorRouting(column, row, screen);
      _exit(0);		/* terminate child process */

    case -1: /* fork failed */
      LogError("fork");
      routingProcess = 0;
      break;

    default: /* parent, wait for child to return */
      started = 1;
      break;
  }

  sigprocmask(SIG_SETMASK, &oldMask, NULL); /* unblock SIGCHLD */
  return started;
}
