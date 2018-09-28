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

#ifndef BRLTTY_INCLUDED_REGEXOBJ
#define BRLTTY_INCLUDED_REGEXOBJ

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct RegularExpressionObjectStruct RegularExpressionObject;

extern RegularExpressionObject *newRegularExpressionObject (void *data);
extern void destroyRegularExpressionObject (RegularExpressionObject *regex);

typedef struct {
  void *objectData;
  void *patternData;
  void *matchData;
} RegularExpressionHandlerParameters;

#define REGULAR_EXPRESSION_HANDLER(name) void name (const RegularExpressionHandlerParameters *parameters);
typedef REGULAR_EXPRESSION_HANDLER(RegularExpressionHandler);

extern int addRegularExpression (
  RegularExpressionObject *regex,
  const char *expression,
  size_t submatches,
  RegularExpressionHandler *handler,
  void *data
);

extern int matchRegularExpressions (
  RegularExpressionObject *regex,
  const char *string,
  void *data
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_REGEXOBJ */
