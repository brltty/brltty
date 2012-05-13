/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_EMBED
#define BRLTTY_INCLUDED_EMBED

#include "program.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FUNCTION_TYPE(functionName) functionName ## _t

#define FUNCTION_DECLARATION(functionName, returnType, argumentList) \
  returnType functionName argumentList

#define FUNCTION_TYPEDEF(functionName, returnType, argumentList) \
  typedef FUNCTION_DECLARATION(FUNCTION_TYPE(functionName), returnType, argumentList)

#define FUNCTION_DECLARE(functionName, returnType, argumentList) \
  FUNCTION_TYPEDEF(functionName, returnType, argumentList); \
  extern FUNCTION_TYPE(functionName) functionName

FUNCTION_DECLARE(brlttyConstruct, ProgramExitStatus, (int argc, char *argv[]));
FUNCTION_DECLARE(brlttyUpdate, int, (void));
FUNCTION_DECLARE(brlttyDestruct, void, (void));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_EMBED */
