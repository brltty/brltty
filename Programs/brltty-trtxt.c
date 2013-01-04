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
#include <errno.h>

#include "program.h"
#include "options.h"
#include "log.h"
#include "file.h"
#include "unicode.h"
#include "charset.h"
#include "brldots.h"
#include "ttb.h"

static char *opt_tablesDirectory;
static char *opt_inputTable;
static char *opt_outputTable;
static int opt_sixDots;

static const char tableName_autoselect[] = "auto";
static const char tableName_unicode[] = "unicode";

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'T',
    .word = "tables-directory",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("directory"),
    .setting.string = &opt_tablesDirectory,
    .defaultSetting = TABLES_DIRECTORY,
    .description = strtext("Path to directory for text tables.")
  },

  { .letter = 'i',
    .word = "input-table",
    .flags = OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_inputTable,
    .defaultSetting = tableName_autoselect,
    .description = strtext("Path to input text table.")
  },

  { .letter = 'o',
    .word = "output-table",
    .flags = OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_outputTable,
    .defaultSetting = tableName_unicode,
    .description = strtext("Path to output text table.")
  },

  { .letter = '6',
    .word = "six-dots",
    .flags = OPT_Config | OPT_Environ,
    .setting.flag = &opt_sixDots,
    .description = strtext("Remove dots seven and eight.")
  },
END_OPTION_TABLE

static TextTable *inputTable;
static TextTable *outputTable;

static FILE *outputStream;
static const char *outputName;

static unsigned char (*toDots) (wchar_t character);
static wchar_t (*toCharacter) (unsigned char dots);

static unsigned char
toDots_mapped (wchar_t character) {
  return convertCharacterToDots(inputTable, character);
}

static unsigned char
toDots_unicode (wchar_t character) {
  return ((character & UNICODE_ROW_MASK) == UNICODE_BRAILLE_ROW)?
         character & UNICODE_CELL_MASK:
         (BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8);
}

static wchar_t
toCharacter_mapped (unsigned char dots) {
  return convertDotsToCharacter(outputTable, dots);
}

static wchar_t
toCharacter_unicode (unsigned char dots) {
  return UNICODE_BRAILLE_ROW | dots;
}

static int
writeCharacter (const wchar_t *character, mbstate_t *state) {
  char bytes[0X1000];
  size_t result = wcrtomb(bytes, (character? *character: WC_C('\0')), state);

  if (result == (size_t)-1) return 0;
  if (!character) result -= 1;

  fwrite(bytes, 1, result, outputStream);
  return !ferror(outputStream);
}

static int
processStream (FILE *inputStream, const char *inputName) {
  mbstate_t inputState;
  mbstate_t outputState;

  memset(&inputState, 0, sizeof(inputState));
  memset(&outputState, 0, sizeof(outputState));

  while (!feof(inputStream)) {
    char inputBuffer[0X1000];
    size_t inputCount = fread(inputBuffer, 1, sizeof(inputBuffer), inputStream);

    if (ferror(inputStream)) goto inputError;
    if (!inputCount) break;

    {
      char *byte = inputBuffer;

      while (inputCount) {
        wchar_t character;

        {
          size_t result = mbrtowc(&character, byte, inputCount, &inputState);

          if (result == (size_t)-2) break;
          if (result == (size_t)-1) goto inputError;
          if (!result) result = 1;

          byte += result;
          inputCount -= result;
        }

        if (!iswcntrl(character)) {
          unsigned char dots = toDots(character);
          if (opt_sixDots) dots &= ~(BRL_DOT7 | BRL_DOT8);
          character = toCharacter(dots);
        }

        if (!writeCharacter(&character, &outputState)) goto outputError;
      }
    }
  }

  if (!writeCharacter(NULL, &outputState)) goto outputError;
  fflush(outputStream);
  if (ferror(outputStream)) goto outputError;

  if (!mbsinit(&inputState)) {
    errno = EILSEQ;
    goto inputError;
  }

  return 1;

inputError:
  logMessage(LOG_ERR, "input error: %s: %s", inputName, strerror(errno));
  return 0;

outputError:
  logMessage(LOG_ERR, "output error: %s: %s", outputName, strerror(errno));
  return 0;
}

static int
getTable (TextTable **table, const char *directory, const char *name) {
  *table = NULL;

  if (strcmp(name, tableName_unicode) != 0) {
    char *allocated = NULL;

    if (strcmp(name, tableName_autoselect) == 0) {
      if (!(name = allocated = selectTextTable(directory))) {
        logMessage(LOG_ERR, "cannot find text table for current locale");
      }
    }

    if (name) {
      char *path = makeTextTablePath(directory, name);

      if (path) {
        *table = compileTextTable(path);
        free(path);
      }
    }

    if (allocated) free(allocated);
    if (!*table) return 0;
  }

  return 1;
}

int
main (int argc, char *argv[]) {
  ProgramExitStatus exitStatus = PROG_EXIT_FATAL;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-trtxt",
      .argumentsSummary = "[{input-file | -} ...]"
    };
    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  {
    char **const paths[] = {
      &opt_tablesDirectory,
      NULL
    };
    fixInstallPaths(paths);
  }

  if (getTable(&inputTable, opt_tablesDirectory, opt_inputTable)) {
    if (getTable(&outputTable, opt_tablesDirectory, opt_outputTable)) {
      outputStream = stdout;
      outputName = standardOutputName;

      toDots = inputTable? toDots_mapped: toDots_unicode;
      toCharacter = outputTable? toCharacter_mapped: toCharacter_unicode;

      if (argc) {
        do {
          const char *file = argv[0];
          FILE *stream;

          if (strcmp(file, standardStreamArgument) == 0) {
            if (!processStream(stdin, standardInputName)) break;
          } else if ((stream = fopen(file, "r"))) {
            int ok = processStream(stream, file);
            fclose(stream);
            if (!ok) break;
          } else {
            logMessage(LOG_ERR, "cannot open file: %s: %s", file, strerror(errno));
            exitStatus = PROG_EXIT_SEMANTIC;
            break;
          }

          argv += 1, argc -= 1;
        } while (argc);

        if (!argc) exitStatus = PROG_EXIT_SUCCESS;
      } else if (processStream(stdin, standardInputName)) {
        exitStatus = PROG_EXIT_SUCCESS;
      }

      if (outputTable) destroyTextTable(outputTable);
    }

    if (inputTable) destroyTextTable(inputTable);
  }

  return exitStatus;
}
