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

typedef struct {
  const KeyNameEntry **keyNameTable;
  unsigned int keyNameCount;

  const CommandEntry **commandTable;
  unsigned int commandCount;

  KeyBinding *keyBindingTable;
  unsigned int keyBindingsSize;
  unsigned int keyBindingCount;

  unsigned char context;
} KeyTableData;

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
searchKeyName (const void *target, const void *element) {
  const DataOperand *name = target;
  const KeyNameEntry *const *kne = element;
  return compareToName(name->characters, name->length, (*kne)->name);
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
searchCommandName (const void *target, const void *element) {
  const DataOperand *name = target;
  const CommandEntry *const *command = element;
  return compareToName(name->characters, name->length, (*command)->name);
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
  KeyBinding *binding;

  if (ktd->keyBindingCount == ktd->keyBindingsSize) {
    unsigned int newSize = ktd->keyBindingsSize? ktd->keyBindingsSize<<1: 0X10;
    KeyBinding *newTable = realloc(ktd->keyBindingTable, ARRAY_SIZE(newTable, newSize));

    if (!newTable) {
      LogError("realloc");
      return 0;
    }

    ktd->keyBindingTable = newTable;
    ktd->keyBindingsSize = newSize;
  }

  binding = &ktd->keyBindingTable[ktd->keyBindingCount];
  memset(binding, 0, sizeof(*binding));
  binding->context = ktd->context;

  if (getKeysOperand(file, &binding->keys, ktd)) {
    if (getCommandOperand(file, &binding->command, ktd)) {
      if (!binding->keys.set)
        if (getCommandEntry(binding->command)->isCharacter)
          binding->command |= BRL_MSK_ARG;

      ktd->keyBindingCount += 1;
    }
  }

  return 1;
}

static int
processContextOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  DataOperand context;

  if (getDataOperand(file, &context, "context name or number")) {
    if (isKeyword(WS_C("default"), context.characters, context.length)) {
      ktd->context = BRL_CTX_DEFAULT;
    } else if (isKeyword(WS_C("menu"), context.characters, context.length)) {
      ktd->context = BRL_CTX_MENU;
    } else {
      int number;

      if (!isNumber(&number, context.characters, context.length)) {
        reportDataError(file, "unknown context name: %.*" PRIws, context.length, context.characters);
      } else if ((number < 0) || (number > (BRL_MSK_ARG - BRL_CTX_DEFAULT))) {
        reportDataError(file, "invalid context number: %.*" PRIws, context.length, context.characters);
      } else {
        ktd->context = BRL_CTX_DEFAULT + number;
      }
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
    {.name=NULL, .processor=NULL}
  };

  return processPropertyOperand(file, properties, "key table directive", data);
}

static int
sortKeyNames (const void *element1, const void *element2) {
  const KeyNameEntry *const *kne1 = element1;
  const KeyNameEntry *const *kne2 = element2;
  return strcasecmp((*kne1)->name, (*kne2)->name);
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
sortCommandNames (const void *element1, const void *element2) {
  const CommandEntry *const *command1 = element1;
  const CommandEntry *const *command2 = element2;
  return strcasecmp((*command1)->name, (*command2)->name);
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
sortKeyBindings (const void *element1, const void *element2) {
  const KeyBinding *const *binding1 = element1;
  const KeyBinding *const *binding2 = element2;
  return compareKeyBindings(*binding1, *binding2);
}

KeyTable *
makeKeyTable (KeyTableData *ktd) {
  KeyTable *table;

  if ((table = malloc(sizeof(*table)))) {
    int keyBindingsAllocated = (table->keyBindingCount = ktd->keyBindingCount) == 0;

    if (keyBindingsAllocated) {
      table->keyBindingTable = NULL;
      table->sortedKeyBindings = NULL;
    } else {
      if ((table->keyBindingTable = realloc(ktd->keyBindingTable, ARRAY_SIZE(table->keyBindingTable, table->keyBindingCount)))) {
        ktd->keyBindingTable = NULL;

        if ((table->sortedKeyBindings = malloc(ARRAY_SIZE(table->sortedKeyBindings, table->keyBindingCount)))) {
          {
            KeyBinding *source = table->keyBindingTable;
            const KeyBinding **target = table->sortedKeyBindings;
            unsigned int count = table->keyBindingCount;

            while (count) {
              *target++ = source++;
              count -= 1;
            }
          }

          qsort(table->sortedKeyBindings, table->keyBindingCount, sizeof(*table->sortedKeyBindings), sortKeyBindings);
          keyBindingsAllocated = 1;
        } else {
          LogError("malloc");
        }

        if (!keyBindingsAllocated) free(table->keyBindingTable);
      } else {
        LogError("realloc");
      }
    }

    if (keyBindingsAllocated) {
      table->keyNameTable = ktd->keyNameTable;
      ktd->keyNameTable = NULL;
      table->keyNameCount = ktd->keyNameCount;
      ktd->keyNameCount = 0;
      qsort(table->keyNameTable, table->keyNameCount, sizeof(*table->keyNameTable), sortKeyValues);

      resetKeyTable(table);
      return table;
    }

    free(table);
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
      ktd.keyBindingTable = NULL;
      ktd.keyBindingsSize = 0;
      ktd.keyBindingCount = 0;

      if (processDataFile(name, processKeyTableLine, &ktd)) {
        table = makeKeyTable(&ktd);

        if (ktd.keyBindingTable) free(ktd.keyBindingTable);
      }

      if (ktd.commandTable) free(ktd.commandTable);
    }

    if (ktd.keyNameTable) free(ktd.keyNameTable);
  }

  return table;
}

void
destroyKeyTable (KeyTable *table) {
  if (table->keyNameTable) free(table->keyNameTable);
  if (table->keyBindingTable) free(table->keyBindingTable);
  if (table->sortedKeyBindings) free(table->sortedKeyBindings);
  free(table);
}

char *
ensureKeyTableExtension (const char *path) {
  return ensureExtension(path, KEY_TABLE_EXTENSION);
}
