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
#include <sys/time.h>
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
#define CURSOR_ROUTING_NICENESS	10	/* niceness of cursor routing subprocess */
#define CURSOR_ROUTING_INTERVAL	0	/* how often to check for response */
#define CURSOR_ROUTING_TIMEOUT	2000	/* max wait for response to key press */

volatile pid_t routingProcess = 0;
volatile int routingStatus = -1;

typedef enum {
  CRR_DONE,
  CRR_NEAR,
  CRR_FAIL
} CursorRoutingResult;

typedef struct {
  sigset_t signalMask;

  int screenNumber;
  int cury, curx;
  int oldy, oldx;

  long timeSum;
  int timeCount;
} CursorRoutingData;

static int
getCurrentPosition (CursorRoutingData *crd) {
  ScreenDescription scr;
  describeRoutingScreen(&scr);

  if (scr.no != crd->screenNumber) {
    crd->screenNumber = scr.no;
    return 0;
  }

  crd->cury = scr.posy;
  crd->curx = scr.posx;
  return 1;
}

static void
insertCursorKey (CursorRoutingData *crd, ScreenKey key) {
  sigset_t oldMask;
  sigprocmask(SIG_BLOCK, &crd->signalMask, &oldMask);
  insertKey(key);
  sigprocmask(SIG_SETMASK, &oldMask, NULL);
}

static int
awaitCursorMotion (CursorRoutingData *crd) {
  long timeout = crd->timeSum / crd->timeCount;
  struct timeval start;
  gettimeofday(&start, NULL);

  while (1) {
    long time;
    approximateDelay(CURSOR_ROUTING_INTERVAL);
    time = millisecondsSince(&start) + 1;

    crd->oldy = crd->cury;
    crd->oldx = crd->curx;
    if (!getCurrentPosition(crd)) return 0;

    if ((crd->cury != crd->oldy) || (crd->curx != crd->oldx)) {
      crd->timeSum += time * 8;
      crd->timeCount += 1;
      break;
    }

    if (time > timeout) break;
  }

  return 1;
}

static CursorRoutingResult
adjustCursorPosition (CursorRoutingData *crd, int where, int trgy, int trgx, ScreenKey forward, ScreenKey backward) {
  while (1) {
    int dify = trgy - crd->cury;
    int difx = (trgx >= 0)? (trgx - crd->curx): 0;
    int dir;

    /* determine which direction the cursor needs to move in */
    if (dify) {
      dir = (dify > 0)? 1: -1;
    } else if (difx) {
      dir = (difx > 0)? 1: -1;
    } else {
      return CRR_DONE;
    }

    /* tell the cursor to move in the needed direction */
    insertCursorKey(crd, ((dir > 0)? forward: backward));
    if (!awaitCursorMotion(crd)) return CRR_FAIL;

    if (crd->cury != crd->oldy) {
      if (crd->oldy != trgy) {
        if (((crd->cury - crd->oldy) * dir) > 0) {
          int dif = trgy - crd->cury;
          if ((dif * dify) >= 0) continue;
          if (where > 0) {
            if (crd->cury > trgy) return CRR_NEAR;
          } else if (where < 0) {
            if (crd->cury < trgy) return CRR_NEAR;
          } else {
            if ((dif * dif) < (dify * dify)) return CRR_NEAR;
          }
        }
      }
    } else if (crd->curx != crd->oldx) {
      if (((crd->curx - crd->oldx) * dir) > 0) {
        int dif = trgx - crd->curx;
        if (crd->cury != trgy) continue;
        if ((dif * difx) >= 0) continue;
        if (where > 0) {
          if (crd->curx > trgx) return CRR_NEAR;
        } else if (where < 0) {
          if (crd->curx < trgx) return CRR_NEAR;
        } else {
          if ((dif * dif) < (difx * difx)) return CRR_NEAR;
        }
      }
    } else {
      return CRR_NEAR;
    }

    /* We're getting farther from our target. Before giving up, let's
     * try going back to the previous position since it was obviously
     * the nearest ever reached.
     */
    insertCursorKey(crd, ((dir > 0)? backward: forward));
    return awaitCursorMotion(crd)? CRR_NEAR: CRR_FAIL;
  }
}

static CursorRoutingResult
adjustCursorHorizontally (CursorRoutingData *crd, int where, int trgy, int trgx) {
  return adjustCursorPosition(crd, where, trgy, trgx, SCR_KEY_CURSOR_RIGHT, SCR_KEY_CURSOR_LEFT);
}

static CursorRoutingResult
adjustCursorVertically (CursorRoutingData *crd, int where, int trgy) {
  return adjustCursorPosition(crd, where, trgy, -1, SCR_KEY_CURSOR_DOWN, SCR_KEY_CURSOR_UP);
}

static int
doCursorRouting (int column, int row, int screen) {
  CursorRoutingData crd;

  /* Configure the cursor routing subprocess. */
  nice(CURSOR_ROUTING_NICENESS); /* reduce scheduling priority */

  /* Set up the signal mask. */
  sigemptyset(&crd.signalMask);
  sigaddset(&crd.signalMask, SIGUSR1);
  sigprocmask(SIG_UNBLOCK, &crd.signalMask, NULL);

  /* initialize the key response time cache */
  crd.timeSum = CURSOR_ROUTING_TIMEOUT;
  crd.timeCount = 1;

  /* Initialise second thread of screen reading: */
  if (!openRoutingScreen()) return ROUTE_ERROR;
  crd.screenNumber = screen;

  if (getCurrentPosition(&crd)) {
    if (column < 0) {
      adjustCursorVertically(&crd, 0, row);
    } else {
      if (adjustCursorVertically(&crd, -1, row) != CRR_FAIL)
        if (adjustCursorHorizontally(&crd, 0, row, column) == CRR_NEAR)
          if (crd.cury < row)
            if (adjustCursorVertically(&crd, 1, crd.cury+1) != CRR_FAIL)
              adjustCursorHorizontally(&crd, 0, row, column);
    }
  }

  closeRoutingScreen();		/* close second thread of screen reading */
  if (crd.screenNumber != screen) return ROUTE_ERROR;
  if (crd.cury != row) return ROUTE_WRONG_ROW;
  if ((column >= 0) && (crd.curx != column)) return ROUTE_WRONG_COLUMN;
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
