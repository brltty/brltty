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

#ifndef BRLTTY_INCLUDED_CLIPBOARD
#define BRLTTY_INCLUDED_CLIPBOARD

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const wchar_t *cpbGetContent (size_t *length);
extern int cpbSetContent (const wchar_t *characters, size_t length);
extern int cpbAddContent (const wchar_t *characters, size_t length);
extern void cpbClearContent (void);
extern void cpbTruncateContent (size_t length);

extern void cpbBeginOperation (int column, int row);
extern int cpbRectangularCopy (int column, int row);
extern int cpbLinearCopy (int column, int row);
extern int cpbPaste (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CLIPBOARD */
