/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "prologue.h"

#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
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

typedef enum {
  CRR_DONE,
  CRR_NEAR,
  CRR_FAIL
} RoutingResult;

typedef struct {
  sigset_t signalMask;

  int screenNumber;
  int screenRows;
  int screenColumns;

  int verticalDelta;
  ScreenCharacter *rowBuffer;

  int cury, curx;
  int oldy, oldx;

  long timeSum;
  int timeCount;
} RoutingData;

static int
readScreenRow (RoutingData *crd, ScreenCharacter *buffer, int row) {
  if (!buffer) buffer = crd->rowBuffer;
  return readScreen(0, row, crd->screenColumns, 1, buffer);
}

static int
getCurrentPosition (RoutingData *crd) {
  ScreenDescription description;
  describeScreen(&description);

  if (description.number != crd->screenNumber) {
    crd->screenNumber = description.number;
    return 0;
  }

  if (!crd->rowBuffer) {
    crd->screenRows = description.rows;
    crd->screenColumns = description.cols;
    crd->verticalDelta = 0;
    if (!(crd->rowBuffer = calloc(crd->screenColumns, sizeof(*crd->rowBuffer)))) goto error;
  } else if ((crd->screenRows != description.rows) ||
             (crd->screenColumns != description.cols)) {
    goto error;
  }

  crd->cury = description.posy - crd->verticalDelta;
  crd->curx = description.posx;
  if (readScreenRow(crd, NULL, description.posy)) return 1;

error:
  crd->screenNumber = -1;
  return 0;
}

static void
insertCursorKey (RoutingData *crd, ScreenKey key) {
#ifdef SIGUSR1
  sigset_t oldMask;
  sigprocmask(SIG_BLOCK, &crd->signalMask, &oldMask);
#endif /* SIGUSR1 */

  insertScreenKey(key);

#ifdef SIGUSR1
  sigprocmask(SIG_SETMASK, &oldMask, NULL);
#endif /* SIGUSR1 */
}

static int
awaitCursorMotion (RoutingData *crd, int direction) {
  long timeout = crd->timeSum / crd->timeCount;
  struct timeval start;
  gettimeofday(&start, NULL);

  while (1) {
    long time;
    approximateDelay(CURSOR_ROUTING_INTERVAL);
    time = millisecondsSince(&start) + 1;

    {
      int row = crd->cury + crd->verticalDelta;
      int bestRow = row;
      int bestLength = 0;

      do {
        ScreenCharacter buffer[crd->screenColumns];
        if (!readScreenRow(crd, buffer, row)) break;

        {
          int before = crd->curx;
          int after = before;

          while (buffer[before].text == crd->rowBuffer[before].text)
            if (--before < 0)
              break;

          while (buffer[after].text == crd->rowBuffer[after].text)
            if (++after >= crd->screenColumns)
              break;

          {
            int length = after - before - 1;
            if (length > bestLength) {
              bestRow = row;
              if ((bestLength = length) == crd->screenColumns) break;
            }
          }
        }

        row -= direction;
      } while ((row >= 0) && (row < crd->screenRows));

      crd->verticalDelta = bestRow - crd->cury;
    }

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

static RoutingResult
adjustCursorPosition (RoutingData *crd, int where, int trgy, int trgx, ScreenKey forward, ScreenKey backward) {
  while (1) {
    int dify = trgy - crd->cury;
    int difx = (trgx < 0)? 0: (trgx - crd->curx);
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
    if (!awaitCursorMotion(crd, dir)) return CRR_FAIL;

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
    return awaitCursorMotion(crd, -dir)? CRR_NEAR: CRR_FAIL;
  }
}

static RoutingResult
adjustCursorHorizontally (RoutingData *crd, int where, int row, int column) {
  return adjustCursorPosition(crd, where, row, column, SCR_KEY_CURSOR_RIGHT, SCR_KEY_CURSOR_LEFT);
}

static RoutingResult
adjustCursorVertically (RoutingData *crd, int where, int row) {
  return adjustCursorPosition(crd, where, row, -1, SCR_KEY_CURSOR_DOWN, SCR_KEY_CURSOR_UP);
}

static RoutingStatus
doRouting (int column, int row, int screen) {
  RoutingData crd;

#ifdef SIGUSR1
  /* Set up the signal mask. */
  sigemptyset(&crd.signalMask);
  sigaddset(&crd.signalMask, SIGUSR1);
  sigprocmask(SIG_UNBLOCK, &crd.signalMask, NULL);
#endif /* SIGUSR1 */

  /* initialize the routing data structure */
  crd.screenNumber = screen;
  crd.rowBuffer = NULL;
  crd.timeSum = CURSOR_ROUTING_TIMEOUT;
  crd.timeCount = 1;

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

  if (crd.rowBuffer) free(crd.rowBuffer);

  if (crd.screenNumber != screen) return ROUTE_ERROR;
  if (crd.cury != row) return ROUTE_WRONG_ROW;
  if ((column >= 0) && (crd.curx != column)) return ROUTE_WRONG_COLUMN;
  return ROUTE_DONE;
}

#ifdef SIGUSR1
#define NOT_ROUTING 0

static pid_t routingProcess = NOT_ROUTING;

int
isRouting (void) {
  return routingProcess != NOT_ROUTING;
}

RoutingStatus
getRoutingStatus (int wait) {
  if (isRouting()) {
    int options = 0;
    if (!wait) options |= WNOHANG;

  doWait:
    {
      int status;
      pid_t process = waitpid(routingProcess, &status, options);

      if (process == routingProcess) {
        routingProcess = NOT_ROUTING;
        return WIFEXITED(status)? WEXITSTATUS(status): ROUTE_ERROR;
      }

      if (process == -1) {
        if (errno == EINTR) goto doWait;
        LogError("waitpid");
      }
    }
  }

  return ROUTE_NONE;
}

static void
stopRouting (void) {
  if (isRouting()) {
    kill(routingProcess, SIGUSR1);
    getRoutingStatus(1);
  }
}

static void
exitRouting (void) {
  stopRouting();
}
#else /* SIGUSR1 */
static RoutingStatus routingStatus = ROUTE_NONE;

RoutingStatus
getRoutingStatus (int wait) {
  RoutingStatus status = routingStatus;
  routingStatus = ROUTE_NONE;
  return status;
}

int
isRouting (void) {
  return 0;
}
#endif /* SIGUSR1 */

int
startRouting (int column, int row, int screen) {
#ifdef SIGUSR1
  int started = 0;

  stopRouting();

  switch (routingProcess = fork()) {
    case 0: { /* child: cursor routing subprocess */
      int result = ROUTE_ERROR;
      nice(CURSOR_ROUTING_NICENESS);
      if (constructRoutingScreen())
        result = doRouting(column, row, screen);		/* terminate child process */
      destructRoutingScreen();		/* close second thread of screen reading */
      _exit(result);		/* terminate child process */
    }

    case -1: /* error: fork() failed */
      LogError("fork");
      routingProcess = NOT_ROUTING;
      break;

    default: /* parent: continue while cursor is being routed */
      {
        static int first = 1;
        if (first) {
          first = 0;
          atexit(exitRouting);
        }
      }

      started = 1;
      break;
  }

  return started;
#else /* SIGUSR1 */
  routingStatus = doRouting(column, row, screen);
  return 1;
#endif /* SIGUSR1 */
}
