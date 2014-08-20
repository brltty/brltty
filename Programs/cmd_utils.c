/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#include "parameters.h"
#include "cmd_utils.h"
#include "brl_cmds.h"
#include "async_wait.h"
//#include "scr.h"
#include "brltty.h"

ToggleResult
toggleBit (
  int *bits, int bit, int command,
  AlertIdentifier offAlert,
  AlertIdentifier onAlert
) {
  int oldBits = *bits;

  switch (command & BRL_FLG_TOGGLE_MASK) {
    case 0:
      *bits ^= bit;
      break;

    case BRL_FLG_TOGGLE_ON:
      *bits |= bit;
      break;

    case BRL_FLG_TOGGLE_OFF:
      *bits &= ~bit;
      break;

    default:
      alert(ALERT_COMMAND_REJECTED);
      return TOGGLE_ERROR;
  }

  {
    int isOn = (*bits & bit) != 0;
    AlertIdentifier identifier = isOn? onAlert: offAlert;

    alert(identifier);
    if (*bits != oldBits) return isOn? TOGGLE_ON: TOGGLE_OFF;

    asyncWait(TUNE_TOGGLE_REPEAT_DELAY);
    alert(identifier);
    return TOGGLE_SAME;
  }
}

int
isTextOffset (int *arg, int end, int relaxed) {
  int value = *arg;

  if (value < textStart) return 0;
  if ((value -= textStart) >= textCount) return 0;

  if ((ses->winx + value) >= scr.cols) {
    if (!relaxed) return 0;
    value = scr.cols - 1 - ses->winx;
  }

#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracted) {
    int result = 0;
    int index;

    for (index=0; index<contractedLength; index+=1) {
      int offset = contractedOffsets[index];

      if (offset != CTB_NO_OFFSET) {
        if (offset > value) {
          if (end) result = index - 1;
          break;
        }

        result = index;
      }
    }
    if (end && (index == contractedLength)) result = contractedLength - 1;

    value = result;
  }
#endif /* ENABLE_CONTRACTED_BRAILLE */

  *arg = value;
  return 1;
}

int
getCharacterCoordinates (int arg, int *column, int *row, int end, int relaxed) {
  if (arg == BRL_MSK_ARG) {
    if (!SCR_CURSOR_OK()) return 0;
    *column = scr.posx;
    *row = scr.posy;
  } else {
    if (!isTextOffset(&arg, end, relaxed)) return 0;
    *column = ses->winx + arg;
    *row = ses->winy;
  }
  return 1;
}
