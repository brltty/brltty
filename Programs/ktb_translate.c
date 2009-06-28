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

  if (ctx && ctx->sortedKeyBindings) {
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
isModifiers (KeyTable *table, unsigned char context) {
  const KeyContext *ctx = getKeyContext(table, context);

  if (ctx) {
    const KeyBinding *binding = ctx->keyBindingTable;
    unsigned int count = ctx->keyBindingCount;

    while (count) {
      if (isKeySubset(binding->keys.modifiers, table->keys.mask)) return 1;
      binding += 1, count -= 1;
    }
  }

  return 0;
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

typedef struct {
  KeyTable *const keyTable;
  const wchar_t *sectionTitle;

  wchar_t *lineCharacters;
  size_t lineSize;
  size_t lineLength;

  KeyTableListHandler *const handleLine;
  void *const handlerData;
} ListGenerationData;

static int
handleLine (ListGenerationData *lgd, const wchar_t *line) {
  return lgd->handleLine(line, lgd->handlerData);
}

static int
putCharacters (ListGenerationData *lgd, const wchar_t *characters, size_t count) {
  size_t newLength = lgd->lineLength + count;

  if (lgd->sectionTitle) {
    if (!handleLine(lgd, WS_C(""))) return 0;
    if (!handleLine(lgd, lgd->sectionTitle)) return 0;
    lgd->sectionTitle = NULL;
  }

  if (newLength > lgd->lineSize) {
    size_t newSize = (newLength | 0X3F) + 1;
    wchar_t *newCharacters = realloc(lgd->lineCharacters, ARRAY_SIZE(newCharacters, newSize));

    if (!newCharacters) {
      LogError("realloc");
      return 0;
    }

    lgd->lineCharacters = newCharacters;
    lgd->lineSize = newSize;
  }

  wmemcpy(&lgd->lineCharacters[lgd->lineLength], characters, count);
  lgd->lineLength = newLength;
  return 1;
}

static int
putCharacter (ListGenerationData *lgd, wchar_t character) {
  return putCharacters(lgd, &character, 1);
}

static int
putCharacterString (ListGenerationData *lgd, const wchar_t *string) {
  return putCharacters(lgd, string, wcslen(string));
}

static int
putUtf8String (ListGenerationData *lgd, const char *string) {
  size_t size = strlen(string) + 1;
  wchar_t characters[size];
  wchar_t *character = characters;

  convertStringToWchars(&string, &character, size);
  return putCharacters(lgd, characters, character-characters);
}

static int
putKeyName (ListGenerationData *lgd, unsigned char set, unsigned char key) {
  int first = 0;
  int last = lgd->keyTable->keyNameCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    const KeyNameEntry *kne = lgd->keyTable->keyNameTable[current];

    if (set < kne->set) goto searchBelow;
    if (set > kne->set) goto searchAbove;
    if (key == kne->key) return putUtf8String(lgd, kne->name);

    if (key < kne->key) {
    searchBelow:
      last = current - 1;
    } else {
    searchAbove:
      first = current + 1;
    }
  }

  return putCharacter(lgd, WC_C('?'));
}

static int
putKeyCombination (ListGenerationData *lgd, const KeyCombination *keys) {
  wchar_t delimiter = 0;

  {
    int key;
    for (key=0; key<BITMASK_SIZE(keys->modifiers); key+=1) {
      if (BITMASK_TEST(keys->modifiers, key)) {
        if (!delimiter) {
          delimiter = WC_C('+');
        } else if (!putCharacter(lgd, delimiter)) {
          return 0;
        }

        if (!putKeyName(lgd, 0, key)) return 0;
      }
    }
  }

  if (keys->set || keys->key) {
    if (delimiter)
      if (!putCharacter(lgd, delimiter))
        return 0;

    if (!putKeyName(lgd, keys->set, keys->key)) return 0;
  }

  return 1;
}

static int
putCommandDescription (ListGenerationData *lgd, int command) {
  char description[0X60];

  describeCommand(command, description, sizeof(description), 0);
  return putUtf8String(lgd, description);
}

static int
endLine (ListGenerationData *lgd) {
  if (!putCharacter(lgd, 0)) return 0;
  if (!handleLine(lgd, lgd->lineCharacters)) return 0;

  lgd->lineLength = 0;
  return 1;
}

static int
listKeyboardFunctions (ListGenerationData *lgd, const KeyContext *ctx) {
  if (ctx->keyMap) {
    {
      unsigned int key;

      for (key=0; key<KEYS_PER_SET; key+=1) {
        KeyboardFunction function = ctx->keyMap[key];

        if (function != KBF_None) {
          const KeyboardFunctionEntry *kbf = &keyboardFunctionTable[function];

          if (!putUtf8String(lgd, kbf->name)) return 0;
          if (!putCharacterString(lgd, WS_C(" key"))) return 0;
          if (!putCharacterString(lgd, WS_C(": "))) return 0;
          if (!putKeyName(lgd, 0, key)) return 0;
          if (!endLine(lgd)) return 0;
        }
      }
    }

    {
      KeyboardFunction function;

      for (function=0; function<KBF_None; function+=1) {
        const KeyboardFunctionEntry *kbf = &keyboardFunctionTable[function];

        if (ctx->superimposedBits & kbf->bit) {
          if (!putUtf8String(lgd, kbf->name)) return 0;
          if (!putCharacterString(lgd, WS_C(" key: superimposed"))) return 0;
          if (!endLine(lgd)) return 0;
        }
      }
    }
  }

  return 1;
}

static int
listKeyContext (ListGenerationData *lgd, const KeyContext *ctx, const wchar_t *keysPrefix) {
  if (!listKeyboardFunctions(lgd, ctx)) return 0;

  {
    const KeyBinding *binding = ctx->keyBindingTable;
    unsigned int count = ctx->keyBindingCount;

    while (count) {
      size_t keysOffset;

      if (!putCommandDescription(lgd, binding->command)) return 0;
      if (!putCharacterString(lgd, WS_C(": "))) return 0;
      keysOffset = lgd->lineLength;

      if (keysPrefix) {
        if (!putCharacterString(lgd, keysPrefix)) return 0;
        if (!putCharacterString(lgd, WS_C(", "))) return 0;
      }

      if (!putKeyCombination(lgd, &binding->keys)) return 0;

      if ((binding->command & BRL_MSK_BLK) == BRL_BLK_CONTEXT) {
        const KeyContext *c = getKeyContext(lgd->keyTable, (BRL_CTX_DEFAULT + (binding->command & BRL_MSK_ARG)));

        if (c) {
          size_t length = lgd->lineLength - keysOffset;
          wchar_t keys[length + 1];

          wmemcpy(keys, &lgd->lineCharacters[keysOffset], length);
          keys[length] = 0;
          lgd->lineLength = 0;

          if (isTemporaryKeyContext(lgd->keyTable, c)) {
            if (!listKeyContext(lgd, c, keys)) return 0;
          } else {
            if (!putCharacterString(lgd, WS_C("switch to "))) return 0;
            if (!putCharacterString(lgd, c->name)) return 0;
            if (!putCharacterString(lgd, WS_C(": "))) return 0;
            if (!putCharacterString(lgd, keys)) return 0;
            if (!endLine(lgd)) return 0;
          }
        }
      } else {
        if (!endLine(lgd)) return 0;
      }

      binding += 1, count -= 1;
    }
  }

  return 1;
}

static int
doListKeyTable (ListGenerationData *lgd) {
  if (!putCharacterString(lgd, WS_C("Key Table"))) return 0;
  if (lgd->keyTable->title) {
    if (!putCharacterString(lgd, WS_C(" for "))) return 0;
    if (!putCharacterString(lgd, lgd->keyTable->title)) return 0;
    if (!endLine(lgd)) return 0;
  }

  {
    static const unsigned char contexts[] = {
      BRL_CTX_DEFAULT,
      BRL_CTX_MENU
    };

    const unsigned char *context = contexts;
    unsigned int count = ARRAY_COUNT(contexts);

    while (count) {
      const KeyContext *ctx = getKeyContext(lgd->keyTable, *context);

      if (ctx) {
        lgd->sectionTitle = ctx->name;
        if (!listKeyContext(lgd, ctx, NULL)) return 0;
      }

      context += 1, count -= 1;
    }
  }

  {
    unsigned int context;

    for (context=BRL_CTX_DEFAULT+1; context<lgd->keyTable->keyContextCount; context+=1) {
      const KeyContext *ctx = getKeyContext(lgd->keyTable, context);

      if (ctx && !isTemporaryKeyContext(lgd->keyTable, ctx)) {
        lgd->sectionTitle = ctx->name;
        if (!listKeyContext(lgd, ctx, NULL)) return 0;
      }
    }
  }

  return 1;
}

int
listKeyTable (KeyTable *table, KeyTableListHandler handleLine, void *data) {
  ListGenerationData lgd = {
    .keyTable = table,
    .sectionTitle = NULL,

    .lineCharacters = NULL,
    .lineSize = 0,
    .lineLength = 0,

    .handleLine = handleLine,
    .handlerData = data
  };

  int result = doListKeyTable(&lgd);
  if (lgd.lineCharacters) free(lgd.lineCharacters);
  return result;
}
