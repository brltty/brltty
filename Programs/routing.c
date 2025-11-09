/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "parameters.h"
#include "log.h"
#include "async_alarm.h"
#include "async_handle.h"
#include "timing.h"
#include "scr.h"
#include "routing.h"
#include "embed.h"

typedef enum {
  CRR_DONE,
  CRR_NEAR,
  CRR_FAIL
} RoutingResult;

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

typedef enum {
  RS_IDLE,
  RS_INITIAL_POSITION,
  RS_ADJUSTING,
  RS_WAITING_FOR_MOTION,
  RS_COMPLETED
} RoutingState;

typedef enum {
  RP_INITIAL_VERTICAL,
  RP_INITIAL_HORIZONTAL,
  RP_RETRY_VERTICAL,
  RP_RETRY_HORIZONTAL_FINAL
} RoutingPhase;

typedef struct {
  RoutingState state;
  RoutingPhase phase;

  struct {
    int column;
    int row;
    int screen;
  } target;

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

  struct {
    TimeValue start;
    long timeout;
    int hasMoved;
    int direction;
    const CursorAxisEntry *axis;
    int where;
    int targetRow;
    int targetColumn;
  } motion;

  RoutingStatus status;
  AsyncHandle timeoutAlarm;
} RoutingContext;

static RoutingContext routingContext = {
  .state = RS_IDLE
};

#define logRouting(...) logMessage(LOG_CATEGORY(CURSOR_ROUTING), __VA_ARGS__)

static void completeRouting(RoutingStatus status);
static void processRoutingStateMachine(void);
static RoutingResult evaluateCursorMotion(RoutingContext *ctx);

static int
readRow (RoutingContext *ctx, ScreenCharacter *buffer, int row) {
  if (!buffer) buffer = ctx->vertical.buffer;
  if (readScreenRow(row, ctx->screen.width, buffer)) return 1;
  logRouting("read failed: row=%d", row);
  return 0;
}

static int
getCurrentPosition (RoutingContext *ctx) {
  ScreenDescription description;
  describeScreen(&description);

  if (description.number != ctx->screen.number) {
    logRouting("screen changed: %d -> %d", ctx->screen.number, description.number);
    ctx->screen.number = description.number;
    return 0;
  }

  if (!ctx->vertical.buffer) {
    ctx->screen.width = description.cols;
    ctx->screen.height = description.rows;
    ctx->vertical.scroll = 0;

    if (!(ctx->vertical.buffer = malloc(ARRAY_SIZE(ctx->vertical.buffer, ctx->screen.width)))) {
      logMallocError();
      goto error;
    }

    logRouting("screen: num=%d cols=%d rows=%d",
               ctx->screen.number,
               ctx->screen.width, ctx->screen.height);
  } else if ((ctx->screen.width != description.cols) ||
             (ctx->screen.height != description.rows)) {
    logRouting("size changed: %dx%d -> %dx%d",
               ctx->screen.width, ctx->screen.height,
               description.cols, description.rows);
    goto error;
  }

  ctx->current.row = description.posy + ctx->vertical.scroll;
  ctx->current.column = description.posx;
  return 1;

error:
  ctx->screen.number = -1;
  return 0;
}

static void
handleVerticalScrolling (RoutingContext *ctx, int direction) {
  int firstRow = ctx->vertical.row;
  int currentRow = firstRow;

  int bestRow = firstRow;
  int bestLength = 0;

  do {
    ScreenCharacter buffer[ctx->screen.width];
    if (!readRow(ctx, buffer, currentRow)) break;

    int length;
    {
      int before = ctx->current.column;
      int after = before;

      while (buffer[before].text == ctx->vertical.buffer[before].text)
        if (--before < 0)
          break;

      while (buffer[after].text == ctx->vertical.buffer[after].text)
        if (++after >= ctx->screen.width)
          break;

      length = after - before - 1;
    }

    if (length > bestLength) {
      bestRow = currentRow;
      if ((bestLength = length) == ctx->screen.width) break;
    }

    currentRow -= direction;
  } while ((currentRow >= 0) && (currentRow < ctx->screen.height));

  int delta = bestRow - firstRow;
  ctx->vertical.scroll -= delta;
  ctx->current.row -= delta;
}

static void
startMotionTimeout(RoutingContext *ctx) {
  long timeout = ctx->time.sum / ctx->time.count;

  if (ctx->timeoutAlarm) {
    asyncCancelRequest(ctx->timeoutAlarm);
  }

  getMonotonicTime(&ctx->motion.start);
  ctx->motion.timeout = timeout;
  ctx->motion.hasMoved = 0;
}

