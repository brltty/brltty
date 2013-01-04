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
      logSystemError("realloc");
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

  convertUtf8ToWchars(&string, &character, size);
  return putCharacters(lgd, characters, character-characters);
}

static int
putNumber (ListGenerationData *lgd, int number) {
  char buffer[0X10];

  snprintf(buffer, sizeof(buffer), "%d", number);
  return putUtf8String(lgd, buffer);
}

static int
searchKeyNameEntry (const void *target, const void *element) {
  const KeyValue *value = target;
  const KeyNameEntry *const *kne = element;
  return compareKeyValues(value, &(*kne)->value);
}

static const KeyNameEntry *
findKeyNameEntry (ListGenerationData *lgd, const KeyValue *value) {
  const KeyNameEntry *const *array = lgd->keyTable->keyNameTable;
  unsigned int count = lgd->keyTable->keyNameCount;

  const KeyNameEntry *const *kne = bsearch(value, array, count, sizeof(*array), searchKeyNameEntry);
  if (!kne) return NULL;

  while (kne > array) {
    if (compareKeyValues(value, &(*--kne)->value) != 0) {
      kne += 1;
      break;
    }
  }

  return *kne;
}

static int
putKeyName (ListGenerationData *lgd, const KeyValue *value) {
  const KeyNameEntry *kne = findKeyNameEntry(lgd, value);
  if (kne) return putUtf8String(lgd, kne->name);

  if (value->key != KTB_KEY_ANY) {
    const KeyValue anyKey = {
      .set = value->set,
      .key = KTB_KEY_ANY
    };

    if ((kne = findKeyNameEntry(lgd, &anyKey))) {
      if (!putUtf8String(lgd, kne->name)) return 0;
      if (!putCharacter(lgd, WC_C('.'))) return 0;
      if (!putNumber(lgd, value->key+1)) return 0;
      return 1;
    }
  }

  return putUtf8String(lgd, "?");
}

static int
putKeyCombination (ListGenerationData *lgd, const KeyCombination *combination) {
  wchar_t delimiter = 0;

  {
    unsigned char index;
    for (index=0; index<combination->modifierCount; index+=1) {
      if (!delimiter) {
        delimiter = WC_C('+');
      } else if (!putCharacter(lgd, delimiter)) {
        return 0;
      }

      if (!putKeyName(lgd, &combination->modifierKeys[combination->modifierPositions[index]])) return 0;
    }
  }

  if (combination->flags & KCF_IMMEDIATE_KEY) {
    if (delimiter)
      if (!putCharacter(lgd, delimiter))
        return 0;

    if (!putKeyName(lgd, &combination->immediateKey)) return 0;
  }

  return 1;
}

static int
putCommandDescription (ListGenerationData *lgd, int command, int details) {
  char description[0X60];

  describeCommand(command, description, sizeof(description),
                  details? (CDO_IncludeOperand | CDO_DefaultOperand): 0);
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
  const char *prefix = "braille keyboard ";

  {
    unsigned int index;

    for (index=0; index<ctx->mappedKeys.count; index+=1) {
      const MappedKeyEntry *map = &ctx->mappedKeys.table[index];
      const KeyboardFunction *kbf = map->keyboardFunction;

      if (!putUtf8String(lgd, prefix)) return 0;
      if (!putUtf8String(lgd, kbf->name)) return 0;
      if (!putCharacterString(lgd, WS_C(": "))) return 0;
      if (!putKeyName(lgd, &map->keyValue)) return 0;
      if (!endLine(lgd)) return 0;
    }
  }

  {
    const KeyboardFunction *kbf = keyboardFunctionTable;
    const KeyboardFunction *end = kbf + keyboardFunctionCount;

    while (kbf < end) {
      if (ctx->mappedKeys.superimpose & kbf->bit) {
        if (!putUtf8String(lgd, prefix)) return 0;
        if (!putUtf8String(lgd, kbf->name)) return 0;
        if (!putCharacterString(lgd, WS_C(": superimposed"))) return 0;
        if (!endLine(lgd)) return 0;
      }

      kbf += 1;
    }
  }

  return 1;
}

