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
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#include "program.h"
#include "options.h"
#include "log.h"
#include "file.h"
#include "parse.h"
#include "system.h"

#undef ALLOW_DOS_OPTION_SYNTAX
#if defined(__MINGW32__) || defined(__MSDOS__)
#define ALLOW_DOS_OPTION_SYNTAX
#endif /* allow DOS syntax */

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

typedef struct {
  const OptionEntry *optionTable;
  unsigned int optionCount;

  unsigned char ensuredSettings[0X100];

  unsigned exitImmediately:1;
  unsigned warning:1;
  unsigned syntaxError:1;
} OptionProcessingInformation;

static int
wordMeansTrue (const char *word) {
  return strcasecmp(word, FLAG_TRUE_WORD) == 0;
}

static int
wordMeansFalse (const char *word) {
  return strcasecmp(word, FLAG_FALSE_WORD) == 0;
}

static int
extendSetting (char **setting, const char *value, int prepend) {
  if (value && *value) {
    if (!*setting) {
      if (!(*setting = strdup(value))) {
        logMallocError();
        return 0;
      }
    } else {
      size_t newSize = strlen(*setting) + 1 + strlen(value) + 1;
      char *newSetting = malloc(newSize);

      if (!newSetting) {
        logMallocError();
        return 0;
      }

      if (prepend) {
        snprintf(newSetting, newSize, "%s,%s", value, *setting);
      } else {
        snprintf(newSetting, newSize, "%s,%s", *setting, value);
      }

      free(*setting);
      *setting = newSetting;
    }
  }

  return 1;
}

static int
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
          if (!extendSetting(option->setting.string, value, 1)) return 0;
        } else {
          if (!(*option->setting.string = strdup(value))) {
            logMallocError();
            return 0;
          }
        }
      }
    } else {
      if (option->setting.flag) {
        if (wordMeansTrue(value)) {
          *option->setting.flag = 1;
        } else if (wordMeansFalse(value)) {
          *option->setting.flag = 0;
        } else if (!(option->flags & OPT_Extend)) {
          logMessage(LOG_ERR, "%s: %s", gettext("invalid flag setting"), value);
          info->warning = 1;
        } else {
          int count;
          if (isInteger(&count, value) && (count >= 0)) {
            *option->setting.flag = count;
          } else {
            logMessage(LOG_ERR, "%s: %s", gettext("invalid counter setting"), value);
            info->warning = 1;
          }
        }
      }
    }
  }

  return 1;
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

#ifdef ALLOW_DOS_OPTION_SYNTAX
  const char dosPrefix = '/';
  int dosSyntax = 0;
#endif /* ALLOW_DOS_OPTION_SYNTAX */

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
        opt += 1;

        if (!entry->argument && entry->setting.flag) {
          static const char *const noPrefix = "no-";
          size_t noLength = strlen(noPrefix);
          char *name;

          if (strncasecmp(noPrefix, entry->word, noLength) == 0) {
            name = strdup(&entry->word[noLength]);
          } else {
            size_t size = noLength + strlen(entry->word) + 1;

            if ((name = malloc(size))) {
              snprintf(name, size, "%s%s", noPrefix, entry->word);
            }
          }

          if (name) {
            opt->name = name;
            opt->has_arg = no_argument;
            opt->flag = &resetLetter;
            opt->val = entry->letter;
            opt += 1;
          } else {
            logMallocError();
          }
        }
      }
    }

    memset(opt, 0, sizeof(*opt));
  }
#endif /* HAVE_GETOPT_LONG */

  for (index=0; index<0X100; index+=1) optionEntries[index] = NULL;

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
#ifdef ALLOW_DOS_OPTION_SYNTAX
    if (*(*argumentVector)[1] == dosPrefix) dosSyntax = 1;
#endif /* ALLOW_DOS_OPTION_SYNTAX */

  opterr = 0;
  optind = 1;

  while (1) {
    int option;
    char prefix = '-';

    if (optind == *argumentCount) {
      option = -1;
    } else {
      char *argument = (*argumentVector)[optind];

#ifdef ALLOW_DOS_OPTION_SYNTAX
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
      } else
#endif /* ALLOW_DOS_OPTION_SYNTAX */

      if (reset) {
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
              *entry->setting.flag += 1;
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
        logMessage(LOG_ERR, "%s: %c%c", gettext("unknown option"), prefix, optopt);
        info->syntaxError = 1;
        break;

      case ':': /* An invalid option has been specified. */
        logMessage(LOG_ERR, "%s: %c%c", gettext("missing operand"), prefix, optopt);
        info->syntaxError = 1;
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
    info->exitImmediately = 1;
  }

#ifdef HAVE_GETOPT_LONG
  {
    struct option *opt = longOptions;

    while (opt->name) {
      if (opt->flag) free((char *)opt->name);
      opt += 1;
    }
  }
#endif /* HAVE_GETOPT_LONG */
}

