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

static int
getCommand (KeyTable *table, unsigned char context, unsigned char set, unsigned char key) {
  const KeyBinding *binding = table->keyBindingTable;
  unsigned int count = table->keyBindingCount;
  int candidate = EOF;

  while (count) {
    if ((set == binding->keys.set) && (key == binding->keys.key) &&
        sameKeys(table->keys.mask, binding->keys.modifiers)) {
      if (binding->context == context) return binding->command;

      if (candidate == EOF)
        if (binding->context == BRL_CTX_DEFAULT)
          candidate = binding->command;
    }

    binding += 1, count -= 1;
  }

  return candidate;
}

static int
isModifiers (KeyTable *table, unsigned char context) {
  const KeyBinding *binding = table->keyBindingTable;
  unsigned int count = table->keyBindingCount;

  while (count) {
    if ((binding->context == context) || (binding->context == BRL_CTX_DEFAULT)) {
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

  if (0) {
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

    LogPrint(LOG_DEBUG, "%s", buffer);
  }

  return state;
}

static const char *
getKeyName (KeyTable *table, unsigned char set, unsigned char key) {
  int first = 0;
  int last = table->keyNameCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    const KeyNameEntry *kne = table->keyNameTable[current];

    if (set < kne->set) goto searchBelow;
    if (set > kne->set) goto searchAbove;
    if (key == kne->key) return kne->name;

    if (key < kne->key) {
    searchBelow:
      last = current - 1;
    } else {
    searchAbove:
      first = current + 1;
    }
  }

  return "?";
}

static void
formatKeyCombination (KeyTable *table, const KeyCombination *keys, char *buffer, size_t size) {
  char delimiter = 0;
  int length;

  {
    int key;
    for (key=0; key<BITMASK_SIZE(keys->modifiers); key+=1) {
      if (BITMASK_TEST(keys->modifiers, key)) {
        if (!delimiter) {
          delimiter = '+';
        } else if (size) {
          *buffer++ = delimiter;
          size -= 1;
        }

        snprintf(buffer, size, "%s%n", getKeyName(table, 0, key), &length);
        buffer += length, size -= length;
      }
    }
  }

  if (keys->set || keys->key) {
    if (delimiter && size) *buffer++ = delimiter, size -= 1;
    if (size) *buffer++ = '!', size -= 1;
    snprintf(buffer, size, "%s%n", getKeyName(table, keys->set, keys->key), &length);
    buffer += length, size -= length;
  }
}

static int
listContext (
  KeyTable *table, unsigned char context,
  const char **title, const char *keysPrefix,
  LineHandler handleLine, void *data
) {
  const KeyBinding *binding = table->keyBindingTable;
  unsigned int count = table->keyBindingCount;

  while (count) {
    if (binding->context == context) {
      char line[0X80];
      size_t size = sizeof(line);
      unsigned int offset = 0;
      int length;

      unsigned int keysOffset;

      {
        char description[0X60];
        describeCommand(binding->command, description, sizeof(description), 0);
        snprintf(&line[offset], size, "%s: %n", description, &length);
        offset += length, size -= length;
      }

      keysOffset = offset;
      if (keysPrefix) {
        snprintf(&line[offset], size, "%s, %n", keysPrefix, &length);
        offset += length, size -= length;
      }

      {
        char keys[0X40];
        formatKeyCombination(table, &binding->keys, keys, sizeof(keys));
        snprintf(&line[offset], size, "%s%n", keys, &length);
        offset += length, size -= length;
      }

      if ((binding->command & BRL_MSK_BLK) == BRL_BLK_CONTEXT) {
        unsigned char context = BRL_CTX_DEFAULT + (binding->command & BRL_MSK_ARG);
        if (!listContext(table, context, title, &line[keysOffset], handleLine, data)) return 0;
      } else {
        if (*title) {
          if (!handleLine((char *)*title, data)) return 0;
          *title = NULL;
        }

        if (!handleLine(line, data)) return 0;
      }
    }

    binding += 1, count -= 1;
  }

  return 1;
}

int
listKeyBindings (KeyTable *table, LineHandler handleLine, void *data) {
  typedef struct {
    const char *title;
    unsigned char context;
  } ContextEntry;

  static const ContextEntry contextTable[] = {
    { .context=BRL_CTX_DEFAULT,
      .title = "Default Bindings"
    }
    ,
    { .context=BRL_CTX_MENU,
      .title = "Menu Bindings"
    }
    ,
    { .title = NULL }
  };
  const ContextEntry *ctx = contextTable;

  while (ctx->title) {
    const char *title = ctx->title;

    if (!listContext(table, ctx->context, &title, NULL, handleLine, data)) return 0;
    ctx += 1;
  }

  return 1;
}
