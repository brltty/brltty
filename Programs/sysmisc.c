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
  const char *driver,
  const char *driverDirectory, const char *driverSymbol,
  const char *driverType, char driverCharacter,
  const DriverEntry *driverTable,
  const void *nullAddress, const char *nullIdentifier
) {
  const void *driverAddress = NULL;

  if (!driver) {
    if (driverTable)
      if (driverTable->address)
        return driverTable->address;
    return nullAddress;
  }

  if (strcmp(driver, nullIdentifier) == 0) {
    return nullAddress;
  }

  if (driverTable) {
    const DriverEntry *driverEntry = driverTable;
    while (driverEntry->address) {
      if (strcmp(driver, *driverEntry->identifier) == 0) {
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
      int length = strlen(LIBRARY_NAME) + strlen(driver) + strlen(LIBRARY_EXTENSION) + 3;
      char *name = mallocWrapper(length);
      snprintf(name, length, "%s%c%s.%s",
               LIBRARY_NAME, driverCharacter, driver, LIBRARY_EXTENSION);
      libraryName = name;
    }
    libraryPath = makePath(driverDirectory, libraryName);

    {
      int length = strlen(driverSymbol) + strlen(driver) + 2;
      char *name = mallocWrapper(length);
      snprintf(name, length, "%s_%s",
               driverSymbol, driver);
      symbolName = name;
    }

    {
      void *libraryHandle = loadSharedObject(libraryPath);
      if (libraryHandle) {
        if (!findSharedSymbol(libraryHandle, symbolName, &driverAddress)) {
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
