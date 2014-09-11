/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_UPDATE
#define BRLTTY_INCLUDED_UPDATE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void scheduleUpdate (const char *reason);
extern void scheduleUpdateIn (const char *reason, int delay);

extern void beginUpdates (void);
extern void suspendUpdates (void);
extern void resumeUpdates (int refresh);

#ifdef ENABLE_SPEECH_SUPPORT
extern void doAutospeak (int force);
#endif /* ENABLE_SPEECH_SUPPORT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_UPDATE */
