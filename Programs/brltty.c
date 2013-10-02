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
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#ifdef HAVE_ICU
#include <unicode/uchar.h>
#endif /* HAVE_ICU */

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif /* HAVE_LANGINFO_H */

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif /* HAVE_SIGNAL_H */

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

#include "embed.h"
#include "log.h"
#include "cmd_queue.h"
#include "cmd_navigation.h"
#include "cmd_speech.h"
#include "timing.h"
#include "async_alarm.h"
#include "async_wait.h"
#include "message.h"
#include "tunes.h"
#include "ttb.h"
#include "atb.h"
#include "ctb.h"
#include "routing.h"
#include "touch.h"
#include "charset.h"
#include "unicode.h"
#include "scr.h"
#include "status.h"
#include "ses.h"
#include "scancodes.h"
#include "brl.h"
#include "brltty.h"
#include "api_control.h"
#include "prefs.h"
#include "defaults.h"

#ifdef ENABLE_SPEECH_SUPPORT
#include "spk.h"
#endif /* ENABLE_SPEECH_SUPPORT */

int updateInterval = DEFAULT_UPDATE_INTERVAL;
int messageDelay = DEFAULT_MESSAGE_DELAY;

static volatile unsigned int terminationCount;
static volatile time_t terminationTime;

#define TERMINATION_COUNT_EXIT_THRESHOLD 3
#define TERMINATION_COUNT_RESET_TIME 5

BrailleDisplay brl;                        /* For the Braille routines */
ScreenDescription scr;
SessionEntry *ses = NULL;

unsigned char infoMode = 0;

unsigned int textStart;
unsigned int textCount;
unsigned char textMaximized = 0;

unsigned int statusStart;
unsigned int statusCount;

unsigned int fullWindowShift;                /* Full window horizontal distance */
unsigned int halfWindowShift;                /* Half window horizontal distance */
unsigned int verticalWindowShift;                /* Window vertical distance */

#ifdef ENABLE_CONTRACTED_BRAILLE
int isContracted = 0;
int contractedLength;
static int contractedStart;
static int contractedOffsets[0X100];
static int contractedTrack = 0;
#endif /* ENABLE_CONTRACTED_BRAILLE */

static void
checkRoutingStatus (RoutingStatus ok, int wait) {
  RoutingStatus status = getRoutingStatus(wait);

  if (status != ROUTING_NONE) {
    playTune((status > ok)? &tune_routing_failed: &tune_routing_succeeded);

    ses->spkx = scr.posx;
    ses->spky = scr.posy;
  }
}

static int
executeCommand (int command) {
  int oldmotx = ses->winx;
  int oldmoty = ses->winy;
  int handled = handleCommand(command);

  if (handled) {
    resetUpdateAlarm(10);
  }

  if ((ses->winx != oldmotx) || (ses->winy != oldmoty)) {
    /* The window has been manually moved. */
    ses->motx = ses->winx;
    ses->moty = ses->winy;

#ifdef ENABLE_CONTRACTED_BRAILLE
    isContracted = 0;
#endif /* ENABLE_CONTRACTED_BRAILLE */

#ifdef ENABLE_SPEECH_SUPPORT
    if (ses->trackCursor && speechTracking && (scr.number == speechScreen)) {
      ses->trackCursor = 0;
      playTune(&tune_cursor_unlinked);
    }
#endif /* ENABLE_SPEECH_SUPPORT */
  }

  if (!(command & BRL_MSK_BLK)) {
    if (command & BRL_FLG_MOTION_ROUTE) {
      int left = ses->winx;
      int right = MIN(left+textCount, scr.cols) - 1;

      int top = ses->winy;
      int bottom = MIN(top+brl.textRows, scr.rows) - 1;

      if ((scr.posx < left) || (scr.posx > right) ||
          (scr.posy < top) || (scr.posy > bottom)) {
        if (routeCursor(MIN(MAX(scr.posx, left), right),
                        MIN(MAX(scr.posy, top), bottom),
                        scr.number)) {
          playTune(&tune_routing_started);
          checkRoutingStatus(ROUTING_WRONG_COLUMN, 1);

          {
            ScreenDescription description;
            describeScreen(&description);

            if (description.number == scr.number) {
              slideWindowVertically(description.posy);
              placeWindowHorizontally(description.posx);
            }
          }
        }
      }
    }
  }

  return handled;
}

static int
handleUnhandledCommand (int command, void *datga) {
  playTune(&tune_command_rejected);
  return 0;
}

static void
setSessionEntry (void) {
  describeScreen(&scr);
  if (scr.number == -1) scr.number = userVirtualTerminal(0);

  {
    typedef enum {SAME, DIFFERENT, FIRST} State;
    State state = (!ses)? FIRST:
                  (scr.number == ses->number)? SAME:
                  DIFFERENT;

    if (state != SAME) {
      ses = getSessionEntry(scr.number);

      if (state == FIRST) {
        currentCommandExecuter = executeCommand;
        pushCommandHandler(KTB_CTX_DEFAULT, handleUnhandledCommand, NULL);

#ifdef ENABLE_SPEECH_SUPPORT
        pushCommandHandler(KTB_CTX_DEFAULT, handleSpeechCommand, NULL);
#endif /*  ENABLE_SPEECH_SUPPORT */

        pushCommandHandler(KTB_CTX_DEFAULT, handleNavigationCommand, NULL);
        pushCommandHandler(KTB_CTX_DEFAULT, handleScreenCommand, NULL);
      }
    }
  }
}

void
updateSessionAttributes (void) {
  setSessionEntry();

  {
    int maximum = MAX(scr.rows-(int)brl.textRows, 0);
    int *table[] = {&ses->winy, &ses->moty, NULL};
    int **value = table;
    while (*value) {
      if (**value > maximum) **value = maximum;
      value += 1;
    }
  }

  {
    int maximum = MAX(scr.cols-1, 0);
    int *table[] = {&ses->winx, &ses->motx, NULL};
    int **value = table;
    while (*value) {
      if (**value > maximum) **value = maximum;
      value += 1;
    }
  }
}

static void
exitSessions (void *data) {
  currentCommandExecuter = NULL;
  while (popCommandHandler());

  ses = NULL;
  deallocateSessionEntries();
}

static void
exitLog (void *data) {
  closeSystemLog();
  closeLogFile();
}

#ifdef HAVE_SIGNAL_H
static void
handleSignal (int number, void (*handler) (int)) {
#ifdef HAVE_SIGACTION
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  sigemptyset(&action.sa_mask);
  action.sa_handler = handler;
  if (sigaction(number, &action, NULL) == -1) {
    logSystemError("sigaction");
  }
#else /* HAVE_SIGACTION */
  if (signal(number, handler) == SIG_ERR) {
    logSystemError("signal");
  }
#endif /* HAVE_SIGACTION */
}

static void 
handleTerminationRequest (int signalNumber) {
  time_t now = time(NULL);
  if (difftime(now, terminationTime) > TERMINATION_COUNT_RESET_TIME) terminationCount = 0;
  if ((terminationCount += 1) > TERMINATION_COUNT_EXIT_THRESHOLD) exit(1);
  terminationTime = now;
}
#endif /* HAVE_SIGNAL_H */

static int
writeStatusCells (void) {
  if (braille->writeStatus) {
    const unsigned char *fields = prefs.statusFields;
    unsigned int length = getStatusFieldsLength(fields);

    if (length > 0) {
      unsigned char cells[length];        /* status cell buffer */
      unsigned int count = brl.statusColumns * brl.statusRows;

      memset(cells, 0, length);
      renderStatusFields(fields, cells);

      if (count > length) {
        unsigned char buffer[count];
        memcpy(buffer, cells, length);
        memset(&buffer[length], 0, count-length);
        if (!braille->writeStatus(&brl, buffer)) return 0;
      } else if (!braille->writeStatus(&brl, cells)) {
        return 0;
      }
    } else if (!clearStatusCells(&brl)) {
      return 0;
    }
  }

  return 1;
}

