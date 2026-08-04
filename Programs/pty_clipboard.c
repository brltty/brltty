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
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "log.h"
#include "pty_clipboard.h"

/* Optional file override: instead of the host clipboard, write to a file. It is
 * fork-free (plain stdio), so it is safe from the I/O loop. It lets non-macOS
 * users wire up their own clipboard (e.g. a fifo paired with xclip/wl-copy), and
 * lets the test suite intercept what would be published without a real terminal
 * or pasteboard. It takes precedence over the configured mode below. */
#define CLIPBOARD_WRITE_FILE_VARIABLE "BRLTTY_PTY_CLIPBOARD_FILE"

/* Override for the auto-detected publish mode (auto|passthrough|native). */
#define CLIPBOARD_MODE_VARIABLE "BRLTTY_PTY_CLIPBOARD_MODE"

static int
writeClipboardToFile (const char *bytes, size_t length) {
  const char *path = getenv(CLIPBOARD_WRITE_FILE_VARIABLE);
  if (!(path && *path)) return 0;

  FILE *file = fopen(path, "w");
  if (!file) return 0;

  fwrite(bytes, 1, length, file);
  fclose(file);
  return 1;
}

#ifdef __APPLE__
#import <AppKit/AppKit.h>

static int
setNativeClipboard (const char *bytes, size_t length) {
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

static int nativeClipboardAvailable (void) { return 1; }

#else /* __APPLE__ - no native backend on this platform yet */

static int setNativeClipboard (const char *bytes, size_t length) { return 0; }
static int nativeClipboardAvailable (void) { return 0; }

#endif /* __APPLE__ */

/* Passthrough: re-emit the clipboard as an OSC 52 sequence to the outer terminal
 * (our stdout), so that terminal publishes it to the host - exactly what tmux's
 * "set-clipboard on" does. This is cross-platform and lets the terminal own the
 * clipboard (and its own security prompts). OSC 52 is an invisible control
 * string, so writing it between curses screen updates does not disturb the
 * display. */
static const char base64Alphabet[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *
encodeBase64 (const unsigned char *data, size_t length, size_t *encodedLength) {
  size_t outLength = ((length + 2) / 3) * 4;
  char *out = malloc(outLength + 1);
  if (!out) return NULL;

  size_t in = 0;
  size_t off = 0;

  while ((length - in) >= 3) {
    unsigned int triple = (data[in] << 16) | (data[in+1] << 8) | data[in+2];
    out[off++] = base64Alphabet[(triple >> 18) & 0X3F];
    out[off++] = base64Alphabet[(triple >> 12) & 0X3F];
    out[off++] = base64Alphabet[(triple >> 6) & 0X3F];
    out[off++] = base64Alphabet[triple & 0X3F];
    in += 3;
  }

  size_t remaining = length - in;
  if (remaining == 1) {
    unsigned int triple = data[in] << 16;
    out[off++] = base64Alphabet[(triple >> 18) & 0X3F];
    out[off++] = base64Alphabet[(triple >> 12) & 0X3F];
    out[off++] = '=';
    out[off++] = '=';
  } else if (remaining == 2) {
    unsigned int triple = (data[in] << 16) | (data[in+1] << 8);
    out[off++] = base64Alphabet[(triple >> 18) & 0X3F];
    out[off++] = base64Alphabet[(triple >> 12) & 0X3F];
    out[off++] = base64Alphabet[(triple >> 6) & 0X3F];
    out[off++] = '=';
  }

  out[off] = 0;
  if (encodedLength) *encodedLength = off;
  return out;
}

static int
writeAll (int fd, const char *data, size_t length) {
  size_t total = 0;

  while (total < length) {
    ssize_t count = write(fd, data + total, length - total);

    if (count < 0) {
      if (errno == EINTR) continue;
      return 0;
    }

    total += count;
  }

  return 1;
}

static int
emitClipboardPassthrough (const char *bytes, size_t length) {
  size_t encodedLength;
  char *encoded = encodeBase64((const unsigned char *)bytes, length, &encodedLength);
  if (!encoded) return 0;

  static const char prefix[] = "\033]52;c;";
  static const char suffix[] = "\a"; /* BEL string terminator */

  int ok = writeAll(STDOUT_FILENO, prefix, sizeof(prefix) - 1)
        && writeAll(STDOUT_FILENO, encoded, encodedLength)
        && writeAll(STDOUT_FILENO, suffix, sizeof(suffix) - 1);

  free(encoded);
  return ok;
}

typedef enum {
  CLIPBOARD_MODE_PASSTHROUGH,
  CLIPBOARD_MODE_NATIVE,
} ClipboardMode;

static int clipboardModeResolved = 0;
static ClipboardMode clipboardMode = CLIPBOARD_MODE_PASSTHROUGH;

/* Over ssh there is no local pasteboard we can usefully write: any native
 * clipboard we could reach would be the *remote* machine's, not the user's. So
 * when running remotely the only way to the user's clipboard is to hand an OSC 52
 * to the terminal at the near end (passthrough). */
static int
runningRemotely (void) {
  return (getenv("SSH_CONNECTION") != NULL) || (getenv("SSH_TTY") != NULL);
}

static ClipboardMode
resolveClipboardMode (void) {
  const char *requested = getenv(CLIPBOARD_MODE_VARIABLE);

  if (requested && *requested) {
    if (strcmp(requested, "passthrough") == 0) return CLIPBOARD_MODE_PASSTHROUGH;
    if (strcmp(requested, "native") == 0) return CLIPBOARD_MODE_NATIVE;
    /* "auto" or anything unrecognized falls through to detection. */
  }

  /* Locally, writing the pasteboard directly is the most reliable path: it
   * reaches the user's clipboard regardless of which terminal is in use, and -
   * unlike OSC 52 - it works even under screen/tmux, which can swallow the
   * sequence. We only hand the terminal an OSC 52 when we cannot reach a local
   * pasteboard: over ssh, or where there is no native backend at all. */
  if (nativeClipboardAvailable() && !runningRemotely()) return CLIPBOARD_MODE_NATIVE;
  return CLIPBOARD_MODE_PASSTHROUGH;
}

void
ptyConfigureClipboard (void) {
  clipboardMode = resolveClipboardMode();
  clipboardModeResolved = 1;

  logMessage(LOG_DEBUG, "clipboard publish mode: %s",
             (clipboardMode == CLIPBOARD_MODE_NATIVE)? "native": "passthrough");
}

int
ptyPublishClipboard (const char *bytes, size_t length) {
  /* The file override always wins: it is the test / DIY-wiring intercept. */
  if (writeClipboardToFile(bytes, length)) return 1;

  if (!clipboardModeResolved) ptyConfigureClipboard();

  if (clipboardMode == CLIPBOARD_MODE_NATIVE) {
    if (setNativeClipboard(bytes, length)) return 1;

    /* The pasteboard was unreachable (e.g. no GUI session) or the text wasn't
     * valid UTF-8. Don't lose the copy: fall back to handing it to the terminal. */
    logMessage(LOG_WARNING, "native clipboard write failed; using passthrough");
    return emitClipboardPassthrough(bytes, length);
  }

  if (emitClipboardPassthrough(bytes, length)) return 1;
  logMessage(LOG_WARNING, "clipboard passthrough write failed");
  return 0;
}
