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

#include <stdio.h>
#include <string.h>

#include "program.h"
#include "options.h"
#include "log.h"
#include "system.h"
#include "ktb.h"
#include "kbdkeys.h"
#include "brl.h"

static char *opt_libraryDirectory;
static char *opt_tablesDirectory;
static int opt_listKeyTable;

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'l',
    .word = "list",
    .flags = OPT_Config | OPT_Environ,
    .setting.flag = &opt_listKeyTable,
    .description = strtext("List key table on standard output.")
  },

  { .letter = 'L',
    .word = "library-directory",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("directory"),
    .setting.string = &opt_libraryDirectory,
    .defaultSetting = LIBRARY_DIRECTORY,
    .description = strtext("Path to directory for loading drivers.")
  },

  { .letter = 'T',
    .word = "tables-directory",
    .flags = OPT_Hidden,
    .argument = strtext("directory"),
    .setting.string = &opt_tablesDirectory,
    .defaultSetting = DATA_DIRECTORY,
    .description = strtext("Path to directory containing tables.")
  },
END_OPTION_TABLE

static int
listKeyTableLine (const wchar_t *line, void *data) {
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
          LogPrint(LOG_ERR, "missing keyboard bindings name");
        }
      } else if (strcmp(keyTableType, "brl") == 0) {
        if (componentsLeft) {
          void *driverObject;
          const char *driverCode = (componentsLeft--, *currentComponent++);

          if (loadBrailleDriver(driverCode, &driverObject, opt_libraryDirectory)) {
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
                    LogPrint(LOG_ERR, "unknown %s braille driver bindings name: %s",
                             driverCode, bindingsName);
                  }
                } else {
                  LogPrint(LOG_ERR, "missing braille driver bindings name");
                }
              }

              free(keyTablesSymbol);
            } else {
              LogError("malloc");
            }
          }
        } else {
          LogPrint(LOG_ERR, "missing braille driver code");
        }
      } else {
        LogPrint(LOG_ERR, "unknown key table type: %s", keyTableType);
      }
    } else {
      LogPrint(LOG_ERR, "missing key table type");
    }
  }

  if (keyNameTables) {
    if (componentsLeft) {
      LogPrint(LOG_ERR, "too many key table name components");
      keyNameTables = NULL;
    }
  }

  deallocateStrings(nameComponents);
  return keyNameTables;
}

int
main (int argc, char *argv[]) {
  int status;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "ktbtest",
      .argumentsSummary = "key-table"
    };
    processOptions(&descriptor, &argc, &argv);
  }

  {
    char **const paths[] = {
      &opt_libraryDirectory,
      &opt_tablesDirectory,
      NULL
    };
    fixInstallPaths(paths);
  }

  if (argc) {
    const char *keyTableName = (argc--, *argv++);
    char *keyTableFile = ensureKeyTableExtension(keyTableName);

    if (keyTableFile) {
      KEY_NAME_TABLES_REFERENCE keyNameTables;

      {
        size_t length = strrchr(keyTableFile, '.') - keyTableFile;
        char name[length + 1];

        memcpy(name, keyTableFile, length);
        name[length] = 0;

        keyNameTables = getKeyNameTables(name);
      }

      if (keyNameTables) {
        char *keyTablePath = makePath(opt_tablesDirectory, keyTableFile);

        if (keyTablePath) {
          KeyTable *keyTable = compileKeyTable(keyTablePath, keyNameTables);

          if (keyTable) {
            status = 0;

            if (opt_listKeyTable)
              if (!listKeyTable(keyTable, listKeyTableLine, NULL))
                status = 5;

            destroyKeyTable(keyTable);
          } else {
            status = 4;
          }

          free(keyTablePath);
        } else {
          status = 10;
        }
      } else {
        status = 3;
      }

      free(keyTableFile);
    } else {
      status = 10;
    }
  } else {
    LogPrint(LOG_ERR, "missing key table name");
    status = 2;
  }

  return status;
}

unsigned int textStart;
unsigned int textCount;
int apiStarted = 0;

int
api_handleCommand (int command) {
  return command;
}

int
api_handleKeyEvent (unsigned char set, unsigned char key, int press) {
  return EOF;
}

int
message (const char *mode, const char *string, short flags) {
  return 1;
}
