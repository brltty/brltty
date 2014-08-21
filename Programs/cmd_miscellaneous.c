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

#include <stdio.h>

#include "cmd_queue.h"
#include "cmd_miscellaneous.h"
#include "cmd_utils.h"
#include "brl_cmds.h"
#include "prefs.h"
#include "scr.h"
#include "message.h"
#include "alert.h"
#include "brltty.h"

#ifdef ENABLE_SPEECH_SUPPORT
static size_t
formatSpeechDate (char *buffer, size_t size, const TimeFormattingData *fmt) {
  size_t length;

  const char *yearFormat = "%u";
  const char *monthFormat = "%s";
  const char *dayFormat = "%u";

  uint16_t year = fmt->components.year;
  const char *month;
  uint8_t day = fmt->components.day + 1;

#ifdef MON_1
  {
    static const int months[] = {
      MON_1, MON_2, MON_3, MON_4, MON_5, MON_6,
      MON_7, MON_8, MON_9, MON_10, MON_11, MON_12
    };

    month = (fmt->components.month < ARRAY_COUNT(months))? nl_langinfo(months[fmt->components.month]): "?";
  }
#else /* MON_1 */
  {
    static const char *const months[] = {
      strtext("January"),
      strtext("February"),
      strtext("March"),
      strtext("April"),
      strtext("May"),
      strtext("June"),
      strtext("July"),
      strtext("August"),
      strtext("September"),
      strtext("October"),
      strtext("November"),
      strtext("December")
    };

    month = (fmt->components.month < ARRAY_COUNT(months))? gettext(months[fmt->components.month]): "?";
  }
#endif /* MON_1 */

  STR_BEGIN(buffer, size);

  switch (prefs.dateFormat) {
    default:
    case dfYearMonthDay:
      STR_PRINTF(yearFormat, year);
      STR_PRINTF(" ");
      STR_PRINTF(monthFormat, month);
      STR_PRINTF(" ");
      STR_PRINTF(dayFormat, day);
      break;

    case dfMonthDayYear:
      STR_PRINTF(monthFormat, month);
      STR_PRINTF(" ");
      STR_PRINTF(dayFormat, day);
      STR_PRINTF(", ");
      STR_PRINTF(yearFormat, year);
      break;

    case dfDayMonthYear:
      STR_PRINTF(dayFormat, day);
      STR_PRINTF(" ");
      STR_PRINTF(monthFormat, month);
      STR_PRINTF(", ");
      STR_PRINTF(yearFormat, year);
      break;
  }

  length = STR_LENGTH;
  STR_END

  return length;
}

static size_t
formatSpeechTime (char *buffer, size_t size, const TimeFormattingData *fmt) {
  size_t length;

  STR_BEGIN(buffer, size);
  STR_PRINTF("%u", fmt->components.hour);
  if (fmt->components.minute < 10) STR_PRINTF(" 0");
  STR_PRINTF(" %u", fmt->components.minute);

  if (fmt->meridian) {
    const char *character = fmt->meridian;
    while (*character) STR_PRINTF(" %c", *character++);
  }

  if (prefs.showSeconds) {
    STR_PRINTF(", ");

    if (fmt->components.second == 0) {
      STR_PRINTF("%s", gettext("exactly"));
    } else {
      STR_PRINTF("%s %u %s",
                 gettext("and"), fmt->components.second,
                 ngettext("second", "seconds", fmt->components.second));
    }
  }

  length = STR_LENGTH;
  STR_END

  return length;
}

static void
doSpeechTime (const TimeFormattingData *fmt) {
  char announcement[0X100];
  char time[0X80];

  STR_BEGIN(announcement, sizeof(announcement));
  formatSpeechTime(time, sizeof(time), fmt);

  if (prefs.datePosition == dpNone) {
    STR_PRINTF("%s", time);
  } else {
    char date[0X40];
    formatSpeechDate(date, sizeof(date), fmt);

    switch (prefs.datePosition) {
      case dpBeforeTime:
        STR_PRINTF("%s, %s", date, time);
        break;

      case dpAfterTime:
        STR_PRINTF("%s, %s", time, date);
        break;

      default:
        STR_PRINTF("%s", date);
        break;
    }
  }

  STR_PRINTF(".");
  STR_END
  sayString(announcement, 1);
}
#endif /* ENABLE_SPEECH_SUPPORT */

