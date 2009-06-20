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
#include "charset.h"
#include "ktb.h"
#include "ktb_internal.h"
#include "cmd.h"
#include "brldefs.h"

static inline const KeyContext *
getKeyContext (KeyTable *table, unsigned char context) {
  if (context < table->keyContextCount) return &table->keyContextTable[context];
  return NULL;
}

static inline int
isTemporaryKeyContext (const KeyTable *table, const KeyContext *ctx) {
  return ((ctx - table->keyContextTable) > BRL_CTX_DEFAULT) && !ctx->name;
}

int
compareKeyBindings (const KeyBinding *binding1, const KeyBinding *binding2) {
  if (binding1->keys.set < binding2->keys.set) return -1;
  if (binding1->keys.set > binding2->keys.set) return 1;

  if (binding1->keys.key < binding2->keys.key) return -1;
  if (binding1->keys.key > binding2->keys.key) return 1;

  return compareKeys(binding1->keys.modifiers, binding2->keys.modifiers);
}

static int
searchKeyBinding (const void *target, const void *element) {
  const KeyBinding *reference = target;
  const KeyBinding *const *binding = element;
  return compareKeyBindings(reference, *binding);
}

static const KeyBinding *
getKeyBinding (KeyTable *table, unsigned char context, unsigned char set, unsigned char key) {
  const KeyContext *ctx = getKeyContext(table, context);

  if (ctx) {
    KeyBinding target;

    target.keys.set = set;
    target.keys.key = key;
    copyKeySetMask(target.keys.modifiers, table->keys.mask);

    {
      const KeyBinding **binding = bsearch(&target, ctx->sortedKeyBindings, ctx->keyBindingCount, sizeof(*ctx->sortedKeyBindings), searchKeyBinding);
      if (binding) return *binding;
    }
  }

  return NULL;
}

static int
getCommand (KeyTable *table, unsigned char context, unsigned char set, unsigned char key) {
  const KeyBinding *binding = getKeyBinding(table, context, set, key);
  if (binding) return binding->command;

  if (context != BRL_CTX_DEFAULT)
    if ((binding = getKeyBinding(table, BRL_CTX_DEFAULT, set, key)))
      return binding->command;

  return EOF;
}

static int
getInputCommand (KeyTable *table, unsigned char context) {
  const KeyContext *ctx;
  int chordsRequested = context == BRL_CTX_CHORDS;
  int dotsCommand = BRL_BLK_PASSDOTS;
  int dotPressed = 0;
  int spacePressed = 0;
  unsigned int keyCount = 0;
  KeySetMask keyMask;

  if (chordsRequested) context = table->persistentContext;
  ctx = getKeyContext(table, context);
  if (!ctx) return EOF;
  copyKeySetMask(keyMask, table->keys.mask);

  {
    int bitsForced = 0;
    unsigned char function;

    for (function=0; function<InputFunctionCount; function+=1) {
      unsigned char key = ctx->inputKeys[function];

      if (key) {
        const InputFunctionEntry *ifn = &inputFunctionTable[function];

        if (key == BRL_MSK_ARG) {
          bitsForced |= ifn->bit;
        } else if (BITMASK_TEST(keyMask, key)) {
          if (!ifn->bit) {
            spacePressed = 1;
          } else if (ifn->bit & BRL_MSK_ARG) {
            dotPressed = 1;
          }

          dotsCommand |= ifn->bit;
          BITMASK_CLEAR(keyMask, key);
          keyCount += 1;
        }
      }
    }

    if (keyCount < table->keys.count) return EOF;
    if (dotPressed) dotsCommand |= bitsForced;
  }

  if (chordsRequested && spacePressed) {
    dotsCommand |= BRL_DOTC;
  } else if (dotPressed == spacePressed) {
    return EOF;
  }

  return dotsCommand;
}

static int
isModifiers (KeyTable *table, unsigned char context) {
  while (1) {
    const KeyContext *ctx = getKeyContext(table, context);

    if (ctx) {
      const KeyBinding *binding = ctx->keyBindingTable;
      unsigned int count = ctx->keyBindingCount;

      while (count) {
        if (isKeySubset(binding->keys.modifiers, table->keys.mask)) return 1;
        binding += 1, count -= 1;
      }
    }

    if (context == BRL_CTX_DEFAULT) break;
    context = BRL_CTX_DEFAULT;
  }

  return 0;
}

