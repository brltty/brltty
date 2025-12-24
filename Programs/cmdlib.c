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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "log.h"
#include "cmdlib.h"
#include "utf8.h"
#include "file.h"
#include "datafile.h"
#include "parse.h"
#include "queue.h"

struct CommandArgumentsStruct {
  Queue *queue;
};

CommandArguments *
newCommandArguments (void) {
  CommandArguments *arguments;
  arguments = malloc(sizeof(*arguments));
  arguments->queue = newQueue(NULL, NULL);
  return arguments;
}

void
destroyCommandArguments (CommandArguments *arguments) {
  destroyQueue(arguments->queue);
  free(arguments);
}

void
removeArguments (CommandArguments *arguments) {
  deleteElements(arguments->queue);
}

void
addArgument (CommandArguments *arguments, char *argument) {
  enqueueItem(arguments->queue, argument);
}

void
addArgumentsFromString (CommandArguments *arguments, char *string) {
  const char *delimiters = " ";
  char *argument;

  while ((argument = strtok(string, delimiters))) {
    addArgument(arguments, argument);
    string = NULL;
  }
}

void
addArgumentsFromArray (CommandArguments *arguments, char **array, size_t count) {
  char **end = array + count;
  while (array < end) addArgument(arguments, *array++);
}

char *
getNextArgument (CommandArguments *arguments, const char *name) {
  char *argument = dequeueItem(arguments->queue);

  if (!argument) {
    logMessage(LOG_ERR, "missing %s", name);
  }

  return argument;
}

void
restoreArgument (CommandArguments *arguments, char *argument) {
  prequeueItem(arguments->queue, argument);
}

int
checkNoMoreArguments (CommandArguments *arguments) {
  return isEmptyQueue(arguments->queue);
}

int
verifyNoMoreArguments (CommandArguments *arguments) {
  const char *argument = dequeueItem(arguments->queue);
  if (!argument) return 1;

  logMessage(LOG_ERR, "too many arguments: %s", argument);
  return 0;
}

int
parseInteger (int *value, const char *argument, int minimum, int maximum, const char *name) {
  if (validateInteger(value, argument, &minimum, &maximum)) return 1;

  logMessage(LOG_ERR,
    "invalid %s: %s (must be an integer >= %d and <= %d)",
    name, argument, minimum, maximum
  );

  return 0;
}

int
parseFloat (float *value, const char *argument, float minimum, float maximum, int inclusive, const char *name) {
  if (validateFloat(value, argument, &minimum, &maximum)) {
    if (inclusive || (*value < maximum)) {
      return 1;
    }
  }

  logMessage(LOG_ERR,
    "invalid %s: %s (must be a real number >= %g and %s %g)",
    name, argument, minimum, (inclusive? "<=": "<"), maximum
  );

  return 0;
}

int
parseDegrees (float *degrees, const char *argument, const char *name) {
  return parseFloat(degrees, argument, 0.0f, 360.0f, 0, name);
}

int
parsePercent (float *value, const char *argument, const char *name) {
  const float maximum = 100.0f;
  if (!parseFloat(value, argument, 0.0f, maximum, 1, name)) return 0;

  *value /= maximum;
  return 1;
}

size_t
getConsoleWidth (void) {
  size_t width = UINT16_MAX;
  getConsoleSize(&width, NULL);
  return width;
}

const char *
getTranslatedText (const char *text) {
  if (!text) return "";
  if (*text) text = gettext(text);
  return text;
}

static void
checkForOutputError (void) {
  if (ferror(stdout)) {
    logSystemError("standard output write");
    exit(PROG_EXIT_FATAL);
  }
}

void
putFlush (void) {
  fflush(stdout);
  checkForOutputError();
}

void
putConsole (const char *bytes, size_t count) {
  writeWithConsoleEncoding(stdout, bytes, count);
  checkForOutputError();
}

void
putString (const char *string) {
  fputs(string, stdout);
  checkForOutputError();
}

void
putBytes (const void *bytes, size_t count) {
  fwrite(bytes, 1, count, stdout);
  checkForOutputError();
}

void
putByte (char byte) {
  fputc(byte, stdout);
  checkForOutputError();
}

void
putNewline (void) {
  putByte('\n');
}

void
vputf (const char *format, va_list args) {
  vprintf(format, args);
  checkForOutputError();
}

