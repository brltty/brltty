/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif /* HAVE_SIGNAL_H */

#ifdef SIGUSR1
#include <sys/wait.h>
#endif /* SIGUSR1 */

#include "log.h"
#include "program.h"
#include "timing.h"
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
#define ROUTING_INTERVAL	1	/* how often to check for response */
#define ROUTING_TIMEOUT	2000	/* max wait for response to key press */

typedef enum {
  CRR_DONE,
  CRR_NEAR,
  CRR_FAIL
} RoutingResult;

typedef struct {
#ifdef HAVE_SIGNAL_H
  sigset_t signalMask;
#endif /* HAVE_SIGNAL_H */

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

typedef enum {
  CURSOR_DIR_LEFT,
  CURSOR_DIR_RIGHT,
  CURSOR_DIR_UP,
  CURSOR_DIR_DOWN
} CursorDirection;

typedef struct {
  const char *name;
  ScreenKey key;
} CursorDirectionEntry;

static const CursorDirectionEntry cursorDirectionTable[] = {
  [CURSOR_DIR_LEFT]  = {.name="left" , .key=SCR_KEY_CURSOR_LEFT },
  [CURSOR_DIR_RIGHT] = {.name="right", .key=SCR_KEY_CURSOR_RIGHT},
  [CURSOR_DIR_UP]    = {.name="up"   , .key=SCR_KEY_CURSOR_UP   },
  [CURSOR_DIR_DOWN]  = {.name="down" , .key=SCR_KEY_CURSOR_DOWN }
};

typedef enum {
  CURSOR_AXIS_HORIZONTAL,
  CURSOR_AXIS_VERTICAL
} CursorAxis;

static void
adjustHorizontalCoordinate (int *y, int *x, int amount) {
  *x += amount;
}

static void
adjustVerticalCoordinate (int *y, int *x, int amount) {
  *y += amount;
}

typedef struct {
  const CursorDirectionEntry *forward;
  const CursorDirectionEntry *backward;
  void (*adjustCoordinate) (int *y, int *x, int amount);
} CursorAxisEntry;

static const CursorAxisEntry cursorAxisTable[] = {
  [CURSOR_AXIS_HORIZONTAL] = {
    .forward  = &cursorDirectionTable[CURSOR_DIR_RIGHT],
    .backward = &cursorDirectionTable[CURSOR_DIR_LEFT],
    .adjustCoordinate = adjustHorizontalCoordinate
  }
  ,
  [CURSOR_AXIS_VERTICAL] = {
    .forward  = &cursorDirectionTable[CURSOR_DIR_DOWN],
    .backward = &cursorDirectionTable[CURSOR_DIR_UP],
    .adjustCoordinate = adjustVerticalCoordinate
  }
};

static void
logRouting (const char *format, ...) {
  va_list arguments;

  va_start(arguments, format);
  vlogMessage(LOG_CATEGORY(CURSOR_ROUTING), format, &arguments);
  va_end(arguments);
}

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
    logRouting("screen changed: num=%d", description.number);
    routing->screenNumber = description.number;
    return 0;
  }

  if (!routing->rowBuffer) {
    routing->screenRows = description.rows;
    routing->screenColumns = description.cols;
    routing->verticalDelta = 0;

    if (!(routing->rowBuffer = calloc(routing->screenColumns, sizeof(*routing->rowBuffer)))) {
      logMallocError();
      goto error;
    }

    logRouting("screen: num=%d cols=%d rows=%d",
               routing->screenNumber,
               routing->screenColumns, routing->screenRows);
  } else if ((routing->screenRows != description.rows) ||
             (routing->screenColumns != description.cols)) {
    logRouting("size changed: cols=%d rows=%d",
               description.cols, description.rows);
    goto error;
  }

  routing->cury = description.posy - routing->verticalDelta;
  routing->curx = description.posx;

  if (readScreenRow(routing, NULL, description.posy)) return 1;
  logRouting("read failed: row=%d", description.posy);

