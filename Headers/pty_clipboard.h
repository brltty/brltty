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

#ifndef BRLTTY_INCLUDED_PTY_CLIPBOARD
#define BRLTTY_INCLUDED_PTY_CLIPBOARD

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Put UTF-8 text onto the host's system clipboard (used to honor OSC 52 from a
 * program running inside the terminal). Returns non-zero on success. Does
 * nothing and returns zero where no integration is available. This must not
 * fork: brltty-pty's single select() loop and its CoreFoundation linkage make
 * spawning a subprocess (e.g. pbcopy) from the I/O path unsafe on macOS. */
extern int ptySetSystemClipboard (const char *bytes, size_t length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PTY_CLIPBOARD */
