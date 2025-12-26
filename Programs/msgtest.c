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

#include "log.h"
#include "cmdline.h"
#include "cmdput.h"
#include "messages.h"
#include "parse.h"
#include "file.h"

static char *opt_localeDirectory;
static char *opt_localeSpecifier;
static char *opt_domainName;

static int opt_utf8Output;

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
  { .word = "directory",
    .letter = 'd',
    .argument = strtext("path"),
    .setting.string = &opt_localeDirectory,
    .internal.adjust = toAbsoluteInstallPath,
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
END_COMMAND_LINE_OPTIONS(programOptions)

static const char *requestedAction;

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
  { .name = "action",
    .description = "the action to perform",
    .setting = &requestedAction,
  },
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
  "Action names aren't case-sensitive and may be abbreviated.",
  "The available actions are:",
  "  count",
  "  list`",
  "  metadata",
  "  property name [attribute]",
  "  translation message [plural quantity]",
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "msgtest",
  .purpose = strtext("Test message localization using the message catalog reader."),

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),

  .extraParameters = {
    .name = "arg",
    .description = "arguments for the requested action",
  },
END_COMMAND_LINE_DESCRIPTOR

static void
putCharacters (const char *characters, size_t count) {
  while (count) {
    size_t last = count - 1;
    if (characters[last] != '\n') break;
    count = last;
  }

  if (opt_utf8Output) {
    putBytes(characters, count);
  } else {
    putConsole(characters, count);
  }
}

static void
putText (const char *text) {
  putCharacters(text, strlen(text));
}

static void
putMessage (const Message *message) {
  putCharacters(getMessageText(message), getMessageLength(message));
}

static void
listTranslation (const Message *source, const Message *translation) {
  putMessage(source);
  putString(" -> ");
  putMessage(translation);
  putNewline();
}

static void
listAllTranslations (void) {
  uint32_t count = getMessageCount();

  for (unsigned int index=0; index<count; index+=1) {
    const Message *source = getSourceMessage(index);
    if (!getMessageLength(source)) continue;

    const Message *translation = getTranslatedMessage(index);
    listTranslation(source, translation);
  }
}

static int
showSimpleTranslation (const char *text) {
  {
    unsigned int index;

    if (findSourceMessage(text, strlen(text), &index)) {
      putMessage(getTranslatedMessage(index));
      putNewline();
      return 1;
    }
  }

  logMessage(LOG_WARNING, "translation not found: %s", text);
  return 0;
}

static void
showPluralTranslation (const char *singular, const char *plural, int count) {
  const char *translation = getPluralTranslation(singular, plural, count);
  putText(translation);
  putNewline();
}

static int
showProperty (const char *propertyName, const char *attributeName) {
  int ok = 0;
  char *propertyValue = getMessagesProperty(propertyName);

  if (propertyValue) {
    if (!attributeName) {
      putf("%s\n", propertyValue);
      ok = 1;
    } else {
      char *attributeValue = getMessagesAttribute(propertyValue, attributeName);

      if (attributeValue) {
        putf("%s\n", attributeValue);
        ok = 1;
        free(attributeValue);
      } else {
        logMessage(LOG_WARNING,
          "attribute not defined: %s: %s",
          propertyName, attributeName
        );
      }
    }

    free(propertyValue);
  } else {
    logMessage(LOG_WARNING, "property not defined: %s", propertyName);
  }

  return ok;
}

static int
parseQuantity (int *count, const char *quantity) {
  static const int minimum = 0;
  static const int maximum = 999999999;

  if (validateInteger(count, quantity, &minimum, &maximum)) return 1;
  logMessage(LOG_ERR, "invalid quantity: %s", quantity);
  return 0;
}

static void
beginAction (char ***argv, int *argc) {
  noMoreProgramArguments(argv, argc);

  {
    const char *directory = opt_localeDirectory;

    if (*directory) {
      if (!testDirectoryPath(directory)) {
        logMessage(LOG_WARNING, "not a directory: %s", directory);
        exit(PROG_EXIT_SEMANTIC);
      }

      setMessagesDirectory(directory);
    }
  }

  if (*opt_localeSpecifier) setMessagesLocale(opt_localeSpecifier);
  if (*opt_domainName) setMessagesDomain(opt_domainName);
  if (!loadMessageCatalog()) exit(PROG_EXIT_FATAL);
}

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  int ok = 1;

  if (isAbbreviation("translation", requestedAction)) {
    const char *message = nextProgramArgument(&argv, &argc, "message");
    const char *plural = nextProgramArgument(&argv, &argc, NULL);

    if (plural) {
      const char *quantity = nextProgramArgument(&argv, &argc, "quantity");

      int count;
      if (!parseQuantity(&count, quantity)) return PROG_EXIT_SYNTAX;

      beginAction(&argv, &argc);
      showPluralTranslation(message, plural, count);
    } else {
      beginAction(&argv, &argc);
      ok = showSimpleTranslation(message);
    }
  } else if (isAbbreviation("count", requestedAction)) {
    beginAction(&argv, &argc);
    putf("%u\n", getMessageCount());
  } else if (isAbbreviation("list`", requestedAction)) {
    beginAction(&argv, &argc);
    listAllTranslations();
  } else if (isAbbreviation("metadata", requestedAction)) {
    beginAction(&argv, &argc);
    putf("%s\n", getMessagesMetadata());
  } else if (isAbbreviation("property", requestedAction)) {
    const char *property = nextProgramArgument(&argv, &argc, "property name");
    const char *attribute = nextProgramArgument(&argv, &argc, NULL);

    beginAction(&argv, &argc);
    ok = showProperty(property, attribute);
  } else {
    logMessage(LOG_ERR, "unrecognized action: %s", requestedAction);
    return PROG_EXIT_SYNTAX;
  }

  return ok? PROG_EXIT_SUCCESS: PROG_EXIT_SEMANTIC;
}
