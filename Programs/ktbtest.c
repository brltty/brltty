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

#include <stdio.h>
#include <string.h>

#include "program.h"
#include "options.h"
#include "log.h"
#include "file.h"
#include "parse.h"
#include "system.h"
#include "ktb.h"
#include "ktb_keyboard.h"
#include "brl.h"

static char *opt_driversDirectory;
static char *opt_tablesDirectory;
static int opt_listKeyNames;
static int opt_listKeyTable;

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'k',
    .word = "keys",
    .flags = OPT_Config | OPT_Environ,
    .setting.flag = &opt_listKeyNames,
    .description = strtext("List key names on standard output.")
  },

  { .letter = 'l',
    .word = "list",
    .flags = OPT_Config | OPT_Environ,
    .setting.flag = &opt_listKeyTable,
    .description = strtext("List key table on standard output.")
  },

  { .letter = 'D',
    .word = "drivers-directory",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("directory"),
    .setting.string = &opt_driversDirectory,
    .defaultSetting = DRIVERS_DIRECTORY,
    .description = strtext("Path to directory for loading drivers.")
  },

  { .letter = 'T',
    .word = "tables-directory",
    .flags = OPT_Hidden,
    .argument = strtext("directory"),
    .setting.string = &opt_tablesDirectory,
    .defaultSetting = TABLES_DIRECTORY,
    .description = strtext("Path to directory containing tables.")
  },
END_OPTION_TABLE

static int
listLine (const wchar_t *line, void *data) {
  FILE *stream = stdout;

  fprintf(stream, "%" PRIws "\n", line);
  return !ferror(stream);
}

static KEY_NAME_TABLES_REFERENCE
getKeyNameTables (const char *keyTableName) {
  KEY_NAME_TABLES_REFERENCE keyNameTables = NULL;
  int componentsLeft;
  char **nameComponents = splitString(keyTableName, '-', &componentsLeft);

  if (nameComponents) {
    char **currentComponent = nameComponents;

    if (componentsLeft) {
      const char *keyTableType = (componentsLeft--, *currentComponent++);

      if (strcmp(keyTableType, "kbd") == 0) {
        if (componentsLeft) {
          componentsLeft--; currentComponent++;
          keyNameTables = KEY_NAME_TABLES(keyboard);
        } else {
          logMessage(LOG_ERR, "missing keyboard bindings name");
        }
      } else if (strcmp(keyTableType, "brl") == 0) {
        if (componentsLeft) {
          void *driverObject;
          const char *driverCode = (componentsLeft--, *currentComponent++);

          if (loadBrailleDriver(driverCode, &driverObject, opt_driversDirectory)) {
            char *keyTablesSymbol;

            {
              const char *strings[] = {"brl_ktb_", driverCode};
              keyTablesSymbol = joinStrings(strings, ARRAY_COUNT(strings));
            }

            if (keyTablesSymbol) {
              const KeyTableDefinition *const *keyTableDefinitions;

              if (findSharedSymbol(driverObject, keyTablesSymbol, &keyTableDefinitions)) {
                const KeyTableDefinition *const *currentDefinition = keyTableDefinitions;

                if (componentsLeft) {
                  const char *bindingsName = (componentsLeft--, *currentComponent++);

                  while (*currentDefinition) {
                    if (strcmp(bindingsName, (*currentDefinition)->bindings) == 0) {
                      keyNameTables = (*currentDefinition)->names;
                      break;
                    }

                    currentDefinition += 1;
                  }

                  if (!keyNameTables) {
                    logMessage(LOG_ERR, "unknown %s braille driver bindings name: %s",
                               driverCode, bindingsName);
                  }
                } else {
                  logMessage(LOG_ERR, "missing braille driver bindings name");
                }
              }

              free(keyTablesSymbol);
            } else {
              logMallocError();
            }
          }
        } else {
          logMessage(LOG_ERR, "missing braille driver code");
        }
      } else {
        logMessage(LOG_ERR, "unknown key table type: %s", keyTableType);
      }
    } else {
      logMessage(LOG_ERR, "missing key table type");
    }
  }

  if (keyNameTables) {
    if (componentsLeft) {
      logMessage(LOG_ERR, "too many key table name components");
      keyNameTables = NULL;
    }
  }

  deallocateStrings(nameComponents);
  return keyNameTables;
}

int
main (int argc, char *argv[]) {
  ProgramExitStatus exitStatus = PROG_EXIT_SUCCESS;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "ktbtest",
      .argumentsSummary = "key-table"
    };
    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  {
    char **const paths[] = {
      &opt_driversDirectory,
      &opt_tablesDirectory,
      NULL
    };
    fixInstallPaths(paths);
  }

  if (argc) {
    const char *keyTableName = (argc--, *argv++);
    char *keyTablePath = makeKeyTablePath(opt_tablesDirectory, keyTableName);

    if (keyTablePath) {
      KEY_NAME_TABLES_REFERENCE keyNameTables;

      {
        const char *file = locatePathName(keyTablePath);
        size_t length = strrchr(file, '.') - file;
        char name[length + 1];

        memcpy(name, file, length);
        name[length] = 0;

        keyNameTables = getKeyNameTables(name);
      }

      if (keyNameTables) {
        if (opt_listKeyNames)
          if (!listKeyNames(keyNameTables, listLine, NULL))
            exitStatus = PROG_EXIT_FATAL;

        if (exitStatus == PROG_EXIT_SUCCESS) {
          KeyTable *keyTable = compileKeyTable(keyTablePath, keyNameTables);

          if (keyTable) {
            if (opt_listKeyTable)
              if (!listKeyTable(keyTable, listLine, NULL))
                exitStatus = PROG_EXIT_FATAL;

            destroyKeyTable(keyTable);
          } else {
            exitStatus = PROG_EXIT_FATAL;
          }
        }
      } else {
        exitStatus = PROG_EXIT_FATAL;
      }

      free(keyTablePath);
    } else {
      exitStatus = PROG_EXIT_FATAL;
    }
  } else {
    logMessage(LOG_ERR, "missing key table name");
    exitStatus = PROG_EXIT_SYNTAX;
  }

  return exitStatus;
}

KeyTableState
processKeyEvent (KeyTable *table, unsigned char context, unsigned char set, unsigned char key, int press) {
  return KTS_UNBOUND;
}

#include "brltty.h"

unsigned int textStart;
unsigned int textCount;
int apiStarted = 0;

unsigned char
getCursorDots (void) {
  return 0;
}

int
api_handleCommand (int command) {
  return command;
}

int
api_handleKeyEvent (unsigned char set, unsigned char key, int press) {
  return EOF;
}

#include "message.h"

int
message (const char *mode, const char *text, short flags) {
  return 1;
}

#include "scr.h"

int
currentVirtualTerminal (void) {
  return 0;
}