static void
fillStatusSeparator (wchar_t *text, unsigned char *dots) {
  if ((prefs.statusSeparator != ssNone) && (statusCount > 0)) {
    int onRight = statusStart > 0;
    unsigned int column = (onRight? statusStart: textStart) - 1;

    wchar_t textSeparator;
#ifdef HAVE_WCHAR_H 
    const wchar_t textSeparator_left  = 0X23B8; /* LEFT VERTICAL BOX LINE */
    const wchar_t textSeparator_right = 0X23B9; /* RIGHT VERTICAL BOX LINE */
    const wchar_t textSeparator_block = 0X2503; /* BOX DRAWINGS HEAVY VERTICAL */
#else /* HAVE_WCHAR_H */
    const wchar_t textSeparator_left  = 0X5B; /* LEFT SQUARE BRACKET */
    const wchar_t textSeparator_right = 0X5D; /* RIGHT SQUARE BRACKET */
    const wchar_t textSeparator_block = 0X7C; /* VERTICAL LINE */
#endif /* HAVE_WCHAR_H */

    unsigned char dotsSeparator;
    const unsigned char dotsSeparator_left = BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT7;
    const unsigned char dotsSeparator_right = BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8;
    const unsigned char dotsSeparator_block = dotsSeparator_left | dotsSeparator_right;

    text += column;
    dots += column;

    switch (prefs.statusSeparator) {
      case ssBlock:
        textSeparator = textSeparator_block;
        dotsSeparator = dotsSeparator_block;
        break;

      case ssStatusSide:
        textSeparator = onRight? textSeparator_right: textSeparator_left;
        dotsSeparator = onRight? dotsSeparator_right: dotsSeparator_left;
        break;

      case ssTextSide:
        textSeparator = onRight? textSeparator_left: textSeparator_right;
        dotsSeparator = onRight? dotsSeparator_left: dotsSeparator_right;
        break;

      default:
        textSeparator = WC_C(' ');
        dotsSeparator = 0;
        break;
    }

    {
      unsigned int row;
      for (row=0; row<brl.textRows; row+=1) {
        *text = textSeparator;
        text += brl.textColumns;

        *dots = dotsSeparator;
        dots += brl.textColumns;
      }
    }
  }
}

int
writeBrailleCharacters (const char *mode, const wchar_t *characters, size_t length) {
  wchar_t textBuffer[brl.textColumns * brl.textRows];

  fillTextRegion(textBuffer, brl.buffer,
                 textStart, textCount, brl.textColumns, brl.textRows,
                 characters, length);

  {
    size_t modeLength = mode? getUtf8Length(mode): 0;
    wchar_t modeCharacters[modeLength + 1];
    convertTextToWchars(modeCharacters, mode, ARRAY_COUNT(modeCharacters));
    fillTextRegion(textBuffer, brl.buffer,
                   statusStart, statusCount, brl.textColumns, brl.textRows,
                   modeCharacters, modeLength);
  }

  fillStatusSeparator(textBuffer, brl.buffer);

  return braille->writeWindow(&brl, textBuffer);
}

int
writeBrailleText (const char *mode, const char *text) {
  size_t count = getTextLength(text) + 1;
  wchar_t characters[count];
  size_t length = convertTextToWchars(characters, text, count);
  return writeBrailleCharacters(mode, characters, length);
}

int
showBrailleText (const char *mode, const char *text, int minimumDelay) {
  int ok = writeBrailleText(mode, text);
  drainBrailleOutput(&brl, minimumDelay);
  return ok;
}

static inline const char *
getMeridianString_am (void) {
#ifdef AM_STR
  return nl_langinfo(AM_STR);
#else /* AM_STR */
  return "am";
#endif /* AM_STR */
}

static inline const char *
getMeridianString_pm (void) {
#ifdef PM_STR
  return nl_langinfo(PM_STR);
#else /* PM_STR */
  return "pm";
#endif /* PM_STR */
}

static const char *
getMeridianString (uint8_t *hour) {
  const char *string = NULL;

  switch (prefs.timeFormat) {
    case tf12Hour: {
      const uint8_t twelve = 12;

      string = (*hour < twelve)? getMeridianString_am(): getMeridianString_pm();
      *hour %= twelve;
      if (!*hour) *hour = twelve;
      break;
    }

    default:
      break;
  }

  return string;
}

size_t
formatBrailleTime (char *buffer, size_t size, const TimeFormattingData *fmt) {
  size_t length;
  char time[0X40];
  STR_BEGIN(buffer, size);

  {
    const char *hourFormat = "%02" PRIu8;
    const char *minuteFormat = "%02" PRIu8;
    const char *secondFormat = "%02" PRIu8;
    char separator;

    switch (prefs.timeSeparator) {
      default:
      case tsColon:
        separator = ':';
        break;

      case tsDot:
        separator = '.';
        break;
    }

    switch (prefs.timeFormat) {
      default:
      case tf24Hour:
        break;

      case tf12Hour:
        hourFormat = "%" PRIu8;
        break;
    }

    STR_BEGIN(time, sizeof(time));
    STR_PRINTF(hourFormat, fmt->time.hour);
    STR_PRINTF("%c", separator);
    STR_PRINTF(minuteFormat, fmt->time.minute);

    if (prefs.showSeconds) {
      STR_PRINTF("%c", separator);
      STR_PRINTF(secondFormat, fmt->time.second);
    }

    if (fmt->meridian) STR_PRINTF("%s", fmt->meridian);
    STR_END
  }

  if (prefs.datePosition == dpNone) {
    STR_PRINTF("%s", time);
  } else {
    char date[0X40];

    {
      const char *yearFormat = "%04" PRIu16;
      const char *monthFormat = "%02" PRIu8;
      const char *dayFormat = "%02" PRIu8;

      uint8_t month = fmt->time.month + 1;
      uint8_t day = fmt->time.day + 1;

      char separator;

      switch (prefs.dateSeparator) {
        default:
        case dsDash:
          separator = '-';
          break;

        case dsSlash:
          separator = '/';
          break;

        case dsDot:
          separator = '.';
          break;
      }

      STR_BEGIN(date, sizeof(date));
      switch (prefs.dateFormat) {
        default:
        case dfYearMonthDay:
          STR_PRINTF(yearFormat, fmt->time.year);
          STR_PRINTF("%c", separator);
          STR_PRINTF(monthFormat, month);
          STR_PRINTF("%c", separator);
          STR_PRINTF(dayFormat, day);
          break;

        case dfMonthDayYear:
          STR_PRINTF(monthFormat, month);
          STR_PRINTF("%c", separator);
          STR_PRINTF(dayFormat, day);
          STR_PRINTF("%c", separator);
          STR_PRINTF(yearFormat, fmt->time.year);
          break;

        case dfDayMonthYear:
          STR_PRINTF(dayFormat, day);
          STR_PRINTF("%c", separator);
          STR_PRINTF(monthFormat, month);
          STR_PRINTF("%c", separator);
          STR_PRINTF(yearFormat, fmt->time.year);
          break;
      }
      STR_END

      switch (prefs.datePosition) {
        case dpBeforeTime:
          STR_PRINTF("%s %s", date, time);
          break;

        case dpAfterTime:
          STR_PRINTF("%s %s", time, date);
          break;

        default:
          STR_PRINTF("%s", date);
          break;
      }
    }
  }

  length = STR_LENGTH;
  STR_END
  return length;
}

void
getTimeFormattingData (TimeFormattingData *fmt) {
  TimeValue now;

  getCurrentTime(&now);
  expandTimeValue(&now, &fmt->time);
  fmt->meridian = getMeridianString(&fmt->time.hour);
}