static int
listHotkeyEvent (ListGenerationData *lgd, const KeyValue *keyValue, const char *event, int command) {
  if (command != BRL_CMD_NOOP) {
    if ((command & BRL_MSK_BLK) == BRL_BLK_CONTEXT) {
      const KeyContext *c = getKeyContext(lgd->keyTable, (KTB_CTX_DEFAULT + (command & BRL_MSK_ARG)));
      if (!c) return 0;
      if (!putUtf8String(lgd, "switch to ")) return 0;
      if (!putCharacterString(lgd, c->title)) return 0;
    } else {
      if (!putCommandDescription(lgd, command, (keyValue->key != KTB_KEY_ANY))) return 0;
    }

    if (!putCharacterString(lgd, WS_C(": "))) return 0;
    if (!putUtf8String(lgd, event)) return 0;
    if (!putCharacter(lgd, WC_C(' '))) return 0;
    if (!putKeyName(lgd, keyValue)) return 0;
    if (!endLine(lgd)) return 0;
  }

  return 1;
}

static int
listKeyContext (ListGenerationData *lgd, const KeyContext *ctx, const wchar_t *keysPrefix) {
  {
    const HotkeyEntry *hotkey = ctx->hotkeys.table;
    unsigned int count = ctx->hotkeys.count;

    while (count) {
      if (!(hotkey->flags & HKF_HIDDEN)) {
        if (!listHotkeyEvent(lgd, &hotkey->keyValue, "press", hotkey->pressCommand)) return 0;
        if (!listHotkeyEvent(lgd, &hotkey->keyValue, "release", hotkey->releaseCommand)) return 0;
      }

      hotkey += 1, count -= 1;
    }
  }

  {
    const KeyBinding *binding = ctx->keyBindings.table;
    unsigned int count = ctx->keyBindings.count;

    while (count) {
      if (!(binding->flags & KBF_HIDDEN)) {
        size_t keysOffset;

        if (!putCommandDescription(lgd, binding->command, !binding->combination.anyKeyCount)) return 0;
        if (!putCharacterString(lgd, WS_C(": "))) return 0;
        keysOffset = lgd->lineLength;

        if (keysPrefix) {
          if (!putCharacterString(lgd, keysPrefix)) return 0;
          if (!putCharacterString(lgd, WS_C(", "))) return 0;
        }

        if (!putKeyCombination(lgd, &binding->combination)) return 0;

        if ((binding->command & BRL_MSK_BLK) == BRL_BLK_CONTEXT) {
          const KeyContext *c = getKeyContext(lgd->keyTable, (KTB_CTX_DEFAULT + (binding->command & BRL_MSK_ARG)));
          if (!c) return 0;

          {
            size_t length = lgd->lineLength - keysOffset;
            wchar_t keys[length + 1];

            wmemcpy(keys, &lgd->lineCharacters[keysOffset], length);
            keys[length] = 0;
            lgd->lineLength = 0;

            if (isTemporaryKeyContext(lgd->keyTable, c)) {
              if (!listKeyContext(lgd, c, keys)) return 0;
            } else {
              if (!putCharacterString(lgd, WS_C("switch to "))) return 0;
              if (!putCharacterString(lgd, c->title)) return 0;
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
  if (!putUtf8String(lgd, gettext("Key Table"))) return 0;
  if (lgd->keyTable->title) {
    if (!putCharacterString(lgd, WS_C(": "))) return 0;
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
      KTB_CTX_DEFAULT,
      KTB_CTX_MENU
    };

    const unsigned char *context = contexts;
    unsigned int count = ARRAY_COUNT(contexts);

    while (count) {
      const KeyContext *ctx = getKeyContext(lgd->keyTable, *context);

      if (ctx) {
        lgd->sectionTitle = ctx->title;
        if (!listKeyContext(lgd, ctx, NULL)) return 0;
      }

      context += 1, count -= 1;
    }
  }

  {
    unsigned int context;

    for (context=KTB_CTX_DEFAULT+1; context<lgd->keyTable->keyContextCount; context+=1) {
      const KeyContext *ctx = getKeyContext(lgd->keyTable, context);

      if (ctx && !isTemporaryKeyContext(lgd->keyTable, ctx)) {
        lgd->sectionTitle = ctx->title;
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

int
listKeyNames (KEY_NAME_TABLES_REFERENCE keys, KeyTableListHandler handleLine, void *data) {
  const KeyNameEntry *const *knt = keys;

  while (*knt) {
    const KeyNameEntry *kne = *knt;

    if (knt != keys)
      if (!handleLine(WS_C(""), data))
        return 0;

    while (kne->name) {
      const char *string = kne->name;
      size_t size = strlen(string) + 1;
      wchar_t characters[size];

      {
        wchar_t *character = characters;
        convertUtf8ToWchars(&string, &character, size);
      }

      if (!handleLine(characters, data)) return 0;
      kne += 1;
    }

    knt += 1;
  }

  return 1;
}
