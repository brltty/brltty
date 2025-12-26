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
#include <ctype.h>
#include <errno.h>

#include "program.h"
#include "cmdline.h"
#include "cmdput.h"
#include "params.h"
#include "log.h"
#include "strfmt.h"
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
  const CommandLineOptions *const options;
  uint8_t *const ensuredSettings;

  uint8_t showHelp:1;
  uint8_t warning:1;
  uint8_t syntaxError:1;
} OptionProcessingData;

static void
putHeader (const char *header) {
  putString(header);
  putByte(':');
}

static void
putTitle (const char *title) {
  putNewline();
  putHeader(title);
  putNewline();
}

typedef struct {
  const char *text;
  size_t length;
} HelpTextDescriptor;

static const char *defaultParameterName = "?";
static const char *repeatableArgumentIndicator = " ...";
static const char optionalArgumentPrefix = '[';
static const char optionalArgumentSuffix = ']';

static void
showParameterSyntax (const CommandLineDescriptor *descriptor) {
  const CommandLineParameters *parameters = descriptor->parameters;

  const char *extra = descriptor->extraParameters.name;
  unsigned int depth = 0;

  const CommandLineParameter *parameter = parameters->table;
  const CommandLineParameter *end = parameter + parameters->count;

  while (parameter < end) {
    putByte(' ');

    if (parameter->optional) {
      putByte(optionalArgumentPrefix);
      depth += 1;
    }

    {
      const char *name = parameter->name;
      if (!name) name = defaultParameterName;
      putString(getTranslatedText(name));
    }

    parameter += 1;
  }

  if (extra) {
    putByte(' ');
    putByte(optionalArgumentPrefix);
    putString(getTranslatedText(extra));
    putString(repeatableArgumentIndicator);
    depth += 1;
  }

  while (depth > 0) {
    putByte(optionalArgumentSuffix);
    depth -= 1;
  }
}

static void
showSyntax (const CommandLineDescriptor *descriptor) {
  putHeader(gettext("Syntax"));
  putByte(' ');
  putString(programName);

  if (descriptor->options) {
    if (descriptor->options->count > 0) {
      putByte(' ');
      putByte(optionalArgumentPrefix);
      putByte('-');
      putString(gettext("option"));
      putString(repeatableArgumentIndicator);
      putByte(optionalArgumentSuffix);
    }
  }

  showParameterSyntax(descriptor);
  putNewline();
}

static void
showParameter (
  char *line, unsigned int lineWidth,
  const char *name, int isRepeatable, unsigned int nameWidth,
  const char *description
) {
  unsigned int lineLength = 0;
  while (lineLength < 2) line[lineLength++] = ' ';

  {
    unsigned int end = lineLength + nameWidth;

    if (name) {
      size_t nameLength = strlen(name);
      memcpy(line+lineLength, name, nameLength);
      lineLength += nameLength;

      if (isRepeatable) {
        const char *indicator = repeatableArgumentIndicator;
        size_t indicatorLength = strlen(indicator);
        memcpy(line+lineLength, indicator, indicatorLength);
        lineLength += indicatorLength;
      }
    }

    while (lineLength < end) line[lineLength++] = ' ';
  }
  line[lineLength++] = ' ';

  line[lineLength++] = ' ';
  {
    if (!description) description = "";
    putWrappedText(description, line, lineLength, lineWidth);
  }
}

