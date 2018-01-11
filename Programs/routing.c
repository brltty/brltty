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
#include "thread.h"
#include "async_wait.h"
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
  struct {
    sigset_t mask;
  } signal;
#endif /* HAVE_SIGNAL_H */

  struct {
    int number;
    int width;
    int height;
  } screen;

  struct {
    int scroll;
    int row;
    ScreenCharacter *buffer;
  } vertical;

  struct {
    int column;
    int row;
  } current;

  struct {
    int column;
    int row;
  } previous;

  struct {
    long sum;
    int count;
  } time;
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

typedef struct {
  const CursorDirectionEntry *forward;
  const CursorDirectionEntry *backward;
} CursorAxisEntry;

static const CursorAxisEntry cursorAxisTable[] = {
  [CURSOR_AXIS_HORIZONTAL] = {
    .forward  = &cursorDirectionTable[CURSOR_DIR_RIGHT],
    .backward = &cursorDirectionTable[CURSOR_DIR_LEFT]
  }
  ,
  [CURSOR_AXIS_VERTICAL] = {
    .forward  = &cursorDirectionTable[CURSOR_DIR_DOWN],
    .backward = &cursorDirectionTable[CURSOR_DIR_UP]
  }
};

#define logRouting(...) logMessage(LOG_CATEGORY(CURSOR_ROUTING), __VA_ARGS__)

static int
readRow (RoutingData *routing, ScreenCharacter *buffer, int row) {
  if (!buffer) buffer = routing->vertical.buffer;
  if (readScreenRow(row, routing->screen.width, buffer)) return 1;
  logRouting("read failed: row=%d", row);
  return 0;
}

static int
getCurrentPosition (RoutingData *routing) {
  ScreenDescription description;
  describeScreen(&description);

  if (description.number != routing->screen.number) {
    logRouting("screen changed: %d -> %d", routing->screen.number, description.number);
    routing->screen.number = description.number;
    return 0;
  }

  if (!routing->vertical.buffer) {
    routing->screen.width = description.cols;
    routing->screen.height = description.rows;
    routing->vertical.scroll = 0;

    if (!(routing->vertical.buffer = malloc(ARRAY_SIZE(routing->vertical.buffer, routing->screen.width)))) {
      logMallocError();
      goto error;
    }

    logRouting("screen: num=%d cols=%d rows=%d",
               routing->screen.number,
               routing->screen.width, routing->screen.height);
  } else if ((routing->screen.width != description.cols) ||
             (routing->screen.height != description.rows)) {
    logRouting("size changed: %dx%d -> %dx%d",
               routing->screen.width, routing->screen.height,
               description.cols, description.rows);
    goto error;
  }

  routing->current.row = description.posy + routing->vertical.scroll;
  routing->current.column = description.posx;
  return 1;

error:
  routing->screen.number = -1;
  return 0;
}

static void
handleVerticalScrolling (RoutingData *routing, int direction) {
  int firstRow = routing->vertical.row;
  int currentRow = firstRow;

  int bestRow = firstRow;
  int bestLength = 0;

  do {
    ScreenCharacter buffer[routing->screen.width];
    if (!readRow(routing, buffer, currentRow)) break;

    int length;
    {
      int before = routing->current.column;
      int after = before;

      while (buffer[before].text == routing->vertical.buffer[before].text)
        if (--before < 0)
          break;

      while (buffer[after].text == routing->vertical.buffer[after].text)
        if (++after >= routing->screen.width)
          break;

      length = after - before - 1;
    }

    if (length > bestLength) {
      bestRow = currentRow;
      if ((bestLength = length) == routing->screen.width) break;
    }

    currentRow -= direction;
  } while ((currentRow >= 0) && (currentRow < routing->screen.height));

  int delta = bestRow - firstRow;
  routing->vertical.scroll -= delta;
  routing->current.row -= delta;
}

static int
awaitCursorMotion (RoutingData *routing, int direction) {
  routing->previous.column = routing->current.column;
  routing->previous.row = routing->current.row;

  TimeValue start;
  getMonotonicTime(&start);

  int moved = 0;
  long int timeout = routing->time.sum / routing->time.count;

  while (1) {
    asyncWait(ROUTING_INTERVAL);

    TimeValue now;
    getMonotonicTime(&now);
    long int time = millisecondsBetween(&start, &now) + 1;

    int oldy = routing->current.row;
    int oldx = routing->current.column;
    if (!getCurrentPosition(routing)) return 0;

    if ((routing->current.row != oldy) || (routing->current.column != oldx)) {
      logRouting("moved: [%d,%d] -> [%d,%d] (%ldms)",
                 oldx, oldy, routing->current.column, routing->current.row, time);

      if (!moved) {
        moved = 1;
        timeout = (time * 2) + 1;

        routing->time.sum += time * 8;
        routing->time.count += 1;
      }

      if (ROUTING_INTERVAL) {
        start = now;
      } else {
        asyncWait(1);
        getMonotonicTime(&start);
      }
    } else if (time > timeout) {
      break;
    }
  }

  handleVerticalScrolling(routing, direction);
  return 1;
}

