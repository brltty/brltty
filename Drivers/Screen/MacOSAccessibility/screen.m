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
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>

#include "log.h"
#include "scr_driver.h"
#include "async_handle.h"
#include "async_io.h"
#include "ax_bridge.h"

// Hard upper bounds to keep buffer sizes sane on rogue input.
#define SCREEN_MAX_COLUMNS 512
#define SCREEN_MAX_ROWS    256

// Minimum dimensions reported even when we have nothing to display, so
// brltty has a non-empty screen to draw to.
#define SCREEN_MIN_COLUMNS 40
#define SCREEN_MIN_ROWS    1

static wchar_t *screenBuffer = NULL;
static int screenRows = SCREEN_MIN_ROWS;
static int screenCols = SCREEN_MIN_COLUMNS;
static int screenCapacity = 0;

static int cursorRow = 0;
static int cursorCol = 0;

static char lastFingerprint[2048] = {0};
static AsyncHandle wakeMonitor = NULL;
static int wakeFd = -1;

static int
handleWakeFromObserver(const AsyncMonitorCallbackParameters *parameters) {
  ax_observer_drain(wakeFd);
  mainScreenUpdated();
  return 1; // keep monitoring
}

static void
ensureBufferCapacity(int rows, int cols) {
  int need = rows * cols;
  if (need <= screenCapacity) return;

  wchar_t *newBuffer = realloc(screenBuffer, need * sizeof(wchar_t));
  if (!newBuffer) {
    logMessage(LOG_WARNING, "mo: failed to grow screen buffer to %d cells", need);
    return;
  }
  screenBuffer = newBuffer;
  screenCapacity = need;
}

static void
fillScreenWithSpaces(void) {
  for (int i = 0; i < screenRows * screenCols; i += 1) {
    screenBuffer[i] = L' ';
  }
}

// Decode one UTF-8 sequence starting at *p (must point inside a buffer of at
// least `remaining` bytes). Returns the number of bytes consumed and writes
// the codepoint into *out. Invalid sequences yield U+FFFD and consume one
// byte so we always make progress.
static int
decodeUtf8(const unsigned char *p, size_t remaining, uint32_t *out) {
  if (remaining == 0) { *out = 0; return 0; }
  unsigned char c = p[0];
  if (c < 0x80) { *out = c; return 1; }
  if ((c & 0xE0) == 0xC0 && remaining >= 2
      && (p[1] & 0xC0) == 0x80) {
    *out = ((c & 0x1F) << 6) | (p[1] & 0x3F);
    return 2;
  }
  if ((c & 0xF0) == 0xE0 && remaining >= 3
      && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
    *out = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    return 3;
  }
  if ((c & 0xF8) == 0xF0 && remaining >= 4
      && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80
      && (p[3] & 0xC0) == 0x80) {
    *out = ((c & 0x07) << 18) | ((p[1] & 0x3F) << 12)
         | ((p[2] & 0x3F) << 6) |  (p[3] & 0x3F);
    return 4;
  }
  *out = 0xFFFD;
  return 1;
}

// First pass: scan the source text (UTF-8) to discover the actual grid
// geometry it needs (longest line in codepoints, line count). Returns rows
// and cols clamped to maxima.
static void
measureText(const char *text, int *outRows, int *outCols) {
  int rows = 1;
  int cols = 0;
  int curCol = 0;
  if (!text) { *outRows = SCREEN_MIN_ROWS; *outCols = SCREEN_MIN_COLUMNS; return; }

  size_t len = strlen(text);
  const unsigned char *p = (const unsigned char *)text;
  size_t i = 0;
  while (i < len) {
    uint32_t cp;
    int n = decodeUtf8(p + i, len - i, &cp);
    if (n <= 0) break;
    i += n;
    if (cp == '\n' || cp == '\r') {
      if (curCol > cols) cols = curCol;
      rows += 1;
      curCol = 0;
    } else {
      curCol += 1;
    }
  }
  if (curCol > cols) cols = curCol;

  if (cols < SCREEN_MIN_COLUMNS) cols = SCREEN_MIN_COLUMNS;
  if (rows < SCREEN_MIN_ROWS) rows = SCREEN_MIN_ROWS;
  if (cols > SCREEN_MAX_COLUMNS) cols = SCREEN_MAX_COLUMNS;
  if (rows > SCREEN_MAX_ROWS) rows = SCREEN_MAX_ROWS;

  *outRows = rows;
  *outCols = cols;
}

