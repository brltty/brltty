/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "program.h"
#include "options.h"
#include "log.h"
#include "cldr.h"

#define DEFAULT_OUTPUT_FORMAT "%n\\n"

static char *opt_outputFormat;

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'f',
    .word = "output-format",
    .argument = strtext("string"),
    .setting.string = &opt_outputFormat,
    .internal.setting = DEFAULT_OUTPUT_FORMAT,
    .description = strtext("Format of each output line.")
  },
END_OPTION_TABLE

static void
putCharacter (int character) {
  if (putchar(character) == EOF) {
    logMessage(LOG_ERR, "output error %d: %s", errno, strerror(errno));
    exit(PROG_EXIT_FATAL);
  }
}

static void
putString (const char *string) {
  while (*string) putCharacter(*string++ & 0XFF);
}

static
CLDR_ANNOTATION_HANDLER(handleAnnotation) {
  typedef enum {LITERAL, FORMAT, ESCAPE} State;
  State state = LITERAL;
  const char *format = opt_outputFormat;

  while (*format) {
    int character = *format & 0XFF;

    switch (state) {
      case LITERAL: {
        switch (character) {
          case '%':
            state = FORMAT;
            break;

          case '\\':
            state = ESCAPE;
            break;

          default:
            putCharacter(character);
            break;
        }

        break;
      }

      case FORMAT: {
        switch (character) {
          case 'n':
            putString(parameters->name);
            break;

          case 's':
            putString(parameters->sequence);
            break;

          case '%':
            putCharacter(character);
            break;

          default:
            logMessage(LOG_ERR, "unrecognized format character: %c", character);
            exit(PROG_EXIT_SYNTAX);
        }

        state = LITERAL;
        break;
      }

      case ESCAPE: {
        static const char escapes[] = {
          ['a'] = '\a',
          ['b'] = '\b',
          ['e'] = '\e',
          ['f'] = '\f',
          ['n'] = '\n',
          ['r'] = '\r',
          ['t'] = '\t',
          ['v'] = '\v',
          ['\\'] = '\\'
        };

        switch (character) {
          default: {
            if (character < ARRAY_COUNT(escapes)) {
              char escape = escapes[character];

              if (escape) {
                putCharacter(escape);
                break;
              }
            }

            logMessage(LOG_ERR, "unrecognized escape character: %c", character);
            exit(PROG_EXIT_SYNTAX);
          }
        }

        state = LITERAL;
        break;
      }
    }

    format += 1;
  }

  switch (state) {
    case LITERAL:
      return 1;

    case FORMAT:
      logMessage(LOG_ERR, "missing format character");
      break;

    case ESCAPE:
      logMessage(LOG_ERR, "missing escape character");
      break;
  }

  exit(PROG_EXIT_SYNTAX);
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-cldr",
      .argumentsSummary = "input-file"
    };
    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  if (argc < 1) {
    fprintf(stderr, "missing annotations file name\n");
    return PROG_EXIT_SYNTAX;
  }

  const char *inputFile = *argv++;
  argc -= 1;

  if (argc > 0) {
    fprintf(stderr, "too many parameters\n");
    return PROG_EXIT_SYNTAX;
  }

  return cldrParseFile(inputFile, handleAnnotation, NULL)?
         PROG_EXIT_SUCCESS:
         PROG_EXIT_FATAL;
}
