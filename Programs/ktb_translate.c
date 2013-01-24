/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "ktb.h"
#include "ktb_internal.h"
#include "ktb_inspect.h"
#include "brl.h"

static int
sortModifierKeys (const void *element1, const void *element2) {
  const KeyValue *modifier1 = element1;
  const KeyValue *modifier2 = element2;
  return compareKeyValues(modifier1, modifier2);
}

static int
searchKeyBinding (const void *target, const void *element) {
  const KeyBinding *reference = target;
  const KeyBinding *const *binding = element;
  return compareKeyBindings(reference, *binding);
}

static const KeyBinding *
findKeyBinding (KeyTable *table, unsigned char context, const KeyValue *immediate, int *isIncomplete) {
  const KeyContext *ctx = getKeyContext(table, context);

  if (ctx && ctx->keyBindings.sorted &&
      (table->pressedCount <= MAX_MODIFIERS_PER_COMBINATION)) {
    KeyBinding target;
    memset(&target, 0, sizeof(target));

    if (immediate) {
      target.combination.immediateKey = *immediate;
      target.combination.flags |= KCF_IMMEDIATE_KEY;
    }
    target.combination.modifierCount = table->pressedCount;

    while (1) {
      unsigned int all = (1 << table->pressedCount) - 1;
      unsigned int bits;

      for (bits=0; bits<=all; bits+=1) {
        {
          unsigned int index;
          unsigned int bit;

          for (index=0, bit=1; index<table->pressedCount; index+=1, bit<<=1) {
            KeyValue *modifier = &target.combination.modifierKeys[index];

            *modifier = table->pressedKeys[index];
            if (bits & bit) modifier->key = KTB_KEY_ANY;
          }
        }
        qsort(target.combination.modifierKeys, table->pressedCount, sizeof(*target.combination.modifierKeys), sortModifierKeys);

        {
          const KeyBinding *const *binding = bsearch(&target, ctx->keyBindings.sorted, ctx->keyBindings.count, sizeof(*ctx->keyBindings.sorted), searchKeyBinding);

          if (binding) {
            if ((*binding)->command != EOF) return *binding;
            *isIncomplete = 1;
          }
        }
      }

      if (!(target.combination.flags & KCF_IMMEDIATE_KEY)) break;
      if (target.combination.immediateKey.key == KTB_KEY_ANY) break;
      target.combination.immediateKey.key = KTB_KEY_ANY;
    }
  }

  return NULL;
}

static int
searchHotkeyEntry (const void *target, const void *element) {
  const HotkeyEntry *reference = target;
  const HotkeyEntry *const *hotkey = element;
  return compareKeyValues(&reference->keyValue, &(*hotkey)->keyValue);
}

static const HotkeyEntry *
findHotkeyEntry (KeyTable *table, unsigned char context, const KeyValue *keyValue) {
  const KeyContext *ctx = getKeyContext(table, context);

  if (ctx && ctx->hotkeys.sorted) {
    HotkeyEntry target = {
      .keyValue = *keyValue
    };

    {
      const HotkeyEntry *const *hotkey = bsearch(&target, ctx->hotkeys.sorted, ctx->hotkeys.count, sizeof(*ctx->hotkeys.sorted), searchHotkeyEntry);
      if (hotkey) return *hotkey;
    }
  }

  return NULL;
}

static int
searchMappedKeyEntry (const void *target, const void *element) {
  const MappedKeyEntry *reference = target;
  const MappedKeyEntry *const *map = element;
  return compareKeyValues(&reference->keyValue, &(*map)->keyValue);
}

static const MappedKeyEntry *
findMappedKeyEntry (const KeyContext *ctx, const KeyValue *keyValue) {
  if (ctx->mappedKeys.sorted) {
    MappedKeyEntry target = {
      .keyValue = *keyValue
    };

    {
      const MappedKeyEntry *const *map = bsearch(&target, ctx->mappedKeys.sorted, ctx->mappedKeys.count, sizeof(*ctx->mappedKeys.sorted), searchMappedKeyEntry);
      if (map) return *map;
    }
  }

  return NULL;
}

