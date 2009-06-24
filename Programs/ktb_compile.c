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
#include <ctype.h>

#include "misc.h"
#include "datafile.h"
#include "cmd.h"
#include "brldefs.h"
#include "ktb.h"
#include "ktb_internal.h"

const KeyboardFunctionEntry keyboardFunctionTable[KBF_None] = {
  [KBF_Dot1] = {.name="dot1", .bit=BRL_DOT1},
  [KBF_Dot2] = {.name="dot2", .bit=BRL_DOT2},
  [KBF_Dot3] = {.name="dot3", .bit=BRL_DOT3},
  [KBF_Dot4] = {.name="dot4", .bit=BRL_DOT4},
  [KBF_Dot5] = {.name="dot5", .bit=BRL_DOT5},
  [KBF_Dot6] = {.name="dot6", .bit=BRL_DOT6},
  [KBF_Dot7] = {.name="dot7", .bit=BRL_DOT7},
  [KBF_Dot8] = {.name="dot8", .bit=BRL_DOT8},

  [KBF_Space] = {.name="space", .bit=0},
  [KBF_Shift] = {.name="shift", .bit=BRL_FLG_CHAR_SHIFT},
  [KBF_Uppercase] = {.name="uppercase", .bit=BRL_FLG_CHAR_UPPER},
  [KBF_Control] = {.name="control", .bit=BRL_FLG_CHAR_CONTROL},
  [KBF_Meta] = {.name="meta", .bit=BRL_FLG_CHAR_META}
};

typedef struct {
  wchar_t *title;

  const KeyNameEntry **keyNameTable;
  unsigned int keyNameCount;

  const CommandEntry **commandTable;
  unsigned int commandCount;

  KeyContext *keyContextTable;
  unsigned int keyContextCount;

  unsigned char context;
} KeyTableData;

static KeyContext *
getKeyContext (KeyTableData *ktd, unsigned char context) {
  if (context >= ktd->keyContextCount) {
    unsigned int newCount = context + 1;
    KeyContext *newTable = realloc(ktd->keyContextTable, ARRAY_SIZE(newTable, newCount));

    if (!newTable) {
      LogError("realloc");
      return NULL;
    }
    ktd->keyContextTable = newTable;

    while (ktd->keyContextCount < newCount) {
      KeyContext *ctx = &ktd->keyContextTable[ktd->keyContextCount++];
      memset(ctx, 0, sizeof(*ctx));

      ctx->name = NULL;

      ctx->keyBindingTable = NULL;
      ctx->keyBindingsSize = 0;
      ctx->keyBindingCount = 0;
      ctx->sortedKeyBindings = NULL;

      ctx->keyMap = NULL;
      ctx->superimposedBits = 0;
    }
  }

  return &ktd->keyContextTable[context];
}

static inline KeyContext *
getCurrentKeyContext (KeyTableData *ktd) {
  return getKeyContext(ktd, ktd->context);
}

static int
setKeyContextName (KeyContext *ctx, const wchar_t *name, size_t length) {
  if (ctx->name) free(ctx->name);

  if (!(ctx->name = malloc(ARRAY_SIZE(ctx->name, length+1)))) {
    LogError("malloc");
    return 0;
  }

  wmemcpy(ctx->name, name, length);
  ctx->name[length] = 0;
  return 1;
}

static int
setDefaultKeyContextProperties (KeyTableData *ktd) {
  typedef struct {
    unsigned char context;
    const wchar_t *name;
  } PropertiesEntry;

  static const PropertiesEntry propertiesTable[] = {
    { .context=BRL_CTX_DEFAULT,
      .name = WS_C("Default Bindings")
    }
    ,
    { .context=BRL_CTX_MENU,
      .name = WS_C("Menu Bindings")
    }
    ,
    { .name = NULL }
  };
  const PropertiesEntry *properties = propertiesTable;

  while (properties->name) {
    KeyContext *ctx = getKeyContext(ktd, properties->context);
    if (!ctx) return 0;

    if (!ctx->name)
      if (!setKeyContextName(ctx, properties->name, wcslen(properties->name)))
        return 0;

    properties += 1;
  }

  return 1;
}

