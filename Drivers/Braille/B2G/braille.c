/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#include "log.h"

#include "brl_driver.h"
#include "brldefs-bg.h"
#include "metec_flat20_ioctl.h"

#define BRAILLE_CELL_COUNT MAX_BRAILLE_LINE_SIZE

//#define KEYBOARD_DEVICE_NAME "cp430_keypad"

#ifdef KEYBOARD_DEVICE_NAME
#include <dirent.h>
#include <linux/ioctl.h>
#include <linux/input.h>

#include "io_misc.h"
#include "parse.h"

#define BG_SET_VALUE(value) (((value) >> 8) & 0XFF)
#define BG_KEY_VALUE(value) (((value) >> 0) & 0XFF)
#define BG_KEY_ENTRY(n) {.name=#n, .value={.set=BG_SET_VALUE(BG_KEY_##n), .key=BG_KEY_VALUE(BG_KEY_##n)}}

BEGIN_KEY_NAME_TABLE(navigation)
  BG_KEY_ENTRY(Left),
  BG_KEY_ENTRY(Right),
  BG_KEY_ENTRY(Up),
  BG_KEY_ENTRY(Down),
  BG_KEY_ENTRY(Enter),

  BG_KEY_ENTRY(Backward),
  BG_KEY_ENTRY(Forward),

  BG_KEY_ENTRY(Dot1),
  BG_KEY_ENTRY(Dot2),
  BG_KEY_ENTRY(Dot3),
  BG_KEY_ENTRY(Dot4),
  BG_KEY_ENTRY(Dot5),
  BG_KEY_ENTRY(Dot6),
  BG_KEY_ENTRY(Dot7),
  BG_KEY_ENTRY(Dot8),
  BG_KEY_ENTRY(Space),

  KEY_SET_ENTRY(BG_SET_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(navigation),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST
#endif /* KEYBOARD_DEVICE_NAME */

struct BrailleDataStruct {
  int brailleDevice;
  int forceRewrite;
  unsigned char textCells[BRAILLE_CELL_COUNT];

#ifdef KEYBOARD_DEVICE_NAME
  int keyboardDevice;
#endif /* KEYBOARD_DEVICE_NAME */
};

static int
openBrailleDevice (BrailleDisplay *brl, const char *device) {
  if ((brl->data->brailleDevice = open(device, O_RDWR)) != -1) return 1;
  logMessage(LOG_DEBUG, "open error: %s: %s", device, strerror(errno));
  return 0;
}

static void
closeBrailleDevice (BrailleDisplay *brl) {
  if (brl->data->brailleDevice != -1) {
    close(brl->data->brailleDevice);
    brl->data->brailleDevice = -1;
  }
}

static int
logBrailleVersion (BrailleDisplay *brl) {
  char buffer[10];
  memset(buffer, 0, sizeof(buffer));

  if (ioctl(brl->data->brailleDevice, METEC_FLAT20_GET_DRIVER_VERSION, buffer) != -1) {
    logMessage(LOG_INFO, "B2G Driver Version: %s", buffer);
    return 1;
  } else {
    logSystemError("ioctl[METEC_FLAT20_GET_DRIVER_VERSION]");
  }

  return 0;
}

static int
clearBrailleCells (BrailleDisplay *brl) {
  if (ioctl(brl->data->brailleDevice, METEC_FLAT20_CLEAR_DISPLAY, 0) != -1) return 1;
  logSystemError("ioctl[METEC_FLAT20_CLEAR_DISPLAY]");
  return 0;
}

static int
writeBrailleCells (BrailleDisplay *brl, const unsigned char *cells, size_t count) {
  if (write(brl->data->brailleDevice, cells, count) != -1) return 1;
  logSystemError("write");
  return 0;
}

#ifdef KEYBOARD_DEVICE_NAME
static char *
findKeyboardDevice (const char *deviceName) {
  char *devicePath = NULL;

  const char *directoryComponents[] = {
    "/sys/devices/platform/",
    deviceName,
    "/input"
  };
  char *directoryPath = joinStrings(directoryComponents, ARRAY_COUNT(directoryComponents));

  if (directoryPath) {
    DIR *directory = opendir(directoryPath);

    if (directory) {
      const char *filePrefix = "input";
      size_t filePrefixLength = strlen(filePrefix);
      struct dirent *entry;

      while ((entry = readdir(directory))) {
        if (strncmp(entry->d_name, filePrefix, filePrefixLength) == 0) {
          int eventNumber;

          if (isInteger(&eventNumber, &entry->d_name[filePrefixLength])) {
            char path[0X80];

            snprintf(path, sizeof(path), "/dev/input/event%d", eventNumber);
            if (!(devicePath = strdup(path))) logMallocError();
            break;
          }
        }
      }

      closedir(directory);
    } else {
      logMessage(LOG_DEBUG, "keyboard directory open error: %s: %s",
                 directoryPath, strerror(errno));
    }

    free(directoryPath);
  }

  return devicePath;
}

static int
openKeyboardDevice (BrailleDisplay *brl) {
  static const char deviceName[] = KEYBOARD_DEVICE_NAME;
  char *devicePath = findKeyboardDevice(deviceName);

  if (devicePath) {
    int deviceDescriptor = open(devicePath, O_RDONLY);

    if (deviceDescriptor != -1) {
      if (setBlockingIo(deviceDescriptor, 0)) {
        if (ioctl(deviceDescriptor, EVIOCGRAB, 1) != -1) {
          logMessage(LOG_DEBUG, "Keyboard Device Opened: %s: %s: fd=%d",
                     deviceName, devicePath, deviceDescriptor);
          free(devicePath);

          brl->data->keyboardDevice = deviceDescriptor;
          return 1;
        } else {
          logSystemError("ioctl[EVIOCGRAB]");
        }
      }

      close(deviceDescriptor);
    } else {
      logMessage(LOG_DEBUG, "keyboard device open error: %s: %s",
                 devicePath, strerror(errno));
    }

    free(devicePath);
  }

  return 0;
}

static void
closeKeyboardDevice (BrailleDisplay *brl) {
  if (brl->data->keyboardDevice != -1) {
    close(brl->data->keyboardDevice);
    brl->data->keyboardDevice = -1;
  }
}
#endif /* KEYBOARD_DEVICE_NAME */

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));
    brl->data->brailleDevice = -1;