ASYNC_ALARM_CALLBACK(handleRoutingTimeout) {
  RoutingContext *ctx = &routingContext;

  asyncDiscardHandle(ctx->timeoutAlarm);
  ctx->timeoutAlarm = NULL;

  logRouting("timeout: cursor did not move within %ldms", ctx->motion.timeout);

  // Cursor didn't move - treat this as reaching the nearest position
  handleVerticalScrolling(ctx, ctx->motion.direction);

  // Evaluate the result - cursor stuck means we're as close as we can get
  RoutingResult result = evaluateCursorMotion(ctx);

  if (result == CRR_NEAR || result == CRR_FAIL) {
    // Can't get closer or failed
    if (ctx->current.row != ctx->target.row) {
      completeRouting(ROUTING_STATUS_ROW);
    } else if (ctx->current.column != ctx->target.column) {
      completeRouting(ROUTING_STATUS_COLUMN);
    } else {
      completeRouting(ROUTING_STATUS_FAILURE);
    }
  } else {
    // CRR_DONE - shouldn't happen in timeout, but handle it
    ctx->state = RS_ADJUSTING;
    processRoutingStateMachine();
  }
}

static int
moveCursor (RoutingContext *ctx, const CursorDirectionEntry *direction) {
  ctx->vertical.row = ctx->current.row - ctx->vertical.scroll;
  if (!readRow(ctx, NULL, ctx->vertical.row)) return 0;

  logRouting("move: %s", direction->name);
  insertScreenKey(direction->key);

  return 1;
}

static RoutingResult
evaluateCursorMotion(RoutingContext *ctx) {
  int trgy = ctx->motion.targetRow;
  int trgx = ctx->motion.targetColumn;
  int where = ctx->motion.where;
  int dir = ctx->motion.direction;

  if (ctx->current.row != ctx->previous.row) {
    if (ctx->previous.row != trgy) {
      if (((ctx->current.row - ctx->previous.row) * dir) > 0) {
        int dif = trgy - ctx->current.row;
        int dify = trgy - ctx->previous.row;
        if ((dif * dify) >= 0) return CRR_DONE; // Continue moving
        if (where > 0) {
          if (ctx->current.row > trgy) return CRR_NEAR;
        } else if (where < 0) {
          if (ctx->current.row < trgy) return CRR_NEAR;
        } else {
          if ((dif * dif) < (dify * dify)) return CRR_NEAR;
        }
      }
    }
  } else if (ctx->current.column != ctx->previous.column) {
    if (((ctx->current.column - ctx->previous.column) * dir) > 0) {
      int dif = trgx - ctx->current.column;
      int difx = trgx - ctx->previous.column;
      if (ctx->current.row != trgy) return CRR_DONE; // Continue
      if ((dif * difx) >= 0) return CRR_DONE; // Continue
      if (where > 0) {
        if (ctx->current.column > trgx) return CRR_NEAR;
      } else if (where < 0) {
        if (ctx->current.column < trgx) return CRR_NEAR;
      } else {
        if ((dif * dif) < (difx * difx)) return CRR_NEAR;
      }
    }
  } else {
    // Cursor didn't move at all
    return CRR_NEAR;
  }

  // Getting farther - try going back
  if (!moveCursor(ctx, ((dir > 0)? ctx->motion.axis->backward: ctx->motion.axis->forward))) {
    return CRR_FAIL;
  }

  ctx->motion.direction = -dir;
  ctx->state = RS_WAITING_FOR_MOTION;
  startMotionTimeout(ctx);

  // Schedule timeout alarm
  asyncNewRelativeAlarm(&ctx->timeoutAlarm, ctx->motion.timeout,
                        handleRoutingTimeout, NULL);

  return CRR_NEAR; // Will be handled after motion
}