static int
makeKeyboardCommand (KeyTable *table, unsigned char context) {
  int chordsRequested = context == KTB_CTX_CHORDS;
  const KeyContext *ctx;

  if (chordsRequested) context = table->context.persistent;

  if ((ctx = getKeyContext(table, context))) {
    int keyboardCommand = BRL_BLK_PASSDOTS;

    {
      unsigned int pressedIndex;

      for (pressedIndex=0; pressedIndex<table->pressedCount; pressedIndex+=1) {
        const KeyValue *keyValue = &table->pressedKeys[pressedIndex];
        const MappedKeyEntry *map = findMappedKeyEntry(ctx, keyValue);

        if (!map) return EOF;
        keyboardCommand |= map->keyboardFunction->bit;
      }
    }

    {
      int dotPressed = !!(keyboardCommand & (BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8));
      int spacePressed = !!(keyboardCommand & BRL_DOTC);

      if (dotPressed) keyboardCommand |= ctx->mappedKeys.superimpose;

      if (!chordsRequested) {
        if (dotPressed == spacePressed) return EOF;
        keyboardCommand &= ~BRL_DOTC;
      }
    }

    return keyboardCommand;
  }

  return EOF;
}

static int
findPressedKey (KeyTable *table, const KeyValue *value, unsigned int *position) {
  return findKeyValue(table->pressedKeys, table->pressedCount, value, position);
}

static int
insertPressedKey (KeyTable *table, const KeyValue *value, unsigned int position) {
  return insertKeyValue(&table->pressedKeys, &table->pressedCount, &table->pressedSize, value, position);
}

static void
removePressedKey (KeyTable *table, unsigned int position) {
  removeKeyValue(table->pressedKeys, &table->pressedCount, position);
}

static inline void
deleteExplicitKeyValue (KeyValue *values, unsigned int *count, const KeyValue *value) {
  if (value->key != KTB_KEY_ANY) deleteKeyValue(values, count, value);
}

static int
processCommand (KeyTable *table, int command) {
  int blk = command & BRL_MSK_BLK;
  int arg = command & BRL_MSK_ARG;

  switch (blk) {
    case BRL_BLK_CONTEXT:
      if (!BRL_DELAYED_COMMAND(command)) {
        unsigned char context = KTB_CTX_DEFAULT + arg;
        const KeyContext *ctx = getKeyContext(table, context);

        if (ctx) {
          command = BRL_CMD_NOOP;
          table->context.next = context;

          if (isTemporaryKeyContext(table, ctx)) {
            command |= BRL_FLG_TOGGLE_ON;
          } else {
            table->context.persistent = context;
            command |= BRL_FLG_TOGGLE_OFF;
          }
        }
      }
      break;

    default:
      break;
  }

  return enqueueCommand(command);
}

static int
sortKeyOffsets (const void *element1, const void *element2) {
  const KeyValue *value1 = element1;
  const KeyValue *value2 = element2;

  if (value1->key < value2->key) return -1;
  if (value1->key > value2->key) return 1;
  return 0;
}

