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

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif /* HAVE_LANGINFO_H */

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif /* HAVE_SIGNAL_H */

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

#ifdef HAVE_ICU
#include <unicode/uchar.h>
#endif /* HAVE_ICU */

#include "embed.h"
#include "log.h"
#include "parse.h"
#include "message.h"
#include "tunes.h"
#include "ttb.h"
#include "atb.h"
#include "ctb.h"
#include "routing.h"
#include "clipboard.h"
#include "touch.h"
#include "cmd.h"
#include "charset.h"
#include "unicode.h"
#include "scancodes.h"
#include "scr.h"
#include "status.h"
#include "ses.h"
#include "brl.h"
#include "brltty.h"
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
unsigned int textStart;
unsigned int textCount;
unsigned char textMaximized = 0;
unsigned int statusStart;
unsigned int statusCount;
unsigned int fullWindowShift;                /* Full window horizontal distance */
unsigned int halfWindowShift;                /* Half window horizontal distance */
unsigned int verticalWindowShift;                /* Window vertical distance */

#ifdef ENABLE_CONTRACTED_BRAILLE
static int contracted = 0;
static int contractedLength;
static int contractedStart;
static int contractedOffsets[0X100];
static int contractedTrack = 0;
#endif /* ENABLE_CONTRACTED_BRAILLE */

ScreenDescription scr;
SessionEntry *ses = NULL;
unsigned int updateIntervals = 0;
unsigned char infoMode = 0;

static void
setSessionEntry (void) {
  describeScreen(&scr);
  if (scr.number == -1) scr.number = userVirtualTerminal(0);
  if (!ses || (scr.number != ses->number)) ses = getSessionEntry(scr.number);
}

static void
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
exitSessions (void) {
  ses = NULL;
  deallocateSessionEntries();
}

static void
exitLog (void) {
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

typedef struct {
  TimeComponents time;
  const char *meridian;
} TimeFormattingData;

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

static size_t
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

static void
doBrailleTime (const TimeFormattingData *fmt) {
  char buffer[0X80];

  formatBrailleTime(buffer, sizeof(buffer), fmt);
  message(NULL, buffer, MSG_SILENT);
}

#ifdef ENABLE_SPEECH_SUPPORT
static size_t
formatSpeechTime (char *buffer, size_t size, const TimeFormattingData *fmt) {
  size_t length;

  STR_BEGIN(buffer, size);
  STR_PRINTF("%u", fmt->time.hour);
  if (fmt->time.minute < 10) STR_PRINTF(" 0");
  STR_PRINTF(" %u", fmt->time.minute);

  if (fmt->meridian) {
    const char *character = fmt->meridian;
    while (*character) STR_PRINTF(" %c", *character++);
  }

  if (prefs.showSeconds) {
    STR_PRINTF(", ");

    if (fmt->time.second == 0) {
      STR_PRINTF("%s", gettext("exactly"));
    } else {
      STR_PRINTF("%s %u %s",
                 gettext("and"), fmt->time.second,
                 ngettext("second", "seconds", fmt->time.second));
    }
  }

  length = STR_LENGTH;
  STR_END

  return length;
}

static size_t
formatSpeechDate (char *buffer, size_t size, const TimeFormattingData *fmt) {
  size_t length;

  const char *yearFormat = "%u";
  const char *monthFormat = "%s";
  const char *dayFormat = "%u";

  const char *month;
  uint8_t day = fmt->time.day + 1;

#ifdef MON_1
  {
    static const int months[] = {
      MON_1, MON_2, MON_3, MON_4, MON_5, MON_6,
      MON_7, MON_8, MON_9, MON_10, MON_11, MON_12
    };

    month = (fmt->time.month < ARRAY_COUNT(months))? nl_langinfo(months[fmt->time.month]): "?";
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

    month = (fmt->time.month < ARRAY_COUNT(months))? gettext(months[fmt->time.month]): "?";
  }
#endif /* MON_1 */

  STR_BEGIN(buffer, size);

  switch (prefs.dateFormat) {
    default:
    case dfYearMonthDay:
      STR_PRINTF(yearFormat, fmt->time.year);
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
      STR_PRINTF(yearFormat, fmt->time.year);
      break;

    case dfDayMonthYear:
      STR_PRINTF(dayFormat, day);
      STR_PRINTF(" ");
      STR_PRINTF(monthFormat, month);
      STR_PRINTF(", ");
      STR_PRINTF(yearFormat, fmt->time.year);
      break;
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
  sayString(&spk, announcement, 1);
}
#endif /* ENABLE_SPEECH_SUPPORT */

static void
getTimeFormattingData (TimeFormattingData *fmt) {
  TimeValue now;

  getCurrentTime(&now);
  expandTimeValue(&now, &fmt->time);
  fmt->meridian = getMeridianString(&fmt->time.hour);
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

static void 
slideWindowVertically (int y) {
  if (y < ses->winy)
    ses->winy = y;
  else if (y >= (int)(ses->winy + brl.textRows))
    ses->winy = y - (brl.textRows - 1);
}

static void 
placeWindowHorizontally (int x) {
  if (prefs.slidingWindow) {
    ses->winx = MAX(0, (x - (int)(textCount / 2)));
  } else {
    ses->winx = x / textCount * textCount;
  }
}

static int
trackCursor (int place) {
  if (!SCR_CURSOR_OK()) return 0;

#ifdef ENABLE_CONTRACTED_BRAILLE
  if (contracted) {
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

#ifdef ENABLE_SPEECH_SUPPORT
SpeechSynthesizer spk;
static int speechTracking = 0;
static int speechScreen = -1;
static int speechLine = 0;
static int speechIndex = -1;

static void
trackSpeech (int index) {
  placeWindowHorizontally(index % scr.cols);
  slideWindowVertically((index / scr.cols) + speechLine);
}

static void
sayWideCharacters (const wchar_t *characters, const unsigned char *attributes, size_t count, int immediate) {
  size_t length;
  char *text = makeUtf8FromWchars(characters, count, &length);

  if (text) {
    if (immediate) speech->mute(&spk);
    speech->say(&spk, text, length, count, attributes);
    free(text);
  } else {
    logMallocError();
  }
}

static void
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

static void
sayScreenRegion (int left, int top, int width, int height, int track, SayMode mode) {
  size_t count = width * height;
  ScreenCharacter characters[count];

  readScreen(left, top, width, height, characters);
  sayScreenCharacters(characters, count, mode==sayImmediate);

  speechTracking = track;
  speechScreen = scr.number;
  speechLine = top;
}

static void
sayScreenLines (int line, int count, int track, SayMode mode) {
  sayScreenRegion(0, line, scr.cols, count, track, mode);
}

typedef enum {
  SCT_WORD,
  SCT_NONWORD,
  SCT_SPACE
} ScreenCharacterType;

static ScreenCharacterType
getScreenCharacterType (const ScreenCharacter *character) {
  if (iswspace(character->text)) return SCT_SPACE;
  if (iswalnum(character->text)) return SCT_WORD;
  if (wcschr(WS_C("_"), character->text)) return SCT_WORD;
  return SCT_NONWORD;
}

static int
findFirstNonSpaceCharacter (const ScreenCharacter *characters, int count) {
  int index = 0;

  while (index < count) {
    if (getScreenCharacterType(&characters[index]) != SCT_SPACE) return index;
    index += 1;
  }

  return -1;
}

static int
findLastNonSpaceCharacter (const ScreenCharacter *characters, int count) {
  int index = count;

  while (index > 0)
    if (getScreenCharacterType(&characters[--index]) != SCT_SPACE)
      return index;

  return -1;
}

static int
isAllSpaceCharacters (const ScreenCharacter *characters, int count) {
  return findFirstNonSpaceCharacter(characters, count) < 0;
}

static void
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

static void
speakDone (const ScreenCharacter *line, int column, int count, int spell) {
  ScreenCharacter internalBuffer[count];

  if (line) {
    line = &line[column];
  } else {
    readScreen(column, ses->spky, count, 1, internalBuffer);
    line = internalBuffer;
  }

  speakCharacters(line, count, spell);
  placeWindowHorizontally(ses->spkx);
  slideWindowVertically(ses->spky);
}

static void
speakCurrentCharacter (void) {
  speakDone(NULL, ses->spkx, 1, 0);
}

static void
speakCurrentLine (void) {
  speakDone(NULL, 0, scr.cols, 0);
}

static int
autospeak (void) {
  if (speech->definition.code == noSpeech.definition.code) return 0;
  if (prefs.autospeak) return 1;
  return braille->definition.code == noBraille.definition.code;
}
#endif /* ENABLE_SPEECH_SUPPORT */

static inline int
showCursor (void) {
  return scr.cursor && prefs.showCursor && !ses->hideCursor;
}

typedef int (*IsSameCharacter) (
  const ScreenCharacter *character1,
  const ScreenCharacter *character2
);

static int
isSameText (
  const ScreenCharacter *character1,
  const ScreenCharacter *character2
) {
  return character1->text == character2->text;
}

static int
isSameAttributes (
  const ScreenCharacter *character1,
  const ScreenCharacter *character2
) {
  return character1->attributes == character2->attributes;
}

static int
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
        playTune(&tune_skip_first);
      } else if (skipped <= 4) {
        playTune(&tune_skip);
      } else if (skipped % 4 == 0) {
        playTune(&tune_skip_more);
      }
      skipped++;
    } while (canMoveWindow());
  }

  playTune(&tune_bounce);
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
  if (ses->winy > 0) {
    ses->winy--;
  } else {
    playTune(&tune_bounce);
  }
}

static void
downOneLine (void) {
  if (ses->winy < (int)(scr.rows - brl.textRows)) {
    ses->winy++;
  } else {
    playTune(&tune_bounce);
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
  playTune(&tune_bounce);
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

#ifdef ENABLE_CONTRACTED_BRAILLE
static int
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

static int
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

static void
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

static void
placeWindowRight (void) {
  placeRightEdge(scr.cols-1);
}

static int
moveWindowLeft (unsigned int amount) {
  if (ses->winx < 1) return 0;
  if (amount < 1) return 0;

  ses->winx -= MIN(ses->winx, amount);
  return 1;
}

static int
moveWindowRight (unsigned int amount) {
  int newx = ses->winx + amount;

  if ((newx > ses->winx) && (newx < scr.cols)) {
    ses->winx = newx;
    return 1;
  }

  return 0;
}

static int
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

static int
shiftWindowRight (unsigned int amount) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracting()) amount = getContractedLength(amount);
#endif /* ENABLE_CONTRACTED_BRAILLE */

  return moveWindowRight(amount);
}

static int
getWindowLength (void) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracting()) return getContractedLength(textCount);
#endif /* ENABLE_CONTRACTED_BRAILLE */

  return textCount;
}