static void
destroyKeyContextTable (KeyContext *table, unsigned int count) {
  while (count) {
    KeyContext *ctx = &table[--count];

    if (ctx->name) free(ctx->name);
    if (ctx->keyBindingTable) free(ctx->keyBindingTable);
    if (ctx->sortedKeyBindings) free(ctx->sortedKeyBindings);
    if (ctx->keyMap) free(ctx->keyMap);
  }

  if (table) free(table);
}

static int
compareToName (const wchar_t *location1, int length1, const char *location2) {
  const wchar_t *end1 = location1 + length1;

  while (1) {
    if (location1 == end1) return *location2? -1: 0;
    if (!*location2) return 1;

    {
      wchar_t character1 = towlower(*location1);
      char character2 = tolower(*location2);

      if (character1 < character2) return -1;
      if (character1 > character2) return 1;
    }

    location1 += 1;
    location2 += 1;
  }
}

static int
sortKeyNames (const void *element1, const void *element2) {
  const KeyNameEntry *const *kne1 = element1;
  const KeyNameEntry *const *kne2 = element2;
  return strcasecmp((*kne1)->name, (*kne2)->name);
}

static int
searchKeyName (const void *target, const void *element) {
  const DataOperand *name = target;
  const KeyNameEntry *const *kne = element;
  return compareToName(name->characters, name->length, (*kne)->name);
}

static int
sortKeyValues (const void *element1, const void *element2) {
  const KeyNameEntry *const *kne1 = element1;
  const KeyNameEntry *const *kne2 = element2;

  if ((*kne1)->set < (*kne2)->set) return -1;
  if ((*kne1)->set > (*kne2)->set) return 1;

  if ((*kne1)->key < (*kne2)->key) return -1;
  if ((*kne1)->key > (*kne2)->key) return 1;

  return 0;
}

static int
allocateKeyNameTable (KeyTableData *ktd, const KeyNameEntry *const *keys) {
  {
    const KeyNameEntry *const *knt = keys;

    ktd->keyNameCount = 0;
    while (*knt) {
      const KeyNameEntry *kne = *knt;
      while (kne->name) kne += 1;
      ktd->keyNameCount += kne - *knt;
      knt += 1;
    }
  }

  if ((ktd->keyNameTable = malloc(ARRAY_SIZE(ktd->keyNameTable, ktd->keyNameCount)))) {
    {
      const KeyNameEntry **address = ktd->keyNameTable;
      const KeyNameEntry *const *knt = keys;

      while (*knt) {
        const KeyNameEntry *kne = *knt++;
        while (kne->name)  *address++ = kne++;
      }
    }

    qsort(ktd->keyNameTable, ktd->keyNameCount, sizeof(*ktd->keyNameTable), sortKeyNames);
    return 1;
  }

  return 0;
}

static int
parseKeyName (DataFile *file, unsigned char *set, unsigned char *key, const wchar_t *characters, int length, KeyTableData *ktd) {
  const DataOperand name = {
    .characters = characters,
    .length = length
  };
  const KeyNameEntry **kne = bsearch(&name, ktd->keyNameTable, ktd->keyNameCount, sizeof(*ktd->keyNameTable), searchKeyName);

  if (kne) {
    *set = (*kne)->set;
    *key = (*kne)->key;
    return 1;
  }

  reportDataError(file, "unknown key: %.*" PRIws, length, characters);
  return 0;
}

