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
getCommand (KeyTable *table, KeyCode code) {
  const KeyTableHeader *header = table->header.fields;
  const KeyBinding *binding = getKeyTableItem(table, header->bindingsTable);
  unsigned int count = header->bindingsCount;

  while (count) {
    if ((code == binding->key.code) &&
        sameKeyCodeMasks(table->modifiers.mask, binding->key.modifiers))
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
    if (isKeySubset(binding->key.modifiers, table->modifiers.mask)) return 1;
    binding += 1, count -= 1;
  }

  return 0;
}

void
resetKeyTable (KeyTable *table) {
  removeAllKeyCodes(&table->modifiers);
  table->lastCommand = EOF;
}

KeyCodesState
processKeyEvent (KeyTable *table, KeyCode code, int press) {
  KeyCodesState state = KCS_UNBOUND;

  removeKeyCode(&table->modifiers, code);

  if (press) {
    int command = getCommand(table, code);

    addKeyCode(&table->modifiers, code);

    if (command == EOF) {
      if (isModifiers(table)) state = KCS_MODIFIERS;
      goto release;
    }

    if (command != table->lastCommand) {
      table->lastCommand = command;
      enqueueCommand(command | BRL_FLG_REPEAT_INITIAL | BRL_FLG_REPEAT_DELAY);
    }

    state = KCS_COMMAND;
  } else {
  release:
    if (table->lastCommand != EOF) {
      table->lastCommand = EOF;
      enqueueCommand(BRL_CMD_NOOP);
    }
  }

  return state;
}
