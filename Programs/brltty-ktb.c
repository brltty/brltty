/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>

#include "cmdline.h"
#include "cmdput.h"
#include "options.h"
#include "log.h"
#include "file.h"
#include "parse.h"
#include "dynld.h"
#include "ktb.h"
#include "ktb_keyboard.h"
#include "brl.h"

char *opt_brailleDriver;
static int opt_audit;
static int opt_listKeyNames;
static int opt_listHelpScreen;
static int opt_listRestructuredText;
char *opt_tablesDirectory;
char *opt_driversDirectory;

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
  { .word = "braille-driver",
    .letter = 'b',
    .argument = strtext("driver"),
    .setting.string = &opt_brailleDriver,
    .description = strtext("Braille driver code."),
  },

  { .word = "audit",
    .letter = 'a',
    .setting.flag = &opt_audit,
    .description = strtext("Report problems with the key table.")
  },

  { .word = "keys",
    .letter = 'k',
    .setting.flag = &opt_listKeyNames,
    .description = strtext("List key names.")
  },

  { .word = "list",
    .letter = 'l',
    .setting.flag = &opt_listHelpScreen,
    .description = strtext("List key table in help screen format.")
  },

  { .word = "reStructuredText",
    .letter = 'r',
    .setting.flag = &opt_listRestructuredText,
    .description = strtext("List key table in reStructuredText format.")
  },

  { .word = "tables-directory",
    .letter = 'T',
    .argument = strtext("directory"),
    .setting.string = &opt_tablesDirectory,
    .internal.setting = TABLES_DIRECTORY,
    .internal.adjust = toAbsoluteInstallPath,
    .description = strtext("Path to directory containing tables.")
  },

  { .word = "drivers-directory",
    .letter = 'D',
    .argument = strtext("directory"),
    .setting.string = &opt_driversDirectory,
    .internal.setting = DRIVERS_DIRECTORY,
    .internal.adjust = toAbsoluteInstallPath,
    .description = strtext("Path to directory for loading drivers.")
  },
END_COMMAND_LINE_OPTIONS(programOptions)

static const char *tableName;

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
  { .name = "table",
    .description = "the name of (or path to) the key table",
    .setting = &tableName,
  },
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "brltty-ktb",
  .purpose = strtext("check a key table, list the key naems it can use, or write the key bindings it defines in useful formats."),

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),
END_COMMAND_LINE_DESCRIPTOR

static void *driverObject;

typedef struct {
  KEY_NAME_TABLES_REFERENCE names;
  char *path;
} KeyTableDescriptor;

static int
getKeyTableDescriptor (KeyTableDescriptor *ktd, const char *tableName) {
  int ok = 0;

  memset(ktd, 0, sizeof(*ktd));
  ktd->names = NULL;
  ktd->path = NULL;

  if (*opt_brailleDriver) {
    if (loadBrailleDriver(opt_brailleDriver, &driverObject, opt_driversDirectory)) {
      char *keyTablesSymbol;

      {
        const char *strings[] = {"brl_ktb_", opt_brailleDriver};
        keyTablesSymbol = joinStrings(strings, ARRAY_COUNT(strings));
      }

      if (keyTablesSymbol) {
        const KeyTableDefinition *const *keyTableDefinitions;

        if (findSharedSymbol(driverObject, keyTablesSymbol, &keyTableDefinitions)) {
          const KeyTableDefinition *const *currentDefinition = keyTableDefinitions;

          while (*currentDefinition) {
            if (strcmp(tableName, (*currentDefinition)->bindings) == 0) {
              ktd->names = (*currentDefinition)->names;
              if ((ktd->path = makeInputTablePath(opt_tablesDirectory, opt_brailleDriver, tableName))) ok = 1;
              break;
            }

            currentDefinition += 1;
          }

          if (!ktd->names) {
            logMessage(LOG_ERR,
              "unknown braille device model: %s-%s",
              opt_brailleDriver, tableName
            );
          }
        }

        free(keyTablesSymbol);
      } else {
        logMallocError();
      }
    }
  } else {
    ktd->names = KEY_NAME_TABLES(keyboard);
    if ((ktd->path = makeKeyboardTablePath(opt_tablesDirectory, tableName))) ok = 1;
  }

  if (!ok) {
    if (ktd->path) free(ktd->path);
    memset(ktd, 0, sizeof(*ktd));
  }

  return ok;
}

static int
writeLine (const wchar_t *line) {
  putf("%" PRIws "\n", line);
  return 1;
}