static void
showParameters (
  char *line, unsigned int lineWidth,
  const CommandLineDescriptor *descriptor
) {
  const CommandLineParameters *parameters = descriptor->parameters;

  if (parameters) {
    size_t parameterCount = parameters->count;
    HelpTextDescriptor names[parameterCount + 1];
    unsigned int nameWidth = 0;

    for (unsigned int parameterIndex=0; parameterIndex<parameters->count; parameterIndex+=1) {
      const CommandLineParameter *parameter = &parameters->table[parameterIndex];

      {
        HelpTextDescriptor *name = &names[parameterIndex];
        if (!(name->text = parameter->name)) name->text = defaultParameterName;

        name->text = getTranslatedText(name->text);
        name->length = strlen(name->text);
        nameWidth = MAX(nameWidth, name->length);
      }
    }

    {
      HelpTextDescriptor *name = &names[parameterCount];
      const char *extra = descriptor->extraParameters.name;

      if (extra) {
        name->text = getTranslatedText(extra);
        name->length = strlen(name->text);
        size_t width = name->length + strlen(repeatableArgumentIndicator);
        nameWidth = MAX(nameWidth, width);
      } else {
        name->text = NULL;
        name->length = 0;
      }
    }

    if (nameWidth > 0) {
      putTitle(gettext("Parameters"));

      for (unsigned int parameterIndex=0; parameterIndex<parameters->count; parameterIndex+=1) {
        const CommandLineParameter *parameter = &parameters->table[parameterIndex];

        showParameter(
          line, lineWidth,
          names[parameterIndex].text, 0, nameWidth,
          getTranslatedText(parameter->description)
        );
      }

      {
        const HelpTextDescriptor *name = &names[parameterCount];
        const char *extra = name->text;

        if (extra) {
          showParameter(
            line, lineWidth,
            extra, 1, nameWidth,
            getTranslatedText(descriptor->extraParameters.description)
          );
        }
      }
    }
  }
}

static void
showOptions (
  char *line, unsigned int lineWidth,
  const OptionProcessingData *opd
) {
  size_t optionCount = opd->options->count;

  if (optionCount > 0) {
    HelpTextDescriptor arguments[optionCount];

    unsigned int letterWidth = 0;
    unsigned int wordWidth = 0;
    unsigned int argumentWidth = 0;

    for (unsigned int optionIndex=0; optionIndex<optionCount; optionIndex+=1) {
      const CommandLineOption *option = &opd->options->table[optionIndex];

      if (option->word) {
        unsigned int length = strlen(option->word);
        if (option->argument) length += 1;
        wordWidth = MAX(wordWidth, length);
      }

      {
        HelpTextDescriptor *argument = &arguments[optionIndex];

        if (option->argument) {
          argument->text = getTranslatedText(option->argument);
          argument->length = strlen(argument->text);
          argumentWidth = MAX(argumentWidth, argument->length);
        } else {
          argument->text = NULL;
          argument->length = 0;
        }
      }

      if (option->letter) letterWidth = 2;
    }

    putTitle(gettext("Options"));

    for (unsigned int optionIndex=0; optionIndex<optionCount; optionIndex+=1) {
      const CommandLineOption *option = &opd->options->table[optionIndex];

      unsigned int lineLength = 0;
      while (lineLength < 2) line[lineLength++] = ' ';

      {
        unsigned int end = lineLength + letterWidth;

        if (option->letter) {
          line[lineLength++] = '-';
          line[lineLength++] = option->letter;
        }

        while (lineLength < end) line[lineLength++] = ' ';
      }
      line[lineLength++] = ' ';

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
          const HelpTextDescriptor *argument = &arguments[optionIndex];

          memcpy(line+lineLength, argument->text, argument->length);
          lineLength += argument->length;
        }

        while (lineLength < end) line[lineLength++] = ' ';
      }
      line[lineLength++] = ' ';

      line[lineLength++] = ' ';
      {
        const char *description = getTranslatedText(option->description);

        const int formatStrings = !!(option->flags & OPT_Format);
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
            description, strings[0], strings[1], strings[2], strings[3]
          );

          description = from;
        }

        putWrappedText(description, line, lineLength, lineWidth);
      }
    }
  }
}

static void
showHelp (
  const CommandLineDescriptor *descriptor,
  const OptionProcessingData *opd
) {
  size_t width = getConsoleWidth();
  char line[width+1];

  {
    const char *purpose = getTranslatedText(descriptor->purpose);

    if (purpose && *purpose) {
      putWrappedText(purpose, line, 0, width);
      putNewline();
    }
  }

  showSyntax(descriptor);
  showParameters(line, width, descriptor);
  showOptions(line, width, opd);

  {
    const char *const *const *notes = descriptor->notes;

    if (notes && *notes && **notes) {
      putNewline();
      putFormattedLines(notes, line, width);
    }
  }
}

