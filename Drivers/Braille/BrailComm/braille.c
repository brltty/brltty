/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include <errno.h>
#include <string.h>

#include "io_serial.h"

typedef enum {
  PARM_BAUD
} DriverParameter;
#define BRLPARMS "baud"

#define BRL_STATUS_FIELDS sfGeneric
#define BRL_HAVE_STATUS_CELLS
#include "brl_driver.h"

static SerialDevice *serialDevice = NULL;
static int serialBaud;
static int charactersPerSecond;
static const int *initialCommand;

typedef enum {
  IPT_MINIMUM_LINE     =   1,
  IPT_MAXIMUM_LINE     =  25,
  IPT_SEARCH_ATTRIBUTE =  90,
  IPT_CURRENT_LINE     = 100,
  IPT_CURRENT_LOCATION = 101,
} InputPacketType;

typedef union {
  unsigned char bytes[4];

  struct {
    unsigned char type;

    union {
      struct {
        unsigned char line;
        unsigned char column;
        unsigned char attributes;
      } PACKED search;
    } fields;
  } PACKED data;
} InputPacket;

typedef int (*WriteFunction) (BrailleDisplay *brl);
static WriteFunction writeFunction;

static unsigned char statusCells[GSC_COUNT];

static const TranslationTable outputTable = {
  [0] = 0X20,
  [BRL_DOT1] = 0X61,
  [BRL_DOT1 | BRL_DOT2] = 0X62,
  [BRL_DOT1 | BRL_DOT4] = 0X63,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT5] = 0X64,
  [BRL_DOT1 | BRL_DOT5] = 0X65,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4] = 0X66,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5] = 0X67,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT5] = 0X68,
  [BRL_DOT2 | BRL_DOT4] = 0X69,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT5] = 0X6A,
  [BRL_DOT1 | BRL_DOT3] = 0X6B,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3] = 0X6C,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4] = 0X6D,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X6E,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT5] = 0X6F,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4] = 0X70,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X71,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5] = 0X72,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4] = 0X73,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X74,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT6] = 0X75,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6] = 0X76,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X78,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X79,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X7A,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT1 | BRL_DOT6] = 0XE5,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT6] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT6] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT1 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT6] = 0XF6,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X77,
  [BRL_DOT2] = 0X00,
  [BRL_DOT2 | BRL_DOT3] = 0X00,
  [BRL_DOT2 | BRL_DOT5] = 0X00,
  [BRL_DOT2 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT2 | BRL_DOT6] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT5] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT6] = 0X00,
  [BRL_DOT3 | BRL_DOT5] = 0X00,
  [BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT3 | BRL_DOT4] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0XE4,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X00,
  [BRL_DOT3] = 0X00,
  [BRL_DOT3 | BRL_DOT6] = 0X00,
  [BRL_DOT4] = 0X00,
  [BRL_DOT4 | BRL_DOT5] = 0X00,
  [BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT5] = 0X00,
  [BRL_DOT4 | BRL_DOT6] = 0X00,
  [BRL_DOT5 | BRL_DOT6] = 0X00,
  [BRL_DOT6] = 0X00,
  [BRL_DOT7] = 0X00,
  [BRL_DOT1 | BRL_DOT7] = 0X41,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT7] = 0X42,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT7] = 0X43,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X44,
  [BRL_DOT1 | BRL_DOT5 | BRL_DOT7] = 0X45,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT7] = 0X46,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X47,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT7] = 0X48,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT7] = 0X49,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X4A,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT7] = 0X4B,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT7] = 0X4C,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0X4D,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X4E,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0X4F,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0X50,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X51,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0X52,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0X53,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X54,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0X55,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0X56,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X58,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X59,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X5A,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT1 | BRL_DOT6 | BRL_DOT7] = 0XC5,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT1 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0XD6,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X57,
  [BRL_DOT2 | BRL_DOT7] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT7] = 0X00,
  [BRL_DOT2 | BRL_DOT5 | BRL_DOT7] = 0X00,
  [BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT2 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0X00,
  [BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0XC4,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT3 | BRL_DOT7] = 0X00,
  [BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT4 | BRL_DOT7] = 0X00,
  [BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X00,
  [BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT5 | BRL_DOT7] = 0X00,
  [BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT6 | BRL_DOT7] = 0X00,
  [BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00,
  [BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT8] = 0X00,
  [BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT4 | BRL_DOT8] = 0X00,
  [BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT5 | BRL_DOT8] = 0X00,
  [BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X00,
  [BRL_DOT6 | BRL_DOT8] = 0X00
};

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const int baudTable[] = {9600, 19200, 0};
    const char *baudParameter = parameters[PARM_BAUD];

    if (!*baudParameter ||
        !serialValidateBaud(&serialBaud, "baud", baudParameter, baudTable))
      serialBaud = baudTable[0];
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, serialBaud)) {
      charactersPerSecond = serialBaud / 10;
      writeFunction = NULL;

      {
        static const int initialCommands[] = {
          BRL_CMD_TUNES | BRL_FLG_TOGGLE_OFF,
          BRL_CMD_CSRTRK | BRL_FLG_TOGGLE_OFF,
          BRL_CMD_CSRVIS | BRL_FLG_TOGGLE_OFF,
          BRL_CMD_ATTRVIS | BRL_FLG_TOGGLE_OFF,
          EOF
        };

        initialCommand = initialCommands;
      }

      brl->textColumns = 80;
      return 1;
    }

    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
}