#ifdef KEYBOARD_DEVICE_NAME
    brl->data->keyboardDevice = -1;
#endif /* KEYBOARD_DEVICE_NAME */

    if (openBrailleDevice(brl, device)) {
      logBrailleVersion(brl);

#ifdef KEYBOARD_DEVICE_NAME
      if (openKeyboardDevice(brl)) {
#endif /* KEYBOARD_DEVICE_NAME */
        if (clearBrailleCells(brl)) {
#ifdef KEYBOARD_DEVICE_NAME
          {
            const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

            brl->keyBindings = ktd->bindings;
            brl->keyNameTables = ktd->names;
          }
#endif /* KEYBOARD_DEVICE_NAME */

          brl->textColumns = BRAILLE_CELL_COUNT;
          makeOutputTable(dotsTable_ISO11548_1);
          brl->data->forceRewrite = 1;
          return 1;
        }

#ifdef KEYBOARD_DEVICE_NAME
        closeKeyboardDevice(brl);
      }
#endif /* KEYBOARD_DEVICE_NAME */

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
  if (brl->data) {
    closeBrailleDevice(brl);

#ifdef KEYBOARD_DEVICE_NAME
    closeKeyboardDevice(brl);
#endif /* KEYBOARD_DEVICE_NAME */

    free(brl->data);
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(brl->data->textCells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->forceRewrite)) {
    unsigned char cells[brl->textColumns];

    translateOutputCells(cells, brl->data->textCells, brl->textColumns);
    if (!writeBrailleCells(brl, cells, brl->textColumns)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
#ifdef KEYBOARD_DEVICE_NAME
  struct input_event event;
  ssize_t length;

  while ((length = read(brl->data->keyboardDevice, &event, sizeof(event))) != -1) {
    if (length < sizeof(event)) continue;
    if (event.type != EV_KEY) continue;

    {
      unsigned char set;
      unsigned char key;
      int press;

      if ((event.code >= BG_KEY_ROUTE) && (event.code < (BG_KEY_ROUTE + brl->textColumns))) {
        set = BG_SET_RoutingKeys;
        key = event.code - BG_KEY_ROUTE;
      } else {
        set = BG_SET_VALUE(event.code);
        key = BG_KEY_VALUE(event.code);
      }

      switch (event.value) {
        case 0:
          press = 0;
          break;

        case 1:
          press = 1;
          break;

        default:
          continue;
      }

      if ((event.code >= BG_KEY_ROUTE) && (event.code < (BG_KEY_ROUTE + brl->textColumns))) {
        set = 2;
        key = event.code - BG_KEY_ROUTE;
      } else {
        set = BG_SET_VALUE(event.code);
        key = BG_KEY_VALUE(event.code);
      }

      enqueueKeyEvent(brl, set, key, press);
    }
  }

  if (errno != EAGAIN) {
    logSystemError("keyboard input error");
    return BRL_CMD_RESTARTBRL;
  }
#endif /* KEYBOARD_DEVICE_NAME */

  return EOF;
}
