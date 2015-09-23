/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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
#include <fcntl.h>
#include <sys/stat.h>

#include "log.h"

#include "brl_driver.h"
#include "brldefs-bg.h"
#include "metec_flat20_ioctl.h"

#define BRAILLE_DEVICE_PATH "/dev/braille0"
#define TEXT_CELL_COUNT 20

BEGIN_KEY_NAME_TABLE(navigation)
  KEY_NAME_ENTRY(BG_NAV_Dot1, "Dot1"),
  KEY_NAME_ENTRY(BG_NAV_Dot2, "Dot2"),
  KEY_NAME_ENTRY(BG_NAV_Dot3, "Dot3"),
  KEY_NAME_ENTRY(BG_NAV_Dot4, "Dot4"),
  KEY_NAME_ENTRY(BG_NAV_Dot5, "Dot5"),
  KEY_NAME_ENTRY(BG_NAV_Dot6, "Dot6"),
  KEY_NAME_ENTRY(BG_NAV_Dot7, "Dot7"),
  KEY_NAME_ENTRY(BG_NAV_Dot8, "Dot8"),

  KEY_NAME_ENTRY(BG_NAV_Space, "Space"),
  KEY_NAME_ENTRY(BG_NAV_Backward, "Backward"),
  KEY_NAME_ENTRY(BG_NAV_Forward, "Forward"),

  KEY_NAME_ENTRY(BG_NAV_Center, "Center"),
  KEY_NAME_ENTRY(BG_NAV_Left, "Left"),
  KEY_NAME_ENTRY(BG_NAV_Right, "Right"),
  KEY_NAME_ENTRY(BG_NAV_Up, "Up"),
  KEY_NAME_ENTRY(BG_NAV_Down, "Down"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routing)
  KEY_GROUP_ENTRY(BG_GRP_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(navigation),
  KEY_NAME_TABLE(routing),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

struct BrailleDataStruct {
  struct {
    int fileDescriptor;
  } keyboard;

  struct {
    int fileDescriptor;
  } braille;

  struct {
    int rewrite;
    unsigned char cells[TEXT_CELL_COUNT];
  } text;
};

static int
openBrailleDevice (BrailleDisplay *brl) {
  if (brl->data->braille.fileDescriptor == -1) {
    if ((brl->data->braille.fileDescriptor = open(BRAILLE_DEVICE_PATH, O_WRONLY)) == -1) {
      logSystemError("open[braille]");
      return 0;
    }
  }

  return 1;
}

static void
closeBrailleDevice (BrailleDisplay *brl) {
  if (brl->data->braille.fileDescriptor != -1) {
    close(brl->data->braille.fileDescriptor);
    brl->data->braille.fileDescriptor = -1;
  }
}

static int
writeBrailleCells (BrailleDisplay *brl, const unsigned char *cells, size_t count) {
  logOutputPacket(cells, count);
  if (write(brl->data->braille.fileDescriptor, cells, count) != -1) return 1;

  logSystemError("write[braille]");
  return 0;
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  if (connectBrailleResource(brl, "null:", &descriptor, NULL)) {
    return 1;
  }

  return 0;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));
    brl->data->keyboard.fileDescriptor = -1;
    brl->data->braille.fileDescriptor = -1;

    if (openBrailleDevice(brl)) {
      if (connectResource(brl, device)) {
        brl->textColumns = TEXT_CELL_COUNT;

        {
          const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

          brl->keyBindings = ktd->bindings;
          brl->keyNames = ktd->names;
        }

        makeOutputTable(dotsTable_ISO11548_1);
        brl->data->text.rewrite = 1;
        return 1;
      }

      closeBrailleDevice(brl);
    }

    free(brl->data);
  } else {
    logMallocError();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  disconnectBrailleResource(brl, NULL);

  if (brl->data) {
    closeBrailleDevice(brl);

    free(brl->data);
    brl->data = NULL;
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(brl->data->text.cells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->text.rewrite)) {
    unsigned char cells[brl->textColumns];

    translateOutputCells(cells, brl->data->text.cells, brl->textColumns);
    if (!writeBrailleCells(brl, cells, sizeof(cells))) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  return EOF;
}
