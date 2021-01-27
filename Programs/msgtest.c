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
#include <errno.h>

#include "log.h"
#include "program.h"
#include "options.h"
#include "messages.h"
#include "parse.h"
#include "file.h"

static char *opt_localeDirectory;
static char *opt_localeSpecifier;
static char *opt_domainName;

static int opt_utf8Output;
static int opt_showCount;
static int opt_showMetadata;
static int opt_listTranslations;

BEGIN_OPTION_TABLE(programOptions)
  { .word = "directory",
    .letter = 'd',
    .argument = strtext("path"),
    .setting.string = &opt_localeDirectory,
    .internal.adjust = fixInstallPath,
    .description = strtext("the locale directory containing the translations")
  },

  { .word = "locale",
    .letter = 'l',
    .argument = strtext("specifier"),
    .setting.string = &opt_localeSpecifier,
    .description = strtext("the locale in which to look up a translation")
  },

  { .word = "domain",
    .letter = 'n',
    .argument = strtext("name"),
    .setting.string = &opt_domainName,
    .description = strtext("the name of the domain containing the translations")
  },

  { .word = "utf8",
    .letter = 'u',
    .setting.flag = &opt_utf8Output,
    .description = strtext("write the translations using UTF-8")
  },

  { .word = "count",
    .letter = 'c',
    .setting.flag = &opt_showCount,
    .description = strtext("show the message count")
  },

  { .word = "metadata",
    .letter = 'm',
    .setting.flag = &opt_showMetadata,
    .description = strtext("show the translation metadata")
  },

  { .word = "translations",
    .letter = 't',
    .setting.flag = &opt_listTranslations,
    .description = strtext("list all of the translations (the default)")
  },
END_OPTION_TABLE

static int
noOutputErrorYet (void) {
  if (!ferror(stdout)) return 1;
  logMessage(LOG_ERR, "output error: %s", strerror(errno));
  return 0;
}

static int
putCharacter (char c) {
  fputc(c, stdout);
  return noOutputErrorYet();
}

static int
putNewline (void) {
  return putCharacter('\n');
}

static int
putBytes (const char *bytes, size_t count) {
  if (opt_utf8Output) {
    fwrite(bytes, 1, count, stdout);
  } else {
    writeWithConsoleEncoding(stdout, bytes, count);
  }

  return noOutputErrorYet();
}

static int
putString (const char *string) {
  return putBytes(string, strlen(string));
}

static int
putMessage (const Message *message) {
  const char *text = getMessageText(message);
  uint32_t length = getMessageLength(message);

  while (length) {
    uint32_t last = length - 1;
    if (text[last] != '\n') break;
    length = last;
  }

  return putBytes(text, length);
}

static int
listTranslation (const Message *original, const Message *translation) {
  return putMessage(original)
      && putString(" -> ")
      && putMessage(translation)
      && putNewline();
}

static int
listTranslations (void) {
  uint32_t count = getMessageCount();

  for (unsigned int index=0; index<count; index+=1) {
    const Message *original = getOriginalMessage(index);
    if (getMessageLength(original) == 0) continue;

    const Message *translation = getTranslatedMessage(index);
    if (!listTranslation(original, translation)) return 0;
  }

  return 1;
}

static int
showSimpleTranslation (const char *text) {
  {
    unsigned int index;

    if (findOriginalMessage(text, strlen(text), &index)) {
      return putMessage(getTranslatedMessage(index)) && putNewline();
    }
  }

  logMessage(LOG_WARNING, "translation not found: %s", text);
  return 0;
}

static int
showPluralTranslation (const char *singular, const char *plural, int count) {
  const char *translation = getPluralTranslation(singular, plural, count);
  char text[0X10000];
  snprintf(text, sizeof(text), translation, count);

  return putString(text) && putNewline();
}

static int
parseQuantity (int *count, const char *quantity) {
  static const int minimum = 0;
  static const int maximum = 999999999;

  if (validateInteger(count, quantity, &minimum, &maximum)) return 1;
  logMessage(LOG_ERR, "invalid quantity: %s", quantity);
  return 0;
}

int
main (int argc, char *argv[]) {
  int problemEncountered = 0;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "msgtest",
      .argumentsSummary = "[message [plural quantity]]"
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  {
    const char *directory = opt_localeDirectory;

    if (*directory) {
      if (testDirectoryPath(directory)) {
        setMessagesDirectory(directory);
      } else {
        logMessage(LOG_WARNING, "not a directory: %s", directory);
        problemEncountered = 1;
      }
    }
  }

  if (*opt_localeSpecifier) setMessagesLocale(opt_localeSpecifier);
  if (*opt_domainName) setMessagesDomain(opt_domainName);

  if (problemEncountered) return PROG_EXIT_SEMANTIC;
  if (!loadMessagesData()) return PROG_EXIT_FATAL;

  if (!(opt_showCount || opt_showMetadata || opt_listTranslations)) {
    int ok;

    switch (argc) {
      case 0:
        ok = listTranslations();
        break;

      case 1:
        ok = showSimpleTranslation(argv[0]);
        break;

      case 2:
        logMessage(LOG_ERR, "missing quantity");
        return PROG_EXIT_SYNTAX;

      case 3: {
        int count;
        if (!parseQuantity(&count, argv[2])) return PROG_EXIT_SYNTAX;

         ok = showPluralTranslation(argv[0], argv[1], count);
         break;
      }

      default:
        goto TOO_MANY_PARAMETERS;
    }

    if (ok) return PROG_EXIT_SUCCESS;
    if (ferror(stdout)) return PROG_EXIT_FATAL;
    return PROG_EXIT_SEMANTIC;
  } else if (!argc) {
    int ok = 1;

    if (ok) {
      if (opt_showCount) {
        printf("%u\n", getMessageCount());
        ok = noOutputErrorYet();
      }
    }

    if (ok) {
      if (opt_showMetadata) {
        printf("%s\n", getMessagesMetadata());
        ok = noOutputErrorYet();
      }
    }

    if (ok) {
      if (opt_listTranslations) {
        ok = listTranslations();
      }
    }

    return ok? PROG_EXIT_SUCCESS: PROG_EXIT_FATAL;
  }

TOO_MANY_PARAMETERS:
  logMessage(LOG_ERR, "too many parameters");
  return PROG_EXIT_SYNTAX;
}
