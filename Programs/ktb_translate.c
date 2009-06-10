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

#include "prologue.h"

#include <string.h>

#include "misc.h"
#include "ktb.h"
#include "ktb_internal.h"
#include "cmd.h"
#include "brldefs.h"

static inline const void *
getKeyTableItem (KeyTable *table, KeyTableOffset offset) {
  return &table->header.bytes[offset];
}

static int
getCommand (KeyTable *table, unsigned char set, unsigned char key) {
  const KeyTableHeader *header = table->header.fields;
  const KeyBinding *binding = getKeyTableItem(table, header->bindingsTable);
  unsigned int count = header->bindingsCount;

  while (count) {
    if ((set == binding->keys.set) && (key == binding->keys.key) &&
        sameKeys(table->keys.mask, binding->keys.modifiers))
      return binding->command;

    binding += 1, count -= 1;
  }

  return EOF;
}

static int
isModifiers (KeyTable *table) {
  const KeyTableHeader *header = table->header.fields;
  const KeyBinding *binding = getKeyTableItem(table, header->bindingsTable);
  unsigned int count = header->bindingsCount;

  while (count) {
    if (isKeySubset(binding->keys.modifiers, table->keys.mask)) return 1;
    binding += 1, count -= 1;
  }

  return 0;
}

void
resetKeyTable (KeyTable *table) {
  removeAllKeys(&table->keys);
  table->command = EOF;
}

KeyTableState
processKeyEvent (KeyTable *table, unsigned char set, unsigned char key, int press) {
  KeyTableState state = KTS_UNBOUND;

  if (set) {
    if (press) {
      int command;

      if ((command = getCommand(table, set, 0)) != EOF) {
        command += key;
      } else {
        command = BRL_CMD_NOOP;
      }

      enqueueCommand(command);
      table->command = EOF;
      state = KTS_COMMAND;
    }

    return state;
  }

  removeKey(&table->keys, key);

  if (press) {
    int command = getCommand(table, 0, key);

    addKey(&table->keys, key);

    if (command == EOF) {
      if (isModifiers(table)) state = KTS_MODIFIERS;
      goto release;
    }

    if (command != table->command) {
      table->command = command;
      enqueueCommand(command | BRL_FLG_REPEAT_INITIAL | BRL_FLG_REPEAT_DELAY);
    }

    state = KTS_COMMAND;
  } else {
  release:
    if (table->command != EOF) {
      table->command = EOF;
      enqueueCommand(BRL_CMD_NOOP);
    }
  }

  return state;
}
