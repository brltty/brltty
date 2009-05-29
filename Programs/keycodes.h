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

#ifndef BRLTTY_INCLUDED_KEYCODES
#define BRLTTY_INCLUDED_KEYCODES

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "keydefs.h"
#include "bitmask.h"

#define KEY_CODE_MASK_ELEMENT_TYPE char
typedef BITMASK(KeyCodeMask, KEY_CODE_COUNT, KEY_CODE_MASK_ELEMENT_TYPE);
#define KEY_CODE_MASK_SIZE sizeof(KeyCodeMask)
#define KEY_CODE_MASK_ELEMENT_COUNT (KEY_CODE_MASK_SIZE / sizeof(KEY_CODE_MASK_ELEMENT_TYPE))

extern void copyKeyCodeMask (KeyCodeMask to, const KeyCodeMask from);
extern int sameKeyCodeMasks (const KeyCodeMask mask1, const KeyCodeMask mask2);
extern int isKeySubset (const KeyCodeMask set, const KeyCodeMask subset);

typedef struct {
  KeyCodeMask mask;
  unsigned int count;
  KeyCode codes[KEY_CODE_COUNT];
} KeyCodeSet;

extern int addKeyCode (KeyCodeSet *set, KeyCode code);
extern int removeKeyCode (KeyCodeSet *set, KeyCode code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KEYCODES */
