/*
 * brltty - macOS Accessibility screen driver: pure C bridge to the AX API.
 *
 * Kept in a separate header so the implementation (ax_bridge.m) can include
 * <ApplicationServices/ApplicationServices.h> — whose Quickdraw RGBColor
 * typedef collides with brltty's color_types.h — without leaking that
 * inclusion into screen.m.
 */

#ifndef BRLTTY_AX_BRIDGE_H
#define BRLTTY_AX_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// Request macOS Accessibility permission (with system prompt). Returns 1 if
// already trusted, 0 if the prompt was shown / permission is pending.
int ax_request_trust(void);

// Start / stop a background CFRunLoop thread that owns an AXObserver and
// raises a dirty flag whenever macOS pushes us a focus/value/selection
// notification. ax_observer_start returns a file descriptor that brltty's
// main thread should monitor for readable input: the observer writes one
// byte to it on every event, which is the thread-safe way to wake up the
// brltty async loop from a different thread.
int  ax_observer_start(void);
void ax_observer_stop(void);

// Drain any pending wake-up bytes from the observer self-pipe (called by
// brltty's async file-input callback before mainScreenUpdated()).
void ax_observer_drain(int fd);

// Returns 1 (and clears the flag) if the observer thread saw a change since
// the last call, 0 otherwise.
int ax_consume_dirty(void);

// Inject a key into the macOS event stream, mirroring brltty's ScreenKey
// encoding: bits 0..19 carry either a Unicode codepoint or a special key
// code (>= 0xF800), and bits 24..30 carry modifier flags (SCR_KEY_SHIFT,
// SCR_KEY_CONTROL, SCR_KEY_ALT_LEFT, SCR_KEY_ALT_RIGHT, SCR_KEY_GUI,
// SCR_KEY_CAPSLOCK, SCR_KEY_UPPER). Returns 1 on success, 0 otherwise.
int ax_post_key(uint32_t key);

// Post a synthetic keyboard shortcut: virtual keycode + modifier mask.
// Used for app-level commands (tab switching, etc.) where we want a real
// shortcut interpretation rather than text injection. Modifier mask uses
// the same flags as the brltty BR_* set; see ax_post_key.
//   AX_SHORTCUT_TAB_NEXT, AX_SHORTCUT_TAB_PREV: Cmd+Shift+] / Cmd+Shift+[
//   AX_SHORTCUT_TAB_INDEX_1..9: Cmd+1..Cmd+9
int ax_post_shortcut_tab_next(void);
int ax_post_shortcut_tab_prev(void);
int ax_post_shortcut_tab_index(int n);  // n in 1..9

// Compute a cheap fingerprint of the frontmost window's accessibility state.
// Written into out_buf (length out_len). Returns the byte count written or
// 0 on error. The fingerprint is opaque; callers only compare equality.
size_t ax_fingerprint(char *out_buf, size_t out_len);

// Snapshot of the frontmost application's accessibility view, rendered as a
// sequence of \n-separated lines into out_buf (length out_len).
// Returns the byte count written (NUL terminator not included), 0 on error.
// If non-null, *out_row and *out_col receive the cursor position within the
// rendered text (0-based). The values are clamped to the content bounds and
// default to (0,0) when no caret can be derived.
size_t ax_snapshot_lines(char *out_buf, size_t out_len,
                         int *out_row, int *out_col);

#ifdef __cplusplus
}
#endif

#endif
