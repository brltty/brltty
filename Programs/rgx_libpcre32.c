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

#include "rgx.h"
#include "rgx_internal.h"
#include "strfmt.h"

RGX_CodeType *
rgxCompilePattern (
  const RGX_CharacterType *characters, size_t length,
  RGX_OptionsType options, RGX_OffsetType *offset,
  int *error
) {
  const char *message;

  return pcre32_compile2(
    characters, options, error, &message, offset, NULL
  );
}

void
rgxDeallocateCode (RGX_CodeType *code) {
  pcre32_free(code);
}

RGX_DataType *
rgxAllocateData (RGX_CodeType *code) {
  RGX_DataType *data;
  size_t size = sizeof(*data);

  size_t matches = 10;
  size_t count = matches * 3;
  size += count * sizeof(data->offsets[0]);

  if (!(data = malloc(size))) return NULL;
  memset(data, 0, size);

  data->matches = matches;
  data->count = count;

  {
    const char *message;
    data->study = pcre32_study(code, 0, &message);
  }

  return data;
}

void
rgxDeallocateData (RGX_DataType *data) {
  if (data->study) pcre32_free_study(data->study);
  free(data);
}

int
rgxMatchText (
  const RGX_CharacterType *characters, size_t length,
  RGX_CodeType *code, RGX_DataType *data,
  RGX_OptionsType options, size_t *count, int *error
) {
  int result = pcre32_exec(
    code, data->study,
    characters, length,
    0, options,
    data->offsets, data->count
  );

  if (result < 0) {
    *error = result;
    return 0;
  }

  if (!result) result = data->matches;
  *count = result - 1;
  return 1;
}

int
rgxNameNumber (
  RGX_CodeType *code, const RGX_CharacterType *name,
  size_t *number, int *error
) {
  int result = pcre32_get_stringnumber(code, name);

  if (result > 0) {
    *number = result;
    return 1;
  } else {
    *error = result;
    return 0;
  }
}

int
rgxCaptureBounds (
  RGX_DataType *data, size_t number, size_t *from, size_t *to
) {
  const RGX_OffsetType *offsets = data->offsets;
  offsets += number * 2;

  if (offsets[0] == -1) return 0;
  if (offsets[1] == -1) return 0;

  *from = offsets[0];
  *to = offsets[1];
  return 1;
}

STR_BEGIN_FORMATTER(rgxFormatErrorMessage, int error)
  const char *message;

  switch (error) {
    case PCRE_ERROR_NOMATCH:
      message = "no match";
      break;

    case PCRE_ERROR_NULL:
      message = "null";
      break;

    case PCRE_ERROR_BADOPTION:
      message = "bad option";
      break;

    case PCRE_ERROR_BADMAGIC:
      message = "bad magic";
      break;

    case PCRE_ERROR_UNKNOWN_OPCODE:
      message = "unknown opcode";
      break;

    case PCRE_ERROR_NOMEMORY:
      message = "no memory";
      break;

    case PCRE_ERROR_NOSUBSTRING:
      message = "no substring";
      break;

    case PCRE_ERROR_MATCHLIMIT:
      message = "match limit";
      break;

    case PCRE_ERROR_CALLOUT:
      message = "call out";
      break;

    case PCRE_ERROR_BADUTF32:
      message = "bad UTF32";
      break;

    case PCRE_ERROR_BADUTF16_OFFSET:
      message = "bad UTF16 offset";
      break;

    case PCRE_ERROR_PARTIAL:
      message = "partial";
      break;

    case PCRE_ERROR_BADPARTIAL:
      message = "bad partial";
      break;

    case PCRE_ERROR_INTERNAL:
      message = "internal";
      break;

    case PCRE_ERROR_BADCOUNT:
      message = "bad count";
      break;

    case PCRE_ERROR_DFA_UITEM:
      message = "dfa uitem";
      break;

    case PCRE_ERROR_DFA_UCOND:
      message = "dfa ucond";
      break;

    case PCRE_ERROR_DFA_UMLIMIT:
      message = "dfa umlimit";
      break;

    case PCRE_ERROR_DFA_WSSIZE:
      message = "dfa wssize";
      break;

    case PCRE_ERROR_DFA_RECURSE:
      message = "dfa recurse";
      break;

    case PCRE_ERROR_RECURSIONLIMIT:
      message = "recursion limit";
      break;

    case PCRE_ERROR_NULLWSLIMIT:
      message = "null ws limit";
      break;

    case PCRE_ERROR_BADNEWLINE:
      message = "bad newline";
      break;

    case PCRE_ERROR_BADOFFSET:
      message = "bad offset";
      break;

    case PCRE_ERROR_SHORTUTF16:
      message = "short UTF16";
      break;

    case PCRE_ERROR_RECURSELOOP:
      message = "recurse loop";
      break;

    case PCRE_ERROR_JIT_STACKLIMIT:
      message = "jit stack limit";
      break;

    case PCRE_ERROR_BADMODE:
      message = "bad mode";
      break;

    case PCRE_ERROR_BADENDIANNESS:
      message = "bad endianness";
      break;

    case PCRE_ERROR_DFA_BADRESTART:
      message = "dfa bad restart";
      break;

    case PCRE_ERROR_JIT_BADOPTION:
      message = "jit bad option";
      break;

    case PCRE_ERROR_BADLENGTH:
      message = "bad length";
      break;

    case PCRE_ERROR_UNSET:
      message = "unset";
      break;

    default:
      message = NULL;
      break;
  }

  if (message) STR_PRINTF("%s", message);
STR_END_FORMATTER

RGX_BEGIN_OPTION_MAP(rgxCompileOptions)
  [RGX_COMPILE_ANCHOR_START] = PCRE_ANCHORED,

  [RGX_COMPILE_IGNORE_CASE] = PCRE_CASELESS,
  [RGX_COMPILE_UNICODE_PROPERTIES] = PCRE_UCP,
RGX_END_OPTION_MAP(rgxCompileOptions)

RGX_BEGIN_OPTION_MAP(rgxMatchOptions)
  [RGX_MATCH_ANCHOR_START] = PCRE_ANCHORED,
RGX_END_OPTION_MAP(rgxMatchOptions)
