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

const InputFunctionEntry inputFunctionTable[InputFunctionCount] = {
  [IFN_Space] = {.name="Space", .bit=0},

  [IFN_Dot1] = {.name="Dot1", .bit=BRL_DOT1},
  [IFN_Dot2] = {.name="Dot2", .bit=BRL_DOT2},
  [IFN_Dot3] = {.name="Dot3", .bit=BRL_DOT3},
  [IFN_Dot4] = {.name="Dot4", .bit=BRL_DOT4},
  [IFN_Dot5] = {.name="Dot5", .bit=BRL_DOT5},
  [IFN_Dot6] = {.name="Dot6", .bit=BRL_DOT6},
  [IFN_Dot7] = {.name="Dot7", .bit=BRL_DOT7},
  [IFN_Dot8] = {.name="Dot8", .bit=BRL_DOT8},

  [IFN_Shift] = {.name="Shift", .bit=BRL_FLG_CHAR_SHIFT},
  [IFN_Upper] = {.name="Upper", .bit=BRL_FLG_CHAR_UPPER},
  [IFN_Control] = {.name="Control", .bit=BRL_FLG_CHAR_CONTROL},
  [IFN_Meta] = {.name="Meta", .bit=BRL_FLG_CHAR_META}
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
    }
  }

  return &ktd->keyContextTable[context];
}

static KeyContext *
getCurrentKeyContext (KeyTableData *ktd) {
  return getKeyContext(ktd, ktd->context);
}

static void
destroyKeyContextTable (KeyContext *table, unsigned int count) {
  while (count) {
    KeyContext *ctx = &table[--count];

    if (ctx->name) free(ctx->name);
    if (ctx->keyBindingTable) free(ctx->keyBindingTable);
    if (ctx->sortedKeyBindings) free(ctx->sortedKeyBindings);
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

  reportDataError(file, "unknown key name: %.*" PRIws, length, characters);
  return 0;
}

static int
getKeyOperand (DataFile *file, unsigned char *key, KeyTableData *ktd) {
  DataOperand name;

  if (getDataOperand(file, &name, "key name")) {
    unsigned char set;

    if (parseKeyName(file, &set, key, name.characters, name.length, ktd)) {
      if (!set) return 1;
      reportDataError(file, "invalid key name: %.*" PRIws, name.length, name.characters);
    }
  }

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
        reportDataError(file, "missing modifier key name");
        return 0;
      }
      if (!parseKeyName(file, &set, &key, characters, count, ktd)) return 0;

      if (set) {
        reportDataError(file, "unexpected modifier key name: %.*" PRIws, count, characters);
        return 0;
      }

      if (BITMASK_TEST(keys->modifiers, key)) {
        reportDataError(file, "duplicate modifier key name: %.*" PRIws, count, characters);
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
    reportDataError(file, "missing key name");
    return 0;
  }
  if (!parseKeyName(file, &keys->set, &keys->key, characters, length, ktd)) return 0;

  if (!keys->set) {
    if (BITMASK_TEST(keys->modifiers, keys->key)) {
      reportDataError(file, "duplicate key name: %.*" PRIws, length, characters);
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
sortInputFunctionNames (const void *element1, const void *element2) {
  const InputFunctionEntry *const *ifn1 = element1;
  const InputFunctionEntry *const *ifn2 = element2;
  return strcasecmp((*ifn1)->name, (*ifn2)->name);
}

static int
searchInputFunctionName (const void *target, const void *element) {
  const DataOperand *name = target;
  const InputFunctionEntry *const *ifn = element;
  return compareToName(name->characters, name->length, (*ifn)->name);
}

static int
parseInputFunctionName (DataFile *file, unsigned char *function, const wchar_t *characters, int length, KeyTableData *ktd) {
  static const InputFunctionEntry **sortedInputFunctions = NULL;

  if (!sortedInputFunctions) {
    const InputFunctionEntry **newTable = malloc(ARRAY_SIZE(newTable, InputFunctionCount));

    if (!newTable) {
      LogError("malloc");
      return 0;
    }

    {
      const InputFunctionEntry *source = inputFunctionTable;
      const InputFunctionEntry **target = newTable;
      unsigned int count = InputFunctionCount;

      do {
        *target++ = source++;
      } while (--count);

      qsort(newTable, InputFunctionCount, sizeof(*newTable), sortInputFunctionNames);
    }

    sortedInputFunctions = newTable;
  }

  {
    const DataOperand name = {
      .characters = characters,
      .length = length
    };
    const InputFunctionEntry **ifn = bsearch(&name, sortedInputFunctions, InputFunctionCount, sizeof(*sortedInputFunctions), searchInputFunctionName);

    if (ifn) {
      *function = *ifn - inputFunctionTable;
      return 1;
    }
  }

  reportDataError(file, "unknown input function name: %.*" PRIws, length, characters);
  return 0;
}

static int
getInputFunctionOperand (DataFile *file, unsigned char *function, KeyTableData *ktd) {
  DataOperand name;

  if (getDataOperand(file, &name, "input key name")) {
    if (parseInputFunctionName(file, function, name.characters, name.length, ktd)) return 1;
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
          } else if (!(ctx->name = malloc(ARRAY_SIZE(ctx->name, name.length+1)))) {
            LogError("malloc");
          } else {
            wmemcpy(ctx->name, name.characters, name.length);
            ctx->name[name.length] = 0;
          }
        }
      }
    }
  }

  return 1;
}

static int
processInputOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  KeyContext *ctx = getCurrentKeyContext(ktd);

  if (!ctx) return 0;

  {
    unsigned char function;

    if (getInputFunctionOperand(file, &function, ktd)) {
      unsigned char key;

      if (getKeyOperand(file, &key, ktd)) {
        ctx->inputKeys[function] = key;
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
      reportDataError(file, "title specified more than once");
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
    {.name=WS_C("input"), .processor=processInputOperands},
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

KeyTable *
makeKeyTable (KeyTableData *ktd) {
  KeyTable *table;

  {
    unsigned int context;

    for (context=0; context<ktd->keyContextCount; context+=1) {
      KeyContext *ctx = &ktd->keyContextTable[context];

      if (ctx->keyBindingCount < ctx->keyBindingsSize) {
        KeyBinding *newTable = realloc(ctx->keyBindingTable, ARRAY_SIZE(newTable, ctx->keyBindingCount));
        if (!newTable) return NULL;

        ctx->keyBindingTable = newTable;
        ctx->keyBindingsSize = ctx->keyBindingCount;
      }

      if (ctx->keyBindingCount) {
        if (!(ctx->sortedKeyBindings = malloc(ARRAY_SIZE(ctx->sortedKeyBindings, ctx->keyBindingCount)))) {
          LogError("malloc");
          return NULL;
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