static int
isTextOffset (int *arg, int end, int relaxed) {
  int value = *arg;

  if (value < textStart) return 0;
  if ((value -= textStart) >= textCount) return 0;

  if ((ses->winx + value) >= scr.cols) {
    if (!relaxed) return 0;
    value = scr.cols - 1 - ses->winx;
  }

#ifdef ENABLE_CONTRACTED_BRAILLE
  if (contracted) {
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

static int
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

static size_t
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

typedef struct {
  const unsigned char *const blinkingEnabled;
  const unsigned char *const visibleTime;
  const unsigned char *const invisibleTime;

  int isVisible;
  int timer;
} BlinkingState;

static BlinkingState cursorBlinkingState = {
  .blinkingEnabled = &prefs.blinkingCursor,
  .visibleTime = &prefs.cursorVisibleTime,
  .invisibleTime = &prefs.cursorInvisibleTime
};

static BlinkingState attributesBlinkingState = {
  .blinkingEnabled = &prefs.blinkingAttributes,
  .visibleTime = &prefs.attributesVisibleTime,
  .invisibleTime = &prefs.attributesInvisibleTime
};

static BlinkingState capitalsBlinkingState = {
  .blinkingEnabled = &prefs.blinkingCapitals,
  .visibleTime = &prefs.capitalsVisibleTime,
  .invisibleTime = &prefs.capitalsInvisibleTime
};

static BlinkingState speechCursorBlinkingState = {
  .blinkingEnabled = &prefs.blinkingSpeechCursor,
  .visibleTime = &prefs.speechCursorVisibleTime,
  .invisibleTime = &prefs.speechCursorInvisibleTime
};

static int
isBlinkedOn (const BlinkingState *state) {
  if (!*state->blinkingEnabled) return 1;
  return state->isVisible;
}

static void
setBlinkingState (BlinkingState *state, int visible) {
  state->timer = PREFERENCES_TIME((state->isVisible = visible)? *state->visibleTime: *state->invisibleTime);
}

static void
updateBlinkingState (BlinkingState *state) {
  if (*state->blinkingEnabled)
      if ((state->timer -= updateInterval) <= 0)
        setBlinkingState(state, !state->isVisible);
}

static void
resetBlinkingStates (void) {
  setBlinkingState(&cursorBlinkingState, 0);
  setBlinkingState(&attributesBlinkingState, 1);
  setBlinkingState(&capitalsBlinkingState, 1);
  setBlinkingState(&speechCursorBlinkingState, 0);
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
  if (contracted) {
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

static inline int
toggleResult (int isOn, const TuneDefinition *offTune, const TuneDefinition *onTune) {
  const TuneDefinition *tune = isOn? onTune: offTune;

  if (tune) playTune(tune);
  return isOn;
}

static int
toggleFlag (
  int *bits, int bit, int command,
  const TuneDefinition *offTune, const TuneDefinition *onTune
) {
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
    case BRL_FLG_TOGGLE_ON | BRL_FLG_TOGGLE_OFF:
      break;
  }

  return toggleResult(!!(*bits & bit), offTune, onTune);
}

static int
toggleSetting (
  unsigned char *setting, int command,
  const TuneDefinition *offTune, const TuneDefinition *onTune
) {
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
    case BRL_FLG_TOGGLE_ON | BRL_FLG_TOGGLE_OFF:
      break;
  }

  return toggleResult(!!*setting, offTune, onTune);
}

static inline int
toggleModeSetting (unsigned char *setting, int command) {
  return toggleSetting(setting, command, NULL, NULL);
}

static inline int
toggleFeatureSetting (unsigned char *setting, int command) {
  return toggleSetting(setting, command, &tune_toggle_off, &tune_toggle_on);
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

static int
insertKey (ScreenKey key, int flags) {
  if (flags & BRL_FLG_CHAR_SHIFT) key |= SCR_KEY_SHIFT;
  if (flags & BRL_FLG_CHAR_UPPER) key |= SCR_KEY_UPPER;
  if (flags & BRL_FLG_CHAR_CONTROL) key |= SCR_KEY_CONTROL;
  if (flags & BRL_FLG_CHAR_META) key |= SCR_KEY_ALT_LEFT;
  return insertScreenKey(key);
}

static RepeatState repeatState;

void
resetAutorepeat (void) {
  resetRepeatState(&repeatState);
}

void
handleAutorepeat (int *command, RepeatState *state) {
  if (!prefs.autorepeat) {
    state = NULL;
  } else if (!state) {
    state = &repeatState;
  }
  handleRepeatFlags(command, state, prefs.autorepeatPanning,
                    PREFERENCES_TIME(prefs.autorepeatDelay),
                    PREFERENCES_TIME(prefs.autorepeatInterval));
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

void
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

static int
switchVirtualTerminal (int vt) {
  int switched = switchScreenVirtualTerminal(vt);

  if (switched) {
    updateSessionAttributes();
  } else {
    playTune(&tune_command_rejected);
  }

  return switched;
}

static int
brlttyPrepare_unconstructed (void) {
  logMessage(LOG_ERR, "not constructed yet");
  return 0;
}

static int (*brlttyPrepare) (void) = brlttyPrepare_unconstructed;
static int oldwinx;
static int oldwiny;
static int restartRequired;
static int isOffline;
static int isSuspended;
static int inputModifiers;

static int
brlttyPrepare_next (void) {
  drainBrailleOutput(&brl, updateInterval);
  updateIntervals += 1;
  updateSessionAttributes();
  return 1;
}

static void
resetBrailleState (void) {
  resetScanCodes();
  resetBlinkingStates();
  if (prefs.autorepeat) resetAutorepeat();
  inputModifiers = 0;
}

static void
applyInputModifiers (int *modifiers) {
  *modifiers |= inputModifiers;
  inputModifiers = 0;
}

static int
brlttyPrepare_first (void) {
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

  brlttyPrepare = brlttyPrepare_next;
  return 1;
}

ProgramExitStatus
brlttyConstruct (int argc, char *argv[]) {
  srand((unsigned int)time(NULL));
  onProgramExit(exitLog);
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

  {
    ProgramExitStatus exitStatus = brlttyStart(argc, argv);
    if (exitStatus != PROG_EXIT_SUCCESS) return exitStatus;
  }

  onProgramExit(exitSessions);
  brlttyPrepare = brlttyPrepare_first;
  return PROG_EXIT_SUCCESS;
}

void
brlttyDestruct (void) {
  endProgram();
}

static int
brlttyCommand (void) {
  static const char modeString_preferences[] = "prf";
  static Preferences savedPreferences;

  int oldmotx = ses->winx;
  int oldmoty = ses->winy;

  int command;

  command = restartRequired? BRL_CMD_RESTARTBRL: readBrailleCommand(&brl, getScreenCommandContext());

  if (brl.highlightWindow) {
    brl.highlightWindow = 0;
    highlightWindow();
  }

  if (command != EOF) {
    int real = command;

    if (prefs.skipIdenticalLines) {
      switch (command & BRL_MSK_CMD) {
        case BRL_CMD_LNUP:
          real = BRL_CMD_PRDIFLN;
          break;

        case BRL_CMD_LNDN:
          real = BRL_CMD_NXDIFLN;
          break;

        case BRL_CMD_PRDIFLN:
          real = BRL_CMD_LNUP;
          break;

        case BRL_CMD_NXDIFLN:
          real = BRL_CMD_LNDN;
          break;
      }
    }

    if (real == command) {
      logCommand(command);
    } else {
      real |= (command & ~BRL_MSK_CMD);
      logTransformedCommand(command, real);
      command = real;
    }

    switch (command & BRL_MSK_CMD) {
      case BRL_CMD_OFFLINE:
        if (!isOffline) {
          logMessage(LOG_DEBUG, "braille display offline");
          isOffline = 1;
        }
        return 0;
    }
  }

  if (isOffline) {
    logMessage(LOG_DEBUG, "braille display online");
    isOffline = 0;
  }

  handleAutorepeat(&command, NULL);
  if (command == EOF) return 0;

doCommand:
  if (!executeScreenCommand(&command)) {
    switch (command & BRL_MSK_CMD) {
      case BRL_CMD_NOOP:        /* do nothing but loop */
        if (command & BRL_FLG_TOGGLE_ON) {
          playTune(&tune_toggle_on);
        } else if (command & BRL_FLG_TOGGLE_OFF) {
          playTune(&tune_toggle_off);
        }
        break;

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
        if (ses->winy > 0) {
          ses->winy -= MIN(verticalWindowShift, ses->winy);
        } else {
          playTune(&tune_bounce);
        }
        break;
      case BRL_CMD_WINDN:
        if (ses->winy < (int)(scr.rows - brl.textRows)) {
          ses->winy += MIN(verticalWindowShift, (scr.rows - brl.textRows - ses->winy));
        } else {
          playTune(&tune_bounce);
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
          if (!found) playTune(&tune_bounce);
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
            playTune(&tune_command_rejected);
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

          if (!found) playTune(&tune_bounce);
        } else {
          playTune(&tune_command_rejected);
        }

        break;
      }

      case BRL_CMD_LNBEG:
        if (ses->winx) {
          ses->winx = 0;
        } else {
          playTune(&tune_bounce);
        }
        break;

      case BRL_CMD_LNEND: {
        int end = MAX(scr.cols, textCount) - textCount;

        if (ses->winx < end) {
          ses->winx = end;
        } else {
          playTune(&tune_bounce);
        }
        break;
      }

      case BRL_CMD_CHRLT:
        if (!moveWindowLeft(1)) playTune(&tune_bounce);
        break;
      case BRL_CMD_CHRRT:
        if (!moveWindowRight(1)) playTune(&tune_bounce);
        break;

      case BRL_CMD_HWINLT:
        if (!shiftWindowLeft(halfWindowShift)) playTune(&tune_bounce);
        break;
      case BRL_CMD_HWINRT:
        if (!shiftWindowRight(halfWindowShift)) playTune(&tune_bounce);
        break;

      case BRL_CMD_FWINLT:
        if (!(prefs.skipBlankWindows && (prefs.skipBlankWindowsMode == sbwAll))) {
          int oldX = ses->winx;
          if (shiftWindowLeft(fullWindowShift)) {
            if (prefs.skipBlankWindows) {
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
                for (charIndex=0; charIndex<charCount; ++charIndex) {
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
            playTune(&tune_bounce);
            ses->winx = oldX;
            break;
          }
          playTune(&tune_wrap_up);
          upLine(isSameText);
          placeWindowRight();
        skipEndOfLine:
          if (prefs.skipBlankWindows && (prefs.skipBlankWindowsMode == sbwEndOfLine)) {
            int charIndex;
            ScreenCharacter characters[scr.cols];
            readScreen(0, ses->winy, scr.cols, 1, characters);
            for (charIndex=scr.cols-1; charIndex>0; --charIndex) {
              wchar_t text = characters[charIndex].text;
              if (text != WC_C(' ')) break;
            }
            if (showCursor() && (scr.posy == ses->winy) && SCR_COLUMN_OK(scr.posx)) {
              charIndex = MAX(charIndex, scr.posx);
            }
            if (charIndex < ses->winx) placeRightEdge(charIndex);
          }
          break;
        }
      case BRL_CMD_FWINLTSKIP: {
        int oldX = ses->winx;
        int oldY = ses->winy;
        int tuneLimit = 3;
        int charCount;
        int charIndex;
        ScreenCharacter characters[scr.cols];
        while (1) {
          if (!shiftWindowLeft(fullWindowShift)) {
            if (ses->winy == 0) {
              playTune(&tune_bounce);
              ses->winx = oldX;
              ses->winy = oldY;
              break;
            }
            if (tuneLimit-- > 0) playTune(&tune_wrap_up);
            upLine(isSameText);
            placeWindowRight();
          }
          charCount = getWindowLength();
          charCount = MIN(charCount, scr.cols-ses->winx);
          readScreen(ses->winx, ses->winy, charCount, 1, characters);
          for (charIndex=charCount-1; charIndex>=0; charIndex--) {
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

      case BRL_CMD_FWINRT:
        if (!(prefs.skipBlankWindows && (prefs.skipBlankWindowsMode == sbwAll))) {
          int oldX = ses->winx;
          if (shiftWindowRight(fullWindowShift)) {
            if (prefs.skipBlankWindows) {
              if (!showCursor() ||
                  (scr.posy != ses->winy) ||
                  (scr.posx < ses->winx)) {
                int charCount = scr.cols - ses->winx;
                int charIndex;
                ScreenCharacter characters[charCount];
                readScreen(ses->winx, ses->winy, charCount, 1, characters);
                for (charIndex=0; charIndex<charCount; ++charIndex) {
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
            playTune(&tune_bounce);
            ses->winx = oldX;
            break;
          }
          playTune(&tune_wrap_down);
          downLine(isSameText);
          ses->winx = 0;
          break;
        }
      case BRL_CMD_FWINRTSKIP: {
        int oldX = ses->winx;
        int oldY = ses->winy;
        int tuneLimit = 3;
        int charCount;
        int charIndex;
        ScreenCharacter characters[scr.cols];
        while (1) {
          if (!shiftWindowRight(fullWindowShift)) {
            if (ses->winy >= (scr.rows - brl.textRows)) {
              playTune(&tune_bounce);
              ses->winx = oldX;
              ses->winy = oldY;
              break;
            }
            if (tuneLimit-- > 0) playTune(&tune_wrap_down);
            downLine(isSameText);
            ses->winx = 0;
          }
          charCount = getWindowLength();
          charCount = MIN(charCount, scr.cols-ses->winx);
          readScreen(ses->winx, ses->winy, charCount, 1, characters);
          for (charIndex=0; charIndex<charCount; charIndex++) {
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

      case BRL_CMD_RETURN:
        if ((ses->winx != ses->motx) || (ses->winy != ses->moty)) {
      case BRL_CMD_BACK:
          ses->winx = ses->motx;
          ses->winy = ses->moty;
          break;
        }
      case BRL_CMD_HOME:
        if (!trackCursor(1)) playTune(&tune_command_rejected);
        break;

      case BRL_CMD_RESTARTBRL:
        restartBrailleDriver();
        resetBrailleState();
        restartRequired = 0;
        break;
      case BRL_CMD_PASTE:
        if (isLiveScreen() && !isRouting()) {
          if (cpbPaste()) break;
        }
        playTune(&tune_command_rejected);
        break;
      case BRL_CMD_CSRJMP_VERT:
        playTune(routeCursor(-1, ses->winy, scr.number)?
                 &tune_routing_started:
                 &tune_command_rejected);
        break;

      case BRL_CMD_CSRVIS:
        /* toggles the preferences option that decides whether cursor
           is shown at all */
        toggleFeatureSetting(&prefs.showCursor, command);
        break;
      case BRL_CMD_CSRHIDE:
        /* This is for briefly hiding the cursor */
        toggleModeSetting(&ses->hideCursor, command);
        /* no tune */
        break;
      case BRL_CMD_CSRSIZE:
        toggleFeatureSetting(&prefs.cursorStyle, command);
        break;
      case BRL_CMD_CSRTRK:
        if (toggleSetting(&ses->trackCursor, command, &tune_cursor_unlinked, &tune_cursor_linked)) {
#ifdef ENABLE_SPEECH_SUPPORT
          if (speechTracking && (scr.number == speechScreen)) {
            speechIndex = -1;
          } else
#endif /* ENABLE_SPEECH_SUPPORT */
            trackCursor(1);
        }
        break;
      case BRL_CMD_CSRBLINK:
        if (toggleFeatureSetting(&prefs.blinkingCursor, command)) {
          setBlinkingState(&cursorBlinkingState, 1);
        }
        break;

      case BRL_CMD_ATTRVIS:
        toggleFeatureSetting(&prefs.showAttributes, command);
        break;
      case BRL_CMD_ATTRBLINK:
        if (toggleFeatureSetting(&prefs.blinkingAttributes, command)) {
          setBlinkingState(&attributesBlinkingState, 1);
        }
        break;

      case BRL_CMD_CAPBLINK:
        if (toggleFeatureSetting(&prefs.blinkingCapitals, command)) {
          setBlinkingState(&capitalsBlinkingState, 1);
        }
        break;

      case BRL_CMD_SKPIDLNS:
        toggleFeatureSetting(&prefs.skipIdenticalLines, command);
        break;
      case BRL_CMD_SKPBLNKWINS:
        toggleFeatureSetting(&prefs.skipBlankWindows, command);
        break;
      case BRL_CMD_SLIDEWIN:
        toggleFeatureSetting(&prefs.slidingWindow, command);
        break;

      case BRL_CMD_DISPMD:
        toggleModeSetting(&ses->displayMode, command);
        break;
      case BRL_CMD_SIXDOTS:
        toggleFeatureSetting(&prefs.textStyle, command);
        break;

      case BRL_CMD_AUTOREPEAT:
        if (toggleFeatureSetting(&prefs.autorepeat, command)) resetAutorepeat();
        break;
      case BRL_CMD_TUNES:
        toggleFeatureSetting(&prefs.alertTunes, command);        /* toggle sound on/off */
        break;
      case BRL_CMD_FREEZE:
        if (isLiveScreen()) {
          playTune(activateFrozenScreen()? &tune_screen_frozen: &tune_command_rejected);
        } else if (isFrozenScreen()) {
          deactivateFrozenScreen();
          playTune(&tune_screen_unfrozen);
        } else {
          playTune(&tune_command_rejected);
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

      doModifier:
        toggleFlag(&inputModifiers, modifier, command, &tune_toggle_off, &tune_toggle_on);
        break;
      }

      case BRL_CMD_PREFMENU: {
        int ok = 0;

        if (isMenuScreen()) {
          if (prefs.saveOnExit)
            if (savePreferences())
              playTune(&tune_command_done);

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
          playTune(&tune_command_rejected);
        }

        break;
      }

      case BRL_CMD_PREFSAVE:
        if (isMenuScreen()) {
          if (savePreferences()) playTune(&tune_command_done);
          deactivateMenuScreen();
        } else if (savePreferences()) {
          playTune(&tune_command_done);
        } else {
          playTune(&tune_command_rejected);
        }
        break;
      case BRL_CMD_PREFLOAD:
        if (isMenuScreen()) {
          setPreferences(&savedPreferences);
          message(modeString_preferences, gettext("changes discarded"), 0);
        } else if (loadPreferences()) {
          resetBlinkingStates();
          playTune(&tune_command_done);
        } else {
          playTune(&tune_command_rejected);
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

      case BRL_CMD_INFO:
        if ((prefs.statusPosition == spNone) || haveStatusCells()) {
          toggleModeSetting(&infoMode, command);
        } else {
          toggleModeSetting(&textMaximized, command);
          reconfigureWindow();
        }
        break;

#ifdef ENABLE_LEARN_MODE
      case BRL_CMD_LEARN:
        if (!learnMode(&brl, updateInterval, 10000)) restartRequired = 1;
        break;
#endif /* ENABLE_LEARN_MODE */

      case BRL_CMD_SWITCHVT_PREV:
        switchVirtualTerminal(scr.number-1);
        break;
      case BRL_CMD_SWITCHVT_NEXT:
        switchVirtualTerminal(scr.number+1);
        break;

      case BRL_CMD_TIME: {
        TimeFormattingData fmt;
        getTimeFormattingData(&fmt);

#ifdef ENABLE_SPEECH_SUPPORT
        if (autospeak()) doSpeechTime(&fmt);
#endif /* ENABLE_SPEECH_SUPPORT */

        doBrailleTime(&fmt);
        break;
      }

#ifdef ENABLE_SPEECH_SUPPORT
      case BRL_CMD_RESTARTSPEECH:
        restartSpeechDriver();
        break;
      case BRL_CMD_SPKHOME:
        if (scr.number == speechScreen) {
          trackSpeech(speech->getTrack(&spk));
        } else {
          playTune(&tune_command_rejected);
        }
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

      case BRL_CMD_MUTE:
        speech->mute(&spk);
        break;

      case BRL_CMD_SAY_LINE:
        sayScreenLines(ses->winy, 1, 0, prefs.sayLineMode);
        break;
      case BRL_CMD_SAY_ABOVE:
        sayScreenLines(0, ses->winy+1, 1, sayImmediate);
        break;
      case BRL_CMD_SAY_BELOW:
        sayScreenLines(ses->winy, scr.rows-ses->winy, 1, sayImmediate);
        break;

      case BRL_CMD_SAY_SLOWER:
        if (speech->setRate && (prefs.speechRate > 0)) {
          setSpeechRate(&spk, --prefs.speechRate, 1);
        } else {
          playTune(&tune_command_rejected);
        }
        break;
      case BRL_CMD_SAY_FASTER:
        if (speech->setRate && (prefs.speechRate < SPK_RATE_MAXIMUM)) {
          setSpeechRate(&spk, ++prefs.speechRate, 1);
        } else {
          playTune(&tune_command_rejected);
        }
        break;

      case BRL_CMD_SAY_SOFTER:
        if (speech->setVolume && (prefs.speechVolume > 0)) {
          setSpeechVolume(&spk, --prefs.speechVolume, 1);
        } else {
          playTune(&tune_command_rejected);
        }
        break;
      case BRL_CMD_SAY_LOUDER:
        if (speech->setVolume && (prefs.speechVolume < SPK_VOLUME_MAXIMUM)) {
          setSpeechVolume(&spk, ++prefs.speechVolume, 1);
        } else {
          playTune(&tune_command_rejected);
        }
        break;

      case BRL_CMD_SPEAK_CURR_CHAR:
        speakCurrentCharacter();
        break;

      case BRL_CMD_SPEAK_PREV_CHAR:
        if (ses->spkx > 0) {
          ses->spkx -= 1;
          speakCurrentCharacter();
        } else if (ses->spky > 0) {
          ses->spky -= 1;
          ses->spkx = scr.cols - 1;
          playTune(&tune_wrap_up);
          speakCurrentCharacter();
        } else {
          playTune(&tune_bounce);
        }
        break;

      case BRL_CMD_SPEAK_NEXT_CHAR:
        if (ses->spkx < (scr.cols - 1)) {
          ses->spkx += 1;
          speakCurrentCharacter();
        } else if (ses->spky < (scr.rows - 1)) {
          ses->spky += 1;
          ses->spkx = 0;
          playTune(&tune_wrap_down);
          speakCurrentCharacter();
        } else {
          playTune(&tune_bounce);
        }
        break;

      case BRL_CMD_SPEAK_FRST_CHAR: {
        ScreenCharacter characters[scr.cols];
        int column;

        readScreen(0, ses->spky, scr.cols, 1, characters);
        if ((column = findFirstNonSpaceCharacter(characters, scr.cols)) >= 0) {
          ses->spkx = column;
          speakDone(characters, column, 1, 0);
        } else {
          playTune(&tune_command_rejected);
        }

        break;
      }

      case BRL_CMD_SPEAK_LAST_CHAR: {
        ScreenCharacter characters[scr.cols];
        int column;

        readScreen(0, ses->spky, scr.cols, 1, characters);
        if ((column = findLastNonSpaceCharacter(characters, scr.cols)) >= 0) {
          ses->spkx = column;
          speakDone(characters, column, 1, 0);
        } else {
          playTune(&tune_command_rejected);
        }

        break;
      }

      {
        int direction;
        int spell;

      case BRL_CMD_SPEAK_PREV_WORD:
        direction = -1;
        spell = 0;
        goto speakWord;

      case BRL_CMD_SPEAK_NEXT_WORD:
        direction = 1;
        spell = 0;
        goto speakWord;

      case BRL_CMD_SPEAK_CURR_WORD:
        direction = 0;
        spell = 0;
        goto speakWord;

      case BRL_CMD_SPELL_CURR_WORD:
        direction = 0;
        spell = 1;
        goto speakWord;

      speakWord:
        {
          int row = ses->spky;
          int column = ses->spkx;

          ScreenCharacter characters[scr.cols];
          ScreenCharacterType type;
          int onCurrentWord;

          int from = column;
          int to = from + 1;

        findWord:
          readScreen(0, row, scr.cols, 1, characters);
          type = (row == ses->spky)? getScreenCharacterType(&characters[column]): SCT_SPACE;
          onCurrentWord = type != SCT_SPACE;

          if (direction < 0) {
            while (1) {
              if (column == 0) {
                if ((type != SCT_SPACE) && !onCurrentWord) {
                  ses->spkx = from = column;
                  ses->spky = row;
                  break;
                }

                if (row == 0) goto noWord;
                if (row-- == ses->spky) playTune(&tune_wrap_up);
                column = scr.cols;
                goto findWord;
              }

              {
                ScreenCharacterType newType = getScreenCharacterType(&characters[--column]);

                if (newType != type) {
                  if (onCurrentWord) {
                    onCurrentWord = 0;
                  } else if (type != SCT_SPACE) {
                    ses->spkx = from = column + 1;
                    ses->spky = row;
                    break;
                  }

                  if (newType != SCT_SPACE) to = column + 1;
                  type = newType;
                }
              }
            }
          } else if (direction > 0) {
            while (1) {
              if (++column == scr.cols) {
                if ((type != SCT_SPACE) && !onCurrentWord) {
                  to = column;
                  ses->spkx = from;
                  ses->spky = row;
                  break;
                }

                if (row == (scr.rows - 1)) goto noWord;
                if (row++ == ses->spky) playTune(&tune_wrap_down);
                column = -1;
                goto findWord;
              }

              {
                ScreenCharacterType newType = getScreenCharacterType(&characters[column]);

                if (newType != type) {
                  if (onCurrentWord) {
                    onCurrentWord = 0;
                  } else if (type != SCT_SPACE) {
                    to = column;
                    ses->spkx = from;
                    ses->spky = row;
                    break;
                  }

                  if (newType != SCT_SPACE) from = column;
                  type = newType;
                }
              }
            }
          } else if (type != SCT_SPACE) {
            while (from > 0) {
              if (getScreenCharacterType(&characters[--from]) != type) {
                from += 1;
                break;
              }
            }

            while (to < scr.cols) {
              if (getScreenCharacterType(&characters[to]) != type) break;
              to += 1;
            }
          }

          speakDone(characters, from, to-from, spell);
          break;
        }

      noWord:
        playTune(&tune_bounce);
        break;
      }

      case BRL_CMD_SPEAK_CURR_LINE:
        speakCurrentLine();
        break;

      {
        int increment;
        int limit;

      case BRL_CMD_SPEAK_PREV_LINE:
        increment = -1;
        limit = 0;
        goto speakLine;

      case BRL_CMD_SPEAK_NEXT_LINE:
        increment = 1;
        limit = scr.rows - 1;
        goto speakLine;

      speakLine:
        if (ses->spky == limit) {
          playTune(&tune_bounce);
        } else {
          if (prefs.skipIdenticalLines) {
            ScreenCharacter original[scr.cols];
            ScreenCharacter current[scr.cols];
            int count = 0;

            readScreen(0, ses->spky, scr.cols, 1, original);

            do {
              readScreen(0, ses->spky+=increment, scr.cols, 1, current);
              if (!isSameRow(original, current, scr.cols, isSameText)) break;

              if (!count) {
                playTune(&tune_skip_first);
              } else if (count < 4) {
                playTune(&tune_skip);
              } else if (!(count % 4)) {
                playTune(&tune_skip_more);
              }

              count += 1;
            } while (ses->spky != limit);
          } else {
            ses->spky += increment;
          }

          speakCurrentLine();
        }

        break;
      }

      case BRL_CMD_SPEAK_FRST_LINE: {
        ScreenCharacter characters[scr.cols];
        int row = 0;

        while (row < scr.rows) {
          readScreen(0, row, scr.cols, 1, characters);
          if (!isAllSpaceCharacters(characters, scr.cols)) break;
          row += 1;
        }

        if (row < scr.rows) {
          ses->spky = row;
          ses->spkx = 0;
          speakCurrentLine();
        } else {
          playTune(&tune_command_rejected);
        }

        break;
      }

      case BRL_CMD_SPEAK_LAST_LINE: {
        ScreenCharacter characters[scr.cols];
        int row = scr.rows - 1;

        while (row >= 0) {
          readScreen(0, row, scr.cols, 1, characters);
          if (!isAllSpaceCharacters(characters, scr.cols)) break;
          row -= 1;
        }

        if (row >= 0) {
          ses->spky = row;
          ses->spkx = 0;
          speakCurrentLine();
        } else {
          playTune(&tune_command_rejected);
        }

        break;
      }

      case BRL_CMD_DESC_CURR_CHAR: {
        char description[0X50];
        formatCharacterDescription(description, sizeof(description), ses->spkx, ses->spky);
        sayString(&spk, description, 1);
        break;
      }

      case BRL_CMD_ROUTE_CURR_LOCN:
        if (routeCursor(ses->spkx, ses->spky, scr.number)) {
          playTune(&tune_routing_started);
        } else {
          playTune(&tune_command_rejected);
        }
        break;

      case BRL_CMD_SPEAK_CURR_LOCN: {
        char buffer[0X50];
        snprintf(buffer, sizeof(buffer), "%s %d, %s %d",
                 gettext("line"), ses->spky+1,
                 gettext("column"), ses->spkx+1);
        sayString(&spk, buffer, 1);
        break;
      }

      case BRL_CMD_SHOW_CURR_LOCN:
        toggleFeatureSetting(&prefs.showSpeechCursor, command);
        break;
#endif /* ENABLE_SPEECH_SUPPORT */

      default: {
        int blk = command & BRL_MSK_BLK;
        int arg = command & BRL_MSK_ARG;
        int ext = BRL_CODE_GET(EXT, command);
        int flags = command & BRL_MSK_FLG;

        switch (blk) {
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
              playTune(&tune_command_rejected);
            }
            break;
          }

          case BRL_BLK_PASSCHAR: {
            applyInputModifiers(&flags);
            if (!insertKey(BRL_ARG_GET(command), flags)) playTune(&tune_command_rejected);
            break;
          }

          case BRL_BLK_PASSDOTS:
            applyInputModifiers(&flags);
            if (!insertKey(convertDotsToCharacter(textTable, arg), flags)) playTune(&tune_command_rejected);
            break;

          case BRL_BLK_PASSAT:
            if (flags & BRL_FLG_KBD_RELEASE) atInterpretScanCode(&command, 0XF0);
            if (flags & BRL_FLG_KBD_EMUL0) atInterpretScanCode(&command, 0XE0);
            if (flags & BRL_FLG_KBD_EMUL1) atInterpretScanCode(&command, 0XE1);
            if (atInterpretScanCode(&command, arg)) goto doCommand;
            break;

          case BRL_BLK_PASSXT:
            if (flags & BRL_FLG_KBD_EMUL0) xtInterpretScanCode(&command, 0XE0);
            if (flags & BRL_FLG_KBD_EMUL1) xtInterpretScanCode(&command, 0XE1);
            if (xtInterpretScanCode(&command, arg)) goto doCommand;
            break;

          case BRL_BLK_PASSPS2:
            /* not implemented yet */
            playTune(&tune_command_rejected);
            break;

          case BRL_BLK_ROUTE: {
            int column, row;

            if (getCharacterCoordinates(arg, &column, &row, 0, 1)) {
              if (routeCursor(column, row, scr.number)) {
                playTune(&tune_routing_started);
                break;
              }
            }
            playTune(&tune_command_rejected);
            break;
          }

          {
            int clear;
            int column, row;

          case BRL_BLK_CLIP_NEW:
            clear = 1;
            goto doClipBegin;

          case BRL_BLK_CLIP_ADD:
            clear = 0;
            goto doClipBegin;

          doClipBegin:
            if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
              if (clear) cpbClearContent();
              cpbBeginOperation(column, row);
            } else {
              playTune(&tune_command_rejected);
            }

            break;
          }

          case BRL_BLK_COPY_RECT: {
            int column, row;

            if (getCharacterCoordinates(arg, &column, &row, 1, 1))
              if (cpbRectangularCopy(column, row))
                break;

            playTune(&tune_command_rejected);
            break;
          }

          case BRL_BLK_COPY_LINE: {
            int column, row;

            if (getCharacterCoordinates(arg, &column, &row, 1, 1))
              if (cpbLinearCopy(column, row))
                break;

            playTune(&tune_command_rejected);
            break;
          }

          {
            int clear;

          case BRL_BLK_CLIP_COPY:
            clear = 1;
            goto doCopy;

          case BRL_BLK_CLIP_APPEND:
            clear = 0;
            goto doCopy;

          doCopy:
            if (ext > arg) {
              int column1, row1;

              if (getCharacterCoordinates(arg, &column1, &row1, 0, 0)) {
                int column2, row2;

                if (getCharacterCoordinates(ext, &column2, &row2, 1, 1)) {
                  if (clear) cpbClearContent();
                  cpbBeginOperation(column1, row1);
                  if (cpbLinearCopy(column2, row2)) break;
                }
              }
            }

            playTune(&tune_command_rejected);
            break;
          }

          case BRL_BLK_PWGEN: {
            static const char codeset[] =
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "abcdefghijklmnopqrstuvwxyz"
              "0123456789";
            const size_t codesetLength = strlen(codeset);

            wchar_t password[arg + 1];
            wchar_t *character = password;
            const wchar_t *end = character + ARRAY_COUNT(password);

            while (character < end) {
              *character++ = codeset[rand() % codesetLength];
            }

            cpbSetContent(password, character-password);
            playTune(&tune_clipboard_end);
            break;
          }

          case BRL_BLK_DESCCHAR: {
            int column, row;

            if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
              char description[0X50];
              formatCharacterDescription(description, sizeof(description), column, row);
              message(NULL, description, 0);
            } else {
              playTune(&tune_command_rejected);
            }
            break;
          }

          case BRL_BLK_SETLEFT: {
            int column, row;

            if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
              ses->winx = column;
              ses->winy = row;
            } else {
              playTune(&tune_command_rejected);
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
              playTune(&tune_command_rejected);
            }
            break;

          case BRL_BLK_SETMARK: {
            ScreenLocation *mark = &ses->marks[arg];
            mark->column = ses->winx;
            mark->row = ses->winy;
            playTune(&tune_mark_set);
            break;
          }
          case BRL_BLK_GOTOMARK: {
            ScreenLocation *mark = &ses->marks[arg];
            ses->winx = mark->column;
            ses->winy = mark->row;
            break;
          }

          case BRL_BLK_SWITCHVT:
            switchVirtualTerminal(arg+1);
            break;

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
              playTune(&tune_command_rejected);
            }
            break;
          }

          case BRL_BLK_PRDIFCHAR: {
            int column, row;

            if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
              ses->winy = row;
              upDifferentCharacter(isSameText, column);
            } else {
              playTune(&tune_command_rejected);
            }
            break;
          }

          case BRL_BLK_NXDIFCHAR: {
            int column, row;

            if (getCharacterCoordinates(arg, &column, &row, 0, 0)) {
              ses->winy = row;
              downDifferentCharacter(isSameText, column);
            } else {
              playTune(&tune_command_rejected);
            }
            break;
          }

          default:
            playTune(&tune_command_rejected);
            logMessage(LOG_WARNING, "%s: %04X", gettext("unrecognized command"), command);
        }
        break;
      }
    }
  }

  if ((ses->winx != oldmotx) || (ses->winy != oldmoty)) {
    /* The window has been manually moved. */
    ses->motx = ses->winx;
    ses->moty = ses->winy;

#ifdef ENABLE_CONTRACTED_BRAILLE
    contracted = 0;
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

  return 1;
}

int
brlttyUpdate (void) {
  if (!brlttyPrepare()) return 0;
  if (terminationCount) return 0;

  closeTuneDevice(0);
  checkRoutingStatus(ROUTING_DONE, 0);

  if (opt_releaseDevice) {
    if (scr.unreadable) {
      if (!isSuspended) {
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
      }
    } else {
      if (isSuspended) {
        isSuspended = !(
#ifdef ENABLE_API
          apiStarted? api_resume(&brl):
#endif /* ENABLE_API */
          constructBrailleDriver());
      }
    }
  }

  if (!isSuspended) {
    int pointerMoved = 0;

#ifdef ENABLE_API
    int claimed = apiStarted && api_claimDriver(&brl);
#endif /* ENABLE_API */

    /*
     * Process any Braille input 
     */
    do {
      if (terminationCount) return 0;
    } while (brlttyCommand());

    /* some commands (key insertion, virtual terminal switching, etc)
     * may have moved the cursor
     */
    updateSessionAttributes();

    /*
     * Update blink counters: 
     */
    updateBlinkingState(&cursorBlinkingState);
    updateBlinkingState(&attributesBlinkingState);
    updateBlinkingState(&capitalsBlinkingState);
    updateBlinkingState(&speechCursorBlinkingState);

#ifdef ENABLE_SPEECH_SUPPORT
    /* called continually even if we're not tracking so that the pipe doesn't fill up. */
    speech->doTrack(&spk);

    if (speechTracking && !speech->isSpeaking(&spk)) speechTracking = 0;
#endif /* ENABLE_SPEECH_SUPPORT */

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

#ifdef ENABLE_SPEECH_SUPPORT
    if (autospeak()) {
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

    if (!isOffline) {
      if (infoMode) {
        if (!showInfo()) restartRequired = 1;
      } else {
        const unsigned int windowLength = brl.textColumns * brl.textRows;
        const unsigned int textLength = textCount * brl.textRows;
        wchar_t textBuffer[windowLength];

        memset(brl.buffer, 0, windowLength);
        wmemset(textBuffer, WC_C(' '), windowLength);

#ifdef ENABLE_CONTRACTED_BRAILLE
        contracted = 0;
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
            contracted = 1;

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

        if (!contracted)
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
    if (claimed) api_releaseDriver(&brl);
  } else if (apiStarted) {
    switch (readBrailleCommand(&brl, KTB_CTX_DEFAULT)) {
      case BRL_CMD_RESTARTBRL:
        restartBrailleDriver();
        break;

      default:
        break;
    }
#endif /* ENABLE_API */
  }

#ifdef ENABLE_API
  if (apiStarted) {
    if (!api_flush(&brl)) restartRequired = 1;
  }
#endif /* ENABLE_API */

#ifdef ENABLE_SPEECH_SUPPORT
  processSpeechInput(&spk);
#endif /* ENABLE_SPEECH_SUPPORT */

  return 1;
}

static int
readCommand (KeyTableCommandContext context) {
  int command = readBrailleCommand(&brl, context);
  if (command != EOF) {
    logCommand(command);
    if (BRL_DELAYED_COMMAND(command)) command = BRL_CMD_NOOP;
    command &= BRL_MSK_CMD;
  }
  return command;
}

int 
message (const char *mode, const char *text, short flags) {
  int ok = 1;

  if (!mode) mode = "";

#ifdef ENABLE_SPEECH_SUPPORT
  if (!(flags & MSG_SILENT))
    if (autospeak())
      sayString(&spk, text, 1);
#endif /* ENABLE_SPEECH_SUPPORT */

  if (braille && brl.buffer && !brl.noDisplay) {
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
    while (length) {
      const int delay = MIN(updateInterval, 100);
      int count;

      /* strip leading spaces */
      while (iswspace(*character)) character++, length--;

      if (length <= size) {
        count = length; /* the whole message fits in the braille window */
      } else {
        /* split the message across multiple windows on space characters */
        for (count=size-2; count>0 && iswspace(characters[count]); count--);
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

      if (flags & MSG_WAITKEY) {
        while (!terminationCount) {
          int command = readCommand(KTB_CTX_WAITING);

          if (command == EOF) {
            drainBrailleOutput(&brl, delay);
            closeTuneDevice(0);
          } else if (command != BRL_CMD_NOOP) {
            break;
          }
        }
      } else if (length || !(flags & MSG_NODELAY)) {
        TimePeriod period;
        startTimePeriod(&period, messageDelay);

        while (!terminationCount) {
          drainBrailleOutput(&brl, delay);
          if (afterTimePeriod(&period, NULL)) break;

          {
            int command = readCommand(KTB_CTX_WAITING);

            if (command != EOF) break;
          }
        }
      }
    }

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
