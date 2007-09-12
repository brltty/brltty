/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/*
 * main.c - Main processing loop plus signal handling
 */

#include "prologue.h"

#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

#include "misc.h"
#include "message.h"
#include "tunes.h"
#include "ctb.h"
#include "route.h"
#include "cut.h"
#include "touch.h"
#include "cmd.h"
#include "kbd.h"
#include "scr.h"
#include "brl.h"
#ifdef ENABLE_SPEECH_SUPPORT
#include "spk.h"
#endif /* ENABLE_SPEECH_SUPPORT */
#include "brltty.h"
#include "defaults.h"

int updateInterval = DEFAULT_UPDATE_INTERVAL;
int messageDelay = DEFAULT_MESSAGE_DELAY;

static volatile int terminationSignal = 0;

/*
 * Misc param variables
 */
Preferences prefs;                /* environment (i.e. global) parameters */
static unsigned char infoMode = 0; /* display screen image or info */

BrailleDisplay brl;                        /* For the Braille routines */
short fwinshift;                /* Full window horizontal distance */
short hwinshift;                /* Half window horizontal distance */
short vwinshift;                /* Window vertical distance */

#ifdef ENABLE_CONTRACTED_BRAILLE
static int contracted = 0;
static int contractedLength;
static int contractedStart;
static int contractedOffsets[0X100];
static int contractedTrack = 0;
#endif /* ENABLE_CONTRACTED_BRAILLE */

static unsigned int updateIntervals = 0;        /* incremented each main loop cycle */


/*
 * useful macros
 */

#define BRL_ISUPPER(c) \
  (isupper((c)) || (c)=='@' || (c)=='[' || (c)=='^' || (c)==']' || (c)=='\\')

static unsigned char *translationTable = textTable;        /* active translation table */
static void
setTranslationTable (int attributes) {
  translationTable = attributes? attributesTable: textTable;
}


/* 
 * Structure definition for volatile screen state variables.
 */
typedef struct {
  short column;
  short row;
} ScreenMark;
typedef struct {
  unsigned char trackCursor;		/* cursor tracking mode */
  unsigned char hideCursor;		/* for temporarily hiding the cursor */
  unsigned char showAttributes;		/* text or attributes display */
  int winx, winy;	/* upper-left corner of braille window */
  int motx, moty;	/* last user motion of braille window */
  int trkx, trky;	/* tracked cursor position */
  int ptrx, ptry;	/* last known mouse/pointer position */
  ScreenMark marks[0X100];
} ScreenState;

static const ScreenState initialScreenState = {
  .trackCursor = DEFAULT_TRACK_CURSOR,
  .hideCursor = DEFAULT_HIDE_CURSOR,

  .ptrx = -1,
  .ptry = -1
};

/* 
 * Array definition containing pointers to ScreenState structures for 
 * each screen.  Each structure is dynamically allocated when its 
 * screen is used for the first time.
 */
static ScreenState **screenStates = NULL;
static int screenCount = 0;
static ScreenState *p;
ScreenDescription scr;          /* For screen state infos */

static void
getScreenAttributes (void) {
  int previousScreen = scr.number;

  describeScreen(&scr);
  if (scr.number == -1) scr.number = userVirtualTerminal(0);

  if (scr.number >= screenCount) {
    int newCount = (scr.number + 1) | 0XF;
    screenStates = reallocWrapper(screenStates, newCount*sizeof(*screenStates));
    while (screenCount < newCount) screenStates[screenCount++] = NULL;
  } else if (scr.number == previousScreen) {
    return;
  }

  {
    ScreenState **state = &screenStates[scr.number];
    if (!*state) {
      *state = mallocWrapper(sizeof(**state));
      **state = initialScreenState;
    }
    p = *state;
  }

  setTranslationTable(p->showAttributes);
}

static void
updateScreenAttributes (void) {
  getScreenAttributes();

  {
    int maximum = MAX(scr.rows-(int)brl.y, 0);
    int *table[] = {&p->winy, &p->moty, NULL};
    int **value = table;
    while (*value) {
      if (**value > maximum) **value = maximum;
      ++value;
    }
  }

  {
    int maximum = MAX(scr.cols-1, 0);
    int *table[] = {&p->winx, &p->motx, NULL};
    int **value = table;
    while (*value) {
      if (**value > maximum) **value = maximum;
      ++value;
    }
  }
}

static void
exitScreenStates (void) {
  if (screenStates) {
    while (screenCount > 0) free(screenStates[--screenCount]);
    free(screenStates);
    screenStates = NULL;
  } else {
    screenCount = 0;
  }
}

static void
exitLog (void) {
  /* Reopen syslog (in case -e closed it) so that there will
   * be a "stopped" message to match the "starting" message.
   */
  LogOpen(0);
  setPrintOff();
  LogPrint(LOG_INFO, gettext("terminated"));
  LogClose();
}

static void
handleSignal (int number, void (*handler) (int)) {
#ifdef HAVE_SIGACTION
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  sigemptyset(&action.sa_mask);
  action.sa_handler = handler;
  if (sigaction(number, &action, NULL) == -1) {
    LogError("sigaction");
  }
#else /* HAVE_SIGACTION */
  if (signal(number, handler) == SIG_ERR) {
    LogError("signal");
  }
#endif /* HAVE_SIGACTION */
}

void
testProgramTermination (void) {
  if (terminationSignal) exit(0);
}

static void 
terminationHandler (int signalNumber) {
  terminationSignal = signalNumber;
}

#ifdef SIGCHLD
static void
childDeathHandler (int signalNumber) {
  pid_t process;
  int status;
  while ((process = waitpid(-1, &status, WNOHANG)) > 0) {
    if (process == routingProcess) {
      routingProcess = 0;
      routingStatus = WIFEXITED(status)? WEXITSTATUS(status): ROUTE_ERROR;
    }
  }
}
#endif /* SIGCHLD */

static int
testRoutingStatus (RoutingStatus ok) {
  if (routingStatus != ROUTE_NONE) {
    playTune((routingStatus > ok)? &tune_routing_failed: &tune_routing_succeeded);
    routingStatus = ROUTE_NONE;
    return 1;
  }
  return 0;
}

static void
awaitRoutingStatus (RoutingStatus ok) {
#ifdef SIGCHLD
  sigset_t newMask, oldMask;
  sigemptyset(&newMask);
  sigaddset(&newMask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &newMask, &oldMask);
#endif /* SIGCHLD */

  while (!testRoutingStatus(ok)) {
#ifdef SIGCHLD
    sigsuspend(&oldMask);
#else /* SIGCHLD */
    approximateDelay(100);
#endif /* SIGCHLD */
  }

#ifdef SIGCHLD
  sigprocmask(SIG_SETMASK, &oldMask, NULL);
#endif /* SIGCHLD */
}

static void
setDigitUpper (unsigned char *cell, int digit) {
  *cell |= portraitDigits[digit];
}

static void
setDigitLower (unsigned char *cell, int digit) {
  *cell |= lowerDigit(portraitDigits[digit]);
}

static void
setNumberUpper (unsigned char *cells, int number) {
  setDigitUpper(&cells[0], (number / 10) % 10);
  setDigitUpper(&cells[1], number % 10);
}

static void
setNumberLower (unsigned char *cells, int number) {
  setDigitLower(&cells[0], (number / 10) % 10);
  setDigitLower(&cells[1], number % 10);
}

static void
setNumberVertical (unsigned char *cell, int number) {
  setDigitUpper(cell, (number / 10) % 10);
  setDigitLower(cell, number % 10);
}

static void
setCoordinateUpper (unsigned char *cells, int x, int y) {
  setNumberUpper(&cells[0], x);
  setNumberUpper(&cells[2], y);
}

static void
setCoordinateLower (unsigned char *cells, int x, int y) {
  setNumberLower(&cells[0], x);
  setNumberLower(&cells[2], y);
}

static void
setCoordinateVertical (unsigned char *cells, int x, int y) {
  setNumberUpper(&cells[0], y);
  setNumberLower(&cells[0], x);
}

static void
setCoordinateAlphabetic (unsigned char *cell, int x, int y) {
  /* The coords are given with letters as the DOS tsr */
  *cell = ((updateIntervals / 16) % (y / 25 + 1))? 0:
          textTable[y % 25 + 'a'] |
          ((x / brl.x) << 6);
}

static void
setStateLetter (unsigned char *cell) {
  *cell = textTable[p->showAttributes? 'a':
                    isFrozenScreen()? 'f':
                    p->trackCursor? 't':
                    ' '];
}

static void
setStateDots (unsigned char *cell) {
  *cell = (isFrozenScreen()    ? BRL_DOT1: 0) |
          (prefs.showCursor    ? BRL_DOT4: 0) |
          (p->showAttributes   ? BRL_DOT2: 0) |
          (prefs.cursorStyle   ? BRL_DOT5: 0) |
          (prefs.alertTunes    ? BRL_DOT3: 0) |
          (prefs.blinkingCursor? BRL_DOT6: 0) |
          (p->trackCursor      ? BRL_DOT7: 0) |
          (prefs.slidingWindow ? BRL_DOT8: 0);
}

