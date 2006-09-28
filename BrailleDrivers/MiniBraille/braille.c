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

#define KEY_F1     0x01
#define KEY_F2     0x02
#define KEY_LEFT   0x04
#define KEY_UP     0x08
#define KEY_CENTER 0x10
#define KEY_DOWN   0x20
#define KEY_RIGHT  0x40

#define AFTER_CMD_DELAY 30

static int
inputFunction_showTime (BrailleDisplay *brl) {
  time_t clock = time(NULL);
  const struct tm *local = localtime(&clock);
  char text[200];
  strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", local);
  message(text, 0);
  return BRL_CMD_NOOP;
}

typedef struct InputModeStruct InputMode;

typedef enum {
  IBT_unbound = 0, /* automatically set if not explicitly initialized */
  IBT_command,
  IBT_function,
  IBT_submode
} InputBindingType;

typedef union {
  int command;
  int (*function) (BrailleDisplay *brl);
  const InputMode *submode;
} InputBindingValue;

typedef struct {
  InputBindingType type;
  InputBindingValue value;
} InputBinding;

struct InputModeStruct {
  const char *name;
  InputBinding f1, f2, left, up, center, down, right;
};

#define BIND(k,t,v) .k = {.type = IBT_##t, .value.t = (v)}
#define BIND_COMMAND(k,c) BIND(k, command, BRL_CMD_##c)
#define BIND_FUNCTION(k,f) BIND(k, function, inputFunction_##f)
#define BIND_SUBMODE(k,m) BIND(k, submode, &inputMode_##m)

static const InputMode inputMode_f1_f1 = {
  BIND_COMMAND(f1, HELP),
  BIND_COMMAND(f2, LEARN),
  BIND_COMMAND(left, INFO),
  BIND_FUNCTION(right, showTime),
  BIND_COMMAND(up, PREFLOAD),
  BIND_COMMAND(down, PREFMENU),
  BIND_COMMAND(center, PREFSAVE),
  .name = "f1-f1"
};

static const InputMode inputMode_f1_f2 = {
  BIND_COMMAND(f1, FREEZE),
  BIND_COMMAND(f2, DISPMD),
  BIND_COMMAND(left, ATTRVIS),
  BIND_COMMAND(right, CSRVIS),
  BIND_COMMAND(up, SKPBLNKWINS),
  BIND_COMMAND(down, SKPIDLNS),
  BIND_COMMAND(center, SIXDOTS),
  .name = "f1-f2"
};

static const InputMode inputMode_f1_left = {
  .name = "f1-left"
};

static const InputMode inputMode_f1_right = {
  .name = "f1-right"
};

static const InputMode inputMode_f1_up = {
  BIND_COMMAND(f1, PRSEARCH),
  BIND_COMMAND(f2, NXSEARCH),
  BIND_COMMAND(left, ATTRUP),
  BIND_COMMAND(right, ATTRDN),
  BIND_COMMAND(up, PRPGRPH),
  BIND_COMMAND(down, NXPGRPH),
  BIND_COMMAND(center, CSRJMP_VERT),
  .name = "f1-up"
};

static const InputMode inputMode_f1_down = {
  BIND_COMMAND(f1, PRPROMPT),
  BIND_COMMAND(f2, NXPROMPT),
  BIND_COMMAND(left, FWINLTSKIP),
  BIND_COMMAND(right, FWINRTSKIP),
  BIND_COMMAND(up, PRDIFLN),
  BIND_COMMAND(down, NXDIFLN),
  BIND_COMMAND(center, BACK),
  .name = "f1-down"
};

static const InputMode inputMode_f1_center = {
  .name = "f1-center"
};

static const InputMode inputMode_f1 = {
  BIND_SUBMODE(f1, f1_f1),
  BIND_SUBMODE(f2, f1_f2),
  BIND_SUBMODE(left, f1_left),
  BIND_SUBMODE(right, f1_right),
  BIND_SUBMODE(up, f1_up),
  BIND_SUBMODE(down, f1_down),
  BIND_SUBMODE(center, f1_center),
  .name = "f1"
};

static const InputMode inputMode_f2 = {
  BIND_COMMAND(f1, TOP_LEFT),
  BIND_COMMAND(f2, BOT_LEFT),
  BIND_COMMAND(left, LNBEG),
  BIND_COMMAND(right, LNEND),
  BIND_COMMAND(up, TOP),
  BIND_COMMAND(down, BOT),
  BIND_COMMAND(center, CSRTRK),
  .name = "f2"
};

static const InputMode inputMode_basic = {
  BIND_SUBMODE(f1, f1),
  BIND_SUBMODE(f2, f2),
  BIND_COMMAND(left, FWINLT),
  BIND_COMMAND(right, FWINRT),
  BIND_COMMAND(up, LNUP),
  BIND_COMMAND(down, LNDN),
  BIND_COMMAND(center, HOME),
  .name = "basic"
};

static const InputMode *inputMode;
static struct timeval inputTime;

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
      inputMode = &inputMode_basic;

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
  if (refreshNeeded && (inputMode == &inputMode_basic)) {
    writeCells(brl);
    refreshNeeded = 0;
  }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *s) {
  updateCells(statusCells, s, sizeof(statusCells));
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  unsigned char byte;
  const InputMode *mode;
  const InputBinding *binding;

  {
    int result = serialReadData(serialDevice, &byte, 1, 0, 0);

    if (result == 0) {
      if (inputMode != &inputMode_basic) {
        struct timeval now;
        gettimeofday(&now, NULL);
        if (millisecondsBetween(&inputTime, &now) > 3000) inputMode = &inputMode_basic;
      }

      return EOF;
    }

    if (result == -1) {
      LogError("read");
      return BRL_CMD_RESTARTBRL;
    }
  }

  mode = inputMode;
  inputMode = &inputMode_basic;

  switch (byte) {
    case KEY_F1:     binding = &mode->f1;     break;
    case KEY_F2:     binding = &mode->f2;     break;
    case KEY_LEFT:   binding = &mode->left;   break;
    case KEY_RIGHT:  binding = &mode->right;  break;
    case KEY_UP:     binding = &mode->up;     break;
    case KEY_DOWN:   binding = &mode->down;   break;
    case KEY_CENTER: binding = &mode->center; break;

    default:
      LogPrint(LOG_WARNING, "unhandled key: %02X (%s)", byte, mode->name);
      beep(brl);
      return EOF;
  }

  switch (binding->type) {
    case IBT_unbound:
      LogPrint(LOG_WARNING, "unbound key: %s -> %02X", mode->name, byte);
      beep(brl);
      break;

    case IBT_command:
      return binding->value.command;

    case IBT_function:
      return binding->value.function(brl);

    case IBT_submode:
      mode = binding->value.submode;
      message(mode->name, MSG_NODELAY|MSG_SILENT);
      inputMode = mode;
      gettimeofday(&inputTime, NULL);
      break;

    default:
      LogPrint(LOG_WARNING, "unhandled input binding type: %02X", binding->type);
      break;
  }

  return BRL_CMD_NOOP;
}
