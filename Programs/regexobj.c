/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
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
#include <regex.h>

#include "log.h"
#include "regexobj.h"
#include "queue.h"

struct RegularExpressionObjectStruct {
  Queue *patterns;
  int compileFlags;
  void *data;
};

typedef struct {
  char *expression;
  regex_t *matcher;
  size_t submatches;
  RegularExpressionHandler *handler;
  void *data;
} RegularExpressionPattern;

static void
deallocateRegularExpressionPattern (void *item, void *data) {
  RegularExpressionPattern *pattern = item;

  regfree(pattern->matcher);
  free(pattern->expression);
  free(pattern);
}

int
addRegularExpression (
  RegularExpressionObject *regex,
  const char *expression,
  size_t submatches,
  RegularExpressionHandler *handler,
  void *data
) {
  RegularExpressionPattern *pattern;

  if ((pattern = malloc(sizeof(*pattern)))) {
    memset(pattern, 0, sizeof(*pattern));
    pattern->submatches = submatches;
    pattern->handler = handler;
    pattern->data = data;

    if ((pattern->expression = strdup(expression))) {
      int result = regcomp(pattern->matcher, expression, regex->compileFlags);

      if (!result) {
        if (enqueueItem(regex->patterns, pattern)) {
          return 1;
        }

        regfree(pattern->matcher);
      } else {
        char message[0X100];
        regerror(result, pattern->matcher, message, sizeof(message));
        logMessage(LOG_WARNING, "regular expression error: %s", message);
      }

      free(pattern->expression);
    } else {
      logMallocError();
    }

    free(pattern);
  } else {
    logMallocError();
  }

  return 0;
}

typedef struct {
  const char *string;
  RegularExpressionHandlerParameters *parameters;
} TestRegularExpressionPatternData;

static int
testRegularExpressionPattern (const void *item, void *data) {
  const RegularExpressionPattern *pattern = item;
  TestRegularExpressionPatternData *tpd = data;

  size_t matches = pattern->submatches + 1;
  regmatch_t match[matches];
  int result = regexec(pattern->matcher, tpd->string, matches, match, 0);

  if (!result) {
    RegularExpressionHandlerParameters *parameters = tpd->parameters;
    parameters->patternData = pattern->data;

    pattern->handler(parameters);
    return 1;
  }

  return 0;
}

int
matchRegularExpressions (
  RegularExpressionObject *regex,
  const char *string,
  void *data
) {
  RegularExpressionHandlerParameters parameters = {
    .objectData = regex->data,
    .matchData = data
  };

  TestRegularExpressionPatternData tpd = {
    .string = string,
    .parameters = &parameters
  };

  return !!findElement(regex->patterns, testRegularExpressionPattern, &tpd);
}

RegularExpressionObject *
newRegularExpressionObject (void *data) {
  RegularExpressionObject *regex;

  if ((regex = malloc(sizeof(*regex)))) {
    memset(regex, 0, sizeof(*regex));
    regex->data = data;

    regex->compileFlags = 0;
    regex->compileFlags |= REG_EXTENDED;

    if ((regex->patterns = newQueue(deallocateRegularExpressionPattern, NULL))) {
      return regex;
    }

    free(regex);
  } else {
    logMallocError();
  }

  return NULL;
}

void
destroyRegularExpressionObject (RegularExpressionObject *regex) {
  deallocateQueue(regex->patterns);
  free(regex);
}
