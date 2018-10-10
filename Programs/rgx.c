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

#include "log.h"
#include "rgx.h"
#include "charset.h"
#include "queue.h"
#include "strfmt.h"

#define RGX_BEGIN_OPTION_MAP(name) static const RGX_OptionsType name[] = {
#define RGX_END_OPTION_MAP };

#ifdef HAVE_PCRE2
#include "rgx_pcre2.h"

RGX_BEGIN_OPTION_MAP(rgxCompileOptionMap)
  [RGX_COMPILE_ANCHOR_START] = PCRE2_ANCHORED,
  [RGX_COMPILE_ANCHOR_END] = PCRE2_ENDANCHORED,

  [RGX_COMPILE_IGNORE_CASE] = PCRE2_CASELESS,
  [RGX_COMPILE_LITERAL_TEXT] = PCRE2_LITERAL,
  [RGX_COMPILE_UNICODE_PROPERTIES] = PCRE2_UCP,
RGX_END_OPTION_MAP

RGX_BEGIN_OPTION_MAP(rgxMatchOptionMap)
  [RGX_MATCH_ANCHOR_START] = PCRE2_ANCHORED,
  [RGX_MATCH_ANCHOR_END] = PCRE2_ENDANCHORED,
RGX_END_OPTION_MAP

#else /* Unicode regular expression support */
#warning Unicode regular expression support has not been included
#endif /* Unicode regular expression support */

struct RGX_ObjectStruct {
  void *data;
  Queue *matchers;
  RGX_OptionsType options;
};

struct RGX_MatcherStruct {
  void *data;
  RGX_MatchHandler *handler;
  RGX_OptionsType options;

  struct {
    wchar_t *characters;
    size_t length;
  } pattern;

  struct {
    RGX_CodeType *code;
    RGX_DataType *data;
  } compiled;
};

static void
rgxLogError (const RGX_Matcher *matcher, int error, RGX_OffsetType *offset) {
  char log[0X100];
  STR_BEGIN(log, sizeof(log));

  STR_PRINTF("regular expression error %d", error);
  if (offset) STR_PRINTF(" at offset %"PRIsize, *offset);

  {
    size_t size = 0X100;
    RGX_CharacterType message[size];
    int length = pcre2_get_error_message(error, message, size);

    if (length > 0) {
      STR_PRINTF(": ");

      for (unsigned int index=0; index<length; index+=1) {
        STR_PRINTF("%"PRIwc, message[index]);
      }
    }
  }

  if (matcher) {
    STR_PRINTF(
      ": %.*"PRIws, (int)matcher->pattern.length, matcher->pattern.characters
    );
  }

  STR_END;
  logMessage(LOG_WARNING, "%s", log);
}

static void
rgxDeallocateMatcher (void *item, void *data) {
  RGX_Matcher *matcher = item;

  rgxDeallocateData(matcher->compiled.data);
  rgxDeallocateCode(matcher->compiled.code);
  free(matcher->pattern.characters);
  free(matcher);
}

RGX_Matcher *
rgxAddPatternCharacters (
  RGX_Object *rgx,
  const wchar_t *characters, size_t length,
  RGX_MatchHandler *handler, void *data
) {
  RGX_Matcher *matcher;

  if ((matcher = malloc(sizeof(*matcher)))) {
    memset(matcher, 0, sizeof(*matcher));
    matcher->data = data;
    matcher->handler = handler;
    matcher->options = 0;

    matcher->pattern.characters = calloc(
      (matcher->pattern.length = length),
      sizeof(*matcher->pattern.characters)
    );

    if (matcher->pattern.characters) {
      RGX_CharacterType internal[length];

      for (unsigned int index=0; index<length; index+=1) {
        wchar_t character = characters[index];
        internal[index] = character;
        matcher->pattern.characters[index] = character;
      }

      int error;
      RGX_OffsetType offset;

      matcher->compiled.code = rgxCompile(
        internal, length, rgx->options, &offset, &error
      );

      if (matcher->compiled.code) {
        matcher->compiled.data = rgxAllocateData(matcher->compiled.code);

        if (matcher->compiled.data) {
          if (enqueueItem(rgx->matchers, matcher)) {
            return matcher;
          }

          rgxDeallocateData(matcher->compiled.data);
        } else {
          logMallocError();
        }

        rgxDeallocateCode(matcher->compiled.code);
      } else {
        rgxLogError(matcher, error, &offset);
      }

      free(matcher->pattern.characters);
    } else {
      logMallocError();
    }

    free(matcher);
  } else {
    logMallocError();
  }

  return NULL;
}

