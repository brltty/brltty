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

/* Albatross/braille.c - Braille display library
 * Tivomatic's Albatross series
 * Author: Dave Mielke <dave@mielke.cc>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Programs/misc.h"

#include "Programs/brl_driver.h"
#include "braille.h"
#include "Programs/io_serial.h"

#define LOWER_ROUTING_DEFAULT BRL_BLK_ROUTE
#define UPPER_ROUTING_DEFAULT BRL_BLK_DESCCHAR

static SerialDevice *serialDevice = NULL;
static int charactersPerSecond;

static TranslationTable inputMap;
static const unsigned char topLeftKeys[]  = { 84,  83,  87,  85,  86,  88,  89,  90};
static const unsigned char topRightKeys[] = {194, 193, 198, 195, 196, 197, 199, 200};
static int lowerRoutingFunction;
static int upperRoutingFunction;

static TranslationTable outputTable;
static unsigned char displayContent[80];
static int displaySize;
static int windowWidth;
static int windowStart;
static int statusCount;
static int statusStart;

static int
readByte (unsigned char *byte) {
  int received = serialReadData(serialDevice, byte, 1, 0, 0);
  if (received == -1) LogError("Albatross read");
  return received == 1;
}

static int
awaitByte (unsigned char *byte) {
  if (readByte(byte)) return 1;
  if (serialAwaitInput(serialDevice, 1000))
    if (readByte(byte))
      return 1;
  return 0;
}

static int
writeBytes (BrailleDisplay *brl, unsigned char *bytes, int count) {
  brl->writeDelay += (count * 1000 / charactersPerSecond) + 1;
  if (serialWriteData(serialDevice, bytes, count) != -1) return 1;
  LogError("Albatross write");
  return 0;
}

static int
acknowledgeDisplay (BrailleDisplay *brl) {
  unsigned char description;
  if (!awaitByte(&description)) return 0;
  if (description == 0XFF) return 0;

  {
    unsigned char byte;

    if (!awaitByte(&byte)) return 0;
    if (byte != 0XFF) return 0;

    if (!awaitByte(&byte)) return 0;
    if (byte != description) return 0;
  }

  {
    unsigned char acknowledgement[] = {0XFE, 0XFF, 0XFE, 0XFF};
    if (!writeBytes(brl, acknowledgement, sizeof(acknowledgement))) return 0;

    serialDiscardInput(serialDevice);
    approximateDelay(100);
    serialDiscardInput(serialDevice);
  }
  LogPrint(LOG_DEBUG, "Albatross description byte: %02X", description);

  windowStart = statusStart = 0;
  displaySize = (description & 0X80)? 80: 46;
  if ((statusCount = description & 0X0F)) {
    windowWidth = displaySize - statusCount - 1;
    if (description & 0X20) {
      statusStart = windowWidth + 1;
      displayContent[statusStart - 1] = 0;
    } else {
      windowStart = statusCount + 1;
      displayContent[windowStart - 1] = 0;
    }
  } else {
    windowWidth = displaySize;
  }

  {
    int i;
    for (i=0; i<sizeof(inputMap); ++i) inputMap[i] = i;

    /* top keypad remapping */
    {
      const unsigned char *left = NULL;
      const unsigned char *right = NULL;

      switch (description & 0X50) {
        case 0X00: /* left right */
          break;

        case 0X10: /* right right */
          left = topRightKeys;
          break;

        case 0X50: /* left left */
          right = topLeftKeys;
          break;

        case 0X40: /* right left */
          left = topRightKeys;
          right = topLeftKeys;
          break;
      }

      if (left)
        for (i=0; i<8; ++i)
          inputMap[topLeftKeys[i]] = left[i];
      if (right)
        for (i=0; i<8; ++i)
          inputMap[topRightKeys[i]] = right[i];
    }
  }

  lowerRoutingFunction = LOWER_ROUTING_DEFAULT;
  upperRoutingFunction = UPPER_ROUTING_DEFAULT;

  LogPrint(LOG_INFO, "Albatross: %d cells (%d text, %d%s status), top keypads [%s,%s].",
           displaySize, windowWidth, statusCount,
           !statusCount? "": statusStart? " right": " left",
           (inputMap[topLeftKeys[0]] == topLeftKeys[0])? "left": "right",
           (inputMap[topRightKeys[0]] == topRightKeys[0])? "right": "left");
  return 1;
}