error:
  routing->screenNumber = -1;
  return 0;
}

static void
moveCursor (RoutingData *routing, const CursorDirectionEntry *direction) {
#ifdef SIGUSR1
  sigset_t oldMask;
  sigprocmask(SIG_BLOCK, &routing->signalMask, &oldMask);
#endif /* SIGUSR1 */

  logRouting("move: %s", direction->name);
  insertScreenKey(direction->key);

#ifdef SIGUSR1
  sigprocmask(SIG_SETMASK, &oldMask, NULL);
#endif /* SIGUSR1 */
}

static int
awaitCursorMotion (RoutingData *routing, int direction, const CursorAxisEntry *axis) {
  int moved = 0;
  long int timeout = routing->timeSum / routing->timeCount;
  TimeValue start;

  int trgy = routing->cury;
  int trgx = routing->curx;

  routing->oldy = routing->cury;
  routing->oldx = routing->curx;

  axis->adjustCoordinate(&trgy, &trgx, direction);
  getMonotonicTime(&start);

  while (1) {
    long int time;
    TimeValue now;

    int oldy;
    int oldx;

    approximateDelay(ROUTING_INTERVAL);
    getMonotonicTime(&now);
    time = millisecondsBetween(&start, &now) + 1;

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

    oldy = routing->cury;
    oldx = routing->curx;
    if (!getCurrentPosition(routing)) return 0;

    if ((routing->cury != oldy) || (routing->curx != oldx)) {
      logRouting("moved: [%d,%d] -> [%d,%d] (%dms)",
                 oldx, oldy, routing->curx, routing->cury, time);

      if (!moved) {
        moved = 1;
        timeout = (time * 2) + 1;

        routing->timeSum += time * 8;
        routing->timeCount += 1;
      }

      if ((routing->cury == trgy) && (routing->curx == trgx)) break;

      if (ROUTING_INTERVAL) {
        start = now;
      } else {
        approximateDelay(1);
        getMonotonicTime(&start);
      }
    } else if (time > timeout) {
      if (!moved) logRouting("timed out: %ldms", timeout);
      break;
    }
  }

  return 1;
}

static RoutingResult
adjustCursorPosition (RoutingData *routing, int where, int trgy, int trgx, const CursorAxisEntry *axis) {
  logRouting("to: [%d,%d]", trgx, trgy);

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
    moveCursor(routing, ((dir > 0)? axis->forward: axis->backward));
    if (!awaitCursorMotion(routing, dir, axis)) return CRR_FAIL;

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
    moveCursor(routing, ((dir > 0)? axis->backward: axis->forward));
    return awaitCursorMotion(routing, -dir, axis)? CRR_NEAR: CRR_FAIL;
  }
}

static RoutingResult
adjustCursorHorizontally (RoutingData *routing, int where, int row, int column) {
  return adjustCursorPosition(routing, where, row, column, &cursorAxisTable[CURSOR_AXIS_HORIZONTAL]);
}

static RoutingResult
adjustCursorVertically (RoutingData *routing, int where, int row) {
  return adjustCursorPosition(routing, where, row, -1, &cursorAxisTable[CURSOR_AXIS_VERTICAL]);
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
    logRouting("from: [%d,%d]", routing.curx, routing.cury);

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

        logSystemError("waitpid");
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

      if (!ROUTING_INTERVAL) {
        int niceness = nice(ROUTING_NICENESS);

        if (niceness == -1) {
          logSystemError("nice");
        }
      }

      if (constructRoutingScreen()) {
        result = doRouting(column, row, screen);		/* terminate child process */
        destructRoutingScreen();		/* close second thread of screen reading */
      }

      _exit(result);		/* terminate child process */
    }

    case -1: /* error: fork() failed */
      logSystemError("fork");
      routingProcess = NOT_ROUTING;
      break;

    default: /* parent: continue while cursor is being routed */
      {
        static int first = 1;
        if (first) {
          first = 0;
          onProgramExit(exitRouting);
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