static void
setStatusCellsNone (unsigned char *status) {
}

static void
setStatusCellsAlva (unsigned char *status) {
  if (isHelpScreen()) {
    status[0] = textTable['h'];
    status[1] = textTable['l'];
    status[2] = textTable['p'];
  } else {
    setCoordinateAlphabetic(&status[0], scr.posx, scr.posy);
    setCoordinateAlphabetic(&status[1], p->winx, p->winy);
    setStateLetter(&status[2]);
  }
}

static void
setStatusCellsTieman (unsigned char *status) {
  setCoordinateUpper(&status[0], scr.posx+1, scr.posy+1);
  setCoordinateLower(&status[0], p->winx+1, p->winy+1);
  setStateDots(&status[4]);
}

static void
setStatusCellsPB80 (unsigned char *status) {
  setNumberVertical(&status[0], p->winy+1);
}

static void
setStatusCellsGeneric (unsigned char *status) {
  status[BRL_firstStatusCell] = BRL_STATUS_CELLS_GENERIC;
  status[BRL_GSC_BRLCOL] = p->winx+1;
  status[BRL_GSC_BRLROW] = p->winy+1;
  status[BRL_GSC_CSRCOL] = scr.posx+1;
  status[BRL_GSC_CSRROW] = scr.posy+1;
  status[BRL_GSC_SCRNUM] = scr.number;
  status[BRL_GSC_FREEZE] = isFrozenScreen();
  status[BRL_GSC_DISPMD] = p->showAttributes;
  status[BRL_GSC_SIXDOTS] = prefs.textStyle;
  status[BRL_GSC_SLIDEWIN] = prefs.slidingWindow;
  status[BRL_GSC_SKPIDLNS] = prefs.skipIdenticalLines;
  status[BRL_GSC_SKPBLNKWINS] = prefs.skipBlankWindows;
  status[BRL_GSC_CSRVIS] = prefs.showCursor;
  status[BRL_GSC_CSRHIDE] = p->hideCursor;
  status[BRL_GSC_CSRTRK] = p->trackCursor;
  status[BRL_GSC_CSRSIZE] = prefs.cursorStyle;
  status[BRL_GSC_CSRBLINK] = prefs.blinkingCursor;
  status[BRL_GSC_ATTRVIS] = prefs.showAttributes;
  status[BRL_GSC_ATTRBLINK] = prefs.blinkingAttributes;
  status[BRL_GSC_CAPBLINK] = prefs.blinkingCapitals;
  status[BRL_GSC_TUNES] = prefs.alertTunes;
  status[BRL_GSC_HELP] = isHelpScreen();
  status[BRL_GSC_INFO] = infoMode;
  status[BRL_GSC_AUTOREPEAT] = prefs.autorepeat;
  status[BRL_GSC_AUTOSPEAK] = prefs.autospeak;
}

static void
setStatusCellsMDV (unsigned char *status) {
  setCoordinateVertical(&status[0], p->winx+1, p->winy+1);
}

static void
setStatusCellsVoyager (unsigned char *status) {
  setNumberVertical(&status[0], p->winy+1);
  setNumberVertical(&status[1], scr.posy+1);
  if (isFrozenScreen()) {
    status[2] = textTable['F'];
  } else {
    setNumberVertical(&status[2], scr.posx+1);
  }
}

typedef void (*SetStatusCellsHandler) (unsigned char *status);
typedef struct {
  SetStatusCellsHandler set;
  unsigned char count;
} StatusStyleEntry;
static const StatusStyleEntry statusStyleTable[] = {
  {setStatusCellsNone, 0},
  {setStatusCellsAlva, 3},
  {setStatusCellsTieman, 5},
  {setStatusCellsPB80, 1},
  {setStatusCellsGeneric, 0},
  {setStatusCellsMDV, 2},
  {setStatusCellsVoyager, 3}
};
static const int statusStyleCount = ARRAY_COUNT(statusStyleTable);

static void
setStatusCells (void) {
  unsigned char status[BRL_MAX_STATUS_CELL_COUNT];        /* status cell buffer */
  memset(status, 0, sizeof(status));
  if (prefs.statusStyle < statusStyleCount)
    statusStyleTable[prefs.statusStyle].set(status);
  braille->writeStatus(&brl, status);
}

static void
showInfo (void) {
  /* Here we must be careful. Some displays (e.g. Braille Lite 18)
   * are very small, and others (e.g. Bookworm) are even smaller.
   */
  char text[22];
  setStatusText(&brl, gettext("info"));

  if (brl.x*brl.y >= 21) {
    snprintf(text, sizeof(text), "%02d:%02d %02d:%02d %02d %c%c%c%c%c%c",
             p->winx+1, p->winy+1, scr.posx+1, scr.posy+1, scr.number, 
             p->trackCursor? 't': ' ',
             prefs.showCursor? (prefs.blinkingCursor? 'B': 'v'):
                               (prefs.blinkingCursor? 'b': ' '),
             p->showAttributes? 'a': 't',
             isFrozenScreen()? 'f': ' ',
             prefs.textStyle? '6': '8',
             prefs.blinkingCapitals? 'B': ' ');
    writeBrailleString(&brl, text);
  } else {
    brl.cursor = -1;
    snprintf(text, sizeof(text), "xxxxx %02d %c%c%c%c%c%c     ",
             scr.number,
             p->trackCursor? 't': ' ',
             prefs.showCursor? (prefs.blinkingCursor? 'B': 'v'):
                               (prefs.blinkingCursor? 'b': ' '),
             p->showAttributes? 'a': 't',
             isFrozenScreen()? 'f': ' ',
             prefs.textStyle? '6': '8',
             prefs.blinkingCapitals? 'B': ' ');

    if (braille->writeVisual) {
      memcpy(brl.buffer, text, brl.x*brl.y);
      braille->writeVisual(&brl);
    }

    {
      unsigned char dots[sizeof(text)];
      int i;

      memset(&dots, 0, 5);
      setCoordinateUpper(&dots[0], scr.posx+1, scr.posy+1);
      setCoordinateLower(&dots[0], p->winx+1, p->winy+1);
      setStateDots(&dots[4]);
      for (i=5; text[i]; i++) dots[i] = textTable[(unsigned char)text[i]];
      memcpy(brl.buffer, dots, brl.x*brl.y);
      braille->writeWindow(&brl);
    }
  }
}

static void 
slideWindowVertically (int y) {
  if (y < p->winy)
    p->winy = y;
  else if  (y >= (p->winy + brl.y))
    p->winy = y - (brl.y - 1);
}

static void 
placeWindowHorizontally (int x) {
  p->winx = x / brl.x * brl.x;
}

static void 
trackCursor (int place) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (contracted) {
    p->winy = scr.posy;
    if (scr.posx < p->winx) {
      int length = scr.posx + 1;
      unsigned char buffer[length];
      int onspace = 1;
      readScreen(0, p->winy, length, 1, buffer, SCR_TEXT);
      while (length) {
        if ((isspace(buffer[--length]) != 0) != onspace) {
          if (onspace) {
            onspace = 0;
          } else {
            ++length;
            break;
          }
        }
      }
      p->winx = length;
    }
    contractedTrack = 1;
    return;
  }
#endif /* ENABLE_CONTRACTED_BRAILLE */

  if (place)
    if ((scr.posx < p->winx) || (scr.posx >= (p->winx + brl.x)) ||
        (scr.posy < p->winy) || (scr.posy >= (p->winy + brl.y)))
      placeWindowHorizontally(scr.posx);

  if (prefs.slidingWindow) {
    int reset = brl.x * 3 / 10;
    int trigger = prefs.eagerSlidingWindow? brl.x*3/20: 0;
    if (scr.posx < (p->winx + trigger))
      p->winx = MAX(scr.posx-reset, 0);
    else if (scr.posx >= (p->winx + brl.x - trigger))
      p->winx = MAX(MIN(scr.posx+reset+1, scr.cols)-(int)brl.x, 0);
  } else if (scr.posx < p->winx) {
    p->winx -= ((p->winx - scr.posx - 1) / brl.x + 1) * brl.x;
    if (p->winx < 0) p->winx = 0;
  } else
    p->winx += (scr.posx - p->winx) / brl.x * brl.x;

  slideWindowVertically(scr.posy);
}

#ifdef ENABLE_SPEECH_SUPPORT
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
sayRegion (int left, int top, int width, int height, int track, SayMode mode) {
  /* OK heres a crazy idea: why not send the attributes with the
   * text, in case some inflection or marking can be added...! The
   * speech driver's say function will receive a buffer of text
   * and a length, but in reality the buffer will contain twice
   * len bytes: the text followed by the video attribs data.
   */
  int length = width * height;
  unsigned char buffer[length * 2];

  if (mode == sayImmediate) speech->mute();
  readScreen(left, top, width, height, buffer, SCR_TEXT);
  if (speech->express) {
    readScreen(left, top, width, height, buffer+length, SCR_ATTRIB);
    speech->express(buffer, length);
  } else {
    speech->say(buffer, length);
  }

  speechTracking = track;
  speechScreen = scr.number;
  speechLine = top;
}

