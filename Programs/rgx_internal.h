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

#ifndef BRLTTY_INCLUDED_RGX_INTERNAL
#define BRLTTY_INCLUDED_RGX_INTERNAL

#if defined(USE_PKG_RGX_LIBPCRE2_32)
#define PCRE2_CODE_UNIT_WIDTH 32
#include <pcre2.h>
typedef PCRE2_UCHAR RGX_CharacterType;
typedef PCRE2_SIZE RGX_OffsetType;
typedef uint32_t RGX_OptionsType;
typedef pcre2_code RGX_CodeType;
typedef pcre2_match_data RGX_DataType;
#define RGX_NO_MATCH PCRE2_ERROR_NOMATCH

#elif defined(USE_PKG_RGX_NONE)
typedef wchar_t RGX_CharacterType;
typedef size_t RGX_OffsetType;
typedef int RGX_OptionsType;
typedef uint8_t RGX_CodeType;
typedef uint8_t RGX_DataType;
#define RGX_NO_MATCH 1

#endif /* regular expression package */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern RGX_CodeType *rgxCompile (
  const RGX_CharacterType *characters, size_t length,
  RGX_OptionsType options, RGX_OffsetType *offset,
  int *error
);

extern void rgxDeallocateCode (RGX_CodeType *code);
extern RGX_DataType *rgxAllocateData (RGX_CodeType *code);
extern void rgxDeallocateData (RGX_DataType *data);

extern int rgxMatch (
  const RGX_CharacterType *characters, size_t length,
  RGX_CodeType *code, RGX_DataType *data,
  RGX_OptionsType options, size_t *count, int *error
);

extern int rgxBounds (
  RGX_DataType *data, size_t index, size_t *from, size_t *to
);

#define RGX_DECLARE_OPTION_MAP(name) const RGX_OptionsType name##Map[]
#define RGX_DECLARE_OPTION_COUNT(name) const unsigned char name##Count

extern RGX_DECLARE_OPTION_MAP(rgxCompileOptions);
extern RGX_DECLARE_OPTION_COUNT(rgxCompileOptions);

extern RGX_DECLARE_OPTION_MAP(rgxMatchOptions);
extern RGX_DECLARE_OPTION_COUNT(rgxMatchOptions);

#define RGX_BEGIN_OPTIONS(name) RGX_DECLARE_OPTION_MAP(name) = {
#define RGX_END_OPTIONS(name) }; RGX_DECLARE_OPTION_COUNT(name) = ARRAY_COUNT(name##Map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_RGX_INTERNAL */