// Second pass: paint the source text (UTF-8) into the (already sized)
// buffer as Unicode codepoints, so brltty's text table can translate
// accented characters correctly.
static void
renderTextIntoGrid(const char *text) {
  fillScreenWithSpaces();
  if (!text || !*text) return;

  int row = 0;
  int col = 0;
  size_t len = strlen(text);
  const unsigned char *p = (const unsigned char *)text;
  size_t i = 0;
  while (i < len && row < screenRows) {
    uint32_t cp;
    int n = decodeUtf8(p + i, len - i, &cp);
    if (n <= 0) break;
    i += n;

    if (cp == '\n' || cp == '\r') {
      row += 1;
      col = 0;
      continue;
    }
    if (cp == '\t') cp = ' ';
    // Replace non-printable ASCII controls; keep all other codepoints (they
    // map to printable Unicode that the text table handles).
    if (cp < 0x20 || cp == 0x7f) cp = '?';

    if (col >= screenCols) {
      row += 1;
      col = 0;
      if (row >= screenRows) break;
    }
    screenBuffer[row * screenCols + col] = (wchar_t)cp;
    col += 1;
  }
}

static int
construct_MacOSAccessibilityScreen(void) {
  logMessage(LOG_DEBUG, "mo: construct: entered");
  int trusted = ax_request_trust();
  logMessage(LOG_DEBUG, "mo: construct: ax_request_trust=%d", trusted);
  if (!trusted) {
    logMessage(LOG_WARNING,
      "macOS Accessibility permission not granted. "
      "Approve brltty in System Settings -> Privacy & Security -> Accessibility, "
      "then restart brltty.");
  }
  cursorRow = 0;
  cursorCol = 0;
  lastFingerprint[0] = '\0';

  screenRows = SCREEN_MIN_ROWS;
  screenCols = SCREEN_MIN_COLUMNS;
  ensureBufferCapacity(screenRows, screenCols);
  if (screenBuffer) fillScreenWithSpaces();
  logMessage(LOG_DEBUG, "mo: construct: buffer ready");

  const char *initial = trusted
    ? "brltty: macOS Accessibility ready"
    : "brltty: waiting for Accessibility permission";
  renderTextIntoGrid(initial);
  logMessage(LOG_DEBUG, "mo: construct: rendered initial");

  wakeFd = ax_observer_start();
  logMessage(LOG_DEBUG, "mo: construct: ax_observer_start=%d", wakeFd);
  if (wakeFd >= 0) {
    if (!asyncMonitorFileInput(&wakeMonitor, wakeFd, handleWakeFromObserver, NULL)) {
      logMessage(LOG_WARNING, "mo: failed to register wake monitor");
      wakeMonitor = NULL;
    }
  }
  logMessage(LOG_DEBUG, "mo: construct: returning success");
  return 1;
}

static void
destruct_MacOSAccessibilityScreen(void) {
  if (wakeMonitor) {
    asyncCancelRequest(wakeMonitor);
    wakeMonitor = NULL;
  }
  ax_observer_stop();
  wakeFd = -1;
  free(screenBuffer);
  screenBuffer = NULL;
  screenCapacity = 0;
}

/* When Terminal.app fires AXSelectedTextChanged *before* the new prompt
 * is published in AXStringForRange (after Enter), we capture a transient
 * state where the cursor sits on an empty row beyond column 0. This flag
 * tells poll() to force one more refresh shortly after, up to a few
 * retries — cheap, targeted, and self-clearing when the state settles. */
static int pendingResettleRefreshes = 0;
#define AX_MAX_RESETTLE_RETRIES 3

static int
isCursorOnEmptyRow(void) {
  if (!screenBuffer || cursorRow < 0 || cursorRow >= screenRows) return 0;
  /* Only treat it as suspicious if cursorCol > 0 — a true empty row with
   * the cursor at column 0 is a legitimate "blank line" state (e.g. just
   * after pressing Enter on a blank prompt). The bad state is "cursor is
   * past column 0 but the row up to that point is whitespace". */
  if (cursorCol <= 0) return 0;
  int limit = cursorCol < screenCols ? cursorCol : screenCols;
  for (int c = 0; c < limit; c += 1) {
    wchar_t ch = screenBuffer[cursorRow * screenCols + c];
    if (ch != L' ' && ch != 0) return 0;
  }
  return 1;
}

static int
poll_MacOSAccessibilityScreen(void) {
  // The AXObserver thread sets a dirty flag whenever macOS pushes us a
  // notification. We trust that signal first because brltty's own update
  // cadence is too slow for typing.
  if (ax_consume_dirty()) {
    logMessage(LOG_DEBUG, "mo: poll: dirty (from AX observer)");
    pendingResettleRefreshes = 0;
    return 1;
  }

  if (pendingResettleRefreshes > 0) {
    pendingResettleRefreshes -= 1;
    logMessage(LOG_DEBUG, "mo: poll: resettle retry (remaining=%d)", pendingResettleRefreshes);
    return 1;
  }

  // Belt-and-braces: also recompute the fingerprint here in case the
  // observer missed a change.
  char fp[2048];
  ax_fingerprint(fp, sizeof(fp));
  if (strcmp(fp, lastFingerprint) != 0) {
    logMessage(LOG_DEBUG, "mo: poll: fp changed");
    strncpy(lastFingerprint, fp, sizeof(lastFingerprint) - 1);
    lastFingerprint[sizeof(lastFingerprint) - 1] = '\0';
    return 1;
  }
  return 0;
}

