/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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
      int row;
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
    int length;
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
    snprintf(text, sizeof(text), "%.*s %02d %c%c%c%c%c%c%n",
             cellCount, prefix,
             scr.number,
             ses->trackCursor? 't': ' ',
             prefs.showCursor? (prefs.blinkingCursor? 'B': 'v'):
                               (prefs.blinkingCursor? 'b': ' '),
             ses->displayMode? 'a': 't',
             isFrozenScreen()? 'f': ' ',
             prefs.textStyle? '6': '8',
             prefs.blinkingCapitals? 'B': ' ',
             &length);
    if (length > size) length = size;

    {
      int i;
      for (i=0; i<length; ++i) {
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

  {
    snprintf(text, sizeof(text), "%02d:%02d %02d:%02d %02d %c%c%c%c%c%c",
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
    return writeBrailleText(mode, text);
  }
}

static void 
slideWindowVertically (int y) {
  if (y < ses->winy)
    ses->winy = y;
  else if  (y >= (ses->winy + brl.textRows))
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
    if ((scr.posx < ses->winx) || (scr.posx >= (ses->winx + textCount)) ||
        (scr.posy < ses->winy) || (scr.posy >= (ses->winy + brl.textRows))) {
      placeWindowHorizontally(scr.posx);
    }
  }

  if (prefs.slidingWindow) {
    int reset = textCount * 3 / 10;
    int trigger = prefs.eagerSlidingWindow? textCount*3/20: 0;

    if (scr.posx < (ses->winx + trigger)) {
      ses->winx = MAX(scr.posx-reset, 0);
    } else if (scr.posx >= (ses->winx + textCount - trigger)) {
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
  unsigned char text[(count * UTF8_LEN_MAX) + 1];
  unsigned char *t = text;

  {
    unsigned int i;

    for (i=0; i<count; i+=1) {
      Utf8Buffer utf8;
      size_t length = convertWcharToUtf8(characters[i], utf8);

      if (length) {
        t = mempcpy(t, utf8, length);
      } else {
        *t++ = ' ';
      }
    }

    *t = 0;
  }

  if (immediate) speech->mute(&spk);
  speech->say(&spk, text, t-text, count, attributes);
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
findFirstNonblankCharacter (const ScreenCharacter *characters, int count) {
  int index = 0;

  while (index < count) {
    if (getScreenCharacterType(&characters[index]) != SCT_SPACE) return index;
    index += 1;
  }

  return -1;
}

static int
findLastNonblankCharacter (const ScreenCharacter *characters, int count) {
  int index = count;

  while (index > 0)
    if (getScreenCharacterType(&characters[--index]) != SCT_SPACE)
      return index;

  return -1;
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

  if (findFirstNonblankCharacter(line, count) < 0) {
    sayString(&spk, gettext("space"), 1);
  } else if (spell) {
    wchar_t string[count * 2];
    size_t length = 0;
    unsigned int index = 0;

    while (index < count) {
      string[length++] = line[index++].text;
      string[length++] = WC_C(' ');
    }

    string[length] = WC_C('\0');
    sayWideCharacters(string, NULL, length, 1);
  } else {
    sayScreenCharacters(line, count, 1);
  }

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
  return ses->winy < (scr.rows - brl.textRows);
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
  if (ses->winy < (scr.rows - brl.textRows)) {
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
testIndent (int column, int row, void *data) {
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
getCursorOffset (void) {
  return ((scr.posy == ses->winy) && (scr.posx >= ses->winx) && (scr.posx < scr.cols))? (scr.posx - ses->winx): -1;
}

static int
getContractedCursor (int offset) {
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
               NULL, getContractedCursor(getCursorOffset()));
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

static void
setBlinkingState (int *state, int *timer, int visible, unsigned char invisibleTime, unsigned char visibleTime) {
  *timer = PREFERENCES_TIME((*state = visible)? visibleTime: invisibleTime);
}

static int cursorState;                /* display cursor on (toggled during blink) */
static int cursorTimer;
static void
setBlinkingCursor (int visible) {
  setBlinkingState(&cursorState, &cursorTimer, visible,
                   prefs.cursorInvisibleTime, prefs.cursorVisibleTime);
}

static int attributesState;
static int attributesTimer;
static void
setBlinkingAttributes (int visible) {
  setBlinkingState(&attributesState, &attributesTimer, visible,
                   prefs.attributesInvisibleTime, prefs.attributesVisibleTime);
}

static int capitalsState;                /* display caps off (toggled during blink) */
static int capitalsTimer;
static void
setBlinkingCapitals (int visible) {
  setBlinkingState(&capitalsState, &capitalsTimer, visible,
                   prefs.capitalsInvisibleTime, prefs.capitalsVisibleTime);
}

static void
resetBlinkingStates (void) {
  setBlinkingCursor(0);
  setBlinkingAttributes(1);
  setBlinkingCapitals(1);
}

unsigned char
getCursorDots (void) {
  if (prefs.blinkingCursor && !cursorState) return 0;
  return prefs.cursorStyle? (BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8): (BRL_DOT7 | BRL_DOT8);
}

static int
toggleFlag (unsigned char *flag, int command, const TuneDefinition *off, const TuneDefinition *on) {
  const TuneDefinition *tune;
  if ((command & BRL_FLG_TOGGLE_MASK) != BRL_FLG_TOGGLE_MASK)
    *flag = (command & BRL_FLG_TOGGLE_ON)? 1: ((command & BRL_FLG_TOGGLE_OFF)? 0: !*flag);
  if ((tune = *flag? on: off)) playTune(tune);
  return *flag;
}
#define TOGGLE(flag, off, on) toggleFlag(&flag, command, off, on)
#define TOGGLE_NOPLAY(flag) TOGGLE(flag, NULL, NULL)
#define TOGGLE_PLAY(flag) TOGGLE(flag, &tune_toggle_off, &tune_toggle_on)

static inline int
showAttributesUnderline (void) {
  return prefs.showAttributes && (!prefs.blinkingAttributes || attributesState);
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
      } else if (column >= (ses->winx + textCount)) {
        ses->winx = column + 1 - textCount;
      }

      if (row < ses->winy) {
        ses->winy = row;
      } else if (row >= (ses->winy + brl.textRows)) {
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
        if (ses->winy < (scr.rows - brl.textRows)) {
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
          while ((line >= 0) && (line <= (scr.rows - brl.textRows))) {
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

            while ((line >= 0) && (line <= (scr.rows - brl.textRows))) {
              const wchar_t *address = buffer;
              size_t length = scr.cols;
              readScreenText(0, line, length, 1, buffer);

              {
                int i;
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
        TOGGLE_PLAY(prefs.showCursor);
        break;
      case BRL_CMD_CSRHIDE:
        /* This is for briefly hiding the cursor */
        TOGGLE_NOPLAY(ses->hideCursor);
        /* no tune */
        break;
      case BRL_CMD_CSRSIZE:
        TOGGLE_PLAY(prefs.cursorStyle);
        break;
      case BRL_CMD_CSRTRK:
        if (TOGGLE(ses->trackCursor, &tune_cursor_unlinked, &tune_cursor_linked)) {
#ifdef ENABLE_SPEECH_SUPPORT
          if (speechTracking && (scr.number == speechScreen)) {
            speechIndex = -1;
          } else
#endif /* ENABLE_SPEECH_SUPPORT */
            trackCursor(1);
        }
        break;
      case BRL_CMD_CSRBLINK:
        setBlinkingCursor(1);
        if (TOGGLE_PLAY(prefs.blinkingCursor)) {
          setBlinkingAttributes(1);
          setBlinkingCapitals(0);
        }
        break;

      case BRL_CMD_ATTRVIS:
        TOGGLE_PLAY(prefs.showAttributes);
        break;
      case BRL_CMD_ATTRBLINK:
        setBlinkingAttributes(1);
        if (TOGGLE_PLAY(prefs.blinkingAttributes)) {
          setBlinkingCapitals(1);
          setBlinkingCursor(0);
        }
        break;

      case BRL_CMD_CAPBLINK:
        setBlinkingCapitals(1);
        if (TOGGLE_PLAY(prefs.blinkingCapitals)) {
          setBlinkingAttributes(0);
          setBlinkingCursor(0);
        }
        break;

      case BRL_CMD_SKPIDLNS:
        TOGGLE_PLAY(prefs.skipIdenticalLines);
        break;
      case BRL_CMD_SKPBLNKWINS:
        TOGGLE_PLAY(prefs.skipBlankWindows);
        break;
      case BRL_CMD_SLIDEWIN:
        TOGGLE_PLAY(prefs.slidingWindow);
        break;

      case BRL_CMD_DISPMD:
        TOGGLE_NOPLAY(ses->displayMode);
        break;
      case BRL_CMD_SIXDOTS:
        TOGGLE_PLAY(prefs.textStyle);
        break;

      case BRL_CMD_AUTOREPEAT:
        if (TOGGLE_PLAY(prefs.autorepeat)) resetAutorepeat();
        break;
      case BRL_CMD_TUNES:
        TOGGLE_PLAY(prefs.alertTunes);        /* toggle sound on/off */
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
        if (inputModifiers & modifier) {
          inputModifiers &= ~modifier;
          playTune(&tune_toggle_off);
        } else {
          inputModifiers |= modifier;
          playTune(&tune_toggle_on);
        }
        break;
      }

      case BRL_CMD_PREFMENU:
        if (isMenuScreen()) {
          if (prefs.saveOnExit)
            if (savePreferences())
              playTune(&tune_command_done);

          deactivateMenuScreen();
        } else if (activateMenuScreen()) {
          updateSessionAttributes();
          ses->hideCursor = 1;
          writeStatusCells();
          message(modeString_preferences, gettext("Preferences Menu"), 0);
          savedPreferences = prefs;
        } else {
          playTune(&tune_command_rejected);
        }
        break;
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

      case BRL_CMD_HELP:
        infoMode = 0;
        if (isHelpScreen()) {
          deactivateHelpScreen();
        } else if (!activateHelpScreen()) {
          message(NULL, gettext("help not available"), 0);
        }
        break;
      case BRL_CMD_INFO:
        if ((prefs.statusPosition == spNone) || haveStatusCells()) {
          TOGGLE_NOPLAY(infoMode);
        } else {
          TOGGLE_NOPLAY(textMaximized);
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
        TOGGLE_PLAY(prefs.autospeak);
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
        if (speech->rate && (prefs.speechRate > 0)) {
          setSpeechRate(&spk, --prefs.speechRate, 1);
        } else {
          playTune(&tune_command_rejected);
        }
        break;
      case BRL_CMD_SAY_FASTER:
        if (speech->rate && (prefs.speechRate < SPK_RATE_MAXIMUM)) {
          setSpeechRate(&spk, ++prefs.speechRate, 1);
        } else {
          playTune(&tune_command_rejected);
        }
        break;

      case BRL_CMD_SAY_SOFTER:
        if (speech->volume && (prefs.speechVolume > 0)) {
          setSpeechVolume(&spk, --prefs.speechVolume, 1);
        } else {
          playTune(&tune_command_rejected);
        }
        break;
      case BRL_CMD_SAY_LOUDER:
        if (speech->volume && (prefs.speechVolume < SPK_VOLUME_MAXIMUM)) {
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
        if ((column = findFirstNonblankCharacter(characters, scr.cols)) >= 0) {
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
        if ((column = findLastNonblankCharacter(characters, scr.cols)) >= 0) {
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

      case BRL_CMD_SPEAK_PREV_LINE:
        if (ses->spky > 0) {
          ses->spky -= 1;
          speakCurrentLine();
        } else {
          playTune(&tune_bounce);
        }
        break;

      case BRL_CMD_SPEAK_NEXT_LINE:
        if (ses->spky < (scr.rows - 1)) {
          ses->spky += 1;
          speakCurrentLine();
        } else {
          playTune(&tune_bounce);
        }
        break;

      case BRL_CMD_SPEAK_FRST_LINE: {
        ScreenCharacter characters[scr.cols];
        int row = 0;

        while (row < scr.rows) {
          readScreen(0, row, scr.cols, 1, characters);
          if (findFirstNonblankCharacter(characters, scr.cols) >= 0) break;
          row += 1;
        }

        if (row < scr.rows) {
          ses->spky = row;
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
          if (findFirstNonblankCharacter(characters, scr.cols) >= 0) break;
          row -= 1;
        }

        if (row >= 0) {
          ses->spky = row;
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

      case BRL_CMD_ROUTE_LOCATION:
        if (routeCursor(ses->spkx, ses->spky, scr.number)) {
          playTune(&tune_routing_started);
        } else {
          playTune(&tune_command_rejected);
        }
        break;

      case BRL_CMD_SPEAK_LOCATION: {
        char buffer[0X50];
        snprintf(buffer, sizeof(buffer), "%d, %d", ses->spky+1, ses->spkx+1);
        sayString(&spk, buffer, 1);
        break;
      }
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
            static const wchar_t codeset[] =
              WS_C("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
              WS_C("abcdefghijklmnopqrstuvwxyz")
              WS_C("0123456789");
            const size_t codesetLength = wcslen(codeset);

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

  if (!isSuspended
#ifdef ENABLE_API
      && (!apiStarted || api_claimDriver(&brl))
#endif /* ENABLE_API */
      ) {
    int pointerMoved = 0;

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
    if (prefs.blinkingCursor)
      if ((cursorTimer -= updateInterval) <= 0)
        setBlinkingCursor(!cursorState);
    if (prefs.blinkingAttributes)
      if ((attributesTimer -= updateInterval) <= 0)
        setBlinkingAttributes(!attributesState);
    if (prefs.blinkingCapitals)
      if ((capitalsTimer -= updateInterval) <= 0)
        setBlinkingCapitals(!capitalsState);

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
            setBlinkingCursor(0);
          } else if (scr.posx != ses->trkx) {
            /* turn on cursor to see it moving on the line */
            setBlinkingCursor(1);
          }
        }

        /* If the cursor moves in cursor tracking mode: */
        if (!isRouting() && (scr.posx != ses->trkx || scr.posy != ses->trky)) {
          int oldx = ses->winx;
          int oldy = ses->winy;

          trackCursor(0);
          logMessage(LOG_DEBUG, "cursor tracking: scr=%u csr=[%u,%u]->[%u,%u] win=[%u,%u]->[%u,%u]",
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
    if (prefs.autospeak) {
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

        if (oldCharacters) {
          if ((newScreen == oldScreen) && (ses->winy == oldwiny) && (newWidth == oldWidth)) {
            int onScreen = (newX >= 0) && (newX < newWidth);

            if (!isSameRow(newCharacters, oldCharacters, newWidth, isSameText)) {
              if ((newY == ses->winy) && (newY == oldY) && onScreen) {
                if ((newX == oldX) &&
                    isSameRow(newCharacters, oldCharacters, newX, isSameText)) {
                  int oldLength = oldWidth;
                  int newLength = newWidth;
                  int x = newX + 1;

                  while (oldLength > oldX) {
                    if (!iswspace(oldCharacters[oldLength-1].text)) break;
                    oldLength -= 1;
                  }

                  while (newLength > newX) {
                    if (!iswspace(newCharacters[newLength-1].text)) break;
                    newLength -= 1;
                  }

                  while (1) {
                    int done = 1;

                    if (x < newLength) {
                      if (isSameRow(newCharacters+x, oldCharacters+oldX, newWidth-x, isSameText)) {
                        column = newX;
                        count = prefs.autospeakInsertedCharacters? (x - newX): 0;
                        goto speak;
                      }

                      done = 0;
                    }

                    if (x < oldLength) {
                      if (isSameRow(newCharacters+newX, oldCharacters+x, oldWidth-x, isSameText)) {
                        characters = oldCharacters;
                        column = oldX;
                        count = prefs.autospeakDeletedCharacters? (x - oldX): 0;
                        goto speak;
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
                    int found = 0;

                    if (iswspace(characters[column+count-1].text)) {
                      while (column > 0) {
                        if (iswspace(characters[--column].text)) {
                          column += 1;
                          break;
                        }

                        count += 1;
                        found = 1;
                      }
                    }

                    if (found) goto speak;
                  }

                  if (!prefs.autospeakInsertedCharacters) count = 0;
                  goto speak;
                }

                if (oldX >= oldWidth) oldX = oldWidth - 1;
                if ((newX < oldX) &&
                    isSameRow(newCharacters, oldCharacters, newX, isSameText) &&
                    isSameRow(newCharacters+newX, oldCharacters+oldX, oldWidth-oldX, isSameText)) {
                  characters = oldCharacters;
                  column = newX;
                  count = prefs.autospeakDeletedCharacters? (oldX - newX): 0;
                  goto speak;
                }
              }

              while (newCharacters[column].text == oldCharacters[column].text) ++column;
              while (newCharacters[count-1].text == oldCharacters[count-1].text) --count;
              count -= column;
              if (!prefs.autospeakReplacedCharacters) count = 0;
            } else if ((newY == ses->winy) && ((newX != oldX) || (newY != oldY)) && onScreen) {
              column = newX;
              count = prefs.autospeakCurrentCharacter? 1: 0;

              if (prefs.autospeakCompletedWords) {
                if (column >= 2) {
                  int length = newWidth;

                  while (length > 0) {
                    if (!iswspace(characters[--length].text)) {
                      length += 1;
                      break;
                    }
                  }

                  if ((length + 1) == column) {
                    column = length - 1;

                    while (column > 0) {
                      if (iswspace(characters[--column].text)) {
                        column += 1;
                        break;
                      }
                    }

                    count = length - column;
                    goto speak;
                  }
                }
              }
            } else {
              count = 0;
            }
          }
        }

      speak:
        if (count) {
          sayScreenCharacters(characters+column, count, 1);
        }
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
        setBlinkingAttributes(1);
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
        brl.cursor = -1;

#ifdef ENABLE_CONTRACTED_BRAILLE
        contracted = 0;
        if (isContracting()) {
          while (1) {
            int cursorOffset = getCursorOffset();

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
                         contractedOffsets, getContractedCursor(cursorOffset));

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

              if (cursorOffset < inputEnd) {
                while (cursorOffset >= 0) {
                  int offset = contractedOffsets[cursorOffset];
                  if (offset != CTB_NO_OFFSET) {
                    brl.cursor = ((offset / textCount) * brl.textColumns) + textStart + (offset % textCount);
                    break;
                  }
                  --cursorOffset;
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

          /*
           * If the cursor is visible and in range: 
           */
          if ((scr.posx >= ses->winx) && (scr.posx < (ses->winx + textCount)) &&
              (scr.posy >= ses->winy) && (scr.posy < (ses->winy + brl.textRows)) &&
              (scr.posx < scr.cols) && (scr.posy < scr.rows))
            brl.cursor = ((scr.posy - ses->winy) * brl.textColumns) + textStart + scr.posx - ses->winx;

          /* blank out capital letters if they're blinking and should be off */
          if (prefs.blinkingCapitals && !capitalsState) {
            int i;
            for (i=0; i<textCount*brl.textRows; i++) {
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

        if (brl.cursor >= 0) {
          if (showCursor()) {
            brl.buffer[brl.cursor] |= getCursorDots();
          }
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
    if (apiStarted) api_releaseDriver(&brl);
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

int 
message (const char *mode, const char *text, short flags) {
  int ok = 1;

  if (!mode) mode = "";

#ifdef ENABLE_SPEECH_SUPPORT
  if (prefs.autospeak && !(flags & MSG_SILENT)) sayString(&spk, text, 1);
#endif /* ENABLE_SPEECH_SUPPORT */

  if (braille && brl.buffer) {
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
        while (1) {
          int command = readCommand(KTB_CTX_WAITING);
          if (terminationCount) break;
          if (command == EOF) {
            drainBrailleOutput(&brl, updateInterval);
            closeTuneDevice(0);
          } else if (command != BRL_CMD_NOOP) {
            break;
          }
        }
      } else if (length || !(flags & MSG_NODELAY)) {
        int i;
        for (i=0; i<messageDelay; i+=updateInterval) {
          int command;

          if (terminationCount) break;
          drainBrailleOutput(&brl, updateInterval);

          command = readCommand(KTB_CTX_WAITING);
          if (command != EOF) break;
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
