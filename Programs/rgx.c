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
#include "strfmt.h"
#include "rgx.h"
#include "charset.h"
#include "queue.h"

struct RGX_ObjectStruct {
  void *data;
  Queue *patterns;
  uint32_t options;
};

struct RGX_PatternStruct {
  struct {
    wchar_t *characters;
    size_t length;
  } expression;

  struct {
    pcre2_code *code;
    pcre2_match_data *matches;
  } compiled;

  struct {
    void *data;
    RGX_MatchHandler *handler;
    uint32_t options;
  } match;
};

static void
rgxLogError (const RGX_Pattern *pattern, int error, PCRE2_SIZE *offset) {
  char log[0X100];
  STR_BEGIN(log, sizeof(log));

  STR_PRINTF("regular expression error %d", error);
  if (offset) STR_PRINTF(" at offset %"PRIsize, *offset);

  {
    size_t size = 0X100;
    PCRE2_UCHAR message[size];
    int length = pcre2_get_error_message(error, message, size);

    if (length > 0) {
      STR_PRINTF(": ");

      for (unsigned int index=0; index<length; index+=1) {
        STR_PRINTF("%"PRIwc, message[index]);
      }
    }
  }

  STR_END;
  logMessage(LOG_WARNING, "%s", log);
}

static void
rgxDeallocatePattern (void *item, void *data) {
  RGX_Pattern *pattern = item;

  pcre2_match_data_free(pattern->compiled.matches);
  pcre2_code_free(pattern->compiled.code);
  free(pattern->expression.characters);
  free(pattern);
}

RGX_Pattern *
rgxAddPatternCharacters (
  RGX_Object *rgx,
  const wchar_t *characters, size_t length,
  RGX_MatchHandler *handler, void *data
) {
  RGX_Pattern *pattern;

  if ((pattern = malloc(sizeof(*pattern)))) {
    memset(pattern, 0, sizeof(*pattern));

    pattern->match.data = data;
    pattern->match.handler = handler;
    pattern->match.options = 0;

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
            return pattern;
          }

          pcre2_match_data_free(pattern->compiled.matches);
        } else {
          logMallocError();
        }

        pcre2_code_free(pattern->compiled.code);
      } else {
        rgxLogError(pattern, error, &offset);
      }

      free(pattern->expression.characters);
    } else {
      logMallocError();
    }

    free(pattern);
  } else {
    logMallocError();
  }

  return NULL;
}

RGX_Pattern *
rgxAddPatternString (
  RGX_Object *rgx,
  const wchar_t *string,
  RGX_MatchHandler *handler, void *data
) {
  return rgxAddPatternCharacters(rgx, string, wcslen(string), handler, data);
}

RGX_Pattern *
rgxAddPatternUTF8 (
  RGX_Object *rgx,
  const char *string,
  RGX_MatchHandler *handler, void *data
) {
  size_t size = strlen(string) + 1;
  wchar_t characters[size];

  const char *from = string;
  wchar_t *to = characters;
  convertUtf8ToWchars(&from, &to, size);

  return rgxAddPatternCharacters(
    rgx, characters, (to - characters), handler, data
  );
}

static int
rgxTestPattern (const void *item, void *data) {
  const RGX_Pattern *pattern = item;
  RGX_Match *match = data;

  int count = pcre2_match(
    pattern->compiled.code, match->text.internal, match->text.length,
    0, pattern->match.options, pattern->compiled.matches, NULL
  );

  if (count > 0) {
    match->data.pattern = pattern->match.data;

    match->pattern.characters = pattern->expression.characters;
    match->pattern.length = pattern->expression.length;

    match->captures.internal = pattern->compiled.matches;
    match->captures.count = count - 1;

    pattern->match.handler(match);
    return 1;
  }

  if (count != PCRE2_ERROR_NOMATCH) rgxLogError(pattern, count, NULL);
  return 0;
}

int
rgxMatchTextCharacters (
  RGX_Object *rgx,
  const wchar_t *characters, size_t length,
  void *data
) {
  PCRE2_UCHAR internal[length];

  for (unsigned int index=0; index<length; index+=1) {
    internal[index] = characters[index];
  }

  RGX_Match match = {
    .text = {
      .internal = internal,
      .characters = characters,
      .length = length
    },

    .data = {
      .object = rgx->data,
      .match = data
    }
  };

  return !!findElement(rgx->patterns, rgxTestPattern, &match);
}

int
rgxMatchTextString (
  RGX_Object *rgx,
  const wchar_t *string,
  void *data
) {
  return rgxMatchTextCharacters(rgx, string, wcslen(string), data);
}

int
rgxMatchTextUTF8 (
  RGX_Object *rgx,
  const char *string,
  void *data
) {
  size_t size = strlen(string) + 1;
  wchar_t characters[size];

  const char *from = string;
  wchar_t *to = characters;
  convertUtf8ToWchars(&from, &to, size);

  return rgxMatchTextCharacters(
    rgx, characters, (to - characters), data
  );
}

size_t
rgxGetCaptureCount (
  const RGX_Match *match
) {
  return match->captures.count;
}

int
rgxGetCaptureBounds (
  const RGX_Match *match,
  unsigned int index, int *from, int *to
) {
  if (index < 0) return 0;
  if (index > match->captures.count) return 0;

  const PCRE2_SIZE *offsets = pcre2_get_ovector_pointer(match->captures.internal);
  offsets += index * 2;

  if ((*from = offsets[0]) == PCRE2_UNSET) return 0;
  if ((*to = offsets[1]) == PCRE2_UNSET) return 0;
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
