/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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

#include "log.h"
#include "report.h"
#include "parameters.h"
#include "cmd_queue.h"
#include "cmd_input.h"
#include "cmd_utils.h"
#include "brl_cmds.h"
#include "unicode.h"
#include "ttb.h"
#include "scr.h"
#include "async_alarm.h"
#include "brltty.h"

typedef struct {
  ReportListenerInstance *resetListener;

  struct {
    int persistent;
    int temporary;
    AsyncHandle timeout;
  } modifiers;
} InputCommandData;

static void
initializeModifiersTimeout (InputCommandData *icd) {
  icd->modifiers.timeout = NULL;
}

static void
cancelModifiersTimeout (InputCommandData *icd) {
  if (icd->modifiers.timeout) {
    asyncCancelRequest(icd->modifiers.timeout);
    initializeModifiersTimeout(icd);
  }
}

static void
clearModifiers (InputCommandData *icd) {
  icd->modifiers.persistent = 0;
  icd->modifiers.temporary = 0;
  alert(ALERT_TOGGLE_OFF);
}

ASYNC_ALARM_CALLBACK(handleInputModifiersTimeout) {
  InputCommandData *icd = parameters->data;

  asyncDiscardHandle(icd->modifiers.timeout);
  initializeModifiersTimeout(icd);

  clearModifiers(icd);
}

static void
applyModifiers (InputCommandData *icd, int *modifiers) {
  *modifiers |= icd->modifiers.persistent;
  *modifiers |= icd->modifiers.temporary;
  icd->modifiers.temporary = 0;
  cancelModifiersTimeout(icd);
}

static int
insertKey (ScreenKey key, int flags) {
  if (flags & BRL_FLG_CHAR_SHIFT) key |= SCR_KEY_SHIFT;
  if (flags & BRL_FLG_CHAR_UPPER) key |= SCR_KEY_UPPER;
  if (flags & BRL_FLG_CHAR_CONTROL) key |= SCR_KEY_CONTROL;
  if (flags & BRL_FLG_CHAR_META) key |= SCR_KEY_ALT_LEFT;
  if (flags & BRL_FLG_CHAR_ALTGR) key |= SCR_KEY_ALT_RIGHT;
  if (flags & BRL_FLG_CHAR_GUI) key |= SCR_KEY_GUI;
  return insertScreenKey(key);
}

static int
switchVirtualTerminal (int vt) {
  int switched = switchScreenVirtualTerminal(vt);

  if (switched) {
    updateSessionAttributes();
  } else {
    alert(ALERT_COMMAND_REJECTED);
  }

  return switched;
}