static int
parseKeyCombination (DataFile *file, KeyCombination *keys, const wchar_t *characters, int length, KeyTableData *ktd) {
  int immediate;

  while (1) {
    const wchar_t *end = wmemchr(characters, WC_C('+'), length);
    if (!end) break;

    {
      int count = end - characters;
      unsigned char set;
      unsigned char key;

      if (!count) {
        reportDataError(file, "missing modifier key");
        return 0;
      }
      if (!parseKeyName(file, &set, &key, characters, count, ktd)) return 0;

      if (set) {
        reportDataError(file, "unexpected modifier key: %.*" PRIws, count, characters);
        return 0;
      }

      if (BITMASK_TEST(keys->modifiers, key)) {
        reportDataError(file, "duplicate modifier key: %.*" PRIws, count, characters);
        return 0;
      }
      BITMASK_SET(keys->modifiers, key);

      length -= count + 1;
      characters = end + 1;
    }
  }

  immediate = 0;
  if (length) {
    if (*characters == WC_C('!')) {
      characters += 1, length -= 1;
      immediate = 1;
    }
  }

  if (!length) {
    reportDataError(file, "missing key");
    return 0;
  }
  if (!parseKeyName(file, &keys->set, &keys->key, characters, length, ktd)) return 0;

  if (!keys->set) {
    if (BITMASK_TEST(keys->modifiers, keys->key)) {
      reportDataError(file, "duplicate key: %.*" PRIws, length, characters);
      return 0;
    }

    if (!immediate) {
      BITMASK_SET(keys->modifiers, keys->key);
      keys->key = 0;
    }
  }

  return 1;
}

static int
getKeysOperand (DataFile *file, KeyCombination *key, KeyTableData *ktd) {
  DataOperand names;

  if (getDataOperand(file, &names, "key combination")) {
    if (parseKeyCombination(file, key, names.characters, names.length, ktd)) return 1;
  }

  return 0;
}

static int
getMappedKeyOperand (DataFile *file, unsigned char *key, KeyTableData *ktd) {
  DataOperand name;

  if (getDataOperand(file, &name, "mapped key name")) {
    unsigned char set;

    if (isKeyword(WS_C("superimpose"), name.characters, name.length)) {
      *key = 0;
      return 1;
    }

    if (parseKeyName(file, &set, key, name.characters, name.length, ktd)) {
      if (!set) return 1;
      reportDataError(file, "invalid mapped key: %.*" PRIws, name.length, name.characters);
    }
  }

  return 0;
}

static int
sortKeyboardFunctionNames (const void *element1, const void *element2) {
  const KeyboardFunctionEntry *const *kbf1 = element1;
  const KeyboardFunctionEntry *const *kbf2 = element2;
  return strcasecmp((*kbf1)->name, (*kbf2)->name);
}

static int
searchKeyboardFunctionName (const void *target, const void *element) {
  const DataOperand *name = target;
  const KeyboardFunctionEntry *const *kbf = element;
  return compareToName(name->characters, name->length, (*kbf)->name);
}

static int
parseKeyboardFunctionName (DataFile *file, unsigned char *function, const wchar_t *characters, int length, KeyTableData *ktd) {
  static const KeyboardFunctionEntry **sortedKeyboardFunctions = NULL;

  if (!sortedKeyboardFunctions) {
    const KeyboardFunctionEntry **newTable = malloc(ARRAY_SIZE(newTable, KBF_None));

    if (!newTable) {
      LogError("malloc");
      return 0;
    }

    {
      const KeyboardFunctionEntry *source = keyboardFunctionTable;
      const KeyboardFunctionEntry **target = newTable;
      unsigned int count = KBF_None;

      do {
        *target++ = source++;
      } while (--count);

      qsort(newTable, KBF_None, sizeof(*newTable), sortKeyboardFunctionNames);
    }

    sortedKeyboardFunctions = newTable;
  }

  {
    const DataOperand name = {
      .characters = characters,
      .length = length
    };
    const KeyboardFunctionEntry **kbf = bsearch(&name, sortedKeyboardFunctions, KBF_None, sizeof(*sortedKeyboardFunctions), searchKeyboardFunctionName);

    if (kbf) {
      *function = *kbf - keyboardFunctionTable;
      return 1;
    }
  }

  reportDataError(file, "unknown keyboard function: %.*" PRIws, length, characters);
  return 0;
}

