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

#include "prologue.h"

#include <string.h>

#include "misc.h"
#include "ktb.h"
#include "ktb_internal.h"

KeyTable *keyTable = NULL;

static inline const void *
getKeyTableItem (KeyTable *table, KeyTableOffset offset) {
  return &table->header.bytes[offset];
}

const KeyBinding *
getKeyBinding (KeyTable *table, KeyCodeMask modifiers, KeyCode code) {
  const KeyTableHeader *header = table->header.fields;
  const KeyBinding *binding = getKeyTableItem(table, header->bindingsTable);
  unsigned int count = header->bindingsCount;

  while (count) {
    if ((code == binding->key.code) &&
        (memcmp(modifiers, binding->key.modifiers, sizeof(binding->key.modifiers)) == 0))
      return binding;
    binding += 1, count -= 1;
  }

  return NULL;
}

int
isKeySubset (const KeyCodeMask set, const KeyCodeMask subset) {
  unsigned int count = sizeof(KeyCodeMask) / sizeof(*set);

  while (count) {
    if (~*set & *subset) return 0;
    set += 1, subset += 1, count -= 1;
  }

  return 1;
}

int
isKeyModifiers (KeyTable *table, KeyCodeMask modifiers) {
  const KeyTableHeader *header = table->header.fields;
  const KeyBinding *binding = getKeyTableItem(table, header->bindingsTable);
  unsigned int count = header->bindingsCount;

  while (count) {
    if (isKeySubset(binding->key.modifiers, modifiers)) return 1;
    binding += 1, count -= 1;
  }

  return 0;
}
