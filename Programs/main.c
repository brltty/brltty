/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "misc.h"
#include "message.h"
#include "tunes.h"
#include "contract.h"
#include "route.h"
#include "cut.h"
#include "scr.h"
#include "brl.h"
#ifdef ENABLE_SPEECH_SUPPORT
#include "spk.h"
#endif /* ENABLE_SPEECH_SUPPORT */
#include "brltty.h"
#include "defaults.h"

int refreshInterval = DEFAULT_REFRESH_INTERVAL;
int messageDelay = DEFAULT_MESSAGE_DELAY;

/*
 * Misc param variables
 */
Preferences prefs;                /* environment (i.e. global) parameters */
BrailleDisplay brl;                        /* For the Braille routines */
short fwinshift;                /* Full window horizontal distance */
short hwinshift;                /* Half window horizontal distance */
short vwinshift;                /* Window vertical distance */
ScreenDescription scr;                        /* For screen state infos */
static short dispmd = LIVE_SCRN;        /* freeze screen on/off */
static short infmode = 0;                /* display screen image or info */

static int contracted = 0;
#ifdef ENABLE_CONTRACTED_BRAILLE
static int contractedLength;
static int contractedStart;
static int contractedOffsets[0X100];
static int contractedTrack = 0;
#endif /* ENABLE_CONTRACTED_BRAILLE */

static unsigned char statusCells[StatusCellCount];        /* status cell buffer */
static unsigned int TickCount = 0;        /* incremented each main loop cycle */


/*
 * useful macros
 */

#define BRL_ISUPPER(c) \
  (isupper((c)) || (c)=='@' || (c)=='[' || (c)=='^' || (c)==']' || (c)=='\\')

#define TOGGLEPLAY(var) playTune((var)? &tune_toggle_on: &tune_toggle_off)
#define TOGGLE(var) \
  (var = (keypress & VAL_SWITCHON)? 1: ((keypress & VAL_SWITCHOFF)? 0: (!var)))


unsigned char *curtbl = textTable;        /* active translation table */

static void
setTranslationTable (int attributes) {
  curtbl = attributes? attributesTable: textTable;
}


/* 
 * Structure definition for volatile screen state variables.
 */
typedef struct {
  short column;
  short row;
} ScreenMark;
typedef struct {
  int trackCursor;		/* tracking mode */
  int hideCursor;		/* For temporarily hiding cursor */
  int showAttributes;		/* text or attributes display */
  int winx, winy;	/* upper-left corner of braille window */
  int motx, moty;	/* user motion of braille window */
  int trkx, trky;	/* tracked cursor position */
  int ptrx, ptry;	/* mouse pointer position */
  ScreenMark marks[0X100];
} ScreenState;
static ScreenState initialScreenState = {
  DEFAULT_TRACK_CURSOR, DEFAULT_HIDE_CURSOR, 0,
  0, 0, /* winx/y */
  0, 0, /* motx/y */
  0, 0, /* trkx/y */
  0, 0, /* ptrx/y */
  {[0X00 ... 0XFF]={0, 0}}
};

/* 
 * Array definition containing pointers to ScreenState structures for 
 * each screen.  Each structure is dynamically allocated when its 
 * screen is used for the first time.
 * Screen 0 is reserved for the help screen; its structure is static.
 */
#define MAX_SCR 0X3F              /* actual number of separate screens */
static ScreenState scr0;        /* at least one is statically allocated */
static ScreenState *scrparam[MAX_SCR+1] = {&scr0, };
static ScreenState *p = &scr0;        /* pointer to current state structure */
static int curscr;                        /* current screen number */

static void
switchto (unsigned int scrno) {
  curscr = scrno;
  if (scrno > MAX_SCR)
    scrno = 0;
  if (!scrparam[scrno]) {         /* if not already allocated... */
    {
      if (!(scrparam[scrno] = malloc(sizeof(*p))))
        scrno = 0;         /* unable to allocate a new structure */
      else
        *scrparam[scrno] = initialScreenState;
    }
  }
  p = scrparam[scrno];
  setTranslationTable(p->showAttributes);
}

void
clearStatusCells (void) {
   memset(statusCells, 0, sizeof(statusCells));
   braille->writeStatus(&brl, statusCells);
}

static void
setStatusCells (void) {
   memset(statusCells, 0, sizeof(statusCells));
   switch (prefs.statusStyle) {
      case ST_AlvaStyle:
         if ((dispmd & HELP_SCRN) == HELP_SCRN) {
            statusCells[0] = textTable['h'];
            statusCells[1] = textTable['l'];
            statusCells[2] = textTable['p'];
         } else {
            /* The coords are given with letters as the DOS tsr */
            statusCells[0] = ((TickCount / 16) % (scr.posy / 25 + 1))? 0:
                             textTable[scr.posy % 25 + 'a'] |
                             ((scr.posx / brl.x) << 6);
            statusCells[1] = ((TickCount / 16) % (p->winy / 25 + 1))? 0:
                             textTable[p->winy % 25 + 'a'] |
                             ((p->winx / brl.x) << 6);
            statusCells[2] = textTable[(p->showAttributes)? 'a':
                                        ((dispmd & FROZ_SCRN) == FROZ_SCRN)? 'f':
                                        p->trackCursor? 't':
                                        ' '];
         }
         break;
      case ST_TiemanStyle:
         statusCells[0] = (portraitDigits[(p->winx / 10) % 10] << 4) |
                          portraitDigits[(scr.posx / 10) % 10];
         statusCells[1] = (portraitDigits[p->winx % 10] << 4) |
                          portraitDigits[scr.posx % 10];
         statusCells[2] = (portraitDigits[(p->winy / 10) % 10] << 4) |
                          portraitDigits[(scr.posy / 10) % 10];
         statusCells[3] = (portraitDigits[p->winy % 10] << 4) |
                          portraitDigits[scr.posy % 10];
         statusCells[4] = ((dispmd & FROZ_SCRN) == FROZ_SCRN? 1: 0) |
                          (prefs.showCursor << 1) |
                          (p->showAttributes << 2) |
                          (prefs.cursorStyle << 3) |
                          (prefs.alertTunes << 4) |
                          (prefs.blinkingCursor << 5) |
                          (p->trackCursor << 6) |
                          (prefs.slidingWindow << 7);
         break;
      case ST_PB80Style:
         statusCells[0] = (portraitDigits[(p->winy+1) % 10] << 4) |
                          portraitDigits[((p->winy+1) / 10) % 10];
         break;
      case ST_Generic:
         statusCells[FirstStatusCell] = FSC_GENERIC;
         statusCells[STAT_BRLCOL] = p->winx+1;
         statusCells[STAT_BRLROW] = p->winy+1;
         statusCells[STAT_CSRCOL] = scr.posx+1;
         statusCells[STAT_CSRROW] = scr.posy+1;
         statusCells[STAT_SCRNUM] = scr.no;
         statusCells[STAT_FREEZE] = (dispmd & FROZ_SCRN) == FROZ_SCRN;
         statusCells[STAT_DISPMD] = p->showAttributes;
         statusCells[STAT_SIXDOTS] = prefs.textStyle;
         statusCells[STAT_SLIDEWIN] = prefs.slidingWindow;
         statusCells[STAT_SKPIDLNS] = prefs.skipIdenticalLines;
         statusCells[STAT_SKPBLNKWINS] = prefs.skipBlankWindows;
         statusCells[STAT_CSRVIS] = prefs.showCursor;
         statusCells[STAT_CSRHIDE] = p->hideCursor;
         statusCells[STAT_CSRTRK] = p->trackCursor;
         statusCells[STAT_CSRSIZE] = prefs.cursorStyle;
         statusCells[STAT_CSRBLINK] = prefs.blinkingCursor;
         statusCells[STAT_ATTRVIS] = prefs.showAttributes;
         statusCells[STAT_ATTRBLINK] = prefs.blinkingAttributes;
         statusCells[STAT_CAPBLINK] = prefs.blinkingCapitals;
         statusCells[STAT_TUNES] = prefs.alertTunes;
         statusCells[STAT_HELP] = (dispmd & HELP_SCRN) != 0;
         statusCells[STAT_INFO] = infmode;
         break;
      case ST_MDVStyle:
         statusCells[0] = portraitDigits[((p->winy+1) / 10) % 10] |
                          (portraitDigits[((p->winx+1) / 10) % 10] << 4);
         statusCells[1] = portraitDigits[(p->winy+1) % 10] |
                          (portraitDigits[(p->winx+1) % 10] << 4);
         break;
      case ST_VoyagerStyle: /* 3cells (+1 blank) */
         statusCells[0] = (portraitDigits[p->winy % 10] << 4) |
                          portraitDigits[(p->winy / 10) % 10];
         statusCells[1] = (portraitDigits[scr.posy % 10] << 4) |
                          portraitDigits[(scr.posy / 10) % 10];
         if ((dispmd & FROZ_SCRN) == FROZ_SCRN)
            statusCells[2] = textTable['F'];
         else
            statusCells[2] = (portraitDigits[scr.posx % 10] << 4) |
                             portraitDigits[(scr.posx / 10) % 10];
         break;
      default:
         break;
   }
   braille->writeStatus(&brl, statusCells);
}

