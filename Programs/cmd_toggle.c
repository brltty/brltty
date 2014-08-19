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

#include "cmd_toggle.h"
#include "brl_cmds.h"
#include "prefs.h"
#include "scr.h"
#include "brltty.h"

static ToggleResult
toggleSetting (
  unsigned char *setting, int command,
  AlertIdentifier offAlert,
  AlertIdentifier onAlert
) {
  const int bit = 1;
  int bits = *setting? bit: 0;
  ToggleResult result = toggleBit(&bits, bit, command, offAlert, onAlert);

  *setting = (bits & bit)? bit: 0;
  return result;
}

static ToggleResult
toggleFeatureSetting (unsigned char *setting, int command) {
  return toggleSetting(setting, command, ALERT_TOGGLE_OFF, ALERT_TOGGLE_ON);
}

static ToggleResult
toggleModeSetting (unsigned char *setting, int command) {
  return toggleSetting(setting, command, ALERT_NONE, ALERT_NONE);
}

int
handleToggleCommand (int command, void *data) {
  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_SKPIDLNS:
      toggleFeatureSetting(&prefs.skipIdenticalLines, command);
      break;

    case BRL_CMD_SKPBLNKWINS:
      toggleFeatureSetting(&prefs.skipBlankWindows, command);
      break;

    case BRL_CMD_SLIDEWIN:
      toggleFeatureSetting(&prefs.slidingWindow, command);
      break;

    case BRL_CMD_SIXDOTS:
      toggleFeatureSetting(&prefs.textStyle, command);
      break;

    case BRL_CMD_CSRSIZE:
      toggleFeatureSetting(&prefs.cursorStyle, command);
      break;

    case BRL_CMD_CSRVIS:
      toggleFeatureSetting(&prefs.showCursor, command);
      break;

    case BRL_CMD_CSRBLINK:
      toggleFeatureSetting(&prefs.blinkingCursor, command);
      break;

    case BRL_CMD_ATTRVIS:
      toggleFeatureSetting(&prefs.showAttributes, command);
      break;

    case BRL_CMD_ATTRBLINK:
      toggleFeatureSetting(&prefs.blinkingAttributes, command);
      break;

    case BRL_CMD_CAPBLINK:
      toggleFeatureSetting(&prefs.blinkingCapitals, command);
      break;

    case BRL_CMD_AUTOREPEAT:
      toggleFeatureSetting(&prefs.autorepeat, command);
      break;

    case BRL_CMD_BRLUCDOTS:
      toggleFeatureSetting(&prefs.brailleInputMode, command);
      break;

    case BRL_CMD_TUNES:
      toggleFeatureSetting(&prefs.alertTunes, command);        /* toggle sound on/off */
      break;

    case BRL_CMD_AUTOSPEAK:
      toggleFeatureSetting(&prefs.autospeak, command);
      break;

    case BRL_CMD_ASPK_SEL_LINE:
      toggleFeatureSetting(&prefs.autospeakSelectedLine, command);
      break;

    case BRL_CMD_ASPK_SEL_CHAR:
      toggleFeatureSetting(&prefs.autospeakSelectedCharacter, command);
      break;

    case BRL_CMD_ASPK_INS_CHARS:
      toggleFeatureSetting(&prefs.autospeakInsertedCharacters, command);
      break;

    case BRL_CMD_ASPK_DEL_CHARS:
      toggleFeatureSetting(&prefs.autospeakDeletedCharacters, command);
      break;

    case BRL_CMD_ASPK_REP_CHARS:
      toggleFeatureSetting(&prefs.autospeakReplacedCharacters, command);
      break;

    case BRL_CMD_ASPK_CMP_WORDS:
      toggleFeatureSetting(&prefs.autospeakCompletedWords, command);
      break;

    case BRL_CMD_SHOW_CURR_LOCN:
      toggleFeatureSetting(&prefs.showSpeechCursor, command);
      break;

    case BRL_CMD_INFO:
      if ((prefs.statusPosition == spNone) || haveStatusCells()) {
        toggleModeSetting(&infoMode, command);
      } else {
        ToggleResult result = toggleModeSetting(&textMaximized, command);

        if (result > TOGGLE_SAME) reconfigureWindow();
      }
      break;

    case BRL_CMD_DISPMD:
      toggleModeSetting(&ses->displayMode, command);
      break;

    case BRL_CMD_CSRHIDE:
      toggleModeSetting(&ses->hideCursor, command);
      break;

    case BRL_CMD_CSRTRK:
      toggleSetting(&ses->trackCursor, command, ALERT_CURSOR_UNLINKED, ALERT_CURSOR_LINKED);

      if (ses->trackCursor) {
#ifdef ENABLE_SPEECH_SUPPORT
        if (spk.track.isActive && (scr.number == spk.track.screenNumber)) {
          spk.track.speechLocation = SPK_LOC_NONE;
        } else
#endif /* ENABLE_SPEECH_SUPPORT */

        {
          trackCursor(1);
        }
      }
      break;

    case BRL_CMD_FREEZE: {
      unsigned char setting;

      if (isMainScreen()) {
        setting = 0;
      } else if (isFrozenScreen()) {
        setting = 1;
      } else {
        alert(ALERT_COMMAND_REJECTED);
        break;
      }

      switch (toggleSetting(&setting, command, ALERT_SCREEN_UNFROZEN, ALERT_SCREEN_FROZEN)) {
        case TOGGLE_OFF:
          deactivateFrozenScreen();
          break;

        case TOGGLE_ON:
          if (!activateFrozenScreen()) alert(ALERT_COMMAND_REJECTED);
          break;

        default:
          break;
      }

      break;
    }

    default:
      return 0;
  }

  return 1;
}
