/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#include "program.h"
#include "options.h"
#include "params.h"
#include "log.h"
#include "file.h"
#include "datafile.h"
#include "utf8.h"
#include "parse.h"

#undef ALLOW_DOS_OPTION_SYNTAX
#if defined(__MINGW32__) || defined(__MSDOS__)
#define ALLOW_DOS_OPTION_SYNTAX
#endif /* allow DOS syntax */

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

typedef struct {
  const OptionEntry *const optionTable;
  unsigned int const optionCount;
  uint8_t *const ensuredSettings;

  uint8_t exitImmediately:1;
  uint8_t warning:1;
  uint8_t syntaxError:1;
} OptionProcessingInformation;

static int
hasExtendableArgument (const OptionEntry *option) {
  return option->argument && (option->flags & OPT_Extend);
}

static uint8_t *
getEnsuredSetting (
  const OptionProcessingInformation *info,
  const OptionEntry *option
) {
  return &info->ensuredSettings[option - info->optionTable];
}

static void
setEnsuredSetting (
  const OptionProcessingInformation *info,
  const OptionEntry *option,
  uint8_t yes
) {
  *getEnsuredSetting(info, option) = yes;
}

static int
ensureSetting (
  OptionProcessingInformation *info,
  const OptionEntry *option,
  const char *value
) {
  uint8_t *ensured = getEnsuredSetting(info, option);

  if (!*ensured || hasExtendableArgument(option)) {
    *ensured = 1;

    if (option->argument) {
      if (option->setting.string) {
        if (option->flags & OPT_Extend) {
          if (!extendStringSetting(option->setting.string, value, 1)) return 0;
        } else if (!changeStringSetting(option->setting.string, value)) {
          return 0;
        }
      }
    } else {
      if (option->setting.flag) {
        if (option->flags & OPT_Extend) {
          int count;

          if (isInteger(&count, value) && (count >= 0)) {
            *option->setting.flag = count;
          } else {
            logMessage(LOG_ERR, "%s: %s", gettext("invalid counter setting"), value);
            info->warning = 1;
          }
        } else {
          unsigned int on;

          if (validateFlagKeyword(&on, value)) {
            *option->setting.flag = on;
          } else {
            logMessage(LOG_ERR, "%s: %s", gettext("invalid flag setting"), value);
            info->warning = 1;
          }
        }
      }
    }
  }

  return 1;
}

