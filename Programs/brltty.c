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

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

#include "embed.h"
#include "log.h"
#include "parameters.h"
#include "cmd_queue.h"
#include "cmd_navigation.h"
#include "cmd_speech.h"
#include "timing.h"
#include "async_wait.h"
#include "async_event.h"
#include "async_signal.h"
#include "tunes.h"
#include "ctb.h"
#include "routing.h"
#include "charset.h"
#include "scr.h"
#include "update.h"
#include "ses.h"
#include "scancodes.h"
#include "brl.h"
#include "prefs.h"
#include "api_control.h"
#include "brltty.h"

#ifdef ENABLE_SPEECH_SUPPORT
#include "spk.h"
#endif /* ENABLE_SPEECH_SUPPORT */

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
int contractedStart;
int contractedOffsets[0X100];
int contractedTrack = 0;
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

typedef struct {
  int motionColumn;
  int motionRow;
} PrecommandState;

static void *
preprocessCommand (void) {
  PrecommandState *pre;

  if ((pre = malloc(sizeof(*pre)))) {
    pre->motionColumn = ses->winx;
    pre->motionRow = ses->winy;

    suspendUpdates();
    return pre;
  } else {
    logMallocError();
  }

  return NULL;
}

static void
postprocessCommand (void *state, int command, int handled) {
  PrecommandState *pre = state;

  if (pre) {
    resumeUpdates(0);
    if (handled) scheduleUpdate("command executed");

    if ((ses->winx != pre->motionColumn) || (ses->winy != pre->motionRow)) {
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

    free(pre);
  }
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
        pushCommandEnvironment("main", preprocessCommand, postprocessCommand);
        pushCommandHandler("unhandled", KTB_CTX_DEFAULT, handleUnhandledCommand, NULL);

#ifdef ENABLE_SPEECH_SUPPORT
        pushCommandHandler("speech", KTB_CTX_DEFAULT, handleSpeechCommand, NULL);
#endif /*  ENABLE_SPEECH_SUPPORT */

        pushCommandHandler("navigation", KTB_CTX_DEFAULT, handleNavigationCommand, NULL);
        pushCommandHandler("screen", KTB_CTX_DEFAULT, handleScreenCommand, NULL);
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

void
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
    STR_PRINTF(hourFormat, fmt->components.hour);
    STR_PRINTF("%c", separator);
    STR_PRINTF(minuteFormat, fmt->components.minute);

    if (prefs.showSeconds) {
      STR_PRINTF("%c", separator);
      STR_PRINTF(secondFormat, fmt->components.second);
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

      uint16_t year = fmt->components.year;
      uint8_t month = fmt->components.month + 1;
      uint8_t day = fmt->components.day + 1;

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
          STR_PRINTF(yearFormat, year);
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
          STR_PRINTF(yearFormat, year);
          break;

        case dfDayMonthYear:
          STR_PRINTF(dayFormat, day);
          STR_PRINTF("%c", separator);
          STR_PRINTF(monthFormat, month);
          STR_PRINTF("%c", separator);
          STR_PRINTF(yearFormat, year);
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
  getCurrentTime(&fmt->value);
  expandTimeValue(&fmt->value, &fmt->components);
  fmt->meridian = getMeridianString(&fmt->components.hour);
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

void
trackSpeech (void) {
  placeWindowHorizontally(speechIndex % scr.cols);
  slideWindowVertically((speechIndex / scr.cols) + speechLine);
  scheduleUpdate("speech tracked");
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
    sayUtf8Characters(text, attributes, length, count, immediate);
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
        if (immediate) muteSpeech("white space");
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
          if (canSetSpeechPitch()) {
            unsigned char pitch = prefs.speechPitch + 7;
            if (pitch > SPK_PITCH_MAXIMUM) pitch = SPK_PITCH_MAXIMUM;

            if (pitch != prefs.speechPitch) {
              setSpeechPitch(pitch, 0);
              restorePitch = 1;
            }
          }
          break;
      }
    }

    if (canSetSpeechPunctuation()) {
      unsigned char punctuation = SPK_PUNCTUATION_ALL;

      if (punctuation != prefs.speechPunctuation) {
        setSpeechPunctuation(punctuation, 0);
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

    if (restorePunctuation) setSpeechPunctuation(prefs.speechPunctuation, 0);
    if (restorePitch) setSpeechPitch(prefs.speechPitch, 0);
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

int
getUncontractedCursorOffset (int x, int y) {
  return ((y == ses->winy) && (x >= ses->winx) && (x < scr.cols))? (x - ses->winx): -1;
}

int
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

ToggleResult
toggleBit (
  int *bits, int bit, int command,
  const TuneDefinition *offTune,
  const TuneDefinition *onTune
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
      return TOGGLE_ERROR;
  }

  {
    int isOn = (*bits & bit) != 0;
    const TuneDefinition *tune = isOn? onTune: offTune;

    playTune(tune);
    if (*bits != oldBits) return isOn? TOGGLE_ON: TOGGLE_OFF;

    asyncWait(TUNE_TOGGLE_REPEAT_DELAY);
    playTune(tune);
    return TOGGLE_SAME;
  }
}

ToggleResult
toggleSetting (
  unsigned char *setting, int command,
  const TuneDefinition *offTune,
  const TuneDefinition *onTune
) {
  const int bit = 1;
  int bits = *setting? bit: 0;
  ToggleResult result = toggleBit(&bits, bit, command, offTune, onTune);

  *setting = (bits & bit)? 1: 0;
  return result;
}

ToggleResult
toggleModeSetting (unsigned char *setting, int command) {
  return toggleSetting(setting, command, NULL, NULL);
}

ToggleResult
toggleFeatureSetting (unsigned char *setting, int command) {
  return toggleSetting(setting, command, &tune_toggle_off, &tune_toggle_on);
}

int
showCursor (void) {
  return scr.cursor && prefs.showCursor && !ses->hideCursor;
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
  inputModifiers = 0;
}

int
canBraille (void) {
  return braille && brl.buffer && !brl.noDisplay && !isSuspended;
}

static unsigned int interruptEnabledCount;
static AsyncEvent *interruptEvent;
static int interruptPending;
static WaitResult waitResult;

typedef struct {
  WaitResult waitResult;
} InterruptEventParameters;

int
brlttyInterrupt (WaitResult waitResult) {
  if (interruptEvent) {
    InterruptEventParameters *iep;

    if ((iep = malloc(sizeof(*iep)))) {
      memset(iep, 0, sizeof(*iep));
      iep->waitResult = waitResult;

      if (asyncSignalEvent(interruptEvent, iep)) {
        return 1;
      }

      free(iep);
    } else {
      logMallocError();
    }
  }

  return 0;
}

ASYNC_EVENT_CALLBACK(handleCoreInterrupt) {
  InterruptEventParameters *iep = parameters->signalData;

  if (iep) {
    interruptPending = 1;
    waitResult = iep->waitResult;
    free(iep);
  }
}

int
brlttyEnableInterrupt (void) {
  if (!interruptEnabledCount) {
    if (!(interruptEvent = asyncNewEvent(handleCoreInterrupt, NULL))) {
      return 0;
    }
  }

  interruptEnabledCount += 1;
  return 1;
}

int
brlttyDisableInterrupt (void) {
  if (!interruptEnabledCount) return 0;

  if (!--interruptEnabledCount) {
    asyncDiscardEvent(interruptEvent);
    interruptEvent = NULL;
  }

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

static void
handleRestartRequired (const void *data) {
  restartBrailleDriver();
}

static volatile unsigned int programTerminationRequestCount;
static volatile time_t programTerminationRequestTime;

typedef struct {
  UnmonitoredConditionHandler *handler;
  const void *data;
} UnmonitoredConditionDescriptor;

ASYNC_CONDITION_TESTER(checkUnmonitoredConditions) {
  UnmonitoredConditionDescriptor *ucd = data;

  if (interruptPending) {
    ucd->data = &waitResult;
    interruptPending = 0;
    return 1;
  }

  if (programTerminationRequestCount) {
    static const WaitResult result = WAIT_STOP;
    ucd->data = &result;
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

  if (restartRequired) {
    ucd->handler = handleRestartRequired;
    return 1;
  }

  return 0;
}

WaitResult
brlttyWait (int duration) {
  UnmonitoredConditionDescriptor ucd = {
    .handler = NULL,
    .data = NULL
  };

  if (asyncAwaitCondition(duration, checkUnmonitoredConditions, &ucd)) {
    if (!ucd.handler) {
      const WaitResult *result = ucd.data;
      return *result;
    }

    ucd.handler(ucd.data);
  }

  return 1;
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

static void
exitLog (void *data) {
  closeSystemLog();
  closeLogFile();
}

static void
exitSessions (void *data) {
  if (ses) {
    popCommandEnvironment();
    ses = NULL;
  }

  deallocateSessionEntries();
}

#ifdef ASYNC_CAN_HANDLE_SIGNALS
ASYNC_SIGNAL_HANDLER(handleProgramTerminationRequest) {
  time_t now = time(NULL);

  if (difftime(now, programTerminationRequestTime) > PROGRAM_TERMINATION_REQUEST_RESET_SECONDS) {
    programTerminationRequestCount = 0;
  }

  if ((programTerminationRequestCount += 1) > PROGRAM_TERMINATION_REQUEST_COUNT_THRESHOLD) {
    exit(1);
  }

  programTerminationRequestTime = now;
}
#endif /* ASYNC_CAN_HANDLE_SIGNALS */

ProgramExitStatus
brlttyConstruct (int argc, char *argv[]) {
  {
    TimeValue now;

    getMonotonicTime(&now);
    srand(now.seconds ^ now.nanoseconds);
  }

  onProgramExit("log", exitLog, NULL);
  openSystemLog();

  programTerminationRequestCount = 0;
  programTerminationRequestTime = time(NULL);

#ifdef ASYNC_CAN_OBTAIN_SIGNALS
  asyncBlockObtainableSignals();
#endif /* ASYNC_CAN_OBTAIN_SIGNALS */

#ifdef ASYNC_CAN_HANDLE_SIGNALS
#ifdef SIGPIPE
  /* We ignore SIGPIPE before calling brlttyStart() so that a driver which uses
   * a broken pipe won't abort program execution.
   */
  asyncIgnoreSignal(SIGPIPE, NULL);
#endif /* SIGPIPE */

#ifdef SIGTERM
  asyncHandleSignal(SIGTERM, handleProgramTerminationRequest, NULL);
#endif /* SIGTERM */

#ifdef SIGINT
  asyncHandleSignal(SIGINT, handleProgramTerminationRequest, NULL);
#endif /* SIGINT */
#endif /* ASYNC_CAN_HANDLE_SIGNALS */

  interruptEnabledCount = 0;
  interruptEvent = NULL;
  interruptPending = 0;

  beginCommandQueue();
  beginUpdates();
  suspendUpdates();

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
  resumeUpdates(1);

  restartRequired = 0;
  isOffline = 0;
  isSuspended = 0;
  resetBrailleState();

  return PROG_EXIT_SUCCESS;
}

int
brlttyDestruct (void) {
  suspendUpdates();
  endProgram();
  endCommandQueue();
  return 1;
}
