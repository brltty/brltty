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
#include "rgx.h"
#include "queue.h"

struct RegularExpressionObjectStruct {
  Queue *patterns;
  int compileFlags;
  void *data;
};

typedef struct {
  char *expression;
  regex_t matcher;
  size_t submatches;
  RegularExpressionHandler *handler;
  void *data;
} RegularExpressionPattern;

static void
logRegularExpressionError (const RegularExpressionPattern *pattern, int error) {
  char message[0X100];
  regerror(error, &pattern->matcher, message, sizeof(message));
  logMessage(LOG_WARNING, "regular expression error: %s", message);
}

static void
deallocateRegularExpressionPattern (void *item, void *data) {
  RegularExpressionPattern *pattern = item;

  regfree(&pattern->matcher);
  free(pattern->expression);
  free(pattern);
}

int
addRegularExpression (
  RegularExpressionObject *rgx,
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
      int error = regcomp(&pattern->matcher, expression, rgx->compileFlags);

      if (!error) {
        if (enqueueItem(rgx->patterns, pattern)) {
          return 1;
        }

        regfree(&pattern->matcher);
      } else {
        logRegularExpressionError(pattern, error);
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

static int
testRegularExpressionPattern (const void *item, void *data) {
  const RegularExpressionPattern *pattern = item;
  RegularExpressionHandlerParameters *parameters = data;

  size_t matches = pattern->submatches + 1;
  regmatch_t match[matches];
  int error = regexec(&pattern->matcher, parameters->string, matches, match, 0);

  if (!error) {
    parameters->expression = pattern->expression;
    parameters->patternData = pattern->data;

    parameters->matches.array = match;
    parameters->matches.size = matches;

    pattern->handler(parameters);
    return 1;
  }

  if (error != REG_NOMATCH) logRegularExpressionError(pattern, error);
  return 0;
}

int
matchRegularExpressions (
  RegularExpressionObject *rgx,
  const char *string,
  void *data
) {
  RegularExpressionHandlerParameters parameters = {
    .string = string,
    .objectData = rgx->data,
    .matchData = data
  };

  return !!findElement(rgx->patterns, testRegularExpressionPattern, &parameters);
}

int
getRegularExpressionMatch (
  const RegularExpressionHandlerParameters *parameters,
  unsigned int index, int *start, int *end
) {
  if (index < 0) return 0;
  if (index >= parameters->matches.size) return 0;

  const regmatch_t *matches = parameters->matches.array;
  const regmatch_t *match = &matches[index];

  if ((*start = match->rm_so) == -1) return 0;
  if ((*end = match->rm_eo) == -1) return 0;
  return 1;
}

RegularExpressionObject *
newRegularExpressionObject (void *data) {
  RegularExpressionObject *rgx;

  if ((rgx = malloc(sizeof(*rgx)))) {
    memset(rgx, 0, sizeof(*rgx));
    rgx->data = data;

    rgx->compileFlags = 0;
    rgx->compileFlags |= REG_EXTENDED;

    if ((rgx->patterns = newQueue(deallocateRegularExpressionPattern, NULL))) {
      return rgx;
    }

    free(rgx);
  } else {
    logMallocError();
  }

  return NULL;
}

void
destroyRegularExpressionObject (RegularExpressionObject *rgx) {
  deallocateQueue(rgx->patterns);
  free(rgx);
}
