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
#include "prefs.h"
#include "ktb.h"
#include "ktb_internal.h"
#include "ktb_inspect.h"
#include "cmd.h"
#include "cmd_queue.h"
#include "async_alarm.h"

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
      (table->pressedKeys.count <= MAX_MODIFIERS_PER_COMBINATION)) {
    KeyBinding target;
    memset(&target, 0, sizeof(target));

    if (immediate) {
      target.keyCombination.immediateKey = *immediate;
      target.keyCombination.flags |= KCF_IMMEDIATE_KEY;
    }
    target.keyCombination.modifierCount = table->pressedKeys.count;

    while (1) {
      unsigned int all = (1 << table->pressedKeys.count) - 1;
      unsigned int bits;

      for (bits=0; bits<=all; bits+=1) {
        {
          unsigned int index;
          unsigned int bit;

          for (index=0, bit=1; index<table->pressedKeys.count; index+=1, bit<<=1) {
            KeyValue *modifier = &target.keyCombination.modifierKeys[index];

            *modifier = table->pressedKeys.table[index];
            if (bits & bit) modifier->key = KTB_KEY_ANY;
          }
        }
        qsort(target.keyCombination.modifierKeys, table->pressedKeys.count, sizeof(*target.keyCombination.modifierKeys), sortModifierKeys);

        {
          const KeyBinding *const *binding = bsearch(&target, ctx->keyBindings.sorted, ctx->keyBindings.count, sizeof(*ctx->keyBindings.sorted), searchKeyBinding);

          if (binding) {
            if ((*binding)->primaryCommand.value != EOF) return *binding;
            *isIncomplete = 1;
          }
        }
      }

      if (!(target.keyCombination.flags & KCF_IMMEDIATE_KEY)) break;
      if (target.keyCombination.immediateKey.key == KTB_KEY_ANY) break;
      target.keyCombination.immediateKey.key = KTB_KEY_ANY;
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

      for (pressedIndex=0; pressedIndex<table->pressedKeys.count; pressedIndex+=1) {
        const KeyValue *keyValue = &table->pressedKeys.table[pressedIndex];
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
  return findKeyValue(table->pressedKeys.table, table->pressedKeys.count, value, position);
}

static int
insertPressedKey (KeyTable *table, const KeyValue *value, unsigned int position) {
  return insertKeyValue(&table->pressedKeys.table, &table->pressedKeys.count, &table->pressedKeys.size, value, position);
}

static void
removePressedKey (KeyTable *table, unsigned int position) {
  removeKeyValue(table->pressedKeys.table, &table->pressedKeys.count, position);
}

static inline void
deleteExplicitKeyValue (KeyValue *values, unsigned int *count, const KeyValue *value) {
  if (value->key != KTB_KEY_ANY) deleteKeyValue(values, count, value);
}

static int
sortKeyOffsets (const void *element1, const void *element2) {
  const KeyValue *value1 = element1;
  const KeyValue *value2 = element2;

  if (value1->key < value2->key) return -1;
  if (value1->key > value2->key) return 1;
  return 0;
}

static void
addCommandArguments (KeyTable *table, int *command, const CommandEntry *entry, const KeyBinding *binding) {
  if (entry->isOffset | entry->isColumn | entry->isRow | entry->isRange | entry->isKeyboard) {
    unsigned int keyCount = table->pressedKeys.count;
    KeyValue keyValues[keyCount];
    copyKeyValues(keyValues, table->pressedKeys.table, keyCount);

    {
      int index;

      for (index=0; index<binding->keyCombination.modifierCount; index+=1) {
        deleteExplicitKeyValue(keyValues, &keyCount, &binding->keyCombination.modifierKeys[index]);
      }
    }

    if (binding->keyCombination.flags & KCF_IMMEDIATE_KEY) {
      deleteExplicitKeyValue(keyValues, &keyCount, &binding->keyCombination.immediateKey);
    }

    if (keyCount > 0) {
      if (keyCount > 1) {
        qsort(keyValues, keyCount, sizeof(*keyValues), sortKeyOffsets);
        if (entry->isRange) *command |= BRL_EXT(keyValues[1].key);
      }

      *command += keyValues[0].key;
    } else if (entry->isColumn) {
      if (!entry->isRouting) *command |= BRL_MSK_ARG;
    }
  }
}

static int
processCommand (KeyTable *table, int command) {
  int blk = command & BRL_MSK_BLK;
  int arg = command & BRL_MSK_ARG;

  switch (blk) {
    case BRL_BLK_CONTEXT: {
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
      break;
    }

    default:
      break;
  }

  return enqueueCommand(command);
}

static void
logKeyEvent (
  KeyTable *table, const char *action,
  unsigned char context, const KeyValue *keyValue, int command
) {
  if (table->logKeyEventsFlag && *table->logKeyEventsFlag) {
    char buffer[0X100];

    STR_BEGIN(buffer, sizeof(buffer));
    if (table->logLabel) STR_PRINTF("%s ", table->logLabel);
    STR_PRINTF("key %s: ", action);

    {
      size_t length = formatKeyName(table, STR_NEXT, STR_LEFT, keyValue);
      STR_ADJUST(length);
    }

    STR_PRINTF(" (Ctx:%u Set:%u Key:%u)", context, keyValue->set, keyValue->key);

    if (command != EOF) {
      const CommandEntry *cmd = getCommandEntry(command);
      const char *name = cmd? cmd->name: "?";

      STR_PRINTF(" -> %s (Cmd:%06X)", name, command);
    }

    STR_END;
    logMessage(categoryLogLevel, "%s", buffer);
  }
}

static void setLongPressAlarm (KeyTable *table, unsigned char when);

static void
handleLongPressAlarm (const AsyncAlarmCallbackParameters *parameters) {
  KeyTable *table = parameters->data;
  int command = table->longPress.secondaryCommand;

  asyncDiscardHandle(table->longPress.alarm);
  table->longPress.alarm = NULL;

  if (command != BRL_CMD_NOOP) {
    logKeyEvent(table, table->longPress.keyAction,
                table->longPress.keyContext,
                &table->longPress.keyValue,
                command);

    if (table->longPress.repeat) {
      table->longPress.keyAction = "repeat";
      setLongPressAlarm(table, prefs.autorepeatInterval);
    }

    table->longPress.primaryCommand = BRL_CMD_NOOP;
    processCommand(table, command);
  }
}

static void
setLongPressAlarm (KeyTable *table, unsigned char when) {
  asyncSetAlarmIn(&table->longPress.alarm, PREFERENCES_TIME(when),
                  handleLongPressAlarm, table);
}

static int
isRepeatableCommand (int command) {
  if (prefs.autorepeat) {
    switch (command & BRL_MSK_BLK) {
      case BRL_BLK_PASSCHAR:
      case BRL_BLK_PASSDOTS:
        return 1;

      default:
        switch (command & BRL_MSK_CMD) {
          case BRL_CMD_LNUP:
          case BRL_CMD_LNDN:
          case BRL_CMD_PRDIFLN:
          case BRL_CMD_NXDIFLN:
          case BRL_CMD_CHRLT:
          case BRL_CMD_CHRRT:

          case BRL_CMD_MENU_PREV_ITEM:
          case BRL_CMD_MENU_NEXT_ITEM:
          case BRL_CMD_MENU_PREV_SETTING:
          case BRL_CMD_MENU_NEXT_SETTING:

          case BRL_BLK_PASSKEY + BRL_KEY_BACKSPACE:
          case BRL_BLK_PASSKEY + BRL_KEY_DELETE:
          case BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP:
          case BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN:
          case BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP:
          case BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN:
          case BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT:
          case BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT:

          case BRL_CMD_SPEAK_PREV_CHAR:
          case BRL_CMD_SPEAK_NEXT_CHAR:
          case BRL_CMD_SPEAK_PREV_WORD:
          case BRL_CMD_SPEAK_NEXT_WORD:
          case BRL_CMD_SPEAK_PREV_LINE:
          case BRL_CMD_SPEAK_NEXT_LINE:
            return 1;

          case BRL_CMD_FWINLT:
          case BRL_CMD_FWINRT:
            if (prefs.autorepeatPanning) return 1;

          default:
            break;
        }
        break;
    }
  }

  return 0;
}

KeyTableState
processKeyEvent (KeyTable *table, unsigned char context, unsigned char set, unsigned char key, int press) {
  const KeyValue keyValue = {
    .set = set,
    .key = key
  };

  KeyTableState state = KTS_UNBOUND;
  int command = EOF;
  const HotkeyEntry *hotkey;

  if (press && !table->pressedKeys.count) {
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
    state = KTS_HOTKEY;
    resetLongPressData(table);

    {
      const BoundCommand *cmd = press? &hotkey->pressCommand: &hotkey->releaseCommand;

      if (cmd->value != BRL_CMD_NOOP) processCommand(table, (command = cmd->value));
    }
  } else {
    int isImmediate = 1;
    unsigned int keyPosition;
    int wasPressed = findPressedKey(table, &keyValue, &keyPosition);

    if (wasPressed) removePressedKey(table, keyPosition);

    if (press) {
      int isIncomplete = 0;
      const KeyBinding *binding = findKeyBinding(table, context, &keyValue, &isIncomplete);
      int inserted = insertPressedKey(table, &keyValue, keyPosition);

      if (binding) {
        command = binding->primaryCommand.value;
      } else if ((binding = findKeyBinding(table, context, NULL, &isIncomplete))) {
        command = binding->primaryCommand.value;
        isImmediate = 0;
      } else if ((command = makeKeyboardCommand(table, context)) != EOF) {
        isImmediate = 0;
      } else if (context == KTB_CTX_DEFAULT) {
        command = EOF;
      } else if (!inserted) {
        command = EOF;
      } else {
        removePressedKey(table, keyPosition);
        binding = findKeyBinding(table, KTB_CTX_DEFAULT, &keyValue, &isIncomplete);
        inserted = insertPressedKey(table, &keyValue, keyPosition);

        if (binding) {
          command = binding->primaryCommand.value;
        } else if ((binding = findKeyBinding(table, KTB_CTX_DEFAULT, NULL, &isIncomplete))) {
          command = binding->primaryCommand.value;
          isImmediate = 0;
        } else {
          command = EOF;
        }
      }

      if (command == EOF) {
        command = BRL_CMD_NOOP;
        if (isIncomplete) state = KTS_MODIFIERS;
      } else {
        state = KTS_COMMAND;
      }

      if (!wasPressed) {
        int secondaryCommand = BRL_CMD_NOOP;

        resetLongPressData(table);

        if (binding) {
          addCommandArguments(table, &command, binding->primaryCommand.entry, binding);

          secondaryCommand = binding->secondaryCommand.value;
          addCommandArguments(table, &secondaryCommand, binding->secondaryCommand.entry, binding);
        }

        if (context != KTB_CTX_WAITING) {
          int pending = !isImmediate;
          int secondary;
          int repeat;

          if (secondaryCommand == BRL_CMD_NOOP) {
            if (isRepeatableCommand(command)) {
              secondaryCommand = command;
            }
          }

          secondary = secondaryCommand != BRL_CMD_NOOP;
          repeat = isRepeatableCommand(secondaryCommand);

          if (secondary || pending || repeat) {
            table->longPress.primaryCommand = pending? command: BRL_CMD_NOOP;
            if (pending) command = BRL_CMD_NOOP;

            table->longPress.secondaryCommand = secondaryCommand;
            table->longPress.repeat = repeat;

            table->longPress.keyAction = "long";
            table->longPress.keyContext = context;
            table->longPress.keyValue = keyValue;

            setLongPressAlarm(table, prefs.longPressTime);
          }
        }

        processCommand(table, command);
      }
    } else {
      int *cmd = &table->longPress.primaryCommand;

      if (*cmd != BRL_CMD_NOOP) {
        processCommand(table, (command = *cmd));
        *cmd = BRL_CMD_NOOP;
      }

      resetLongPressData(table);
    }
  }

  logKeyEvent(table, (press? "press": "release"), context, &keyValue, command);
  return state;
}

void
setKeyTableLogLabel (KeyTable *table, const char *label) {
  table->logLabel = label;
}

void
setLogKeyEventsFlag (KeyTable *table, const unsigned char *flag) {
  table->logKeyEventsFlag = flag;
}
