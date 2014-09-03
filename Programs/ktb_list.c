/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include "brl_cmds.h"
#include "ktb.h"
#include "ktb_internal.h"
#include "ktb_inspect.h"

typedef struct {
  KeyTable *const keyTable;
  const wchar_t *sectionTitle;

  struct {
    wchar_t *characters;
    size_t size;
    size_t length;
  } line;

  struct {
    KeyTableListHandler *const handler;
    void *const data;
  } list;
} ListGenerationData;

static int
listLine (ListGenerationData *lgd, const wchar_t *line) {
  return lgd->list.handler(line, lgd->list.data);
}

static int
putCharacters (ListGenerationData *lgd, const wchar_t *characters, size_t count) {
  size_t newLength = lgd->line.length + count;

  if (lgd->sectionTitle) {
    if (!listLine(lgd, WS_C(""))) return 0;
    if (!listLine(lgd, lgd->sectionTitle)) return 0;
    lgd->sectionTitle = NULL;
  }

  if (newLength > lgd->line.size) {
    size_t newSize = (newLength | 0X3F) + 1;
    wchar_t *newCharacters = realloc(lgd->line.characters, ARRAY_SIZE(newCharacters, newSize));

    if (!newCharacters) {
      logSystemError("realloc");
      return 0;
    }

    lgd->line.characters = newCharacters;
    lgd->line.size = newSize;
  }

  wmemcpy(&lgd->line.characters[lgd->line.length], characters, count);
  lgd->line.length = newLength;
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
searchKeyNameEntry (const void *target, const void *element) {
  const KeyValue *value = target;
  const KeyNameEntry *const *kne = element;
  return compareKeyValues(value, &(*kne)->value);
}

static const KeyNameEntry *
findKeyNameEntry (KeyTable *table, const KeyValue *value) {
  const KeyNameEntry *const *array = table->keyNames.table;
  unsigned int count = table->keyNames.count;

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

size_t
formatKeyName (KeyTable *table, char *buffer, size_t size, const KeyValue *value) {
  const KeyNameEntry *kne = findKeyNameEntry(table, value);

  size_t length;
  STR_BEGIN(buffer, size);

  if (kne) {
    STR_PRINTF("%s", kne->name);
  } else if (value->number != KTB_KEY_ANY) {
    const KeyValue anyKey = {
      .group = value->group,
      .number = KTB_KEY_ANY
    };

    if ((kne = findKeyNameEntry(table, &anyKey))) {
      STR_PRINTF("%s.%u", kne->name, value->number+1);
    }
  }

  if (STR_LENGTH == 0) STR_PRINTF("?");
  length = STR_LENGTH;
  STR_END;
  return length;
}

static int
putKeyName (ListGenerationData *lgd, const KeyValue *value) {
  char name[0X100];

  formatKeyName(lgd->keyTable, name, sizeof(name), value);
  return putUtf8String(lgd, name);
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
putCommandDescription (ListGenerationData *lgd, const BoundCommand *cmd, int details) {
  char description[0X60];

  describeCommand(cmd->value, description, sizeof(description),
                  details? (CDO_IncludeOperand | CDO_DefaultOperand): 0);
  return putUtf8String(lgd, description);
}

static int
endLine (ListGenerationData *lgd) {
  if (!putCharacter(lgd, 0)) return 0;
  if (!listLine(lgd, lgd->line.characters)) return 0;

  lgd->line.length = 0;
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
listHotkeyEvent (ListGenerationData *lgd, const KeyValue *keyValue, const char *event, const BoundCommand *cmd) {
  if (cmd->value != BRL_CMD_NOOP) {
    if ((cmd->value & BRL_MSK_BLK) == BRL_CMD_BLK(CONTEXT)) {
      const KeyContext *c = getKeyContext(lgd->keyTable, (KTB_CTX_DEFAULT + (cmd->value & BRL_MSK_ARG)));
      if (!c) return 0;
      if (!putUtf8String(lgd, "switch to ")) return 0;
      if (!putCharacterString(lgd, c->title)) return 0;
    } else {
      if (!putCommandDescription(lgd, cmd, (keyValue->number != KTB_KEY_ANY))) return 0;
    }

    if (!putCharacterString(lgd, WS_C(": "))) return 0;
    if (!putUtf8String(lgd, event)) return 0;
    if (!putCharacter(lgd, WC_C(' '))) return 0;
    if (!putKeyName(lgd, keyValue)) return 0;
    if (!endLine(lgd)) return 0;
  }

  return 1;
}

static int listKeyContext (ListGenerationData *lgd, const KeyContext *ctx, const wchar_t *keysPrefix);

static int
listKeyBinding (ListGenerationData *lgd, const KeyBinding *binding, int longPress, const wchar_t *keysPrefix) {
  const BoundCommand *cmd = longPress? &binding->secondaryCommand: &binding->primaryCommand;
  size_t keysOffset;

  if (cmd->value == BRL_CMD_NOOP) return 1;

  if (!putCommandDescription(lgd, cmd, !binding->keyCombination.anyKeyCount)) return 0;
  if (!putCharacterString(lgd, WS_C(": "))) return 0;
  keysOffset = lgd->line.length;

  if (keysPrefix) {
    if (!putCharacterString(lgd, keysPrefix)) return 0;
    if (!putCharacterString(lgd, WS_C(", "))) return 0;
  }

  if (longPress) {
    if (!putCharacterString(lgd, WS_C("long "))) return 0;
  }

  if (!putKeyCombination(lgd, &binding->keyCombination)) return 0;

  if ((cmd->value & BRL_MSK_BLK) == BRL_CMD_BLK(CONTEXT)) {
    const KeyContext *c = getKeyContext(lgd->keyTable, (KTB_CTX_DEFAULT + (cmd->value & BRL_MSK_ARG)));
    if (!c) return 0;

    {
      size_t length = lgd->line.length - keysOffset;
      wchar_t keys[length + 1];

      wmemcpy(keys, &lgd->line.characters[keysOffset], length);
      keys[length] = 0;
      lgd->line.length = 0;

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

  return 1;
}

static int
listKeyContext (ListGenerationData *lgd, const KeyContext *ctx, const wchar_t *keysPrefix) {
  {
    const HotkeyEntry *hotkey = ctx->hotkeys.table;
    unsigned int count = ctx->hotkeys.count;

    while (count) {
      if (!(hotkey->flags & HKF_HIDDEN)) {
        if (!listHotkeyEvent(lgd, &hotkey->keyValue, "press", &hotkey->pressCommand)) return 0;
        if (!listHotkeyEvent(lgd, &hotkey->keyValue, "release", &hotkey->releaseCommand)) return 0;
      }

      hotkey += 1, count -= 1;
    }
  }

  {
    const KeyBinding *binding = ctx->keyBindings.table;
    unsigned int count = ctx->keyBindings.count;

    while (count) {
      if (!(binding->flags & KBF_HIDDEN)) {
        if (!listKeyBinding(lgd, binding, 0, keysPrefix)) return 0;
        if (!listKeyBinding(lgd, binding, 1, keysPrefix)) return 0;
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

  if (lgd->keyTable->notes.count) {
    unsigned int noteIndex;

    if (!endLine(lgd)) return 0;

    for (noteIndex=0; noteIndex<lgd->keyTable->notes.count; noteIndex+=1) {
      if (!putCharacterString(lgd, lgd->keyTable->notes.table[noteIndex])) return 0;
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

    for (context=KTB_CTX_DEFAULT+1; context<lgd->keyTable->keyContexts.count; context+=1) {
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
listKeyTable (KeyTable *table, KeyTableListHandler *handleLine, void *data) {
  ListGenerationData lgd = {
    .keyTable = table,
    .sectionTitle = NULL,

    .line = {
      .characters = NULL,
      .size = 0,
      .length = 0,
    },

    .list = {
      .handler = handleLine,
      .data = data
    }
  };

  int result = doListKeyTable(&lgd);
  if (lgd.line.characters) free(lgd.line.characters);
  return result;
}

int
forEachKeyName (KEY_NAME_TABLES_REFERENCE keys, KeyNameEntryHandler *handleKeyNameEntry, void *data) {
  const KeyNameEntry *const *knt = keys;

  while (*knt) {
    const KeyNameEntry *kne = *knt;

    if (knt != keys) {
      if (!handleKeyNameEntry(NULL, data)) {
        return 0;
      }
    }

    while (kne->name) {
      if (!handleKeyNameEntry(kne, data)) return 0;
      kne += 1;
    }

    knt += 1;
  }

  return 1;
}

typedef struct {
  KeyTableListHandler *const handleLine;
  void *const data;
} ListKeyNameData;

static int
listKeyName (const KeyNameEntry *kne, void *data) {
  const ListKeyNameData *lkn = data;
  const char *name = kne? kne->name: "";
  size_t size = strlen(name) + 1;
  wchar_t characters[size];
  wchar_t *character = characters;

  convertUtf8ToWchars(&name, &character, size);
  return lkn->handleLine(characters, lkn->data);
}

int
listKeyNames (KEY_NAME_TABLES_REFERENCE keys, KeyTableListHandler *handleLine, void *data) {
  ListKeyNameData lkn = {
    .handleLine = handleLine,
    .data = data
  };

  return forEachKeyName(keys, listKeyName, &lkn);
}
