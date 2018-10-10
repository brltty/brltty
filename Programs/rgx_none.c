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

#include "rgx.h"
#include "rgx_internal.h"

RGX_BEGIN_OPTIONS(rgxCompileOptions)
RGX_END_OPTIONS(rgxCompileOptions)

RGX_BEGIN_OPTIONS(rgxMatchOptions)
RGX_END_OPTIONS(rgxMatchOptions)

RGX_CodeType *
rgxCompile (
  const RGX_CharacterType *characters, size_t length,
  RGX_OptionsType options, RGX_OffsetType *offset,
  int *error
) {
  return NULL;
}

void
rgxDeallocateCode (RGX_CodeType *code) {
}

RGX_DataType *
rgxAllocateData (RGX_CodeType *code) {
  return NULL;
}

void
rgxDeallocateData (RGX_DataType *data) {
}

int
rgxMatch (
  const RGX_CharacterType *characters, size_t length,
  RGX_CodeType *code, RGX_DataType *data,
  RGX_OptionsType options, size_t *count, int *error
) {
  *error = RGX_NO_MATCH;
  return 0;
}

int
rgxBounds (
  RGX_DataType *data, size_t index, size_t *from, size_t *to
) {
  return 0;
}
