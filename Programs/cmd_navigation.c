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

#include "log.h"
#include "parameters.h"
#include "cmd_queue.h"
#include "cmd_navigation.h"
#include "cmd_utils.h"
#include "cmd_clipboard.h"
#include "parse.h"
#include "prefs.h"
#include "alert.h"
#include "routing.h"
#include "message.h"
#include "brl_cmds.h"
#include "scr.h"
#include "brltty.h"

static int
getWindowLength (void) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracting()) return getContractedLength(textCount);
#endif /* ENABLE_CONTRACTED_BRAILLE */

  return textCount;
}

typedef int (*CanMoveWindow) (void);

static int
canMoveUp (void) {
  return ses->winy > 0;
}

static int
canMoveDown (void) {
  return ses->winy < (int)(scr.rows - brl.textRows);
}

static int
toDifferentLine (
  IsSameCharacter isSameCharacter,
  CanMoveWindow canMoveWindow,
  int amount, int from, int width
) {
  if (canMoveWindow()) {
    ScreenCharacter characters1[width];
    int skipped = 0;

    if ((isSameCharacter == isSameText) && ses->displayMode) isSameCharacter = isSameAttributes;
    readScreen(from, ses->winy, width, 1, characters1);

    do {
      ScreenCharacter characters2[width];
      readScreen(from, ses->winy+=amount, width, 1, characters2);

      if (!isSameRow(characters1, characters2, width, isSameCharacter) ||
          (showCursor() && (scr.posy == ses->winy) &&
           (scr.posx >= from) && (scr.posx < (from + width))))
        return 1;

      /* lines are identical */
      if (skipped == 0) {
        alert(ALERT_SKIP_FIRST);
      } else if (skipped <= 4) {
        alert(ALERT_SKIP);
      } else if (skipped % 4 == 0) {
        alert(ALERT_SKIP_MORE);
      }
      skipped++;
    } while (canMoveWindow());
  }

  alert(ALERT_BOUNCE);
  return 0;
}

static int
upDifferentLine (IsSameCharacter isSameCharacter) {
  return toDifferentLine(isSameCharacter, canMoveUp, -1, 0, scr.cols);
}

static int
downDifferentLine (IsSameCharacter isSameCharacter) {
  return toDifferentLine(isSameCharacter, canMoveDown, 1, 0, scr.cols);
}

static int
upDifferentCharacter (IsSameCharacter isSameCharacter, int column) {
  return toDifferentLine(isSameCharacter, canMoveUp, -1, column, 1);
}

static int
downDifferentCharacter (IsSameCharacter isSameCharacter, int column) {
  return toDifferentLine(isSameCharacter, canMoveDown, 1, column, 1);
}

static void
upOneLine (void) {
  if (canMoveUp()) {
    ses->winy--;
  } else {
    alert(ALERT_BOUNCE);
  }
}

static void
downOneLine (void) {
  if (canMoveDown()) {
    ses->winy++;
  } else {
    alert(ALERT_BOUNCE);
  }
}

static void
upLine (IsSameCharacter isSameCharacter) {
  if (prefs.skipIdenticalLines) {
    upDifferentLine(isSameCharacter);
  } else {
    upOneLine();
  }
}

static void
downLine (IsSameCharacter isSameCharacter) {
  if (prefs.skipIdenticalLines) {
    downDifferentLine(isSameCharacter);
  } else {
    downOneLine();
  }
}

typedef int (*RowTester) (int column, int row, void *data);

static void
findRow (int column, int increment, RowTester test, void *data) {
  int row = ses->winy + increment;
  while ((row >= 0) && (row <= scr.rows-(int)brl.textRows)) {
    if (test(column, row, data)) {
      ses->winy = row;
      return;
    }
    row += increment;
  }
  alert(ALERT_BOUNCE);
}

static int
testIndent (int column, int row, void *data UNUSED) {
  int count = column+1;
  ScreenCharacter characters[count];
  readScreen(0, row, count, 1, characters);
  while (column >= 0) {
    wchar_t text = characters[column].text;
    if (text != WC_C(' ')) return 1;
    --column;
  }
  return 0;
}

