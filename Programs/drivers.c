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

#include "prologue.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "system.h"
#include "drivers.h"

int
isDriverAvailable (const char *code, const char *codes) {
  int length = strlen(code);
  const char *string;

  while ((string = strstr(codes, code))) {
    if (((string == codes) || (string[-1] == ' ')) &&
        (!string[length] || (string[length] == ' '))) {
      return 1;
    }

    string += length;
  }

  return 0;
}

int
isDriverIncluded (const char *code, const DriverEntry *table) {
  while (table->address) {
    if (strcmp(code, *table->code) == 0) return 1;
    ++table;
  }
  return 0;
}

int
haveDriver (const char *code, const char *codes, const DriverEntry *table) {
  return (table && table->address)? isDriverIncluded(code, table):
                                    isDriverAvailable(code, codes);
}

const char *
getDefaultDriver (const DriverEntry *table) {
  return (table && table[0].address && !table[1].address)? *table[0].code: NULL;
}

const void *
loadDriver (
  const char *driverCode, void **driverObject,
  const char *driverDirectory, const DriverEntry *driverTable,
  const char *driverType, char driverCharacter, const char *driverSymbol,
  const void *nullAddress, const char *nullCode
) {
  const void *driverAddress = NULL;
  *driverObject = NULL;

  if (!driverCode || !*driverCode) {
    if (driverTable)
      if (driverTable->address)
        return driverTable->address;
    return nullAddress;
  }

  if (strcmp(driverCode, nullCode) == 0) return nullAddress;

  if (driverTable) {
    const DriverEntry *driverEntry = driverTable;
    while (driverEntry->address) {
      if (strcmp(driverCode, *driverEntry->code) == 0) {
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
      int length = strlen(MODULE_NAME) + strlen(driverCode) + strlen(MODULE_EXTENSION) + 3;
      char *name = mallocWrapper(length);
      snprintf(name, length, "%s%c%s.%s",
               MODULE_NAME, driverCharacter, driverCode, MODULE_EXTENSION);
      libraryName = name;
    }
    libraryPath = makePath(driverDirectory, libraryName);

    {
      int length = strlen(driverSymbol) + strlen(driverCode) + 2;
      char *name = mallocWrapper(length);
      snprintf(name, length, "%s_%s",
               driverSymbol, driverCode);
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
