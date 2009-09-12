/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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

#include "program.h"
#include "options.h"
#include "misc.h"
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
  if (ferror(stream)) return 0;
  return 1;
}

static KEY_NAME_TABLES_REFERENCE
getKeyNameTables (const char *keyTableName) {
  KEY_NAME_TABLES_REFERENCE keyNameTables = NULL;
  int count;
  char **components = splitString(keyTableName, '-', &count);

  if (components) {
    char **component = components;

    if (count) {
      const char *type = *component++; count--;

      if (strcmp(type, "kbd") == 0) {
        if (count) {
          keyNameTables = KEY_NAME_TABLES(keyboard);
        } else {
          LogPrint(LOG_ERR, "missing keyboard key table name");
        }
      } else if (strcmp(type, "brl") == 0) {
        if (count) {
          void *object;
          const char *code = *component++; count--;

          if (loadBrailleDriver(code, &object, opt_libraryDirectory)) {
            char *symbol;

            {
              const char *components[] = {"brl_ktb_", code};
              symbol = joinStrings(components, ARRAY_COUNT(components));
            }

            if (symbol) {
              const KeyTableDefinition *const *definitions;

              if (findSharedSymbol(object, symbol, &definitions)) {
                const KeyTableDefinition *const *definition = definitions;

                if (count) {
                  const char *name = *component++; count--;

                  while (*definition) {
                    if (strcmp(name, (*definition)->bindings) == 0) {
                      keyNameTables = (*definition)->names;
                      break;
                    }

                    definition += 1;
                  }

                  if (!keyNameTables) {
                    LogPrint(LOG_ERR, "unknown %s braille driver key table name: %s", code, name);
                  }
                } else {
                  LogPrint(LOG_ERR, "missing braille driver key table name");
                }
              }
            } else {
              LogError("malloc");
            }
          }
        } else {
          LogPrint(LOG_ERR, "missing braille driver code");
        }
      } else {
        LogPrint(LOG_ERR, "unknown key table type: %s", type);
      }
    } else {
      LogPrint(LOG_ERR, "missing key table type");
    }
  }

  deallocateStrings(components);
  return keyNameTables;
}

int
main (int argc, char *argv[]) {
  int status = 3;
  const char *keyTableName;
  KEY_NAME_TABLES_REFERENCE keyNameTables;

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

  if (!argc) {
    LogPrint(LOG_ERR, "missing key table name");
    exit(2);
  }
  keyTableName = *argv++; argc--;

  if ((keyNameTables = getKeyNameTables(keyTableName))) {
    char *keyTableFile = ensureKeyTableExtension(keyTableName);

    if (keyTableFile) {
      char *keyTablePath = makePath(opt_tablesDirectory, keyTableFile);

      if (keyTablePath) {
        KeyTable *keyTable = compileKeyTable(keyTablePath, keyNameTables);

        if (keyTable) {
          if (opt_listKeyTable) {
            listKeyTable(keyTable, listKeyTableLine, NULL);
          }

          destroyKeyTable(keyTable);
        }

        free(keyTablePath);
      }

      free(keyTableFile);
    }
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
message (const char *mode, const char *string, short flags) {
  return 1;
}