static void
startCursorAdjustment(RoutingContext *ctx, int where, int trgy, int trgx,
                      const CursorAxisEntry *axis) {
  logRouting("to: [%d,%d]", trgx, trgy);

  ctx->motion.where = where;
  ctx->motion.targetRow = trgy;
  ctx->motion.targetColumn = trgx;
  ctx->motion.axis = axis;

  int dify = trgy - ctx->current.row;
  int difx = (trgx < 0)? 0: (trgx - ctx->current.column);
  int dir;

  if (dify) {
    dir = (dify > 0)? 1: -1;
  } else if (difx) {
    dir = (difx > 0)? 1: -1;
  } else {
    // Already at target
    processRoutingStateMachine();
    return;
  }

  ctx->motion.direction = dir;

  if (!moveCursor(ctx, ((dir > 0)? axis->forward: axis->backward))) {
    completeRouting(ROUTING_STATUS_FAILURE);
    return;
  }

  ctx->previous.column = ctx->current.column;
  ctx->previous.row = ctx->current.row;
  ctx->state = RS_WAITING_FOR_MOTION;

  startMotionTimeout(ctx);

  // Schedule timeout alarm
  asyncNewRelativeAlarm(&ctx->timeoutAlarm, ctx->motion.timeout,
                        handleRoutingTimeout, NULL);
}

static void
processRoutingStateMachine(void) {
  RoutingContext *ctx = &routingContext;

  switch (ctx->state) {
    case RS_IDLE:
      // Nothing to do
      break;

    case RS_INITIAL_POSITION:
      if (!getCurrentPosition(ctx)) {
        completeRouting(ROUTING_STATUS_FAILURE);
        return;
      }

      logRouting("from: [%d,%d]", ctx->current.column, ctx->current.row);

      if (ctx->target.column < 0) {
        // Only adjust vertical
        ctx->phase = RP_INITIAL_VERTICAL;
        ctx->state = RS_ADJUSTING;
        startCursorAdjustment(ctx, 0, ctx->target.row, -1,
                            &cursorAxisTable[CURSOR_AXIS_VERTICAL]);
      } else {
        // Start with vertical adjustment
        ctx->phase = RP_INITIAL_VERTICAL;
        ctx->state = RS_ADJUSTING;
        startCursorAdjustment(ctx, -1, ctx->target.row, -1,
                            &cursorAxisTable[CURSOR_AXIS_VERTICAL]);
      }
      break;

    case RS_ADJUSTING: {
      // Check if we reached the target
      int dify = ctx->motion.targetRow - ctx->current.row;
      int difx = (ctx->motion.targetColumn < 0)? 0:
                 (ctx->motion.targetColumn - ctx->current.column);

      if (!dify && !difx) {
        // Reached exact target
        if (ctx->phase == RP_INITIAL_VERTICAL && ctx->target.column >= 0) {
          // Move to horizontal adjustment
          ctx->phase = RP_INITIAL_HORIZONTAL;
          startCursorAdjustment(ctx, 0, ctx->target.row, ctx->target.column,
                              &cursorAxisTable[CURSOR_AXIS_HORIZONTAL]);
        } else {
          completeRouting(ROUTING_STATUS_SUCCEESS);
        }
        return;
      }

      // Need to continue adjusting - inject next movement
      int dir = ctx->motion.direction;
      if (!moveCursor(ctx, ((dir > 0)? ctx->motion.axis->forward:
                                       ctx->motion.axis->backward))) {
        completeRouting(ROUTING_STATUS_FAILURE);
        return;
      }

      ctx->previous.column = ctx->current.column;
      ctx->previous.row = ctx->current.row;
      ctx->state = RS_WAITING_FOR_MOTION;

      startMotionTimeout(ctx);
      asyncNewRelativeAlarm(&ctx->timeoutAlarm, ctx->motion.timeout,
                           handleRoutingTimeout, NULL);
      break;
    }

    case RS_WAITING_FOR_MOTION:
      // Should not be called in this state - wait for cursor change or timeout
      break;

    case RS_COMPLETED:
      // Already completed
      break;
  }
}

static void
completeRouting(RoutingStatus status) {
  RoutingContext *ctx = &routingContext;

  if (ctx->timeoutAlarm) {
    asyncCancelRequest(ctx->timeoutAlarm);
    ctx->timeoutAlarm = NULL;
  }

  // Check final position
  if (ctx->screen.number != ctx->target.screen) {
    status = ROUTING_STATUS_FAILURE;
  } else if (status == ROUTING_STATUS_SUCCEESS) {
    if (ctx->current.row != ctx->target.row) {
      status = ROUTING_STATUS_ROW;
    } else if ((ctx->target.column >= 0) &&
               (ctx->current.column != ctx->target.column)) {
      status = ROUTING_STATUS_COLUMN;
    }
  }

  logRouting("completed: status=%d at [%d,%d]", status,
             ctx->current.column, ctx->current.row);

  ctx->status = status;
  ctx->state = RS_COMPLETED;

  // Wake up the main loop to process the completion
  brlttyInterrupt(1);
}

