/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

/* MiniBraille/braille.c - Braille display library
 * the following Tieman B.V. braille terminals are supported
 *
 * - MiniBraille v 1.5 (20 braille cells + 2 status)
 *   (probably other versions too)
 *
 * Brailcom o.p.s. <technik@brailcom.cz>
 *
 * Thanks to Tieman B.V., which gives me protocol information. Author.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Programs/misc.h"
#include "Programs/message.h"

#define BRLSTAT ST_TiemanStyle
#include "Programs/brl_driver.h"

#include "Programs/io_serial.h"
static SerialDevice *serialDevice = NULL;
static const int serialBaud = 9600;
static int serialCharactersPerSecond;

#define CR  0X0D
#define ESC 0X1B

#define KEY_F1    0x01
#define KEY_F2    0x02
#define KEY_LEFT  0x04
#define KEY_UP    0x08
#define KEY_C     0x10
#define KEY_DOWN  0x20
#define KEY_RIGHT 0x40

#define AFTER_CMD_DELAY 30

typedef enum {
  IM_Normal,
  IM_F1,
  IM_F2,
  IM_Manage
} InputMode;
static InputMode inputMode;

static TranslationTable outputTable;
static unsigned char textCells[20];
static unsigned char statusCells[2];
static int refreshNeeded;

static int
writeData (BrailleDisplay *brl, const unsigned char *bytes, int count) {
  ssize_t result = serialWriteData(serialDevice, bytes, count);

  if (result == -1) {
    LogError("write");
    return 0;
  }

  drainBrailleOutput(brl, 0);
  brl->writeDelay += (result * 1000 / serialCharactersPerSecond) + AFTER_CMD_DELAY;
  return 1;
}

static int
writeCells  (BrailleDisplay *brl) {
  static const unsigned char beginSequence[] = {ESC, 'Z', '1'};
  static const unsigned char endSequence[] = {CR};

  unsigned char buffer[sizeof(beginSequence) + sizeof(statusCells) + sizeof(textCells) + sizeof(endSequence)];
  unsigned char *byte = buffer;
  int i;

  memcpy(byte, beginSequence, sizeof(beginSequence));
  byte += sizeof(beginSequence);

  for (i=0; i<sizeof(statusCells); ++i) *byte++ = outputTable[statusCells[i]];
  for (i=0; i<sizeof(textCells); ++i) *byte++ = outputTable[textCells[i]];

  memcpy(byte, endSequence, sizeof(endSequence));
  byte += sizeof(endSequence);

  return writeData(brl, buffer, byte-buffer);
}

static void
updateCells (unsigned char *target, const unsigned char *source, size_t count) {
  if (memcmp(target, source, count) != 0) {
    memcpy(target, source, count);
    refreshNeeded = 1;
  }
}

static void
clearCells (unsigned char *cells, size_t count) {
  memset(cells, 0, count);
  refreshNeeded = 1;
}

static int
beep (BrailleDisplay *brl) {
  static const unsigned char sequence[] = {ESC, 'B', CR};
  return writeData(brl, sequence, sizeof(sequence));
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X80, 0X40, 0X20, 0X08, 0X10};
    makeOutputTable(dots, outputTable);
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, serialBaud)) {
      serialCharactersPerSecond = serialBaud / serialGetCharacterBits(serialDevice);

      /* hm, how to switch to 38400 ? 
      static const unsigned char sequence[] = {ESC, 'V', CR};
      writeData(brl, sequence, sizeof(sequence));
      serialDiscardInput(serialDevice);
      serialSetBaud(serialDevice, 38400);
      */

      clearCells(textCells,  sizeof(textCells));
      clearCells(statusCells,  sizeof(statusCells));
      inputMode = IM_Normal;

      brl->x = 20;
      brl->y = 1;

      beep(brl);
      return 1;
    }

    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }

  LogPrint(LOG_ERR, "cannot initialize MiniBraille");
  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  serialCloseDevice(serialDevice);
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  updateCells(textCells, brl->buffer, sizeof(textCells));
  if (refreshNeeded && (inputMode == IM_Normal)) {
    writeCells(brl);
    refreshNeeded = 0;
  }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *s) {
  updateCells(statusCells, s, sizeof(statusCells));
}

static void
setInputMode (InputMode mode, const char *name) {
  if (name) message(name, MSG_NODELAY|MSG_SILENT);
  inputMode = mode;
  refreshNeeded = 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  unsigned char byte;
  InputMode mode;

  {
    int result = serialReadData(serialDevice, &byte, 1, 0, 0);
    if (result == 0) return EOF;

    if (result == -1) {
      LogError("read");
      return BRL_CMD_RESTARTBRL;
    }
  }

  mode = inputMode;
  inputMode = IM_Normal;

  switch (mode) {
    case IM_Normal:
      switch (byte) {
        case KEY_F1:
          setInputMode(IM_F1, "F1 Mode");
          return BRL_CMD_NOOP;
        case KEY_F2:
          setInputMode(IM_F2, "F2 Mode");
          return BRL_CMD_NOOP;
        case KEY_C:
          return BRL_CMD_HOME;
        case KEY_UP:
          return BRL_CMD_LNUP;
        case KEY_DOWN:
          return BRL_CMD_LNDN;
        case KEY_LEFT:
          return BRL_CMD_FWINLT;
        case KEY_RIGHT:
          return BRL_CMD_FWINRT;
      }
      break;

    case IM_F1:
      switch (byte) {
        case KEY_F1:
          setInputMode(IM_Manage, "Manage Mode"); /* F1-F1 prechod na management */
          return BRL_CMD_NOOP;
        case KEY_F2:
          return BRL_CMD_CSRVIS;
        case KEY_C:
          return BRL_CMD_CAPBLINK;
        case KEY_UP:
          return BRL_CMD_TOP_LEFT;
        case KEY_DOWN:
          return BRL_CMD_BOT_LEFT;
        case KEY_LEFT:
          return BRL_CMD_LNBEG;
        case KEY_RIGHT:
          return BRL_CMD_LNEND;
      }
      break;

    case IM_F2:
      switch (byte) {
        case KEY_F1:
          return BRL_CMD_CSRSIZE;
        case KEY_F2:
          return BRL_CMD_SKPIDLNS;
        case KEY_C:
          return BRL_CMD_CSRTRK;
        case KEY_UP:
          return BRL_CMD_FREEZE;
        case KEY_DOWN:
          return BRL_CMD_DISPMD;
        case KEY_LEFT:
          return BRL_CMD_INFO;
        case KEY_RIGHT:
          return BRL_CMD_SIXDOTS;
      }
      break;

    case IM_Manage:
      switch (byte) {
        case KEY_F1:
          return BRL_CMD_HELP;
        case KEY_F2:
          return BRL_CMD_INFO;
        case KEY_C:
          return BRL_CMD_PREFLOAD;
        case KEY_UP:
          return BRL_CMD_PREFMENU;
        case KEY_DOWN:
          return BRL_CMD_PREFSAVE;
        case KEY_LEFT:
          return BRL_CMD_RESTARTBRL;
        case KEY_RIGHT: {
          time_t clock = time(NULL);
          const struct tm *local = localtime(&clock);
          char text[200];
          strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", local);
          message(text, 0);
          return BRL_CMD_NOOP;
        }
      }
      break;
  }

  LogPrint(LOG_WARNING, "unhandled key: %02X (mode %02X)", byte, mode);
  beep(brl);
  return EOF;
}