static void
showUsage (
  OptionProcessingInformation *info,
  FILE *outputStream,
  unsigned int lineWidth,
  const char *argumentsSummary,
  int all
) {
  char line[lineWidth+1];
  unsigned int letterWidth = 0;
  unsigned int wordWidth = 0;
  unsigned int argumentWidth = 0;

  for (unsigned int optionIndex=0; optionIndex<info->optionCount; optionIndex+=1) {
    const OptionEntry *option = &info->optionTable[optionIndex];
    if (!all && (option->flags & OPT_Hidden)) continue;

    if (option->word) {
      unsigned int length = strlen(option->word);
      if (option->argument) length += 1;
      wordWidth = MAX(wordWidth, length);
    }

    if (option->letter) letterWidth = 3;
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

  for (unsigned int optionIndex=0; optionIndex<info->optionCount; optionIndex+=1) {
    const OptionEntry *option = &info->optionTable[optionIndex];
    if (!all && (option->flags & OPT_Hidden)) continue;

    unsigned int lineLength = 0;

    if (option->letter) {
      line[lineLength++] = '-';
      line[lineLength++] = option->letter;
    }

    while (lineLength < letterWidth) {
      line[lineLength++] = ' ';
    }

    {
      unsigned int end = lineLength + 2 + wordWidth;

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

    line[lineLength++] = ' ';
    {
      const unsigned int headerWidth = lineLength;
      const unsigned int descriptionWidth = lineWidth - headerWidth;
      const int formatStrings = !!(option->flags & OPT_Format);
      const char *description = option->description? gettext(option->description): "";

      char buffer[0X400];
      char *from = buffer;
      const char *const to = from + sizeof(buffer);

      if (formatStrings? !!option->strings.format: !!option->strings.array) {
        unsigned int index = 0;
        const unsigned int limit = 4;
        const char *strings[limit];

        while (index < limit) {
          const char *string;

          if (formatStrings) {
            size_t length = option->strings.format(from, (to - from), index);

            if (length) {
              string = from;
              from += length + 1;
            } else {
              string = NULL;
            }
          } else {
            string = option->strings.array[index];
          }

          if (!string) break;
          strings[index++] = string;
        }

        while (index < limit) strings[index++] = "";
        snprintf(from, (to - from),
                 description, strings[0], strings[1], strings[2], strings[3]);
        description = from;
      }

      {
        unsigned int charsLeft = strlen(description);

        while (1) {
          unsigned int charCount = charsLeft;

          if (charCount > descriptionWidth) {
            charCount = descriptionWidth;

            while (charCount > 0) {
              if (description[charCount] == ' ') break;
              charCount -= 1;
            }

            while (charCount > 0) {
              if (description[--charCount] != ' ') {
                charCount += 1;
                break;
              }
            }
          }

          if (charCount > 0) {
            memcpy(line+lineLength, description, charCount);
            lineLength += charCount;

            line[lineLength] = 0;
            fprintf(outputStream, "%s\n", line);
          }

          while (charCount < charsLeft) {
            if (description[charCount] != ' ') break;
            charCount += 1;
          }

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
  const char *reset = NULL;
  const char resetPrefix = '+';
  int resetLetter;

  const int firstNonLetter = 0X80;
  const OptionEntry *optionEntries[firstNonLetter + info->optionCount];

  for (unsigned int index=0; index<ARRAY_COUNT(optionEntries); index+=1) {
    optionEntries[index] = NULL;
  }

  int optionLetters[info->optionCount];
  char shortOptions[2 + (info->optionCount * 2) + 1];

  {
    int nextNonLetter = firstNonLetter;

    char *opt = shortOptions;
    *opt++ = '+';
    *opt++ = ':';

    for (unsigned int index=0; index<info->optionCount; index+=1) {
      const OptionEntry *entry = &info->optionTable[index];
      int letter = entry->letter;

      if (letter) {
        if (optionEntries[letter]) {
          logMessage(LOG_WARNING, "duplicate short option: -%c", letter);
          letter = 0;
        } else {
          *opt++ = letter;
          if (entry->argument) *opt++ = ':';
        }
      }

      if (!letter) letter = nextNonLetter++;
      optionLetters[index] = letter;
      optionEntries[letter] = entry;

      if (entry->argument) {
        if (entry->setting.string) *entry->setting.string = NULL;
      } else {
        if (entry->setting.flag) *entry->setting.flag = 0;
      }
    }

    *opt = 0;
  }

#ifdef HAVE_GETOPT_LONG
  struct option longOptions[(info->optionCount * 2) + 1];

  {
    struct option *opt = longOptions;

    for (unsigned int index=0; index<info->optionCount; index+=1) {
      const OptionEntry *entry = &info->optionTable[index];
      const char *word = entry->word;
      if (!word) continue;
      int letter = optionLetters[index];

      opt->name = word;
      opt->has_arg = entry->argument? required_argument: no_argument;
      opt->flag = NULL;
      opt->val = letter;
      opt += 1;

      if (!entry->argument && entry->setting.flag) {
        char *name;

        const char *noPrefix = "no-";
        size_t noLength = strlen(noPrefix);

        if (strncasecmp(noPrefix, word, noLength) == 0) {
          name = strdup(&word[noLength]);
        } else {
          size_t size = noLength + strlen(word) + 1;

          if ((name = malloc(size))) {
            snprintf(name, size, "%s%s", noPrefix, word);
          }
        }

        if (name) {
          opt->name = name;
          opt->has_arg = no_argument;
          opt->flag = &resetLetter;
          opt->val = letter;
          opt += 1;
        } else {
          logMallocError();
        }
      }
    }

    memset(opt, 0, sizeof(*opt));
  }
#endif /* HAVE_GETOPT_LONG */

#ifdef ALLOW_DOS_OPTION_SYNTAX
  const char dosPrefix = '/';
  int dosSyntax = 0;

  if (*argumentCount > 1) {
    if (*(*argumentVector)[1] == dosPrefix) {
      dosSyntax = 1;
    }
  }
#endif /* ALLOW_DOS_OPTION_SYNTAX */

  opterr = 0;
  optind = 1;
  int lastOptInd = -1;

  int optHelp = 0;
  int optHelpAll = 0;

  while (1) {
    int option;
    char prefix = '-';
    resetLetter = 0;

    if (optind == *argumentCount) {
      option = -1;
    } else {
      char *argument = (*argumentVector)[optind];

#ifdef ALLOW_DOS_OPTION_SYNTAX
      if (dosSyntax) {
        prefix = dosPrefix;
        optind += 1;

        if (*argument != dosPrefix) {
          option = -1;
        } else {
          char *name = argument + 1;
          size_t nameLength = strcspn(name, ":");
          char *value = name[nameLength]? (name + nameLength + 1): NULL;
          const OptionEntry *entry;

          if (nameLength == 1) {
            entry = optionEntries[option = *name];
          } else {
            option = -1;

            for (unsigned int index=0; index<info->optionCount; index+=1) {
              entry = &info->optionTable[index];
              const char *word = entry->word;

              if (word) {
                if ((nameLength == strlen(word)) &&
                    (strncasecmp(word, name, nameLength) == 0)) {
                  option = optionLetters[index];
                  break;
                }
              }
            }

            if (option < 0) {
              entry = NULL;
              option = 0;
            }
          }

          optopt = option;
          optarg = value;

          if (!entry) {
            option = '?';
          } else if (entry->argument) {
            if (!optarg) option = ':';
          } else if (value) {
            unsigned int on;

            if (!validateFlagKeyword(&on, value)) {
              option = '-';
            } else if (!on) {
              resetLetter = option;
              option = 0;
            }
          }
        }
      } else
#endif /* ALLOW_DOS_OPTION_SYNTAX */

      if (reset) {
        prefix = resetPrefix;

        if (!(option = *reset++)) {
          reset = NULL;
          optind += 1;
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
            setEnsuredSetting(info, entry, 0);
            break;
          }

          if (entry->setting.string) {
            if (entry->flags & OPT_Extend) {
              extendStringSetting(entry->setting.string, optarg, 0);
            } else {
              changeStringSetting(entry->setting.string, optarg);
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

        setEnsuredSetting(info, entry, 1);
        break;
      }

      case 0: { // reset a flag
        const OptionEntry *entry = optionEntries[resetLetter];
        *entry->setting.flag = 0;
        setEnsuredSetting(info, entry, 1);
        break;
      }

      case '?': { // an unknown option has been specified
        const char *message = gettext("unknown option");

        if (optopt) {
          logMessage(LOG_ERR, "%s: %c%c", message, prefix, optopt);
        } else {
          logMessage(LOG_ERR, "%s: %s", message, (*argumentVector)[optind-1]);
        }

        info->syntaxError = 1;
        break;
      }

      case ':': { // the operand for a string option hasn't been specified
        logMessage(LOG_ERR, "%s: %c%c", gettext("missing operand"), prefix, optopt);
        info->syntaxError = 1;
        break;
      }

      case '-': { // the operand for an option is invalid
        logMessage(LOG_ERR, "%s: %c%c: %s", gettext("invalid operand"), prefix, optopt, optarg);
        info->warning = 1;
        break;
      }

      case 'H': // full help
        optHelpAll = 1;
        /* fall through */
      case 'h': // help - show usage summary and then exit
        optHelp = 1;
        break;
    }
  }

  *argumentVector += optind;
  *argumentCount -= optind;

  if (optHelp) {
    showUsage(info, stdout, 79, argumentsSummary, optHelpAll);
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

  if (!(value = allocated = getBootParameters(parameter))) {
    if (!(value = getenv(parameter))) {
      return;
    }
  }

  {
    int parameterCount = 0;
    char **parameters = splitString(value, ',', &parameterCount);

    for (unsigned int optionIndex=0; optionIndex<info->optionCount; optionIndex+=1) {
      const OptionEntry *option = &info->optionTable[optionIndex];

      if ((option->bootParameter) && (option->bootParameter <= parameterCount)) {
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

static int
processEnvironmentVariable (
  OptionProcessingInformation *info,
  const OptionEntry *option,
  const char *prefix
) {
  size_t prefixLength = strlen(prefix);

  if ((option->flags & OPT_EnvVar) && option->word) {
    size_t nameSize = prefixLength + 1 + strlen(option->word) + 1;
    char name[nameSize];

    snprintf(name, nameSize, "%s_%s", prefix, option->word);

    {
      char *character = name;

      while (*character) {
        if (*character == '-') {
          *character = '_';
        } else if (islower((unsigned char)*character)) {
          *character = toupper((unsigned char)*character);
        }

        character += 1;
      }
    }

    {
      const char *setting = getenv(name);

      if (setting && *setting) {
        if (!ensureSetting(info, option, setting)) {
          return 0;
        }
      }
    }
  }

  return 1;
}

static int
processEnvironmentVariables (
  OptionProcessingInformation *info,
  const char *prefix
) {
  for (unsigned int optionIndex=0; optionIndex<info->optionCount; optionIndex+=1) {
    const OptionEntry *option = &info->optionTable[optionIndex];

    if (!processEnvironmentVariable(info, option, prefix)) return 0;
  }

  return 1;
}

static void
processInternalSettings (
  OptionProcessingInformation *info,
  int config
) {
  for (unsigned int optionIndex=0; optionIndex<info->optionCount; optionIndex+=1) {
    const OptionEntry *option = &info->optionTable[optionIndex];

    if (!(option->flags & OPT_Config) == !config) {
      const char *setting = option->internal.setting;
      char *newSetting = NULL;

      if (!setting) setting = option->argument? "": FLAG_FALSE_WORD;

      if (option->internal.adjust) {
        if (*setting) {
          if ((newSetting = strdup(setting))) {
            if (option->internal.adjust(&newSetting)) {
              setting = newSetting;
            }
          } else {
            logMallocError();
          }
        }
      }

      ensureSetting(info, option, setting);
      if (newSetting) free(newSetting);
    }
  }
}

typedef struct {
  unsigned int option;
  wchar_t keyword[0];
} ConfigurationDirective;

static int
sortConfigurationDirectives (const void *element1, const void *element2) {
  const ConfigurationDirective *const *directive1 = element1;
  const ConfigurationDirective *const *directive2 = element2;

  return compareKeywords((*directive1)->keyword, (*directive2)->keyword);
}

static int
searchConfigurationDirective (const void *target, const void *element) {
  const wchar_t *keyword = target;
  const ConfigurationDirective *const *directive = element;

  return compareKeywords(keyword, (*directive)->keyword);
}

typedef struct {
  OptionProcessingInformation *info;
  char **settings;

  struct {
    ConfigurationDirective **table;
    unsigned int count;
  } directive;
} ConfigurationFileProcessingData;

static const ConfigurationDirective *
findConfigurationDirective (const wchar_t *keyword, const ConfigurationFileProcessingData *conf) {
  const ConfigurationDirective *const *directive = bsearch(keyword, conf->directive.table, conf->directive.count, sizeof(*conf->directive.table), searchConfigurationDirective);

  if (directive) return *directive;
  return NULL;
}

static int
processConfigurationDirective (
  const wchar_t *keyword,
  const char *value,
  const ConfigurationFileProcessingData *conf
) {
  const ConfigurationDirective *directive = findConfigurationDirective(keyword, conf);

  if (directive) {
    const OptionEntry *option = &conf->info->optionTable[directive->option];
    char **setting = &conf->settings[directive->option];

    if (*setting && !hasExtendableArgument(option)) {
      logMessage(LOG_ERR, "%s: %" PRIws, gettext("configuration directive specified more than once"), keyword);
      conf->info->warning = 1;

      free(*setting);
      *setting = NULL;
    }

    if (*setting) {
      if (!extendStringSetting(setting, value, 0)) return 0;
    } else {
      if (!(*setting = strdup(value))) {
        logMallocError();
        return 0;
      }
    }
  } else {
    logMessage(LOG_ERR, "%s: %" PRIws, gettext("unknown configuration directive"), keyword);
    conf->info->warning = 1;
  }

  return 1;
}

static DATA_OPERANDS_PROCESSOR(processConfigurationOperands) {
  const ConfigurationFileProcessingData *conf = data;
  int ok = 1;
  DataString keyword;

  if (getDataString(file, &keyword, 0, "configuration directive")) {
    DataString value;

    if (getDataString(file, &value, 0, "configuration value")) {
      char *v = getUtf8FromWchars(value.characters, value.length, NULL);

      if (v) {
        if (!processConfigurationDirective(keyword.characters, v, conf)) ok = 0;

        free(v);
      } else {
        ok = 0;
      }
    } else {
      conf->info->warning = 1;
    }
  } else {
    conf->info->warning = 1;
  }

  return ok;
}

static DATA_CONDITION_TESTER(testConfigurationDirectiveSet) {
  const ConfigurationFileProcessingData *conf = data;
  wchar_t keyword[identifier->length + 1];

  wmemcpy(keyword, identifier->characters, identifier->length);
  keyword[identifier->length] = 0;

  {
    const ConfigurationDirective *directive = findConfigurationDirective(keyword, conf);

    if (directive) {
      if (conf->settings[directive->option]) {
        return 1;
      }
    }
  }

  return 0;
}

static int
processConfigurationDirectiveTestOperands (DataFile *file, int not, void *data) {
  return processConditionOperands(file, testConfigurationDirectiveSet, not, "configuration directive", data);
}

static DATA_OPERANDS_PROCESSOR(processIfSetOperands) {
  return processConfigurationDirectiveTestOperands(file,00, data);
}

static DATA_OPERANDS_PROCESSOR(processIfNotSetOperands) {
  return processConfigurationDirectiveTestOperands(file, 1, data);
}

static DATA_OPERANDS_PROCESSOR(processConfigurationLine) {
  BEGIN_DATA_DIRECTIVE_TABLE
    DATA_NESTING_DIRECTIVES,
    DATA_VARIABLE_DIRECTIVES,
    DATA_CONDITION_DIRECTIVES,
    {.name=WS_C("ifset"), .processor=processIfSetOperands, .unconditional=1},
    {.name=WS_C("ifnotset"), .processor=processIfNotSetOperands, .unconditional=1},
    {.name=NULL, .processor=processConfigurationOperands},
  END_DATA_DIRECTIVE_TABLE

  return processDirectiveOperand(file, &directives, "configuration file directive", data);
}

static void
freeConfigurationDirectives (ConfigurationFileProcessingData *conf) {
  while (conf->directive.count > 0) free(conf->directive.table[--conf->directive.count]);
}

static int
addConfigurationDirectives (ConfigurationFileProcessingData *conf) {
  for (unsigned int optionIndex=0; optionIndex<conf->info->optionCount; optionIndex+=1) {
    const OptionEntry *option = &conf->info->optionTable[optionIndex];

    if ((option->flags & OPT_Config) && option->word) {
      ConfigurationDirective *directive;
      const char *keyword = option->word;
      size_t length = countUtf8Characters(keyword);
      size_t size = sizeof(*directive) + ((length + 1) * sizeof(wchar_t));

      if (!(directive = malloc(size))) {
        logMallocError();
        freeConfigurationDirectives(conf);
        return 0;
      }

      directive->option = optionIndex;

      {
        const char *utf8 = keyword;
        wchar_t *wc = directive->keyword;
        convertUtf8ToWchars(&utf8, &wc, length+1);
      }

      conf->directive.table[conf->directive.count++] = directive;
    }
  }

  qsort(conf->directive.table, conf->directive.count,
        sizeof(*conf->directive.table), sortConfigurationDirectives);

  return 1;
}

static void
processConfigurationFile (
  OptionProcessingInformation *info,
  const char *path,
  int optional
) {
  if (setBaseDataVariables(NULL)) {
    FILE *file = openDataFile(path, "r", optional);

    if (file) {
      char *settings[info->optionCount];
      ConfigurationDirective *directives[info->optionCount];

      ConfigurationFileProcessingData conf = {
        .info = info,
        .settings = settings,

        .directive = {
          .table = directives,
          .count = 0
        }
      };

      if (addConfigurationDirectives(&conf)) {
        int processed;

        for (unsigned int index=0; index<info->optionCount; index+=1) {
          conf.settings[index] = NULL;
        }

        {
          const DataFileParameters dataFileParameters = {
            .processOperands = processConfigurationLine,
            .data = &conf
          };

          processed = processDataStream(NULL, file, path, &dataFileParameters);
        }

        for (unsigned int index=0; index<info->optionCount; index+=1) {
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

        freeConfigurationDirectives(&conf);
      }

      fclose(file);
    } else if (!optional || (errno != ENOENT)) {
      info->warning = 1;
    }
  }
}

void
resetOptions (const OptionsDescriptor *descriptor) {
  unsigned int optionIndex = 0;

  for (optionIndex=0; optionIndex<descriptor->optionCount; optionIndex+=1) {
    const OptionEntry *option = &descriptor->optionTable[optionIndex];

    if (option->argument) {
      char **string = option->setting.string;

      if (string) changeStringSetting(string, NULL);
    } else {
      int *flag = option->setting.flag;

      if (flag) *flag = 0;
    }
  }
}

static void
exitOptions (void *data) {
  const OptionsDescriptor *descriptor = data;

  resetOptions(descriptor);
}

ProgramExitStatus
processOptions (const OptionsDescriptor *descriptor, int *argumentCount, char ***argumentVector) {
  uint8_t ensuredSettings[descriptor->optionCount];
  memset(ensuredSettings, 0, sizeof(ensuredSettings));

  OptionProcessingInformation info = {
    .optionTable = descriptor->optionTable,
    .optionCount = descriptor->optionCount,
    .ensuredSettings = ensuredSettings,

    .exitImmediately = 0,
    .warning = 0,
    .syntaxError = 0
  };

  onProgramExit("options", exitOptions, (void *)descriptor);
  beginProgram(*argumentCount, *argumentVector);
  processCommandLine(&info, argumentCount, argumentVector, descriptor->argumentsSummary);

  if (descriptor->doBootParameters && *descriptor->doBootParameters) {
    processBootParameters(&info, descriptor->applicationName);
  }

  if (descriptor->doEnvironmentVariables && *descriptor->doEnvironmentVariables) {
    processEnvironmentVariables(&info, descriptor->applicationName);
  }

  processInternalSettings(&info, 0);
  {
    int configurationFileSpecified = descriptor->configurationFile && *descriptor->configurationFile;

    if (configurationFileSpecified) {
      processConfigurationFile(&info, *descriptor->configurationFile, !configurationFileSpecified);
    }
  }
  processInternalSettings(&info, 1);

  if (info.exitImmediately) return PROG_EXIT_FORCE;
  if (info.syntaxError) return PROG_EXIT_SYNTAX;
  return PROG_EXIT_SUCCESS;
}

static ProgramExitStatus
processInputStream (
  FILE *stream, const char *name,
  const InputFilesProcessingParameters *parameters
) {
  int ok = 0;

  if (parameters->beginStream) {
    parameters->beginStream(name, parameters->dataFileParameters.data);
  }

  if (setBaseDataVariables(NULL)) {
    if (processDataStream(NULL, stream, name, &parameters->dataFileParameters)) {
      ok = 1;
    }
  }

  if (parameters->endStream) {
    parameters->endStream(!ok, parameters->dataFileParameters.data);
  }

  return ok? PROG_EXIT_SUCCESS: PROG_EXIT_FATAL;
}

static ProgramExitStatus
processStandardInput (const InputFilesProcessingParameters *parameters) {
  return processInputStream(stdin, standardInputName, parameters);
}

static ProgramExitStatus
processInputFile (const char *path, const InputFilesProcessingParameters *parameters) {
  if (strcmp(path, standardStreamArgument) == 0) {
    return processStandardInput(parameters);
  }

  {
    FILE *stream = fopen(path, "r");

    if (!stream) {
      logMessage(LOG_ERR, "input file open error: %s: %s", path, strerror(errno));
      return PROG_EXIT_FATAL;
    }

    ProgramExitStatus status = processInputStream(stream, path, parameters);
    fclose(stream);
    return status;
  }
}

ProgramExitStatus
processInputFiles (
  char **paths, int count,
  const InputFilesProcessingParameters *parameters
) {
  if (!count) return processStandardInput(parameters);

  do {
    ProgramExitStatus status = processInputFile(*paths++, parameters);
    if (status != PROG_EXIT_SUCCESS) return status;
  } while (count -= 1);

  return PROG_EXIT_SUCCESS;
}