size_t
formatCharacterDescription (char *buffer, size_t size, int column, int row) {
  static char *const colours[] = {
    strtext("black"),
    strtext("blue"),
    strtext("green"),
    strtext("cyan"),
    strtext("red"),
    strtext("magenta"),
    strtext("brown"),
    strtext("light grey"),
    strtext("dark grey"),
    strtext("light blue"),
    strtext("light green"),
    strtext("light cyan"),
    strtext("light red"),
    strtext("light magenta"),
    strtext("yellow"),
    strtext("white")
  };

  size_t length;
  ScreenCharacter character;

  readScreen(column, row, 1, 1, &character);
  STR_BEGIN(buffer, size);

  {
    uint32_t text = character.text;

    STR_PRINTF("char %" PRIu32 " (U+%04" PRIX32 "): %s on %s",
               text, text,
               gettext(colours[character.attributes & 0X0F]),
               gettext(colours[(character.attributes & 0X70) >> 4]));
  }

  if (character.attributes & SCR_ATTR_BLINK) {
    STR_PRINTF(" %s", gettext("blink"));
  }

#ifdef HAVE_ICU
  {
    char name[0X40];
    UErrorCode error = U_ZERO_ERROR;

    u_charName(character.text, U_EXTENDED_CHAR_NAME, name, sizeof(name), &error);
    if (U_SUCCESS(error)) {
      STR_PRINTF(" [%s]", name);
    }
  }
#endif /* HAVE_ICU */

  length = STR_LENGTH;
  STR_END;
  return length;
}

static int
showInfo (void) {
  const char *mode = "info";
  const size_t size = brl.textColumns * brl.textRows;
  char text[size + 1];

  if (!setStatusText(&brl, mode)) return 0;

  /* Here we must be careful. Some displays (e.g. Braille Lite 18)
   * are very small, and others (e.g. Bookworm) are even smaller.
   */
  if (size < 21) {
    wchar_t characters[size];
    size_t length;

    const int cellCount = 5;
    unsigned char cells[cellCount];
    char prefix[cellCount];

    {
      static const unsigned char fields[] = {
        sfCursorAndWindowColumn, sfCursorAndWindowRow, sfStateDots, sfEnd
      };

      memset(cells, 0, cellCount);
      renderStatusFields(fields, cells);
    }

    memset(prefix, 'x', cellCount);
    length = snprintf(text, sizeof(text), "%.*s %02d %c%c%c%c%c%c",
                      cellCount, prefix,
                      scr.number,
                      ses->trackCursor? 't': ' ',
                      prefs.showCursor? (prefs.blinkingCursor? 'B': 'v'):
                                        (prefs.blinkingCursor? 'b': ' '),
                      ses->displayMode? 'a': 't',
                      isFrozenScreen()? 'f': ' ',
                      prefs.textStyle? '6': '8',
                      prefs.blinkingCapitals? 'B': ' ');
    if (length > size) length = size;

    {
      unsigned int i;

      for (i=0; i<length; i+=1) {
        if (i < cellCount) {
          characters[i] = UNICODE_BRAILLE_ROW | cells[i];
        } else {
          wint_t character = convertCharToWchar(text[i]);
          if (character == WEOF) character = WC_C('?');
          characters[i] = character;
        }
      }
    }

    return writeBrailleCharacters(mode, characters, length);
  }

  STR_BEGIN(text, sizeof(text));
  STR_PRINTF("%02d:%02d %02d:%02d %02d %c%c%c%c%c%c",
             SCR_COLUMN_NUMBER(ses->winx), SCR_ROW_NUMBER(ses->winy),
             SCR_COLUMN_NUMBER(scr.posx), SCR_ROW_NUMBER(scr.posy),
             scr.number, 
             ses->trackCursor? 't': ' ',
             prefs.showCursor? (prefs.blinkingCursor? 'B': 'v'):
                               (prefs.blinkingCursor? 'b': ' '),
             ses->displayMode? 'a': 't',
             isFrozenScreen()? 'f': ' ',
             prefs.textStyle? '6': '8',
             prefs.blinkingCapitals? 'B': ' ');

  {
    TimeFormattingData fmt;
    getTimeFormattingData(&fmt);

    {
      char buffer[0X80];
      size_t length = formatBrailleTime(buffer, sizeof(buffer), &fmt);

      if (length < STR_LEFT) STR_PRINTF("%s", buffer);
    }
  }

  STR_END
  return writeBrailleText(mode, text);
}

void 
slideWindowVertically (int y) {
  if (y < ses->winy)
    ses->winy = y;
  else if (y >= (int)(ses->winy + brl.textRows))
    ses->winy = y - (brl.textRows - 1);
}

void 
placeWindowHorizontally (int x) {
  if (prefs.slidingWindow) {
    ses->winx = MAX(0, (x - (int)(textCount / 2)));
  } else {
    ses->winx = x / textCount * textCount;
  }
}

void
placeRightEdge (int column) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracting()) {
    ses->winx = 0;

    while (1) {
      int length = getContractedLength(textCount);
      int end = ses->winx + length;

      if (end > column) break;
      if (end == ses->winx) break;
      ses->winx = end;
    }
  } else
#endif /* ENABLE_CONTRACTED_BRAILLE */
  {
    ses->winx = column / textCount * textCount;
  }
}

void
placeWindowRight (void) {
  placeRightEdge(scr.cols-1);
}

int
moveWindowLeft (unsigned int amount) {
  if (ses->winx < 1) return 0;
  if (amount < 1) return 0;

  ses->winx -= MIN(ses->winx, amount);
  return 1;
}

int
moveWindowRight (unsigned int amount) {
  int newx = ses->winx + amount;

  if ((newx > ses->winx) && (newx < scr.cols)) {
    ses->winx = newx;
    return 1;
  }

  return 0;
}

int
shiftWindowLeft (unsigned int amount) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracting()) {
    int reference = ses->winx;
    int first = 0;
    int last = ses->winx - 1;

    while (first <= last) {
      int end = (ses->winx = (first + last) / 2) + getContractedLength(amount);

      if (end < reference) {
        first = ses->winx + 1;
      } else {
        last = ses->winx - 1;
      }
    }

    if (first == reference) {
      if (!first) return 0;
      first -= 1;
    }

    ses->winx = first;
    return 1;
  }
#endif /* ENABLE_CONTRACTED_BRAILLE */

  return moveWindowLeft(amount);
}

int
shiftWindowRight (unsigned int amount) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracting()) amount = getContractedLength(amount);
#endif /* ENABLE_CONTRACTED_BRAILLE */

  return moveWindowRight(amount);
}

int
trackCursor (int place) {
  if (!SCR_CURSOR_OK()) return 0;

#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracted) {
    ses->winy = scr.posy;
    if (scr.posx < ses->winx) {
      int length = scr.posx + 1;
      ScreenCharacter characters[length];
      int onspace = 1;
      readScreen(0, ses->winy, length, 1, characters);
      while (length) {
        if ((iswspace(characters[--length].text) != 0) != onspace) {
          if (onspace) {
            onspace = 0;
          } else {
            ++length;
            break;
          }
        }
      }
      ses->winx = length;
    }
    contractedTrack = 1;
    return 1;
  }
#endif /* ENABLE_CONTRACTED_BRAILLE */

  if (place) {
    if ((scr.posx < ses->winx) || (scr.posx >= (int)(ses->winx + textCount)) ||
        (scr.posy < ses->winy) || (scr.posy >= (int)(ses->winy + brl.textRows))) {
      placeWindowHorizontally(scr.posx);
    }
  }

  if (prefs.slidingWindow) {
    int reset = textCount * 3 / 10;
    int trigger = prefs.eagerSlidingWindow? textCount*3/20: 0;

    if (scr.posx < (ses->winx + trigger)) {
      ses->winx = MAX(scr.posx-reset, 0);
    } else if (scr.posx >= (int)(ses->winx + textCount - trigger)) {
      ses->winx = MAX(MIN(scr.posx+reset+1, scr.cols)-(int)textCount, 0);
    }
  } else if (scr.posx < ses->winx) {
    ses->winx -= ((ses->winx - scr.posx - 1) / textCount + 1) * textCount;
    if (ses->winx < 0) ses->winx = 0;
  } else {
    ses->winx += (scr.posx - ses->winx) / textCount * textCount;
  }

  slideWindowVertically(scr.posy);
  return 1;
}