static int
handleInputCommands (int command, void *data) {
  InputCommandData *icd = data;

  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_NOMODS:
      if (icd->modifiers.temporary) {
        cancelModifiersTimeout(icd);
        clearModifiers(icd);
      } else {
        alert(ALERT_COMMAND_REJECTED);
      }
      break;

    {
      int modifier;

    case BRL_CMD_SHIFT:
      modifier = BRL_FLG_CHAR_SHIFT;
      goto doModifier;

    case BRL_CMD_UPPER:
      modifier = BRL_FLG_CHAR_UPPER;
      goto doModifier;

    case BRL_CMD_CONTROL:
      modifier = BRL_FLG_CHAR_CONTROL;
      goto doModifier;

    case BRL_CMD_META:
      modifier = BRL_FLG_CHAR_META;
      goto doModifier;

    case BRL_CMD_ALTGR:
      modifier = BRL_FLG_CHAR_ALTGR;
      goto doModifier;

    case BRL_CMD_GUI:
      modifier = BRL_FLG_CHAR_GUI;
      goto doModifier;

    doModifier:
      cancelModifiersTimeout(icd);

      if (icd->modifiers.persistent & modifier) {
        icd->modifiers.persistent &= ~modifier;
        icd->modifiers.temporary &= ~modifier;
        alert(ALERT_TOGGLE_OFF);
      } else if (icd->modifiers.temporary & modifier) {
        icd->modifiers.persistent |= modifier;
        alert(ALERT_TOGGLE_ON);
      } else {
        icd->modifiers.temporary |= modifier;
        alert(ALERT_TOGGLE_ON);
      }

      if (icd->modifiers.temporary != icd->modifiers.persistent) {
        asyncSetAlarmIn(&icd->modifiers.timeout,
                        INPUT_TEMPORARY_MODIFIERS_TIMEOUT,
                        handleInputModifiersTimeout, icd);
      }

      break;
    }

    case BRL_CMD_SWITCHVT_PREV:
      switchVirtualTerminal(scr.number-1);
      break;

    case BRL_CMD_SWITCHVT_NEXT:
      switchVirtualTerminal(scr.number+1);
      break;

    default: {
      int arg = command & BRL_MSK_ARG;
      int flags = command & BRL_MSK_FLG;

      switch (command & BRL_MSK_BLK) {
        case BRL_CMD_BLK(PASSKEY): {
          ScreenKey key;

          switch (arg) {
            case BRL_KEY_ENTER:
              key = SCR_KEY_ENTER;
              break;
            case BRL_KEY_TAB:
              key = SCR_KEY_TAB;
              break;
            case BRL_KEY_BACKSPACE:
              key = SCR_KEY_BACKSPACE;
              break;
            case BRL_KEY_ESCAPE:
              key = SCR_KEY_ESCAPE;
              break;
            case BRL_KEY_CURSOR_LEFT:
              key = SCR_KEY_CURSOR_LEFT;
              break;
            case BRL_KEY_CURSOR_RIGHT:
              key = SCR_KEY_CURSOR_RIGHT;
              break;
            case BRL_KEY_CURSOR_UP:
              key = SCR_KEY_CURSOR_UP;
              break;
            case BRL_KEY_CURSOR_DOWN:
              key = SCR_KEY_CURSOR_DOWN;
              break;
            case BRL_KEY_PAGE_UP:
              key = SCR_KEY_PAGE_UP;
              break;
            case BRL_KEY_PAGE_DOWN:
              key = SCR_KEY_PAGE_DOWN;
              break;
            case BRL_KEY_HOME:
              key = SCR_KEY_HOME;
              break;
            case BRL_KEY_END:
              key = SCR_KEY_END;
              break;
            case BRL_KEY_INSERT:
              key = SCR_KEY_INSERT;
              break;
            case BRL_KEY_DELETE:
              key = SCR_KEY_DELETE;
              break;
            default:
              if (arg < BRL_KEY_FUNCTION) goto badKey;
              key = SCR_KEY_FUNCTION + (arg - BRL_KEY_FUNCTION);
              break;
          }

          applyModifiers(icd, &flags);
          if (!insertKey(key, flags)) {
          badKey:
            alert(ALERT_COMMAND_REJECTED);
          }
          break;
        }

        case BRL_CMD_BLK(PASSCHAR): {
          applyModifiers(icd, &flags);
          if (!insertKey(BRL_ARG_GET(command), flags)) alert(ALERT_COMMAND_REJECTED);
          break;
        }

        case BRL_CMD_BLK(PASSDOTS): {
          wchar_t character;

          switch (prefs.brailleInputMode) {
            case BRL_INPUT_TEXT:
              character = convertDotsToCharacter(textTable, arg);
              break;

            case BRL_INPUT_DOTS:
              character = UNICODE_BRAILLE_ROW | arg;
              break;

            default:
              character = UNICODE_REPLACEMENT_CHARACTER;
              break;
          }

          applyModifiers(icd, &flags);
          if (!insertKey(character, flags)) alert(ALERT_COMMAND_REJECTED);
          break;
        }

        case BRL_CMD_BLK(SWITCHVT):
          switchVirtualTerminal(arg+1);
          break;

        default:
          return 0;
      }

      break;
    }
  }

  return 1;
}

static void
initializeInputCommandData (InputCommandData *icd) {
  icd->modifiers.persistent = 0;
  icd->modifiers.temporary = 0;
  initializeModifiersTimeout(icd);
}

static void
resetInputCommandData (InputCommandData *icd) {
  cancelModifiersTimeout(icd);
  initializeInputCommandData(icd);
}

REPORT_LISTENER(inputCommandDataResetListener) {
  InputCommandData *icd = parameters->listenerData;

  resetInputCommandData(icd);
}

static void
destroyInputCommandData (void *data) {
  InputCommandData *icd = data;

  unregisterReportListener(icd->resetListener);
  free(icd);
}

int
addInputCommands (void) {
  InputCommandData *icd;

  if ((icd = malloc(sizeof(*icd)))) {
    memset(icd, 0, sizeof(*icd));
    initializeInputCommandData(icd);

    if ((icd->resetListener = registerReportListener(REPORT_BRAILLE_ONLINE, inputCommandDataResetListener, icd))) {
      if (pushCommandHandler("input", KTB_CTX_DEFAULT,
                             handleInputCommands, destroyInputCommandData, icd)) {
        return 1;
      }

      unregisterReportListener(icd->resetListener);
    }

    free(icd);
  } else {
    logMallocError();
  }

  return 0;
}