KeyTableState
processKeyEvent (KeyTable *table, unsigned char context, unsigned char set, unsigned char key, int press) {
  KeyValue keyValue = {
    .set = set,
    .key = key
  };

  KeyTableState state = KTS_UNBOUND;
  int command = EOF;
  const HotkeyEntry *hotkey;

  if (press && !table->pressedCount) {
    table->context.current = table->context.next;
    table->context.next = table->context.persistent;
  }
  if (context == KTB_CTX_DEFAULT) context = table->context.current;

  if (!(hotkey = findHotkeyEntry(table, context, &keyValue))) {
    const KeyValue anyKey = {
      .set = keyValue.set,
      .key = KTB_KEY_ANY
    };

    hotkey = findHotkeyEntry(table, context, &anyKey);
  }

  if (hotkey) {
    int cmd = press? hotkey->pressCommand: hotkey->releaseCommand;
    if (cmd != BRL_CMD_NOOP) processCommand(table, (command = cmd));
    state = KTS_HOTKEY;
  } else {
    int immediate = 1;
    unsigned int keyPosition;

    if (findPressedKey(table, &keyValue, &keyPosition)) removePressedKey(table, keyPosition);

    if (press) {
      int isIncomplete = 0;
      const KeyBinding *binding = findKeyBinding(table, context, &keyValue, &isIncomplete);
      int inserted = insertPressedKey(table, &keyValue, keyPosition);

      if (binding) {
        command = binding->command;
      } else if ((binding = findKeyBinding(table, context, NULL, &isIncomplete))) {
        command = binding->command;
        immediate = 0;
      } else if ((command = makeKeyboardCommand(table, context)) != EOF) {
        immediate = 0;
      } else if (context == KTB_CTX_DEFAULT) {
        command = EOF;
      } else if (!inserted) {
        command = EOF;
      } else {
        removePressedKey(table, keyPosition);
        binding = findKeyBinding(table, KTB_CTX_DEFAULT, &keyValue, &isIncomplete);
        inserted = insertPressedKey(table, &keyValue, keyPosition);

        if (binding) {
          command = binding->command;
        } else if ((binding = findKeyBinding(table, KTB_CTX_DEFAULT, NULL, &isIncomplete))) {
          command = binding->command;
          immediate = 0;
        } else {
          command = EOF;
        }
      }

      if (command == EOF) {
        if (isIncomplete) state = KTS_MODIFIERS;

        if (table->command != EOF) {
          table->command = EOF;
          processCommand(table, (command = BRL_CMD_NOOP));
        }
      } else {
        if (command != table->command) {
          if (binding) {
            if (binding->flags & (KBF_OFFSET | KBF_COLUMN | KBF_ROW | KBF_RANGE | KBF_KEYBOARD)) {
              unsigned int keyCount = table->pressedCount;
              KeyValue keyValues[keyCount];
              copyKeyValues(keyValues, table->pressedKeys, keyCount);

              {
                int index;

                for (index=0; index<binding->combination.modifierCount; index+=1) {
                  deleteExplicitKeyValue(keyValues, &keyCount, &binding->combination.modifierKeys[index]);
                }
              }

              if (binding->combination.flags & KCF_IMMEDIATE_KEY) {
                deleteExplicitKeyValue(keyValues, &keyCount, &binding->combination.immediateKey);
              }

              if (keyCount > 0) {
                if (keyCount > 1) {
                  qsort(keyValues, keyCount, sizeof(*keyValues), sortKeyOffsets);
                  if (binding->flags & KBF_RANGE) command |= BRL_EXT(keyValues[1].key);
                }

                command += keyValues[0].key;
              } else if (binding->flags & KBF_COLUMN) {
                if (!(binding->flags & KBF_ROUTE)) command |= BRL_MSK_ARG;
              }
            }
          }

          if (context == KTB_CTX_WAITING) {
            table->command = EOF;
            table->immediate = 0;
          } else {
            table->command = command;

            if ((table->immediate = immediate)) {
              command |= BRL_FLG_REPEAT_INITIAL | BRL_FLG_REPEAT_DELAY;
            } else {
              command |= BRL_FLG_REPEAT_DELAY;
            }
          }

          processCommand(table, command);
        } else {
          command = EOF;
        }

        state = KTS_COMMAND;
      }
    } else {
      if (context == KTB_CTX_WAITING) table->command = EOF;

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

  if (table->logKeyEvents && *table->logKeyEvents) {
    char buffer[0X40];

    STR_BEGIN(buffer, sizeof(buffer));
    STR_PRINTF("Key %s: Ctx:%u Set:%u Key:%u",
               press? "Press": "Release",
               context, set, key);

    if (command != EOF) {
      STR_PRINTF(" Cmd:%06X", command);
    }

    STR_END
    logMessage(categoryLogLevel, "%s", buffer);
  }

  return state;
}

void
setKeyEventLoggingFlag (KeyTable *table, const unsigned char *flag) {
  table->logKeyEvents = flag;
}
