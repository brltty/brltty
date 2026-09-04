/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SYSTEM
#define BRLTTY_INCLUDED_SYSTEM

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void initializeSystemObject (void);

/* Tell the OS that the user just did something, so that its idle/display
 * sleep timers get reset the same way they would for a real keypress or
 * mouse move. Braille input never goes through the OS's normal HID input
 * path (BRLTTY talks to the display directly over USB or Bluetooth), so
 * without this, the OS has no way to know the user is active and can sleep
 * the machine mid-session. Call this on every braille key command that
 * reaches the core, not just on some of them - it's meant to be called
 * that often (see notifyUserActivity() in system_darwin.c). A no-op on
 * platforms where the OS already sees this activity some other way, or
 * where nothing has been implemented for this yet. */
extern void notifyUserActivity (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SYSTEM */
