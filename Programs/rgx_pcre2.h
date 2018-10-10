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

#ifndef BRLTTY_INCLUDED_RGX_PCRE2
#define BRLTTY_INCLUDED_RGX_PCRE2

#define PCRE2_CODE_UNIT_WIDTH 32
#include <pcre2.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef PCRE2_UCHAR RGX_CharacterType;
typedef PCRE2_SIZE RGX_OffsetType;
typedef uint32_t RGX_OptionsType;
typedef pcre2_code RGX_CodeType;
typedef pcre2_match_data RGX_DataType;

#define RGX_NO_MATCH PCRE2_ERROR_NOMATCH

static inline RGX_CodeType *
rgxCompile (
  const RGX_CharacterType *characters, size_t length,
  RGX_OptionsType options, RGX_OffsetType *offset,
  int *error
) {
  return pcre2_compile(
    characters, length, options, error, offset, NULL
  );
}

static inline void
rgxDeallocateCode (RGX_CodeType *code) {
  pcre2_code_free(code);
}

static inline RGX_DataType *
rgxAllocateData (RGX_CodeType *code) {
  return pcre2_match_data_create_from_pattern(code, NULL);
}

static inline void
rgxDeallocateData (RGX_DataType *data) {
  pcre2_match_data_free(data);
}

static inline int
rgxMatch (
  const RGX_CharacterType *characters, size_t length,
  RGX_CodeType *code, RGX_DataType *data,
  RGX_OptionsType options, size_t *count, int *error
) {
  int result = pcre2_match(
    code, characters, length, 0, options, data, NULL
  );

  if (result > 0) {
    *count = result - 1;
    return 1;
  } else {
    *error = result;
    return 0;
  }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_RGX_PCRE2 */