static int
getKeyboardFunctionOperand (DataFile *file, unsigned char *function, KeyTableData *ktd) {
  DataOperand name;

  if (getDataOperand(file, &name, "keyboard function name")) {
    if (parseKeyboardFunctionName(file, function, name.characters, name.length, ktd)) return 1;
  }

  return 0;
}

static int
sortCommandNames (const void *element1, const void *element2) {
  const CommandEntry *const *cmd1 = element1;
  const CommandEntry *const *cmd2 = element2;
  return strcasecmp((*cmd1)->name, (*cmd2)->name);
}

static int
searchCommandName (const void *target, const void *element) {
  const DataOperand *name = target;
  const CommandEntry *const *cmd = element;
  return compareToName(name->characters, name->length, (*cmd)->name);
}

static int
allocateCommandTable (KeyTableData *ktd) {
  {
    const CommandEntry *command = commandTable;

    ktd->commandCount = 0;
    while (command->name) {
      ktd->commandCount += 1;
      command += 1;
    }
  }

  if ((ktd->commandTable = malloc(ktd->commandCount * sizeof(*ktd->commandTable)))) {
    {
      const CommandEntry *command = commandTable;
      const CommandEntry **address = ktd->commandTable;
      while (command->name) *address++ = command++;
    }

    qsort(ktd->commandTable, ktd->commandCount, sizeof(*ktd->commandTable), sortCommandNames);
    return 1;
  }

  return 0;
}

static int
parseCommandName (DataFile *file, int *value, const wchar_t *characters, int length, KeyTableData *ktd) {
  const wchar_t *end = wmemchr(characters, WC_C('+'), length);
  const DataOperand name = {
    .characters = characters,
    .length = end? end-characters: length
  };
  const CommandEntry **command = bsearch(&name, ktd->commandTable, ktd->commandCount, sizeof(*ktd->commandTable), searchCommandName);

  if (command) {
    *value = (*command)->code;
    if (!end) return 1;

    if (!(length -= end - characters + 1)) {
      reportDataError(file, "missing command modifier");
      return 0;
    }
    characters = end + 1;

    if ((*command)->isToggle) {
      if (isKeyword(WS_C("on"), characters, length)) {
        *value |= BRL_FLG_TOGGLE_ON;
        return 1;
      }

      if (isKeyword(WS_C("off"), characters, length)) {
        *value |= BRL_FLG_TOGGLE_OFF;
        return 1;
      }
    } else if ((*command)->isBase) {
      unsigned int maximum = BRL_MSK_ARG - ((*command)->code & BRL_MSK_ARG);
      unsigned int offset = 0;
      int index;

      for (index=0; index<length; index+=1) {
        wchar_t character = characters[index];

        if ((character < WC_C('0')) || (character > WC_C('9'))) {
          reportDataError(file, "invalid command offset: %.*" PRIws, length, characters);
          return 0;
        }

        if ((offset = (offset * 10) + (character - WC_C('0'))) > maximum) {
          reportDataError(file, "command offset too large: %.*" PRIws, length, characters);
          return 0;
        }
      }

      *value += offset;
      return 1;
    }

    reportDataError(file, "unknown command modifier: %.*" PRIws, length, characters);
  } else {
    reportDataError(file, "unknown command name: %.*" PRIws, length, characters);
  }

  return 0;
}

static int
getCommandOperand (DataFile *file, int *value, KeyTableData *ktd) {
  DataOperand name;

  if (getDataOperand(file, &name, "command name")) {
    if (parseCommandName(file, value, name.characters, name.length, ktd)) return 1;
  }

  return 0;
}

