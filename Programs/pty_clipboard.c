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

#include "prologue.h"

#include <stdio.h>
#include <stdlib.h>

#include "pty_clipboard.h"

/* An optional override: write the clipboard text to this file instead of the
 * platform clipboard. It is fork-free (plain fopen), so it's safe from the I/O
 * loop; it lets non-macOS users wire up their own clipboard (e.g. a fifo read
 * by xclip/wl-copy) and lets the test suite verify OSC 52 without a pasteboard. */
static int
writeClipboardToFile (const char *bytes, size_t length) {
  const char *path = getenv("BRLTTY_PTY_CLIPBOARD_FILE");
  if (!(path && *path)) return 0;

  FILE *file = fopen(path, "w");
  if (!file) return 0;

  fwrite(bytes, 1, length, file);
  fclose(file);
  return 1;
}

#ifdef __APPLE__
#import <AppKit/AppKit.h>

/* Set the macOS system pasteboard directly (no fork). Safe to call from
 * brltty-pty's I/O loop, unlike spawning pbcopy. */
int
ptySetSystemClipboard (const char *bytes, size_t length) {
  if (writeClipboardToFile(bytes, length)) return 1;

  @autoreleasepool {
    NSString *string = [[NSString alloc] initWithBytes:bytes
                                                length:length
                                              encoding:NSUTF8StringEncoding];
    if (!string) return 0;

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    return [pasteboard setString:string forType:NSPasteboardTypeString]? 1: 0;
  }
}

#else /* __APPLE__ */

int
ptySetSystemClipboard (const char *bytes, size_t length) {
  /* No native clipboard on this platform; honor the file override if set. */
  return writeClipboardToFile(bytes, length);
}

#endif /* __APPLE__ */
