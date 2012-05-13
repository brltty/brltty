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

#define FUNCTION_DECLARATION(name,returns,arguments) returns name arguments
#define FUNCTION_TYPE(name) name##_t
#define FUNCTION_TYPEDEF(name,returns,arguments) typedef FUNCTION_DECLARATION(FUNCTION_TYPE(name), returns, arguments)
#define FUNCTION_DECLARE(name,returns,arguments) FUNCTION_TYPEDEF(name, returns, arguments); extern FUNCTION_TYPE(name) name

FUNCTION_DECLARE(brlttyConstruct, ProgramExitStatus, (int argc, char *argv[]));
FUNCTION_DECLARE(brlttyUpdate, int, (void));
FUNCTION_DECLARE(brlttyDestruct, void, (void));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_EMBED */
