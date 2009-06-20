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

typedef struct {
  const wchar_t *sectionTitle;

  wchar_t *lineCharacters;
  size_t lineSize;
  size_t lineLength;

  KeyTableListHandler *handleLine;
  void *handlerData;
} ListGenerationData;

static int
handleLine (const wchar_t *line, ListGenerationData *lgd) {
  return lgd->handleLine(line, lgd->handlerData);
}

static int
addCharacters (const wchar_t *characters, size_t count, ListGenerationData *lgd) {
  size_t newLength = lgd->lineLength + count;

  if (lgd->sectionTitle) {
    if (!handleLine(WS_C(""), lgd)) return 0;
    if (!handleLine(lgd->sectionTitle, lgd)) return 0;
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
addCharacter (wchar_t character, ListGenerationData *lgd) {
  return addCharacters(&character, 1, lgd);
}

static int
addCharacterString (const wchar_t *string, ListGenerationData *lgd) {
  return addCharacters(string, wcslen(string), lgd);
}

static int
addUtf8String (const char *string, ListGenerationData *lgd) {
  size_t size = strlen(string) + 1;
  wchar_t characters[size];
  wchar_t *character = characters;

  convertStringToWchars(&string, &character, size);
  return addCharacters(characters, character-characters, lgd);
}

static int
endLine (ListGenerationData *lgd) {
  if (!addCharacter(0, lgd)) return 0;
  if (!handleLine(lgd->lineCharacters, lgd)) return 0;

  lgd->lineLength = 0;
  return 1;
}

static int
addKeyName (KeyTable *table, unsigned char set, unsigned char key, ListGenerationData *lgd) {
  int first = 0;
  int last = table->keyNameCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    const KeyNameEntry *kne = table->keyNameTable[current];

    if (set < kne->set) goto searchBelow;
    if (set > kne->set) goto searchAbove;
    if (key == kne->key) return addUtf8String(kne->name, lgd);

    if (key < kne->key) {
    searchBelow:
      last = current - 1;
    } else {
    searchAbove:
      first = current + 1;
    }
  }

  return addCharacter(WC_C('?'), lgd);
}

static int
addKeyCombination (KeyTable *table, const KeyCombination *keys, ListGenerationData *lgd) {
  wchar_t delimiter = 0;

  {
    int key;
    for (key=0; key<BITMASK_SIZE(keys->modifiers); key+=1) {
      if (BITMASK_TEST(keys->modifiers, key)) {
        if (!delimiter) {
          delimiter = WC_C('+');
        } else if (!addCharacter(delimiter, lgd)) {
          return 0;
        }

        if (!addKeyName(table, 0, key, lgd)) return 0;
      }
    }
  }

  if (keys->set || keys->key) {
    if (delimiter)
      if (!addCharacter(delimiter, lgd))
        return 0;

    if (!addKeyName(table, keys->set, keys->key, lgd)) return 0;
  }

  return 1;
}

static int
listKeyContext (KeyTable *table, const KeyContext *ctx, const wchar_t *keysPrefix, ListGenerationData *lgd) {
  {
    unsigned char function;

    for (function=0; function<InputFunctionCount; function+=1) {
      unsigned char key = ctx->inputKeys[function];

      if (key) {
        const InputFunctionEntry *ifn = &inputFunctionTable[function];

        if (!addCharacterString(WS_C("input "), lgd)) return 0;
        if (!addUtf8String(ifn->name, lgd)) return 0;
        if (!addCharacterString(WS_C(": "), lgd)) return 0;

        if (key == BRL_MSK_ARG) {
          if (!addCharacterString(WS_C("On"), lgd)) return 0;
        } else {
          if (!addKeyName(table, 0, key, lgd)) return 0;
        }

        if (!endLine(lgd)) return 0;
      }
    }
  }

  {
    const KeyBinding *binding = ctx->keyBindingTable;
    unsigned int count = ctx->keyBindingCount;

    while (count) {
      size_t keysOffset;

      {
        char description[0X60];

        describeCommand(binding->command, description, sizeof(description), 0);
        if (!addUtf8String(description, lgd)) return 0;
        if (!addCharacterString(WS_C(": "), lgd)) return 0;
      }

      keysOffset = lgd->lineLength;
      if (keysPrefix) {
        if (!addCharacterString(keysPrefix, lgd)) return 0;
        if (!addCharacterString(WS_C(", "), lgd)) return 0;
      }

      if (!addKeyCombination(table, &binding->keys, lgd)) return 0;

      if ((binding->command & BRL_MSK_BLK) == BRL_BLK_CONTEXT) {
        const KeyContext *c = getKeyContext(table, (BRL_CTX_DEFAULT + (binding->command & BRL_MSK_ARG)));

        if (c) {
          size_t length = lgd->lineLength - keysOffset;
          wchar_t keys[length + 1];

          wmemcpy(keys, &lgd->lineCharacters[keysOffset], length);
          keys[length] = 0;
          lgd->lineLength = 0;

          if (isTemporaryKeyContext(table, c)) {
            if (!listKeyContext(table, c, keys, lgd)) return 0;
          } else {
            if (!addCharacterString(WS_C("switch to "), lgd)) return 0;
            if (!addCharacterString(c->name, lgd)) return 0;
            if (!addCharacterString(WS_C(": "), lgd)) return 0;
            if (!addCharacterString(keys, lgd)) return 0;
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
listKeyContexts (KeyTable *table, ListGenerationData *lgd) {
  if (!addCharacterString(WS_C("Key Table"), lgd)) return 0;
  if (table->title) {
    if (!addCharacterString(WS_C(" for "), lgd)) return 0;
    if (!addCharacterString(table->title, lgd)) return 0;
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
      const KeyContext *ctx = getKeyContext(table, *context);

      if (ctx) {
        lgd->sectionTitle = ctx->name;
        if (!listKeyContext(table, ctx, NULL, lgd)) return 0;
      }

      context += 1, count -= 1;
    }
  }

  {
    unsigned int context;

    for (context=BRL_CTX_DEFAULT+1; context<table->keyContextCount; context+=1) {
      const KeyContext *ctx = getKeyContext(table, context);

      if (ctx && !isTemporaryKeyContext(table, ctx)) {
        lgd->sectionTitle = ctx->name;
        if (!listKeyContext(table, ctx, NULL, lgd)) return 0;
      }
    }
  }

  return 1;
}

int
listKeyTable (KeyTable *table, KeyTableListHandler handleLine, void *data) {
  ListGenerationData lgd = {
    .sectionTitle = NULL,
    .lineCharacters = NULL,
    .lineSize = 0,
    .lineLength = 0,
    .handleLine = handleLine,
    .handlerData = data
  };
  int ok = listKeyContexts(table, &lgd);

  if (lgd.lineCharacters) free(lgd.lineCharacters);
  return ok;
}
