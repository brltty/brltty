/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_KEYMAP
#define BRLTTY_INCLUDED_KEYMAP

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct KeymapStruct Keymap;

extern Keymap *keymap;

extern Keymap *compileKeymap (const char *name);
extern void destroyKeymap (Keymap *map);

extern char *ensureKeymapExtension (const char *path);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KEYMAP */
