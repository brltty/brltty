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

#include <string.h>

#include "log.h"
#include "charset.h"
#include "cmd.h"
#include "ktb.h"
#include "ktb_internal.h"
#include "ktb_inspect.h"

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
    unsigned char index;
    for (index=0; index<keys->modifiers.count; index+=1) {
      if (!delimiter) {
        delimiter = WC_C('+');
      } else if (!putCharacter(lgd, delimiter)) {
        return 0;
      }

      if (!putKeyName(lgd, 0, keys->modifiers.keys[index])) return 0;
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
  {
    const HotkeyEntry *hotkey = ctx->hotkeyTable;
    unsigned int count = ctx->hotkeyCount;

    while (count) {
      if (hotkey->press != BRL_CMD_NOOP) {
        if (!putCommandDescription(lgd, hotkey->press)) return 0;
        if (!putCharacterString(lgd, WS_C(": press "))) return 0;
        if (!putKeyName(lgd, hotkey->set, hotkey->key)) return 0;
        if (!endLine(lgd)) return 0;
      }

      if (hotkey->release != BRL_CMD_NOOP) {
        if (!putCommandDescription(lgd, hotkey->release)) return 0;
        if (!putCharacterString(lgd, WS_C(": release "))) return 0;
        if (!putKeyName(lgd, hotkey->set, hotkey->key)) return 0;
        if (!endLine(lgd)) return 0;
      }

      hotkey += 1, count -= 1;
    }
  }

  {
    const KeyBinding *binding = ctx->keyBindingTable;
    unsigned int count = ctx->keyBindingCount;

    while (count) {
      if (!binding->hidden) {
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
      }

      binding += 1, count -= 1;
    }
  }

  if (!listKeyboardFunctions(lgd, ctx)) return 0;
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

  if (lgd->keyTable->noteCount) {
    unsigned int noteIndex;

    if (!endLine(lgd)) return 0;

    for (noteIndex=0; noteIndex<lgd->keyTable->noteCount; noteIndex+=1) {
      if (!putCharacterString(lgd, lgd->keyTable->noteTable[noteIndex])) return 0;
      if (!endLine(lgd)) return 0;
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
