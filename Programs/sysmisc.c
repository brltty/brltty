/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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
  const char *driverIdentifier, void **driverObject,
  const char *driverDirectory, const DriverEntry *driverTable,
  const char *driverType, char driverCharacter, const char *driverSymbol,
  const void *nullAddress, const char *nullIdentifier
) {
  const void *driverAddress = NULL;
  *driverObject = NULL;

  if (!driverIdentifier || !*driverIdentifier) {
    if (driverTable)
      if (driverTable->address)
        return driverTable->address;
    return nullAddress;
  }

  if (strcmp(driverIdentifier, nullIdentifier) == 0) return nullAddress;

  if (driverTable) {
    const DriverEntry *driverEntry = driverTable;
    while (driverEntry->address) {
      if (strcmp(driverIdentifier, *driverEntry->identifier) == 0) {
        return driverEntry->address;
      }
      ++driverEntry;
    }
  }

  {
    const char *libraryName;
    const char *libraryPath;
    const char *symbolName;

    {
      int length = strlen(LIBRARY_NAME) + strlen(driverIdentifier) + strlen(LIBRARY_EXTENSION) + 3;
      char *name = mallocWrapper(length);
      snprintf(name, length, "%s%c%s.%s",
               LIBRARY_NAME, driverCharacter, driverIdentifier, LIBRARY_EXTENSION);
      libraryName = name;
    }
    libraryPath = makePath(driverDirectory, libraryName);

    {
      int length = strlen(driverSymbol) + strlen(driverIdentifier) + 2;
      char *name = mallocWrapper(length);
      snprintf(name, length, "%s_%s",
               driverSymbol, driverIdentifier);
      symbolName = name;
    }

    {
      void *libraryHandle = loadSharedObject(libraryPath);
      if (libraryHandle) {
        if (findSharedSymbol(libraryHandle, symbolName, &driverAddress)) {
          *driverObject = libraryHandle;
        } else {
          LogPrint(LOG_ERR, "Cannot find %s driver symbol: %s", driverType, symbolName);
          unloadSharedObject(libraryHandle);
          driverAddress = NULL;
        }
      } else {
        LogPrint(LOG_ERR, "Cannot load %s driver: %s", driverType, libraryPath);
      }
    }

    free((void *)symbolName);
    free((void *)libraryPath);
    free((void *)libraryName);
  }

  return driverAddress;
}