static int
hasExtendableArgument (const CommandLineOption *option) {
  return option->argument && (option->flags & OPT_Extend);
}

static uint8_t *
getEnsuredSetting (
  const OptionProcessingData *opd,
  const CommandLineOption *option
) {
  return &opd->ensuredSettings[option - opd->options->table];
}

static void
setEnsuredSetting (
  const OptionProcessingData *opd,
  const CommandLineOption *option,
  uint8_t yes
) {
  *getEnsuredSetting(opd, option) = yes;
}

static int
ensureSetting (
  OptionProcessingData *opd,
  const CommandLineOption *option,
  const char *value
) {
  uint8_t *ensured = getEnsuredSetting(opd, option);

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
            opd->warning = 1;
          }
        } else {
          unsigned int on;

          if (validateFlagKeyword(&on, value)) {
            *option->setting.flag = on;
          } else {
            logMessage(LOG_ERR, "%s: %s", gettext("invalid flag setting"), value);
            opd->warning = 1;
          }
        }
      }
    }
  }

  return 1;
}

static void
processOptions (
  OptionProcessingData *opd,
  int *argumentCount,
  char ***argumentVector
) {
  const char *reset = NULL;
  const char resetPrefix = '+';
  int resetLetter;

  int dosSyntax = 0;
  const int firstNonLetter = 0X80;
  const CommandLineOption *letterToOption[firstNonLetter + opd->options->count];

  for (unsigned int index=0; index<ARRAY_COUNT(letterToOption); index+=1) {
    letterToOption[index] = NULL;
  }

  int indexToLetter[opd->options->count];
  char shortOptions[2 + (opd->options->count * 2) + 1];

  {
    int nextNonLetter = firstNonLetter;

    char *opt = shortOptions;
    *opt++ = '+'; // stop parsing options as soon as a non-option argument is encountered
    *opt++ = ':'; // Don't write any error messages

    for (unsigned int index=0; index<opd->options->count; index+=1) {
      const CommandLineOption *option = &opd->options->table[index];
      int letter = option->letter;

      if (letter) {
        if (letterToOption[letter]) {
          logMessage(LOG_WARNING, "duplicate short option: -%c", letter);
          letter = 0;
        } else {
          *opt++ = letter;
          if (option->argument) *opt++ = ':';
        }
      }

      if (!letter) letter = nextNonLetter++;
      indexToLetter[index] = letter;
      letterToOption[letter] = option;

      if (option->argument) {
        if (option->setting.string) *option->setting.string = NULL;
      } else {
        if (option->setting.flag) *option->setting.flag = 0;
      }
    }

    *opt = 0;
  }

#ifdef HAVE_GETOPT_LONG
  struct option longOptions[(opd->options->count * 2) + 1];

  {
    struct option *opt = longOptions;

    for (unsigned int index=0; index<opd->options->count; index+=1) {
      const CommandLineOption *option = &opd->options->table[index];
      const char *word = option->word;
      if (!word) continue;
      int letter = indexToLetter[index];

      opt->name = word;
      opt->has_arg = option->argument? required_argument: no_argument;
      opt->flag = NULL;
      opt->val = letter;
      opt += 1;

      if (!option->argument && option->setting.flag) {
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

  if (*argumentCount > 1) {
    if (*(*argumentVector)[1] == dosPrefix) {
      dosSyntax = 1;
    }
  }
#endif /* ALLOW_DOS_OPTION_SYNTAX */

  opterr = 0;
  optind = 1;
  int lastOptInd = -1;

  while (1) {
    int letter;
    char prefix = '-';
    resetLetter = 0;

    if (optind == *argumentCount) {
      letter = -1;
    } else {
      char *argument = (*argumentVector)[optind];

#ifdef ALLOW_DOS_OPTION_SYNTAX
      if (dosSyntax) {
        prefix = dosPrefix;
        optind += 1;

        if (*argument != dosPrefix) {
          letter = -1;
        } else {
          char *name = argument + 1;
          size_t nameLength = strcspn(name, ":");
          char *value = name[nameLength]? (name + nameLength + 1): NULL;
          const CommandLineOption *option;

          if (nameLength == 1) {
            option = letterToOption[letter = *name];
          } else {
            letter = -1;

            for (unsigned int index=0; index<opd->options->count; index+=1) {
              option = &opd->options->table[index];
              const char *word = option->word;

              if (word) {
                if ((nameLength == strlen(word)) &&
                    (strncasecmp(word, name, nameLength) == 0)) {
                  letter = indexToLetter[index];
                  break;
                }
              }
            }

            if (letter < 0) {
              option = NULL;
              letter = 0;
            }
          }

          optopt = letter;
          optarg = value;

          if (!option) {
            letter = '?';
          } else if (option->argument) {
            if (!optarg) letter = ':';
          } else if (value) {
            unsigned int on;

            if (!validateFlagKeyword(&on, value)) {
              letter = '-';
            } else if (!on) {
              resetLetter = letter;
              letter = 0;
            }
          }
        }
      } else
#endif /* ALLOW_DOS_OPTION_SYNTAX */

      if (reset) {
        prefix = resetPrefix;

        if (!(letter = *reset++)) {
          reset = NULL;
          optind += 1;
          continue;
        }

        {
          const CommandLineOption *option = letterToOption[letter];

          if (option && !option->argument && option->setting.flag) {
            resetLetter = letter;
            letter = 0;
          } else {
            optopt = letter;
            letter = '?';
          }
        }
      } else {
        if (optind != lastOptInd) {
          lastOptInd = optind;
          if ((reset = (*argument == resetPrefix)? argument+1: NULL)) continue;
        }

#ifdef HAVE_GETOPT_LONG
        letter = getopt_long(*argumentCount, *argumentVector, shortOptions, longOptions, NULL);
#else /* HAVE_GETOPT_LONG */
        letter = getopt(*argumentCount, *argumentVector, shortOptions);
#endif /* HAVE_GETOPT_LONG */
      }
    }

    if (letter == -1) break;
    /* continue on error as much as possible, as often we are typing blind
     * and won't even see the error message unless the display comes up.
     */

    switch (letter) {
      default: {
        const CommandLineOption *option = letterToOption[letter];

        if (option->argument) {
          if (!*optarg) {
            setEnsuredSetting(opd, option, 0);
            break;
          }

          if (option->setting.string) {
            if (option->flags & OPT_Extend) {
              extendStringSetting(option->setting.string, optarg, 0);
            } else {
              changeStringSetting(option->setting.string, optarg);
            }
          }
        } else {
          if (option->setting.flag) {
            if (option->flags & OPT_Extend) {
              *option->setting.flag += 1;
            } else {
              *option->setting.flag = 1;
            }
          }
        }

        setEnsuredSetting(opd, option, 1);
        break;
      }

      case 0: { // reset a flag
        const CommandLineOption *option = letterToOption[resetLetter];
        *option->setting.flag = 0;
        setEnsuredSetting(opd, option, 1);
        break;
      }

      {
        const char *problem;
        char message[0X100];

      case '?': // an unknown option has been specified
        opd->syntaxError = 1;
        problem = gettext("unknown option");
        goto logOptionProblem;

      case ':': // the operand for a string option hasn't been specified
        opd->syntaxError = 1;
        problem = gettext("missing operand");
        goto logOptionProblem;

      case '-': // the operand for an option is invalid
        opd->warning = 1;
        problem = gettext("invalid operand");
        goto logOptionProblem;

      logOptionProblem:
        STR_BEGIN(message, sizeof(message));

        STR_PRINTF("%s: ", problem);
        size_t optionStart = STR_LENGTH;

        if (optopt) {
          const CommandLineOption *option = letterToOption[optopt];

          if (option) {
            const char *beforeLetter = "";
            const char *afterLetter = "";

            if (option->word) {
              if (!dosSyntax) STR_PRINTF("%c", prefix);
              STR_PRINTF("%c%s", prefix, option->word);

              beforeLetter = " (";
              afterLetter = ")";
            }

            if (option->letter) {
              STR_PRINTF("%s%c%c%s", beforeLetter, prefix, option->letter, afterLetter);
            }
          } else if (optopt < firstNonLetter) {
            STR_PRINTF("%c%c", prefix, optopt);
          }
        }

        if (STR_LENGTH == optionStart) {
          STR_PRINTF("%s", (*argumentVector)[optind-1]);
        }

        STR_END;
        logMessage(LOG_WARNING, "%s", message);
        break;
      }

      case 'h': // help - show usage summary and then exit
        opd->showHelp = 1;
        break;
    }
  }

  *argumentVector += optind;
  *argumentCount -= optind;

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

static int
processParameters (
  const CommandLineDescriptor *descriptor,
  int *argumentCount,
  char ***argumentVector
) {
  const CommandLineParameters *parameters = descriptor->parameters;

  if (parameters) {
    const CommandLineParameter *parameter = parameters->table;
    const CommandLineParameter *end = parameter + parameters->count;

    while (parameter < end) {
      if (!*argumentCount) {
        if (parameter->optional) break;

        logMessage(LOG_ERR, 
          "%s: %s",
          gettext("missing parameter"),
          parameter->name
        );

        return 0;
      }

      if (parameter->setting) {
        *parameter->setting = **argumentVector;
      }

      *argumentVector += 1;
      *argumentCount -= 1;

      parameter += 1;
    }

    while (parameter < end) {
      if (parameter->setting) *parameter->setting = NULL;
      parameter += 1;
    }

    if (*argumentCount > 0) {
      if (!descriptor->extraParameters.name) {
        logMessage(LOG_ERR, "%s", gettext("too many parameters"));
        return 0;
      }
    }
  }

  return 1;
}

static void
processBootParameters (
  OptionProcessingData *opd,
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

    for (unsigned int optionIndex=0; optionIndex<opd->options->count; optionIndex+=1) {
      const CommandLineOption *option = &opd->options->table[optionIndex];

      if ((option->bootParameter) && (option->bootParameter <= parameterCount)) {
        char *parameter = parameters[option->bootParameter-1];

        if (*parameter) {
          {
            char *byte = parameter;

            do {
              if (*byte == '+') *byte = ',';
            } while (*++byte);
          }

          ensureSetting(opd, option, parameter);
        }
      }
    }

    deallocateStrings(parameters);
  }

  if (allocated) free(allocated);
}

static int
processEnvironmentVariable (
  OptionProcessingData *opd,
  const CommandLineOption *option,
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
        if (!ensureSetting(opd, option, setting)) {
          return 0;
        }
      }
    }
  }

  return 1;
}

