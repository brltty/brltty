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
#include <ctype.h>
#include <errno.h>

#include "options.h"
#include "misc.h"
#include "system.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

const char *programPath;
const char *programName;

static void
extendSetting (char **setting, const char *operand, int prepend) {
  if (operand && *operand) {
    if (!*setting) {
      *setting = strdupWrapper(operand);
    } else if (prepend) {
      char *area = mallocWrapper(strlen(operand) + 1 + strlen(*setting) + 1);
      sprintf(area, "%s,%s", operand, *setting);
      free(*setting);
      *setting = area;
    } else {
      size_t length = strlen(*setting);
      *setting = reallocWrapper(*setting, length+1+strlen(operand)+1);
      sprintf((*setting)+length, ",%s", operand);
    }
  }
}

static void
ensureSetting (const OptionEntry *option, const char *operand) {
  if (operand) {
    if (option->flags & OPT_Extend) {
      extendSetting(option->setting, operand, 1);
    } else if (!*option->setting) {
      *option->setting = strdupWrapper(operand);
    }
  }
}

static void
processBootParameters (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  const char *parameterName
) {
  char *parameterString;
  int allocated = 0;

  if ((parameterString = getBootParameters(parameterName))) {
    allocated = 1;
  } else if ((parameterString = getenv(parameterName))) {
    allocated = 0;
  } else {
    return;
  }

  {
    int count = 0;
    char **parameters = splitString(parameterString, ',', &count);
    int optionIndex;

    for (optionIndex=0; optionIndex<optionCount; ++optionIndex) {
      const OptionEntry *option = &optionTable[optionIndex];
      if ((option->bootParameter) && (option->bootParameter <= count)) {
        char *parameter = parameters[option->bootParameter-1];
        if (*parameter) ensureSetting(option, parameter);
      }
    }

    deallocateStrings(parameters);
  }

  if (allocated) free(parameterString);
}

static void
processEnvironmentVariables (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  const char *prefix
) {
  int prefixLength = strlen(prefix);
  int optionIndex;
  for (optionIndex=0; optionIndex<optionCount; ++optionIndex) {
    const OptionEntry *option = &optionTable[optionIndex];
    if (option->flags & OPT_Environ) {
      unsigned char name[prefixLength + 1 + strlen(option->word) + 1];
      sprintf(name, "%s_%s", prefix, option->word);

      {
        unsigned char *character = name;
        while (*character) {
          if (*character == '-') {
            *character = '_';
	  } else if (islower(*character)) {
            *character = toupper(*character);
          }
          ++character;
        }
      }

      ensureSetting(option, getenv(name));
    }
  }
}

static void
setDefaultOptions (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  int config
) {
  int optionIndex;
  for (optionIndex=0; optionIndex<optionCount; ++optionIndex) {
    const OptionEntry *option = &optionTable[optionIndex];
    if (!(option->flags & OPT_Config) != !config) continue;
    ensureSetting(option, option->defaultSetting);
  }
}

typedef struct {
  unsigned int count;
  const OptionEntry *options;
  char **settings;
} ConfigurationFileProcessingData;

static int
processConfigurationLine (
  char *line,
  void *data
) {
  const ConfigurationFileProcessingData *conf = data;
  static const char *delimiters = " \t"; /* Characters which separate words. */
  char *directive; /* Points to first word of each line. */

  /* Remove comment from end of line. */
  {
    char *comment = strchr(line, '#');
    if (comment) *comment = 0;
  }

  if ((directive = strtok(line, delimiters))) {
    int optionIndex;
    for (optionIndex=0; optionIndex<conf->count; ++optionIndex) {
      const OptionEntry *option = &conf->options[optionIndex];
      if (option->flags & OPT_Config) {
        if (strcasecmp(directive, option->word) == 0) {
          const char *operand = strtok(NULL, delimiters);

          if (!operand) {
            LogPrint(LOG_ERR, "Operand not supplied for configuration directive: %s", line);
          } else if (strtok(NULL, delimiters)) {
            while (strtok(NULL, delimiters));
            LogPrint(LOG_ERR, "Too many operands for configuration directive: %s", line);
          } else {
            char **setting = &conf->settings[optionIndex];

            if (*setting && !(option->flags & OPT_Extend)) {
              LogPrint(LOG_ERR, "Configuration directive specified more than once: %s", line);
              free(*setting);
              *setting = NULL;
            }

            if (*setting) {
              extendSetting(setting, operand, 0);
            } else {
              *setting = strdupWrapper(operand);
            }
          }

          return 1;
        }
      }
    }
    LogPrint(LOG_ERR, "Unknown configuration directive: %s", line);
  }
  return 1;
}