static void
doBrailleTime (const TimeFormattingData *fmt) {
  char buffer[0X80];

  formatBrailleTime(buffer, sizeof(buffer), fmt);
  message(NULL, buffer, MSG_SILENT);
}

static int
handleMiscellaneousCommands (int command, void *data) {
  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_RESTARTBRL:
      restartRequired = 1;
      break;

    case BRL_CMD_HELP: {
      int ok = 0;
      unsigned int pageNumber;

      if (isHelpScreen()) {
        pageNumber = getHelpPageNumber() + 1;
        ok = 1;
      } else {
        pageNumber = haveHelpScreen()? getHelpPageNumber(): 1;
        if (!activateHelpScreen()) pageNumber = 0;
      }

      if (pageNumber) {
        unsigned int pageCount = getHelpPageCount();

        while (pageNumber <= pageCount) {
          if (setHelpPageNumber(pageNumber))
            if (getHelpLineCount())
              break;

          pageNumber += 1;
        }

        if (pageNumber > pageCount) {
          deactivateHelpScreen();
          updateSessionAttributes();
        } else {
          updateSessionAttributes();
          ok = 1;
        }
      }

      if (ok) {
        infoMode = 0;
      } else {
        message(NULL, gettext("help not available"), 0);
      }

      break;
    }

    {
      static const char modeString_preferences[] = "prf";
      static Preferences savedPreferences;

    case BRL_CMD_PREFMENU: {
      int ok = 0;

      if (isMenuScreen()) {
        if (prefs.saveOnExit) {
          if (savePreferences()) {
            alert(ALERT_COMMAND_DONE);
          }
        }

        deactivateMenuScreen();
        ok = 1;
      } else if (activateMenuScreen()) {
        updateSessionAttributes();
        ses->hideCursor = 1;
        savedPreferences = prefs;
        ok = 1;
      }

      if (ok) {
        infoMode = 0;
      } else {
        alert(ALERT_COMMAND_REJECTED);
      }

      break;
    }

    case BRL_CMD_PREFSAVE:
      if (isMenuScreen()) {
        if (savePreferences()) alert(ALERT_COMMAND_DONE);
        deactivateMenuScreen();
      } else if (savePreferences()) {
        alert(ALERT_COMMAND_DONE);
      } else {
        alert(ALERT_COMMAND_REJECTED);
      }
      break;

    case BRL_CMD_PREFLOAD:
      if (isMenuScreen()) {
        setPreferences(&savedPreferences);
        message(modeString_preferences, gettext("changes discarded"), 0);
      } else if (loadPreferences()) {
        alert(ALERT_COMMAND_DONE);
      } else {
        alert(ALERT_COMMAND_REJECTED);
      }
      break;
    }

    case BRL_CMD_TIME: {
      TimeFormattingData fmt;
      getTimeFormattingData(&fmt);

#ifdef ENABLE_SPEECH_SUPPORT
      if (autospeak()) doSpeechTime(&fmt);
#endif /* ENABLE_SPEECH_SUPPORT */

      doBrailleTime(&fmt);
      break;
    }

    default: {
      int arg = command & BRL_MSK_ARG;

      switch (command & BRL_MSK_BLK) {
        case BRL_BLK_DESCCHAR: {
          int column, row;

          if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
            char description[0X50];
            formatCharacterDescription(description, sizeof(description), column, row);
            message(NULL, description, 0);
          } else {
            alert(ALERT_COMMAND_REJECTED);
          }

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

int
addMiscellaneousCommands (void) {
  return pushCommandHandler("miscellaneous", KTB_CTX_DEFAULT,
                            handleMiscellaneousCommands, NULL, NULL);
}
