/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "system.h"
#include "sysmisc.h"

const void *
loadDriver (
  const char **driver,
  const char *driverDirectory, const char *driverSymbol,
  const char *driverType, char driverCharacter,
  const void *builtinAddress, const char *builtinIdentifier,
  const void *nullAddress, const char *nullIdentifier
) {
  const void *driverAddress = NULL;

  if (*driver)
    if (strcmp(*driver, nullIdentifier) == 0) {
      *driver = NULL;
      return nullAddress;
    }

  if (builtinAddress) {
    if (*driver)
      if (strcmp(*driver, builtinIdentifier) == 0)
        *driver = NULL;
    if (!*driver)
      return builtinAddress;
  } else if (!*driver)
    return nullAddress;

  {
    const char *libraryName = *driver;

    /* allow shortcuts */
    if (strlen(libraryName) == 2) {
      int nameLength = strlen(LIBRARY_NAME) + strlen(libraryName) + strlen(LIBRARY_EXTENSION) + 3;
      char *name = mallocWrapper(nameLength);
      snprintf(name, nameLength, "%s%c%s.%s",
               LIBRARY_NAME, driverCharacter, libraryName, LIBRARY_EXTENSION);
      libraryName = name;
    }

    {
      const char *libraryPath;
      if (driverDirectory && (libraryName[0] != '/')) {
        int pathLength = strlen(driverDirectory) + strlen(libraryName) + 2;
        char *path = mallocWrapper(pathLength);
        snprintf(path, pathLength, "%s/%s", driverDirectory, libraryName);
        libraryPath = path;
      } else {
        libraryPath = libraryName;
      }

      {
        void *libraryHandle = loadSharedObject(libraryPath);
        if (libraryHandle) {
          if (!findSharedSymbol(libraryHandle, driverSymbol, &driverAddress)) {
            LogPrint(LOG_ERR, "Cannot find %s driver symbol: %s", driverType, driverSymbol);
            unloadSharedObject(libraryHandle);
            driverAddress = NULL;
          }
        } else {
          LogPrint(LOG_ERR, "Cannot load %s driver: %s", driverType, libraryPath);
        }
      }

      if (libraryPath != libraryName) free((void *)libraryPath);
    }

    if (libraryName != *driver) free((void *)libraryName);
  }

  return driverAddress;
}
