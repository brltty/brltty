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

#include "report.h"
#include "cmd_input.h"
#include "brl_cmds.h"
#include "unicode.h"
#include "ttb.h"
#include "scr.h"
#include "brltty.h"

static int inputModifiers;

static void
applyInputModifiers (int *modifiers) {
  *modifiers |= inputModifiers;
  inputModifiers = 0;
}

static int
insertKey (ScreenKey key, int flags) {
  if (flags & BRL_FLG_CHAR_SHIFT) key |= SCR_KEY_SHIFT;
  if (flags & BRL_FLG_CHAR_UPPER) key |= SCR_KEY_UPPER;
  if (flags & BRL_FLG_CHAR_CONTROL) key |= SCR_KEY_CONTROL;
  if (flags & BRL_FLG_CHAR_META) key |= SCR_KEY_ALT_LEFT;
  return insertScreenKey(key);
}

int
handleInputCommand (int command, void *data) {
  switch (command & BRL_MSK_CMD) {
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

    doModifier:
      toggleBit(&inputModifiers, modifier, command, ALERT_TOGGLE_OFF, ALERT_TOGGLE_ON);
      break;
    }

    default: {
      int arg = command & BRL_MSK_ARG;
      int flags = command & BRL_MSK_FLG;

      switch (command & BRL_MSK_BLK) {
        case BRL_BLK_PASSKEY: {
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

          applyInputModifiers(&flags);
          if (!insertKey(key, flags)) {
          badKey:
            alert(ALERT_COMMAND_REJECTED);
          }
          break;
        }

        case BRL_BLK_PASSCHAR: {
          applyInputModifiers(&flags);
          if (!insertKey(BRL_ARG_GET(command), flags)) alert(ALERT_COMMAND_REJECTED);
          break;
        }

        case BRL_BLK_PASSDOTS: {
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

          applyInputModifiers(&flags);
          if (!insertKey(character, flags)) alert(ALERT_COMMAND_REJECTED);
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
resetInputVariables (void) {
  inputModifiers = 0;
}

REPORT_LISTENER(inputCommandsResetListener) {
  resetInputVariables();
}

void
initializeInputCommandHandling (void) {
  resetInputVariables();
  registerReportListener(REPORT_BRAILLE_ON, inputCommandsResetListener);
}