static void
sayLines (int line, int count, int track, SayMode mode) {
  sayRegion(0, line, scr.cols, count, track, mode);
}
#endif /* ENABLE_SPEECH_SUPPORT */

static inline int
showCursor (void) {
  return scr.cursor && prefs.showCursor && !p->hideCursor;
}

static int
toDifferentLine (ScreenCharacterProperty property, int (*canMove) (void), int amount, int from, int width) {
  if (canMove()) {
    unsigned char buffer1[width], buffer2[width];
    int skipped = 0;

    if ((property == SCR_TEXT) && p->showAttributes) property = SCR_ATTRIB;
    readScreen(from, p->winy, width, 1, buffer1, property);

    do {
      readScreen(from, p->winy+=amount, width, 1, buffer2, property);
      if ((memcmp(buffer1, buffer2, width) != 0) ||
          ((property == SCR_TEXT) && showCursor() && (scr.posy == p->winy) &&
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
    } while (canMove());
  }

  playTune(&tune_bounce);
  return 0;
}

static int
canMoveUp (void) {
  return p->winy > 0;
}

static int
canMoveDown (void) {
  return p->winy < (scr.rows - brl.y);
}

static int
upDifferentLine (ScreenCharacterProperty property) {
  return toDifferentLine(property, canMoveUp, -1, 0, scr.cols);
}

static int
downDifferentLine (ScreenCharacterProperty property) {
  return toDifferentLine(property, canMoveDown, 1, 0, scr.cols);
}

static int
upDifferentCharacter (ScreenCharacterProperty property, int column) {
  return toDifferentLine(property, canMoveUp, -1, column, 1);
}

static int
downDifferentCharacter (ScreenCharacterProperty property, int column) {
  return toDifferentLine(property, canMoveDown, 1, column, 1);
}

static void
upOneLine (ScreenCharacterProperty property) {
  if (p->winy > 0) {
    p->winy--;
  } else {
    playTune(&tune_bounce);
  }
}

static void
downOneLine (ScreenCharacterProperty property) {
  if (p->winy < (scr.rows - brl.y)) {
    p->winy++;
  } else {
    playTune(&tune_bounce);
  }
}

static void
upLine (ScreenCharacterProperty property) {
  if (prefs.skipIdenticalLines) {
    upDifferentLine(property);
  } else {
    upOneLine(property);
  }
}

static void
downLine (ScreenCharacterProperty property) {
  if (prefs.skipIdenticalLines) {
    downDifferentLine(property);
  } else {
    downOneLine(property);
  }
}

typedef int (*RowTester) (int column, int row, void *data);
static void
findRow (int column, int increment, RowTester test, void *data) {
  int row = p->winy + increment;
  while ((row >= 0) && (row <= scr.rows-(int)brl.y)) {
    if (test(column, row, data)) {
      p->winy = row;
      return;
    }
    row += increment;
  }
  playTune(&tune_bounce);
}

static int
testIndent (int column, int row, void *data) {
  int count = column+1;
  unsigned char buffer[count];
  readScreen(0, row, count, 1, buffer, SCR_TEXT);
  while (column >= 0) {
    if ((buffer[column] != ' ') && (buffer[column] != 0)) return 1;
    --column;
  }
  return 0;
}

static int
testPrompt (int column, int row, void *data) {
  const unsigned char *prompt = data;
  int count = column+1;
  unsigned char buffer[count];
  readScreen(0, row, count, 1, buffer, SCR_TEXT);
  return memcmp(buffer, prompt, count) == 0;
}

static int
findBytes (const unsigned char **address, size_t *length, const unsigned char *bytes, size_t count) {
  const unsigned char *ptr = *address;
  size_t len = *length;

  while (count <= len) {
    const unsigned char *next = memchr(ptr, *bytes, len);
    if (!next) break;

    len -= next - ptr;
    if (memcmp((ptr = next), bytes, count) == 0) {
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
  return prefs.textStyle && contractionTable;
}

static int
getCursorOffset (int x, int y) {
  if ((scr.posy == y) && (scr.posx >= x)) return scr.posx - x;
  return -1;
}

static int
getContractedLength (int x, int y) {
  int inputLength = scr.cols - x;
  int outputLength = brl.x * brl.y;
  unsigned char inputBuffer[inputLength];
  unsigned char outputBuffer[outputLength];
  int outputOffsets[inputLength];
  readScreen(x, y, inputLength, 1, inputBuffer, SCR_TEXT);
  if (!contractText(contractionTable,
                    inputBuffer, &inputLength,
                    outputBuffer, &outputLength,
                    outputOffsets, getCursorOffset(x, y))) return 0;
  return inputLength;
}
#endif /* ENABLE_CONTRACTED_BRAILLE */

static void
placeRightEdge (int column) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracting()) {
    p->winx = 0;
    while (1) {
      int length = getContractedLength(p->winx, p->winy);
      int end = p->winx + length;
      if (end > column) break;
      p->winx = end;
    }
  } else
#endif /* ENABLE_CONTRACTED_BRAILLE */
  {
    p->winx = column / brl.x * brl.x;
  }
}

static void
placeWindowRight (void) {
  placeRightEdge(scr.cols-1);
}

static int
shiftWindowLeft (void) {
  if (p->winx == 0) return 0;

#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracting()) {
    placeRightEdge(p->winx-1);
  } else
#endif /* ENABLE_CONTRACTED_BRAILLE */
  {
    p->winx -= MIN(fwinshift, p->winx);
  }

  return 1;
}

static int
shiftWindowRight (void) {
  int shift;

#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracting()) {
    shift = getContractedLength(p->winx, p->winy);
  } else
#endif /* ENABLE_CONTRACTED_BRAILLE */
  {
    shift = fwinshift;
  }

  if (p->winx >= (scr.cols - shift)) return 0;
  p->winx += shift;
  return 1;
}

static int
getWindowLength (void) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracting()) return getContractedLength(p->winx, p->winy);
#endif /* ENABLE_CONTRACTED_BRAILLE */

  return brl.x;
}

static int
getOffset (int arg, int end) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (contracted) {
    int result = 0;
    int index;
    for (index=0; index<contractedLength; ++index) {
      int offset = contractedOffsets[index];
      if (offset != -1) {
        if (offset > arg) {
          if (end) result = index - 1;
          break;
        }
        result = index;
      }
    }
    return result;
  }
#endif /* ENABLE_CONTRACTED_BRAILLE */
  return arg;
}

static void
overlayAttributes (const unsigned char *attributes, int width, int height) {
  int row;
  for (row=0; row<height; row++) {
    int column;
    for (column=0; column<width; column++) {
      switch (attributes[row*width + column]) {
        /* Experimental! Attribute values are hardcoded... */
        case 0x08: /* dark-gray on black */
        case 0x07: /* light-gray on black */
        case 0x17: /* light-gray on blue */
        case 0x30: /* black on cyan */
          break;
        case 0x70: /* black on light-gray */
          brl.buffer[row*brl.x + column] |= (BRL_DOT7 | BRL_DOT8);
          break;
        case 0x0F: /* white on black */
        default:
          brl.buffer[row*brl.x + column] |= (BRL_DOT8);
          break;
      }
    }
  }
}

unsigned char
cursorDots (void) {
  return prefs.cursorStyle?  (BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8): (BRL_DOT7 | BRL_DOT8);
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

static int
insertCharacter (unsigned char character, int flags) {
  if (islower(character)) {
    if (flags & (BRL_FLG_CHAR_SHIFT | BRL_FLG_CHAR_UPPER)) character = toupper(character);
  } else if (flags & BRL_FLG_CHAR_SHIFT) {
    switch (character) {
      case '1': character = '!'; break;
      case '2': character = '@'; break;
      case '3': character = '#'; break;
      case '4': character = '$'; break;
      case '5': character = '%'; break;
      case '6': character = '^'; break;
      case '7': character = '&'; break;
      case '8': character = '*'; break;
      case '9': character = '('; break;
      case '0': character = ')'; break;
      case '-': character = '_'; break;
      case '=': character = '+'; break;
      case '[': character = '{'; break;
      case ']': character = '}'; break;
      case'\\': character = '|'; break;
      case ';': character = ':'; break;
      case'\'': character = '"'; break;
      case '`': character = '~'; break;
      case ',': character = '<'; break;
      case '.': character = '>'; break;
      case '/': character = '?'; break;
    }
  }

  if (flags & BRL_FLG_CHAR_CONTROL) {
    if ((character & 0X6F) == 0X2F)
      character |= 0X50;
    else
      character &= 0X9F;
  }

  {
    ScreenKey key = character;
    if (flags & BRL_FLG_CHAR_META) key |= SCR_KEY_MOD_META;
    return insertKey(key);
  }
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
    if (column != p->ptrx) {
      if (p->ptrx >= 0) moved = 1;
      p->ptrx = column;
    }

    if (row != p->ptry) {
      if (p->ptry >= 0) moved = 1;
      p->ptry = row;
    }

    if (moved) {
      if (column < p->winx) {
        p->winx = column;
      } else if (column >= (p->winx + brl.x)) {
        p->winx = column + 1 - brl.x;
      }

      if (row < p->winy) {
        p->winy = row;
      } else if (row >= (p->winy + brl.y)) {
        p->winy = row + 1 - brl.y;
      }
    }
  } else {
    p->ptrx = p->ptry = -1;
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
      right = brl.x - 1;
      bottom = brl.y - 1;
    } else if (!touchGetRegion(&left, &right, &top, &bottom)) {
      unhighlightScreenRegion();
      return;
    }

    highlightScreenRegion(p->winx+left, p->winx+right, p->winy+top, p->winy+bottom);
  }
}

