/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include "misc.h"
#include "options.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

const char *programPath;
const char *programName;

static void
printHelp (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  FILE *outputStream,
  unsigned int lineWidth,
  const char *argumentsDescription,
  int all
) {
   char line[lineWidth+1];
#ifdef HAVE_GETOPT_LONG
   unsigned int wordWidth = 0;
#endif /* HAVE_GETOPT_LONG */
   unsigned int argumentWidth = 0;
   int optionIndex;
   for (optionIndex=0; optionIndex<optionCount; ++optionIndex) {
      const OptionEntry *option = &optionTable[optionIndex];
#ifdef HAVE_GETOPT_LONG
      if (option->word) wordWidth = MAX(wordWidth, strlen(option->word));
#endif /* HAVE_GETOPT_LONG */
      if (option->argument) argumentWidth = MAX(argumentWidth, strlen(option->argument));
   }

   fprintf(outputStream, "Usage: %s [option ...]", programName);
   if (argumentsDescription && *argumentsDescription)
     fprintf(outputStream, " %s", argumentsDescription);
   fprintf(outputStream, "\n");

   for (optionIndex=0; optionIndex<optionCount; ++optionIndex) {
      const OptionEntry *option = &optionTable[optionIndex];
      unsigned int lineLength = 0;
      if (!all && (option->flags & OPT_Hidden)) continue;

      line[lineLength++] = '-';
      line[lineLength++] = option->letter;
      line[lineLength++] = ' ';

      {
         unsigned int end = lineLength + argumentWidth;
         if (option->argument) {
            size_t argumentLength = strlen(option->argument);
            memcpy(line+lineLength, option->argument, argumentLength);
            lineLength += argumentLength;
         }
         while (lineLength < end) line[lineLength++] = ' ';
      }
      line[lineLength++] = ' ';

#ifdef HAVE_GETOPT_LONG
      {
         unsigned int end = lineLength + 2 + wordWidth + 1;
         if (option->word) {
            size_t wordLength = strlen(option->word);
            line[lineLength++] = '-';
            line[lineLength++] = '-';
            memcpy(line+lineLength, option->word, wordLength);
            lineLength += wordLength;
            if (option->argument) line[lineLength++] = '=';
         }
         while (lineLength < end) line[lineLength++] = ' ';
      }
      line[lineLength++] = ' ';
#endif /* HAVE_GETOPT_LONG */

      line[lineLength++] = ' ';
      {
         unsigned int headerWidth = lineLength;
         unsigned int descriptionWidth = lineWidth - headerWidth;
         const char *description = option->description;
         unsigned int charsLeft = strlen(description);
         while (1) {
            unsigned int charCount = charsLeft;
            if (charCount > descriptionWidth) {
               charCount = descriptionWidth;
               while (description[charCount] != ' ') --charCount;
               while (description[charCount] == ' ') --charCount;
               ++charCount;
            }
            memcpy(line+lineLength, description, charCount);
            lineLength += charCount;

            line[lineLength] = 0;
            fprintf(outputStream, "%s\n", line);

            while (description[charCount] == ' ') ++charCount;
            if (!(charsLeft -= charCount)) break;
            description += charCount;

            lineLength = 0;
            while (lineLength < headerWidth) line[lineLength++] = ' ';
         }
      }
   }
}

int
processOptions (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  OptionHandler handleOption,
  int *argc,
  char ***argv,
  const char *argumentsSummary
) {
  short opt_help = 0;
  short opt_helpAll = 0;
  char shortOptions[1 + (optionCount * 2) + 1];

#ifdef HAVE_GETOPT_LONG
  struct option longOptions[optionCount + 1];
  {
    struct option *opt = longOptions;
    int index;
    for (index=0; index<optionCount; ++index) {
      const OptionEntry *option = &optionTable[index];
      opt->name = option->word;
      opt->has_arg = option->argument? required_argument: no_argument;
      opt->flag = NULL;
      opt->val = option->letter;
      ++opt;
    }
    memset(opt, 0, sizeof(*opt));
  }
#endif /* HAVE_GETOPT_LONG */

  {
    char *opt = shortOptions;
    int index;
    *opt++ = '+';
    for (index=0; index<optionCount; ++index) {
      const OptionEntry *option = &optionTable[index];
      *opt++ = option->letter;
      if (option->argument) *opt++ = ':';
    }
    *opt = 0;
  }

  programPath = **argv;
  programName = strrchr(programPath, '/');
  programName = programName? programName+1: programPath;

  /* Parse command line using getopt(): */
  opterr = 0;
  while (1) {
    int option;
#ifdef HAVE_GETOPT_LONG
    option = getopt_long(*argc, *argv, shortOptions, longOptions, NULL);
#else /* HAVE_GETOPT_LONG */
    option = getopt(*argc, *argv, shortOptions);
#endif /* HAVE_GETOPT_LONG */
    if (option == -1) break;

    /* continue on error as much as possible, as often we are typing blind
     * and won't even see the error message unless the display comes up.
     */
    switch (option) {
      default:
        if (!handleOption(option))
          LogPrint(LOG_ERR, "Unimplemented option: -%c", option);
        break;
      case '?':
        LogPrint(LOG_ERR, "Unknown option: -%c", optopt);
        return 0;
      case ':': /* An invalid option has been specified. */
        LogPrint(LOG_ERR, "Missing operand: -%c", optopt);
        return 0;
      case 'H':                /* help */
        opt_helpAll = 1;
      case 'h':                /* help */
        opt_help = 1;
        break;
    }
  }

  if (opt_help) {
    printHelp(optionTable, optionCount,
              stdout, 79,
              argumentsSummary, opt_helpAll);
    exit(0);
  }

  *argv += optind; *argc -= optind;
  return 1;
}

