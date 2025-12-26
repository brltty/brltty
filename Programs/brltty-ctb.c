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
#include <errno.h>

#include "cmdline.h"
#include "cmdput.h"
#include "options.h"
#include "prefs.h"
#include "log.h"
#include "file.h"
#include "datafile.h"
#include "parse.h"
#include "utf8.h"
#include "unicode.h"
#include "ascii.h"
#include "ttb.h"
#include "ctb.h"

char *opt_tablesDirectory;
char *opt_contractionTable;
char *opt_textTable;
static char *opt_verificationTable;

static char *opt_outputWidth;
static int opt_reformatText;
static int opt_forceOutput;

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
  { .word = "output-width",
    .letter = 'w',
    .argument = "columns",
    .setting.string = &opt_outputWidth,
    .internal.setting = "",
    .description = strtext("Maximum length of an output line.")
  },

  { .word = "reformat-text",
    .letter = 'r',
    .setting.flag = &opt_reformatText,
    .description = strtext("Reformat input.")
  },

  { .word = "force-output",
    .letter = 'f',
    .setting.flag = &opt_forceOutput,
    .description = strtext("Force immediate output.")
  },

  { .word = "contraction-table",
    .letter = 'c',
    .argument = "file",
    .setting.string = &opt_contractionTable,
    .internal.setting = "en-us-g2",
    .description = strtext("Contraction table.")
  },

  { .word = "text-table",
    .letter = 't',
    .argument = "file",
    .setting.string = &opt_textTable,
    .description = strtext("Text table.")
  },

  { .word = "verification-table",
    .letter = 'v',
    .argument = "file",
    .setting.string = &opt_verificationTable,
    .description = strtext("Contraction verification table.")
  },

  { .word = "tables-directory",
    .letter = 'T',
    .argument = strtext("directory"),
    .setting.string = &opt_tablesDirectory,
    .internal.setting = TABLES_DIRECTORY,
    .internal.adjust = toAbsoluteInstallPath,
    .description = strtext("Path to directory containing tables.")
  },
END_COMMAND_LINE_OPTIONS(programOptions)

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
  "Contracted braille is written to standard output.",
  "If a text table has been specified then it defines the braille characters which are written.",
  "If a text table hasn't been specified then Unicode braille patterns are written.",
  "",
  "If no files are specified then standard input is translated.",
  "Translation isn't performed if a contraction table ii being verified.",
  "",
  "Each individual input line is translated into a separate output line.",
  "If text reformatting has been requested then each sequence of unindented lines",
  "is joined, separated from one another by a blank, into a single line thus",
  "effectively treating each sequence of unindented lines as a single paragraph.",
  "Empty lines and those which begin with whitespace aren't reformatted.",
  "",
  "The default is for each output lie to be as long as it needs to be.",
  "If, however, an explicit output width has been specified",
  "then longer output lines are word-wrapped at that width.",
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "brltty-ctb",
  .purpose = strtext("Check/validate a contraction (literary braille) table, or translate text into contracted braille."),

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),

  .extraParameters = {
    .name = "file",
    .description = "the files to translate (use - for standard input)",
  },
END_COMMAND_LINE_DESCRIPTOR

static wchar_t *inputBuffer;
static size_t inputSize;
static size_t inputLength;

static unsigned char *outputBuffer;
static int outputWidth;
static int outputExtend;

#define VERIFICATION_TABLE_EXTENSION ".cvb"
#define VERIFICATION_SUBTABLE_EXTENSION ".cvi"

static char *verificationTablePath;
static FILE *verificationTableStream;

static int (*processInputCharacters) (const wchar_t *characters, size_t length, void *data);
static void (*putCell) (unsigned char cell);

typedef struct {
  ProgramExitStatus exitStatus;
} LineProcessingData;

static void
onNoMemory (void *data) {
  LineProcessingData *lpd = data;

  logMallocError();
  lpd->exitStatus = PROG_EXIT_FATAL;
}

static void
putCellCharacter (wchar_t character) {
  putUtf8Character(character);
}

static void
putTextCell (unsigned char cell) {
  putCellCharacter(convertDotsToCharacter(textTable, cell));
}

static void
putBrailleCell (unsigned char cell) {
  putCellCharacter((UNICODE_BRAILLE_ROW | cell));
}

static int
writeCharacters (const wchar_t *inputLine, size_t inputLength, void *data) {
  const wchar_t *inputBuffer = inputLine;

  while (inputLength) {
    int inputCount = inputLength;
    int outputCount = outputWidth;

    if (!outputBuffer) {
      if (!(outputBuffer = malloc(outputWidth))) {
        onNoMemory(data);
        return 0;
      }
    }

    contractText(
      contractionTable, NULL,
      inputBuffer, &inputCount,
      outputBuffer, &outputCount,
      NULL, CTB_NO_CURSOR
    );

    if ((inputCount < inputLength) && outputExtend) {
      free(outputBuffer);
      outputBuffer = NULL;
      outputWidth <<= 1;
    } else {
      for (int index=0; index<outputCount; index+=1) {
        putCell(outputBuffer[index]);
      }

      inputBuffer += inputCount;
      inputLength -= inputCount;
      if (inputLength) putByte('\n');
    }
  }

  return 1;
}

static int
flushCharacters (wchar_t end, void *data) {
  if (inputLength) {
    if (!writeCharacters(inputBuffer, inputLength, data)) return 0;
    inputLength = 0;
    if (end) putByte(end);
  }

  return 1;
}

