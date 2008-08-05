/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#include "program.h"
#include "options.h"
#include "misc.h"
#include "system.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

typedef struct {
  const OptionEntry *optionTable;
  unsigned int optionCount;
  unsigned char ensuredSettings[0X100];
  int errorCount;
} OptionProcessingInformation;

static int
wordMeansTrue (const char *word) {
  return strcasecmp(word, FLAG_TRUE_WORD) == 0;
}

static int
wordMeansFalse (const char *word) {
  return strcasecmp(word, FLAG_FALSE_WORD) == 0;
}

static void
extendSetting (char **setting, const char *value, int prepend) {
  if (value && *value) {
    if (!*setting) {
      *setting = strdupWrapper(value);
    } else if (prepend) {
      char *area = mallocWrapper(strlen(value) + 1 + strlen(*setting) + 1);
      sprintf(area, "%s,%s", value, *setting);
      free(*setting);
      *setting = area;
    } else {
      size_t length = strlen(*setting);
      *setting = reallocWrapper(*setting, length+1+strlen(value)+1);
      sprintf((*setting)+length, ",%s", value);
    }
  }
}

static void
ensureSetting (
  OptionProcessingInformation *info,
  const OptionEntry *option,
  const char *value
) {
  unsigned char *ensured = &info->ensuredSettings[option->letter];

  if (!*ensured) {
    *ensured = 1;

    if (option->argument) {
      if (option->setting.string) {
        if (option->flags & OPT_Extend) {
          extendSetting(option->setting.string, value, 1);
        } else {
          *option->setting.string = strdupWrapper(value);
        }
      }
    } else {
      if (option->setting.flag) {
        if (wordMeansTrue(value)) {
          *option->setting.flag = 1;
        } else if (wordMeansFalse(value)) {
          *option->setting.flag = 0;
        } else if (!(option->flags & OPT_Extend)) {
          LogPrint(LOG_ERR, "%s: %s", gettext("invalid flag setting"), value);
          info->errorCount++;
        } else {
          int count;
          if (isInteger(&count, value) && (count >= 0)) {
            *option->setting.flag = count;
          } else {
            LogPrint(LOG_ERR, "%s: %s", gettext("invalid counter setting"), value);
            info->errorCount++;
          }
        }
      }
    }
  }
}