static int
refresh_MacOSAccessibilityScreen(void) {
  static char buf[131072];  // 128 KB; AX visible ranges fit here easily.
  int row = 0;
  int col = 0;
  ax_snapshot_lines(buf, sizeof(buf), &row, &col);

  int newRows, newCols;
  measureText(buf, &newRows, &newCols);

  screenRows = newRows;
  screenCols = newCols;
  ensureBufferCapacity(newRows, newCols);
  if (!screenBuffer) return 0;

  renderTextIntoGrid(buf);

  if (row < 0) row = 0;
  if (row >= screenRows) row = screenRows - 1;
  if (col < 0) col = 0;
  if (col >= screenCols) col = screenCols - 1;
  cursorRow = row;
  cursorCol = col;

  /* If the cursor landed on an empty row at column > 0, the app probably
   * hasn't finished publishing the new line content. Arm a few re-polls
   * so the next SCREEN_UPDATE_POLL_INTERVAL tick re-snapshots. */
  if (isCursorOnEmptyRow()) {
    if (pendingResettleRefreshes == 0) {
      pendingResettleRefreshes = AX_MAX_RESETTLE_RETRIES;
      logMessage(LOG_DEBUG, "mo: cursor on empty row — arming resettle (row=%d col=%d)",
                 cursorRow, cursorCol);
    }
  }
  return 1;
}

static void
describe_MacOSAccessibilityScreen(ScreenDescription *desc) {
  desc->cols = screenCols;
  desc->rows = screenRows;
  desc->posx = cursorCol;
  desc->posy = cursorRow;
  desc->number = 1;
  desc->hasCursor = 1;
  desc->hasSelection = 0;
  desc->quality = SCQ_FAIR;
}

static int
readCharacters_MacOSAccessibilityScreen(const ScreenBox *box, ScreenCharacter *buffer) {
  if (!validateScreenBox(box, screenCols, screenRows)) return 0;
  if (!screenBuffer) return 0;
  for (int row = 0; row < box->height; row += 1) {
    for (int col = 0; col < box->width; col += 1) {
      ScreenCharacter *target = &buffer[(row * box->width) + col];
      target->text = screenBuffer[(box->top + row) * screenCols + (box->left + col)];
      target->color.vgaAttributes = 0x07;
      target->color.foreground = (RGBColor){255, 255, 255};
      target->color.background = (RGBColor){0, 0, 0};
    }
  }
  return 1;
}

/* macOS has no real virtual terminals; we surface frontmost-app tabs as
 * the brltty VT abstraction. We track our notion of "current tab" as an
 * opaque counter — relative motion (prev/next) translates to Ctrl+Tab
 * / Ctrl+Shift+Tab, indexed motion (1..9) to Cmd+N. */
static int virtualTerminal = 1;

static int
currentVirtualTerminal_MacOSAccessibilityScreen(void) {
  return virtualTerminal;
}

static int
switchVirtualTerminal_MacOSAccessibilityScreen(int vt) {
  if (vt < 1) return 0;

  if (vt == virtualTerminal + 1) {
    if (!ax_post_shortcut_tab_next()) return 0;
    virtualTerminal = vt;
    return 1;
  }
  if (vt == virtualTerminal - 1 && virtualTerminal > 1) {
    if (!ax_post_shortcut_tab_prev()) return 0;
    virtualTerminal = vt;
    return 1;
  }
  if (vt >= 1 && vt <= 9) {
    if (!ax_post_shortcut_tab_index(vt)) return 0;
    virtualTerminal = vt;
    return 1;
  }
  /* Out of range — no equivalent macOS shortcut for tab >= 10. */
  return 0;
}

static int
insertKey_MacOSAccessibilityScreen(ScreenKey key) {
  // brltty's ScreenKey is a uint32 the bridge already knows how to decode.
  if (!ax_post_key((uint32_t)key)) {
    logMessage(LOG_WARNING, "mo: failed to inject key 0x%08X", (unsigned)key);
    return 0;
  }
  return 1;
}

static void
scr_initialize(MainScreen *main) {
  initializeRealScreen(main);
  main->base.poll = poll_MacOSAccessibilityScreen;
  main->base.refresh = refresh_MacOSAccessibilityScreen;
  main->base.describe = describe_MacOSAccessibilityScreen;
  main->base.readCharacters = readCharacters_MacOSAccessibilityScreen;
  main->base.insertKey = insertKey_MacOSAccessibilityScreen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_MacOSAccessibilityScreen;
  main->base.switchVirtualTerminal = switchVirtualTerminal_MacOSAccessibilityScreen;
  main->construct = construct_MacOSAccessibilityScreen;
  main->destruct = destruct_MacOSAccessibilityScreen;
}