static int
processCharacters (const wchar_t *characters, size_t count, wchar_t end, void *data) {
  if (opt_reformatText && count && !iswspace(characters[0])) {
    {
      unsigned int spaces = !inputLength? 0: 1;
      size_t newLength = inputLength + spaces + count;

      if (newLength > inputSize) {
        size_t newSize = newLength | 0XFF;
        wchar_t *newBuffer = calloc(newSize, sizeof(*newBuffer));

        if (!newBuffer) {
          onNoMemory(data);
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
      putByte(end);
    }
  } else {
    if (!flushCharacters('\n', data)) return 0;
    if (!writeCharacters(characters, count, data)) return 0;
    putByte(end);
  }

  return 1;
}

static int
writeContractedBraille (const wchar_t *characters, size_t length, void *data) {
  const wchar_t *character = characters;

  while (1) {
    const wchar_t *end = wmemchr(character, ASCII_FF, length);
    size_t count;

    if (!end) break;
    count = end - character;
    if (!processCharacters(character, count, *end, data)) return 0;

    count += 1;
    character += count;
    length -= count;
  }

  if (!processCharacters(character, length, '\n', data)) return 0;
  if (opt_forceOutput) putFlush();
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

  contractText(contractionTable, NULL,
               characters, &inputCount,
               outputBuffer, &outputCount,
               NULL, CTB_NO_CURSOR);

  if (fprintf(verificationTableStream, "contracts ") == EOF) goto outputError;
  if (!writeEscapedCharacters(verificationTableStream, characters, length)) goto outputError;
  if (fprintf(verificationTableStream, " ") == EOF) goto outputError;
  if (!writeDotsCells(verificationTableStream, outputBuffer, outputCount)) goto outputError;
  if (fprintf(verificationTableStream, " ") == EOF) goto outputError;
  if (!writeUtf8Cells(verificationTableStream, outputBuffer, outputCount)) goto outputError;
  if (fprintf(verificationTableStream, "\n") == EOF) goto outputError;
  return 1;

outputError:
  logMessage(LOG_ERR, "output error: %s", strerror(errno));
  return 0;
}

static DATA_OPERANDS_PROCESSOR(processContractsOperands) {
  DataString text;

  if (getDataString(file, &text, 1, "uncontracted text")) {
    ByteOperand cells;

    if (getCellsOperand(file, &cells, "contracted braille")) {
      int inputCount = text.length;
      int outputCount = inputCount << 3;
      unsigned char outputBuffer[outputCount];

      contractText(contractionTable, NULL,
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
                              "%" PRIws ": expected %s, got %s",
                              text.characters, expected, actual); 
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

static  DATA_OPERANDS_PROCESSOR(processVerificationOperands) {
  BEGIN_DATA_DIRECTIVE_TABLE
    {.name=WS_C("contracts"), .processor=processContractsOperands},
  END_DATA_DIRECTIVE_TABLE

  return processDirectiveOperand(file, &directives, "contraction verification directive", data);
}

static ProgramExitStatus
processVerificationTable (void) {
  if (setTableDataVariables(VERIFICATION_TABLE_EXTENSION, VERIFICATION_SUBTABLE_EXTENSION)) {
    const DataFileParameters parameters = {
      .processOperands = processVerificationOperands,
      .data = NULL
    };

    if (processDataStream(NULL, verificationTableStream, verificationTablePath, &parameters)) {
      return PROG_EXIT_SUCCESS;
    }
  }

  return PROG_EXIT_FATAL;
}

static DATA_OPERANDS_PROCESSOR(processInputLine) {
  DataOperand line;
  getTextRemaining(file, &line);
  return processInputCharacters(line.characters, line.length, data);
}

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  ProgramExitStatus exitStatus = PROG_EXIT_FATAL;
  verificationTablePath = NULL;
  verificationTableStream = NULL;
  processInputCharacters = writeContractedBraille;

  resetPreferences();
  prefs.expandCurrentWord = 0;

  inputBuffer = NULL;
  inputSize = 0;
  inputLength = 0;

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
          putCell = putTextCell;
          char *textTablePath;

          if ((textTablePath = makeTextTablePath(opt_tablesDirectory, opt_textTable))) {
            textTable = compileTextTable(textTablePath);
            exitStatus = textTable? PROG_EXIT_SUCCESS: PROG_EXIT_FATAL;
            free(textTablePath);
          } else {
            exitStatus = PROG_EXIT_FATAL;
          }
        } else {
          putCell = putBrailleCell;
          textTable = NULL;
          exitStatus = PROG_EXIT_SUCCESS;
        }

        if (exitStatus == PROG_EXIT_SUCCESS) {
          if (opt_verificationTable && *opt_verificationTable) {
            if ((verificationTablePath = makeFilePath(NULL, opt_verificationTable, VERIFICATION_TABLE_EXTENSION))) {
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
          if (verificationTableStream && !argc) {
            exitStatus = processVerificationTable();
          } else {
            LineProcessingData lpd = {
              .exitStatus = PROG_EXIT_SUCCESS
            };

            const InputFilesProcessingParameters parameters = {
              .dataFileParameters = {
                .options = DFO_NO_COMMENTS,
                .processOperands = processInputLine,
                .data = &lpd
              }
            };

            if ((exitStatus = processInputFiles(argv, argc, &parameters)) == PROG_EXIT_SUCCESS) {
              if (flushCharacters('\n', &lpd)) {
                putFlush();
              } else {
                exitStatus = lpd.exitStatus;
              }
            }
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
