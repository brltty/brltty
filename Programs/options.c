/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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
#include <limits.h>

#ifdef ENABLE_I18N_SUPPORT
#include <locale.h>
#endif /* ENABLE_I18N_SUPPORT */

#include "options.h"
#include "misc.h"
#include "system.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

const char *programPath;
const char *programName;

typedef char **StringSetting;
typedef int *FlagSetting;

typedef struct {
  const OptionEntry *optionTable;
  unsigned int optionCount;
  unsigned char ensuredSettings[0X100];
  int errorCount;
} OptionProcessingInformation;

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
  if (value) {
    unsigned char *ensured = &info->ensuredSettings[option->letter];
    if (!*ensured) {
      *ensured = 1;

      if (option->argument) {
        StringSetting setting = option->setting;
        if (option->flags & OPT_Extend) {
          extendSetting(setting, value, 1);
        } else {
          *setting = strdupWrapper(value);
        }
      } else {
        FlagSetting setting = option->setting;
        if (strcasecmp(value, "on") == 0) {
          *setting = 1;
        } else if (strcasecmp(value, "off") == 0) {
          *setting = 0;
        } else if (!(option->flags & OPT_Extend)) {
          LogPrint(LOG_ERR, "%s: %s", gettext("invalid flag setting"), value);
          info->errorCount++;
        } else {
          int count;
          if (isInteger(&count, value) && (count >= 0)) {
            *setting = count;
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
  unsigned int argumentWidth = 0;
  int optionIndex;

#ifdef HAVE_GETOPT_LONG
  unsigned int wordWidth = 0;
#endif /* HAVE_GETOPT_LONG */

  for (optionIndex=0; optionIndex<info->optionCount; ++optionIndex) {
    const OptionEntry *option = &info->optionTable[optionIndex];

#ifdef HAVE_GETOPT_LONG
    if (option->word) wordWidth = MAX(wordWidth, strlen(option->word));
#endif /* HAVE_GETOPT_LONG */

    if (option->argument) argumentWidth = MAX(argumentWidth, strlen(option->argument));
  }

  fputs(gettext("Usage"), outputStream);
  fprintf(outputStream, ": %s", programName);
  if (info->optionCount)
    fputs(" [", outputStream);
    fputs(gettext("option"), outputStream);
    fputs(" ...]", outputStream);
  if (argumentsSummary && *argumentsSummary)
    fprintf(outputStream, " %s", argumentsSummary);
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
      const char *description = gettext(option->description);
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
        snprintf(buffer, sizeof(buffer), gettext(option->description),
                 strings[0], strings[1], strings[2], strings[3]);
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
  int opt_help = 0;
  int opt_helpAll = 0;

  const OptionEntry *optionEntries[0X100];
  char shortOptions[1 + (info->optionCount * 2) + 1];
  int index;

#ifdef HAVE_GETOPT_LONG
  struct option longOptions[(info->optionCount * 2) + 1];
  int flagLetter;

  {
    struct option *opt = longOptions;
    for (index=0; index<info->optionCount; ++index) {
      const OptionEntry *entry = &info->optionTable[index];

      opt->name = entry->word;
      opt->has_arg = entry->argument? required_argument: no_argument;
      opt->flag = NULL;
      opt->val = entry->letter;
      ++opt;

      if (!entry->argument && entry->setting) {
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
        opt->flag = &flagLetter;
        opt->val = entry->letter;
        ++opt;
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

      if (entry->setting) {
        if (entry->argument) {
          StringSetting setting = entry->setting;
          *setting = NULL;
        } else {
          FlagSetting setting = entry->setting;
          *setting = 0;
        }
      }
    }

    *opt = 0;
  }

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
      default: {
        const OptionEntry *entry = optionEntries[option];

        if (entry->argument) {
          StringSetting setting = entry->setting;
          if (entry->flags & OPT_Extend) {
            extendSetting(setting, optarg, 0);
          } else {
            *setting = optarg;
          }
        } else {
          FlagSetting setting = entry->setting;
          if (entry->flags & OPT_Extend) {
            ++*setting;
          } else {
            *setting = 1;
          }
        }

        info->ensuredSettings[option] = 1;
        break;
      }

#ifdef HAVE_GETOPT_LONG
      case 0: {
        const OptionEntry *entry = optionEntries[flagLetter];
        FlagSetting setting = entry->setting;
        *setting = 0;
        info->ensuredSettings[flagLetter] = 1;
        break;
      }
#endif /* HAVE_GETOPT_LONG */

      case '?':
        LogPrint(LOG_ERR, "%s: -%c", gettext("unknown option"), optopt);
        info->errorCount++;
        break;

      case ':': /* An invalid option has been specified. */
        LogPrint(LOG_ERR, "%s: -%c", gettext("missing operand"), optopt);
        info->errorCount++;
        break;

      case 'H':                /* help */
        opt_helpAll = 1;
      case 'h':                /* help */
        opt_help = 1;
        break;
    }
  }
  *argumentVector += optind, *argumentCount -= optind;

  if (opt_help) {
    printHelp(info, stdout, 79, argumentsSummary, opt_helpAll);
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
        char *parameter = parameters[option->bootParameter-1];
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
    if (option->flags & OPT_Environ) {
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

      ensureSetting(info, option, getenv(name));
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
    if (!(option->flags & OPT_Config) != !config) continue;
    ensureSetting(info, option, option->defaultSetting);
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
      if (option->flags & OPT_Config) {
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
  FILE *file = fopen(path, "r");
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
        ensureSetting(info, &info->optionTable[index], setting);
        free(setting);
      }
      free(conf.settings);
    }

    fclose(file);
    if (processed) return 1;
    LogPrint(LOG_ERR, gettext("file '%s' processing error."), path);
    info->errorCount++;
  } else {
    int ok = optional && (errno == ENOENT);
    LogPrint((ok? LOG_DEBUG: LOG_ERR), "%s: %s: %s",
             gettext("cannot open configuration file"),
             path, strerror(errno));
    if (ok) return 1;
    info->errorCount++;
  }
  return 0;
}

#ifdef WINDOWS
#include "sys_windows.h"
#endif /* WINDOWS */
int
processOptions (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  const char *applicationName,
  int *argumentCount,
  char ***argumentVector,
  int *doBootParameters,
  int *doEnvironmentVariables,
  char **configurationFile,
  const char *argumentsSummary
) {
  OptionProcessingInformation info;
  int index;

#ifdef WINDOWS
  sysInit();
#endif /* WINDOWS */

#ifdef ENABLE_I18N_SUPPORT
  setlocale(LC_ALL, "");
  textdomain(PACKAGE_NAME);
#endif /* ENABLE_I18N_SUPPORT */

  info.optionTable = optionTable;
  info.optionCount = optionCount;
  for (index=0; index<0X100; ++index) info.ensuredSettings[index] = 0;
  info.errorCount = 0;

  if (!(programPath = getProgramPath())) {
    programPath = **argumentVector;

#if defined(HAVE_REALPATH) && defined(PATH_MAX)
    {
      char buffer[PATH_MAX];
      char *path = realpath(programPath, buffer);

      if (path) {
        programPath = strdupWrapper(path);
      } else {
        LogError("realpath");
      }
    }
#endif /* defined(HAVE_REALPATH) && defined(PATH_MAX) */

    if (!isAbsolutePath(programPath)) {
      char *directory = getWorkingDirectory();
      char buffer[strlen(directory) + 1 + strlen(programPath) + 1];
      sprintf(buffer, "%s/%s", directory, programPath);
      programPath = strdupWrapper(buffer);
      free(directory);
    }
  }

  programName = strrchr(programPath, '/');
  programName = programName? programName+1: programPath;
  setPrintPrefix(programName);

  processCommandLine(&info, argumentCount, argumentVector, argumentsSummary);
  {
    int configurationFileSpecified = configurationFile && *configurationFile;

    if (doBootParameters && *doBootParameters)
      processBootParameters(&info, applicationName);

    if (doEnvironmentVariables && *doEnvironmentVariables)
      processEnvironmentVariables(&info, applicationName);

    setDefaultOptions(&info, 0);
    if (configurationFile && *configurationFile) {
      char **const paths[] = {
        configurationFile,
        NULL
      };
      fixInstallPaths(paths);
      processConfigurationFile(&info, *configurationFile, !configurationFileSpecified);
    }
    setDefaultOptions(&info, 1);
  }

  return info.errorCount;
}

void
fixInstallPaths (char **const *paths) {
  char *programDirectory = getPathDirectory(programPath);
  while (*paths) {
    **paths = makePath(programDirectory, **paths);
    ++paths;
  }
  free(programDirectory);
}
