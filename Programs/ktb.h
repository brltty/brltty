/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_KTB
#define BRLTTY_INCLUDED_KTB

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "keydefs.h"
#include "keysets.h"

extern KeyTable *compileKeyTable (const char *name, const KeyNameEntry *const *keys);
extern void destroyKeyTable (KeyTable *table);

extern char *ensureKeyTableExtension (const char *path);

extern void resetKeyTable (KeyTable *table);
extern KeyTableState processKeyEvent (KeyTable *table, unsigned char context, unsigned char set, unsigned char key, int press);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KTB */