static int
writePacket (BrailleDisplay *brl, const unsigned char *packet, size_t size) {
  logOutputPacket(packet, size);
  brl->writeDelay += (size * 1000 / charactersPerSecond) + 1;
  return serialWriteData(serialDevice, packet, size) != -1;
}

static int
writeLine (BrailleDisplay *brl) {
  unsigned char packet[2 + (brl->textColumns * 2)];
  unsigned char *byte = packet;

  *byte++ = statusCells[gscCursorRow];
  *byte++ = statusCells[gscCursorColumn];

  {
    int i;

    for (i=0; i<brl->textColumns; i+=1) {
      *byte++ = outputTable[brl->buffer[i]];
      *byte++ = 0X07;
    }
  }

  return writePacket(brl, packet, byte-packet);
}

static int
writeLocation (BrailleDisplay *brl) {
  unsigned char packet[2];
  unsigned char *byte = packet;

  *byte++ = statusCells[gscCursorRow];
  *byte++ = statusCells[gscCursorColumn];

  return writePacket(brl, packet, byte-packet);
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (writeFunction) {
    int ok = writeFunction(brl);
    writeFunction = NULL;
    if (!ok) return 0;
  }

  return 1;
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  memcpy(statusCells, status, GSC_COUNT);
  return 1;
}

static int
readByte (unsigned char *byte, int wait) {
  const int timeout = 100;
  ssize_t result = serialReadData(serialDevice,
                                  byte, sizeof(*byte),
                                  (wait? timeout: 0), timeout);

  if (result > 0) return 1;
  if (result == 0) errno = EAGAIN;
  return 0;
}

static int
readPacket (BrailleDisplay *brl, InputPacket *packet) {
  int length = 1;
  int offset = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;
      if (!readByte(&byte, started)) {
        if (started) logPartialPacket(packet->bytes, offset);
        return 0;
      }
    }

    if (!offset) {
      switch (byte) {
        case IPT_CURRENT_LINE:
        case IPT_CURRENT_LOCATION:
          length = 1;
          break;

        case IPT_SEARCH_ATTRIBUTE:
          length = 4;
          break;

        default:
          if ((byte >= IPT_MINIMUM_LINE) && (byte <= IPT_MAXIMUM_LINE)) {
            length = 1;
          } else {
            logIgnoredByte(byte);
            continue;
          }
          break;
      }
    }

    packet->bytes[offset++] = byte;
    if (offset == length) {
      logInputPacket(packet->bytes, offset);
      return length;
    }
  }
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  InputPacket packet;
  int length;

  if (context == BRL_CTX_WAITING) return BRL_CMD_NOOP;
  if (writeFunction) return EOF;
  while (*initialCommand != EOF) enqueueCommand(*initialCommand++);

  while ((length = readPacket(brl, &packet))) {
    if ((packet.data.type >= IPT_MINIMUM_LINE) &&
        (packet.data.type <= IPT_MAXIMUM_LINE)) {
      enqueueCommand(BRL_BLK_GOTOLINE | BRL_FLG_LINE_TOLEFT | (packet.data.type - IPT_MINIMUM_LINE));
      writeFunction = writeLine;
      return EOF;
    }

    switch (packet.data.type) {
      case IPT_SEARCH_ATTRIBUTE:
      case IPT_CURRENT_LINE:
        enqueueCommand(BRL_CMD_HOME);
        enqueueCommand(BRL_CMD_LNBEG);
        writeFunction = writeLine;
        return EOF;

      case IPT_CURRENT_LOCATION:
        writeFunction = writeLocation;
        return EOF;

      default:
        logUnexpectedPacket(&packet, length);
        break;
    }
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