ScreenCharacterType
getScreenCharacterType (const ScreenCharacter *character) {
  if (iswspace(character->text)) return SCT_SPACE;
  if (iswalnum(character->text)) return SCT_WORD;
  if (wcschr(WS_C("_"), character->text)) return SCT_WORD;
  return SCT_NONWORD;
}

int
findFirstNonSpaceCharacter (const ScreenCharacter *characters, int count) {
  int index = 0;

  while (index < count) {
    if (getScreenCharacterType(&characters[index]) != SCT_SPACE) return index;
    index += 1;
  }

  return -1;
}

int
findLastNonSpaceCharacter (const ScreenCharacter *characters, int count) {
  int index = count;

  while (index > 0)
    if (getScreenCharacterType(&characters[--index]) != SCT_SPACE)
      return index;

  return -1;
}

int
isAllSpaceCharacters (const ScreenCharacter *characters, int count) {
  return findFirstNonSpaceCharacter(characters, count) < 0;
}

#ifdef ENABLE_SPEECH_SUPPORT
SpeechSynthesizer spk;
int speechTracking = 0;
int speechScreen = -1;
int speechLine = 0;
int speechIndex = -1;

void
trackSpeech (int index) {
  placeWindowHorizontally(index % scr.cols);
  slideWindowVertically((index / scr.cols) + speechLine);
}

int
autospeak (void) {
  if (speech->definition.code == noSpeech.definition.code) return 0;
  if (prefs.autospeak) return 1;
  if (opt_quietIfNoBraille) return 0;
  return braille->definition.code == noBraille.definition.code;
}

static void
sayWideCharacters (const wchar_t *characters, const unsigned char *attributes, size_t count, int immediate) {
  size_t length;
  void *text = makeUtf8FromWchars(characters, count, &length);

  if (text) {
    if (immediate) speech->mute(&spk);
    speech->say(&spk, text, length, count, attributes);
    free(text);
  } else {
    logMallocError();
  }
}

void
sayScreenCharacters (const ScreenCharacter *characters, size_t count, int immediate) {
  wchar_t text[count];
  wchar_t *t = text;

  unsigned char attributes[count];
  unsigned char *a = attributes;

  {
    unsigned int i;

    for (i=0; i<count; i+=1) {
      const ScreenCharacter *character = &characters[i];
      *t++ = character->text;
      *a++ = character->attributes;
    }
  }

  sayWideCharacters(text, attributes, count, immediate);
}

void
speakCharacters (const ScreenCharacter *characters, size_t count, int spell) {
  int immediate = 1;

  if (isAllSpaceCharacters(characters, count)) {
    switch (prefs.whitespaceIndicator) {
      default:
      case wsNone:
        if (immediate) speech->mute(&spk);
        break;

      case wsSaySpace: {
        wchar_t buffer[0X100];
        size_t length = convertTextToWchars(buffer, gettext("space"), ARRAY_COUNT(buffer));

        sayWideCharacters(buffer, NULL, length, immediate);
        break;
      }
    }
  } else if (count == 1) {
    wchar_t character = characters[0].text;
    const char *prefix = NULL;
    int restorePitch = 0;
    int restorePunctuation = 0;

    if (iswupper(character)) {
      switch (prefs.uppercaseIndicator) {
        default:
        case ucNone:
          break;

        case ucSayCap:
          // "cap" here, used during speech output, is short for "capital".
          // It is spoken just before an uppercase letter, e.g. "cap A".
          prefix = gettext("cap");
          break;

        case ucRaisePitch:
          if (speech->setPitch) {
            unsigned char pitch = prefs.speechPitch + 7;
            if (pitch > SPK_PITCH_MAXIMUM) pitch = SPK_PITCH_MAXIMUM;

            if (pitch != prefs.speechPitch) {
              speech->setPitch(&spk, pitch);
              restorePitch = 1;
            }
          }
          break;
      }
    }

    if (speech->setPunctuation) {
      unsigned char punctuation = SPK_PUNCTUATION_ALL;

      if (punctuation != prefs.speechPunctuation) {
        speech->setPunctuation(&spk, punctuation);
        restorePunctuation = 1;
      }
    }

    if (prefix) {
      wchar_t buffer[0X100];
      size_t length = convertTextToWchars(buffer, prefix, ARRAY_COUNT(buffer));
      buffer[length++] = WC_C(' ');
      buffer[length++] = character;
      sayWideCharacters(buffer, NULL, length, immediate);
    } else {
      sayWideCharacters(&character, NULL, 1, immediate);
    }

    if (restorePunctuation) speech->setPunctuation(&spk, prefs.speechPunctuation);
    if (restorePitch) speech->setPitch(&spk, prefs.speechPitch);
  } else if (spell) {
    wchar_t string[count * 2];
    size_t length = 0;
    unsigned int index = 0;

    while (index < count) {
      string[length++] = characters[index++].text;
      string[length++] = WC_C(' ');
    }

    string[length] = WC_C('\0');
    sayWideCharacters(string, NULL, length, immediate);
  } else {
    sayScreenCharacters(characters, count, immediate);
  }
}
#endif /* ENABLE_SPEECH_SUPPORT */

#ifdef ENABLE_CONTRACTED_BRAILLE
int
isContracting (void) {
  return (prefs.textStyle == tsContractedBraille) && contractionTable;
}

static int
getUncontractedCursorOffset (int x, int y) {
  return ((y == ses->winy) && (x >= ses->winx) && (x < scr.cols))? (x - ses->winx): -1;
}

static int
getContractedCursor (void) {
  int offset = getUncontractedCursorOffset(scr.posx, scr.posy);
  return ((offset >= 0) && !ses->hideCursor)? offset: CTB_NO_CURSOR;
}

int
getContractedLength (unsigned int outputLimit) {
  int inputLength = scr.cols - ses->winx;
  wchar_t inputBuffer[inputLength];

  int outputLength = outputLimit;
  unsigned char outputBuffer[outputLength];

  readScreenText(ses->winx, ses->winy, inputLength, 1, inputBuffer);
  contractText(contractionTable,
               inputBuffer, &inputLength,
               outputBuffer, &outputLength,
               NULL, getContractedCursor());
  return inputLength;
}
#endif /* ENABLE_CONTRACTED_BRAILLE */

BlinkingState cursorBlinkingState = {
  .blinkingEnabled = &prefs.blinkingCursor,
  .visibleTime = &prefs.cursorVisibleTime,
  .invisibleTime = &prefs.cursorInvisibleTime
};

BlinkingState attributesBlinkingState = {
  .blinkingEnabled = &prefs.blinkingAttributes,
  .visibleTime = &prefs.attributesVisibleTime,
  .invisibleTime = &prefs.attributesInvisibleTime
};

BlinkingState capitalsBlinkingState = {
  .blinkingEnabled = &prefs.blinkingCapitals,
  .visibleTime = &prefs.capitalsVisibleTime,
  .invisibleTime = &prefs.capitalsInvisibleTime
};

BlinkingState speechCursorBlinkingState = {
  .blinkingEnabled = &prefs.blinkingSpeechCursor,
  .visibleTime = &prefs.speechCursorVisibleTime,
  .invisibleTime = &prefs.speechCursorInvisibleTime
};

static int
isBlinkedOn (const BlinkingState *state) {
  if (!*state->blinkingEnabled) return 1;
  return state->isVisible;
}

