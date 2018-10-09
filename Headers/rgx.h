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

#ifndef BRLTTY_INCLUDED_RGX
#define BRLTTY_INCLUDED_RGX

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct RegularExpressionObjectStruct RegularExpressionObject;

extern RegularExpressionObject *newRegularExpressionObject (void *data);
extern void destroyRegularExpressionObject (RegularExpressionObject *rgx);

typedef struct {
  struct {
    const wchar_t *characters;
    void *internal;
    size_t length;
  } string;

  struct {
    const wchar_t *characters;
    size_t length;
  } expression;

  struct {
    void *internal;
    size_t count;
  } matches;

  struct {
    void *object;
    void *pattern;
    void *match;
  } data;
} RegularExpressionHandlerParameters;

#define REGULAR_EXPRESSION_HANDLER(name) void name (const RegularExpressionHandlerParameters *parameters)
typedef REGULAR_EXPRESSION_HANDLER(RegularExpressionHandler);

extern int addRegularExpressionCharacters (
  RegularExpressionObject *rgx,
  const wchar_t *characters, size_t length,
  RegularExpressionHandler *handler, void *data
);

extern int
addRegularExpressionString (
  RegularExpressionObject *rgx,
  const wchar_t *string,
  RegularExpressionHandler *handler, void *data
);

extern int matchRegularExpressionsCharacters (
  RegularExpressionObject *rgx,
  const wchar_t *characters,
  size_t length,
  void *data
);

extern int
matchRegularExpressionsString (
  RegularExpressionObject *rgx,
  const wchar_t *string,
  void *data
);

extern unsigned int
getRegularExpressionMatchCount (
  const RegularExpressionHandlerParameters *parameters
);

extern int getRegularExpressionMatch (
  const RegularExpressionHandlerParameters *parameters,
  unsigned int index, int *start, int *end
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_RGX */
