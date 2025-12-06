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

#include "log.h"
#include "cmdlib.h"
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

static void
checkForOutputError (void) {
  if (ferror(stdout)) {
    logSystemError("standard output write");
    exit(PROG_EXIT_FATAL);
  }
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
flushOutput (void) {
  fflush(stdout);
  checkForOutputError();
}

void
beginInteractiveMode (void) {
  pushLogPrefix("ERROR");
}

void
endInteractiveMode (void) {
  popLogPrefix();
}
