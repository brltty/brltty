/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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
#include "routing.h"

/*
 * These control the performance of cursor routing.  The optimal settings
 * will depend heavily on system load, etc.  See the documentation for
 * further details.
 * NOTE: if you try to route the cursor to an invalid place, BRLTTY won't
 * give up until the timeout has elapsed!
 */
#define ROUTING_NICENESS	10	/* niceness of cursor routing subprocess */
#define ROUTING_INTERVAL	0	/* how often to check for response */
#define ROUTING_TIMEOUT	2000	/* max wait for response to key press */

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
readScreenRow (RoutingData *routing, ScreenCharacter *buffer, int row) {
  if (!buffer) buffer = routing->rowBuffer;
  return readScreen(0, row, routing->screenColumns, 1, buffer);
}

static int
getCurrentPosition (RoutingData *routing) {
  ScreenDescription description;
  describeScreen(&description);

  if (description.number != routing->screenNumber) {
    routing->screenNumber = description.number;
    return 0;
  }

  if (!routing->rowBuffer) {
    routing->screenRows = description.rows;
    routing->screenColumns = description.cols;
    routing->verticalDelta = 0;
    if (!(routing->rowBuffer = calloc(routing->screenColumns, sizeof(*routing->rowBuffer)))) goto error;
  } else if ((routing->screenRows != description.rows) ||
             (routing->screenColumns != description.cols)) {
    goto error;
  }

  routing->cury = description.posy - routing->verticalDelta;
  routing->curx = description.posx;
  if (readScreenRow(routing, NULL, description.posy)) return 1;

error:
  routing->screenNumber = -1;
  return 0;
}

static void
insertCursorKey (RoutingData *routing, ScreenKey key) {
#ifdef SIGUSR1
  sigset_t oldMask;
  sigprocmask(SIG_BLOCK, &routing->signalMask, &oldMask);
#endif /* SIGUSR1 */

  insertScreenKey(key);

#ifdef SIGUSR1
  sigprocmask(SIG_SETMASK, &oldMask, NULL);
#endif /* SIGUSR1 */
}

static int
awaitCursorMotion (RoutingData *routing, int direction) {
  long timeout = routing->timeSum / routing->timeCount;
  struct timeval start;
  gettimeofday(&start, NULL);

  while (1) {
    long time;
    approximateDelay(ROUTING_INTERVAL);
    time = millisecondsSince(&start) + 1;

    {
      int row = routing->cury + routing->verticalDelta;
      int bestRow = row;
      int bestLength = 0;

      do {
        ScreenCharacter buffer[routing->screenColumns];
        if (!readScreenRow(routing, buffer, row)) break;

        {
          int before = routing->curx;
          int after = before;

          while (buffer[before].text == routing->rowBuffer[before].text)
            if (--before < 0)
              break;

          while (buffer[after].text == routing->rowBuffer[after].text)
            if (++after >= routing->screenColumns)
              break;

          {
            int length = after - before - 1;
            if (length > bestLength) {
              bestRow = row;
              if ((bestLength = length) == routing->screenColumns) break;
            }
          }
        }

        row -= direction;
      } while ((row >= 0) && (row < routing->screenRows));

      routing->verticalDelta = bestRow - routing->cury;
    }

    routing->oldy = routing->cury;
    routing->oldx = routing->curx;
    if (!getCurrentPosition(routing)) return 0;

    if ((routing->cury != routing->oldy) || (routing->curx != routing->oldx)) {
      routing->timeSum += time * 8;
      routing->timeCount += 1;
      break;
    }

    if (time > timeout) break;
  }

  return 1;
}

