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

/* tbl2hex.c - filter to compile 256-byte table file into C code
 * $Id: tbl2hex.c,v 1.3 1996/09/24 01:04:25 nn201 Exp $
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "program.h"
#include "options.h"
#include "log.h"
#include "misc.h"
#include "charset.h"
#include "unicode.h"
//#include "brl.h"
#include "ttb.h"
#include "ctb.h"

static char *opt_contractionTable;
static char *opt_tablesDirectory;
static char *opt_textTable;
static char *opt_outputWidth;
static int opt_forceOutput;

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'c',
    .word = "contraction-table",
    .argument = "file",
    .setting.string = &opt_contractionTable,
    .defaultSetting = "en-us-g2",
    .description = "Contraction table."
  },

  { .letter = 'T',
    .word = "tables-directory",
    .flags = OPT_Hidden,
    .argument = strtext("directory"),
    .setting.string = &opt_tablesDirectory,
    .defaultSetting = DATA_DIRECTORY,
    .description = strtext("Path to directory containing tables.")
  },

  { .letter = 't',
    .word = "text-table",
    .argument = "file",
    .setting.string = &opt_textTable,
    .description = "Text table."
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

static ContractionTable *contractionTable;

static int outputWidth;
static int outputExtend;
static unsigned char *outputBuffer;

static void
writeLocalCharacter (FILE *stream, unsigned char cell) {
  fputc(convertDotsToCharacter(textTable, cell), stream);
}

static void
writeUtf8Braille (FILE *stream, unsigned char cell) {
  Utf8Buffer utf8;
  size_t utfs = convertWcharToUtf8(cell|UNICODE_BRAILLE_ROW, utf8);
  fprintf(stream, "%.*s", (int)utfs, utf8);
}

typedef struct {
  unsigned char status;
} LineProcessingData;

static int
contractWcharLine (const wchar_t *line, void *data) {
  LineProcessingData *lpd = data;
  int lineLength = wcslen(line);
  const wchar_t *inputBuffer = line;

  while (lineLength) {
    int inputCount = lineLength;
    int outputCount = outputWidth;

    if (!outputBuffer) {
      if (!(outputBuffer = malloc(outputWidth))) {
        LogError("output buffer allocation");
        lpd->status = 10;
        return 0;
      }
    }

    if (!contractText(contractionTable,
                      inputBuffer, &inputCount,
                      outputBuffer, &outputCount,
                      NULL, CTB_NO_CURSOR)) {
      lpd->status = 11;
      return 0;
    }

    if ((inputCount < lineLength) && outputExtend) {
      free(outputBuffer);
      outputBuffer = NULL;
      outputWidth <<= 1;
    } else {
      {
        FILE *output = stdout;
        void (*writeCell) (FILE *stream, unsigned char cell) = textTable? writeLocalCharacter: writeUtf8Braille;
        int index;

        for (index=0; index<outputCount; index+=1) {
          writeCell(output, outputBuffer[index]);
          if (ferror(output)) break;
        }

        if (!ferror(output)) fputc('\n', output);
        if (opt_forceOutput && !ferror(output)) fflush(output);

        if (ferror(output)) {
          LogError("output");
          lpd->status = 12;
          return 0;
        }
      }

      inputBuffer += inputCount;
      lineLength -= inputCount;
    }
  }

  return 1;
}

static int
contractUtf8Line (const char *line, void *data) {
  size_t count = strlen(line) + 1;
  wchar_t buffer[count];
  wchar_t *characters = buffer;

  convertStringToWchars(&line, &characters, count);
  return contractWcharLine(buffer, data);
}

static int
contractLine (char *line, void *data) {
  return contractUtf8Line(line, data);
}

static int
contractFile (FILE *file) {
  LineProcessingData lpd;
  lpd.status = 0;
  return processLines(file, contractLine, &lpd)? lpd.status: 5;
}

int
main (int argc, char *argv[]) {
  int status = 3;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "ctbtest",
      .argumentsSummary = "[{input-file | -} ...]"
    };
    processOptions(&descriptor, &argc, &argv);
  }

  {
    char **const paths[] = {
      &opt_tablesDirectory,
      NULL
    };
    fixInstallPaths(paths);
  }

  if ((outputExtend = !*opt_outputWidth)) {
    outputWidth = 0X80;
  } else {
    static const int minimum = 1;
    if (!validateInteger(&outputWidth, opt_outputWidth, &minimum, NULL)) {
      LogPrint(LOG_ERR, "%s: %s", "invalid output width", opt_outputWidth);
      exit(2);
    }
  }
  outputBuffer = NULL;

  {
    char *contractionTableFile;

    if ((contractionTableFile = ensureContractionTableExtension(opt_contractionTable))) {
      char *contractionTablePath;

      if ((contractionTablePath = makePath(opt_tablesDirectory, contractionTableFile))) {
        if ((contractionTable = compileContractionTable(contractionTablePath))) {
          if (*opt_textTable) {
            char *textTableFile;

            if ((textTableFile = ensureTextTableExtension(opt_textTable))) {
              char *textTablePath;

              if ((textTablePath = makePath(opt_tablesDirectory, textTableFile))) {
                status = (textTable = compileTextTable(textTablePath))? 0: 4;

                free(textTablePath);
              } else {
                status = 4;
              }

              free(textTableFile);
            } else {
              status = 4;
            }
          } else {
            textTable = NULL;
            status = 0;
          }

          if (!status) {
            if (argc) {
              do {
                char *path = *argv;
                if (strcmp(path, "-") == 0) {
                  status = contractFile(stdin);
                } else {
                  FILE *file = fopen(path, "r");
                  if (file) {
                    status = contractFile(file);
                    fclose(file);
                  } else {
                    LogPrint(LOG_ERR, "cannot open input file: %s: %s",
                             path, strerror(errno));
                    status = 6;
                  }
                }
              } while ((status == 0) && (++argv, --argc));
            } else {
              status = contractFile(stdin);
            }

            if (textTable) destroyTextTable(textTable);
          }

          destroyContractionTable(contractionTable);
        } else {
          status = 3;
        }

        free(contractionTablePath);
      } else {
        status = 3;
      }

      free(contractionTableFile);
    }
  }

  if (outputBuffer) free(outputBuffer);
  return status;
}