static int
moveCursor (RoutingData *routing, const CursorDirectionEntry *direction) {
  routing->vertical.row = routing->current.row - routing->vertical.scroll;
  if (!readRow(routing, NULL, routing->vertical.row)) return 0;

#ifdef SIGUSR1
  sigset_t oldMask;
  sigprocmask(SIG_BLOCK, &routing->signal.mask, &oldMask);
#endif /* SIGUSR1 */

  logRouting("move: %s", direction->name);
  insertScreenKey(direction->key);

#ifdef SIGUSR1
  sigprocmask(SIG_SETMASK, &oldMask, NULL);
#endif /* SIGUSR1 */

  return 1;
}

static RoutingResult
adjustCursorPosition (RoutingData *routing, int where, int trgy, int trgx, const CursorAxisEntry *axis) {
  logRouting("to: [%d,%d]", trgx, trgy);

  while (1) {
    int dify = trgy - routing->current.row;
    int difx = (trgx < 0)? 0: (trgx - routing->current.column);
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
    if (!moveCursor(routing, ((dir > 0)? axis->forward: axis->backward))) return CRR_FAIL;
    if (!awaitCursorMotion(routing, dir)) return CRR_FAIL;

    if (routing->current.row != routing->previous.row) {
      if (routing->previous.row != trgy) {
        if (((routing->current.row - routing->previous.row) * dir) > 0) {
          int dif = trgy - routing->current.row;
          if ((dif * dify) >= 0) continue;
          if (where > 0) {
            if (routing->current.row > trgy) return CRR_NEAR;
          } else if (where < 0) {
            if (routing->current.row < trgy) return CRR_NEAR;
          } else {
            if ((dif * dif) < (dify * dify)) return CRR_NEAR;
          }
        }
      }
    } else if (routing->current.column != routing->previous.column) {
      if (((routing->current.column - routing->previous.column) * dir) > 0) {
        int dif = trgx - routing->current.column;
        if (routing->current.row != trgy) continue;
        if ((dif * difx) >= 0) continue;
        if (where > 0) {
          if (routing->current.column > trgx) return CRR_NEAR;
        } else if (where < 0) {
          if (routing->current.column < trgx) return CRR_NEAR;
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
    if (!moveCursor(routing, ((dir > 0)? axis->backward: axis->forward))) return CRR_FAIL;
    return awaitCursorMotion(routing, -dir)? CRR_NEAR: CRR_FAIL;
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

typedef struct {
  int column;
  int row;
  int screen;
} RoutingParameters;

static RoutingStatus
routeCursor (const RoutingParameters *parameters) {
  RoutingData routing;

#ifdef SIGUSR1
  /* Set up the signal mask. */
  sigemptyset(&routing.signal.mask);
  sigaddset(&routing.signal.mask, SIGUSR1);
  sigprocmask(SIG_UNBLOCK, &routing.signal.mask, NULL);
#endif /* SIGUSR1 */

  /* initialize the routing data structure */
  routing.screen.number = parameters->screen;
  routing.vertical.buffer = NULL;
  routing.time.sum = ROUTING_TIMEOUT;
  routing.time.count = 1;

  if (getCurrentPosition(&routing)) {
    logRouting("from: [%d,%d]", routing.current.column, routing.current.row);

    if (parameters->column < 0) {
      adjustCursorVertically(&routing, 0, parameters->row);
    } else {
      if (adjustCursorVertically(&routing, -1, parameters->row) != CRR_FAIL) {
        if (adjustCursorHorizontally(&routing, 0, parameters->row, parameters->column) == CRR_NEAR) {
          if (routing.current.row < parameters->row) {
            if (adjustCursorVertically(&routing, 1, routing.current.row+1) != CRR_FAIL) {
              adjustCursorHorizontally(&routing, 0, parameters->row, parameters->column);
            }
          }
        }
      }
    }
  }

  if (routing.vertical.buffer) free(routing.vertical.buffer);

  if (routing.screen.number != parameters->screen) return ROUTING_ERROR;
  if (routing.current.row != parameters->row) return ROUTING_WRONG_ROW;
  if ((parameters->column >= 0) && (routing.current.column != parameters->column)) return ROUTING_WRONG_COLUMN;
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
exitCursorRouting (void *data) {
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

static int
startRoutingProcess (const RoutingParameters *parameters) {
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
        result = routeCursor(parameters);		/* terminate child process */
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
          onProgramExit("cursor-routing", exitCursorRouting, NULL);
        }
      }

      started = 1;
      break;
  }

  return started;
#else /* SIGUSR1 */
  routingStatus = routeCursor(parameters);
  return 1;
#endif /* SIGUSR1 */
}

#ifdef GOT_PTHREADS
typedef struct {
  const RoutingParameters *const parameters;

  int result;
} RoutingThreadArgument;

THREAD_FUNCTION(runRoutingThread) {
  RoutingThreadArgument *rta = argument;

  rta->result = startRoutingProcess(rta->parameters);
  return NULL;
}
#endif /* GOT_PTHREADS */

int
startRouting (int column, int row, int screen) {
  const RoutingParameters parameters = {
    .column = column,
    .row = row,
    .screen = screen
  };

#ifdef GOT_PTHREADS
  int started = 0;

  RoutingThreadArgument rta = {
    .parameters = &parameters
  };

  if (callThreadFunction("cursor-routing", runRoutingThread, &rta, NULL)) {
    if (rta.result) {
      started = 1;
    }
  }

  return started;
#else /* GOT_PTHREADS */
  return startRoutingProcess(&parameters);
#endif /* GOT_PTHREADS */
}