static int
testPrompt (int column, int row, void *data) {
  const ScreenCharacter *prompt = data;
  int count = column+1;
  ScreenCharacter characters[count];
  readScreen(0, row, count, 1, characters);
  return isSameRow(characters, prompt, count, isSameText);
}

static int
findCharacters (const wchar_t **address, size_t *length, const wchar_t *characters, size_t count) {
  const wchar_t *ptr = *address;
  size_t len = *length;

  while (count <= len) {
    const wchar_t *next = wmemchr(ptr, *characters, len);
    if (!next) break;

    len -= next - ptr;
    if (wmemcmp((ptr = next), characters, count) == 0) {
      *address = ptr;
      *length = len;
      return 1;
    }

    ++ptr, --len;
  }

  return 0;
}

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
handleNavigationCommands (int command, void *data) {
  static const char modeString_preferences[] = "prf";
  static Preferences savedPreferences;

  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_TOP_LEFT:
      ses->winx = 0;
    case BRL_CMD_TOP:
      ses->winy = 0;
      break;
    case BRL_CMD_BOT_LEFT:
      ses->winx = 0;
    case BRL_CMD_BOT:
      ses->winy = scr.rows - brl.textRows;
      break;

    case BRL_CMD_WINUP:
      if (canMoveUp()) {
        ses->winy -= MIN(verticalWindowShift, ses->winy);
      } else {
        alert(ALERT_BOUNCE);
      }
      break;
    case BRL_CMD_WINDN:
      if (canMoveDown()) {
        ses->winy += MIN(verticalWindowShift, (scr.rows - brl.textRows - ses->winy));
      } else {
        alert(ALERT_BOUNCE);
      }
      break;

    case BRL_CMD_LNUP:
      upOneLine();
      break;
    case BRL_CMD_LNDN:
      downOneLine();
      break;

    case BRL_CMD_PRDIFLN:
      upDifferentLine(isSameText);
      break;
    case BRL_CMD_NXDIFLN:
      downDifferentLine(isSameText);
      break;

    case BRL_CMD_ATTRUP:
      upDifferentLine(isSameAttributes);
      break;
    case BRL_CMD_ATTRDN:
      downDifferentLine(isSameAttributes);
      break;

    {
      int increment;
    case BRL_CMD_PRPGRPH:
      increment = -1;
      goto findParagraph;
    case BRL_CMD_NXPGRPH:
      increment = 1;
    findParagraph:
      {
        int found = 0;
        ScreenCharacter characters[scr.cols];
        int findBlank = 1;
        int line = ses->winy;
        int i;
        while ((line >= 0) && (line <= (int)(scr.rows - brl.textRows))) {
          readScreen(0, line, scr.cols, 1, characters);
          for (i=0; i<scr.cols; i++) {
            wchar_t text = characters[i].text;
            if (text != WC_C(' ')) break;
          }
          if ((i == scr.cols) == findBlank) {
            if (!findBlank) {
              found = 1;
              ses->winy = line;
              ses->winx = 0;
              break;
            }
            findBlank = 0;
          }
          line += increment;
        }
        if (!found) alert(ALERT_BOUNCE);
      }
      break;
    }

    {
      int increment;
    case BRL_CMD_PRPROMPT:
      increment = -1;
      goto findPrompt;
    case BRL_CMD_NXPROMPT:
      increment = 1;
    findPrompt:
      {
        ScreenCharacter characters[scr.cols];
        size_t length = 0;
        readScreen(0, ses->winy, scr.cols, 1, characters);
        while (length < scr.cols) {
          if (characters[length].text == WC_C(' ')) break;
          ++length;
        }
        if (length < scr.cols) {
          findRow(length, increment, testPrompt, characters);
        } else {
          alert(ALERT_COMMAND_REJECTED);
        }
      }
      break;
    }

    {
      int increment;

      const wchar_t *cpbBuffer;
      size_t cpbLength;

    case BRL_CMD_PRSEARCH:
      increment = -1;
      goto doSearch;

    case BRL_CMD_NXSEARCH:
      increment = 1;
      goto doSearch;

    doSearch:
      if ((cpbBuffer = cpbGetContent(&cpbLength))) {
        int found = 0;
        size_t count = cpbLength;

        if (count <= scr.cols) {
          int line = ses->winy;
          wchar_t buffer[scr.cols];
          wchar_t characters[count];

          {
            unsigned int i;
            for (i=0; i<count; i+=1) characters[i] = towlower(cpbBuffer[i]);
          }

          while ((line >= 0) && (line <= (int)(scr.rows - brl.textRows))) {
            const wchar_t *address = buffer;
            size_t length = scr.cols;
            readScreenText(0, line, length, 1, buffer);

            {
              size_t i;
              for (i=0; i<length; i++) buffer[i] = towlower(buffer[i]);
            }

            if (line == ses->winy) {
              if (increment < 0) {
                int end = ses->winx + count - 1;
                if (end < length) length = end;
              } else {
                int start = ses->winx + textCount;
                if (start > length) start = length;
                address += start;
                length -= start;
              }
            }
            if (findCharacters(&address, &length, characters, count)) {
              if (increment < 0)
                while (findCharacters(&address, &length, characters, count))
                  ++address, --length;

              ses->winy = line;
              ses->winx = (address - buffer) / textCount * textCount;
              found = 1;
              break;
            }
            line += increment;
          }
        }

        if (!found) alert(ALERT_BOUNCE);
      } else {
        alert(ALERT_COMMAND_REJECTED);
      }

      break;
    }

    case BRL_CMD_LNBEG:
      if (ses->winx) {
        ses->winx = 0;
      } else {
        alert(ALERT_BOUNCE);
      }
      break;

    case BRL_CMD_LNEND: {
      int end = MAX(scr.cols, textCount) - textCount;

      if (ses->winx < end) {
        ses->winx = end;
      } else {
        alert(ALERT_BOUNCE);
      }
      break;
    }

    case BRL_CMD_CHRLT:
      if (!moveWindowLeft(1)) alert(ALERT_BOUNCE);
      break;
    case BRL_CMD_CHRRT:
      if (!moveWindowRight(1)) alert(ALERT_BOUNCE);
      break;

    case BRL_CMD_HWINLT:
      if (!shiftWindowLeft(halfWindowShift)) alert(ALERT_BOUNCE);
      break;
    case BRL_CMD_HWINRT:
      if (!shiftWindowRight(halfWindowShift)) alert(ALERT_BOUNCE);
      break;

    case BRL_CMD_FWINLTSKIP:
      if (prefs.skipBlankWindowsMode == sbwAll) {
        int oldX = ses->winx;
        int oldY = ses->winy;
        int tuneLimit = 3;
        ScreenCharacter characters[scr.cols];

        while (1) {
          int charCount;
          int charIndex;

          if (!shiftWindowLeft(fullWindowShift)) {
            if (ses->winy == 0) {
              ses->winx = oldX;
              ses->winy = oldY;

              alert(ALERT_BOUNCE);
              break;
            }

            if (tuneLimit-- > 0) alert(ALERT_WRAP_UP);
            upLine(isSameText);
            placeWindowRight();
          }

          charCount = getWindowLength();
          charCount = MIN(charCount, scr.cols-ses->winx);
          readScreen(ses->winx, ses->winy, charCount, 1, characters);

          for (charIndex=charCount-1; charIndex>=0; charIndex-=1) {
            wchar_t text = characters[charIndex].text;

            if (text != WC_C(' ')) break;
          }

          if (showCursor() &&
              (scr.posy == ses->winy) &&
              (scr.posx >= 0) &&
              (scr.posx < (ses->winx + charCount))) {
            charIndex = MAX(charIndex, scr.posx-ses->winx);
          }

          if (charIndex >= 0) break;
        }

        break;
      }

    {
      int skipBlankWindows;

      skipBlankWindows = 1;
      goto moveLeft;

    case BRL_CMD_FWINLT:
      skipBlankWindows = 0;
      goto moveLeft;

    moveLeft:
      {
        int oldX = ses->winx;

        if (shiftWindowLeft(fullWindowShift)) {
          if (skipBlankWindows) {
            int charCount;

            if (prefs.skipBlankWindowsMode == sbwEndOfLine) goto skipEndOfLine;
            charCount = MIN(scr.cols, ses->winx+textCount);
            if (!showCursor() ||
                (scr.posy != ses->winy) ||
                (scr.posx < 0) ||
                (scr.posx >= charCount)) {
              int charIndex;
              ScreenCharacter characters[charCount];

              readScreen(0, ses->winy, charCount, 1, characters);

              for (charIndex=0; charIndex<charCount; charIndex+=1) {
                wchar_t text = characters[charIndex].text;

                if (text != WC_C(' ')) break;
              }

              if (charIndex == charCount) goto wrapUp;
            }
          }

          break;
        }

      wrapUp:
        if (ses->winy == 0) {
          ses->winx = oldX;

          alert(ALERT_BOUNCE);
          break;
        }

        alert(ALERT_WRAP_UP);
        upLine(isSameText);
        placeWindowRight();

      skipEndOfLine:
        if (skipBlankWindows && (prefs.skipBlankWindowsMode == sbwEndOfLine)) {
          int charIndex;
          ScreenCharacter characters[scr.cols];

          readScreen(0, ses->winy, scr.cols, 1, characters);

          for (charIndex=scr.cols-1; charIndex>0; charIndex-=1) {
            wchar_t text = characters[charIndex].text;

            if (text != WC_C(' ')) break;
          }

          if (showCursor() && (scr.posy == ses->winy) && SCR_COLUMN_OK(scr.posx)) {
            charIndex = MAX(charIndex, scr.posx);
          }

          if (charIndex < ses->winx) placeRightEdge(charIndex);
        }
      }

      break;
    }

    case BRL_CMD_FWINRTSKIP:
      if (prefs.skipBlankWindowsMode == sbwAll) {
        int oldX = ses->winx;
        int oldY = ses->winy;
        int tuneLimit = 3;
        ScreenCharacter characters[scr.cols];

        while (1) {
          int charCount;
          int charIndex;

          if (!shiftWindowRight(fullWindowShift)) {
            if (ses->winy >= (scr.rows - brl.textRows)) {
              ses->winx = oldX;
              ses->winy = oldY;

              alert(ALERT_BOUNCE);
              break;
            }

            if (tuneLimit-- > 0) alert(ALERT_WRAP_DOWN);
            downLine(isSameText);
            ses->winx = 0;
          }

          charCount = getWindowLength();
          charCount = MIN(charCount, scr.cols-ses->winx);
          readScreen(ses->winx, ses->winy, charCount, 1, characters);

          for (charIndex=0; charIndex<charCount; charIndex+=1) {
            wchar_t text = characters[charIndex].text;

            if (text != WC_C(' ')) break;
          }

          if (showCursor() &&
              (scr.posy == ses->winy) &&
              (scr.posx < scr.cols) &&
              (scr.posx >= ses->winx)) {
            charIndex = MIN(charIndex, scr.posx-ses->winx);
          }

          if (charIndex < charCount) break;
        }

        break;
      }

    {
      int skipBlankWindows;

      skipBlankWindows = 1;
      goto moveRight;

    case BRL_CMD_FWINRT:
      skipBlankWindows = 0;
      goto moveRight;

    moveRight:
      {
        int oldX = ses->winx;

        if (shiftWindowRight(fullWindowShift)) {
          if (skipBlankWindows) {
            if (!showCursor() ||
                (scr.posy != ses->winy) ||
                (scr.posx < ses->winx)) {
              int charCount = scr.cols - ses->winx;
              int charIndex;
              ScreenCharacter characters[charCount];

              readScreen(ses->winx, ses->winy, charCount, 1, characters);

              for (charIndex=0; charIndex<charCount; charIndex+=1) {
                wchar_t text = characters[charIndex].text;

                if (text != WC_C(' ')) break;
              }

              if (charIndex == charCount) goto wrapDown;
            }
          }

          break;
        }

      wrapDown:
        if (ses->winy >= (scr.rows - brl.textRows)) {
          ses->winx = oldX;

          alert(ALERT_BOUNCE);
          break;
        }

        alert(ALERT_WRAP_DOWN);
        downLine(isSameText);
        ses->winx = 0;
      }

      break;
    }

    case BRL_CMD_RETURN:
      if ((ses->winx != ses->motx) || (ses->winy != ses->moty)) {
    case BRL_CMD_BACK:
        ses->winx = ses->motx;
        ses->winy = ses->moty;
        break;
      }
    case BRL_CMD_HOME:
      if (!trackCursor(1)) alert(ALERT_COMMAND_REJECTED);
      break;

    case BRL_CMD_RESTARTBRL:
      restartRequired = 1;
      break;

    case BRL_CMD_CSRJMP_VERT:
      alert(routeCursor(-1, ses->winy, scr.number)?
               ALERT_ROUTING_STARTED:
               ALERT_COMMAND_REJECTED);
      break;

    case BRL_CMD_PREFMENU: {
      int ok = 0;

      if (isMenuScreen()) {
        if (prefs.saveOnExit)
          if (savePreferences())
            alert(ALERT_COMMAND_DONE);

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
      int blk = command & BRL_MSK_BLK;
      int arg = command & BRL_MSK_ARG;
      int flags = command & BRL_MSK_FLG;

      switch (blk) {
        case BRL_BLK_ROUTE: {
          int column, row;

          if (getCharacterCoordinates(arg, &column, &row, 0, 1)) {
            if (routeCursor(column, row, scr.number)) {
              alert(ALERT_ROUTING_STARTED);
              break;
            }
          }
          alert(ALERT_COMMAND_REJECTED);
          break;
        }

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

        case BRL_BLK_SETLEFT: {
          int column, row;

          if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
            ses->winx = column;
            ses->winy = row;
          } else {
            alert(ALERT_COMMAND_REJECTED);
          }
          break;
        }

        case BRL_BLK_GOTOLINE:
          if (flags & BRL_FLG_LINE_SCALED)
            arg = rescaleInteger(arg, BRL_MSK_ARG, scr.rows-1);
          if (arg < scr.rows) {
            slideWindowVertically(arg);
            if (flags & BRL_FLG_LINE_TOLEFT) ses->winx = 0;
          } else {
            alert(ALERT_COMMAND_REJECTED);
          }
          break;

        case BRL_BLK_SETMARK: {
          ScreenLocation *mark = &ses->marks[arg];
          mark->column = ses->winx;
          mark->row = ses->winy;
          alert(ALERT_MARK_SET);
          break;
        }
        case BRL_BLK_GOTOMARK: {
          ScreenLocation *mark = &ses->marks[arg];
          ses->winx = mark->column;
          ses->winy = mark->row;
          break;
        }

        {
          int column, row;
          int increment;

        case BRL_BLK_PRINDENT:
          increment = -1;
          goto findIndent;

        case BRL_BLK_NXINDENT:
          increment = 1;

        findIndent:
          if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
            ses->winy = row;
            findRow(column, increment, testIndent, NULL);
          } else {
            alert(ALERT_COMMAND_REJECTED);
          }
          break;
        }

        case BRL_BLK_PRDIFCHAR: {
          int column, row;

          if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
            ses->winy = row;
            upDifferentCharacter(isSameText, column);
          } else {
            alert(ALERT_COMMAND_REJECTED);
          }
          break;
        }

        case BRL_BLK_NXDIFCHAR: {
          int column, row;

          if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
            ses->winy = row;
            downDifferentCharacter(isSameText, column);
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
addNavigationCommands (void) {
  return pushCommandHandler("navigation", KTB_CTX_DEFAULT,
                            handleNavigationCommands, NULL, NULL);
}
