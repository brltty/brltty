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

/* Albatross/braille.c - Braille display library
 * Tivomatic's Albatross series
 * Author: Dave Mielke <dave@mielke.cc>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "Programs/misc.h"

#include "Programs/brl_driver.h"
#include "braille.h"

static int fileDescriptor = -1;
static struct termios oldSettings;
static struct termios newSettings;

static TranslationTable outputTable;
static int cellCount;
static unsigned char cellContent[80];

static int
readByte (unsigned char *byte) {
  int received = read(fileDescriptor, byte, 1);
  if (received == -1) LogError("Albatross read");
  return received == 1;
}

static int
awaitByte (unsigned char *byte) {
  if (awaitInput(fileDescriptor, 1000))
    if (readByte(byte))
      return 1;
  return 0;
}

static int
writeBytes (unsigned char *bytes, int count) {
  int sent = write(fileDescriptor, bytes, count);
  if (sent == -1) LogError("Albatross write");
  return sent == 1;
}

static int
clearDisplay (void) {
  unsigned char bytes[] = {0XFA};
  return writeBytes(bytes, sizeof(bytes));
}

static int
acknowledgeDisplay (void) {
  unsigned char attributes;
  cellCount = 0;

  if (!awaitByte(&attributes)) return 0;

  {
    unsigned char acknowledgement[] = {0XFE, 0XFF, 0XFE, 0XFF};
    if (!writeBytes(acknowledgement, sizeof(acknowledgement))) return 0;
  }

  if (!clearDisplay()) return 0;
  cellCount = (attributes & 0X80)? 80: 40;
  memset(cellContent, 0, cellCount);

  LogPrint(LOG_INFO, "Albatross detected: %d columns",
           baud2integer(cellCount));
  return 1;
}

static int
updateDisplay (unsigned char *cells) {
  unsigned char bytes[cellCount * 2 + 2];
  unsigned char *byte = bytes;
  int column;
  *byte++ = 0XFB;
  for (column=0; column<cellCount; ++column) {
    unsigned char cell;
    if (!cells) {
      cell = cellContent[column];
    } else if ((cell = outputTable[cells[column]]) == cellContent[column]) {
      continue;
    }
    *byte++ = column + 1;
    *byte++ = cell;
  }
  *byte++ = 0XFC;
  return writeBytes(bytes, byte-bytes);
}

static void
brl_identify (void) {
   LogPrint(LOG_NOTICE, "Albatross Driver");
   LogPrint(LOG_INFO, "   Copyright (C) 2004 by Dave Mielke <dave@mielke.cc>");
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X02, 0X40, 0X20, 0X10, 0X08, 0X04, 0X80, 0X01};
    makeOutputTable(&dots, &outputTable);
  }

  if (openSerialDevice(device, &fileDescriptor, &oldSettings)) {
    speed_t speedTable[] = {B9600, B19200, B0};
    const speed_t *speed = speedTable;

    memset(&newSettings, 0, sizeof(newSettings));
    newSettings.c_cflag = CS8 | CSTOPB | CLOCAL | CREAD;
    newSettings.c_iflag = IGNPAR;

    while (resetSerialDevice(fileDescriptor, &newSettings, *speed)) {
      unsigned char byte;
      while (awaitByte(&byte)) {
        if (byte == 0XFF) {
          if (acknowledgeDisplay()) {
            brl->x = cellCount;
            brl->y = 1;
            return 1;
          }
        }
      }
      if (*++speed == B0) speed = speedTable;
    }

    tcsetattr(fileDescriptor, TCSANOW, &oldSettings);
    close(fileDescriptor);
    fileDescriptor = -1;
  }
  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  tcsetattr(fileDescriptor, TCSADRAIN, &oldSettings);
  close(fileDescriptor);
  fileDescriptor = -1;
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  if (cellCount) {
    updateDisplay(brl->buffer);
  }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  if (cellCount) {
  }
}

static int
brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds) {
  unsigned char byte;
  while (readByte(&byte)) {
    if (byte == 0XFF) {
      if (acknowledgeDisplay()) brl->resizeRequired = 1;
      continue;
    }
    if (!cellCount) continue;

    switch (byte) {
      default:
        if ((byte >= 2) && (byte <= 41)) return CR_ROUTE + (byte - 2);
        if ((byte >= 111) && (byte <= 150)) return CR_ROUTE + (byte - 71);
        break;

      case 0XFB:
        updateDisplay(NULL);
        continue;
    }

    LogPrint(LOG_WARNING, "Unexpected byte: %02X", byte);
  }

  return EOF;
}