void
resetKeyTable (KeyTable *table) {
  table->currentContext = table->persistentContext = BRL_CTX_DEFAULT;
  removeAllKeys(&table->keys);
  table->command = EOF;
  table->immediate = 0;
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

  if (context == BRL_CTX_DEFAULT) context = table->currentContext;
  if (press) table->currentContext = table->persistentContext;

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
        immediate = 0;
        command = getInputCommand(table, context);
        if (command == EOF) command = getCommand(table, context, 0, 0);
      }

      if (command == EOF) {
        if (isModifiers(table, context)) state = KTS_MODIFIERS;

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
    snprintf(buffer, size, "%s%n", getKeyName(table, keys->set, keys->key), &length);
    buffer += length, size -= length;
  }
}

static int
addLine (const char *line, KeyTableHelpLineHandler handleLine, void *data) {
  size_t count = strlen(line) + 1;
  wchar_t buffer[count];
  wchar_t *characters = buffer;

  convertStringToWchars(&line, &characters, count);
  return handleLine(buffer, data);
  return 1;
}

static int
listSectionTitle (const wchar_t **title, KeyTableHelpLineHandler handleLine, void *data) {
  if (*title) {
    if (!addLine("", handleLine, data)) return 0;
    if (!handleLine(*title, data)) return 0;
    *title = NULL;
  }

  return 1;
}

static int
listKeyContext (
  KeyTable *table, const KeyContext *ctx,
  const wchar_t **title, const char *keysPrefix,
  KeyTableHelpLineHandler handleLine, void *data
) {
  {
    unsigned char function;

    for (function=0; function<InputFunctionCount; function+=1) {
      unsigned char key = ctx->inputKeys[function];

      if (key) {
        const InputFunctionEntry *ifn = &inputFunctionTable[function];
        char line[0X40];

        if (!listSectionTitle(title, handleLine, data)) return 0;
        snprintf(line, sizeof(line), "input %s: %s",
                 ifn->name,
                 ((key == BRL_MSK_ARG)? "On": getKeyName(table, 0, key)));
        if (!addLine(line, handleLine, data)) return 0;
      }
    }
  }

  {
    const KeyBinding *binding = ctx->keyBindingTable;
    unsigned int count = ctx->keyBindingCount;

    while (count) {
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
        const KeyContext *c = getKeyContext(table, (BRL_CTX_DEFAULT + (binding->command & BRL_MSK_ARG)));

        if (c) {
          if (isTemporaryKeyContext(table, c)) {
            if (!listKeyContext(table, c, title, &line[keysOffset], handleLine, data)) return 0;
          } else {
            char buffer[0X80];

            snprintf(buffer, sizeof(buffer), "switch to %" PRIws ": %s",
                     c->name, &line[keysOffset]);
            if (!addLine(buffer, handleLine, data)) return 0;
          }
        }
      } else {
        if (!listSectionTitle(title, handleLine, data)) return 0;
        if (!addLine(line, handleLine, data)) return 0;
      }

      binding += 1, count -= 1;
    }
  }

  return 1;
}

int
listKeyBindings (KeyTable *table, KeyTableHelpLineHandler handleLine, void *data) {
  {
    char line[0X40];
    size_t size = sizeof(line);
    int offset = 0;
    int length;

    snprintf(&line[offset], size, "Help%n", &length);
    offset += length, size -= length;

    if (table->title) {
      snprintf(&line[offset], size, " for %" PRIws "%n", table->title, &length);
      offset += length, size -= length;
      if (!addLine(line, handleLine, data)) return 0;
    }
  }

  {
    static const unsigned char contexts[] = {
      BRL_CTX_DEFAULT,
      BRL_CTX_MENU
    };

    const unsigned char *context = contexts;
    unsigned int count = ARRAY_COUNT(contexts);

    while (count) {
      const KeyContext *ctx = getKeyContext(table, *context);

      if (ctx) {
        const wchar_t *title = ctx->name;

        if (!listKeyContext(table, ctx, &title, NULL, handleLine, data)) return 0;
      }

      context += 1, count -= 1;
    }
  }

  {
    unsigned int context;

    for (context=BRL_CTX_DEFAULT+1; context<table->keyContextCount; context+=1) {
      const KeyContext *ctx = getKeyContext(table, context);

      if (ctx && !isTemporaryKeyContext(table, ctx)) {
        const wchar_t *title = ctx->name;

        if (!listKeyContext(table, ctx, &title, NULL, handleLine, data)) return 0;
      }
    }
  }

  return 1;
}
