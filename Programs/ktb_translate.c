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

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "ktb.h"
#include "ktb_internal.h"
#include "ktb_inspect.h"
#include "brl.h"

static int
searchKeyBinding (const void *target, const void *element) {
  const KeyBinding *reference = target;
  const KeyBinding *const *binding = element;
  return compareKeyBindings(reference, *binding);
}

static const KeyBinding *
getKeyBinding (KeyTable *table, unsigned char context, unsigned char set, unsigned char key) {
  const KeyContext *ctx = getKeyContext(table, context);

  if (ctx && ctx->sortedKeyBindings) {
    KeyBinding target;

    target.keys.set = set;
    target.keys.key = key;
    copyKeySetMask(target.keys.modifiers.mask, table->keys.mask);

    {
      const KeyBinding **binding = bsearch(&target, ctx->sortedKeyBindings, ctx->keyBindingCount, sizeof(*ctx->sortedKeyBindings), searchKeyBinding);
      if (binding) return *binding;
    }
  }

  return NULL;
}

static int
isModifiers (KeyTable *table, unsigned char context) {
  const KeyContext *ctx = getKeyContext(table, context);

  if (ctx) {
    const KeyBinding *binding = ctx->keyBindingTable;
    unsigned int count = ctx->keyBindingCount;

    while (count) {
      if (isKeySetSubmask(binding->keys.modifiers.mask, table->keys.mask)) return 1;
      binding += 1, count -= 1;
    }
  }

  return 0;
}

static int
searchHotkeyEntry (const void *target, const void *element) {
  const HotkeyEntry *reference = target;
  const HotkeyEntry *const *hotkey = element;
  return compareKeys(reference->set, reference->key, (*hotkey)->set, (*hotkey)->key);
}

static const HotkeyEntry *
getHotkeyEntry (KeyTable *table, unsigned char context, unsigned char set, unsigned char key) {
  const KeyContext *ctx = getKeyContext(table, context);

  if (ctx && ctx->sortedHotkeyEntries) {
    HotkeyEntry target;

    target.set = set;
    target.key = key;

    {
      const HotkeyEntry **hotkey = bsearch(&target, ctx->sortedHotkeyEntries, ctx->hotkeyCount, sizeof(*ctx->sortedHotkeyEntries), searchHotkeyEntry);
      if (hotkey) return *hotkey;
    }
  }

  return NULL;
}

static int
getKeyboardCommand (KeyTable *table, unsigned char context) {
  int chordsRequested = context == BRL_CTX_CHORDS;
  const KeyContext *ctx;

  if (chordsRequested) context = table->persistentContext;
  if (!(ctx = getKeyContext(table, context))) return EOF;

  if (ctx->keyMap) {
    int keyboardCommand = BRL_BLK_PASSDOTS;
    int dotPressed = 0;
    int spacePressed = 0;

    {
      unsigned int keyIndex;

      for (keyIndex=0; keyIndex<table->keys.count; keyIndex+=1) {
        KeyboardFunction function = ctx->keyMap[table->keys.keys[keyIndex]];

        if (function == KBF_None) return EOF;

        {
          const KeyboardFunctionEntry *kbf = &keyboardFunctionTable[function];

          keyboardCommand |= kbf->bit;

          if (!kbf->bit) {
            spacePressed = 1;
          } else if (kbf->bit & BRL_MSK_ARG) {
            dotPressed = 1;
          }
        }
      }

      if (dotPressed) keyboardCommand |= ctx->superimposedBits;
    }

    if (chordsRequested && spacePressed) {
      keyboardCommand |= BRL_DOTC;
    } else if (dotPressed == spacePressed) {
      return EOF;
    }

    return keyboardCommand;
  }

  return EOF;
}

static int
processCommand (KeyTable *table, int command) {
  int blk = command & BRL_MSK_BLK;
  int arg = command & BRL_MSK_ARG;

  switch (blk) {
    case BRL_BLK_CONTEXT:
      if (!BRL_DELAYED_COMMAND(command)) {
        unsigned char context = BRL_CTX_DEFAULT + arg;
        const KeyContext *ctx = getKeyContext(table, context);

        if (ctx) {
          table->currentContext = context;
          if (!isTemporaryKeyContext(table, ctx)) table->persistentContext = context;
        }
      }

      command = BRL_CMD_NOOP;
      break;

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
  const HotkeyEntry *hotkey;

  if (context == BRL_CTX_DEFAULT) context = table->currentContext;
  if (press) table->currentContext = table->persistentContext;

  if ((hotkey = getHotkeyEntry(table, context, set, key))) {
    int cmd = press? hotkey->press: hotkey->release;
    if (cmd != BRL_CMD_NOOP) processCommand(table, (command = cmd));
    state = KTS_HOTKEY;
  } else if (set) {
    if (press) {
      const KeyBinding *binding = getKeyBinding(table, context, set, 0);

      if (!binding)
        if (context != BRL_CTX_DEFAULT)
          binding = getKeyBinding(table, BRL_CTX_DEFAULT, set, 0);

      if (binding) {
        command = binding->command + key;
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
      const KeyBinding *binding = getKeyBinding(table, context, 0, key);
      addKey(&table->keys, key);

      if (binding) {
        command = binding->command;
      } else if ((binding = getKeyBinding(table, context, 0, 0))) {
        command = binding->command;
        immediate = 0;
      } else if ((command = getKeyboardCommand(table, context)) != EOF) {
        immediate = 0;
      } else if (context == BRL_CTX_DEFAULT) {
        command = EOF;
      } else {
        removeKey(&table->keys, key);
        binding = getKeyBinding(table, BRL_CTX_DEFAULT, 0, key);
        addKey(&table->keys, key);

        if (binding) {
          command = binding->command;
        } else if ((binding = getKeyBinding(table, BRL_CTX_DEFAULT, 0, 0))) {
          command = binding->command;
          immediate = 0;
        } else {
          command = EOF;
        }
      }

      if (command == EOF) {
        if (isModifiers(table, context)) {
          state = KTS_MODIFIERS;
        } else if (context != BRL_CTX_DEFAULT) {
          if (isModifiers(table, BRL_CTX_DEFAULT)) state = KTS_MODIFIERS;
        }

        if (table->command != EOF) {
          table->command = EOF;
          processCommand(table, (command = BRL_CMD_NOOP));
        }
      } else {
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
      }
    } else {
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

  if (table->logKeyEvents) {
    char buffer[0X40];
    size_t size = sizeof(buffer);
    int offset = 0;
    int length;

    snprintf(&buffer[offset], size, "Key %s: Ctx:%u Set:%u Key:%u%n",
             press? "Press": "Release",
             context, set, key, &length);
    offset += length, size -= length;

    if (command != EOF) {
      snprintf(&buffer[offset], size, " Cmd:%06X%n", command, &length);
      offset += length, size -= length;
    }

    LogPrint(LOG_DEBUG, "%s", buffer);
  }

  return state;
}

void
logKeyEvents (KeyTable *table) {
  table->logKeyEvents = 1;
}