static void
processBootParameters (
  OptionProcessingInformation *info,
  const char *parameter
) {
  const char *value;
  char *allocated = NULL;

  if (!(value = allocated = getBootParameters(parameter)))
    if (!(value = getenv(parameter)))
      return;

  {
    int count = 0;
    char **parameters = splitString(value, ',', &count);
    int optionIndex;

    for (optionIndex=0; optionIndex<info->optionCount; ++optionIndex) {
      const OptionEntry *option = &info->optionTable[optionIndex];

      if ((option->bootParameter) && (option->bootParameter <= count)) {
        char *parameter = parameters[option->bootParameter-1];

        if (*parameter) {
          {
            char *byte = parameter;

            do {
              if (*byte == '+') *byte = ',';
            } while (*++byte);
          }

          ensureSetting(info, option, parameter);
        }
      }
    }

    deallocateStrings(parameters);
  }

  if (allocated) free(allocated);
}

static void
processEnvironmentVariables (
  OptionProcessingInformation *info,
  const char *prefix
) {
  size_t prefixLength = strlen(prefix);
  unsigned int optionIndex;

  for (optionIndex=0; optionIndex<info->optionCount; optionIndex+=1) {
    const OptionEntry *option = &info->optionTable[optionIndex];

    if ((option->flags & OPT_Environ) && option->word) {
      size_t nameSize = prefixLength + 1 + strlen(option->word) + 1;
      char name[nameSize];

      snprintf(name, nameSize, "%s_%s", prefix, option->word);

      {
        char *character = name;

        while (*character) {
          if (*character == '-') {
            *character = '_';
	  } else if (islower(*character)) {
            *character = toupper(*character);
          }

          character += 1;
        }
      }

      {
        const char *setting = getenv(name);

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
            logMessage(LOG_ERR, "%s: %s", gettext("operand not supplied for configuration directive"), line);
            conf->info->warning = 1;
          } else if (strtok(NULL, delimiters)) {
            while (strtok(NULL, delimiters));
            logMessage(LOG_ERR, "%s: %s", gettext("too many operands for configuration directive"), line);
            conf->info->warning = 1;
          } else {
            char **setting = &conf->settings[optionIndex];

            if (*setting && !(option->argument && (option->flags & OPT_Extend))) {
              logMessage(LOG_ERR, "%s: %s", gettext("configuration directive specified more than once"), line);
              conf->info->warning = 1;

              free(*setting);
              *setting = NULL;
            }

            if (*setting) {
              if (!extendSetting(setting, operand, 0)) return 0;
            } else {
              if (!(*setting = strdup(operand))) {
                logMallocError();
                return 0;
              }
            }
          }

          return 1;
        }
      }
    }
    logMessage(LOG_ERR, "%s: %s", gettext("unknown configuration directive"), line);
    conf->info->warning = 1;
  }
  return 1;
}

static void
processConfigurationFile (
  OptionProcessingInformation *info,
  const char *path,
  int optional
) {
  FILE *file = openDataFile(path, "r", optional);

  if (file) {
    ConfigurationFileProcessingData conf;

    conf.info = info;
      ;
    if ((conf.settings = malloc(info->optionCount * sizeof(*conf.settings)))) {
      int processed;
      unsigned int index;

      for (index=0; index<info->optionCount; index+=1) conf.settings[index] = NULL;
      processed = processLines(file, processConfigurationLine, &conf);

      for (index=0; index<info->optionCount; index+=1) {
        char *setting = conf.settings[index];

        if (setting) {
          ensureSetting(info, &info->optionTable[index], setting);
          free(setting);
        }
      }

      if (!processed) {
        logMessage(LOG_ERR, gettext("file '%s' processing error."), path);
        info->warning = 1;
      }

      free(conf.settings);
    } else {
      logMallocError();
    }

    fclose(file);
  } else if (!optional || (errno != ENOENT)) {
    info->warning = 1;
  }
}

ProgramExitStatus
processOptions (const OptionsDescriptor *descriptor, int *argumentCount, char ***argumentVector) {
  OptionProcessingInformation info = {
    .optionTable = descriptor->optionTable,
    .optionCount = descriptor->optionCount,

    .exitImmediately = 0,
    .warning = 0,
    .syntaxError = 0
  };

  {
    int index;
    for (index=0; index<0X100; ++index) info.ensuredSettings[index] = 0;
  }

  beginProgram(*argumentCount, *argumentVector);
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

  if (info.exitImmediately) return PROG_EXIT_FORCE;
  if (info.syntaxError) return PROG_EXIT_SYNTAX;
  return PROG_EXIT_SUCCESS;
}
