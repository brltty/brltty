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

#include "prologue.h"

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
    if (strcmp(code, table->definition->code) == 0) return 1;
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
  return (table && table[0].address && !table[1].address)? table[0].definition->code: NULL;
}

static int
isDriverCode (const char *code, const DriverDefinition *definition) {
  if (strcmp(code, definition->code) == 0) return 1;
  return 0;
}

const void *
loadDriver (
  const char *driverCode, void **driverObject,
  const char *driverDirectory, const DriverEntry *driverTable,
  const char *driverType, char driverCharacter, const char *driverSymbol,
  const void *nullAddress, const DriverDefinition *nullDefinition
) {
  const void *driverAddress = NULL;
  *driverObject = NULL;

  if (!driverCode || !*driverCode) {
    if (driverTable)
      if (driverTable->address)
        return driverTable->address;
    return nullAddress;
  }

  if (isDriverCode(driverCode, nullDefinition)) return nullAddress;

  if (driverTable) {
    const DriverEntry *driverEntry = driverTable;
    while (driverEntry->address) {
      if (isDriverCode(driverCode, driverEntry->definition)) return driverEntry->address;
      ++driverEntry;
    }
  }

  {
    char *libraryPath;
    const int libraryNameLength = strlen(MODULE_NAME) + strlen(driverCode) + strlen(MODULE_EXTENSION) + 3;
    char libraryName[libraryNameLength];
    snprintf(libraryName, libraryNameLength, "%s%c%s.%s",
             MODULE_NAME, driverCharacter, driverCode, MODULE_EXTENSION);

    if ((libraryPath = makePath(driverDirectory, libraryName))) {
      void *libraryHandle = loadSharedObject(libraryPath);

      if (libraryHandle) {
        const int symbolNameLength = strlen(driverSymbol) + strlen(driverCode) + 2;
        char symbolName[symbolNameLength];
        snprintf(symbolName, symbolNameLength, "%s_%s",
                 driverSymbol, driverCode);

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

      free(libraryPath);
    }
  }

  return driverAddress;
}

void
identifyDriver (
  const char *type,
  const DriverDefinition *definition,
  int full
) {
  {
    char buffer[0X100];
    int length;
    snprintf(buffer, sizeof(buffer), "%s %s Driver:%n", 
             definition->name, type, &length);

    if (definition->version && *definition->version) {
      int count;
      snprintf(&buffer[length], sizeof(buffer)-length, " version %s%n",
               definition->version, &count);
      length += count;
    }

    if (full) {
      int count;
      snprintf(&buffer[length], sizeof(buffer)-length, " [compiled on %s at %s]%n",
               definition->date, definition->time, &count);
      length += count;
    }

    LogPrint(LOG_NOTICE, "%s", buffer);
  }

  if (full) {
    if (definition->developers && *definition->developers)
      LogPrint(LOG_INFO, "   Developed by %s", definition->developers);
  }
}
