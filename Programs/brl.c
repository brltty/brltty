/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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
#include "cmd.h"

#define BRLSYMBOL noBraille
#define BRLNAME NoBraille
#define BRLCODE no
#define BRLHELP "/dev/null"
#include "brl_driver.h"
static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "No braille support.");
}
static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  brl->x = 80;
  brl->y = 1;
  return 1;
}
static void
brl_close (BrailleDisplay *brl) {
}
static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return EOF;
}
static void
brl_writeWindow (BrailleDisplay *brl) {
}
static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
}

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

void
showBrailleString (BrailleDisplay *brl, const char *string, unsigned int duration) {
  writeBrailleString(brl, string);
  drainBrailleOutput(brl, duration);
}

void
clearStatusCells (BrailleDisplay *brl) {
  unsigned char status[StatusCellCount];        /* status cell buffer */
  memset(status, 0, sizeof(status));
  braille->writeStatus(brl, status);
}

void
setStatusText (BrailleDisplay *brl, const char *text) {
  unsigned char status[StatusCellCount];        /* status cell buffer */
  int i;
  memset(status, 0, sizeof(status));
  for (i=0; i<sizeof(status); ++i) {
    unsigned char character = text[i];
    if (!character) break;
    status[i] = textTable[character];
  }
  braille->writeStatus(brl, status);
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
readBrailleCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  int command = braille->readCommand(brl, context);
  resizeBrailleBuffer(brl);
  return command;
}

#ifdef ENABLE_LEARN_MODE
void
learnMode (BrailleDisplay *brl, int poll, int timeout) {
  setStatusText(brl, "lrn");
  message("command learn mode", MSG_NODELAY);

  timeout_yet(0);
  do {
    int command = readBrailleCommand(brl, BRL_CTX_SCREEN);
    if (command != EOF) {
      LogPrint(LOG_DEBUG, "Learn: command=%06X", command);
      if (IS_DELAYED_COMMAND(command)) continue;

      {
        int cmd = command & VAL_CMD_MASK;
        if (cmd == BRL_CMD_NOOP) continue;
        if (cmd == BRL_CMD_LEARN) return;
      }

      {
        char buffer[0X100];
        describeCommand(command, buffer, sizeof(buffer));
        LogPrint(LOG_DEBUG, "Learn: %s", buffer);
        message(buffer, MSG_NODELAY|MSG_SILENT);
      }

      timeout_yet(0);
    }

    drainBrailleOutput(brl, poll);
  } while (!timeout_yet(timeout));
  message("done", 0);
}
#endif /* ENABLE_LEARN_MODE */

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

void
setBrailleFirmness (BrailleDisplay *brl, int setting) {
  braille->firmness(brl, setting);
}