void
putf (const char *format, ...) {
  va_list args;
  va_start(args, format);
  vputf(format, args);
  va_end(args);
}

void
putWrappedText (
  const char *text, char *line,
  unsigned int lineIndent, unsigned int lineWidth
) {
  unsigned int textWidth = lineWidth - lineIndent;
  unsigned int textLeft = strlen(text);

  while (1) {
    unsigned int textLength = textLeft;

    if (textLength > textWidth) {
      textLength = textWidth;

      while (textLength > 0) {
        if (isspace(text[textLength])) break;
        textLength -= 1;
      }

      while (textLength > 0) {
        if (!isspace(text[--textLength])) {
          textLength += 1;
          break;
        }
      }
    }

    {
      unsigned int lineLength = lineIndent + textLength;

      if (textLength > 0) {
        memcpy(line+lineIndent, text, textLength);
      } else {
        while (lineLength > 0) {
          if (!isspace(line[--lineLength])) {
            lineLength += 1;
            break;
          }
        }
      }

      if (lineLength > 0) {
        putBytes(line, lineLength);
        putNewline();
      }

      while (textLength < textLeft) {
        if (!isspace(text[textLength])) break;
        textLength += 1;
      }

      if (!(textLeft -= textLength)) break;
      text += textLength;
      memset(line, ' ', lineIndent);
    }
  }
}

void
putFormattedLines (
  const char *const *const *blocks,
  char *line, int lineWidth
) {
  const char *const *const *block = blocks;

  char *paragraphText = NULL;
  size_t paragraphSize = 0;
  size_t paragraphLength = 0;

  while (*block) {
    const char *const *chunk = *block++;
    if (!*chunk) continue;

    while (1) {
      const char *text = *chunk;
      if (!text) break;
      text = getTranslatedText(text);

      if (*text && !iswspace(*text)) {
        size_t textLength = strlen(text);
        size_t newLength = paragraphLength + textLength + 1;

        int extendingParagraph = !!paragraphLength;
        if (extendingParagraph) newLength += 1;

        if (newLength > paragraphSize) {
          size_t newSize = (newLength | 0XFF) + 1;
          char *newText = realloc(paragraphText, newSize);

          if (!newText) {
            logMallocError();
            goto done;
          }

          paragraphText = newText;
          paragraphSize = newSize;
        }

        if (extendingParagraph) paragraphText[paragraphLength++] = ' ';
        memcpy(&paragraphText[paragraphLength], text, textLength);
        paragraphText[paragraphLength += textLength] = 0;
      } else {
        if (paragraphLength) {
          putWrappedText(paragraphText, line, 0, lineWidth);
          paragraphLength = 0;
        }

        putString(text);
        putNewline();
      }

      chunk += 1;
    }

    if (paragraphLength) {
      putWrappedText(paragraphText, line, 0, lineWidth);
      paragraphLength = 0;
    }

    if (*block) putNewline();
  }

done:
  if (paragraphText) free(paragraphText);
}

void
putUtf8Character (wchar_t character) {
  if (character) {
    Utf8Buffer utf8;
    size_t utfs = convertWcharToUtf8(character, utf8);
    putBytes(utf8, utfs);
  } else {
    static const unsigned char bytes[] = {0XC0, 0X80};
    putBytes(bytes, ARRAY_COUNT(bytes));
  }
}

void
putUtf8Characters (const wchar_t *characters, size_t count) {
  const wchar_t *character = characters;
  const wchar_t *end = character + count;
  while (character < end) putUtf8Character(*character++);
}

void
putUtf8String (const wchar_t *string) {
  putUtf8Characters(string, wcslen(string));
}

void
putHexadecimalCharacter (wchar_t character) {
  writeHexadecimalCharacter(stdout, character);
  checkForOutputError();
}

void
putHexadecimalCharacters (const wchar_t *characters, size_t count) {
  const wchar_t *character = characters;
  const wchar_t *end = character + count;
  while (character < end) putHexadecimalCharacter(*character++);
}

void
putHexadecimalString (const wchar_t *string) {
  putHexadecimalCharacters(string, wcslen(string));
}

void
beginInteractiveMode (void) {
  pushLogPrefix("ERROR");
}

void
endInteractiveMode (void) {
  popLogPrefix();
}
