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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "misc.h"
#include "sysmisc.h"
#include "message.h"
#include "brl.h"
#include "brl.auto.h"

#define BRLSYMBOL noBraille
#define BRLNAME "NoBraille"
#define BRLDRIVER no
#define BRLHELP "/dev/null"
#define PREFSTYLE ST_None
#include "brl_driver.h"
static void brl_identify (void) {
  LogPrint(LOG_NOTICE, "No braille support.");
}
static int brl_open (BrailleDisplay *brl, char **parameters, const char *device) { return 0; }
static void brl_close (BrailleDisplay *brl) { }
static int brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds) { return EOF; }
static void brl_writeWindow (BrailleDisplay *brl) { }
static void brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) { }

const BrailleDriver *braille = &noBraille;

/*
 * Output braille translation tables.
 * The files *.auto.h (the default tables) are generated at compile-time.
 */
TranslationTable textTable = {
  #include "text.auto.h"
};
TranslationTable untextTable;

TranslationTable attributesTable = {
  #include "attrib.auto.h"
};

void *contractionTable = NULL;

const BrailleDriver *
loadBrailleDriver (const char *driver, const char *driverDirectory) {
  return loadDriver(driver,
                    driverDirectory, "brl_driver",
                    "braille", 'b',
                    driverTable,
                    &noBraille, noBraille.identifier);
}

int
listBrailleDrivers (const char *directory) {
  int ok = 0;
  char *path = makePath(directory, "brltty-brl.lst");
  if (path) {
    int fd = open(path, O_RDONLY);
    if (fd != -1) {
      char buffer[0X40];
      int count;
      fprintf(stderr, "Available Braille Drivers:\n\n");
      fprintf(stderr, "XX  Description\n");
      fprintf(stderr, "--  -----------\n");
      while ((count = read(fd, buffer, sizeof(buffer))))
        fwrite(buffer, count, 1, stderr);
      ok = 1;
      close(fd);
    } else {
      LogPrint(LOG_ERR, "Cannot open braille driver list: %s: %s",
               path, strerror(errno));
    }
    free(path);
  }
  return ok;
}

void
initializeBrailleDisplay (BrailleDisplay *brl) {
   brl->x = brl->y = -1;
   brl->helpPage = 0;
   brl->buffer = NULL;
   brl->writeDelay = 0;
   brl->bufferResized = NULL;
   brl->dataDirectory = NULL;
}

unsigned int
drainBrailleOutput (BrailleDisplay *brl, unsigned int minimumDelay) {
  unsigned int duration = MAX(minimumDelay, brl->writeDelay);
  brl->writeDelay = 0;
  delay(duration);
  return duration;
}

void
writeBrailleBuffer (BrailleDisplay *brl) {
  int i;
  if (braille->writeVisual) braille->writeVisual(brl);
  for (i=0; i<brl->x*brl->y; ++i) brl->buffer[i] = textTable[brl->buffer[i]];
  braille->writeWindow(brl);
}

void
writeBrailleText (BrailleDisplay *brl, const char *text, int length) {
  int width = brl->x * brl->y;
  if (length > width) length = width;
  memcpy(brl->buffer, text, length);
  memset(&brl->buffer[length], ' ', width-length);
  writeBrailleBuffer(brl);
}

void
writeBrailleString (BrailleDisplay *brl, const char *string) {
  writeBrailleText(brl, string, strlen(string));
}

static void
brailleBufferResized (BrailleDisplay *brl) {
  memset(brl->buffer, 0, brl->x*brl->y);
  LogPrint(LOG_INFO, "Braille Display Dimensions: %d %s, %d %s",
           brl->y, (brl->y == 1)? "row": "rows",
           brl->x, (brl->x == 1)? "column": "columns");
  if (brl->bufferResized) brl->bufferResized(brl->y, brl->x);
}

static int
resizeBrailleBuffer (BrailleDisplay *brl) {
  if (brl->resizeRequired) {
    brl->resizeRequired = 0;
    if (brl->isCoreBuffer) {
      int size = brl->x * brl->y;
      unsigned char *buffer = realloc(brl->buffer, size);
      if (!buffer) {
        LogError("braille buffer allocation");
        return 0;
      }
      brl->buffer = buffer;
    }
    brailleBufferResized(brl);
  }
  return 1;
}

int
allocateBrailleBuffer (BrailleDisplay *brl) {
  if ((brl->isCoreBuffer = brl->resizeRequired = brl->buffer == NULL)) {
    if (!resizeBrailleBuffer(brl)) return 0;
  } else {
    brailleBufferResized(brl);
  }
  return 1;
}

int
readBrailleCommand (BrailleDisplay *brl, DriverCommandContext cmds) {
  int command = braille->readCommand(brl, cmds);
  resizeBrailleBuffer(brl);
  return command;
}

#ifdef ENABLE_LEARN_MODE
const CommandEntry commandTable[] = {
  #include "cmds.auto.h"
  {EOF, NULL, NULL}
};