void
setStatusText (const char *text) {
   int i;
   memset(statusCells, 0, sizeof(statusCells));
   for (i=0; i<sizeof(statusCells); ++i) {
      unsigned char character = text[i];
      if (!character) break;
      statusCells[i] = textTable[character];
   }
   braille->writeStatus(&brl, statusCells);
}

static void
showInfo (void) {
  /* Here we must be careful. Some displays (e.g. Braille Lite 18)
   * are very small, and others (e.g. Bookworm) are even smaller.
   */
  unsigned char status[22];
  setStatusText("info");
  if (brl.x*brl.y >= 21) {
    sprintf(status, "%02d:%02d %02d:%02d %02d %c%c%c%c%c%c",
            p->winx, p->winy, scr.posx, scr.posy, curscr, 
            p->trackCursor? 't': ' ',
            prefs.showCursor? (prefs.blinkingCursor? 'B': 'v'):
                              (prefs.blinkingCursor? 'b': ' '),
            p->showAttributes? 'a': 't',
            ((dispmd & FROZ_SCRN) == FROZ_SCRN)? 'f': ' ',
            prefs.textStyle? '6': '8',
            prefs.blinkingCapitals? 'B': ' ');
    writeBrailleString(&brl, status);
  } else {
    int i;
    sprintf(status, "xxxxx %02d %c%c%c%c%c%c     ",
            curscr,
            p->trackCursor? 't': ' ',
            prefs.showCursor? (prefs.blinkingCursor? 'B' : 'v'):
                              (prefs.blinkingCursor? 'b': ' '),
            p->showAttributes? 'a': 't',
            ((dispmd & FROZ_SCRN) == FROZ_SCRN) ?'f': ' ',
            prefs.textStyle? '6': '8',
            prefs.blinkingCapitals? 'B': ' ');
    if (braille->writeVisual) {
      memcpy(brl.buffer, status, brl.x*brl.y);
      braille->writeVisual(&brl);
    }

    status[0] = portraitDigits[(p->winx / 10) % 10] << 4 |
                portraitDigits[(scr.posx / 10) % 10];
    status[1] = portraitDigits[p->winx % 10] << 4 | portraitDigits[scr.posx % 10];
    status[2] = portraitDigits[(p->winy / 10) % 10] << 4 |
                portraitDigits[(scr.posy / 10) % 10];
    status[3] = portraitDigits[p->winy % 10] << 4 | portraitDigits[scr.posy % 10];
    status[4] = (((dispmd & FROZ_SCRN) == FROZ_SCRN)? B1: 0) |
                (p->showAttributes?    B2: 0) |
                (prefs.alertTunes?     B3: 0) |
                (prefs.showCursor?     B4: 0) |
                (prefs.cursorStyle?    B5: 0) |
                (prefs.blinkingCursor? B6: 0) |
                (p->trackCursor?       B7: 0) |
                (prefs.slidingWindow?  B8: 0);

    /* We have to do the Braille translation ourselves, since
     * we don't want the first five characters translated ...
     */
    for (i=5; status[i]; i++) status[i] = textTable[status[i]];
    memcpy(brl.buffer, status, brl.x*brl.y);
    braille->writeWindow(&brl);
  }
}

static void
handleSignal (int number, void (*handler) (int))
{
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  sigemptyset(&action.sa_mask);
  action.sa_handler = handler;
  if (sigaction(number, &action, NULL) == -1) {
    LogError("signal set");
  }
}

static void
exitLog (void) {
  /* Reopen syslog (in case -e closed it) so that there will
   * be a "stopped" message to match the "starting" message.
   */
  LogOpen(0);
  LogPrint(LOG_INFO, "Terminated.");
  LogClose();
}

static void
exitScreenParameters (void) {
  int i;
  /* don't forget that scrparam[0] is staticaly allocated */
  for (i = 1; i <= MAX_SCR; i++) 
    free(scrparam[i]);
}

static void
terminateProgram (int quickly) {
  int flags = MSG_NODELAY;
#ifdef ENABLE_SPEECH_SUPPORT
  int silently = quickly;
  if (speech == &noSpeech) silently = 1;
  if (silently) flags |= MSG_SILENT;
#endif /* ENABLE_SPEECH_SUPPORT */
  clearStatusCells();
  message("BRLTTY exiting.", flags);
#ifdef ENABLE_SPEECH_SUPPORT
  if (!silently) {
    int awaitSilence = speech->isSpeaking();
    int i;
    for (i=0; i<messageDelay; i+=refreshInterval) {
      delay(refreshInterval);
      if (readBrailleCommand(&brl, CMDS_MESSAGE) != EOF) break;
      if (awaitSilence) {
        speech->doTrack();
        if (!speech->isSpeaking()) break;
      }
    }
  }
#endif /* ENABLE_SPEECH_SUPPORT */
  exit(0);
}

static void 
terminationHandler (int signalNumber) {
  terminateProgram(signalNumber == SIGINT);
}