static int
hlpWriteLine (const wchar_t *line, void *data) {
  return writeLine(line);
}

typedef struct {
  unsigned int headerLevel;
  unsigned int elementLevel;
  wchar_t elementBullet;
  unsigned blankLine:1;
} RestructuredTextData;

static int
rstAddLine (const wchar_t *line, RestructuredTextData *rst) {
  if (*line) {
    rst->blankLine = 0;
  } else if (rst->blankLine) {
    return 1;
  } else {
    rst->blankLine = 1;
  }

  return writeLine(line);
}

static int
rstAddBlankLine (RestructuredTextData *rst) {
  return rstAddLine(WS_C(""), rst);
}

static int
rstWriteLine (const wchar_t *line, void *data) {
  RestructuredTextData *rst = data;

  const unsigned int indent = 2;
  const unsigned int count = indent * rst->elementLevel;
  size_t length = wcslen(line);
  wchar_t buffer[count + length + 1];

  if (rst->elementLevel > 0) {
    wmemset(buffer, WC_C(' '), count);
    buffer[count - indent] = rst->elementBullet;
    rst->elementBullet = WC_C(' ');

    wmemcpy(&buffer[count], line, (length + 1));
    line = buffer;
  }

  return rstAddLine(line, rst);
}

static int
rstWriteHeader (const wchar_t *text, unsigned int level, void *data) {
  RestructuredTextData *rst = data;

  if (level > (rst->headerLevel + 1)) {
    level = rst->headerLevel + 1;
  } else {
    rst->headerLevel = level;
  }

  static const wchar_t characters[] = {
    WC_C('='), WC_C('-'), WC_C('~')
  };

  size_t length = wcslen(text);
  wchar_t underline[length + 1];

  wmemset(underline, characters[level], length);
  underline[length] = 0;

  if (!rstAddLine(text, rst)) return 0;
  if (!rstAddLine(underline, rst)) return 0;
  if (!rstAddBlankLine(rst)) return 0;

  if (level == 0) {
    if (!rstAddLine(WS_C(".. contents::"), rst)) return 0;
    if (!rstAddBlankLine(rst)) return 0;
  }

  return 1;
}

static int
rstBeginElement (unsigned int level, void *data) {
  RestructuredTextData *rst = data;

  static const wchar_t bullets[] = {
    WC_C('*'), WC_C('+'), WC_C('o')
  };

  rst->elementLevel = level;
  rst->elementBullet = bullets[level - 1];

  if (!rstAddBlankLine(rst)) return 0;
  return 1;
}

static int
rstEndList (void *data) {
  RestructuredTextData *rst = data;

  rst->elementLevel = 0;
  if (!rstAddBlankLine(rst)) return 0;
  return 1;
}

static const KeyTableListMethods rstMethods = {
  .writeHeader = rstWriteHeader,
  .beginElement = rstBeginElement,
  .endList = rstEndList
};

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  ProgramExitStatus exitStatus = PROG_EXIT_SUCCESS;
  driverObject = NULL;

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
      if (!listKeyNames(ktd.names, hlpWriteLine, NULL)) {
        exitStatus = PROG_EXIT_FATAL;
      }
    }

    if (exitStatus == PROG_EXIT_SUCCESS) {
      KeyTable *keyTable = compileKeyTable(ktd.path, ktd.names);

      if (keyTable) {
        if (opt_audit) {
          if (!auditKeyTable(keyTable, ktd.path)) {
            exitStatus = PROG_EXIT_FATAL;
          }
        }

        if (opt_listHelpScreen) {
          if (!listKeyTable(keyTable, NULL, hlpWriteLine, NULL)) {
            exitStatus = PROG_EXIT_FATAL;
          }
        }

        if (opt_listRestructuredText) {
          RestructuredTextData rst = {
            .headerLevel = 0,
            .elementLevel = 0,
            .elementBullet = WC_C(' '),
            .blankLine = 0
          };

          if (!listKeyTable(keyTable, &rstMethods, rstWriteLine, &rst)) {
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

  if (driverObject) unloadSharedObject(driverObject);
  return exitStatus;
}

#include "scr.h"

KeyTableCommandContext
getScreenCommandContext (void) {
  return KTB_CTX_DEFAULT;
}

int
currentVirtualTerminal (void) {
  return 0;
}

#include "alert.h"

void
alert (AlertIdentifier identifier) {
}

void
speakAlertText (const wchar_t *text) {
}

#include "api_control.h"

const ApiMethods api;

#include "message.h"

int
message (const char *mode, const char *text, MessageOptions options) {
  return 1;
}