static int
clearDisplay (BrailleDisplay *brl) {
  unsigned char bytes[] = {0XFA};
  int cleared = writeBytes(brl, bytes, sizeof(bytes));
  if (cleared) memset(displayContent, 0, displaySize);
  return cleared;
}

static int
updateDisplay (BrailleDisplay *brl, const unsigned char *cells, int count, int start) {
  static time_t lastUpdate = 0;
  unsigned char bytes[count * 2 + 2];
  unsigned char *byte = bytes;
  int index;
  *byte++ = 0XFB;
  for (index=0; index<count; ++index) {
    unsigned char cell;
    if (!cells) {
      cell = displayContent[start+index];
    } else if ((cell = outputTable[cells[index]]) != displayContent[start+index]) {
      displayContent[start+index] = cell;
    } else {
      continue;
    }
    *byte++ = start + index + 1;
    *byte++ = cell;
  }

  if (((byte - bytes) > 1) || (time(NULL) != lastUpdate)) {
    *byte++ = 0XFC;
    if (!writeBytes(brl, bytes, byte-bytes)) return 0;
    lastUpdate = time(NULL);
  }
  return 1;
}

static int
updateWindow (BrailleDisplay *brl, const unsigned char *cells) {
  return updateDisplay(brl, cells, windowWidth, windowStart);
}

static int
updateStatus (BrailleDisplay *brl, const unsigned char *cells) {
  return updateDisplay(brl, cells, statusCount, statusStart);
}