static int
processBindOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  KeyContext *ctx = getCurrentKeyContext(ktd);

  if (!ctx) return 0;

  if (ctx->keyBindingCount == ctx->keyBindingsSize) {
    unsigned int newSize = ctx->keyBindingsSize? ctx->keyBindingsSize<<1: 0X10;
    KeyBinding *newTable = realloc(ctx->keyBindingTable, ARRAY_SIZE(newTable, newSize));

    if (!newTable) {
      LogError("realloc");
      return 0;
    }

    ctx->keyBindingTable = newTable;
    ctx->keyBindingsSize = newSize;
  }

  {
    KeyBinding *binding = &ctx->keyBindingTable[ctx->keyBindingCount];
    memset(binding, 0, sizeof(*binding));

    if (getKeysOperand(file, &binding->keys, ktd)) {
      if (getCommandOperand(file, &binding->command, ktd)) {
        if (!binding->keys.set)
          if (getCommandEntry(binding->command)->isCharacter)
            binding->command |= BRL_MSK_ARG;

        ctx->keyBindingCount += 1;
      }
    }
  }

  return 1;
}

static int
processContextOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  DataOperand context;

  if (getDataOperand(file, &context, "context name or number")) {
    int ok = 0;

    if (isKeyword(WS_C("default"), context.characters, context.length)) {
      ktd->context = BRL_CTX_DEFAULT;
      ok = 1;
    } else if (isKeyword(WS_C("menu"), context.characters, context.length)) {
      ktd->context = BRL_CTX_MENU;
      ok = 1;
    } else {
      int number;

      if (!isNumber(&number, context.characters, context.length)) {
        reportDataError(file, "unknown context name: %.*" PRIws, context.length, context.characters);
      } else if ((number < 0) || (number > (BRL_MSK_ARG - BRL_CTX_DEFAULT))) {
        reportDataError(file, "invalid context number: %.*" PRIws, context.length, context.characters);
      } else {
        ktd->context = BRL_CTX_DEFAULT + number;
        ok = 1;
      }
    }

    if (ok) {
      KeyContext *ctx = getCurrentKeyContext(ktd);

      if (ctx) {
        DataOperand name;

        if (getDataText(file, &name, NULL)) {
          if (ctx->name) {
            reportDataError(file, "context name specified more than once");
          } else if (!setKeyContextName(ctx, name.characters, name.length)) {
            return 0;
          }
        }
      }
    }
  }

  return 1;
}

static int
processMapOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  KeyContext *ctx = getCurrentKeyContext(ktd);
  if (!ctx) return 0;

  {
    unsigned char key;

    if (getMappedKeyOperand(file, &key, ktd)) {
      unsigned char function;

      if (getKeyboardFunctionOperand(file, &function, ktd)) {
        if (!key) {
          ctx->superimposedBits |= keyboardFunctionTable[function].bit;
          return 1;
        }

        if (!ctx->keyMap) {
          if (!(ctx->keyMap = malloc(ARRAY_SIZE(ctx->keyMap, KEYS_PER_SET)))) {
            LogError("malloc");
            return 0;
          }

          memset(ctx->keyMap, KBF_None, KEYS_PER_SET);
        }

        ctx->keyMap[key] = function;
        return 1;
      }
    }
  }

  return 1;
}

static int
processTitleOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  DataOperand title;

  if (getDataText(file, &title, "title text")) {
    if (ktd->title) {
      reportDataError(file, "table title specified more than once");
    } else if (!(ktd->title = malloc(ARRAY_SIZE(ktd->title, title.length+1)))) {
      LogError("malloc");
    } else {
      wmemcpy(ktd->title, title.characters, title.length);
      ktd->title[title.length] = 0;
      return 1;
    }
  }

  return 1;
}

static int
processKeyTableLine (DataFile *file, void *data) {
  static const DataProperty properties[] = {
    {.name=WS_C("bind"), .processor=processBindOperands},
    {.name=WS_C("context"), .processor=processContextOperands},
    {.name=WS_C("include"), .processor=processIncludeOperands},
    {.name=WS_C("map"), .processor=processMapOperands},
    {.name=WS_C("title"), .processor=processTitleOperands},
    {.name=NULL, .processor=NULL}
  };

  return processPropertyOperand(file, properties, "key table directive", data);
}

