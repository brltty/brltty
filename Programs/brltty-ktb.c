/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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
#include "dynld.h"
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

static void *driverObject;

static int
listLine (const wchar_t *line, void *data) {
  FILE *stream = stdout;

  fprintf(stream, "%" PRIws "\n", line);
  return !ferror(stream);
}

typedef struct {
  KEY_NAME_TABLES_REFERENCE names;
  char *path;
} KeyTableDescriptor;

static int
getKeyTableDescriptor (KeyTableDescriptor *ktd, const char *name) {
  int ok = 0;
  int componentsLeft;
  char **nameComponents = splitString(name, '-', &componentsLeft);

  memset(ktd, 0, sizeof(*ktd));
  ktd->names = NULL;
  ktd->path = NULL;

  if (nameComponents) {
    char **currentComponent = nameComponents;

    if (componentsLeft) {
      const char *tableType = (componentsLeft--, *currentComponent++);

      if (strcmp(tableType, "kbd") == 0) {
        if (componentsLeft) {
          const char *keyboardType = (componentsLeft--, *currentComponent++);

          ktd->names = KEY_NAME_TABLES(keyboard);
          if ((ktd->path = makeKeyboardTablePath(opt_tablesDirectory, keyboardType))) ok = 1;
        } else {
          logMessage(LOG_ERR, "missing keyboard type");
        }
      } else if (strcmp(tableType, "brl") == 0) {
        if (componentsLeft) {
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
                  const char *deviceType = (componentsLeft--, *currentComponent++);

                  while (*currentDefinition) {
                    if (strcmp(deviceType, (*currentDefinition)->bindings) == 0) {
                      ktd->names = (*currentDefinition)->names;
                      if ((ktd->path = makeInputTablePath(opt_tablesDirectory, driverCode, deviceType))) ok = 1;
                      break;
                    }

                    currentDefinition += 1;
                  }

                  if (!ktd->names) {
                    logMessage(LOG_ERR, "unknown braille device type: %s-%s",
                               driverCode, deviceType);
                  }
                } else {
                  logMessage(LOG_ERR, "missing braille device type");
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
        logMessage(LOG_ERR, "unknown key table type: %s", tableType);
      }
    } else {
      logMessage(LOG_ERR, "missing key table type");
    }

    deallocateStrings(nameComponents);
  }

  if (ok) {
    if (componentsLeft) {
      logMessage(LOG_ERR, "too many key table name components");
      ok = 0;
    }
  }

  if (ok) return 1;
  if (ktd->path) free(ktd->path);
  memset(ktd, 0, sizeof(*ktd));
  return 0;
}

int
main (int argc, char *argv[]) {
  ProgramExitStatus exitStatus = PROG_EXIT_SUCCESS;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-ktb",
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

  driverObject = NULL;

  if (argc) {
    const char *tableName = (argc--, *argv++);
    KeyTableDescriptor ktd;
    int gotKeyTableDescriptor;

    {
      const char *file = locatePathName(tableName);
      const char *delimiter = strrchr(file, '.');
      size_t length = delimiter? (delimiter - file): strlen(file);
      char name[length + 1];

      memcpy(name, file, length);
      name[length] = 0;

      gotKeyTableDescriptor = getKeyTableDescriptor(&ktd, name);
    }

    if (gotKeyTableDescriptor) {
      if (opt_listKeyNames) {
        if (!listKeyNames(ktd.names, listLine, NULL)) {
          exitStatus = PROG_EXIT_FATAL;
        }
      }

      if (exitStatus == PROG_EXIT_SUCCESS) {
        KeyTable *keyTable = compileKeyTable(ktd.path, ktd.names);

        if (keyTable) {
          if (opt_listKeyTable) {
            if (!listKeyTable(keyTable, listLine, NULL)) {
              exitStatus = PROG_EXIT_FATAL;
            }
          }

          destroyKeyTable(keyTable);
        } else {
          exitStatus = PROG_EXIT_FATAL;
        }
      }

      free(ktd.path);
    } else {
      exitStatus = PROG_EXIT_FATAL;
    }
  } else {
    logMessage(LOG_ERR, "missing key table name");
    exitStatus = PROG_EXIT_SYNTAX;
  }

  if (driverObject) unloadSharedObject(driverObject);
  return exitStatus;
}

KeyTableState
processKeyEvent (KeyTable *table, unsigned char context, KeyGroup group, KeyNumber number, int press) {
  return KTS_UNBOUND;
}

#include "brltty.h"

unsigned int textStart;
unsigned int textCount;
int apiStarted = 0;

unsigned char
getScreenCursorDots (void) {
  return 0;
}

int
api_handleCommand (int command) {
  return 0;
}

int
api_handleKeyEvent (KeyGroup group, KeyNumber number, int press) {
  return 0;
}

#include "scr.h"

KeyTableCommandContext
getScreenCommandContext (void) {
  return KTB_CTX_DEFAULT;
}

#include "message.h"

int
message (const char *mode, const char *text, MessageOptions options) {
  return 1;
}

#include "update.h"

void
scheduleUpdate (const char *reason) {
}