static RoutingResult
adjustCursorPosition (RoutingData *routing, int where, int trgy, int trgx, ScreenKey forward, ScreenKey backward) {
  while (1) {
    int dify = trgy - routing->cury;
    int difx = (trgx < 0)? 0: (trgx - routing->curx);
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
    insertCursorKey(routing, ((dir > 0)? forward: backward));
    if (!awaitCursorMotion(routing, dir)) return CRR_FAIL;

    if (routing->cury != routing->oldy) {
      if (routing->oldy != trgy) {
        if (((routing->cury - routing->oldy) * dir) > 0) {
          int dif = trgy - routing->cury;
          if ((dif * dify) >= 0) continue;
          if (where > 0) {
            if (routing->cury > trgy) return CRR_NEAR;
          } else if (where < 0) {
            if (routing->cury < trgy) return CRR_NEAR;
          } else {
            if ((dif * dif) < (dify * dify)) return CRR_NEAR;
          }
        }
      }
    } else if (routing->curx != routing->oldx) {
      if (((routing->curx - routing->oldx) * dir) > 0) {
        int dif = trgx - routing->curx;
        if (routing->cury != trgy) continue;
        if ((dif * difx) >= 0) continue;
        if (where > 0) {
          if (routing->curx > trgx) return CRR_NEAR;
        } else if (where < 0) {
          if (routing->curx < trgx) return CRR_NEAR;
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
    insertCursorKey(routing, ((dir > 0)? backward: forward));
    return awaitCursorMotion(routing, -dir)? CRR_NEAR: CRR_FAIL;
  }
}

static RoutingResult
adjustCursorHorizontally (RoutingData *routing, int where, int row, int column) {
  return adjustCursorPosition(routing, where, row, column, SCR_KEY_CURSOR_RIGHT, SCR_KEY_CURSOR_LEFT);
}

static RoutingResult
adjustCursorVertically (RoutingData *routing, int where, int row) {
  return adjustCursorPosition(routing, where, row, -1, SCR_KEY_CURSOR_DOWN, SCR_KEY_CURSOR_UP);
}

static RoutingStatus
doRouting (int column, int row, int screen) {
  RoutingData routing;

#ifdef SIGUSR1
  /* Set up the signal mask. */
  sigemptyset(&routing.signalMask);
  sigaddset(&routing.signalMask, SIGUSR1);
  sigprocmask(SIG_UNBLOCK, &routing.signalMask, NULL);
#endif /* SIGUSR1 */

  /* initialize the routing data structure */
  routing.screenNumber = screen;
  routing.rowBuffer = NULL;
  routing.timeSum = ROUTING_TIMEOUT;
  routing.timeCount = 1;

  if (getCurrentPosition(&routing)) {
    if (column < 0) {
      adjustCursorVertically(&routing, 0, row);
    } else {
      if (adjustCursorVertically(&routing, -1, row) != CRR_FAIL)
        if (adjustCursorHorizontally(&routing, 0, row, column) == CRR_NEAR)
          if (routing.cury < row)
            if (adjustCursorVertically(&routing, 1, routing.cury+1) != CRR_FAIL)
              adjustCursorHorizontally(&routing, 0, row, column);
    }
  }

  if (routing.rowBuffer) free(routing.rowBuffer);

  if (routing.screenNumber != screen) return ROUTING_ERROR;
  if (routing.cury != row) return ROUTING_WRONG_ROW;
  if ((column >= 0) && (routing.curx != column)) return ROUTING_WRONG_COLUMN;
  return ROUTING_DONE;
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
        return WIFEXITED(status)? WEXITSTATUS(status): ROUTING_ERROR;
      }

      if (process == -1) {
        if (errno == EINTR) goto doWait;

        if (errno == ECHILD) {
          routingProcess = NOT_ROUTING;
          return ROUTING_ERROR;
        }

        LogError("waitpid");
      }
    }
  }

  return ROUTING_NONE;
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
static RoutingStatus routingStatus = ROUTING_NONE;

RoutingStatus
getRoutingStatus (int wait) {
  RoutingStatus status = routingStatus;
  routingStatus = ROUTING_NONE;
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
      int result = ROUTING_ERROR;
      nice(ROUTING_NICENESS);
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