static int
sortKeyBindings (const void *element1, const void *element2) {
  const KeyBinding *const *binding1 = element1;
  const KeyBinding *const *binding2 = element2;
  return compareKeyBindings(*binding1, *binding2);
}

static int
prepareKeyBindings (KeyContext *ctx) {
  if (ctx->keyBindingCount < ctx->keyBindingsSize) {
    KeyBinding *newTable = realloc(ctx->keyBindingTable, ARRAY_SIZE(newTable, ctx->keyBindingCount));

    if (!newTable) {
      LogError("realloc");
      return 0;
    }

    ctx->keyBindingTable = newTable;
    ctx->keyBindingsSize = ctx->keyBindingCount;
  }

  if (ctx->keyBindingCount) {
    if (!(ctx->sortedKeyBindings = malloc(ARRAY_SIZE(ctx->sortedKeyBindings, ctx->keyBindingCount)))) {
      LogError("malloc");
      return 0;
    }

    {
      KeyBinding *source = ctx->keyBindingTable;
      const KeyBinding **target = ctx->sortedKeyBindings;
      unsigned int count = ctx->keyBindingCount;

      while (count) {
        *target++ = source++;
        count -= 1;
      }
    }

    qsort(ctx->sortedKeyBindings, ctx->keyBindingCount, sizeof(*ctx->sortedKeyBindings), sortKeyBindings);
  }

  return 1;
}

KeyTable *
makeKeyTable (KeyTableData *ktd) {
  KeyTable *table;

  if (!setDefaultKeyContextProperties(ktd)) return 0;

  {
    unsigned int context;

    for (context=0; context<ktd->keyContextCount; context+=1) {
      KeyContext *ctx = &ktd->keyContextTable[context];

      if (!prepareKeyBindings(ctx)) return NULL;
    }
  }

  if ((table = malloc(sizeof(*table)))) {
    table->title = ktd->title;
    ktd->title = NULL;

    table->keyNameTable = ktd->keyNameTable;
    ktd->keyNameTable = NULL;
    table->keyNameCount = ktd->keyNameCount;
    ktd->keyNameCount = 0;
    qsort(table->keyNameTable, table->keyNameCount, sizeof(*table->keyNameTable), sortKeyValues);

    table->keyContextTable = ktd->keyContextTable;
    ktd->keyContextTable = NULL;
    table->keyContextCount = ktd->keyContextCount;
    ktd->keyContextCount = 0;

    resetKeyTable(table);
    return table;
  }

  return NULL;
}

KeyTable *
compileKeyTable (const char *name, const KeyNameEntry *const *keys) {
  KeyTable *table = NULL;
  KeyTableData ktd;

  memset(&ktd, 0, sizeof(ktd));
  ktd.context = BRL_CTX_DEFAULT;

  if (allocateKeyNameTable(&ktd, keys)) {
    if (allocateCommandTable(&ktd)) {
      ktd.title = NULL;

      ktd.keyContextTable = NULL;
      ktd.keyContextCount = 0;

      if (processDataFile(name, processKeyTableLine, &ktd)) {
        table = makeKeyTable(&ktd);

        destroyKeyContextTable(ktd.keyContextTable, ktd.keyContextCount);
        if (ktd.title) free(ktd.title);
      }

      if (ktd.commandTable) free(ktd.commandTable);
    }

    if (ktd.keyNameTable) free(ktd.keyNameTable);
  }

  return table;
}

void
destroyKeyTable (KeyTable *table) {
  if (table->title) free(table->title);
  if (table->keyNameTable) free(table->keyNameTable);
  destroyKeyContextTable(table->keyContextTable, table->keyContextCount);
  free(table);
}

char *
ensureKeyTableExtension (const char *path) {
  return ensureExtension(path, KEY_TABLE_EXTENSION);
}
