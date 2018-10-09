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

#ifdef HAVE_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 32
#include <pcre2.h>
#else /* Unicode regular expression support */
#warning Unicode regular expression support has not been included
#endif /* Unicode regular expression support */

#include "log.h"
#include "rgx.h"
#include "queue.h"

struct RGX_ObjectStruct {
  uint32_t options;
  Queue *patterns;
  void *data;
};

typedef struct {
  struct {
    wchar_t *characters;
    size_t length;
  } expression;

  struct {
    pcre2_code *code;
    pcre2_match_data *matches;
  } compiled;

  uint32_t options;
  RGX_MatchHandler *handler;
  void *data;
} RGX_Pattern;

static void
rgxLogError (const RGX_Pattern *pattern, int error) {
  size_t size = 0X100;
  PCRE2_UCHAR message[size];
  pcre2_get_error_message(error, message, size);
  wchar_t characters[size];

  {
    const PCRE2_UCHAR *from = message;
    wchar_t *to = characters;
    while ((*to++ = *from++));
  }

  logMessage(LOG_WARNING,
    "regular expression error %d: %"PRIws,
    error, characters
  );
}

static void
rgxDeallocatePattern (void *item, void *data) {
  RGX_Pattern *pattern = item;

  pcre2_match_data_free(pattern->compiled.matches);
  pcre2_code_free(pattern->compiled.code);
  free(pattern->expression.characters);
  free(pattern);
}

int
rgxAddPatternCharacters (
  RGX_Object *rgx,
  const wchar_t *characters, size_t length,
  RGX_MatchHandler *handler, void *data
) {
  RGX_Pattern *pattern;

  if ((pattern = malloc(sizeof(*pattern)))) {
    memset(pattern, 0, sizeof(*pattern));

    pattern->options = 0;
    pattern->handler = handler;
    pattern->data = data;

    pattern->expression.characters = calloc(
      (pattern->expression.length = length),
      sizeof(*pattern->expression.characters)
    );

    if (pattern->expression.characters) {
      PCRE2_UCHAR internal[length];

      for (unsigned int index=0; index<length; index+=1) {
        wchar_t character = characters[index];
        internal[index] = character;
        pattern->expression.characters[index] = character;
      }

      int error;
      PCRE2_SIZE offset;

      pattern->compiled.code = pcre2_compile(
        internal, length, rgx->options,
        &error, &offset, NULL
      );

      if (pattern->compiled.code) {
        pattern->compiled.matches = pcre2_match_data_create_from_pattern(
          pattern->compiled.code, NULL
        );

        if (pattern->compiled.matches) {
          if (enqueueItem(rgx->patterns, pattern)) {
            return 1;
          }

          pcre2_match_data_free(pattern->compiled.matches);
        } else {
          logMallocError();
        }

        pcre2_code_free(pattern->compiled.code);
      } else {
        rgxLogError(pattern, error);
      }

      free(pattern->expression.characters);
    } else {
      logMallocError();
    }

    free(pattern);
  } else {
    logMallocError();
  }

  return 0;
}

int
rgxAddPatternString (
  RGX_Object *rgx,
  const wchar_t *string,
  RGX_MatchHandler *handler, void *data
) {
  return rgxAddPatternCharacters(rgx, string, wcslen(string), handler, data);
}

static int
rgxTestPattern (const void *item, void *data) {
  const RGX_Pattern *pattern = item;
  RGX_MatchHandlerParameters *parameters = data;

  int matches = pcre2_match(
    pattern->compiled.code,
    parameters->string.internal, parameters->string.length,
    0, pattern->options, pattern->compiled.matches, NULL
  );

  if (matches > 0) {
    parameters->data.pattern = pattern->data;

    parameters->expression.characters = pattern->expression.characters;
    parameters->expression.length = pattern->expression.length;

    parameters->matches.internal = pattern->compiled.matches;
    parameters->matches.count = matches - 1;

    pattern->handler(parameters);
    return 1;
  }

  if (matches != PCRE2_ERROR_NOMATCH) rgxLogError(pattern, matches);
  return 0;
}

int
rgxMatchPatternsCharacters (
  RGX_Object *rgx,
  const wchar_t *characters, size_t length,
  void *data
) {
  PCRE2_UCHAR internal[length];

  for (unsigned int index=0; index<length; index+=1) {
    internal[index] = characters[index];
  }

  RGX_MatchHandlerParameters parameters = {
    .string = {
      .characters = characters,
      .internal = internal,
      .length = length
    },

    .data = {
      .object = rgx->data,
      .match = data
    }
  };

  return !!findElement(rgx->patterns, rgxTestPattern, &parameters);
}

int
rgxMatchPatternsString (
  RGX_Object *rgx,
  const wchar_t *string,
  void *data
) {
  return rgxMatchPatternsCharacters(rgx, string, wcslen(string), data);
}

unsigned int
rgxGetMatchCount (
  const RGX_MatchHandlerParameters *parameters
) {
  return parameters->matches.count;
}

int
rgxGetMatch (
  const RGX_MatchHandlerParameters *parameters,
  unsigned int index, int *start, int *end
) {
  if (index < 0) return 0;
  if (index > parameters->matches.count) return 0;

  PCRE2_SIZE *matches = pcre2_get_ovector_pointer(parameters->matches.internal);
  matches += index * 2;

  if ((*start = matches[0]) == -1) return 0;
  if ((*end = matches[1]) == -1) return 0;
  return 1;
}

RGX_Object *
rgxNewObject (void *data) {
  RGX_Object *rgx;

  if ((rgx = malloc(sizeof(*rgx)))) {
    memset(rgx, 0, sizeof(*rgx));
    rgx->data = data;
    rgx->options = 0;

    if ((rgx->patterns = newQueue(rgxDeallocatePattern, NULL))) {
      return rgx;
    }

    free(rgx);
  } else {
    logMallocError();
  }

  return NULL;
}

void
rgxDestroyObject (RGX_Object *rgx) {
  deallocateQueue(rgx->patterns);
  free(rgx);
}