typedef struct {
  const OptionEntry *optionTable;
  unsigned int optionCount;
} ConfigurationFileProcessingData;

static int
processConfigurationLine (
  char *line,
  void *data
) {
  const ConfigurationFileProcessingData *conf = data;
  static const char *delimiters = " \t"; /* Characters which separate words. */
  char *keyword; /* Points to first word of each line. */

  /* Remove comment from end of line. */
  {
    char *comment = strchr(line, '#');
    if (comment) *comment = 0;
  }

  if ((keyword = strtok(line, delimiters))) {
    int optionIndex;
    for (optionIndex=0; optionIndex<conf->optionCount; ++optionIndex) {
      const OptionEntry *option = &conf->optionTable[optionIndex];
      if (option->configure) {
        if (strcasecmp(keyword, option->word) == 0) {
          ConfigurationLineStatus status = option->configure(delimiters);
          switch (status) {
            case CFG_OK:
              return 1;

            case CFG_NoValue:
              LogPrint(LOG_ERR,
                       "Operand not supplied for configuration item '%s'.",
                       keyword);
              break;

            case CFG_BadValue:
              LogPrint(LOG_ERR,
                       "Invalid operand specified"
                       " for configuration item '%s'.",
                        keyword);
              break;

            case CFG_TooMany:
              LogPrint(LOG_ERR,
                       "Too many operands supplied"
                       " for configuration item '%s'.",
                       keyword);
              break;

            case CFG_Duplicate:
              LogPrint(LOG_ERR,
                       "Configuration item '%s' specified more than once.",
                       keyword);
              break;

            default:
              LogPrint(LOG_ERR,
                       "Internal error: unsupported configuration line status: %d",
                       status);
              break;
          }
          return 1;
        }
      }
    }
    LogPrint(LOG_ERR, "Unknown configuration item: '%s'.", keyword);
    return 1;
  }
  return 1;
}

int
processConfigurationFile (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  const char *path,
  int optional
) {
  FILE *file = fopen(path, "r");
  if (file != NULL) { /* The configuration file has been successfully opened. */
    int processed;
    {
      ConfigurationFileProcessingData conf;
      conf.optionTable = optionTable;
      conf.optionCount = optionCount;
      processed = processLines(file, processConfigurationLine, &conf);
    }
    fclose(file);
    if (processed) return 1;
    LogPrint(LOG_ERR, "File '%s' processing error.", path);
  } else {
    int ok = optional && (errno == ENOENT);
    LogPrint((ok? LOG_DEBUG: LOG_ERR),
             "Cannot open configuration file: %s: %s",
             path, strerror(errno));
    if (ok) return 1;
  }
  return 0;
}

short
integerArgument (
  const char *argument,
  short minimum,
  short maximum,
  const char *name
) {
  char *end;
  long int value = strtol(argument, &end, 0);
  if ((end == argument) || *end) {
    fprintf(stderr, "%s: Invalid %s: %s\n", programName, name, argument);
  } else if (value < minimum) {
    fprintf(stderr, "%s: %s is less than %d: %ld\n", programName, name, minimum, value);
  } else if (value > maximum) {
    fprintf(stderr, "%s: %s is greater than %d: %ld\n", programName, name, maximum, value);
  } else {
    return value;
  }
  exit(2);
}

unsigned int
wordArgument (
  const char *argument,
  const char *const *choices,
  const char *name
) {
  size_t length = strlen(argument);
  const char *const *choice = choices;
  while (*choice) {
    if (strlen(*choice) >= length)
      if (strncasecmp(*choice, argument, length) == 0)
        return choice - choices;
    ++choice;
  }
  fprintf(stderr, "%s: Invalid %s: %s\n", programName, name, argument);
  exit(2);
}
