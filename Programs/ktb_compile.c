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
#include "dataarea.h"
#include "cmd.h"
#include "brldefs.h"
#include "ktb.h"
#include "ktb_internal.h"

typedef struct {
  DataArea *area;

  const KeyNameEntry **keyNameTable;
  unsigned int keyNameCount;

  const CommandEntry **commandTable;
  unsigned int commandCount;

  KeyBinding *bindingsTable;
  unsigned int bindingsSize;
  unsigned int bindingsCount;
} KeyTableData;

static inline KeyTableHeader *
getKeyTableHeader (KeyTableData *ktd) {
  return getDataItem(ktd->area, 0);
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
compareToKeyName (const void *target, const void *element) {
  const DataOperand *name = target;
  const KeyNameEntry *const *key = element;
  return compareToName(name->characters, name->length, (*key)->name);
}

static int
parseKeyName (DataFile *file, KeyCode *code, const wchar_t *characters, int length, KeyTableData *ktd) {
  const DataOperand name = {
    .characters = characters,
    .length = length
  };
  const KeyNameEntry **key = bsearch(&name, ktd->keyNameTable, ktd->keyNameCount, sizeof(*ktd->keyNameTable), compareToKeyName);

  if (key) {
    *code = (*key)->code;
    return 1;
  }

  reportDataError(file, "unknown key name: %.*" PRIws, length, characters);
  return 0;
}

static int
parseKeyCombination (DataFile *file, KeyCombination *key, const wchar_t *characters, int length, KeyTableData *ktd) {
  while (1) {
    const wchar_t *end = wmemchr(characters, WC_C('+'), length);
    if (!end) break;

    {
      int count = end - characters;
      KeyCode code;

      if (!count) {
        reportDataError(file, "missing modifier key name");
        return 0;
      }
      if (!parseKeyName(file, &code, characters, count, ktd)) return 0;

      if (code.set) {
        reportDataError(file, "unexpected modifier key name: %.*" PRIws, count, characters);
        return 0;
      }

      if (BITMASK_TEST(key->modifiers, code.key)) {
        reportDataError(file, "duplicate modifier key name: %.*" PRIws, count, characters);
        return 0;
      }
      BITMASK_SET(key->modifiers, code.key);

      length -= count + 1;
      characters = end + 1;
    }
  }

  if (!length) {
    reportDataError(file, "missing key name");
    return 0;
  }
  if (!parseKeyName(file, &key->code, characters, length, ktd)) return 0;

  if (!key->code.set && BITMASK_TEST(key->modifiers, key->code.key)) {
    reportDataError(file, "duplicate key name: %.*" PRIws, length, characters);
    return 0;
  }

  return 1;
}

static int
getKeyOperand (DataFile *file, KeyCombination *key, KeyTableData *ktd) {
  DataOperand names;

  if (getDataOperand(file, &names, "key combination")) {
    if (parseKeyCombination(file, key, names.characters, names.length, ktd)) return 1;
  }

  return 0;
}

static int
compareToCommandName (const void *target, const void *element) {
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
  const CommandEntry **command = bsearch(&name, ktd->commandTable, ktd->commandCount, sizeof(*ktd->commandTable), compareToCommandName);

  if (command) {
    *value = (*command)->code;
    if (!end) return 1;

    if (!(length -= end - characters + 1)) {
      reportDataError(file, "missing command modifier");
      return 0;
    }
    characters = end + 1;

    if (isToggleCommand((*command)->code)) {
      if (isKeyword(WS_C("on"), characters, length)) {
        *value |= BRL_FLG_TOGGLE_ON;
        return 1;
      }

      if (isKeyword(WS_C("off"), characters, length)) {
        *value |= BRL_FLG_TOGGLE_OFF;
        return 1;
      }
    } else if (isBaseCommand((*command)->code)) {
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

  if (ktd->bindingsCount == ktd->bindingsSize) {
    unsigned int newSize = ktd->bindingsSize? ktd->bindingsSize<<1: 0X10;
    KeyBinding *newBindings = realloc(ktd->bindingsTable, (newSize * sizeof(*newBindings)));

    if (!newBindings) return 0;
    ktd->bindingsTable = newBindings;
    ktd->bindingsSize = newSize;
  }

  binding = &ktd->bindingsTable[ktd->bindingsCount];
  memset(binding, 0, sizeof(*binding));

  if (getKeyOperand(file, &binding->key, ktd)) {
    if (getCommandOperand(file, &binding->command, ktd)) {
      if (!binding->key.code.set)
        if (isCharacterCommand(binding->command))
          binding->command |= BRL_MSK_ARG;

      ktd->bindingsCount += 1;
    }
  }

  return 1;
}

static int
processKeyTableLine (DataFile *file, void *data) {
  static const DataProperty properties[] = {
    {.name=WS_C("bind"), .processor=processBindOperands},
    {.name=WS_C("include"), .processor=processIncludeOperands},
    {.name=NULL, .processor=NULL}
  };

  return processPropertyOperand(file, properties, "key table directive", data);
}

static int
compareKeyNames (const void *element1, const void *element2) {
  const KeyNameEntry *const *key1 = element1;
  const KeyNameEntry *const *key2 = element2;
  return strcasecmp((*key1)->name, (*key2)->name);
}

static int
allocateKeyNameTable (KeyTableData *ktd, const KeyNameEntry *keys) {
  {
    const KeyNameEntry *key = keys;

    ktd->keyNameCount = 0;
    while (key->name) {
      ktd->keyNameCount += 1;
      key += 1;
    }
  }

  if ((ktd->keyNameTable = malloc(ktd->keyNameCount * sizeof(*ktd->keyNameTable)))) {
    {
      const KeyNameEntry *key = keys;
      const KeyNameEntry **address = ktd->keyNameTable;
      while (key->name) *address++ = key++;
    }

    qsort(ktd->keyNameTable, ktd->keyNameCount, sizeof(*ktd->keyNameTable), compareKeyNames);
    return 1;
  }

  return 0;
}

static int
compareCommandNames (const void *element1, const void *element2) {
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

    qsort(ktd->commandTable, ktd->commandCount, sizeof(*ktd->commandTable), compareCommandNames);
    return 1;
  }

  return 0;
}

static int
saveKeyBindings (KeyTableData *ktd) {
  KeyTableHeader *header = getKeyTableHeader(ktd);

  if ((header->bindingsCount = ktd->bindingsCount)) {
    DataOffset offset;

    if (!saveDataItem(ktd->area, &offset, ktd->bindingsTable,
                      ktd->bindingsCount * sizeof(ktd->bindingsTable[0]),
                      __alignof__(ktd->bindingsTable[0])))
      return 0;

    header->bindingsTable = offset;
  } else {
    header->bindingsTable = 0;
  }

  return 1;
}

KeyTable *
compileKeyTable (const char *name, const KeyNameEntry *keys) {
  KeyTable *table = NULL;
  KeyTableData ktd;

  memset(&ktd, 0, sizeof(ktd));

  ktd.bindingsTable = NULL;
  ktd.bindingsSize = 0;
  ktd.bindingsCount = 0;

  if (allocateKeyNameTable(&ktd, keys)) {
    if (allocateCommandTable(&ktd)) {
      if ((ktd.area = newDataArea())) {
        if (allocateDataItem(ktd.area, NULL, sizeof(KeyTableHeader), __alignof__(KeyTableHeader))) {
          if (processDataFile(name, processKeyTableLine, &ktd)) {
            if (saveKeyBindings(&ktd)) {
              if ((table = malloc(sizeof(*table)))) {
                table->header.fields = getKeyTableHeader(&ktd);
                table->size = getDataSize(ktd.area);
                resetDataArea(ktd.area);
                resetKeyTable(table);
              }
            }
          }
        }

        destroyDataArea(ktd.area);
      }

      free(ktd.commandTable);
    }

    free(ktd.keyNameTable);
  }

  if (ktd.bindingsTable) free(ktd.bindingsTable);

  return table;
}

void
destroyKeyTable (KeyTable *table) {
  if (table->size) {
    free(table->header.fields);
    free(table);
  }
}

char *
ensureKeyTableExtension (const char *path) {
  return ensureExtension(path, KEY_TABLE_EXTENSION);
}