void
setBlinkingState (BlinkingState *state, int visible) {
  state->timer = PREFERENCES_TIME((state->isVisible = visible)? *state->visibleTime: *state->invisibleTime);
}

static void
updateBlinkingState (BlinkingState *state) {
  if (*state->blinkingEnabled)
      if ((state->timer -= updateInterval) <= 0)
        setBlinkingState(state, !state->isVisible);
}

void
resetBlinkingStates (void) {
  setBlinkingState(&cursorBlinkingState, 0);
  setBlinkingState(&attributesBlinkingState, 1);
  setBlinkingState(&capitalsBlinkingState, 1);
  setBlinkingState(&speechCursorBlinkingState, 0);
}

int
toggleFlag (
  int *bits, int bit, int command,
  const TuneDefinition *offTune, const TuneDefinition *onTune
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
      playTune(&tune_command_rejected);
      return 0;
  }

  if (*bits == oldBits) {
    playTune(&tune_no_change);
    return 0;
  }

  playTune((*bits & bit)? onTune: offTune);
  return 1;
}

int
toggleSetting (
  unsigned char *setting, int command,
  const TuneDefinition *offTune, const TuneDefinition *onTune
) {
  unsigned char oldSetting = *setting;

  switch (command & BRL_FLG_TOGGLE_MASK) {
    case 0:
      *setting = !*setting;
      break;

    case BRL_FLG_TOGGLE_ON:
      *setting = 1;
      break;

    case BRL_FLG_TOGGLE_OFF:
      *setting = 0;
      break;

    default:
      playTune(&tune_command_rejected);
      return 0;
  }

  if (*setting == oldSetting) {
    playTune(&tune_no_change);
    return 0;
  }

  playTune(*setting? onTune: offTune);
  return 1;
}

int
toggleModeSetting (unsigned char *setting, int command) {
  return toggleSetting(setting, command, NULL, NULL);
}

int
toggleFeatureSetting (unsigned char *setting, int command) {
  return toggleSetting(setting, command, &tune_toggle_off, &tune_toggle_on);
}

static const unsigned char cursorStyles[] = {
  [csUnderline] = (BRL_DOT7 | BRL_DOT8),
  [csBlock] = (BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8),
  [csLowerLeftDot] = (BRL_DOT7),
  [csLowerRightDot] = (BRL_DOT8)
};

unsigned char
getCursorDots (void) {
  if (!isBlinkedOn(&cursorBlinkingState)) return 0;
  return cursorStyles[prefs.cursorStyle];
}

static int
getCursorPosition (int x, int y) {
  int position = -1;

#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracted) {
    int uncontractedOffset = getUncontractedCursorOffset(x, y);

    if (uncontractedOffset < contractedLength) {
      while (uncontractedOffset >= 0) {
        int contractedOffset = contractedOffsets[uncontractedOffset];
        if (contractedOffset != CTB_NO_OFFSET) {
          position = ((contractedOffset / textCount) * brl.textColumns) + textStart + (contractedOffset % textCount);
          break;
        }

        uncontractedOffset -= 1;
      }
    }
  } else
#endif /* ENABLE_CONTRACTED_BRAILLE */

  {
    if ((x >= ses->winx) && (x < (int)(ses->winx + textCount)) &&
        (y >= ses->winy) && (y < (int)(ses->winy + brl.textRows)) &&
        (x < scr.cols) && (y < scr.rows))
      position = ((y - ses->winy) * brl.textColumns) + textStart + x - ses->winx;
  }

  return position;
}

int
showCursor (void) {
  return scr.cursor && prefs.showCursor && !ses->hideCursor;
}

static inline int
showAttributesUnderline (void) {
  return prefs.showAttributes && isBlinkedOn(&attributesBlinkingState);
}

