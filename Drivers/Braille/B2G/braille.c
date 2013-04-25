/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>

#include "log.h"

#include "brl_driver.h"
#include "metec_flat20_ioctl.h"

#define DEVICE_PATH "/dev/braille0"
#define CELL_COUNT MAX_BRAILLE_LINE_SIZE

BEGIN_KEY_NAME_TABLE(navigation)
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(navigation),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

struct BrailleDataStruct {
  int fileDescriptor;
  int forceRewrite;
  unsigned char textCells[CELL_COUNT];
};

static int
openDevice (BrailleDisplay *brl, const char *device) {
  if ((brl->data->fileDescriptor = open(DEVICE_PATH, O_RDWR)) != -1) return 1;
  logMessage(LOG_DEBUG, "open error: %s: %s", DEVICE_PATH, strerror(errno));
  return 0;
}

static void
closeDevice (BrailleDisplay *brl) {
  if (brl->data->fileDescriptor != -1) {
    close(brl->data->fileDescriptor);
    brl->data->fileDescriptor = -1;
  }
}

static int
logVersion (BrailleDisplay *brl) {
  char buffer[10];

  if (ioctl(brl->data->fileDescriptor, METEC_FLAT20_GET_DRIVER_VERSION, buffer) != -1) {
    logMessage(LOG_INFO, "B2G Driver Version: %s", buffer);
    return 1;
  } else {
    logSystemError("ioctl[METEC_FLAT20_GET_DRIVER_VERSION]");
  }

  return 0;
}

static int
clearCells (BrailleDisplay *brl) {
  if (ioctl(brl->data->fileDescriptor, METEC_FLAT20_CLEAR_DISPLAY, 0) != -1) return 1;
  logSystemError("ioctl[METEC_FLAT20_CLEAR_DISPLAY]");
  return 0;
}

static int
writeCells (BrailleDisplay *brl, const unsigned char *cells, size_t count) {
  if (write(brl->data->fileDescriptor, cells, count) != -1) return 1;
  logSystemError("write");
  return 0;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));
    brl->data->fileDescriptor = -1;

    if (openDevice(brl, device)) {
      logVersion(brl);

      if (clearCells(brl)) {
        {
          const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

          brl->keyBindings = ktd->bindings;
          brl->keyNameTables = ktd->names;
        }

        brl->textColumns = CELL_COUNT;
        makeOutputTable(dotsTable_ISO11548_1);
        brl->data->forceRewrite = 1;
        return 1;
      }

      closeDevice(brl);
    }

    free(brl->data);
  } else {
    logMallocError();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  if (brl->data) {
    closeDevice(brl);
    free(brl->data);
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(brl->data->textCells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->forceRewrite)) {
    unsigned char cells[brl->textColumns];

    translateOutputCells(cells, brl->data->textCells, brl->textColumns);
    if (!writeCells(brl, cells, brl->textColumns)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  return EOF;
}
