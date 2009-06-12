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
getCommand (KeyTable *table, unsigned char context, unsigned char set, unsigned char key) {
  const KeyTableHeader *header = table->header.fields;
  const KeyBinding *binding = getKeyTableItem(table, header->bindingsTable);
  unsigned int count = header->bindingsCount;
  int candidate = EOF;

  while (count) {
    if ((set == binding->keys.set) && (key == binding->keys.key) &&
        sameKeys(table->keys.mask, binding->keys.modifiers)) {
      if (binding->keys.context == context) return binding->command;

      if (candidate == EOF)
        if (binding->keys.context == BRL_CTX_DEFAULT)
          candidate = binding->command;
    }

    binding += 1, count -= 1;
  }

  return candidate;
}

static int
isModifiers (KeyTable *table, unsigned char context) {
  const KeyTableHeader *header = table->header.fields;
  const KeyBinding *binding = getKeyTableItem(table, header->bindingsTable);
  unsigned int count = header->bindingsCount;

  while (count) {
    if ((binding->keys.context == context) || (binding->keys.context == BRL_CTX_DEFAULT)) {
      if (isKeySubset(binding->keys.modifiers, table->keys.mask)) return 1;
    }

    binding += 1, count -= 1;
  }

  return 0;
}

void
resetKeyTable (KeyTable *table) {
  removeAllKeys(&table->keys);
  table->context = BRL_CTX_DEFAULT;
  table->command = EOF;
  table->immediate = 0;
}

static int
processCommand (KeyTable *table, int command) {
  int blk = command & BRL_MSK_BLK;
  int arg = command & BRL_MSK_ARG;

  switch (blk) {
    case BRL_BLK_CONTEXT:
      table->context = BRL_CTX_DEFAULT + arg;
      return 1;

    default:
      break;
  }

  return enqueueCommand(command);
}

KeyTableState
processKeyEvent (KeyTable *table, unsigned char context, unsigned char set, unsigned char key, int press) {
  KeyTableState state = KTS_UNBOUND;
  int command = EOF;
  int immediate = 1;

  if (context == BRL_CTX_DEFAULT) context = table->context;
  if (press) table->context = BRL_CTX_DEFAULT;

  if (set) {
    if (press) {
      if ((command = getCommand(table, context, set, 0)) != EOF) {
        command += key;
      } else {
        command = BRL_CMD_NOOP;
      }

      table->command = EOF;
      processCommand(table, command);
      state = KTS_COMMAND;
    }
  } else {
    removeKey(&table->keys, key);

    if (press) {
      command = getCommand(table, context, 0, key);
      addKey(&table->keys, key);

      if (command == EOF) {
        command = getCommand(table, context, 0, 0);
        immediate = 0;
      }

      if (command == EOF) {
        if (isModifiers(table, context)) state = KTS_MODIFIERS;
        goto release;
      }

      if (command != table->command) {
        table->command = command;

        if ((table->immediate = immediate)) {
          command |= BRL_FLG_REPEAT_INITIAL | BRL_FLG_REPEAT_DELAY;
        } else {
          command |= BRL_FLG_REPEAT_DELAY;
        }

        processCommand(table, command);
      } else {
        command = EOF;
      }

      state = KTS_COMMAND;
    } else {
    release:
      if (table->command != EOF) {
        if (table->immediate) {
          command = BRL_CMD_NOOP;
        } else {
          command = table->command;
        }

        table->command = EOF;
        processCommand(table, command);
      }
    }
  }

  if (1) {
    char buffer[0X40];
    size_t size = sizeof(buffer);
    int offset = 0;
    int length;

    snprintf(&buffer[offset], size, "Key %s: Ctx:%u Set:%u Key:%u%n",
             press? "Press": "Release",
             context, set, key, &length);
    offset += length, size -= length;

    if (command != EOF) {
      snprintf(&buffer[offset], size, " Cmd:%06X", command);
      offset += length, size -= length;
    }

    LogPrint(LOG_NOTICE, "%s", buffer);
  }

  return state;
}
