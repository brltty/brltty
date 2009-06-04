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

static inline int
sameKeyCodes (KeyCode code1, KeyCode code2) {
  return (code1.set == code2.set) && (code1.key == code2.key);
}

static inline const void *
getKeyTableItem (KeyTable *table, KeyTableOffset offset) {
  return &table->header.bytes[offset];
}

static int
getCommand (KeyTable *table, KeyCode code) {
  const KeyTableHeader *header = table->header.fields;
  const KeyBinding *binding = getKeyTableItem(table, header->bindingsTable);
  unsigned int count = header->bindingsCount;

  while (count) {
    if (sameKeyCodes(code, binding->key.code) &&
        sameKeys(table->keys.mask, binding->key.modifiers))
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
    if (isKeySubset(binding->key.modifiers, table->keys.mask)) return 1;
    binding += 1, count -= 1;
  }

  return 0;
}

void
resetKeyTable (KeyTable *table) {
  removeAllKeys(&table->keys);
  table->command = EOF;
}

KeyCodesState
processKeyEvent (KeyTable *table, KeyCode code, int press) {
  KeyCodesState state = KCS_UNBOUND;

  if (code.set) {
    if (press) {
      unsigned char key = code.key;
      int command;

      code.key = 0;
      if ((command = getCommand(table, code)) != EOF) {
        command += key;
      } else {
        command = BRL_CMD_NOOP;
      }

      enqueueCommand(command);
      table->command = EOF;
      state = KCS_COMMAND;
    }

    return state;
  }

  removeKey(&table->keys, code.key);

  if (press) {
    int command = getCommand(table, code);

    addKey(&table->keys, code.key);

    if (command == EOF) {
      if (isModifiers(table)) state = KCS_MODIFIERS;
      goto release;
    }

    if (command != table->command) {
      table->command = command;
      enqueueCommand(command | BRL_FLG_REPEAT_INITIAL | BRL_FLG_REPEAT_DELAY);
    }

    state = KCS_COMMAND;
  } else {
  release:
    if (table->command != EOF) {
      table->command = EOF;
      enqueueCommand(BRL_CMD_NOOP);
    }
  }

  return state;
}