static void 
childDeathHandler (int signalNumber) {
  pid_t pid = wait(NULL);
  if (csr_active) {
    if (pid == csr_pid) {
      csr_pid = 0;
      csr_active = 0;
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
      p->winx = MAX(MIN(scr.posx+reset+1, scr.cols)-brl.x, 0);
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
sayLines (int line, int count, int track, SayMode mode) {
  /* OK heres a crazy idea: why not send the attributes with the
   * text, in case some inflection or marking can be added...! The
   * speech driver's say function will receive a buffer of text
   * and a length, but in reality the buffer will contain twice
   * len bytes: the text followed by the video attribs data.
   */
  int length = scr.cols * count;
  unsigned char buffer[length * 2];

  if (mode == sayImmediate) speech->mute();
  readScreen(0, line, scr.cols, count, buffer, SCR_TEXT);
  if (speech->express) {
    readScreen(0, line, scr.cols, count, buffer+length, SCR_ATTRIB);
    speech->express(buffer, length);
  } else
    speech->say(buffer, length);

  speechTracking = track;
  speechScreen = scr.no;
  speechLine = line;
}
#endif /* ENABLE_SPEECH_SUPPORT */

static int
upDifferentLine (short mode) {
   if (p->winy > 0) {
      char buffer1[scr.cols], buffer2[scr.cols];
      int skipped = 0;
      if (mode==SCR_TEXT && p->showAttributes)
         mode = SCR_ATTRIB;
      readScreen(0, p->winy, scr.cols, 1, buffer1, mode);
      do {
         readScreen(0, --p->winy, scr.cols, 1, buffer2, mode);
         if (memcmp(buffer1, buffer2, scr.cols) ||
             (mode==SCR_TEXT && prefs.showCursor && p->winy==scr.posy))
            return 1;

         /* lines are identical */
         if (skipped == 0)
            playTune(&tune_skip_first);
         else if (skipped <= 4)
            playTune(&tune_skip);
         else if (skipped % 4 == 0)
            playTune(&tune_skip_more);
         skipped++;
      } while (p->winy > 0);
   }

   playTune(&tune_bounce);
   return 0;
}

static int
downDifferentLine (short mode) {
   if (p->winy < (scr.rows - brl.y)) {
      char buffer1[scr.cols], buffer2[scr.cols];
      int skipped = 0;
      if (mode==SCR_TEXT && p->showAttributes)
         mode = SCR_ATTRIB;
      readScreen(0, p->winy, scr.cols, 1, buffer1, mode);
      do {
         readScreen(0, ++p->winy, scr.cols, 1, buffer2, mode);
         if (memcmp(buffer1, buffer2, scr.cols) ||
             (mode==SCR_TEXT && prefs.showCursor && p->winy==scr.posy))
            return 1;

         /* lines are identical */
         if (skipped == 0)
            playTune(&tune_skip_first);
         else if (skipped <= 4)
            playTune(&tune_skip);
         else if (skipped % 4 == 0)
            playTune(&tune_skip_more);
         skipped++;
      } while (p->winy < (scr.rows - brl.y));
   }

   playTune(&tune_bounce);
   return 0;
}

static void
upOneLine (short mode) {
   if (p->winy > 0)
      p->winy--;
   else
      playTune(&tune_bounce);
}

static void
downOneLine (short mode) {
   if (p->winy < (scr.rows - brl.y))
      p->winy++;
   else
      playTune(&tune_bounce);
}

static void
upLine (short mode) {
   if (prefs.skipIdenticalLines)
      upDifferentLine(mode);
   else
      upOneLine(mode);
}

static void
downLine (short mode) {
   if (prefs.skipIdenticalLines)
      downDifferentLine(mode);
   else
      downOneLine(mode);
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
          brl.buffer[row*brl.x + column] |= (B7 | B8);
          break;
        case 0x0F: /* white on black */
        default:
          brl.buffer[row*brl.x + column] |= (B8);
          break;
      }
    }
  }
}


