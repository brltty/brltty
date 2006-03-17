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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "Programs/misc.h"

#define BRL_HAVE_VISUAL_DISPLAY
#include "Programs/brl_driver.h"
#include "Programs/tbl.h"

static TranslationTable outputTable;
static TranslationTable inputTable;
static unsigned char brailleCells[0XFF];
static unsigned char visualCells[0XFF];

#include "Programs/io_serial.h"
static SerialDevice *serialDevice = NULL;
static int charactersPerSecond;

static int
readPacket (BrailleDisplay *brl, unsigned char *packet, int length) {
  size_t offset = 0;
  int size = -1;

  while (offset < length) {
    const unsigned char *byte = &packet[offset];

    if (!serialReadChunk(serialDevice, packet, &offset, 1, 0, 1000)) {
      if (errno == EAGAIN) {
        if (!offset) return 0;
        LogBytes("Aborted Input", packet, offset);
      }
      return -1;
    }

    if (offset == 1) {
      if (*byte) {
        LogBytes("Discarded Input", packet, offset);
        offset = 0;
      }
    } else {
      if (offset == 2) {
        switch (*byte) {
          default:
            size = 1;
            break;
        }
        size += offset;
      }

      if (offset == size) {
      //LogBytes("Input Packet", packet, offset);
        return offset;
      }
    }
  }

  LogBytes("Truncated Input", packet, offset);
  return 0;
}

static int
writePacket (BrailleDisplay *brl, unsigned char function, unsigned char *data, unsigned char count) {
  unsigned char buffer[count + 4];
  unsigned char *byte = buffer;

  *byte++ = 0;
  *byte++ = function;
  *byte++ = count;

  memcpy(byte, data, count);
  byte += count;

  {
    unsigned char checksum = 0;
    const unsigned char *ptr = buffer;
    while (ptr < byte) checksum ^= *ptr++;
    *byte++ = checksum;
  }

  {
    int size = byte - buffer;
  //LogBytes("Output Packet", buffer, size);
    brl->writeDelay += (count * 1000 / charactersPerSecond) + 1;
    if (serialWriteData(serialDevice, buffer, size) != -1) return 1;
  }

  LogError("serial write");
  return 0;
}

static int
writeBrailleCells (BrailleDisplay *brl) {
  return writePacket(brl, 1, brailleCells, brl->x);
}

static int
clearBrailleCells (BrailleDisplay *brl) {
  memset(brailleCells, 0, brl->x);
  return writeBrailleCells(brl);
}

static int
writeVisualCells (BrailleDisplay *brl) {
  return writePacket(brl, 2, visualCells, brl->x);
}