static void
printHelp (
  OptionProcessingInformation *info,
  FILE *outputStream,
  unsigned int lineWidth,
  const char *argumentsSummary,
  int all
) {
  char line[lineWidth+1];
  unsigned int wordWidth = 0;
  unsigned int argumentWidth = 0;
  int optionIndex;

  for (optionIndex=0; optionIndex<info->optionCount; ++optionIndex) {
    const OptionEntry *option = &info->optionTable[optionIndex];
    if (option->word) wordWidth = MAX(wordWidth, strlen(option->word));
    if (option->argument) argumentWidth = MAX(argumentWidth, strlen(option->argument));
  }

  fputs(gettext("Usage"), outputStream);
  fprintf(outputStream, ": %s", programName);
  if (info->optionCount) {
    fputs(" [", outputStream);
    fputs(gettext("option"), outputStream);
    fputs(" ...]", outputStream);
  }
  if (argumentsSummary && *argumentsSummary) {
    fprintf(outputStream, " %s", argumentsSummary);
  }
  fprintf(outputStream, "\n");

  for (optionIndex=0; optionIndex<info->optionCount; ++optionIndex) {
    const OptionEntry *option = &info->optionTable[optionIndex];
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

    line[lineLength++] = ' ';
    {
      unsigned int headerWidth = lineLength;
      unsigned int descriptionWidth = lineWidth - headerWidth;
      const char *description = option->description? gettext(option->description): "";
      char buffer[0X100];

      if (option->strings) {
        int index = 0;
        int count = 4;
        const char *strings[count];

        while (option->strings[index]) {
          strings[index] = option->strings[index];
          ++index;
        }

        while (index < count) strings[index++] = NULL;
        snprintf(buffer, sizeof(buffer),
                 description, strings[0], strings[1], strings[2], strings[3]);
        description = buffer;
      }

      {
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
}

static void
processCommandLine (
  OptionProcessingInformation *info,
  int *argumentCount,
  char ***argumentVector,
  const char *argumentsSummary
) {
  int lastOptInd = -1;
  int index;

  const char resetPrefix = '+';
  const char *reset = NULL;
  int resetLetter;

  const char dosPrefix = '/';
  int dosSyntax = 0;

  int optHelp = 0;
  int optHelpAll = 0;
  const OptionEntry *optionEntries[0X100];
  char shortOptions[1 + (info->optionCount * 2) + 1];

#ifdef HAVE_GETOPT_LONG
  struct option longOptions[(info->optionCount * 2) + 1];

  {
    struct option *opt = longOptions;
    for (index=0; index<info->optionCount; ++index) {
      const OptionEntry *entry = &info->optionTable[index];

      if (entry->word) {
        opt->name = entry->word;
        opt->has_arg = entry->argument? required_argument: no_argument;
        opt->flag = NULL;
        opt->val = entry->letter;
        ++opt;

        if (!entry->argument && entry->setting.flag) {
          static const char *prefix = "no-";
          int length = strlen(prefix);

          if (strncasecmp(prefix, entry->word, length) == 0) {
            opt->name = strdupWrapper(entry->word + length);
          } else {
            char *name = mallocWrapper(length + strlen(entry->word) + 1);
            sprintf(name, "%s%s", prefix, entry->word);
            opt->name = name;
          }

          opt->has_arg = no_argument;
          opt->flag = &resetLetter;
          opt->val = entry->letter;
          ++opt;
        }
      }
    }

    memset(opt, 0, sizeof(*opt));
  }
#endif /* HAVE_GETOPT_LONG */

  for (index=0; index<0X100; ++index) optionEntries[index] = NULL;

  {
    char *opt = shortOptions;
    *opt++ = '+';

    for (index=0; index<info->optionCount; ++index) {
      const OptionEntry *entry = &info->optionTable[index];
      optionEntries[entry->letter] = entry;

      *opt++ = entry->letter;
      if (entry->argument) *opt++ = ':';

      if (entry->argument) {
        if (entry->setting.string) *entry->setting.string = NULL;
      } else {
        if (entry->setting.flag) *entry->setting.flag = 0;
      }
    }

    *opt = 0;
  }

  if (*argumentCount > 1)
    if (*(*argumentVector)[1] == dosPrefix)
      dosSyntax = 1;

  opterr = 0;
  optind = 1;

  while (1) {
    int option;
    char prefix = '-';

    if (optind == *argumentCount) {
      option = -1;
    } else {
      char *argument = (*argumentVector)[optind];

      if (dosSyntax) {
        prefix = dosPrefix;
        optind++;

        if (*argument != dosPrefix) {
          option = -1;
        } else {
          char *name = argument + 1;
          size_t nameLength = strcspn(name, ":");
          char *value = (nameLength == strlen(name))? NULL: (name + nameLength + 1);
          const OptionEntry *entry;

          if (nameLength == 1) {
            entry = optionEntries[option = *name];
          } else {
            int count = info->optionCount;
            entry = info->optionTable;
            option = -1;

            while (count--) {
              if (entry->word) {
                size_t wordLength = strlen(entry->word);

                if ((wordLength == nameLength) &&
                    (strncasecmp(entry->word, name, wordLength) == 0)) {
                  option = entry->letter;
                  break;
                }
              }

              entry++;
            }

            if (option < 0) {
              option = 0;
              entry = NULL;
            }
          }

          optopt = option;
          optarg = NULL;

          if (!entry) {
            option = '?';
          } else if (entry->argument) {
            if (!(optarg = value)) option = ':';
          } else if (value) {
            if (!entry->setting.flag) goto dosBadFlagValue;

            if (!wordMeansTrue(value)) {
              if (wordMeansFalse(value)) {
                resetLetter = option;
                option = 0;
              } else {
              dosBadFlagValue:
                option = '?';
              }
            }
          }
        }
      } else if (reset) {
        prefix = resetPrefix;

        if (!(option = *reset++)) {
          reset = NULL;
          optind++;
          continue;
        }

        {
          const OptionEntry *entry = optionEntries[option];
          if (entry && !entry->argument && entry->setting.flag) {
            resetLetter = option;
            option = 0;
          } else {
            optopt = option;
            option = '?';
          }
        }
      } else {
        if (optind != lastOptInd) {
          lastOptInd = optind;
          if ((reset = (*argument == resetPrefix)? argument+1: NULL)) continue;
        }

#ifdef HAVE_GETOPT_LONG
        option = getopt_long(*argumentCount, *argumentVector, shortOptions, longOptions, NULL);
#else /* HAVE_GETOPT_LONG */
        option = getopt(*argumentCount, *argumentVector, shortOptions);
#endif /* HAVE_GETOPT_LONG */
      }
    }
    if (option == -1) break;

    /* continue on error as much as possible, as often we are typing blind
     * and won't even see the error message unless the display comes up.
     */
    switch (option) {
      default: {
        const OptionEntry *entry = optionEntries[option];

        if (entry->argument) {
          if (!*optarg) {
            info->ensuredSettings[option] = 0;
            break;
          }

          if (entry->setting.string) {
            if (entry->flags & OPT_Extend) {
              extendSetting(entry->setting.string, optarg, 0);
            } else {
              *entry->setting.string = optarg;
            }
          }
        } else {
          if (entry->setting.flag) {
            if (entry->flags & OPT_Extend) {
              ++*entry->setting.flag;
            } else {
              *entry->setting.flag = 1;
            }
          }
        }

        info->ensuredSettings[option] = 1;
        break;
      }

      case 0: {
        const OptionEntry *entry = optionEntries[resetLetter];
        *entry->setting.flag = 0;
        info->ensuredSettings[resetLetter] = 1;
        break;
      }

      case '?':
        LogPrint(LOG_ERR, "%s: %c%c", gettext("unknown option"), prefix, optopt);
        info->errorCount++;
        break;

      case ':': /* An invalid option has been specified. */
        LogPrint(LOG_ERR, "%s: %c%c", gettext("missing operand"), prefix, optopt);
        info->errorCount++;
        break;

      case 'H':                /* help */
        optHelpAll = 1;
      case 'h':                /* help */
        optHelp = 1;
        break;
    }
  }
  *argumentVector += optind, *argumentCount -= optind;

  if (optHelp) {
    printHelp(info, stdout, 79, argumentsSummary, optHelpAll);
    exit(0);
  }

#ifdef HAVE_GETOPT_LONG
  {
    struct option *opt = longOptions;
    while (opt->name) {
      if (opt->flag) free((char *)opt->name);
      ++opt;
    }
  }
#endif /* HAVE_GETOPT_LONG */
}

static void
processBootParameters (
  OptionProcessingInformation *info,
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

    for (optionIndex=0; optionIndex<info->optionCount; ++optionIndex) {
      const OptionEntry *option = &info->optionTable[optionIndex];
      if ((option->bootParameter) && (option->bootParameter <= count)) {
        const char *parameter = parameters[option->bootParameter-1];
        if (*parameter) ensureSetting(info, option, parameter);
      }
    }

    deallocateStrings(parameters);
  }

  if (allocated) free(parameterString);
}

static void
processEnvironmentVariables (
  OptionProcessingInformation *info,
  const char *prefix
) {
  int prefixLength = strlen(prefix);
  int optionIndex;
  for (optionIndex=0; optionIndex<info->optionCount; ++optionIndex) {
    const OptionEntry *option = &info->optionTable[optionIndex];
    if ((option->flags & OPT_Environ) && option->word) {
      char name[prefixLength + 1 + strlen(option->word) + 1];
      sprintf(name, "%s_%s", prefix, option->word);

      {
        char *character = name;
        while (*character) {
          if (*character == '-') {
            *character = '_';
	  } else if (islower(*character)) {
            *character = toupper(*character);
          }
          ++character;
        }
      }

      {
        char *setting = getenv(name);
        if (setting && *setting) ensureSetting(info, option, setting);
      }
    }
  }
}

static void
setDefaultOptions (
  OptionProcessingInformation *info,
  int config
) {
  int optionIndex;
  for (optionIndex=0; optionIndex<info->optionCount; ++optionIndex) {
    const OptionEntry *option = &info->optionTable[optionIndex];
    if (!(option->flags & OPT_Config) == !config) {
      const char *setting = option->defaultSetting;
      if (!setting) setting = option->argument? "": FLAG_FALSE_WORD;
      ensureSetting(info, option, setting);
    }
  }
}

typedef struct {
  OptionProcessingInformation *info;
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
    for (optionIndex=0; optionIndex<conf->info->optionCount; ++optionIndex) {
      const OptionEntry *option = &conf->info->optionTable[optionIndex];
      if ((option->flags & OPT_Config) && option->word) {
        if (strcasecmp(directive, option->word) == 0) {
          const char *operand = strtok(NULL, delimiters);

          if (!operand) {
            LogPrint(LOG_ERR, "%s: %s", gettext("operand not supplied for configuration directive"), line);
            conf->info->errorCount++;
          } else if (strtok(NULL, delimiters)) {
            while (strtok(NULL, delimiters));
            LogPrint(LOG_ERR, "%s: %s", gettext("too many operands for configuration directive"), line);
            conf->info->errorCount++;
          } else {
            char **setting = &conf->settings[optionIndex];

            if (*setting && !(option->argument && (option->flags & OPT_Extend))) {
              LogPrint(LOG_ERR, "%s: %s", gettext("configuration directive specified more than once"), line);
              conf->info->errorCount++;
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
    LogPrint(LOG_ERR, "%s: %s", gettext("unknown configuration directive"), line);
    conf->info->errorCount++;
  }
  return 1;
}

static int
processConfigurationFile (
  OptionProcessingInformation *info,
  const char *path,
  int optional
) {
  FILE *file = openDataFile(path, "r", optional);
  if (file != NULL) { /* The configuration file has been successfully opened. */
    int processed;

    {
      ConfigurationFileProcessingData conf;
      int index;

      conf.info = info;
      conf.settings = mallocWrapper(info->optionCount * sizeof(*conf.settings));
      for (index=0; index<info->optionCount; ++index) conf.settings[index] = NULL;

      processed = processLines(file, processConfigurationLine, &conf);

      for (index=0; index<info->optionCount; ++index) {
        char *setting = conf.settings[index];
        if (setting) {
          ensureSetting(info, &info->optionTable[index], setting);
          free(setting);
        }
      }
      free(conf.settings);
    }

    fclose(file);
    if (processed) return 1;
    LogPrint(LOG_ERR, gettext("file '%s' processing error."), path);
    info->errorCount++;
  } else {
    if (optional && (errno == ENOENT)) return 1;
    info->errorCount++;
  }
  return 0;
}

int
processOptions (const OptionsDescriptor *descriptor, int *argumentCount, char ***argumentVector) {
  OptionProcessingInformation info = {
    .optionTable = descriptor->optionTable,
    .optionCount = descriptor->optionCount,
    .errorCount = 0
  };

  {
    int index;
    for (index=0; index<0X100; ++index) info.ensuredSettings[index] = 0;
  }

  prepareProgram(*argumentCount, *argumentVector);
  processCommandLine(&info, argumentCount, argumentVector, descriptor->argumentsSummary);

  {
    int configurationFileSpecified = descriptor->configurationFile && *descriptor->configurationFile;

    if (descriptor->doBootParameters && *descriptor->doBootParameters)
      processBootParameters(&info, descriptor->applicationName);

    if (descriptor->doEnvironmentVariables && *descriptor->doEnvironmentVariables)
      processEnvironmentVariables(&info, descriptor->applicationName);

    setDefaultOptions(&info, 0);
    if (descriptor->configurationFile && *descriptor->configurationFile) {
      char *configurationFile = *descriptor->configurationFile;
      fixInstallPath(&configurationFile);
      processConfigurationFile(&info, configurationFile, !configurationFileSpecified);
      if (configurationFile != *descriptor->configurationFile) free(configurationFile);
    }
    setDefaultOptions(&info, 1);
  }

  return info.errorCount;
}