static int
insertCharacter (unsigned char character, int flags) {
  if (islower(character)) {
    if (flags & (VPC_SHIFT | VPC_UPPER)) character = toupper(character);
  } else if (flags & VPC_SHIFT) {
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
  if (flags & VPC_CONTROL) {
    if ((character & 0X6F) == 0X2F)
      character |= 0X50;
    else
      character &= 0X9F;
  }
  if (flags & VPC_META) {
    if (prefs.metaMode)
      character |= 0X80;
    else
      if (!insertKey(KEY_ESCAPE)) return 0;
  }
  return insertKey(character);
}

typedef int (*RowTester) (int column, int row, void *data);
static void
findRow (int column, int increment, RowTester test, void *data) {
  int row = p->winy + increment;
  while ((row >= 0) && (row <= scr.rows-brl.y)) {
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
  char buffer[count];
  readScreen(0, row, count, 1, buffer, SCR_TEXT);
  while (column >= 0) {
    if ((buffer[column] != ' ') && (buffer[column] != 0)) return 1;
    --column;
  }
  return 0;
}

static int
testPrompt (int column, int row, void *data) {
  const char *prompt = data;
  int count = column+1;
  char buffer[count];
  readScreen(0, row, count, 1, buffer, SCR_TEXT);
  return memcmp(buffer, prompt, count) == 0;
}

static int
getRightShift (void) {
#ifdef ENABLE_CONTRACTED_BRAILLE
  if (contracted) return contractedLength;
#endif /* ENABLE_CONTRACTED_BRAILLE */
  return fwinshift;
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

int
main (int argc, char *argv[]) {
  int keypress;                        /* character received from braille display */
  int i;                        /* loop counter */
  short csron = 1;                /* display cursor on (toggled during blink) */
  short csrcntr = 1;
  short capon = 0;                /* display caps off (toggled during blink) */
  short capcntr = 1;
  short attron = 0;
  short attrcntr = 1;
  short oldwinx, oldwiny;

#ifdef INIT_PATH
  if (getpid() == 1) {
    fprintf(stderr, "BRLTTY started as INIT.\n");
    fflush(stderr);
    switch (fork()) {
      case -1: /* failed */
        fprintf(stderr, "Fork for BRLTTY failed: %s\n", strerror(errno));
        fflush(stderr);
      default: /* parent */
        fprintf(stderr, "Executing the real INIT: %s\n", INIT_PATH);
        fflush(stderr);
        execv(INIT_PATH, argv);
        /* execv() shouldn't return */
        fprintf(stderr, "Execution of the real INIT failed: %s\n", strerror(errno));
        fflush(stderr);
        exit(1);
      case 0: { /* child */
        static char *arguments[] = {"brltty", "-E", "-n", "-e", "-linfo", NULL};
        argv = arguments;
        argc = (sizeof(arguments) / sizeof(arguments[0])) - 1;
        break;
      }
    }
  }
#endif /* INIT_PATH */

  /* Open the system log. */
  LogOpen(0);
  LogPrint(LOG_INFO, "Starting.");
  atexit(exitLog);

  suppressTuneDeviceOpenErrors();

  /* Initialize global data assumed to be ready by the termination handler. */
  *p = initialScreenState;
  scrparam[0] = p;
  for (i = 1; i <= MAX_SCR; i++)
    scrparam[i] = NULL;
  curscr = 0;
  atexit(exitScreenParameters);
  
  initializeBraille();
#ifdef ENABLE_SPEECH_SUPPORT
  initializeSpeech();
#endif /* ENABLE_SPEECH_SUPPORT */

  /* We install SIGPIPE handler before startup() so that drivers which
   * use pipes can't cause program termination (the call to message() in
   * startup() in particular).
   */
  handleSignal(SIGPIPE, SIG_IGN);

  /* Install the program termination handler. */
  handleSignal(SIGTERM, terminationHandler);
  handleSignal(SIGINT, terminationHandler);

  /* Setup everything required on startup */
  startup(argc, argv);

  /* Install the handler which monitors the death of child processes. */
  handleSignal(SIGCHLD, childDeathHandler);

  describeScreen(&scr);
  /* NB: screen size can sometimes change, f.e. the video mode may be changed
   * when installing a new font. Will be detected by another call to
   * describeScreen in the main loop. Don't assume that scr.rows
   * and scr.cols are constants across loop iterations.
   */
  switchto(scr.no);                        /* allocate current screen params */
  p->trkx = scr.posx; p->trky = scr.posy;
  trackCursor(1);        /* set initial window position */
  p->motx = p->winx; p->moty = p->winy;
  oldwinx = p->winx; oldwiny = p->winy;
  if (prefs.pointerFollowsWindow) setPointer(p->winx, p->winy);
  getPointer(&p->ptrx, &p->ptry);

  /*
   * Main program loop 
   */
  while (1) {
    int pointerMoved = 0;

    /* The braille display can stick out by brl.x-offr columns from the
     * right edge of the screen.
     */
    short offr = scr.cols % brl.x;
    if (!offr) offr = brl.x;

    closeTuneDevice(0);
    TickCount++;
    /*
     * Process any Braille input 
     */
    while ((keypress = readBrailleCommand(&brl,
                                          infmode? CMDS_STATUS:
                                          ((dispmd & HELP_SCRN) == HELP_SCRN)? CMDS_HELP:
                                          CMDS_SCREEN)) != EOF) {
      int oldmotx = p->winx;
      int oldmoty = p->winy;
      LogPrint(LOG_DEBUG, "Command: %5.5X", keypress);
      if (!executeScreenCommand(keypress)) {
        switch (keypress & ~VAL_SWITCHMASK) {
          case CMD_NOOP:        /* do nothing but loop */
            if (keypress & VAL_SWITCHON)
              playTune(&tune_toggle_on);
            else if (keypress & VAL_SWITCHOFF)
              playTune(&tune_toggle_off);
            else
              continue;
            break;
          case CMD_RESTARTBRL:
#ifdef ENABLE_API
            api_unlink();
#endif /* ENABLE_API */
            stopBrailleDriver();
            playTune(&tune_braille_off);
            LogPrint(LOG_INFO, "Reinitializing braille driver.");
            startBrailleDriver();
#ifdef ENABLE_API
            api_link();
#endif /* ENABLE_API */
            break;
          case CMD_TOP:
            p->winy = 0;
            break;
          case CMD_TOP_LEFT:
            p->winy = 0;
            p->winx = 0;
            break;
          case CMD_BOT:
            p->winy = scr.rows - brl.y;
            break;
          case CMD_BOT_LEFT:
            p->winy = scr.rows - brl.y;
            p->winx = 0;
            break;
          case CMD_WINUP:
            if (p->winy == 0)
              playTune (&tune_bounce);
            p->winy = MAX (p->winy - vwinshift, 0);
            break;
          case CMD_WINDN:
            if (p->winy == scr.rows - brl.y)
              playTune (&tune_bounce);
            p->winy = MIN (p->winy + vwinshift, scr.rows - brl.y);
            break;
          case CMD_FWINLT:
            if (!(prefs.skipBlankWindows && (prefs.blankWindowsSkipMode == sbwAll))) {
              int oldX = p->winx;
              if (p->winx > 0) {
                p->winx = MAX(p->winx-fwinshift, 0);
                if (prefs.skipBlankWindows) {
                  if (prefs.blankWindowsSkipMode == sbwEndOfLine)
                    goto skipEndOfLine;
                  if (!prefs.showCursor ||
                      (scr.posy != p->winy) ||
                      (scr.posx >= (p->winx + brl.x))) {
                    int charCount = MIN(scr.cols, p->winx+brl.x);
                    int charIndex;
                    char buffer[charCount];
                    readScreen(0, p->winy, charCount, 1, buffer, SCR_TEXT);
                    for (charIndex=0; charIndex<charCount; ++charIndex)
                      if ((buffer[charIndex] != ' ') && (buffer[charIndex] != 0))
                        break;
                    if (charIndex == charCount)
                      goto wrapUp;
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
              p->winx = MAX((scr.cols-offr)/fwinshift*fwinshift, 0);
              upLine(SCR_TEXT);
            skipEndOfLine:
              if (prefs.skipBlankWindows && (prefs.blankWindowsSkipMode == sbwEndOfLine)) {
                int charIndex;
                char buffer[scr.cols];
                readScreen(0, p->winy, scr.cols, 1, buffer, SCR_TEXT);
                for (charIndex=scr.cols-1; charIndex>=0; --charIndex)
                  if ((buffer[charIndex] != ' ') && (buffer[charIndex] != 0))
                    break;
                if (prefs.showCursor && (scr.posy == p->winy))
                  charIndex = MAX(charIndex, scr.posx);
                charIndex = MAX(charIndex, 0);
                if (charIndex < p->winx)
                  p->winx = charIndex / fwinshift * fwinshift;
              }
              break;
            }
          case CMD_FWINLTSKIP: {
            int oldX = p->winx;
            int oldY = p->winy;
            int tuneLimit = 3;
            int charCount;
            int charIndex;
            char buffer[scr.cols];
            while (1) {
              if (p->winx > 0) {
                p->winx = MAX(p->winx-fwinshift, 0);
              } else {
                if (p->winy == 0) {
                  playTune(&tune_bounce);
                  p->winx = oldX;
                  p->winy = oldY;
                  break;
                }
                if (tuneLimit-- > 0)
                  playTune(&tune_wrap_up);
                p->winx = MAX((scr.cols-offr)/fwinshift*fwinshift, 0);
                upLine(SCR_TEXT);
              }
              charCount = MIN(brl.x, scr.cols-p->winx);
              readScreen(p->winx, p->winy, charCount, 1, buffer, SCR_TEXT);
              for (charIndex=(charCount-1); charIndex>=0; charIndex--)
                if ((buffer[charIndex] != ' ') && (buffer[charIndex] != 0))
                  break;
              if (prefs.showCursor &&
                  (scr.posy == p->winy) &&
                  (scr.posx < (p->winx + charCount)))
                charIndex = MAX(charIndex, scr.posx-p->winx);
              if (charIndex >= 0) {
                if (prefs.slidingWindow)
                  p->winx = MAX(p->winx+charIndex-brl.x+1, 0);
                break;
              }
            }
            break;
          }
          case CMD_LNUP:
            upLine(SCR_TEXT);
            break;
          case CMD_PRDIFLN:
            if (prefs.skipIdenticalLines)
              upOneLine(SCR_TEXT);
            else
              upDifferentLine(SCR_TEXT);
            break;
          case CMD_ATTRUP:
            upDifferentLine(SCR_ATTRIB);
            break;
          case CMD_FWINRT:
            if (!(prefs.skipBlankWindows && (prefs.blankWindowsSkipMode == sbwAll))) {
              int oldX = p->winx;
              int rwinshift = getRightShift();
              if (p->winx < (scr.cols - rwinshift)) {
                p->winx += rwinshift;
                if (prefs.skipBlankWindows) {
                  if (!prefs.showCursor ||
                      (scr.posy != p->winy) ||
                      (scr.posx < p->winx)) {
                    int charCount = scr.cols - p->winx;
                    int charIndex;
                    char buffer[charCount];
                    readScreen(p->winx, p->winy, charCount, 1, buffer, SCR_TEXT);
                    for (charIndex=0; charIndex<charCount; ++charIndex)
                      if ((buffer[charIndex] != ' ') && (buffer[charIndex] != 0))
                        break;
                    if (charIndex == charCount)
                      goto wrapDown;
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
              p->winx = 0;
              downLine(SCR_TEXT);
              break;
            }
          case CMD_FWINRTSKIP: {
            int oldX = p->winx;
            int oldY = p->winy;
            int tuneLimit = 3;
            int charCount;
            int charIndex;
            char buffer[scr.cols];
            while (1) {
              int rwinshift = getRightShift();
              if (p->winx < (scr.cols - rwinshift)) {
                p->winx += rwinshift;
              } else {
                if (p->winy >= (scr.rows - brl.y)) {
                  playTune(&tune_bounce);
                  p->winx = oldX;
                  p->winy = oldY;
                  break;
                }
                if (tuneLimit-- > 0)
                  playTune(&tune_wrap_down);
                p->winx = 0;
                downLine(SCR_TEXT);
              }
              charCount = MIN(brl.x, scr.cols-p->winx);
              readScreen(p->winx, p->winy, charCount, 1, buffer, SCR_TEXT);
              for (charIndex=0; charIndex<charCount; charIndex++)
                if ((buffer[charIndex] != ' ') && (buffer[charIndex] != 0))
                  break;
              if (prefs.showCursor &&
                  (scr.posy == p->winy) &&
                  (scr.posx >= p->winx))
                charIndex = MIN(charIndex, scr.posx-p->winx);
              if (charIndex < charCount) {
                if (prefs.slidingWindow)
                  p->winx = MIN(p->winx+charIndex, scr.cols-offr);
                break;
              }
            }
            break;
          }
          case CMD_LNDN:
            downLine(SCR_TEXT);
            break;
          case CMD_NXDIFLN:
            if (prefs.skipIdenticalLines)
              downOneLine(SCR_TEXT);
            else
              downDifferentLine(SCR_TEXT);
            break;
          case CMD_ATTRDN:
            downDifferentLine(SCR_ATTRIB);
            break;
          {
            int increment;
          case CMD_PRPGRPH:
            increment = -1;
            goto findParagraph;
          case CMD_NXPGRPH:
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
          case CMD_PRPROMPT:
            increment = -1;
            goto findPrompt;
          case CMD_NXPROMPT:
            increment = 1;
          findPrompt:
            {
              unsigned char buffer[scr.cols];
              unsigned char *blank;
              readScreen(0, p->winy, scr.cols, 1, buffer, SCR_TEXT);
              if ((blank = memchr(buffer, ' ', scr.cols))) {
                findRow(blank-buffer, increment, testPrompt, buffer);
              } else {
                playTune(&tune_bad_command);
              }
            }
            break;
          }
          {
            int increment;
          case CMD_PRSEARCH:
            increment = -1;
            goto doSearch;
          case CMD_NXSEARCH:
            increment = 1;
          doSearch:
            if (cut_buffer) {
              int length = strlen(cut_buffer);
              int found = 0;
              if (length <= scr.cols) {
                int line = p->winy;
                unsigned char buffer[scr.cols+1];
                unsigned char string[length+1];
                for (i=0; i<length; i++) string[i] = tolower(cut_buffer[i]);
                string[length] = 0;
                while ((line >= 0) && (line <= (scr.rows - brl.y))) {
                  unsigned char *address = buffer;
                  readScreen(0, line, scr.cols, 1, buffer, SCR_TEXT);
                  for (i=0; i<scr.cols; i++) buffer[i] = tolower(buffer[i]);
                  buffer[scr.cols] = 0;
                  if (line == p->winy) {
                    if (increment < 0) {
                      int end = p->winx + length - 1;
                      if (end < scr.cols) buffer[end] = 0;
                    } else {
                      int start = p->winx + brl.x;
                      if (start > scr.cols) start = scr.cols;
                      address = buffer + start;
                    }
                  }
                  if ((address = strstr(address, string))) {
                    if (increment < 0) {
                      while (1) {
                        unsigned char *next = strstr(address+1, string);
                        if (!next) break;
                        address = next;
                      }
                    }
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
              playTune(&tune_bad_command);
            }
            break;
          }
          case CMD_HOME:
            trackCursor(1);
            break;
          case CMD_BACK:
            p->winx = p->motx;
            p->winy = p->moty;
            break;
          case CMD_LNBEG:
            if (p->winx)
              p->winx = 0;
            else
              playTune(&tune_bounce);
            break;
          case CMD_LNEND:
            if (p->winx == (scr.cols - brl.x))
              playTune(&tune_bounce);
            else
              p->winx = scr.cols - brl.x;
            break;
          case CMD_CHRLT:
            if (p->winx == 0)
              playTune (&tune_bounce);
            p->winx = MAX (p->winx - 1, 0);
            break;
          case CMD_CHRRT:
            if (p->winx < (scr.cols - 1))
              p->winx++;
            else
              playTune(&tune_bounce);
            break;
          case CMD_HWINLT:
            if (p->winx == 0)
              playTune(&tune_bounce);
            else
              p->winx = MAX(p->winx-hwinshift, 0);
            break;
          case CMD_HWINRT:
            if (p->winx < (scr.cols - hwinshift))
              p->winx += hwinshift;
            else
              playTune(&tune_bounce);
            break;
          case CMD_CSRJMP_VERT:
            if (!routeCursor(-1, p->winy, curscr))
              playTune(&tune_bad_command);
            break;
          case CMD_CSRVIS:
            /* toggles the preferences option that decides whether cursor
               is shown at all */
            TOGGLEPLAY ( TOGGLE(prefs.showCursor) );
            break;
          case CMD_CSRHIDE:
            /* This is for briefly hiding the cursor */
            TOGGLE(p->hideCursor);
            /* no tune */
            break;
          case CMD_ATTRVIS:
            TOGGLEPLAY ( TOGGLE(prefs.showAttributes) );
            break;
          case CMD_CSRBLINK:
            csron = 1;
            TOGGLEPLAY ( TOGGLE(prefs.blinkingCursor) );
            if (prefs.blinkingCursor) {
              csrcntr = prefs.cursorVisiblePeriod;
              attron = 1;
              attrcntr = prefs.attributesVisiblePeriod;
              capon = 0;
              capcntr = prefs.capitalsInvisiblePeriod;
            }
            break;
          case CMD_CSRTRK:
            TOGGLE(p->trackCursor);
            if (p->trackCursor) {
#ifdef ENABLE_SPEECH_SUPPORT
              if (speech->isSpeaking()) {
                speechIndex = -1;
              } else
#endif /* ENABLE_SPEECH_SUPPORT */
                trackCursor(1);
              playTune(&tune_link);
            } else
              playTune(&tune_unlink);
            break;
          case CMD_PASTE:
            if ((dispmd & HELP_SCRN) != HELP_SCRN && !csr_active)
              if (cut_paste())
                break;
            playTune(&tune_bad_command);
            break;
          case CMD_TUNES:
            TOGGLEPLAY ( TOGGLE(prefs.alertTunes) );        /* toggle sound on/off */
            break;
          case CMD_DISPMD:
            setTranslationTable(TOGGLE(p->showAttributes));
            break;
          case CMD_FREEZE:
            { unsigned char v = (dispmd & FROZ_SCRN) ? 1 : 0;
              TOGGLE(v);
              if (v) {
                dispmd = selectDisplay(dispmd | FROZ_SCRN);
                playTune(&tune_freeze);
              }else{
                dispmd = selectDisplay(dispmd & ~FROZ_SCRN);
                playTune(&tune_unfreeze);
              }
            }
            break;
          case CMD_HELP:
            { int v;
              infmode = 0;        /* ... and not in info mode */
              v = (dispmd & HELP_SCRN)? 1: 0;
              TOGGLE(v);
              if (v) {
                dispmd = selectDisplay(dispmd | HELP_SCRN);
                if (dispmd & HELP_SCRN) /* help screen selection successful */
                  {
                    switchto(0);        /* screen 0 for help screen */
                  }
                else        /* help screen selection failed */
                    message("help not available", 0);
              }else
                dispmd = selectDisplay(dispmd & ~HELP_SCRN);
            }
            break;
          case CMD_CAPBLINK:
            capon = 1;
            TOGGLEPLAY( TOGGLE(prefs.blinkingCapitals) );
            if (prefs.blinkingCapitals) {
              capcntr = prefs.capitalsVisiblePeriod;
             attron = 0;
             attrcntr = prefs.attributesInvisiblePeriod;
             csron = 0;
             csrcntr = prefs.cursorInvisiblePeriod;
            }
            break;
          case CMD_ATTRBLINK:
            attron = 1;
            TOGGLEPLAY( TOGGLE(prefs.blinkingAttributes) );
            if (prefs.blinkingAttributes) {
              attrcntr = prefs.attributesVisiblePeriod;
              capon = 1;
              capcntr = prefs.capitalsVisiblePeriod;
              csron = 0;
              csrcntr = prefs.cursorInvisiblePeriod;
            }
            break;
          case CMD_INFO:
            TOGGLE(infmode);
            break;
          case CMD_CSRSIZE:
            TOGGLEPLAY ( TOGGLE(prefs.cursorStyle) );
            break;
          case CMD_SIXDOTS:
            TOGGLEPLAY ( TOGGLE(prefs.textStyle) );
            break;
          case CMD_SLIDEWIN:
            TOGGLEPLAY ( TOGGLE(prefs.slidingWindow) );
            break;
          case CMD_SKPIDLNS:
            TOGGLEPLAY ( TOGGLE(prefs.skipIdenticalLines) );
            break;
          case CMD_SKPBLNKWINS:
            TOGGLEPLAY ( TOGGLE(prefs.skipBlankWindows) );
            break;
          case CMD_PREFSAVE:
            if (savePreferences()) {
              playTune(&tune_done);
            }
            break;
#ifdef ENABLE_PREFERENCES_MENU
          case CMD_PREFMENU:
            updatePreferences();
            break;
#endif /* ENABLE_PREFERENCES_MENU */
          case CMD_PREFLOAD:
            if (loadPreferences(1)) {
              csron = 1;
              capon = 0;
              csrcntr = capcntr = 1;
              playTune(&tune_done);
            }
            break;
#ifdef ENABLE_SPEECH_SUPPORT
          case CMD_SAY_LINE:
            sayLines(p->winy, 1, 0, prefs.sayLineMode);
            break;
          case CMD_SAY_ABOVE:
            sayLines(0, p->winy+1, 1, sayImmediate);
            break;
          case CMD_SAY_BELOW:
            sayLines(p->winy, scr.rows-p->winy, 1, sayImmediate);
            break;
          case CMD_MUTE:
            speech->mute();
            break;
          case CMD_SPKHOME:
            if (scr.no == speechScreen) {
              trackSpeech(speech->getTrack());
            } else {
              playTune(&tune_bad_command);
            }
            break;
          case CMD_RESTARTSPEECH:
            stopSpeechDriver();
            LogPrint(LOG_INFO, "Reinitializing speech driver.");
            startSpeechDriver();
            break;
#endif /* ENABLE_SPEECH_SUPPORT */
          case CMD_SWITCHVT_PREV:
            if (!switchVirtualTerminal(scr.no-1))
              playTune(&tune_bad_command);
            break;
          case CMD_SWITCHVT_NEXT:
            if (!switchVirtualTerminal(scr.no+1))
              playTune(&tune_bad_command);
            break;
#ifdef ENABLE_LEARN_MODE
          case CMD_LEARN:
            learnMode(&brl, refreshInterval, 10000);
            break;
#endif /* ENABLE_LEARN_MODE */
          default: {
            int key = keypress & VAL_BLK_MASK;
            int arg = keypress & VAL_ARG_MASK;
            int flags = keypress & VAL_FLG_MASK;
            switch (key) {
              case VAL_PASSKEY: {
                unsigned short key;
                switch (arg) {
                  case VPK_RETURN:
                    key = KEY_RETURN;
                    break;
                  case VPK_TAB:
                    key = KEY_TAB;
                    break;
                  case VPK_BACKSPACE:
                    key = KEY_BACKSPACE;
                    break;
                  case VPK_ESCAPE:
                    key = KEY_ESCAPE;
                    break;
                  case VPK_CURSOR_LEFT:
                    key = KEY_CURSOR_LEFT;
                    break;
                  case VPK_CURSOR_RIGHT:
                    key = KEY_CURSOR_RIGHT;
                    break;
                  case VPK_CURSOR_UP:
                    key = KEY_CURSOR_UP;
                    break;
                  case VPK_CURSOR_DOWN:
                    key = KEY_CURSOR_DOWN;
                    break;
                  case VPK_PAGE_UP:
                    key = KEY_PAGE_UP;
                    break;
                  case VPK_PAGE_DOWN:
                    key = KEY_PAGE_DOWN;
                    break;
                  case VPK_HOME:
                    key = KEY_HOME;
                    break;
                  case VPK_END:
                    key = KEY_END;
                    break;
                  case VPK_INSERT:
                    key = KEY_INSERT;
                    break;
                  case VPK_DELETE:
                    key = KEY_DELETE;
                    break;
                  default:
                    if (arg < VPK_FUNCTION) goto badKey;
                    key = KEY_FUNCTION + (arg - VPK_FUNCTION);
                    break;
                }
                if (!insertKey(key))
                badKey:
                  playTune(&tune_bad_command);
                break;
              }
              case VAL_PASSCHAR:
                if (!insertCharacter(arg, flags)) {
                  playTune(&tune_bad_command);
                }
                break;
              case VAL_PASSDOTS:
                if (!insertCharacter(untextTable[arg], flags)) {
                  playTune(&tune_bad_command);
                }
                break;
              case CR_ROUTE:
                if (arg < brl.x) {
                  arg = getOffset(arg, 0);
                  if (routeCursor(MIN(p->winx+arg, scr.cols-1), p->winy, curscr))
                    break;
                }
                playTune(&tune_bad_command);
                break;
              case CR_CUTBEGIN:
                if (arg < brl.x && p->winx+arg < scr.cols) {
                  arg = getOffset(arg, 0);
                  cut_begin(p->winx+arg, p->winy);
                } else
                  playTune(&tune_bad_command);
                break;
              case CR_CUTAPPEND:
                if (arg < brl.x && p->winx+arg < scr.cols) {
                  arg = getOffset(arg, 0);
                  cut_append(p->winx+arg, p->winy);
                } else
                  playTune(&tune_bad_command);
                break;
              case CR_CUTRECT:
                if (arg < brl.x) {
                  arg = getOffset(arg, 1);
                  if (cut_rectangle(MIN(p->winx+arg, scr.cols-1), p->winy))
                    break;
                }
                playTune(&tune_bad_command);
                break;
              case CR_CUTLINE:
                if (arg < brl.x) {
                  arg = getOffset(arg, 1);
                  if (cut_line(MIN(p->winx+arg, scr.cols-1), p->winy))
                    break;
                }
                playTune(&tune_bad_command);
                break;
              case CR_DESCCHAR:
                if (arg < brl.x && p->winx+arg < scr.cols) {
                  static char *colours[] = {
                    "black",     "blue",          "green",       "cyan",
                    "red",       "magenta",       "brown",       "light grey",
                    "dark grey", "light blue",    "light green", "light cyan",
                    "light red", "light magenta", "yellow",      "white"
                  };
                  char buffer[0X40];
                  unsigned char character, attributes;
                  arg = getOffset(arg, 0);
                  readScreen(p->winx+arg, p->winy, 1, 1, &character, SCR_TEXT);
                  readScreen(p->winx+arg, p->winy, 1, 1, &attributes, SCR_ATTRIB);
                  sprintf(buffer, "char %d (0x%02x): %s on %s",
                          character, character,
                          colours[attributes & 0X0F],
                          colours[(attributes & 0X70) >> 4]);
                  if (attributes & 0X80) strcat(buffer, " blink");
                  message(buffer, 0);
                } else
                  playTune(&tune_bad_command);
                break;
              case CR_SETLEFT:
                if (arg < brl.x && p->winx+arg < scr.cols) {
                  arg = getOffset(arg, 0);
                  p->winx += arg;
                } else
                  playTune(&tune_bad_command);
                break;
              case CR_SETMARK: {
                ScreenMark *mark = &p->marks[arg];
                mark->column = p->winx;
                mark->row = p->winy;
                playTune(&tune_mark_set);
                break;
              }
              case CR_GOTOMARK: {
                ScreenMark *mark = &p->marks[arg];
                p->winx = mark->column;
                p->winy = mark->row;
                break;
              }
              case CR_SWITCHVT:
                  if (!switchVirtualTerminal(arg+1))
                       playTune(&tune_bad_command);
                  break;
              {
                int increment;
              case CR_PRINDENT:
                increment = -1;
                goto findIndent;
              case CR_NXINDENT:
                increment = 1;
              findIndent:
                arg = getOffset(arg, 0);
                findRow(MIN(p->winx+arg, scr.cols-1),
                        increment, testIndent, NULL);
                break;
              }
              default:
                playTune(&tune_bad_command);
                LogPrint(LOG_WARNING, "Driver sent unrecognized command: %X", keypress);
            }
            break;
          }
        }
      }

      if ((p->winx != oldmotx) || (p->winy != oldmoty)) {
        /* The window has been manually moved. */
        p->motx = p->winx;
        p->moty = p->winy;
        contracted = 0;
      }
    }
        
    /*
     * Update blink counters: 
     */
    if (prefs.blinkingCursor)
      if (!--csrcntr)
        csrcntr = (csron ^= 1)? prefs.cursorVisiblePeriod: prefs.cursorInvisiblePeriod;
    if (prefs.blinkingCapitals)
      if (!--capcntr)
        capcntr = (capon ^= 1)? prefs.capitalsVisiblePeriod: prefs.capitalsInvisiblePeriod;
    if (prefs.blinkingAttributes)
      if (!--attrcntr)
        attrcntr = (attron ^= 1)? prefs.attributesVisiblePeriod: prefs.attributesInvisiblePeriod;

    /*
     * Update Braille display and screen information.  Switch screen 
     * params if screen number has changed.
     */
    describeScreen(&scr);
    if (!(dispmd & (HELP_SCRN|FROZ_SCRN)) && curscr != scr.no)
      switchto(scr.no);
    /* NB: This should also accomplish screen resizing: scr.rows and
     * scr.cols may have changed.
     */
    {
      int maximum = scr.rows - brl.y;
      int *table[] = {&p->winy, &p->moty, NULL};
      int **value = table;
      while (*value) {
        if (**value > maximum) **value = maximum;
        ++value;
      }
    }
    {
      int maximum = scr.cols - 1;
      int *table[] = {&p->winx, &p->motx, NULL};
      int **value = table;
      while (*value) {
        if (**value > maximum) **value = maximum;
        ++value;
      }
    }

#ifdef ENABLE_SPEECH_SUPPORT
    /* speech tracking */
    speech->doTrack(); /* called continually even if we're not tracking
                             so that the pipe doesn't fill up. */
    if (prefs.autospeak) {
      if (p->winy != oldwiny) {
        sayLines(p->winy, 1, 0, sayImmediate);
      }
    }
#endif /* ENABLE_SPEECH_SUPPORT */

    if (p->trackCursor) {
#ifdef ENABLE_SPEECH_SUPPORT
      if (speech->isSpeaking() && (scr.no == speechScreen) && speechTracking) {
        int index = speech->getTrack();
        if (index != speechIndex) {
          trackSpeech(speechIndex = index);
        }
      } else
#endif /* ENABLE_SPEECH_SUPPORT */
      {
        /* If cursor moves while blinking is on */
        if (prefs.blinkingCursor) {
          if (scr.posy != p->trky) {
            /* turn off cursor to see what's under it while changing lines */
            csron = 0;
            csrcntr = prefs.cursorInvisiblePeriod;
          } else if (scr.posx != p->trkx) {
            /* turn on cursor to see it moving on the line */
            csron = 1;
            csrcntr = prefs.cursorVisiblePeriod;
          }
        }
        /* If the cursor moves in cursor tracking mode: */
        if (!csr_active && (scr.posx != p->trkx || scr.posy != p->trky)) {
          trackCursor(0);
          p->trkx = scr.posx;
          p->trky = scr.posy;
        } else if (prefs.windowFollowsPointer) {
          int x, y;
          if (getPointer(&x, &y)) {
            if ((x != p->ptrx)) {
              p->ptrx = x;
              if (x < p->winx)
                p->winx = x;
              else if (x >= (p->winx + brl.x))
                p->winx = x + 1 - brl.x;
              pointerMoved = 1;
            }

            if ((y != p->ptry)) {
              p->ptry = y;
              if (y < p->winy)
                p->winy = y;
              else if (y >= (p->winy + brl.y))
                p->winy = y + 1 - brl.y;
              pointerMoved = 1;
            }
          }
        }
      }
    }

    /* There are a few things to take care of if the display has moved. */
    if (p->winx != oldwinx || p->winy != oldwiny) {
      if (prefs.pointerFollowsWindow && !pointerMoved) setPointer(p->winx, p->winy);

      if (prefs.showAttributes && prefs.blinkingAttributes) {
        /* Attributes are blinking.
           We could check to see if we changed screen, but that doesn't
           really matter... this is mainly for when you are hunting up/down
           for the line with attributes. */
        attron = 1;
        attrcntr = prefs.attributesVisiblePeriod;
        /* problem: this still doesn't help when the braille window is
           stationnary and the attributes themselves are moving
           (example: tin). */
      }
    }

    oldwinx = p->winx; oldwiny = p->winy;
    if (infmode) {
      showInfo();
    } else {
      int cursorLocation = -1;
      contracted = 0;
#ifdef ENABLE_CONTRACTED_BRAILLE
      if (prefs.textStyle && contractionTable) {
        int windowLength = brl.x * brl.y;
        while (1) {
          int cursorOffset = cursorLocation;
          int inputLength = scr.cols - p->winx;
          int outputLength = windowLength;
          unsigned char inputBuffer[inputLength];
          unsigned char outputBuffer[outputLength];

          if ((scr.posy == p->winy) && (scr.posx >= p->winx)) cursorOffset = scr.posx - p->winx;
          readScreen(p->winx, p->winy, inputLength, 1, inputBuffer, SCR_TEXT);
          for (i=0; i<inputLength; ++i) contractedOffsets[i] = -1;
          if (!contractText(contractionTable,
                            inputBuffer, &inputLength,
                            outputBuffer, &outputLength,
                            contractedOffsets, cursorOffset))
            break;

          if (contractedTrack) {
            int inputEnd = inputLength;
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
              if ((offset += p->winx) > scr.posx)
                p->winx = (p->winx + scr.posx) / 2;
              else
                p->winx = offset;
              continue;
            }
          }

          memcpy(brl.buffer, outputBuffer, outputLength);
          memset(brl.buffer+outputLength, 0, windowLength-outputLength);
          while (cursorOffset >= 0) {
            int offset = contractedOffsets[cursorOffset];
            if (offset >= 0) {
              cursorLocation = offset;
              break;
            }
            --cursorOffset;
          }
          contractedStart = p->winx;
          contractedLength = inputLength;
          contractedTrack = 0;
          contracted = 1;

          if (p->showAttributes || (prefs.showAttributes && (!prefs.blinkingAttributes || attron))) {
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
#endif /* ENABLE_CONTRACTED_BRAILLE */
      if (!contracted) {
        int winlen = MIN(brl.x, scr.cols-p->winx);
        readScreen(p->winx, p->winy, winlen, brl.y, brl.buffer,
                   p->showAttributes? SCR_ATTRIB: SCR_TEXT);
        if (braille->writeVisual) braille->writeVisual(&brl);

        /* blank out capital letters if they're blinking and should be off */
        if (prefs.blinkingCapitals && !capon)
          for (i=0; i<winlen*brl.y; i++)
            if (BRL_ISUPPER(brl.buffer[i]))
              brl.buffer[i] = ' ';

        /* convert to dots using the current translation table */
        if ((curtbl == attributesTable) || !prefs.textStyle) {
          for (
            i = 0;
            i < (winlen * brl.y);
            brl.buffer[i] = curtbl[brl.buffer[i]], i++
          );
        } else {
          for (
            i = 0;
            i < (winlen * brl.y);
            brl.buffer[i] = curtbl[brl.buffer[i]] & (B1 | B2 | B3 | B4 | B5 | B6), i++
          );
        }

        if (winlen < brl.x) {
          /* We got a rectangular piece of text with readScreen but the display
             is in an off-right position with some cells at the end blank
             so we'll insert these cells and blank them. */
          for (i=brl.y-1; i>0; i--)
            memmove(brl.buffer+i*brl.x, brl.buffer+i*winlen, winlen);
          for (i=0; i<brl.y; i++)
            memset(brl.buffer+i*brl.x+winlen, 0, brl.x-winlen);
        }

        /* Attribute underlining: if viewing text (not attributes), attribute
           underlining is active and visible and we're not in help, then we
           get the attributes for the current region and OR the underline. */
        if (!p->showAttributes && prefs.showAttributes && (!prefs.blinkingAttributes || attron)) {
          unsigned char attrbuf[winlen*brl.y];
          readScreen(p->winx, p->winy, winlen, brl.y, attrbuf, SCR_ATTRIB);
          overlayAttributes(attrbuf, winlen, brl.y);
        }

        /*
         * If the cursor is visible and in range, and help is off: 
         */
        if ((scr.posx >= p->winx) && (scr.posx < (p->winx + brl.x)) &&
            (scr.posy >= p->winy) && (scr.posy < (p->winy + brl.y)))
          cursorLocation = (scr.posy - p->winy) * brl.x + scr.posx - p->winx;
      }
      if (cursorLocation >= 0) {
        if (prefs.showCursor && !p->hideCursor && (!prefs.blinkingCursor || csron)) {
          brl.buffer[cursorLocation] |= prefs.cursorStyle?
                                        (B1 | B2 | B3 | B4 | B5 | B6 | B7 | B8):
                                        (B7 | B8);
        }
      }

      setStatusCells();
      braille->writeWindow(&brl);
    }
    drainBrailleOutput(&brl, refreshInterval);
  }

  terminateProgram(0);
  return 0;
}

void 
message (const char *text, short flags) {
   int length = strlen(text);

#ifdef ENABLE_SPEECH_SUPPORT
   if (prefs.alertTunes && !(flags & MSG_SILENT)) {
      speech->mute();
      speech->say(text, length);
   }
#endif /* ENABLE_SPEECH_SUPPORT */

   if (braille && brl.buffer) {
      while (length) {
         int count;
         int index;

         /* strip leading spaces */
         while (*text == ' ')  text++, length--;

         if (length <= brl.x*brl.y) {
            count = length; /* the whole message fits on the braille window */
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

         /*
          * Do Braille translation using text table. * Six-dot mode is
          * ignored, since case can be important, and * blinking caps won't 
          * work ... 
          */
         writeBrailleBuffer(&brl);

         if (flags & MSG_WAITKEY)
            getBrailleCommand(CMDS_MESSAGE);
         else if (length || !(flags & MSG_NODELAY)) {
            int i;
            for (i=0; i<messageDelay; i+=refreshInterval) {
               delay(refreshInterval);
               if (readBrailleCommand(&brl, CMDS_MESSAGE) != EOF) break;
            }
         }
      }
   }
}

void
showDotPattern (unsigned char dots, unsigned char duration) {
  memset(statusCells, dots, sizeof(statusCells));
  memset(brl.buffer, dots, brl.x*brl.y);
  braille->writeStatus(&brl, statusCells);
  braille->writeWindow(&brl);
  drainBrailleOutput(&brl, duration);
}
