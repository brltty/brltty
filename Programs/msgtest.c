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

static char *opt_locale;
static char *opt_domain;
static char *opt_directory;
static int opt_utf8;

BEGIN_OPTION_TABLE(programOptions)
  { .word = "locale",
    .letter = 'l',
    .argument = strtext("locale"),
    .setting.string = &opt_locale,
    .description = strtext("locale specifier for message translations")
  },

  { .word = "domain",
    .letter = 'n',
    .argument = strtext("name"),
    .setting.string = &opt_domain,
    .description = strtext("domain name for message translations")
  },

  { .word = "directory",
    .letter = 'd',
    .argument = strtext("directory"),
    .setting.string = &opt_directory,
    .internal.adjust = fixInstallPath,
    .description = strtext("locales directory containing message translations")
  },

  { .word = "utf8",
    .letter = 'u',
    .setting.flag = &opt_utf8,
    .description = strtext("encode output with UTF-8 (not console encoding)")
  },
END_OPTION_TABLE

static int
checkForOutputError (void) {
  if (!ferror(stdout)) return 1;
  logMessage(LOG_ERR, "output error: %s", strerror(errno));
  return 0;
}

static int
putCharacter (char c) {
  fputc(c, stdout);
  return checkForOutputError();
}

static int
putNewline (void) {
  return putCharacter('\n');
}

static int
putBytes (const char *bytes, size_t count) {
  if (opt_utf8) {
    fwrite(bytes, 1, count, stdout);
  } else {
    writeWithConsoleEncoding(stdout, bytes, count);
  }

  return checkForOutputError();
}

static int
putText (const char *text) {
  return putBytes(text, strlen(text));
}

static int
putString (const MessagesString *string) {
  return putBytes(getStringText(string), getStringLength(string));
}

static ProgramExitStatus
showBasicTranslation (const char *text) {
  {
    unsigned int index;

    if (findOriginalString(text, strlen(text), &index)) {
      int ok = putString(getTranslatedString(index))
            && putNewline();

      return ok? PROG_EXIT_SUCCESS: PROG_EXIT_FATAL;
    }
  }

  logMessage(LOG_WARNING, "translation not found: %s", text);
  return PROG_EXIT_SEMANTIC;
}

static ProgramExitStatus
showPluralTranslation (const char *singular, const char *plural, const char *quantity) {
  int count;

  {
    const static int minimum = 0;
    const static int maximum = 999999999;

    if (!validateInteger(&count, quantity, &minimum, &maximum)) {
      logMessage(LOG_ERR, "invalid quantity: %s", quantity);
      return PROG_EXIT_SYNTAX;
    }
  }

  const char *translation = getPluralTranslation(singular, plural, count);
  char text[0X10000];
  snprintf(text, sizeof(text), translation, count);

  int ok = putText(text)
        && putNewline();

  return ok? PROG_EXIT_SUCCESS: PROG_EXIT_FATAL;
}

static int
listTranslation (const MessagesString *original, const MessagesString *translation) {
  return putString(original)
      && putText(" -> ")
      && putString(translation)
      && putNewline();
}

static ProgramExitStatus
listTranslations (void) {
  uint32_t count = getStringCount();

  for (unsigned int index=0; index<count; index+=1) {
    const MessagesString *original = getOriginalString(index);
    if (getStringLength(original) == 0) continue;

    const MessagesString *translation = getTranslatedString(index);
    if (!listTranslation(original, translation)) return PROG_EXIT_FATAL;
  }

  return PROG_EXIT_SUCCESS;
}

int
main (int argc, char *argv[]) {
  int problemEncountered = 0;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "msgtest",
      .argumentsSummary = "phrase [plural quantity]"
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  {
    const char *directory = opt_directory;

    if (*directory) {
      if (testDirectoryPath(directory)) {
        setMessagesDirectory(directory);
      } else {
        logMessage(LOG_WARNING, "not a directory: %s", directory);
        problemEncountered = 1;
      }
    }
  }

  if (*opt_locale) setMessagesLocale(opt_locale);
  if (*opt_domain) setMessagesDomain(opt_domain);
  if (problemEncountered) return PROG_EXIT_SEMANTIC;

  ensureAllMessagesProperties();
  if (!loadMessagesData()) return PROG_EXIT_FATAL;

  if (argc == 0) return listTranslations();
  if (argc == 1) return showBasicTranslation(argv[0]);
  if (argc == 3) return showPluralTranslation(argv[0], argv[1], argv[2]);

  if (argc == 2) {
    logMessage(LOG_ERR, "missing quantity");
  } else {
    logMessage(LOG_ERR, "too many parameters");
  }

  return PROG_EXIT_SYNTAX;
}
