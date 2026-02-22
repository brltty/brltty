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

#ifndef BRLTTY_INCLUDED_MATCH
#define BRLTTY_INCLUDED_MATCH

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Set *matchOffset and *matchLength to the wchar_t position and length of
 * the URL/email/hostname at position `target` within buffer `buf` of `len`
 * chars.  Returns 1 if found, 0 otherwise. */
extern int matchSmart (
  const wchar_t *buf, int len, int target,
  int *matchOffset, int *matchLength
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_MATCH */
