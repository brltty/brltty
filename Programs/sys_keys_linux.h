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

#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>
#endif /* HAVE_LINUX_INPUT_H */

#include "async.h"

static int
hasEvent (int device, uint16_t type, uint16_t code, uint16_t max) {
  BITMASK(mask, max+1, long);

  if (ioctl(device, EVIOCGBIT(type, max), mask) != -1)
    if (BITMASK_TEST(mask, code))
      return 1;

  return 0;
}

static int
getKeyboardDevice (void) {
  static int keyboardDevice = -1;

#ifdef HAVE_LINUX_INPUT_H
  if (keyboardDevice == -1) {
    const char root[] = "/dev/input";
    const size_t rootLength = strlen(root);
    DIR *directory;

    if ((directory = opendir(root))) {
      struct dirent *entry;

      while ((entry = readdir(directory))) {
        const size_t nameLength = strlen(entry->d_name);
        char path[rootLength + 1 + nameLength + 1];
        snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);

        {
          struct stat status;
          if (stat(path, &status) == -1) continue;
          if (!S_ISCHR(status.st_mode)) continue;
        }

        {
          int device;

          LogPrint(LOG_DEBUG, "testing device: %s", path);
          if ((device = open(path, O_RDONLY)) != -1) {
            struct input_id id;
            
            if (ioctl(device, EVIOCGID, &id) != -1) {
              LogPrint(LOG_DEBUG, "checking device: %s type=%04X vendor=%04X product=%04X version=%04X",
                       path, id.bustype, id.vendor, id.product, id.version);

              if (hasEvent(device, EV_KEY, KEY_ENTER, KEY_MAX)) {
                LogPrint(LOG_DEBUG, "keyboard opened: %s fd=%d", path, device);
                keyboardDevice = device;
                break;
              }
            }

            close(device);
          } else {
            LogPrint(LOG_DEBUG, "cannot open device: %s: %s", path, strerror(errno));
          }
        }
      }

      closedir(directory);
    } else {
      LogPrint(LOG_DEBUG, "cannot open directory: %s", root);
    }
  }
#endif /* HAVE_LINUX_INPUT_H */

  return keyboardDevice;
}

static size_t
handleKeyEvent (const AsyncInputResult *result) {
  if (result->error) {
    LogPrint(LOG_DEBUG, "keyboard read error: %s", strerror(result->error));
  } else if (result->end) {
    LogPrint(LOG_DEBUG, "keyboard end-of-file");
  } else {
#ifdef HAVE_LINUX_INPUT_H
    const struct input_event *event = result->buffer;

    if (result->length >= sizeof(*event)) {
      if (event->type == EV_KEY) {
        writeKeyEvent(event->code, event->value);
      } else {
        writeInputEvent(event);
      }

      return sizeof(*event);
    }
#else /* HAVE_LINUX_INPUT_H */
    return result->length;
#endif /* HAVE_LINUX_INPUT_H */
  }

  return 0;
}

int
monitorKeyEvents (void) {
  int uinput = getUinputDevice();

  if (uinput != -1) {
    int keyboard = getKeyboardDevice();

    if (keyboard != -1) {
      if (asyncRead(keyboard, sizeof(struct input_event), handleKeyEvent, NULL)) {
#ifdef EVIOCGRAB
        ioctl(keyboard, EVIOCGRAB, 1);
#endif /* EVIOCGRAB */

        return 1;
      }
    }
  }

  return 0;
}