void
learnMode (BrailleDisplay *brl, int poll, int timeout) {
  int timer = 0;
  message("command learn mode", MSG_NODELAY);
  while (1) {
    int key;
    timer += drainBrailleOutput(brl, poll);
    if ((key = readBrailleCommand(brl, CMDS_SCREEN)) != EOF) {
      int blk = key & VAL_BLK_MASK;
      int arg = key & VAL_ARG_MASK;
      int cmd = blk | arg;
      char buffer[0X100];
      const CommandEntry *candidate = NULL;
      const CommandEntry *last = NULL;
      const CommandEntry *command = commandTable;
      if (cmd == CMD_LEARN) return;
      while (command->name) {
        if ((command->code & VAL_BLK_MASK) == blk) {
          if (!last || (last->code < command->code)) last = command;
          if (!candidate || (candidate->code < cmd)) candidate = command;
        }
        ++command;
      }
      if (candidate)
        if (candidate->code != cmd)
          if ((blk == 0) || (candidate->code < last->code))
            candidate = NULL;
      if (!candidate) {
        snprintf(buffer, sizeof(buffer), "unknown: %06X", key);
      } else if ((candidate == last) && (blk != 0)) {
        unsigned int number;
        switch (blk) {
          default:
            number = cmd - candidate->code + 1;
            break;
          case VAL_PASSCHAR:
            number = cmd - candidate->code;
            break;
          case VAL_PASSDOTS: {
            unsigned char dots[] = {B1, B2, B3, B4, B5, B6, B7, B8};
            int dot;
            number = 0;
            for (dot=0; dot<sizeof(dots); ++dot) {
              if (arg & dots[dot]) {
                number = (number * 10) + (dot + 1);
              }
            }
            break;
          }
        }
        snprintf(buffer, sizeof(buffer), "%s[%d]: %s",
                 candidate->name, number, candidate->description);
      } else {
        int offset;
        snprintf(buffer, sizeof(buffer), "%s: %n%s",
                 candidate->name, &offset, candidate->description);
        if ((blk == 0) && (key & VAL_SWITCHMASK)) {
          char *description = buffer + offset;
          const char *oldVerb = "toggle";
          int oldLength = strlen(oldVerb);
          if (strncmp(description, oldVerb, oldLength) == 0) {
            const char *newVerb = "set";
            int newLength = strlen(newVerb);
            memmove(description+newLength, description+oldLength, strlen(description+oldLength)+1);
            memcpy(description, newVerb, newLength);
            if (key & VAL_SWITCHON) {
              char *end = strrchr(description, '/');
              if (end) *end = 0;
            } else {
              char *target = strrchr(description, ' ');
              if (target) {
                const char *source = strchr(++target, '/');
                if (source) {
                  memmove(target, source+1, strlen(source));
                }
              }
            }
          }
        }
      }
      message(buffer, MSG_NODELAY|MSG_SILENT);
      timer = 0;
    } else if (timer > timeout) {
      break;
    }
  }
  message("done", 0);
}
#endif /* ENABLE_LEARN_MODE */

/* Reverse a 256x256 mapping, used for charset maps. */
void
reverseTranslationTable (TranslationTable *from, TranslationTable *to) {
  int byte;
  memset(to, 0, sizeof(*to));
  for (byte=sizeof(*to)-1; byte>=0; byte--) (*to)[(*from)[byte]] = byte;
}

void
makeOutputTable (const DotsTable *dots, TranslationTable *table) {
  static const DotsTable internalDots = {B1, B2, B3, B4, B5, B6, B7, B8};
  int byte, dot;
  memset(table, 0, sizeof(*table));
  for (byte=0; byte<0X100; byte++)
    for (dot=0; dot<sizeof(*dots); dot++)
      if (byte & internalDots[dot])
        (*table)[byte] |= (*dots)[dot];
}

/* Functions which support horizontal status cells, e.g. Papenmeier. */
/* this stuff is real wired - dont try to understand */

/* Dots for landscape (counterclockwise-rotated) digits. */
const unsigned char landscapeDigits[11] = {
  B1+B5+B2,    B4,          B4+B1,       B4+B5,       B4+B5+B2,
  B4+B2,       B4+B1+B5,    B4+B1+B5+B2, B4+B1+B2,    B1+B5,
  B1+B2+B4+B5
};

/* Format landscape representation of numbers 0 through 99. */
int
landscapeNumber (int x) {
  return landscapeDigits[(x / 10) % 10] | (landscapeDigits[x % 10] << 4);  
}

/* Format landscape flag state indicator. */
int
landscapeFlag (int number, int on) {
  int dots = landscapeDigits[number % 10];
  if (on) dots |= landscapeDigits[10] << 4;
  return dots;
}

/* Dots for seascape (clockwise-rotated) digits. */
const unsigned char seascapeDigits[11] = {
  B5+B1+B4,    B2,          B2+B5,       B2+B1,       B2+B1+B4,
  B2+B4,       B2+B5+B1,    B2+B5+B1+B4, B2+B5+B4,    B5+B1,
  B1+B2+B4+B5
};

/* Format seascape representation of numbers 0 through 99. */
int
seascapeNumber (int x) {
  return (seascapeDigits[(x / 10) % 10] << 4) | seascapeDigits[x % 10];  
}

/* Format seascape flag state indicator. */
int
seascapeFlag (int number, int on) {
  int dots = seascapeDigits[number % 10] << 4;
  if (on) dots |= seascapeDigits[10];
  return dots;
}

/* Dots for portrait digits - 2 numbers in one cells */
const unsigned char portraitDigits[11] = {
  B2+B4+B5, B1, B1+B2, B1+B4, B1+B4+B5, B1+B5, B1+B2+B4, 
  B1+B2+B4+B5, B1+B2+B5, B2+B4, B1+B2+B4+B5
};

/* Format portrait representation of numbers 0 through 99. */
int
portraitNumber (int x) {
  return portraitDigits[(x / 10) % 10] | (portraitDigits[x % 10]<<4);  
}

/* Format portrait flag state indicator. */
int
portraitFlag (int number, int on) {
  int dots = portraitDigits[number % 10] << 4;
  if (on) dots |= portraitDigits[10];
  return dots;
}
