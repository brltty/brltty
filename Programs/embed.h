/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include "brltty.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

FUNCTION_DECLARE(brlttyConstruct, ProgramExitStatus, (int argc, char *argv[]));
FUNCTION_DECLARE(brlttyDestruct, int, (void));

FUNCTION_DECLARE(brlttyWait, int, (int duration));
FUNCTION_DECLARE(brlttyInterrupt, int, (void));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_EMBED */
