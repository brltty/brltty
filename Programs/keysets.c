/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include "prologue.h"

#include <string.h>

#include "keysets.h"

void
copyKeySetMask (KeySetMask to, const KeySetMask from) {
  memcpy(to, from, KEY_SET_MASK_SIZE);
}

int
compareKeySetMasks (const KeySetMask mask1, const KeySetMask mask2) {
  return memcmp(mask1, mask2, KEY_SET_MASK_SIZE);
}

int
sameKeySetMasks (const KeySetMask mask1, const KeySetMask mask2) {
  return compareKeySetMasks(mask1, mask2) == 0;
}

int
isKeySetSubmask (const KeySetMask mask, const KeySetMask submask) {
  unsigned int count = KEY_SET_MASK_ELEMENT_COUNT;

  while (count) {
    if (~*mask++ & *submask++) return 0;
    count -= 1;
  }

  return 1;
}

int
testKey (const KeySet *set, unsigned char key) {
  return BITMASK_TEST(set->mask, key);
}

int
addKey (KeySet *set, unsigned char key) {
  if (BITMASK_TEST(set->mask, key)) return 0;
  BITMASK_SET(set->mask, key);
  set->keys[set->count++] = key;
  return 1;
}

int
removeKey (KeySet *set, unsigned char key) {
  if (!BITMASK_TEST(set->mask, key)) return 0;
  BITMASK_CLEAR(set->mask, key);

  {
    int index;
    for (index=0; index<set->count; index+=1) {
      if (set->keys[index] == key) {
        memmove(&set->keys[index], &set->keys[index+1],
                ((--set->count - index) * sizeof(set->keys[0])));
        break;
      }
    }
  }

  return 1;
}

void
removeAllKeys (KeySet *set) {
  set->count = 0;
  memset(set->mask, 0, sizeof(set->mask));
}
