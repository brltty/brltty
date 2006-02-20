/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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

#include "Programs/brl_driver.h"
#include "Programs/io_serial.h"

static SerialDevice *serialDevice = NULL;
static int charactersPerSecond;
static unsigned char *outputBuffer = NULL;
static TranslationTable outputTable;

static int
readBytes (unsigned char *buffer, int size, size_t *length) {
  *length = 0;

  while (*length < size) {
    unsigned char byte;

    if (!serialReadChunk(serialDevice, buffer, length, 1, 0, 100)) {
      return 0;
    }
    byte = buffer[*length - 1];

    if ((*length == 1) && (byte == 0X06)) {
      *length = 0;
      continue;
    }

    if (byte == '\r') {
      LogBytes("Read", buffer, *length);
      return 1;
    }
  }

  return 0;
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, int count) {
  LogBytes("Write", bytes, count);
  if (serialWriteData(serialDevice, bytes, count) == -1) return 0;
  brl->writeDelay += (count * 1000 / charactersPerSecond) + 1;
  return 1;
}

static int
writeAcknowledgement (BrailleDisplay *brl) {
  static const unsigned char acknowledgement[] = {0X06};
  return writeBytes(brl, acknowledgement, sizeof(acknowledgement));
}

static int
writeCells (BrailleDisplay *brl) {
  static const unsigned char header[] = {'D'};
  static const unsigned char trailer[] = {'\r'};
  unsigned char buffer[sizeof(header) + brl->x + sizeof(trailer)];
  unsigned char *byte = buffer;

  memcpy(byte, header, sizeof(header));
  byte += sizeof(header);

  {
    int i;
    for (i=0; i<brl->x; *byte++=outputTable[outputBuffer[i++]]);
  }

  memcpy(byte, trailer, sizeof(trailer));
  byte += sizeof(trailer);

  return writeBytes(brl, buffer, byte-buffer);
}

static int
writeString (BrailleDisplay *brl, const char *string) {
  return writeBytes(brl, (const unsigned char *)string, strlen(string));
}

static int
skipCharacter (unsigned char character, const unsigned char **bytes, int *count) {
  int found = 0;

  while (*count) {
    if (**bytes != character) break;
    found = 1;
    ++*bytes, --*count;
  }

  return found;
}

static int
interpretNumber (int *number, const unsigned char **bytes, int *count) {
  int ok = skipCharacter('0', bytes, count);
  *number = 0;

  while (*count) {
    static unsigned char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    const unsigned char *digit = memchr(digits, **bytes, sizeof(digits));
    if (!digit) break;

    *number = (*number * 10) + (digit - digits);
    ok = 1;
    ++*bytes, --*count;
  }

  return ok;
}

static int
identifyDisplay (BrailleDisplay *brl) {
  static const unsigned char identify[] = {'I', '\r'};

  if (writeBytes(brl, identify, sizeof(identify))) {
    if (serialAwaitInput(serialDevice, 1000)) {
      unsigned char identity[0X100];
      size_t length;

      if (readBytes(identity, sizeof(identity), &length)) {
        static const unsigned char prefix[] = {'b', 'r', 'a', 'u', 'd', 'i', ' '};
        if ((length >= sizeof(prefix)) &&
            (memcmp(identity, prefix, sizeof(prefix)) == 0)) {
          const unsigned char *bytes = memchr(identity, ',', length);
          if (bytes) {
            int count = length - (bytes - identity);
            int cells;

            ++bytes, --count;
            skipCharacter(' ', &bytes, &count);
            if (interpretNumber(&cells, &bytes, &count)) {
              if (!count) {
                LogPrint(LOG_INFO, "Detected: %.*s", (int)length, identity);

                brl->x = cells;
                brl->y = 1;

                return 1;
              }
            }
          }
        }

        LogBytes("Unrecognized identity", identity, length);
      }
    }
  }
  return 0;
}

static int
setTable (BrailleDisplay *brl, int table) {
  char buffer[0X10];
  snprintf(buffer, sizeof(buffer), "L%d\r", table);
  return writeString(brl, buffer);
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X10, 0X20, 0X40, 0X08, 0X80};
    makeOutputTable(dots, outputTable);
  }
  
  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  if ((serialDevice = serialOpenDevice(device))) {
    static const int baud = 9600;
    charactersPerSecond = baud / 10;
    if (serialRestartDevice(serialDevice, baud)) {
      if (identifyDisplay(brl)) {
        if ((outputBuffer = malloc(brl->x))) {
          if (setTable(brl, 0)) {
            memset(outputBuffer, 0, brl->x);
            writeCells(brl);

            return 1;
          }

          free(outputBuffer);
          outputBuffer = NULL;
        } else {
          LogError("Output buffer allocation");
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
  if (outputBuffer) {
    free(outputBuffer);
    outputBuffer = NULL;
  }

  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  if (memcmp(brl->buffer, outputBuffer, brl->x) != 0) {
    memcpy(outputBuffer, brl->buffer, brl->x);
    writeCells(brl);
  }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  unsigned char buffer[0X100];
  size_t length;

  while (readBytes(buffer, sizeof(buffer), &length)) {
    const unsigned char *bytes = buffer;
    int count = length;

    if (count > 0) {
      unsigned char category = *bytes++;
      --count;

      switch (category) {
        case 'F': {
          int keys;
          writeAcknowledgement(brl);

          if (interpretNumber(&keys, &bytes, &count)) {
            if (!count) {
              switch (keys) {
                case  1: return BRL_CMD_TOP_LEFT;
                case  2: return BRL_CMD_FWINLT;
                case  3: return BRL_CMD_LNDN;
                case  4: return BRL_CMD_LNUP;
                case  5: return BRL_CMD_FWINRT;
                case  6: return BRL_CMD_BOT_LEFT;
                case 23: return BRL_CMD_LNBEG;
                case 56: return BRL_CMD_LNEND;
                case 14: return BRL_CMD_CSRVIS;
                case 25: return BRL_CMD_DISPMD;
                case 26: return BRL_CMD_INFO;
                case 36: return BRL_CMD_HOME;
              }
            }
          }

          break;
        }

        case 'K': {
          int key;
          writeAcknowledgement(brl);

          if (interpretNumber(&key, &bytes, &count)) {
            if (!count) {
              if ((key > 0) && (key <= brl->x)) return BRL_BLK_ROUTE + (key - 1);
            }
          }

          break;
        }
      }
    }

    LogBytes("Unexpected input", buffer, length);
  }

  if (errno == EAGAIN) return EOF;
  return BRL_CMD_RESTARTBRL;
}
