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

#ifndef BRLTTY_INCLUDED_KEYSETS
#define BRLTTY_INCLUDED_KEYSETS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "keydefs.h"
#include "bitmask.h"

#define KEY_SET_MASK_ELEMENT_TYPE char
typedef BITMASK(KeySetMask, KEYS_PER_SET, KEY_SET_MASK_ELEMENT_TYPE);
#define KEY_SET_MASK_SIZE sizeof(KeySetMask)
#define KEY_SET_MASK_ELEMENT_COUNT (KEY_SET_MASK_SIZE / sizeof(KEY_SET_MASK_ELEMENT_TYPE))

extern void copyKeySetMask (KeySetMask to, const KeySetMask from);
extern int compareKeys (const KeySetMask mask1, const KeySetMask mask2);
extern int sameKeys (const KeySetMask mask1, const KeySetMask mask2);
extern int isKeySubset (const KeySetMask set, const KeySetMask subset);

typedef struct {
  KeySetMask mask;
  unsigned char count;
  unsigned char keys[KEYS_PER_SET];
} KeySet;

extern int addKey (KeySet *set, unsigned char key);
extern int removeKey (KeySet *set, unsigned char key);
extern void removeAllKeys (KeySet *set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KEYSETS */
