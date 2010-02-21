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

#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>

#include "device.h"
#include "log.h"
#include "misc.h"

int
getConsole (void) {
  static int console = -1;
  if (console == -1) {
    if ((console = open("/dev/console", O_WRONLY)) != -1) {
      LogPrint(LOG_DEBUG, "Console opened: fd=%d", console);
    } else {
      LogError("Console open");
    }
  }
  return console;
}

int
writeConsole (const unsigned char *address, size_t count) {
  int console = getConsole();
  if (console != -1) {
    if (write(console, address, count) != -1) {
      return 1;
    } else {
      LogError("Console write");
    }
  }
  return 0;
}

int
ringBell (void) {
  static unsigned char bellSequence[] = {0X07};
  return writeConsole(bellSequence, sizeof(bellSequence));
}

const char *
getDeviceDirectory (void) {
  static const char *deviceDirectory = NULL;

  if (!deviceDirectory) {
    const char *directory = DEVICE_DIRECTORY;
    const unsigned int directoryLength = strlen(directory);

    static const char *const variables[] = {"DTDEVROOT", "UTDEVROOT", NULL};
    const char *const*variable = variables;

    while (*variable) {
      const char *root = getenv(*variable);

      if (root && *root) {
        const unsigned int rootLength = strlen(root);
        char path[rootLength + directoryLength + 1];
        snprintf(path, sizeof(path), "%s%s", root, directory);
        if (testPath(path)) {
          deviceDirectory = strdupWrapper(path);
          goto found;
        }

        if (errno != ENOENT)
          LogPrint(LOG_ERR, "device directory error: %s (%s): %s",
                   path, *variable, strerror(errno));
      }

      ++variable;
    }

    deviceDirectory = directory;
  found:
    LogPrint(LOG_DEBUG, "device directory: %s", deviceDirectory);
  }

  return deviceDirectory;
}

char *
getDevicePath (const char *device) {
  const char *directory = getDeviceDirectory();
  const unsigned int directoryLength = strlen(directory);
  const unsigned int deviceLength = strlen(device);

  if (directoryLength) {
    if (*device != '/') {
      char path[directoryLength + 1 +  deviceLength + 1];
      unsigned int length = 0;

      memcpy(&path[length], directory, directoryLength);
      length += directoryLength;

      if (path[length-1] != '/') path[length++] = '/';

      memcpy(&path[length], device, deviceLength);
      length += deviceLength;

      path[length] = 0;
      return strdupWrapper(path);
    }
  }
  return strdupWrapper(device);
}

const char *
resolveDeviceName (const char *const *names, const char *description, int mode) {
  const char *first = *names;
  const char *device = NULL;
  const char *name;

  while ((name = *names++)) {
    char *path = getDevicePath(name);

    if (!path) break;
    LogPrint(LOG_DEBUG, "checking %s device: %s", description, path);

    if (access(path, mode) != -1) {
      device = name;
      free(path);
      break;
    }

    LogPrint(LOG_DEBUG, "%s device access error: %s: %s",
             description, path, strerror(errno));
    if (errno != ENOENT)
      if (!device)
        device = name;
    free(path);
  }

  if (!device) {
    if (first) {
      device = first;
    } else {
      LogPrint(LOG_ERR, "%s device names not defined", description);
    }
  }

  if (device) LogPrint(LOG_INFO, "%s device: %s", description, device);
  return device;
}

int
isQualifiedDevice (const char **path, const char *qualifier) {
  size_t count = strcspn(*path, ":");
  if (count == strlen(*path)) return 0;
  if (!qualifier) return 1;
  if (!count) return 0;

  {
    int ok = strncasecmp(*path, qualifier, count) == 0;
    if (ok) *path += count + 1;
    return ok;
  }
}

#ifdef ALLOW_DOS_DEVICE_NAMES
int
isDosDevice (const char *path, const char *prefix) {
  size_t count = strcspn(path, ":");
  size_t length = strlen(prefix);
  if (length > count) return 0;
  if (strncasecmp(path, prefix, length) != 0) return 0;
  if (strspn(path+length, "0123456789") != (count - length)) return 0;
  return 1;
}
#endif /* ALLOW_DOS_DEVICE_NAMES */
