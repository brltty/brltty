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

/* Resolve, once, how the terminal's clipboard is published to the host. Two
 * modes:
 *   - passthrough: re-emit OSC 52 to the outer terminal, which owns the host
 *     clipboard (cross-platform; the same thing tmux's "set-clipboard on" does).
 *   - native: write the host clipboard directly (macOS NSPasteboard).
 * The default is auto: passthrough, except on terminals known to ignore OSC 52
 * (macOS Terminal.app), where it falls back to native so copy still works. The
 * BRLTTY_PTY_CLIPBOARD_MODE environment variable (auto|passthrough|native)
 * overrides the detection. Safe to call more than once; resolved lazily on first
 * publish if never called. */
extern void ptyConfigureClipboard (void);

/* Publish UTF-8 text as the terminal's clipboard, honoring the configured mode
 * (used for OSC 52 from a program inside the terminal, and for BRLTTY's own
 * clipboard). Returns non-zero on success. Must not fork: brltty-pty's single
 * select() loop and its CoreFoundation linkage make spawning a subprocess (e.g.
 * pbcopy) from the I/O path unsafe on macOS. */
extern int ptyPublishClipboard (const char *bytes, size_t length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PTY_CLIPBOARD */