static int
processEnvironmentVariables (
  OptionProcessingData *opd,
  const char *prefix
) {
  for (unsigned int optionIndex=0; optionIndex<opd->options->count; optionIndex+=1) {
    const CommandLineOption *option = &opd->options->table[optionIndex];

    if (!processEnvironmentVariable(opd, option, prefix)) return 0;
  }

  return 1;
}

static void
processInternalSettings (
  OptionProcessingData *opd,
  int config
) {
  for (unsigned int optionIndex=0; optionIndex<opd->options->count; optionIndex+=1) {
    const CommandLineOption *option = &opd->options->table[optionIndex];

    if (!(option->flags & OPT_Config) == !config) {
      const char *setting = option->internal.setting;
      char *newSetting = NULL;

      if (!setting) {
        setting = option->argument? "":
                  (option->flags & OPT_Extend)? "0":
                  OPT_WORD_FALSE;
      }

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

      ensureSetting(opd, option, setting);
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
  OptionProcessingData *opd;
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
    const CommandLineOption *option = &conf->opd->options->table[directive->option];
    char **setting = &conf->settings[directive->option];

    if (*setting && !hasExtendableArgument(option)) {
      logMessage(LOG_ERR, "%s: %" PRIws, gettext("configuration directive specified more than once"), keyword);
      conf->opd->warning = 1;

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
    conf->opd->warning = 1;
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
      conf->opd->warning = 1;
    }
  } else {
    conf->opd->warning = 1;
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
  for (unsigned int optionIndex=0; optionIndex<conf->opd->options->count; optionIndex+=1) {
    const CommandLineOption *option = &conf->opd->options->table[optionIndex];

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
  OptionProcessingData *opd,
  const char *path,
  int optional
) {
  if (setBaseDataVariables(NULL)) {
    FILE *file = openDataFile(path, "r", optional);

    if (file) {
      char *settings[opd->options->count];
      ConfigurationDirective *directives[opd->options->count];

      ConfigurationFileProcessingData conf = {
        .opd = opd,
        .settings = settings,

        .directive = {
          .table = directives,
          .count = 0
        }
      };

      if (addConfigurationDirectives(&conf)) {
        int processed;

        for (unsigned int index=0; index<opd->options->count; index+=1) {
          conf.settings[index] = NULL;
        }

        {
          const DataFileParameters dataFileParameters = {
            .processOperands = processConfigurationLine,
            .data = &conf
          };

          processed = processDataStream(NULL, file, path, &dataFileParameters);
        }

        for (unsigned int index=0; index<opd->options->count; index+=1) {
          char *setting = conf.settings[index];

          if (setting) {
            ensureSetting(opd, &opd->options->table[index], setting);
            free(setting);
          }
        }

        if (!processed) {
          logMessage(LOG_ERR, gettext("file '%s' processing error."), path);
          opd->warning = 1;
        }

        freeConfigurationDirectives(&conf);
      }

      fclose(file);
    } else if (!optional || (errno != ENOENT)) {
      opd->warning = 1;
    }
  }
}

static void
toAbsolutePaths (OptionProcessingData *opd) {
  char *parent = getWorkingDirectory();

  if (parent) {
    for (unsigned int optionIndex=0; optionIndex<opd->options->count; optionIndex+=1) {
      const CommandLineOption *option = &opd->options->table[optionIndex];

      if (option->internal.adjust == toAbsoluteInstallPath) {
        if (**option->setting.string) {
          anchorRelativePath(option->setting.string, parent);
        }
      }
    }

    free(parent);
  }
}

void
resetOptions (const CommandLineOptions *options) {
  for (unsigned int index=0; index<options->count; index+=1) {
    const CommandLineOption *option = &options->table[index];

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
  const CommandLineOptions *options = data;
  resetOptions(options);
}

BEGIN_COMMAND_LINE_OPTIONS(defaultOptions)
END_COMMAND_LINE_OPTIONS(defaultOptions)

BEGIN_COMMAND_LINE_PARAMETERS(defaultParameters)
END_COMMAND_LINE_PARAMETERS(defaultParameters)

ProgramExitStatus
processCommandLine (const CommandLineDescriptor *descriptor, int *argumentCount, char ***argumentVector) {
  CommandLineDescriptor cld = *descriptor;
  if (!cld.options) cld.options = &defaultOptions;
  if (!cld.parameters) cld.parameters = &defaultParameters;

  uint8_t ensuredSettings[cld.options->count];
  memset(ensuredSettings, 0, sizeof(ensuredSettings));

  OptionProcessingData opd = {
    .options = cld.options,
    .ensuredSettings = ensuredSettings,

    .showHelp = 0,
    .warning = 0,
    .syntaxError = 0
  };

  onProgramExit("options", exitOptions, (void *)cld.options);
  beginProgram(*argumentCount, *argumentVector);
  processOptions(&opd, argumentCount, argumentVector);

  if (opd.showHelp) {
    showHelp(&cld, &opd);
    return PROG_EXIT_FORCE;
  }

  if (cld.doBootParameters && *cld.doBootParameters) {
    processBootParameters(&opd, cld.name);
  }

  if (cld.doEnvironmentVariables && *cld.doEnvironmentVariables) {
    processEnvironmentVariables(&opd, cld.name);
  }

  processInternalSettings(&opd, 0);
  {
    int configurationFileSpecified = cld.configurationFile && *cld.configurationFile;

    if (configurationFileSpecified) {
      processConfigurationFile(&opd, *cld.configurationFile, !configurationFileSpecified);
    }
  }
  processInternalSettings(&opd, 1);

  if (opd.syntaxError) {
    return PROG_EXIT_SYNTAX;
  }

  if (!processParameters(&cld, argumentCount, argumentVector)) {
    return PROG_EXIT_SYNTAX;
  }

  toAbsolutePaths(&opd);
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