RGX_Matcher *
rgxAddPatternString (
  RGX_Object *rgx,
  const wchar_t *string,
  RGX_MatchHandler *handler, void *data
) {
  return rgxAddPatternCharacters(rgx, string, wcslen(string), handler, data);
}

RGX_Matcher *
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
rgxTestMatcher (const void *item, void *data) {
  const RGX_Matcher *matcher = item;
  RGX_Match *match = data;

  int error;
  int matched = rgxMatch(
    match->text.internal, match->text.length,
    matcher->compiled.code, matcher->compiled.data,
    matcher->options, &match->captures.count, &error
  );

  if (!matched) {
    if (error != RGX_NO_MATCH) rgxLogError(matcher, error, NULL);
    return 0;
  }

  {
    RGX_MatchHandler *handler = matcher->handler;

    if (handler) {
      match->captures.data = matcher->compiled.data;
      match->data.pattern = matcher->data;

      match->pattern.characters = matcher->pattern.characters;
      match->pattern.length = matcher->pattern.length;

      handler(match);
    }
  }

  return 1;
}

int
rgxMatchTextCharacters (
  RGX_Object *rgx,
  const wchar_t *characters, size_t length,
  void *data
) {
  RGX_CharacterType internal[length];

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

  return !!findElement(rgx->matchers, rgxTestMatcher, &match);
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

  return rgxMatchTextCharacters(rgx, characters, (to - characters), data);
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
  size_t index, size_t *from, size_t *to
) {
  if (index > match->captures.count) return 0;

  const RGX_OffsetType *offsets = pcre2_get_ovector_pointer(match->captures.data);
  offsets += index * 2;

  if (offsets[0] == PCRE2_UNSET) return 0;
  if (offsets[1] == PCRE2_UNSET) return 0;

  *from = offsets[0];
  *to = offsets[1];
  return 1;
}

RGX_Object *
rgxNewObject (void *data) {
  RGX_Object *rgx;

  if ((rgx = malloc(sizeof(*rgx)))) {
    memset(rgx, 0, sizeof(*rgx));
    rgx->data = data;
    rgx->options = 0;

    if ((rgx->matchers = newQueue(rgxDeallocateMatcher, NULL))) {
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
  deallocateQueue(rgx->matchers);
  free(rgx);
}

static int
rgxOption (
  RGX_OptionAction action, RGX_CompileOption option,
  RGX_OptionsType *bits, const RGX_OptionsType *map, size_t end
) {
  RGX_OptionsType bit = ((option >= 0) && (option < end))? map[option]: 0;
  int wasSet = !!(*bits & bit);

  if (action == RGX_OPTION_TOGGLE) {
    action = wasSet? RGX_OPTION_CLEAR: RGX_OPTION_SET;
  }

  switch (action) {
    case RGX_OPTION_SET:
      *bits |= bit;
      break;

    case RGX_OPTION_CLEAR:
      *bits &= ~bit;
      break;

    default:
      logMessage(LOG_WARNING, "unimplemented regular expression option action: %d", action);
      /* fall through */
    case RGX_OPTION_TEST:
      break;
  }

  return wasSet;
}

int
rgxCompileOption (
  RGX_Object *rgx,
  RGX_OptionAction action,
  RGX_CompileOption option
) {
  return rgxOption(
    action, option, &rgx->options,
    rgxCompileOptionMap, ARRAY_COUNT(rgxCompileOptionMap)
  );
}

int
rgxMatchOption (
  RGX_Matcher *matcher,
  RGX_OptionAction action,
  RGX_MatchOption option
) {
  return rgxOption(
    action, option, &matcher->options,
    rgxMatchOptionMap, ARRAY_COUNT(rgxMatchOptionMap)
  );
}