static int
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
      int index;

      conf.count = optionCount;
      conf.options = optionTable;
      conf.settings = mallocWrapper(optionCount * sizeof(*conf.settings));
      for (index=0; index<optionCount; ++index) conf.settings[index] = NULL;

      processed = processLines(file, processConfigurationLine, &conf);

      for (index=0; index<optionCount; ++index) {
        char *setting = conf.settings[index];
        ensureSetting(&optionTable[index], setting);
        free(setting);
      }
      free(conf.settings);
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
  const char *applicationName,
  int *argumentCount,
  char ***argumentVector,
  int *doBootParameters,
  int *doEnvironmentVariables,
  char **configurationFile,
  const char *argumentsSummary
) {
  short opt_help = 0;
  short opt_helpAll = 0;
  char shortOptions[1 + (optionCount * 2) + 1];
  const OptionEntry *optionEntries[0X100];
  int index;

#ifdef HAVE_GETOPT_LONG
  struct option longOptions[optionCount + 1];
  {
    struct option *opt = longOptions;
    for (index=0; index<optionCount; ++index) {
      const OptionEntry *entry = &optionTable[index];
      opt->name = entry->word;
      opt->has_arg = entry->argument? required_argument: no_argument;
      opt->flag = NULL;
      opt->val = entry->letter;
      ++opt;
    }
    memset(opt, 0, sizeof(*opt));
  }
#endif /* HAVE_GETOPT_LONG */

  for (index=0; index<0X100; ++index) optionEntries[index] = NULL;

  {
    char *opt = shortOptions;
    *opt++ = '+';
    for (index=0; index<optionCount; ++index) {
      const OptionEntry *entry = &optionTable[index];
      *opt++ = entry->letter;
      if (entry->argument) *opt++ = ':';

      if (entry->setting) *entry->setting = NULL;
      optionEntries[entry->letter] = entry;
    }
    *opt = 0;
  }

  programPath = **argumentVector;
  programName = strrchr(programPath, '/');
  programName = programName? programName+1: programPath;

  /* Parse command line using getopt(): */
  opterr = 0;
  while (1) {
    int option;

#ifdef HAVE_GETOPT_LONG
    option = getopt_long(*argumentCount, *argumentVector, shortOptions, longOptions, NULL);
#else /* HAVE_GETOPT_LONG */
    option = getopt(*argumentCount, *argumentVector, shortOptions);
#endif /* HAVE_GETOPT_LONG */
    if (option == -1) break;

    /* continue on error as much as possible, as often we are typing blind
     * and won't even see the error message unless the display comes up.
     */
    switch (option) {
      default:
        {
          const OptionEntry *entry = optionEntries[option];
          if (!entry->setting) {
            /* We can't process it directly, let's defer to handleOption() */
            if (!handleOption(option)) {
              LogPrint(LOG_ERR, "Unhandled option: -%c", option);
            }
          } else if (entry->flags & OPT_Extend) {
            extendSetting(entry->setting, optarg, 0);
          } else {
            *entry->setting = optarg;
          }
        }
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
  *argumentVector += optind, *argumentCount -= optind;

  if (opt_help) {
    printHelp(optionTable, optionCount,
              stdout, 79,
              argumentsSummary, opt_helpAll);
    exit(0);
  }

  if (doBootParameters && *doBootParameters) {
    processBootParameters(optionTable, optionCount, applicationName);
  }

  if (doEnvironmentVariables && *doEnvironmentVariables) {
    processEnvironmentVariables(optionTable, optionCount, applicationName);
  }

  if (configurationFile) {
    int optional = !*configurationFile;
    setDefaultOptions(optionTable, optionCount, 0);
    processConfigurationFile(optionTable, optionCount, *configurationFile, optional);
  } else {
    setDefaultOptions(optionTable, optionCount, 0);
  }
  setDefaultOptions(optionTable, optionCount, 1);

  return 1;
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
    if (strncasecmp(argument, *choice, length) == 0) return choice - choices;
    ++choice;
  }
  fprintf(stderr, "%s: Invalid %s: %s\n", programName, name, argument);
  exit(2);
}