static int
beginProgram (int argc, char *argv[]) {
#ifdef INIT_PATH
  if ((getpid() == 1) || (strstr(argv[0], "linuxrc") != NULL)) {
    fprintf(stderr, gettext("%s started as %s\n"), PACKAGE_TITLE, argv[0]);
    fflush(stderr);
    switch (fork()) {
      case -1: /* failed */
        fprintf(stderr, gettext("Fork of %s failed: %s\n"),
                PACKAGE_TITLE, strerror(errno));
        fflush(stderr);
      default: /* parent */
        fprintf(stderr, gettext("Executing %s (from %s)\n"), "INIT", INIT_PATH);
        fflush(stderr);
      exec_init:
        execv(INIT_PATH, argv);
        /* execv() shouldn't return */
        fprintf(stderr, gettext("Execution of %s failed: %s\n"), "INIT", strerror(errno));
        fflush(stderr);
        exit(1);
      case 0: { /* child */
        static char *arguments[] = {"brltty", "-E", "-n", "-e", "-linfo", NULL};
        argv = arguments;
        argc = ARRAY_COUNT(arguments) - 1;
        break;
      }
    }
  } else if (strstr(argv[0], "brltty") == NULL) {
    /* 
     * If we are substituting the real init binary, then we may consider
     * when someone might want to call that binary even when pid != 1.
     * One example is /sbin/telinit which is a symlink to /sbin/init.
     */
    goto exec_init;
  }
#endif /* INIT_PATH */

#ifdef STDERR_PATH
  freopen(STDERR_PATH, "a", stderr);
#endif /* STDERR_PATH */

  /* Open the system log. */
  LogOpen(0);
  LogPrint(LOG_INFO, gettext("starting"));
  atexit(exitLog);

#ifdef SIGPIPE
  /* We install SIGPIPE handler before startup() so that drivers which
   * use pipes can't cause program termination (the call to message() in
   * startup() in particular).
   */
  handleSignal(SIGPIPE, SIG_IGN);
#endif /* SIGPIPE */

  /* Install the program termination handler. */
  handleSignal(SIGTERM, terminationHandler);
  handleSignal(SIGINT, terminationHandler);

  /* Setup everything required on startup */
  startup(argc, argv);

#ifdef SIGCHLD
  /* Install the handler which monitors the death of child processes. */
  handleSignal(SIGCHLD, childDeathHandler);
#endif /* SIGCHLD */

  return 0;
}