static void
overlayAttributesUnderline (unsigned char *cell, unsigned char attributes) {
  unsigned char dots;

  switch (attributes) {
    case SCR_COLOUR_FG_DARK_GREY | SCR_COLOUR_BG_BLACK:
    case SCR_COLOUR_FG_LIGHT_GREY | SCR_COLOUR_BG_BLACK:
    case SCR_COLOUR_FG_LIGHT_GREY | SCR_COLOUR_BG_BLUE:
    case SCR_COLOUR_FG_BLACK | SCR_COLOUR_BG_CYAN:
      return;

    case SCR_COLOUR_FG_BLACK | SCR_COLOUR_BG_LIGHT_GREY:
      dots = BRL_DOT7 | BRL_DOT8;
      break;

    case SCR_COLOUR_FG_WHITE | SCR_COLOUR_BG_BLACK:
    default:
      dots = BRL_DOT8;
      break;
  }

  *cell |= dots;
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
isSameText (
  const ScreenCharacter *character1,
  const ScreenCharacter *character2
) {
  return character1->text == character2->text;
}

int
isSameAttributes (
  const ScreenCharacter *character1,
  const ScreenCharacter *character2
) {
  return character1->attributes == character2->attributes;
}

int
isSameRow (
  const ScreenCharacter *characters1,
  const ScreenCharacter *characters2,
  int count,
  IsSameCharacter isSameCharacter
) {
  int i;
  for (i=0; i<count; ++i)
    if (!isSameCharacter(&characters1[i], &characters2[i]))
      return 0;

  return 1;
}

int restartRequired;
int isOffline;
int isSuspended;
int inputModifiers;

void
resetBrailleState (void) {
  resetScanCodes();
  resetBlinkingStates();
  inputModifiers = 0;
}

static int
checkPointer (void) {
  int moved = 0;
  int column, row;

  if (prefs.windowFollowsPointer && getScreenPointer(&column, &row)) {
    if (column != ses->ptrx) {
      if (ses->ptrx >= 0) moved = 1;
      ses->ptrx = column;
    }

    if (row != ses->ptry) {
      if (ses->ptry >= 0) moved = 1;
      ses->ptry = row;
    }

    if (moved) {
      if (column < ses->winx) {
        ses->winx = column;
      } else if (column >= (int)(ses->winx + textCount)) {
        ses->winx = column + 1 - textCount;
      }

      if (row < ses->winy) {
        ses->winy = row;
      } else if (row >= (int)(ses->winy + brl.textRows)) {
        ses->winy = row + 1 - brl.textRows;
      }
    }
  } else {
    ses->ptrx = ses->ptry = -1;
  }

  return moved;
}

static void
highlightWindow (void) {
  if (prefs.highlightWindow) {
    int left = 0;
    int right;
    int top = 0;
    int bottom;

    if (prefs.showAttributes) {
      right = left;
      bottom = top;
    } else if (!brl.touchEnabled) {
      right = textCount - 1;
      bottom = brl.textRows - 1;
    } else {
      int clear = 1;

      if (touchGetRegion(&left, &right, &top, &bottom)) {
        left = MAX(left, textStart);
        right = MIN(right, textStart+textCount-1);
        if (isTextOffset(&left, 0, 0) && isTextOffset(&right, 1, 1)) clear = 0;
      }

      if (clear) {
        unhighlightScreenRegion();
        return;
      }
    }

    highlightScreenRegion(ses->winx+left, ses->winx+right, ses->winy+top, ses->winy+bottom);
  }
}

static int oldwinx;
static int oldwiny;

#ifdef ENABLE_SPEECH_SUPPORT
static void
doAutospeak (void) {
  static int oldScreen = -1;
  static int oldX = -1;
  static int oldY = -1;
  static int oldWidth = 0;
  static ScreenCharacter *oldCharacters = NULL;

  int newScreen = scr.number;
  int newX = scr.posx;
  int newY = scr.posy;
  int newWidth = scr.cols;
  ScreenCharacter newCharacters[newWidth];

  readScreen(0, ses->winy, newWidth, 1, newCharacters);

  if (!speechTracking) {
    int column = 0;
    int count = newWidth;
    const ScreenCharacter *characters = newCharacters;

    if (!oldCharacters) {
      count = 0;
    } else if ((newScreen != oldScreen) || (ses->winy != oldwiny) || (newWidth != oldWidth)) {
      if (!prefs.autospeakSelectedLine) count = 0;
    } else {
      int onScreen = (newX >= 0) && (newX < newWidth);

      if (!isSameRow(newCharacters, oldCharacters, newWidth, isSameText)) {
        if ((newY == ses->winy) && (newY == oldY) && onScreen) {
          if ((newX == oldX) &&
              isSameRow(newCharacters, oldCharacters, newX, isSameText)) {
            int oldLength = oldWidth;
            int newLength = newWidth;
            int x = newX;

            while (oldLength > oldX) {
              if (!iswspace(oldCharacters[oldLength-1].text)) break;
              oldLength -= 1;
            }
            if (oldLength < oldWidth) oldLength += 1;

            while (newLength > newX) {
              if (!iswspace(newCharacters[newLength-1].text)) break;
              newLength -= 1;
            }
            if (newLength < newWidth) newLength += 1;

            while (1) {
              int done = 1;

              if (x < newLength) {
                if (isSameRow(newCharacters+x, oldCharacters+oldX, newWidth-x, isSameText)) {
                  column = newX;
                  count = prefs.autospeakInsertedCharacters? (x - newX): 0;
                  goto autospeak;
                }

                done = 0;
              }

              if (x < oldLength) {
                if (isSameRow(newCharacters+newX, oldCharacters+x, oldWidth-x, isSameText)) {
                  characters = oldCharacters;
                  column = oldX;
                  count = prefs.autospeakDeletedCharacters? (x - oldX): 0;
                  goto autospeak;
                }

                done = 0;
              }

              if (done) break;
              x += 1;
            }
          }

          if (oldX < 0) oldX = 0;
          if ((newX > oldX) &&
              isSameRow(newCharacters, oldCharacters, oldX, isSameText) &&
              isSameRow(newCharacters+newX, oldCharacters+oldX, newWidth-newX, isSameText)) {
            column = oldX;
            count = newX - oldX;

            if (prefs.autospeakCompletedWords) {
              int last = column + count - 1;

              if (iswspace(characters[last].text)) {
                int first = column;

                while (first > 0) {
                  if (iswspace(characters[--first].text)) {
                    first += 1;
                    break;
                  }
                }

                if (first < column) {
                  while (last >= first) {
                    if (!iswspace(characters[last].text)) break;
                    last -= 1;
                  }

                  if (last > first) {
                    column = first;
                    count = last - first + 1;
                    goto autospeak;
                  }
                }
              }
            }

            if (!prefs.autospeakInsertedCharacters) count = 0;
            goto autospeak;
          }

          if (oldX >= oldWidth) oldX = oldWidth - 1;
          if ((newX < oldX) &&
              isSameRow(newCharacters, oldCharacters, newX, isSameText) &&
              isSameRow(newCharacters+newX, oldCharacters+oldX, oldWidth-oldX, isSameText)) {
            characters = oldCharacters;
            column = newX;
            count = prefs.autospeakDeletedCharacters? (oldX - newX): 0;
            goto autospeak;
          }
        }

        while (newCharacters[column].text == oldCharacters[column].text) ++column;
        while (newCharacters[count-1].text == oldCharacters[count-1].text) --count;
        count -= column;
        if (!prefs.autospeakReplacedCharacters) count = 0;
      } else if ((newY == ses->winy) && ((newX != oldX) || (newY != oldY)) && onScreen) {
        column = newX;
        count = prefs.autospeakSelectedCharacter? 1: 0;

        if (prefs.autospeakCompletedWords) {
          if ((newX > oldX) && (column >= 2)) {
            int length = newWidth;

            while (length > 0) {
              if (!iswspace(characters[--length].text)) {
                length += 1;
                break;
              }
            }

            if ((length + 1) == column) {
              int first = length - 1;

              while (first > 0) {
                if (iswspace(characters[--first].text)) {
                  first += 1;
                  break;
                }
              }

              if ((length -= first) > 1) {
                column = first;
                count = length;
                goto autospeak;
              }
            }
          }
        }
      } else {
        count = 0;
      }
    }

  autospeak:
    characters += column;

    if (count) speakCharacters(characters, count, 0);
  }

  {
    size_t size = newWidth * sizeof(*oldCharacters);

    if ((oldCharacters = realloc(oldCharacters, size))) {
      memcpy(oldCharacters, newCharacters, size);
    } else {
      logMallocError();
    }
  }

  oldScreen = newScreen;
  oldX = newX;
  oldY = newY;
  oldWidth = newWidth;
}
#endif /* ENABLE_SPEECH_SUPPORT */

static int
canBraille (void) {
  return braille && brl.buffer && !brl.noDisplay && !isSuspended;
}

static TimeValue updateTime;
static AsyncHandle updateAlarm;
static int updateSuspendCount;

static void
setUpdateTime (int delay) {
  getRelativeTime(&updateTime, delay);
}

void
resetUpdateAlarm (int delay) {
  setUpdateTime(delay);
  if (updateAlarm) asyncResetAlarmTo(updateAlarm, &updateTime);
}

static void setUpdateAlarm (void *data);

static void
handleUpdateAlarm (const AsyncAlarmResult *result) {
  setUpdateTime(updateInterval);
  asyncDiscardHandle(updateAlarm);
  updateAlarm = NULL;

#ifdef ENABLE_SPEECH_SUPPORT
  speech->doTrack(&spk);
  if (speechTracking && !speech->isSpeaking(&spk)) speechTracking = 0;
#endif /* ENABLE_SPEECH_SUPPORT */

  if (opt_releaseDevice) {
    if (scr.unreadable) {
      if (canBraille()) {
        logMessage(LOG_DEBUG, "suspending braille driver");
        writeStatusCells();
        writeBrailleText("wrn", scr.unreadable);

#ifdef ENABLE_API
        if (apiStarted) {
          api_suspend(&brl);
        } else
#endif /* ENABLE_API */

        {
          destructBrailleDriver();
        }

        isSuspended = 1;
        logMessage(LOG_DEBUG, "braille driver suspended");
      }
    } else {
      if (isSuspended) {
        logMessage(LOG_DEBUG, "resuming braille driver");
        isSuspended = !(
#ifdef ENABLE_API
          apiStarted? api_resume(&brl):
#endif /* ENABLE_API */
          constructBrailleDriver());
        logMessage(LOG_DEBUG, "braille driver resumed");
      }
    }
  }

  if (canBraille()) {
    int pointerMoved = 0;

#ifdef ENABLE_API
    apiClaimDriver();
#endif /*  ENABLE_API */

    if (brl.highlightWindow) {
      brl.highlightWindow = 0;
      highlightWindow();
    }

    updateSessionAttributes();
    updateBlinkingState(&cursorBlinkingState);
    updateBlinkingState(&attributesBlinkingState);
    updateBlinkingState(&capitalsBlinkingState);
    updateBlinkingState(&speechCursorBlinkingState);

    if (ses->trackCursor) {
#ifdef ENABLE_SPEECH_SUPPORT
      if (speechTracking && (scr.number == speechScreen)) {
        int index = speech->getTrack(&spk);
        if (index != speechIndex) trackSpeech(speechIndex = index);
      } else
#endif /* ENABLE_SPEECH_SUPPORT */
      {
        /* If cursor moves while blinking is on */
        if (prefs.blinkingCursor) {
          if (scr.posy != ses->trky) {
            /* turn off cursor to see what's under it while changing lines */
            setBlinkingState(&cursorBlinkingState, 0);
          } else if (scr.posx != ses->trkx) {
            /* turn on cursor to see it moving on the line */
            setBlinkingState(&cursorBlinkingState, 1);
          }
        }

        /* If the cursor moves in cursor tracking mode: */
        if (!isRouting() && (scr.posx != ses->trkx || scr.posy != ses->trky)) {
          int oldx = ses->winx;
          int oldy = ses->winy;

          trackCursor(0);
          logMessage(LOG_CATEGORY(CURSOR_TRACKING),
                     "cursor tracking: scr=%u csr=[%u,%u]->[%u,%u] win=[%u,%u]->[%u,%u]",
                     scr.number,
                     ses->trkx, ses->trky, scr.posx, scr.posy,
                     oldx, oldy, ses->winx, ses->winy);

          ses->spkx = ses->trkx = scr.posx;
          ses->spky = ses->trky = scr.posy;
        } else if (checkPointer()) {
          pointerMoved = 1;
        }
      }
    }

    /* There are a few things to take care of if the display has moved. */
    if ((ses->winx != oldwinx) || (ses->winy != oldwiny)) {
      if (!pointerMoved) highlightWindow();

      if (prefs.showAttributes && prefs.blinkingAttributes) {
        /* Attributes are blinking.
           We could check to see if we changed screen, but that doesn't
           really matter... this is mainly for when you are hunting up/down
           for the line with attributes. */
        setBlinkingState(&attributesBlinkingState, 1);
        /* problem: this still doesn't help when the braille window is
           stationnary and the attributes themselves are moving
           (example: tin). */
      }

      if ((ses->spky < ses->winy) || (ses->spky >= (ses->winy + brl.textRows))) ses->spky = ses->winy;
      if ((ses->spkx < ses->winx) || (ses->spkx >= (ses->winx + textCount))) ses->spkx = ses->winx;

      oldwinx = ses->winx;
      oldwiny = ses->winy;
    }

    if (!isOffline && canBraille()) {
      if (infoMode) {
        if (!showInfo()) restartRequired = 1;
      } else {
        const unsigned int windowLength = brl.textColumns * brl.textRows;
        const unsigned int textLength = textCount * brl.textRows;
        wchar_t textBuffer[windowLength];

        memset(brl.buffer, 0, windowLength);
        wmemset(textBuffer, WC_C(' '), windowLength);

#ifdef ENABLE_CONTRACTED_BRAILLE
        isContracted = 0;

        if (isContracting()) {
          while (1) {
            int inputLength = scr.cols - ses->winx;
            ScreenCharacter inputCharacters[inputLength];
            wchar_t inputText[inputLength];

            int outputLength = textLength;
            unsigned char outputBuffer[outputLength];

            readScreen(ses->winx, ses->winy, inputLength, 1, inputCharacters);

            {
              int i;
              for (i=0; i<inputLength; ++i) {
                inputText[i] = inputCharacters[i].text;
              }
            }

            contractText(contractionTable,
                         inputText, &inputLength,
                         outputBuffer, &outputLength,
                         contractedOffsets, getContractedCursor());

            {
              int inputEnd = inputLength;

              if (contractedTrack) {
                if (outputLength == textLength) {
                  int inputIndex = inputEnd;
                  while (inputIndex) {
                    int offset = contractedOffsets[--inputIndex];
                    if (offset != CTB_NO_OFFSET) {
                      if (offset != outputLength) break;
                      inputEnd = inputIndex;
                    }
                  }
                }

                if (scr.posx >= (ses->winx + inputEnd)) {
                  int offset = 0;
                  int length = scr.cols - ses->winx;
                  int onspace = 0;

                  while (offset < length) {
                    if ((iswspace(inputCharacters[offset].text) != 0) != onspace) {
                      if (onspace) break;
                      onspace = 1;
                    }
                    ++offset;
                  }

                  if ((offset += ses->winx) > scr.posx) {
                    ses->winx = (ses->winx + scr.posx) / 2;
                  } else {
                    ses->winx = offset;
                  }

                  continue;
                }
              }
            }

            contractedStart = ses->winx;
            contractedLength = inputLength;
            contractedTrack = 0;
            isContracted = 1;

            if (ses->displayMode || showAttributesUnderline()) {
              int inputOffset;
              int outputOffset = 0;
              unsigned char attributes = 0;
              unsigned char attributesBuffer[outputLength];

              for (inputOffset=0; inputOffset<contractedLength; ++inputOffset) {
                int offset = contractedOffsets[inputOffset];
                if (offset != CTB_NO_OFFSET) {
                  while (outputOffset < offset) attributesBuffer[outputOffset++] = attributes;
                  attributes = 0;
                }
                attributes |= inputCharacters[inputOffset].attributes;
              }
              while (outputOffset < outputLength) attributesBuffer[outputOffset++] = attributes;

              if (ses->displayMode) {
                for (outputOffset=0; outputOffset<outputLength; ++outputOffset) {
                  outputBuffer[outputOffset] = convertAttributesToDots(attributesTable, attributesBuffer[outputOffset]);
                }
              } else {
                int i;
                for (i=0; i<outputLength; ++i) {
                  overlayAttributesUnderline(&outputBuffer[i], attributesBuffer[i]);
                }
              }
            }

            fillDotsRegion(textBuffer, brl.buffer,
                           textStart, textCount, brl.textColumns, brl.textRows,
                           outputBuffer, outputLength);
            break;
          }
        }

        if (!isContracted)
#endif /* ENABLE_CONTRACTED_BRAILLE */
        {
          int windowColumns = MIN(textCount, scr.cols-ses->winx);
          ScreenCharacter characters[textLength];

          readScreen(ses->winx, ses->winy, windowColumns, brl.textRows, characters);
          if (windowColumns < textCount) {
            /* We got a rectangular piece of text with readScreen but the display
             * is in an off-right position with some cells at the end blank
             * so we'll insert these cells and blank them.
             */
            int i;

            for (i=brl.textRows-1; i>0; i--) {
              memmove(characters + (i * textCount),
                      characters + (i * windowColumns),
                      windowColumns * sizeof(*characters));
            }

            for (i=0; i<brl.textRows; i++) {
              clearScreenCharacters(characters + (i * textCount) + windowColumns,
                                    textCount-windowColumns);
            }
          }

          /* blank out capital letters if they're blinking and should be off */
          if (!isBlinkedOn(&capitalsBlinkingState)) {
            unsigned int i;
            for (i=0; i<textCount*brl.textRows; i+=1) {
              ScreenCharacter *character = &characters[i];
              if (iswupper(character->text)) character->text = WC_C(' ');
            }
          }

          /* convert to dots using the current translation table */
          if (ses->displayMode) {
            int row;

            for (row=0; row<brl.textRows; row+=1) {
              const ScreenCharacter *source = &characters[row * textCount];
              unsigned int start = (row * brl.textColumns) + textStart;
              unsigned char *target = &brl.buffer[start];
              wchar_t *text = &textBuffer[start];
              int column;

              for (column=0; column<textCount; column+=1) {
                text[column] = UNICODE_BRAILLE_ROW | (target[column] = convertAttributesToDots(attributesTable, source[column].attributes));
              }
            }
          } else {
            int underline = showAttributesUnderline();
            int row;

            for (row=0; row<brl.textRows; row+=1) {
              const ScreenCharacter *source = &characters[row * textCount];
              unsigned int start = (row * brl.textColumns) + textStart;
              unsigned char *target = &brl.buffer[start];
              wchar_t *text = &textBuffer[start];
              int column;

              for (column=0; column<textCount; column+=1) {
                const ScreenCharacter *character = &source[column];
                unsigned char *dots = &target[column];

                *dots = convertCharacterToDots(textTable, character->text);
                if (prefs.textStyle) *dots &= ~(BRL_DOT7 | BRL_DOT8);
                if (underline) overlayAttributesUnderline(dots, character->attributes);

                text[column] = character->text;
              }
            }
          }
        }

        if ((brl.cursor = getCursorPosition(scr.posx, scr.posy)) >= 0) {
          if (showCursor()) {
            brl.buffer[brl.cursor] |= getCursorDots();
          }
        }

        if (prefs.showSpeechCursor && isBlinkedOn(&speechCursorBlinkingState)) {
          int position = getCursorPosition(ses->spkx, ses->spky);

          if (position >= 0)
            if (position != brl.cursor)
              brl.buffer[position] |= cursorStyles[prefs.speechCursorStyle];
        }

        if (statusCount > 0) {
          const unsigned char *fields = prefs.statusFields;
          unsigned int length = getStatusFieldsLength(fields);

          if (length > 0) {
            unsigned char cells[length];
            memset(cells, 0, length);
            renderStatusFields(fields, cells);
            fillDotsRegion(textBuffer, brl.buffer,
                           statusStart, statusCount, brl.textColumns, brl.textRows,
                           cells, length);
          }

          fillStatusSeparator(textBuffer, brl.buffer);
        }

        if (!(writeStatusCells() && braille->writeWindow(&brl, textBuffer))) restartRequired = 1;
      }
    }

#ifdef ENABLE_API
    apiReleaseDriver();
#endif /*  ENABLE_API */
  }

#ifdef ENABLE_SPEECH_SUPPORT
  if (autospeak()) doAutospeak();
  processSpeechInput(&spk);
#endif /* ENABLE_SPEECH_SUPPORT */

  drainBrailleOutput(&brl, 0);
  setUpdateAlarm(result->data);
}

static void
setUpdateAlarm (void *data) {
  if (!updateSuspendCount && !updateAlarm) {
    asyncSetAlarmTo(&updateAlarm, &updateTime, handleUpdateAlarm, data);
  }
}

static void
startUpdates (void) {
  setUpdateTime(0);
  updateAlarm = NULL;
  updateSuspendCount = 1;
}

void
suspendUpdates (void) {
  if (updateAlarm) {
    asyncCancelRequest(updateAlarm);
    updateAlarm = NULL;
  }

  updateSuspendCount += 1;
}

void
resumeUpdates (void) {
  if (!--updateSuspendCount) setUpdateAlarm(NULL);
}

ProgramExitStatus
brlttyConstruct (int argc, char *argv[]) {
  srand((unsigned int)time(NULL));
  onProgramExit("log", exitLog, NULL);
  openSystemLog();

  terminationCount = 0;
  terminationTime = time(NULL);

#ifdef SIGPIPE
  /* We ignore SIGPIPE before calling brlttyStart() so that a driver which uses
   * a broken pipe won't abort program execution.
   */
  handleSignal(SIGPIPE, SIG_IGN);
#endif /* SIGPIPE */

#ifdef SIGTERM
  handleSignal(SIGTERM, handleTerminationRequest);
#endif /* SIGTERM */

#ifdef SIGINT
  handleSignal(SIGINT, handleTerminationRequest);
#endif /* SIGINT */

  startUpdates();

  {
    ProgramExitStatus exitStatus = brlttyStart(argc, argv);
    if (exitStatus != PROG_EXIT_SUCCESS) return exitStatus;
  }

  onProgramExit("sessions", exitSessions, NULL);
  setSessionEntry();
  ses->trkx = scr.posx; ses->trky = scr.posy;
  if (!trackCursor(1)) ses->winx = ses->winy = 0;
  ses->motx = ses->winx; ses->moty = ses->winy;
  ses->spkx = ses->winx; ses->spky = ses->winy;

  oldwinx = ses->winx; oldwiny = ses->winy;
  restartRequired = 0;
  isOffline = 0;
  isSuspended = 0;

  highlightWindow();
  checkPointer();
  resetBrailleState();

  resumeUpdates();
  return PROG_EXIT_SUCCESS;
}

int
brlttyDestruct (void) {
  endProgram();
  return 1;
}

typedef void UnmonitoredConditionHandler (const void *data);

static void
handleRoutingDone (const void *data) {
  const TuneDefinition *tune = data;

  playTune(tune);
  ses->spkx = scr.posx;
  ses->spky = scr.posy;
}

typedef struct {
  UnmonitoredConditionHandler *handler;
  const void *data;
} UnmonitoredConditionDescriptor;

static int
checkUnmonitoredConditions (void *data) {
  UnmonitoredConditionDescriptor *ucd = data;

  if (terminationCount) {
    return 1;
  }

  {
    RoutingStatus status = getRoutingStatus(0);

    if (status != ROUTING_NONE) {
      ucd->handler = handleRoutingDone;
      ucd->data = (status > ROUTING_DONE)? &tune_routing_failed: &tune_routing_succeeded;
      return 1;
    }
  }

  return 0;
}

int
brlttyUpdate (void) {
  UnmonitoredConditionDescriptor ucd = {
    .handler = NULL,
    .data = NULL
  };

  if (asyncAwaitCondition(40, checkUnmonitoredConditions, &ucd)) {
    if (!ucd.handler) return 0;
    ucd.handler(ucd.data);
  }

  return 1;
}

typedef struct {
  unsigned endWait:1;
} MessageData;

static int
testEndMessageWait (void *data) {
  MessageData *msg = data;

  return msg->endWait;
}

static int
handleMessageCommand (int command, void *data) {
  MessageData *msg = data;

  msg->endWait = 1;
  return 1;
}

int 
message (const char *mode, const char *text, short flags) {
  int ok = 1;

  if (!mode) mode = "";

#ifdef ENABLE_SPEECH_SUPPORT
  if (!(flags & MSG_SILENT)) {
    if (autospeak()) {
      sayString(&spk, text, 1);
    }
  }
#endif /* ENABLE_SPEECH_SUPPORT */

  if (canBraille()) {
    MessageData msg;

    size_t size = textCount * brl.textRows;
    wchar_t buffer[size];

    size_t length = getTextLength(text);
    wchar_t characters[length + 1];
    const wchar_t *character = characters;

#ifdef ENABLE_API
    int api = apiStarted;
    apiStarted = 0;
    if (api) api_unlink(&brl);
#endif /* ENABLE_API */

    convertTextToWchars(characters, text, ARRAY_COUNT(characters));
    suspendUpdates();
    pushCommandHandler(KTB_CTX_WAITING, handleMessageCommand, &msg);

    while (length) {
      size_t count;

      /* strip leading spaces */
      while (iswspace(*character)) {
        character += 1;
        length -= 1;
      }

      if (length <= size) {
        count = length; /* the whole message fits in the braille window */
      } else {
        /* split the message across multiple windows on space characters */
        for (count=size-2; count>0 && iswspace(characters[count]); count-=1);
        count = count? count+1: size-1;
      }

      wmemcpy(buffer, character, count);
      character += count;

      if (length -= count) {
        while (count < size) buffer[count++] = WC_C('-');
        buffer[count - 1] = WC_C('>');
      }

      if (!writeBrailleCharacters(mode, buffer, count)) {
        ok = 0;
        break;
      }

      if (length || !(flags & MSG_NODELAY)) {
        msg.endWait = 0;

        do {
          if (asyncAwaitCondition(messageDelay, testEndMessageWait, &msg)) break;
        } while (flags & MSG_WAITKEY);
      }
    }

    popCommandHandler();
    resumeUpdates();

#ifdef ENABLE_API
    if (api) api_link(&brl);
    apiStarted = api;
#endif /* ENABLE_API */
  }

  return ok;
}

int
showDotPattern (unsigned char dots, unsigned char duration) {
  if (braille->writeStatus && (brl.statusColumns > 0)) {
    unsigned int length = brl.statusColumns * brl.statusRows;
    unsigned char cells[length];        /* status cell buffer */
    memset(cells, dots, length);
    if (!braille->writeStatus(&brl, cells)) return 0;
  }

  memset(brl.buffer, dots, brl.textColumns*brl.textRows);
  if (!braille->writeWindow(&brl, NULL)) return 0;

  drainBrailleOutput(&brl, duration);
  return 1;
}