static int
clearVisualCells (BrailleDisplay *brl) {
  memset(visualCells, ' ', brl->x);
  return writeVisualCells(brl);
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
    reverseTranslationTable(outputTable, inputTable);
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  if ((serialDevice = serialOpenDevice(device))) {
    int baud = 19200;
    charactersPerSecond = baud / 11;

    if (serialRestartDevice(serialDevice, baud)) {
      if (serialSetParity(serialDevice, SERIAL_PARITY_EVEN)) {
        if (writePacket(brl, 4, NULL, 0)) {
          while (serialAwaitInput(serialDevice, 500)) {
            unsigned char response[3];
            int size = readPacket(brl, response, sizeof(response));
            if (size <= 0) break;

            if (response[1] == 4) {
              brl->x = response[2];
              brl->y = 1;
              brl->helpPage = 0;

              if (!clearBrailleCells(brl)) break;
              if (!clearVisualCells(brl)) break;
              if (!writeBrailleCells(brl)) break;

              return 1;
            }
          }
        }
      }
    }

    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
  
  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  unsigned char cells[brl->x];
  int i;
  for (i=0; i<brl->x; ++i) cells[i] = outputTable[brl->buffer[i]];
  if (memcmp(cells, brailleCells, brl->x) != 0) {
    memcpy(brailleCells, cells, brl->x);
    writeBrailleCells(brl);
  }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
}

static void
brl_writeVisual (BrailleDisplay *brl) {
  if (memcmp(brl->buffer, visualCells, brl->x) != 0) {
    memcpy(visualCells, brl->buffer, brl->x);
    writeVisualCells(brl);
  }
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  while (1) {
    unsigned char packet[3];
    int size = readPacket(brl, packet, sizeof(packet));
    if (size == 0) break;
    if (size < 0) return BRL_CMD_RESTARTBRL;

    switch (packet[1]) {
      default:
        break;

      case 1:
        return BRL_BLK_PASSDOTS | inputTable[packet[2]];

      case 2: {
        unsigned char column = packet[2];
        if (column && (column <= brl->x)) return BRL_BLK_ROUTE + (column - 1);
        break;
      }

      case 3:
        switch (packet[2]) {
          default:
            break;

          // left rear: two columns, one row
          case 0X02: // ESC
            return BRL_CMD_LEARN;
          case 0X01: // M
            return BRL_CMD_PREFMENU;

          // left middle: cross
          case 0X06: // up
            return BRL_CMD_LNUP;
          case 0X03: // left
            return BRL_CMD_FWINLT;
          case 0X05: // right
            return BRL_CMD_FWINRT;
          case 0X04: // down
            return BRL_CMD_LNDN;

          // left front: two columns, three rows
          case 0X09: // ins
            return BRL_CMD_RETURN;
          case 0X0A: // E
            return BRL_CMD_TOP;
          case 0X0B: // supp
            return BRL_CMD_CSRTRK;
          case 0X0C: // L
            return BRL_CMD_BOT;
          case 0X07: // extra 1 (40s only)
            return BRL_CMD_CHRLT;
          case 0X08: // extra 2 (40s only)
            return BRL_CMD_CHRRT;

          case 0x0E: // left thumb
            return BRL_BLK_PASSKEY + BRL_KEY_BACKSPACE;
          case 0x0F: // right thumb
            return BRL_BLK_PASSDOTS;
          case 0x3F: // both thumbs
            return BRL_BLK_PASSKEY + BRL_KEY_ENTER;

          case 0X29: // key under dot 7
            return BRL_BLK_PASSKEY + BRL_KEY_ESCAPE;
          case 0X2A: // key under dot 8
            return BRL_BLK_PASSKEY + BRL_KEY_TAB;

          // right rear: one column, one row
          case 0X19: // extra 3 (40s only)
            return BRL_CMD_INFO;

          // right middle: one column, two rows
          case 0X1B: // extra 4 (40s only)
            return BRL_CMD_PRDIFLN;
          case 0X1A: // extra 5 (40s only)
            return BRL_CMD_NXDIFLN;

          // right front: one column, four rows
          case 0X2B: // slash (40s only)
            return BRL_CMD_FREEZE;
          case 0X2C: // asterisk (40s only)
            return BRL_CMD_DISPMD;
          case 0X2D: // minus (40s only)
            return BRL_CMD_ATTRVIS;
          case 0X2E: // plus (40s only)
            return BRL_CMD_CSRVIS;

          // first (top) row of numeric pad
          case 0X37: // seven (40s only)
            return BRL_BLK_PASSKEY + BRL_KEY_HOME;
          case 0X38: // eight (40s only)
            return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP;
          case 0X39: // nine (40s only)
            return BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP;

          // second row of numeric pad
          case 0X34: // four (40s only)
            return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT;
          case 0X35: // five (40s only)
            return BRL_CMD_CSRJMP_VERT;
          case 0X36: // six (40s only)
            return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT;

          // third row of numeric pad
          case 0X31: // one (40s only)
            return BRL_BLK_PASSKEY + BRL_KEY_END;
          case 0X32: // two (40s only)
            return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN;
          case 0X33: // three (40s only)
            return BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN;

          // fourth (bottom) row of numeric pad
          case 0X28: // verr num (40s only)
            return BRL_CMD_SIXDOTS;
          case 0X30: // zero (40s only)
            return BRL_BLK_PASSKEY + BRL_KEY_INSERT;
          case 0X2F: // supp (40s only)
            return BRL_BLK_PASSKEY + BRL_KEY_DELETE;
        }
        break;

      /* When data is written to the display it acknowledges with:
       * 0X00 0X04 0Xxx
       * where xx is the number of bytes written.
       */
      case 4:
        continue;
    }

    LogBytes("Unhandled Input", packet, size);
  }

  return EOF;
}