static int
runProgram (void) {
  int offline = 0;
  int suspended = 0;
  short oldwinx, oldwiny;

  atexit(exitScreenStates);
  getScreenAttributes();
  /* NB: screen size can sometimes change, e.g. the video mode may be changed
   * when installing a new font. This will be detected by another call to
   * describeScreen() within the main loop. Don't assume that scr.rows
   * and scr.cols are constants across loop iterations.
   */

  p->trkx = scr.posx; p->trky = scr.posy;
  trackCursor(1);        /* set initial window position */
  p->motx = p->winx; p->moty = p->winy;
  oldwinx = p->winx; oldwiny = p->winy;
  highlightWindow();
  checkPointer();

  kbdResetState();
  resetBlinkingStates();
  if (prefs.autorepeat) resetAutorepeat();

  while (1) {
    int pointerMoved = 0;

    testProgramTermination();
    closeTuneDevice(0);
    testRoutingStatus(ROUTE_DONE);

    if (opt_releaseDevice) {
      if (scr.unreadable) {
        if (!suspended) {
          setStatusCells();
          writeBrailleString(&brl, scr.unreadable);
#ifdef ENABLE_API
          if (apiStarted)
            api_suspend(&brl);
          else
#endif /* ENABLE_API */
            destructBrailleDriver();
          suspended = 1;
        }
      } else {
        if (suspended) {
          suspended = !(
#ifdef ENABLE_API
            apiStarted? api_resume(&brl):
#endif /* ENABLE_API */
            constructBrailleDriver());
        }
      }
    }

    if (!suspended
#ifdef ENABLE_API
	&& (!apiStarted || api_claimDriver(&brl))
#endif /* ENABLE_API */
	) {
      /*
       * Process any Braille input 
       */
      while (1) {
        int oldmotx = p->winx;
        int oldmoty = p->winy;
        BRL_DriverCommandContext context = infoMode? BRL_CTX_STATUS:
                                           isHelpScreen()? BRL_CTX_HELP:
                                           BRL_CTX_SCREEN;
        int command;
        testProgramTermination();

        command = readBrailleCommand(&brl, context);

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
            LogPrint(LOG_DEBUG, "command: %06X", command);
          } else {
            real |= (command & ~BRL_MSK_CMD);
            LogPrint(LOG_DEBUG, "command: %06X -> %06X", command, real);
            command = real;
          }

          switch (command & BRL_MSK_CMD) {
            case BRL_CMD_OFFLINE:
              if (!offline) {
                LogPrint(LOG_NOTICE, "braille display offline");
                offline = 1;
              }
              goto isOffline;

            case BRL_CMD_SHUTDOWN:
              LogPrint(LOG_NOTICE, "shutdown requested");
              exit(0);
          }
        }

        if (offline) {
          LogPrint(LOG_NOTICE, "braille display online");
          offline = 0;
        }

        handleAutorepeat(&command, NULL);
        if (command == EOF) break;

      doCommand:
        if (!executeScreenCommand(command)) {
          switch (command & BRL_MSK_CMD) {
            case BRL_CMD_NOOP:        /* do nothing but loop */
              if (command & BRL_FLG_TOGGLE_ON)
                playTune(&tune_toggle_on);
              else if (command & BRL_FLG_TOGGLE_OFF)
                playTune(&tune_toggle_off);
              break;

            case BRL_CMD_TOP_LEFT:
              p->winx = 0;
            case BRL_CMD_TOP:
              p->winy = 0;
              break;
            case BRL_CMD_BOT_LEFT:
              p->winx = 0;
            case BRL_CMD_BOT:
              p->winy = scr.rows - brl.y;
              break;

            case BRL_CMD_WINUP:
              if (p->winy == 0)
                playTune (&tune_bounce);
              p->winy = MAX (p->winy - vwinshift, 0);
              break;
            case BRL_CMD_WINDN:
              if (p->winy == scr.rows - brl.y)
                playTune (&tune_bounce);
              p->winy = MIN (p->winy + vwinshift, scr.rows - brl.y);
              break;

            case BRL_CMD_LNUP:
              upOneLine(SCR_TEXT);
              break;
            case BRL_CMD_LNDN:
              downOneLine(SCR_TEXT);
              break;

            case BRL_CMD_PRDIFLN:
              upDifferentLine(SCR_TEXT);
              break;
            case BRL_CMD_NXDIFLN:
              downDifferentLine(SCR_TEXT);
              break;

            case BRL_CMD_ATTRUP:
              upDifferentLine(SCR_ATTRIB);
              break;
            case BRL_CMD_ATTRDN:
              downDifferentLine(SCR_ATTRIB);
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
                unsigned char buffer[scr.cols];
                int findBlank = 1;
                int line = p->winy;
                int i;
                while ((line >= 0) && (line <= (scr.rows - brl.y))) {
                  readScreen(0, line, scr.cols, 1, buffer, SCR_TEXT);
                  for (i=0; i<scr.cols; i++)
                    if ((buffer[i] != ' ') && (buffer[i] != 0))
                      break;
                  if ((i == scr.cols) == findBlank) {
                    if (!findBlank) {
                      found = 1;
                      p->winy = line;
                      p->winx = 0;
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
                unsigned char buffer[scr.cols];
                unsigned char *blank;
                readScreen(0, p->winy, scr.cols, 1, buffer, SCR_TEXT);
                if ((blank = memchr(buffer, ' ', scr.cols))) {
                  findRow(blank-buffer, increment, testPrompt, buffer);
                } else {
                  playTune(&tune_command_rejected);
                }
              }
              break;
            }

            {
              int increment;
            case BRL_CMD_PRSEARCH:
              increment = -1;
              goto doSearch;
            case BRL_CMD_NXSEARCH:
              increment = 1;
            doSearch:
              if (cutBuffer) {
                int found = 0;
                size_t count = cutLength;
                if (count <= scr.cols) {
                  int line = p->winy;
                  unsigned char buffer[scr.cols];
                  unsigned char bytes[count];

                  {
                    int i;
                    for (i=0; i<count; i++) bytes[i] = tolower(cutBuffer[i]);
                  }

                  while ((line >= 0) && (line <= (scr.rows - brl.y))) {
                    const unsigned char *address = buffer;
                    size_t length = scr.cols;
                    readScreen(0, line, length, 1, buffer, SCR_TEXT);

                    {
                      int i;
                      for (i=0; i<length; i++) buffer[i] = tolower(buffer[i]);
                    }

                    if (line == p->winy) {
                      if (increment < 0) {
                        int end = p->winx + count - 1;
                        if (end < length) length = end;
                      } else {
                        int start = p->winx + brl.x;
                        if (start > length) start = length;
                        address += start;
                        length -= start;
                      }
                    }
                    if (findBytes(&address, &length, bytes, count)) {
                      if (increment < 0)
                        while (findBytes(&address, &length, bytes, count))
                          ++address, --length;

                      p->winy = line;
                      p->winx = (address - buffer) / brl.x * brl.x;
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
              if (p->winx)
                p->winx = 0;
              else
                playTune(&tune_bounce);
              break;
            case BRL_CMD_LNEND:
              if (p->winx < (scr.cols - brl.x))
                p->winx = scr.cols - brl.x;
              else
                playTune(&tune_bounce);
              break;

            case BRL_CMD_CHRLT:
              if (p->winx == 0)
                playTune (&tune_bounce);
              p->winx = MAX (p->winx - 1, 0);
              break;
            case BRL_CMD_CHRRT:
              if (p->winx < (scr.cols - 1))
                p->winx++;
              else
                playTune(&tune_bounce);
              break;

            case BRL_CMD_HWINLT:
              if (p->winx == 0)
                playTune(&tune_bounce);
              else
                p->winx = MAX(p->winx-hwinshift, 0);
              break;
            case BRL_CMD_HWINRT:
              if (p->winx < (scr.cols - hwinshift))
                p->winx += hwinshift;
              else
                playTune(&tune_bounce);
              break;

            case BRL_CMD_FWINLT:
              if (!(prefs.skipBlankWindows && (prefs.blankWindowsSkipMode == sbwAll))) {
                int oldX = p->winx;
                if (shiftWindowLeft()) {
                  if (prefs.skipBlankWindows) {
                    if (prefs.blankWindowsSkipMode == sbwEndOfLine) goto skipEndOfLine;
                    if (!showCursor() ||
                        (scr.posy != p->winy) ||
                        (scr.posx >= (p->winx + brl.x))) {
                      int charCount = MIN(scr.cols, p->winx+brl.x);
                      int charIndex;
                      unsigned char buffer[charCount];
                      readScreen(0, p->winy, charCount, 1, buffer, SCR_TEXT);
                      for (charIndex=0; charIndex<charCount; ++charIndex)
                        if ((buffer[charIndex] != ' ') && (buffer[charIndex] != 0))
                          break;
                      if (charIndex == charCount) goto wrapUp;
                    }
                  }
                  break;
                }
              wrapUp:
                if (p->winy == 0) {
                  playTune(&tune_bounce);
                  p->winx = oldX;
                  break;
                }
                playTune(&tune_wrap_up);
                upLine(SCR_TEXT);
                placeWindowRight();
              skipEndOfLine:
                if (prefs.skipBlankWindows && (prefs.blankWindowsSkipMode == sbwEndOfLine)) {
                  int charIndex;
                  unsigned char buffer[scr.cols];
                  readScreen(0, p->winy, scr.cols, 1, buffer, SCR_TEXT);
                  for (charIndex=scr.cols-1; charIndex>=0; --charIndex)
                    if ((buffer[charIndex] != ' ') && (buffer[charIndex] != 0))
                      break;
                  if (showCursor() && (scr.posy == p->winy))
                    charIndex = MAX(charIndex, scr.posx);
                  charIndex = MAX(charIndex, 0);
                  if (charIndex < p->winx) placeRightEdge(charIndex);
                }
                break;
              }
            case BRL_CMD_FWINLTSKIP: {
              int oldX = p->winx;
              int oldY = p->winy;
              int tuneLimit = 3;
              int charCount;
              int charIndex;
              unsigned char buffer[scr.cols];
              while (1) {
                if (!shiftWindowLeft()) {
                  if (p->winy == 0) {
                    playTune(&tune_bounce);
                    p->winx = oldX;
                    p->winy = oldY;
                    break;
                  }
                  if (tuneLimit-- > 0) playTune(&tune_wrap_up);
                  upLine(SCR_TEXT);
                  placeWindowRight();
                }
                charCount = getWindowLength();
                charCount = MIN(charCount, scr.cols-p->winx);
                readScreen(p->winx, p->winy, charCount, 1, buffer, SCR_TEXT);
                for (charIndex=(charCount-1); charIndex>=0; charIndex--)
                  if ((buffer[charIndex] != ' ') && (buffer[charIndex] != 0))
                    break;
                if (showCursor() &&
                    (scr.posy == p->winy) &&
                    (scr.posx < (p->winx + charCount)))
                  charIndex = MAX(charIndex, scr.posx-p->winx);
                if (charIndex >= 0) break;
              }
              break;
            }

            case BRL_CMD_FWINRT:
              if (!(prefs.skipBlankWindows && (prefs.blankWindowsSkipMode == sbwAll))) {
                int oldX = p->winx;
                if (shiftWindowRight()) {
                  if (prefs.skipBlankWindows) {
                    if (!showCursor() ||
                        (scr.posy != p->winy) ||
                        (scr.posx < p->winx)) {
                      int charCount = scr.cols - p->winx;
                      int charIndex;
                      unsigned char buffer[charCount];
                      readScreen(p->winx, p->winy, charCount, 1, buffer, SCR_TEXT);
                      for (charIndex=0; charIndex<charCount; ++charIndex)
                        if ((buffer[charIndex] != ' ') && (buffer[charIndex] != 0))
                          break;
                      if (charIndex == charCount) goto wrapDown;
                    }
                  }
                  break;
                }
              wrapDown:
                if (p->winy >= (scr.rows - brl.y)) {
                  playTune(&tune_bounce);
                  p->winx = oldX;
                  break;
                }
                playTune(&tune_wrap_down);
                downLine(SCR_TEXT);
                p->winx = 0;
                break;
              }
            case BRL_CMD_FWINRTSKIP: {
              int oldX = p->winx;
              int oldY = p->winy;
              int tuneLimit = 3;
              int charCount;
              int charIndex;
              unsigned char buffer[scr.cols];
              while (1) {
                if (!shiftWindowRight()) {
                  if (p->winy >= (scr.rows - brl.y)) {
                    playTune(&tune_bounce);
                    p->winx = oldX;
                    p->winy = oldY;
                    break;
                  }
                  if (tuneLimit-- > 0) playTune(&tune_wrap_down);
                  downLine(SCR_TEXT);
                  p->winx = 0;
                }
                charCount = getWindowLength();
                charCount = MIN(charCount, scr.cols-p->winx);
                readScreen(p->winx, p->winy, charCount, 1, buffer, SCR_TEXT);
                for (charIndex=0; charIndex<charCount; charIndex++)
                  if ((buffer[charIndex] != ' ') && (buffer[charIndex] != 0))
                    break;
                if (showCursor() &&
                    (scr.posy == p->winy) &&
                    (scr.posx >= p->winx))
                  charIndex = MIN(charIndex, scr.posx-p->winx);
                if (charIndex < charCount) break;
              }
              break;
            }

            case BRL_CMD_RETURN:
              if ((p->winx != p->motx) || (p->winy != p->moty)) {
            case BRL_CMD_BACK:
                p->winx = p->motx;
                p->winy = p->moty;
                break;
              }
            case BRL_CMD_HOME:
              trackCursor(1);
              break;

            case BRL_CMD_RESTARTBRL:
              restartBrailleDriver();
              kbdResetState();
              break;
            case BRL_CMD_PASTE:
              if (isLiveScreen() && !routingProcess) {
                if (cutPaste()) break;
              }
              playTune(&tune_command_rejected);
              break;
            case BRL_CMD_CSRJMP_VERT:
              playTune(routeCursor(-1, p->winy, scr.number)?
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
              TOGGLE_NOPLAY(p->hideCursor);
              /* no tune */
              break;
            case BRL_CMD_CSRSIZE:
              TOGGLE_PLAY(prefs.cursorStyle);
              break;
            case BRL_CMD_CSRTRK:
              if (TOGGLE(p->trackCursor, &tune_cursor_unlinked, &tune_cursor_linked)) {
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
              setTranslationTable(TOGGLE_NOPLAY(p->showAttributes));
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

#ifdef ENABLE_PREFERENCES_MENU
            case BRL_CMD_PREFMENU:
              updatePreferences();
              break;
            case BRL_CMD_PREFSAVE:
              if (savePreferences()) {
                playTune(&tune_command_done);
              }
              break;
#endif /* ENABLE_PREFERENCES_MENU */
            case BRL_CMD_PREFLOAD:
              if (loadPreferences(1)) {
                resetBlinkingStates();
                playTune(&tune_command_done);
              }
              break;

            case BRL_CMD_HELP:
              infoMode = 0;
              if (isHelpScreen()) {
                deactivateHelpScreen();
              } else if (!activateHelpScreen()) {
                message(gettext("help not available"), 0);
              }
              break;
            case BRL_CMD_INFO:
              TOGGLE_NOPLAY(infoMode);
              break;

#ifdef ENABLE_LEARN_MODE
            case BRL_CMD_LEARN:
              learnMode(&brl, updateInterval, 10000);
              break;
#endif /* ENABLE_LEARN_MODE */

            case BRL_CMD_SWITCHVT_PREV:
              if (!switchVirtualTerminal(scr.number-1))
                playTune(&tune_command_rejected);
              break;
            case BRL_CMD_SWITCHVT_NEXT:
              if (!switchVirtualTerminal(scr.number+1))
                playTune(&tune_command_rejected);
              break;

#ifdef ENABLE_SPEECH_SUPPORT
            case BRL_CMD_RESTARTSPEECH:
              restartSpeechDriver();
              break;
            case BRL_CMD_SPKHOME:
              if (scr.number == speechScreen) {
                trackSpeech(speech->getTrack());
              } else {
                playTune(&tune_command_rejected);
              }
              break;
            case BRL_CMD_AUTOSPEAK:
              TOGGLE_PLAY(prefs.autospeak);
              break;
            case BRL_CMD_MUTE:
              speech->mute();
              break;

            case BRL_CMD_SAY_LINE:
              sayLines(p->winy, 1, 0, prefs.sayLineMode);
              break;
            case BRL_CMD_SAY_ABOVE:
              sayLines(0, p->winy+1, 1, sayImmediate);
              break;
            case BRL_CMD_SAY_BELOW:
              sayLines(p->winy, scr.rows-p->winy, 1, sayImmediate);
              break;

            case BRL_CMD_SAY_SLOWER:
              if (speech->rate && (prefs.speechRate > 0)) {
                setSpeechRate(--prefs.speechRate, 1);
              } else {
                playTune(&tune_command_rejected);
              }
              break;
            case BRL_CMD_SAY_FASTER:
              if (speech->rate && (prefs.speechRate < SPK_MAXIMUM_RATE)) {
                setSpeechRate(++prefs.speechRate, 1);
              } else {
                playTune(&tune_command_rejected);
              }
              break;

            case BRL_CMD_SAY_SOFTER:
              if (speech->volume && (prefs.speechVolume > 0)) {
                setSpeechVolume(--prefs.speechVolume, 1);
              } else {
                playTune(&tune_command_rejected);
              }
              break;
            case BRL_CMD_SAY_LOUDER:
              if (speech->volume && (prefs.speechVolume < SPK_MAXIMUM_VOLUME)) {
                setSpeechVolume(++prefs.speechVolume, 1);
              } else {
                playTune(&tune_command_rejected);
              }
              break;
#endif /* ENABLE_SPEECH_SUPPORT */

            default: {
              int blk = command & BRL_MSK_BLK;
              int arg = command & BRL_MSK_ARG;
              int flags = command & BRL_MSK_FLG;

              switch (blk) {
                case BRL_BLK_PASSKEY: {
                  unsigned short key;
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
                  if (flags & BRL_FLG_CHAR_META) key |= SCR_KEY_MOD_META;
                  if (!insertKey(key)) {
                  badKey:
                    playTune(&tune_command_rejected);
                  }
                  break;
                }

                case BRL_BLK_PASSCHAR:
                  if (!insertCharacter(arg, flags)) playTune(&tune_command_rejected);
                  break;

                case BRL_BLK_PASSDOTS:
                  if (!insertCharacter(untextTable[arg], flags)) playTune(&tune_command_rejected);
                  break;

                case BRL_BLK_PASSAT:
                  if (flags & BRL_FLG_KBD_RELEASE) kbdAT_interpretScanCode(&command, 0XF0);
                  if (flags & BRL_FLG_KBD_EMUL0) kbdAT_interpretScanCode(&command, 0XE0);
                  if (flags & BRL_FLG_KBD_EMUL1) kbdAT_interpretScanCode(&command, 0XE1);
                  if (kbdAT_interpretScanCode(&command, arg)) goto doCommand;
                  break;

                case BRL_BLK_PASSXT:
                  if (flags & BRL_FLG_KBD_EMUL0) kbdXT_interpretScanCode(&command, 0XE0);
                  if (flags & BRL_FLG_KBD_EMUL1) kbdXT_interpretScanCode(&command, 0XE1);
                  if (kbdXT_interpretScanCode(&command, arg)) goto doCommand;
                  break;

                case BRL_BLK_PASSPS2:
                  /* not implemented yet */
                  playTune(&tune_command_rejected);
                  break;

                case BRL_BLK_ROUTE:
                  if (arg < brl.x) {
                    arg = getOffset(arg, 0);
                    if (routeCursor(MIN(p->winx+arg, scr.cols-1), p->winy, scr.number)) {
                      playTune(&tune_routing_started);
                      break;
                    }
                  }
                  playTune(&tune_command_rejected);
                  break;

                case BRL_BLK_CUTBEGIN:
                  if (arg < brl.x && p->winx+arg < scr.cols) {
                    arg = getOffset(arg, 0);
                    cutBegin(p->winx+arg, p->winy);
                  } else
                    playTune(&tune_command_rejected);
                  break;
                case BRL_BLK_CUTAPPEND:
                  if (arg < brl.x && p->winx+arg < scr.cols) {
                    arg = getOffset(arg, 0);
                    cutAppend(p->winx+arg, p->winy);
                  } else
                    playTune(&tune_command_rejected);
                  break;
                case BRL_BLK_CUTRECT:
                  if (arg < brl.x) {
                    arg = getOffset(arg, 1);
                    if (cutRectangle(MIN(p->winx+arg, scr.cols-1), p->winy))
                      break;
                  }
                  playTune(&tune_command_rejected);
                  break;
                case BRL_BLK_CUTLINE:
                  if (arg < brl.x) {
                    arg = getOffset(arg, 1);
                    if (cutLine(MIN(p->winx+arg, scr.cols-1), p->winy))
                      break;
                  }
                  playTune(&tune_command_rejected);
                  break;

                case BRL_BLK_DESCCHAR:
                  if (arg < brl.x && p->winx+arg < scr.cols) {
                    static char *colours[] = {
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
                    char buffer[0X40];
                    int size = sizeof(buffer);
                    int length = 0;
                    unsigned char character, attributes;

                    arg = getOffset(arg, 0);
                    readScreen(p->winx+arg, p->winy, 1, 1, &character, SCR_TEXT);
                    readScreen(p->winx+arg, p->winy, 1, 1, &attributes, SCR_ATTRIB);

                    {
                      int count;
                      snprintf(&buffer[length], size-length,
                               "char %d (0x%02x): %s on %s%n",
                               character, character,
                               gettext(colours[attributes & 0X0F]),
                               gettext(colours[(attributes & 0X70) >> 4]),
                               &count);
                      length += count;
                    }

                    if (attributes & 0X80) {
                      int count;
                      snprintf(&buffer[length], size-length,
                               " %s%n",
                               gettext("blink"),
                               &count);
                      length += count;
                    }

                    message(buffer, 0);
                  } else
                    playTune(&tune_command_rejected);
                  break;

                case BRL_BLK_SETLEFT:
                  if (arg < brl.x && p->winx+arg < scr.cols) {
                    arg = getOffset(arg, 0);
                    p->winx += arg;
                  } else
                    playTune(&tune_command_rejected);
                  break;
                case BRL_BLK_GOTOLINE:
                  if (flags & BRL_FLG_LINE_SCALED)
                    arg = rescaleInteger(arg, BRL_MSK_ARG, scr.rows-1);
                  if (arg < scr.rows) {
                    slideWindowVertically(arg);
                    if (flags & BRL_FLG_LINE_TOLEFT) p->winx = 0;
                  } else {
                    playTune(&tune_command_rejected);
                  }
                  break;

                case BRL_BLK_SETMARK: {
                  ScreenMark *mark = &p->marks[arg];
                  mark->column = p->winx;
                  mark->row = p->winy;
                  playTune(&tune_mark_set);
                  break;
                }
                case BRL_BLK_GOTOMARK: {
                  ScreenMark *mark = &p->marks[arg];
                  p->winx = mark->column;
                  p->winy = mark->row;
                  break;
                }

                case BRL_BLK_SWITCHVT:
                  if (!switchVirtualTerminal(arg+1))
                    playTune(&tune_command_rejected);
                  break;

                {
                  int increment;
                case BRL_BLK_PRINDENT:
                  increment = -1;
                  goto findIndent;
                case BRL_BLK_NXINDENT:
                  increment = 1;
                findIndent:
                  arg = getOffset(arg, 0);
                  findRow(MIN(p->winx+arg, scr.cols-1),
                          increment, testIndent, NULL);
                  break;
                }

                case BRL_BLK_PRDIFCHAR:
                  if (arg < brl.x && p->winx+arg < scr.cols)
                    upDifferentCharacter(SCR_TEXT, getOffset(arg, 0));
                  else
                    playTune(&tune_command_rejected);
                  break;
                case BRL_BLK_NXDIFCHAR:
                  if (arg < brl.x && p->winx+arg < scr.cols)
                    downDifferentCharacter(SCR_TEXT, getOffset(arg, 0));
                  else
                    playTune(&tune_command_rejected);
                  break;

                default:
                  playTune(&tune_command_rejected);
                  LogPrint(LOG_WARNING, "%s: %04X", gettext("unrecognized command"), command);
              }
              break;
            }
          }
        }

        if ((p->winx != oldmotx) || (p->winy != oldmoty)) {
          /* The window has been manually moved. */
          p->motx = p->winx;
          p->moty = p->winy;

#ifdef ENABLE_CONTRACTED_BRAILLE
          contracted = 0;
#endif /* ENABLE_CONTRACTED_BRAILLE */

#ifdef ENABLE_SPEECH_SUPPORT
          if (p->trackCursor && speechTracking && (scr.number == speechScreen)) {
            p->trackCursor = 0;
            playTune(&tune_cursor_unlinked);
          }
#endif /* ENABLE_SPEECH_SUPPORT */
        }

        if (!(command & BRL_MSK_BLK)) {
          if (command & BRL_FLG_ROUTE) {
            int left = p->winx;
            int right = left + brl.x - 1;

            int top = p->winy;
            int bottom = top + brl.y - 1;

            if ((scr.posx < left) || (scr.posx > right) ||
                (scr.posy < top) || (scr.posy > bottom)) {
              if (routeCursor(MIN(MAX(scr.posx, left), right),
                              MIN(MAX(scr.posy, top), bottom),
                              scr.number)) {
                playTune(&tune_routing_started);
                awaitRoutingStatus(ROUTE_WRONG_COLUMN);

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
      }

      /* some command (key insertion, virtual terminal switching, etc)
       * may have moved the cursor
       */
      updateScreenAttributes();

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
      speech->doTrack();

      if (speechTracking && !speech->isSpeaking()) speechTracking = 0;
#endif /* ENABLE_SPEECH_SUPPORT */

      if (p->trackCursor) {
#ifdef ENABLE_SPEECH_SUPPORT
        if (speechTracking && (scr.number == speechScreen)) {
          int index = speech->getTrack();
          if (index != speechIndex) trackSpeech(speechIndex = index);
        } else
#endif /* ENABLE_SPEECH_SUPPORT */
        {
          /* If cursor moves while blinking is on */
          if (prefs.blinkingCursor) {
            if (scr.posy != p->trky) {
              /* turn off cursor to see what's under it while changing lines */
              setBlinkingCursor(0);
            } else if (scr.posx != p->trkx) {
              /* turn on cursor to see it moving on the line */
              setBlinkingCursor(1);
            }
          }
          /* If the cursor moves in cursor tracking mode: */
          if (!routingProcess && (scr.posx != p->trkx || scr.posy != p->trky)) {
            trackCursor(0);
            p->trkx = scr.posx;
            p->trky = scr.posy;
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
        static unsigned char *oldText = NULL;

        int newScreen = scr.number;
        int newX = scr.posx;
        int newY = scr.posy;
        int newWidth = scr.cols;
        unsigned char newText[newWidth];

        readScreen(0, p->winy, newWidth, 1, newText, SCR_TEXT);

        if (!speechTracking) {
          int column = 0;
          int count = newWidth;
          const unsigned char *text = newText;

          if (oldText) {
            if ((newScreen == oldScreen) && (p->winy == oldwiny) && (newWidth == oldWidth)) {
              if (memcmp(newText, oldText, newWidth) != 0) {
                if ((newY == p->winy) && (newY == oldY)) {
                  if ((newX > oldX) &&
                      (memcmp(newText, oldText, oldX) == 0) &&
                      (memcmp(newText+newX, oldText+oldX, newWidth-newX) == 0)) {
                    column = oldX;
                    count = newX - oldX;
                    goto speak;
                  }

                  if ((newX < oldX) &&
                      (memcmp(newText, oldText, newX) == 0) &&
                      (memcmp(newText+newX, oldText+oldX, newWidth-oldX) == 0)) {
                    column = newX;
                    count = oldX - newX;
                    text = oldText;
                    goto speak;
                  }

                  if ((newX == oldX) &&
                      (memcmp(newText, oldText, newX) == 0)) {
                    int oldLength = oldWidth;
                    int newLength = newWidth;
                    int x;

                    while (oldLength > oldX) {
                      if (oldText[oldLength-1] != ' ') break;
                      --oldLength;
                    }
                    while (newLength > newX) {
                      if (newText[newLength-1] != ' ') break;
                      --newLength;
                    }

                    for (x=newX+1; 1; ++x) {
                      int done = 1;

                      if (x < newLength) {
                        if (memcmp(newText+x, oldText+oldX, newWidth-x) == 0) {
                          column = newX;
                          count = x - newX;
                          goto speak;
                        }

                        done = 0;
                      }

                      if (x < oldLength) {
                        if (memcmp(newText+newX, oldText+x, oldWidth-x) == 0) {
                          column = oldX;
                          count = x - oldX;
                          text = oldText;
                          goto speak;
                        }

                        done = 0;
                      }

                      if (done) break;
                    }
                  }

                  while (newText[column] == oldText[column]) ++column;
                  while (newText[count-1] == oldText[count-1]) --count;
                  count -= column;
                }
              } else if ((newY == p->winy) && ((newX != oldX) || (newY != oldY))) {
                column = newX;
                count = 1;
              } else {
                count = 0;
              }
            }
          }

        speak:
          if (count) {
            speech->mute();
            speech->say(text+column, count);
          }
        }

        oldText = reallocWrapper(oldText, newWidth);
        memcpy(oldText, newText, newWidth);
        oldWidth = newWidth;

        oldScreen = newScreen;
        oldX = newX;
        oldY = newY;
      }
#endif /* ENABLE_SPEECH_SUPPORT */

      /* There are a few things to take care of if the display has moved. */
      if ((p->winx != oldwinx) || (p->winy != oldwiny)) {
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

        oldwinx = p->winx;
        oldwiny = p->winy;
      }

      if (infoMode) {
        showInfo();
      } else {
        brl.cursor = -1;

#ifdef ENABLE_CONTRACTED_BRAILLE
        contracted = 0;
        if (isContracting()) {
          int windowLength = brl.x * brl.y;
          while (1) {
            int cursorOffset = brl.cursor;
            int inputLength = scr.cols - p->winx;
            int outputLength = windowLength;
            unsigned char inputBuffer[inputLength];
            unsigned char outputBuffer[outputLength];

            if ((scr.posy == p->winy) && (scr.posx >= p->winx)) cursorOffset = scr.posx - p->winx;
            readScreen(p->winx, p->winy, inputLength, 1, inputBuffer, SCR_TEXT);

            {
              int i;
              for (i=0; i<inputLength; ++i) contractedOffsets[i] = -1;
            }

            if (!contractText(contractionTable,
                              inputBuffer, &inputLength,
                              outputBuffer, &outputLength,
                              contractedOffsets, cursorOffset))
              break;

            {
              int inputEnd = inputLength;

              if (contractedTrack) {
                if (outputLength == windowLength) {
                  int inputIndex = inputEnd;
                  while (inputIndex) {
                    int offset = contractedOffsets[--inputIndex];
                    if (offset != -1) {
                      if (offset != outputLength) break;
                      inputEnd = inputIndex;
                    }
                  }
                }

                if (scr.posx >= (p->winx + inputEnd)) {
                  int offset = 0;
                  int onspace = 0;
                  int length = scr.cols - p->winx;
                  unsigned char buffer[length];
                  readScreen(p->winx, p->winy, length, 1, buffer, SCR_TEXT);

                  while (offset < length) {
                    if ((isspace(buffer[offset]) != 0) != onspace) {
                      if (onspace) break;
                      onspace = 1;
                    }
                    ++offset;
                  }

                  if ((offset += p->winx) > scr.posx) {
                    p->winx = (p->winx + scr.posx) / 2;
                  } else {
                    p->winx = offset;
                  }

                  continue;
                }
              }

              memcpy(brl.buffer, outputBuffer, outputLength);
              memset(brl.buffer+outputLength, 0, windowLength-outputLength);

              if (cursorOffset < inputEnd) {
                while (cursorOffset >= 0) {
                  int offset = contractedOffsets[cursorOffset];
                  if (offset >= 0) {
                    if (offset < brl.x) brl.cursor = offset;
                    break;
                  }
                  --cursorOffset;
                }
              }
            }

            contractedStart = p->winx;
            contractedLength = inputLength;
            contractedTrack = 0;
            contracted = 1;

            if (p->showAttributes || (prefs.showAttributes && (!prefs.blinkingAttributes || attributesState))) {
              int inputOffset;
              int outputOffset = 0;
              unsigned char attributes = 0;
              readScreen(contractedStart, p->winy, contractedLength, 1, inputBuffer, SCR_ATTRIB);
              for (inputOffset=0; inputOffset<contractedLength; ++inputOffset) {
                int offset = contractedOffsets[inputOffset];
                if (offset >= 0) {
                  while (outputOffset < offset) outputBuffer[outputOffset++] = attributes;
                  attributes = 0;
                }
                attributes |= inputBuffer[inputOffset];
              }
              while (outputOffset < outputLength) outputBuffer[outputOffset++] = attributes;
              if (p->showAttributes) {
                for (outputOffset=0; outputOffset<outputLength; ++outputOffset)
                  brl.buffer[outputOffset] = attributesTable[outputBuffer[outputOffset]];
              } else {
                overlayAttributes(outputBuffer, outputLength, 1);
              }
            }

            break;
          }
        }

        if (!contracted)
#endif /* ENABLE_CONTRACTED_BRAILLE */
        {
          int winlen = MIN(brl.x, scr.cols-p->winx);

          readScreen(p->winx, p->winy, winlen, brl.y, brl.buffer,
                     p->showAttributes? SCR_ATTRIB: SCR_TEXT);
          if (winlen < brl.x) {
            /* We got a rectangular piece of text with readScreen but the display
             * is in an off-right position with some cells at the end blank
             * so we'll insert these cells and blank them.
             */
            int i;
            for (i=brl.y-1; i>0; i--)
              memmove(brl.buffer+i*brl.x, brl.buffer+i*winlen, winlen);
            for (i=0; i<brl.y; i++)
              memset(brl.buffer+i*brl.x+winlen, ' ', brl.x-winlen);
          }

          /*
           * If the cursor is visible and in range, and help is off: 
           */
          if ((scr.posx >= p->winx) && (scr.posx < (p->winx + brl.x)) &&
              (scr.posy >= p->winy) && (scr.posy < (p->winy + brl.y)))
            brl.cursor = (scr.posy - p->winy) * brl.x + scr.posx - p->winx;

          if (braille->writeVisual) braille->writeVisual(&brl);

          /* blank out capital letters if they're blinking and should be off */
          if (prefs.blinkingCapitals && !capitalsState) {
            int i;
            for (i=0; i<brl.x*brl.y; i++)
              if (BRL_ISUPPER(brl.buffer[i]))
                brl.buffer[i] = ' ';
          }

          /* convert to dots using the current translation table */
          if ((translationTable == attributesTable) || !prefs.textStyle) {
            int i;
            for (
              i = 0;
              i < (brl.x * brl.y);
              brl.buffer[i] = translationTable[brl.buffer[i]], i++
            );
          } else {
            int i;
            for (
              i = 0;
              i < (brl.x * brl.y);
              brl.buffer[i] = translationTable[brl.buffer[i]] & (BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6), i++
            );
          }

          /* Attribute underlining: if viewing text (not attributes), attribute
             underlining is active and visible and we're not in help, then we
             get the attributes for the current region and OR the underline. */
          if (!p->showAttributes && prefs.showAttributes && (!prefs.blinkingAttributes || attributesState)) {
            unsigned char attrbuf[winlen*brl.y];
            readScreen(p->winx, p->winy, winlen, brl.y, attrbuf, SCR_ATTRIB);
            overlayAttributes(attrbuf, winlen, brl.y);
          }
        }

        if (brl.cursor >= 0) {
          if (showCursor() && (!prefs.blinkingCursor || cursorState)) {
            brl.buffer[brl.cursor] |= cursorDots();
          }
        }

        setStatusCells();
        braille->writeWindow(&brl);
      }
#ifdef ENABLE_API
      api_releaseDriver(&brl);
    } else if (apiStarted) {
      api_flush(&brl, BRL_CTX_SCREEN);
#endif /* ENABLE_API */
    }

  isOffline:

#ifdef ENABLE_SPEECH_SUPPORT
    processSpeechFifo();
#endif /* ENABLE_SPEECH_SUPPORT */

    drainBrailleOutput(&brl, updateInterval);
    updateIntervals++;

    /*
     * Update Braille display and screen information.  Switch screen 
     * state if screen number has changed.
     */
    updateScreenAttributes();
  }

  return 0;
}

void 
message (const char *text, short flags) {
#ifdef ENABLE_SPEECH_SUPPORT
  if (prefs.alertTunes && !(flags & MSG_SILENT)) sayString(text, 1);
#endif /* ENABLE_SPEECH_SUPPORT */

  if (braille && brl.buffer) {
    int length = strlen(text);

#ifdef ENABLE_API
  int api = apiStarted;
  apiStarted = 0;
  if (api) api_unlink(&brl);
#endif /* ENABLE_API */

    while (length) {
      int count;
      int index;

      /* strip leading spaces */
      while (*text == ' ')  text++, length--;

      if (length <= brl.x*brl.y) {
        count = length; /* the whole message fits in the braille window */
      } else {
        /* split the message across multiple windows on space characters */
        for (count=brl.x*brl.y-2; count>0 && text[count]!=' '; count--);
        if (!count) count = brl.x * brl.y - 1;
      }

      memset(brl.buffer, ' ', brl.x*brl.y);
      for (index=0; index<count; brl.buffer[index++]=*text++);
      if (length -= count) {
        while (index < brl.x*brl.y) brl.buffer[index++] = '-';
        brl.buffer[brl.x*brl.y - 1] = '>';
      }
      writeBrailleBuffer(&brl);

      if (flags & MSG_WAITKEY) {
        while (1) {
          int command = readCommand(BRL_CTX_MESSAGE);
          testProgramTermination();
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
          testProgramTermination();
          drainBrailleOutput(&brl, updateInterval);
          while ((command = readCommand(BRL_CTX_MESSAGE)) == BRL_CMD_NOOP);
          if (command != EOF) break;
        }
      }
    }

#ifdef ENABLE_API
  if (api) api_link(&brl);
  apiStarted = api;
#endif /* ENABLE_API */
  }
}

void
showDotPattern (unsigned char dots, unsigned char duration) {
  unsigned char status[BRL_MAX_STATUS_CELL_COUNT];        /* status cell buffer */
  memset(status, dots, sizeof(status));
  memset(brl.buffer, dots, brl.x*brl.y);
  braille->writeStatus(&brl, status);
  braille->writeWindow(&brl);
  drainBrailleOutput(&brl, duration);
}

#ifdef __MINGW32__
int isWindowsService;

static SERVICE_STATUS_HANDLE serviceStatusHandle;
static DWORD serviceState;
static int serviceReturnCode;

static BOOL
setServiceState (DWORD state, int exitCode, const char *name) {
  SERVICE_STATUS status = {
    .dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
    .dwCurrentState = state,
    .dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE,
    .dwWin32ExitCode = exitCode? ERROR_SERVICE_SPECIFIC_ERROR: NO_ERROR,
    .dwServiceSpecificExitCode = exitCode,
    .dwCheckPoint = 0,
    .dwWaitHint = 10000, /* milliseconds */
  };

  serviceState = state;
  if (SetServiceStatus(serviceStatusHandle, &status)) return 1;
  LogWindowsError(name);
  return 0;
}

static void WINAPI
serviceHandler (DWORD code) {
  switch (code) {
    case SERVICE_CONTROL_STOP:
      setServiceState(SERVICE_STOP_PENDING, 0, "SERVICE_STOP_PENDING");
      raise(SIGTERM);
      break;

    case SERVICE_CONTROL_PAUSE:
      setServiceState(SERVICE_PAUSE_PENDING, 0, "SERVICE_PAUSE_PENDING");
      /* TODO: suspend */
      break;

    case SERVICE_CONTROL_CONTINUE:
      setServiceState(SERVICE_CONTINUE_PENDING, 0, "SERVICE_CONTINUE_PENDING");
      /* TODO: resume */
      break;

    default:
      setServiceState(serviceState, 0, "SetServiceStatus");
      break;
  }
}

static void
exitService (void) {
  setServiceState(SERVICE_STOPPED, 0, "SERVICE_STOPPED");
}

static void WINAPI
serviceMain (DWORD argc, LPSTR *argv) {
  atexit(exitService);
  isWindowsService = 1;

  if ((serviceStatusHandle = RegisterServiceCtrlHandler("", &serviceHandler))) {
    if ((setServiceState(SERVICE_START_PENDING, 0, "SERVICE_START_PENDING"))) {
      if (!(serviceReturnCode = beginProgram(argc, argv))) {
        if ((setServiceState(SERVICE_RUNNING, 0, "SERVICE_RUNNING"))) {
          runProgram();
        }
      }
      setServiceState(SERVICE_STOPPED, 0, "SERVICE_STOPPED");
    }
  } else {
    LogWindowsError("RegisterServiceCtrlHandler");
  }
}
#endif /* __MINGW32__ */

int
main (int argc, char *argv[]) {
#ifdef __MINGW32__
  static SERVICE_TABLE_ENTRY serviceTable[] = {
    { .lpServiceName="", .lpServiceProc=serviceMain },
    {}
  };

  if (StartServiceCtrlDispatcher(serviceTable)) return serviceReturnCode;
  isWindowsService = 0;

  if (GetLastError() != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
    LogWindowsError("StartServiceCtrlDispatcher");
    return 20;
  }
#endif /* __MINGW32__ */

  {
    int returnCode = beginProgram(argc, argv);
    if (!returnCode) returnCode = runProgram();
    return returnCode;
  }
}
