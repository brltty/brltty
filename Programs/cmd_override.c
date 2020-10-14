/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
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

#include <string.h>

#include "log.h"
#include "alert.h"
#include "cmd_queue.h"
#include "cmd_override.h"
#include "cmd_utils.h"
#include "brl_cmds.h"
#include "scr.h"
#include "prefs.h"
#include "core.h"

typedef struct {
  struct {
    struct {
      int column;
      int row;
    } start;

    struct {
      int column;
      int row;
    } end;

    unsigned char started:1;
  } selection;
} OverrideCommandData;

static int
setSelection (int startColumn, int startRow, int endColumn, int endRow, OverrideCommandData *ocd) {
  int ok = setScreenTextSelection(startColumn, startRow, endColumn, endRow);

  if (ok) {
    ocd->selection.start.column = startColumn;
    ocd->selection.start.row = startRow;

    ocd->selection.end.column = endColumn;
    ocd->selection.end.row = endRow;
  }

  return ok;
}

static int
startSelection (int column, int row, OverrideCommandData *ocd) {
  return setSelection(column, row, column, row, ocd);
}

static int
handleOverrideCommands (int command, void *data) {
  OverrideCommandData *ocd = data;
  if (!scr.hasSelection) ocd->selection.started = 0;

  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_TXTSEL_CLEAR: {
      if (clearScreenTextSelection()) {
        ocd->selection.started = 0;
      } else {
        alert(ALERT_COMMAND_REJECTED);
      }

      break;
    }

    default: {
      int blk = command & BRL_MSK_BLK;
      int arg = command & BRL_MSK_ARG;
      int ext = BRL_CODE_GET(EXT, command);

      switch (blk) {
        case BRL_CMD_BLK(TXTSEL_SET): {
          if (ext > arg) {
            int startColumn, startRow;

            if (getCharacterCoordinates(arg, &startColumn, &startRow, 0, 0)) {
              int endColumn, endRow;

              if (getCharacterCoordinates(ext, &endColumn, &endRow, 1, 1)) {
                int ok = setSelection(
                  startColumn, startRow, endColumn, endRow, ocd
                );

                if (ok) {
                  ocd->selection.started = 0;
                  break;
                }
              }
            }
          }

          alert(ALERT_COMMAND_REJECTED);
          break;
        }

        case BRL_CMD_BLK(TXTSEL_START): {
          int column, row;

          if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
            if (startSelection(column, row, ocd)) {
              ocd->selection.started = 1;
              break;
            }
          }

          alert(ALERT_COMMAND_REJECTED);
          break;
        }

        case BRL_CMD_BLK(ROUTE): {
          if (!ocd->selection.started && !prefs.startTextSelection) return 0;
          int column, row;

          if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
            if (ocd->selection.started) {;
              int ok = setSelection(
                ocd->selection.start.column, ocd->selection.start.row,
                column, row, ocd
              );

              if (ok) break;
            } else if ((column != scr.posx) || (row != scr.posy)) {
              return 0;
            } else if (startSelection(column, row, ocd)) {
              ocd->selection.started = 1;
              break;
            }
          }

          alert(ALERT_COMMAND_REJECTED);
          break;
        }

        default:
          return 0;
      }

      break;
    }
  }

  return 1;
}

static void
destroyOverrideCommandData (void *data) {
  OverrideCommandData *ocd = data;
  free(ocd);
}

int
addOverrideCommands (void) {
  OverrideCommandData *ocd;

  if ((ocd = malloc(sizeof(*ocd)))) {
    memset(ocd, 0, sizeof(*ocd));

    ocd->selection.started = 0;

    if (pushCommandHandler("override", KTB_CTX_DEFAULT,
                           handleOverrideCommands, destroyOverrideCommandData, ocd)) {
      return 1;
    }

    free(ocd);
  } else {
    logMallocError();
  }

  return 0;
}