static void
cleanupRoutingContext(void) {
  if (routingContext.vertical.buffer) {
    free(routingContext.vertical.buffer);
    routingContext.vertical.buffer = NULL;
  }

  if (routingContext.timeoutAlarm) {
    asyncCancelRequest(routingContext.timeoutAlarm);
    routingContext.timeoutAlarm = NULL;
  }

  routingContext.state = RS_IDLE;
}

void
onCursorPositionChanged(void) {
  RoutingContext *ctx = &routingContext;

  if (ctx->state != RS_WAITING_FOR_MOTION) return;

  int oldX = ctx->current.column;
  int oldY = ctx->current.row;

  if (!getCurrentPosition(ctx)) {
    completeRouting(ROUTING_STATUS_FAILURE);
    return;
  }

  if ((ctx->current.row == oldY) && (ctx->current.column == oldX)) {
    // Position hasn't changed yet
    return;
  }

  // Cursor moved!
  if (ctx->timeoutAlarm) {
    asyncCancelRequest(ctx->timeoutAlarm);
    ctx->timeoutAlarm = NULL;
  }

  TimeValue now;
  getMonotonicTime(&now);
  long int time = millisecondsBetween(&ctx->motion.start, &now) + 1;

  logRouting("moved: [%d,%d] -> [%d,%d] (%ldms)",
             oldX, oldY, ctx->current.column, ctx->current.row, time);

  if (!ctx->motion.hasMoved) {
    ctx->motion.hasMoved = 1;
    ctx->motion.timeout = (time * 2) + 1;

    ctx->time.sum += time * 8;
    ctx->time.count += 1;
  }

  handleVerticalScrolling(ctx, ctx->motion.direction);

  // Evaluate if we should continue or if we've reached near enough
  RoutingResult result = evaluateCursorMotion(ctx);

  if (result == CRR_NEAR) {
    // Check phase and decide next action
    if (ctx->phase == RP_INITIAL_HORIZONTAL && ctx->current.row < ctx->target.row) {
      // Try moving down one more row and retry horizontal
      ctx->phase = RP_RETRY_VERTICAL;
      ctx->state = RS_ADJUSTING;
      startCursorAdjustment(ctx, 1, ctx->current.row + 1, -1,
                          &cursorAxisTable[CURSOR_AXIS_VERTICAL]);
      return;
    } else if (ctx->phase == RP_RETRY_VERTICAL) {
      // Try horizontal again
      ctx->phase = RP_RETRY_HORIZONTAL_FINAL;
      ctx->state = RS_ADJUSTING;
      startCursorAdjustment(ctx, 0, ctx->target.row, ctx->target.column,
                          &cursorAxisTable[CURSOR_AXIS_HORIZONTAL]);
      return;
    }

    // Can't get closer
    if (ctx->current.row != ctx->target.row) {
      completeRouting(ROUTING_STATUS_ROW);
    } else {
      completeRouting(ROUTING_STATUS_COLUMN);
    }
  } else if (result == CRR_FAIL) {
    completeRouting(ROUTING_STATUS_FAILURE);
  } else {
    // CRR_DONE - continue adjusting
    ctx->state = RS_ADJUSTING;
    processRoutingStateMachine();
  }
}

int
startRouting(int column, int row, int screen) {
  // Cancel any existing routing
  if (routingContext.state != RS_IDLE) {
    cleanupRoutingContext();
  }

  // Initialize context
  routingContext.state = RS_INITIAL_POSITION;
  routingContext.phase = RP_INITIAL_VERTICAL;
  routingContext.target.column = column;
  routingContext.target.row = row;
  routingContext.target.screen = screen;
  routingContext.screen.number = screen;
  routingContext.vertical.buffer = NULL;
  routingContext.time.sum = ROUTING_MAXIMUM_TIMEOUT;
  routingContext.time.count = 1;
  routingContext.status = ROUTING_STATUS_NONE;
  routingContext.timeoutAlarm = NULL;

  logRouting("start: target=[%d,%d] screen=%d", column, row, screen);

  // Start the state machine
  processRoutingStateMachine();

  return 1;
}

int
isRouting(void) {
  return routingContext.state != RS_IDLE &&
         routingContext.state != RS_COMPLETED;
}

RoutingStatus
getRoutingStatus(int wait) {
  if (routingContext.state == RS_COMPLETED) {
    RoutingStatus status = routingContext.status;
    cleanupRoutingContext();
    return status;
  }

  return ROUTING_STATUS_NONE;
}