static int
refreshDisplay (BrailleDisplay *brl) {
  return updateDisplay(brl, NULL, displaySize, 0);
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X80, 0X40, 0X20, 0X10, 0X08, 0X04, 0X02, 0X01};
    makeOutputTable(dots, outputTable);
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  if ((serialDevice = serialOpenDevice(device))) {
    int baudTable[] = {19200, 9600, 0};
    const int *baud = baudTable;

    while (serialRestartDevice(serialDevice, *baud)) {
      time_t start = time(NULL);
      int count = 0;
      unsigned char byte;

      charactersPerSecond = *baud / 10;
      LogPrint(LOG_DEBUG, "Trying Albatross at %d baud.", *baud);
      while (awaitByte(&byte)) {
        if (byte == 0XFF) {
          if (!acknowledgeDisplay(brl)) break;
          clearDisplay(brl);
          brl->x = windowWidth;
          brl->y = 1;
          return 1;
        }

        if (++count == 100) break;
        if (difftime(time(NULL), start) > 5.0) break;
      }

      if (!*++baud) baud = baudTable;
    }

    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  serialCloseDevice(serialDevice);
  serialDevice = NULL;
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  updateWindow(brl, brl->buffer);
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  updateStatus(brl, status);
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  unsigned char byte;

  while (readByte(&byte)) {
    if (byte == 0XFF) {
      if (acknowledgeDisplay(brl))  {
        refreshDisplay(brl);
        brl->x = windowWidth;
        brl->y = 1;
        brl->resizeRequired = 1;
      }
      continue;
    }

    byte = inputMap[byte];
    {
      int base;
      int offset;

      int lower = lowerRoutingFunction;
      int upper = upperRoutingFunction;
      lowerRoutingFunction = LOWER_ROUTING_DEFAULT;
      upperRoutingFunction = UPPER_ROUTING_DEFAULT;

      if ((byte >= 2) && (byte <= 41)) {
        base = lower;
        offset = byte - 2;
      } else if ((byte >= 111) && (byte <= 150)) {
        base = lower;
        offset = byte - 71;
      } else if ((byte >= 43) && (byte <= 82)) {
        base = upper;
        offset = byte - 43;
      } else if ((byte >= 152) && (byte <= 191)) {
        base = upper;
        offset = byte - 112;
      } else {
        goto notRouting;
      }
      if ((offset >= windowStart) &&
          (offset < (windowStart + windowWidth)))
        return base + offset - windowStart;
    }
  notRouting:

    switch (byte) {
      default:
        break;

      case 0XFB:
        refreshDisplay(brl);
        continue;

      case  83: /* key: top left first lower */
        return BRL_CMD_LEARN;

      case  84: /* key: top left first upper */
        return BRL_CMD_HELP;

      case  85: /* key: top left third upper */
        return BRL_CMD_PASTE;

      case  86: /* key: top left third lower */
        return BRL_CMD_CSRTRK;

      case  87: /* key: top left second */
        lowerRoutingFunction = BRL_BLK_CUTBEGIN;
        upperRoutingFunction = BRL_BLK_SETMARK;
        return BRL_CMD_NOOP;

      case  88: /* key: top left fourth */
        lowerRoutingFunction = BRL_BLK_CUTAPPEND;
        upperRoutingFunction = BRL_BLK_GOTOMARK;
        return BRL_CMD_NOOP;

      case  89: /* key: top left fifth upper */
        return BRL_CMD_PREFMENU;

      case  90: /* key: top left fifth lower */
        return BRL_CMD_INFO;

      case 193: /* key: top right first lower */
        return BRL_CMD_NXPROMPT;

      case 194: /* key: top right first upper */
        return BRL_CMD_PRPROMPT;

      case 195: /* key: top right third upper */
        return BRL_CMD_PRDIFLN;

      case 196: /* key: top right third lower */
        return BRL_CMD_NXDIFLN;

      case 198: /* key: top right second */
        lowerRoutingFunction = BRL_BLK_CUTRECT;
        upperRoutingFunction = BRL_BLK_NXINDENT;
        return BRL_CMD_NOOP;

      case 197: /* key: top right fourth */
        lowerRoutingFunction = BRL_BLK_CUTLINE;
        upperRoutingFunction = BRL_BLK_PRINDENT;
        return BRL_CMD_NOOP;

      case 199: /* key: top right fifth upper */
        return BRL_CMD_PRPGRPH;

      case 200: /* key: top right fifth lower */
        return BRL_CMD_NXPGRPH;

      case  91: /* key: front left first upper */
      case 201: /* key: front right first upper */
        return BRL_CMD_TOP_LEFT;

      case  92: /* key: front left first lower */
      case 202: /* key: front right first lower */
        return BRL_CMD_BOT_LEFT;

      case  93: /* key: front left second upper */
      case 203: /* key: front right second upper */
        return BRL_CMD_BACK;

      case  94: /* key: front left second lower */
      case 204: /* key: front right second lower */
        return BRL_CMD_HOME;

      case  95: /* key: front left third upper */
      case 205: /* key: front right third upper */
      case  98:
        return BRL_CMD_LNUP;

      case  96: /* key: front left third lower */
      case 206: /* key: front right third lower */
      case 208:
        return BRL_CMD_LNDN;

      case  97: /* key: front left fourth */
        return BRL_CMD_FWINLT;

      case 207: /* key: front right fourth */
        return BRL_CMD_FWINRT;

      case 103: /* wheel: front left right */
      case 213: /* wheel: front right right */
        return BRL_CMD_CHRRT;

      case 104: /* wheel: front left left */
      case 214: /* wheel: front right left */
        return BRL_CMD_CHRLT;

      case 105: /* wheel: side left backward */
      case 215: /* wheel: side right backward */
        return BRL_CMD_LNUP;

      case 106: /* wheel: side left forward */
      case 216: /* wheel: side right forward */
        return BRL_CMD_LNDN;

      case  42: /* key: attribute left upper */
        return BRL_CMD_FREEZE;

      case   1: /* key: attribute left lower */
        return BRL_CMD_DISPMD;

      case 192: /* key: attribute right upper */
        return BRL_CMD_ATTRUP;

      case 151: /* key: attribute right lower */
        return BRL_CMD_ATTRDN;
    }

    LogPrint(LOG_WARNING, "Unexpected byte: %02X", byte);
  }

  return EOF;
}
