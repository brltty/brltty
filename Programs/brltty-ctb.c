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

/* tbl2hex.c - filter to compile 256-byte table file into C code
 * $Id: tbl2hex.c,v 1.3 1996/09/24 01:04:25 nn201 Exp $
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "program.h"
#include "options.h"
#include "prefs.h"
#include "log.h"
#include "file.h"
#include "datafile.h"
#include "parse.h"
#include "charset.h"
#include "unicode.h"
#include "ascii.h"
#include "brldots.h"
#include "ttb.h"
#include "ctb.h"

static char *opt_tablesDirectory;
static char *opt_contractionTable;
static char *opt_textTable;
static char *opt_verificationTable;
static int opt_reformatText;
static char *opt_outputWidth;
static int opt_forceOutput;

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'T',
    .word = "tables-directory",
    .flags = OPT_Hidden,
    .argument = strtext("directory"),
    .setting.string = &opt_tablesDirectory,
    .defaultSetting = TABLES_DIRECTORY,
    .description = strtext("Path to directory containing tables.")
  },

  { .letter = 'c',
    .word = "contraction-table",
    .argument = "file",
    .setting.string = &opt_contractionTable,
    .defaultSetting = "en-us-g2",
    .description = "Contraction table."
  },

  { .letter = 't',
    .word = "text-table",
    .argument = "file",
    .setting.string = &opt_textTable,
    .description = "Text table."
  },

  { .letter = 'v',
    .word = "verification-table",
    .argument = "file",
    .setting.string = &opt_verificationTable,
    .description = "Contraction verification table."
  },

  { .letter = 'r',
    .word = "reformat-text",
    .setting.flag = &opt_reformatText,
    .description = "Reformat input."
  },

  { .letter = 'w',
    .word = "output-width",
    .argument = "columns",
    .setting.string = &opt_outputWidth,
    .defaultSetting = "",
    .description = "Maximum length of an output line."
  },

  { .letter = 'f',
    .word = "force-output",
    .setting.flag = &opt_forceOutput,
    .description = "Force immediate output."
  },
END_OPTION_TABLE

static wchar_t *inputBuffer;
static size_t inputSize;
static size_t inputLength;

static FILE *outputStream;
static unsigned char *outputBuffer;
static int outputWidth;
static int outputExtend;

#define VERIFICATION_TABLE_EXTENSION ".cvb"
#define VERIFICATION_SUBTABLE_EXTENSION ".cvi"

static ContractionTable *contractionTable;
static char *verificationTablePath;
static FILE *verificationTableStream;

static int (*processInputCharacters) (const wchar_t *characters, size_t length, void *data);
static int (*putCell) (unsigned char cell, void *data);

typedef struct {
  ProgramExitStatus exitStatus;
} LineProcessingData;

static void
noMemory (void *data) {
  LineProcessingData *lpd = data;

  logMallocError();
  lpd->exitStatus = PROG_EXIT_FATAL;
}

static int
checkOutputStream (void *data) {
  LineProcessingData *lpd = data;

  if (ferror(outputStream)) {
    logSystemError("output");
    lpd->exitStatus = PROG_EXIT_FATAL;
    return 0;
  }

  return 1;
}

static int
flushOutputStream (void *data) {
  fflush(outputStream);
  return checkOutputStream(data);
}

static int
putCharacter (unsigned char character, void *data) {
  fputc(character, outputStream);
  return checkOutputStream(data);
}

static int
putMappedCharacter (unsigned char cell, void *data) {
  fputc(convertDotsToCharacter(textTable, cell), outputStream);
  return checkOutputStream(data);
}

static int
putUnicodeBraille (unsigned char cell, void *data) {
  Utf8Buffer utf8;
  size_t utfs = convertWcharToUtf8(cell|UNICODE_BRAILLE_ROW, utf8);

  fprintf(outputStream, "%.*s", (int)utfs, utf8);
  return checkOutputStream(data);
}

static int
writeCharacters (const wchar_t *inputLine, size_t inputLength, void *data) {
  const wchar_t *inputBuffer = inputLine;

  while (inputLength) {
    int inputCount = inputLength;
    int outputCount = outputWidth;

    if (!outputBuffer) {
      if (!(outputBuffer = malloc(outputWidth))) {
        noMemory(data);
        return 0;
      }
    }

    contractText(contractionTable,
                 inputBuffer, &inputCount,
                 outputBuffer, &outputCount,
                 NULL, CTB_NO_CURSOR);

    if ((inputCount < inputLength) && outputExtend) {
      free(outputBuffer);
      outputBuffer = NULL;
      outputWidth <<= 1;
    } else {
      {
        int index;

        for (index=0; index<outputCount; index+=1)
          if (!putCell(outputBuffer[index], data))
            return 0;
      }

      inputBuffer += inputCount;
      inputLength -= inputCount;

      if (inputLength)
        if (!putCharacter('\n', data))
          return 0;
    }
  }

  return 1;
}

static int
flushCharacters (wchar_t end, void *data) {
  if (inputLength) {
    if (!writeCharacters(inputBuffer, inputLength, data)) return 0;
    inputLength = 0;

    if (end)
      if (!putCharacter(end, data))
        return 0;
  }

  return 1;
}

static int
processCharacters (const wchar_t *characters, size_t count, wchar_t end, void *data) {
  if (opt_reformatText && count) {
    if (iswspace(characters[0]))
      if (!flushCharacters('\n', data))
        return 0;

    {
      unsigned int spaces = !inputLength? 0: 1;
      size_t newLength = inputLength + spaces + count;

      if (newLength > inputSize) {
        size_t newSize = newLength | 0XFF;
        wchar_t *newBuffer = calloc(newSize, sizeof(*newBuffer));

        if (!newBuffer) {
          noMemory(data);
          return 0;
        }

        wmemcpy(newBuffer, inputBuffer, inputLength);
        free(inputBuffer);

        inputBuffer = newBuffer;
        inputSize = newSize;
      }

      while (spaces) {
        inputBuffer[inputLength++] = WC_C(' ');
        spaces -= 1;
      }

      wmemcpy(&inputBuffer[inputLength], characters, count);
      inputLength += count;
    }

    if (end != '\n') {
      if (!flushCharacters(0, data)) return 0;
      if (!putCharacter(end, data)) return 0;
    }
  } else {
    if (!flushCharacters('\n', data)) return 0;
    if (!writeCharacters(characters, count, data)) return 0;
    if (!putCharacter(end, data)) return 0;
  }

  return 1;
}

static int
writeContractedBraille (const wchar_t *characters, size_t length, void *data) {
  const wchar_t *character = characters;

  while (1) {
    const wchar_t *end = wmemchr(character, FF, length);
    size_t count;

    if (!end) break;
    count = end - character;
    if (!processCharacters(character, count, *end, data)) return 0;

    count += 1;
    character += count;
    length -= count;
  }
  if (!processCharacters(character, length, '\n', data)) return 0;

  if (opt_forceOutput)
    if (!flushOutputStream(data))
      return 0;

  return 1;
}

static char *
makeUtf8FromCells (unsigned char *cells, size_t count) {
  char *text = malloc((count * UTF8_LEN_MAX) + 1);

  if (text) {
    char *ch = text;
    size_t i;

    for (i=0; i<count; i+=1) {
      Utf8Buffer utf8;
      size_t utfs = convertWcharToUtf8(cells[i]|UNICODE_BRAILLE_ROW, utf8);

      if (utfs) {
        ch = mempcpy(ch, utf8, utfs);
      } else {
        *ch++ = ' ';
      }
    }

    *ch = 0;

    return text;
  } else {
    logMallocError();
  }

  return NULL;
}

static int
writeVerificationTableLine (const wchar_t *characters, size_t length, void *data) {
  int inputCount = length;
  int outputCount = length << 2;
  unsigned char outputBuffer[outputCount];

  contractText(contractionTable,
               characters, &inputCount,
               outputBuffer, &outputCount,
               NULL, CTB_NO_CURSOR);

  {
    char *utf8 = makeUtf8FromWchars(characters, length, NULL);

    if (utf8) {
      fprintf(verificationTableStream, "contracts %s ", utf8);
      free(utf8);
    }
  }

  {
    int index;

    for (index=0; index<outputCount; index+=1) {
      unsigned char dots = outputBuffer[index];

      if (index > 0) fprintf(verificationTableStream, "-");

      if (dots) {
        while (dots) {
          fprintf(verificationTableStream, "%c", brlDotToNumber(dots));
          dots &= dots - 1;
        }
      } else {
        fprintf(verificationTableStream, "0");
      }
    }
  }

  {
    char *utf8 = makeUtf8FromCells(outputBuffer, outputCount);

    if (utf8) {
      fprintf(verificationTableStream, " %s", utf8);
      free(utf8);
    }
  }

  fprintf(verificationTableStream, "\n");
  return 1;
}

static int
processContractsOperands (DataFile *file, void *data) {
  DataString text;

  if (getDataString(file, &text, 1, "text")) {
    ByteOperand cells;

    if (getCellsOperand(file, &cells, "cells")) {
      int inputCount = text.length;
      int outputCount = inputCount << 3;
      unsigned char outputBuffer[outputCount];

      contractText(contractionTable,
		   text.characters, &inputCount,
		   outputBuffer, &outputCount,
		   NULL, CTB_NO_CURSOR);

      if ((outputCount != cells.length) ||
	  (memcmp(cells.bytes, outputBuffer, outputCount) != 0)) {
	char *expected;

	if ((expected = makeUtf8FromCells(cells.bytes, cells.length))) {
           char *actual;

           if ((actual = makeUtf8FromCells(outputBuffer, outputCount))) {
              reportDataError(file,
                              "%.*" PRIws ": expected %s, got %s",
                              text.length, text.characters,
                              expected, actual);

              free(actual);
           }

           free(expected);
        }
      }

      return 1;
    }
  }

  return 0;
}

static int
processVerificationLine (DataFile *file, void *data) {
  static const DataProperty properties[] = {
    {.name=WS_C("assign"), .processor=processAssignOperands},
    {.name=WS_C("contracts"), .processor=processContractsOperands},
    {.name=WS_C("include"), .processor=processIncludeOperands},
    {.name=NULL, .processor=NULL}
  };

  return processPropertyOperand(file, properties, "contraction verification table directive", data);
}

static ProgramExitStatus
processVerificationTable (void) {
  if (setGlobalTableVariables(VERIFICATION_TABLE_EXTENSION, VERIFICATION_SUBTABLE_EXTENSION)) {
    if (processDataStream(NULL, verificationTableStream, verificationTablePath, processVerificationLine, NULL)) {
      return PROG_EXIT_SUCCESS;
    }
  }

  return PROG_EXIT_FATAL;
}

static int
processInputLine (char *line, void *data) {
  const char *string = line;
  size_t length = strlen(string);
  const char *byte = string;

  size_t count = length + 1;
  wchar_t characters[count];
  wchar_t *character = characters;

  convertUtf8ToWchars(&byte, &character, count);
  length = character - characters;

  return processInputCharacters(characters, length, data);
}

static ProgramExitStatus
processInputStream (FILE *stream) {
  LineProcessingData lpd = {
    .exitStatus = PROG_EXIT_SUCCESS
  };
  ProgramExitStatus exitStatus = processLines(stream, processInputLine, &lpd)? lpd.exitStatus: PROG_EXIT_FATAL;

  if (exitStatus == PROG_EXIT_SUCCESS)
    if (!(flushCharacters('\n', &lpd) && flushOutputStream(&lpd)))
      exitStatus = lpd.exitStatus;

  return exitStatus;
}

int
main (int argc, char *argv[]) {
  ProgramExitStatus exitStatus = PROG_EXIT_FATAL;

  verificationTablePath = NULL;
  verificationTableStream = NULL;
  processInputCharacters = writeContractedBraille;

  resetPreferences();
  prefs.expandCurrentWord = 0;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-ctb",
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

  inputBuffer = NULL;
  inputSize = 0;
  inputLength = 0;

  outputStream = stdout;
  outputBuffer = NULL;

  if ((outputExtend = !*opt_outputWidth)) {
    outputWidth = 0X80;
  } else {
    static const int minimum = 1;

    if (!validateInteger(&outputWidth, opt_outputWidth, &minimum, NULL)) {
      logMessage(LOG_ERR, "%s: %s", "invalid output width", opt_outputWidth);
      return PROG_EXIT_SYNTAX;
    }
  }

  {
    char *contractionTablePath;

    if ((contractionTablePath = makeContractionTablePath(opt_tablesDirectory, opt_contractionTable))) {
      if ((contractionTable = compileContractionTable(contractionTablePath))) {
        if (*opt_textTable) {
          char *textTablePath;

          putCell = putMappedCharacter;

          if ((textTablePath = makeTextTablePath(opt_tablesDirectory, opt_textTable))) {
            exitStatus = (textTable = compileTextTable(textTablePath))? PROG_EXIT_SUCCESS: PROG_EXIT_FATAL;
            free(textTablePath);
          } else {
            exitStatus = PROG_EXIT_FATAL;
          }
        } else {
          putCell = putUnicodeBraille;
          exitStatus = PROG_EXIT_SUCCESS;
        }

        if (exitStatus == PROG_EXIT_SUCCESS) {
          if (opt_verificationTable && *opt_verificationTable) {
            if ((verificationTablePath = makeFilePath(opt_tablesDirectory, opt_verificationTable, VERIFICATION_TABLE_EXTENSION))) {
              const char *verificationTableMode = (argc > 0)? "w": "r";

              if ((verificationTableStream = openDataFile(verificationTablePath, verificationTableMode, 0))) {
                processInputCharacters = writeVerificationTableLine;
              } else {
                exitStatus = PROG_EXIT_FATAL;
              }
            } else {
              exitStatus = PROG_EXIT_FATAL;
            }
          }
        }

        if (exitStatus == PROG_EXIT_SUCCESS) {
          if (argc) {
            do {
              char *path = *argv;
              if (strcmp(path, standardStreamArgument) == 0) {
                exitStatus = processInputStream(stdin);
              } else {
                FILE *stream = fopen(path, "r");
                if (stream) {
                  exitStatus = processInputStream(stream);
                  fclose(stream);
                } else {
                  logMessage(LOG_ERR, "cannot open input file: %s: %s",
                             path, strerror(errno));
                  exitStatus = PROG_EXIT_FATAL;
                }
              }
            } while ((exitStatus == PROG_EXIT_SUCCESS) && (++argv, --argc));
          } else if (verificationTableStream) {
            exitStatus = processVerificationTable();
          } else {
            exitStatus = processInputStream(stdin);
          }

          if (textTable) destroyTextTable(textTable);
        }

        destroyContractionTable(contractionTable);
      } else {
        exitStatus = PROG_EXIT_FATAL;
      }

      free(contractionTablePath);
    }
  }

  if (verificationTableStream) {
    if (fclose(verificationTableStream) == EOF) {
      exitStatus = PROG_EXIT_FATAL;
    }

    verificationTableStream = NULL;
  }

  if (verificationTablePath) {
    free(verificationTablePath);
    verificationTablePath = NULL;
  }

  if (outputBuffer) free(outputBuffer);
  if (inputBuffer) free(inputBuffer);
  return exitStatus;
}
