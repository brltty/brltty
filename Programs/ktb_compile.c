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

#include <string.h>
#include <ctype.h>

#include "log.h"
#include "file.h"
#include "datafile.h"
#include "cmd.h"
#include "brldefs.h"
#include "ktb.h"
#include "ktb_internal.h"

const KeyboardFunction keyboardFunctionTable[] = {
  {.name="dot1", .bit=BRL_DOT1},
  {.name="dot2", .bit=BRL_DOT2},
  {.name="dot3", .bit=BRL_DOT3},
  {.name="dot4", .bit=BRL_DOT4},
  {.name="dot5", .bit=BRL_DOT5},
  {.name="dot6", .bit=BRL_DOT6},
  {.name="dot7", .bit=BRL_DOT7},
  {.name="dot8", .bit=BRL_DOT8},
  {.name="space", .bit=BRL_DOTC},
  {.name="shift", .bit=BRL_FLG_CHAR_SHIFT},
  {.name="uppercase", .bit=BRL_FLG_CHAR_UPPER},
  {.name="control", .bit=BRL_FLG_CHAR_CONTROL},
  {.name="meta", .bit=BRL_FLG_CHAR_META}
};
unsigned char keyboardFunctionCount = ARRAY_COUNT(keyboardFunctionTable);

typedef struct {
  KeyTable *table;

  const CommandEntry **commandTable;
  unsigned int commandCount;

  unsigned char context;

  unsigned hideRequested:1;
  unsigned hideImposed:1;
} KeyTableData;

void
copyKeyValues (KeyValue *target, const KeyValue *source, unsigned int count) {
  memcpy(target, source, count*sizeof(*target));
}

int
compareKeyValues (const KeyValue *value1, const KeyValue *value2) {
  if (value1->set < value2->set) return -1;
  if (value1->set > value2->set) return 1;

  if (value1->key < value2->key) return -1;
  if (value1->key > value2->key) return 1;

  return 0;
}

static int
compareKeyArrays (
  unsigned int count1, const KeyValue *array1,
  unsigned int count2, const KeyValue *array2
) {
  if (count1 < count2) return -1;
  if (count1 > count2) return 1;
  return memcmp(array1, array2, count1*sizeof(*array1));
}

int
findKeyValue (
  const KeyValue *values, unsigned int count,
  const KeyValue *target, unsigned int *position
) {
  int first = 0;
  int last = count - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    const KeyValue *value = &values[current];
    int relation = compareKeyValues(target, value);

    if (!relation) {
      *position = current;
      return 1;
    }

    if (relation < 0) {
      last = current - 1;
    } else {
      first = current + 1;
    }
  }

  *position = first;
  return 0;
}

int
insertKeyValue (
  KeyValue **values, unsigned int *count, unsigned int *size,
  const KeyValue *value, unsigned int position
) {
  if (*count == *size) {
    unsigned int newSize = (*size)? (*size)<<1: 0X10;
    KeyValue *newValues = realloc(*values, ARRAY_SIZE(newValues, newSize));

    if (!newValues) {
      logSystemError("realloc");
      return 0;
    }

    *values = newValues;
    *size = newSize;
  }

  memmove(&(*values)[position+1], &(*values)[position],
          ((*count)++ - position) * sizeof(**values));
  (*values)[position] = *value;
  return 1;
}

void
removeKeyValue (KeyValue *values, unsigned int *count, unsigned int position) {
  memmove(&values[position], &values[position+1],
          (--*count - position) * sizeof(*values));
}

int
deleteKeyValue (KeyValue *values, unsigned int *count, const KeyValue *value) {
  unsigned int position;
  int found = findKeyValue(values, *count, value, &position);

  if (found) removeKeyValue(values, count, position);
  return found;
}

static inline int
hideBindings (const KeyTableData *ktd) {
  return ktd->hideRequested || ktd->hideImposed;
}

static KeyContext *
getKeyContext (KeyTableData *ktd, unsigned char context) {
  if (context >= ktd->table->keyContextCount) {
    unsigned int newCount = context + 1;
    KeyContext *newTable = realloc(ktd->table->keyContextTable, ARRAY_SIZE(newTable, newCount));

    if (!newTable) {
      logSystemError("realloc");
      return NULL;
    }
    ktd->table->keyContextTable = newTable;

    while (ktd->table->keyContextCount < newCount) {
      KeyContext *ctx = &ktd->table->keyContextTable[ktd->table->keyContextCount++];
      memset(ctx, 0, sizeof(*ctx));

      ctx->title = NULL;

      ctx->keyBindings.table = NULL;
      ctx->keyBindings.size = 0;
      ctx->keyBindings.count = 0;
      ctx->keyBindings.sorted = NULL;

      ctx->hotkeys.table = NULL;
      ctx->hotkeys.count = 0;
      ctx->hotkeys.sorted = NULL;

      ctx->mappedKeys.table = NULL;
      ctx->mappedKeys.count = 0;
      ctx->mappedKeys.sorted = NULL;
      ctx->mappedKeys.superimpose = 0;
    }
  }

  return &ktd->table->keyContextTable[context];
}

static inline KeyContext *
getCurrentKeyContext (KeyTableData *ktd) {
  return getKeyContext(ktd, ktd->context);
}

static int
setKeyContextTitle (KeyContext *ctx, const wchar_t *title, size_t length) {
  if (ctx->title) free(ctx->title);

  if (!(ctx->title = malloc(ARRAY_SIZE(ctx->title, length+1)))) {
    logMallocError();
    return 0;
  }

  wmemcpy(ctx->title, title, length);
  ctx->title[length] = 0;
  return 1;
}

static int
setDefaultKeyContextProperties (KeyTableData *ktd) {
  typedef struct {
    unsigned char context;
    const wchar_t *title;
  } PropertiesEntry;

  static const PropertiesEntry propertiesTable[] = {
    { .context = KTB_CTX_DEFAULT,
      .title = WS_C("Default Bindings")
    }
    ,
    { .context = KTB_CTX_MENU,
      .title = WS_C("Menu Bindings")
    }
    ,
    { .title = NULL }
  };
  const PropertiesEntry *properties = propertiesTable;

  while (properties->title) {
    KeyContext *ctx = getKeyContext(ktd, properties->context);
    if (!ctx) return 0;

    if (!ctx->title)
      if (!setKeyContextTitle(ctx, properties->title, wcslen(properties->title)))
        return 0;

    properties += 1;
  }

  return 1;
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
  int result = compareKeyValues(&(*kne1)->value, &(*kne2)->value);
  if (result != 0) return result;
  if (*kne1 < *kne2) return -1;
  if (*kne1 > *kne2) return 1;
  return 0;
}

static int
allocateKeyNameTable (KeyTableData *ktd, KEY_NAME_TABLES_REFERENCE keys) {
  {
    const KeyNameEntry *const *knt = keys;

    ktd->table->keyNameCount = 0;
    while (*knt) {
      const KeyNameEntry *kne = *knt;
      while (kne->name) kne += 1;
      ktd->table->keyNameCount += kne - *knt;
      knt += 1;
    }
  }

  if ((ktd->table->keyNameTable = malloc(ARRAY_SIZE(ktd->table->keyNameTable, ktd->table->keyNameCount)))) {
    {
      const KeyNameEntry **address = ktd->table->keyNameTable;
      const KeyNameEntry *const *knt = keys;

      while (*knt) {
        const KeyNameEntry *kne = *knt++;
        while (kne->name) *address++ = kne++;
      }
    }

    qsort(ktd->table->keyNameTable, ktd->table->keyNameCount, sizeof(*ktd->table->keyNameTable), sortKeyNames);
    return 1;
  }

  return 0;
}

static const KeyNameEntry *const *
findKeyName (const wchar_t *characters, int length, KeyTableData *ktd) {
  const DataOperand name = {
    .characters = characters,
    .length = length
  };

  return bsearch(&name, ktd->table->keyNameTable, ktd->table->keyNameCount, sizeof(*ktd->table->keyNameTable), searchKeyName);
}

static int
parseKeyName (DataFile *file, KeyValue *value, const wchar_t *characters, int length, KeyTableData *ktd) {
  const wchar_t *suffix = wmemchr(characters, WC_C('.'), length);
  int prefixLength;
  int suffixLength;

  if (suffix) {
    if (!(prefixLength = suffix - characters)) {
      reportDataError(file, "missing key set name: %.*" PRIws, length, characters);
      return 0;
    }

    if (!(suffixLength = (characters + length) - ++suffix)) {
      reportDataError(file, "missing key number: %.*" PRIws, length, characters);
      return 0;
    }
  } else {
    prefixLength = length;
    suffixLength = 0;
  }

  {
    const KeyNameEntry *const *kne = findKeyName(characters, prefixLength, ktd);

    if (!kne) {
      reportDataError(file, "unknown key name: %.*" PRIws, prefixLength, characters);
      return 0;
    }

    *value = (*kne)->value;
  }

  if (suffix) {
    int ok = 0;
    int number;

    if (isNumber(&number, suffix, suffixLength))
      if (number > 0)
        if (--number <= KTB_KEY_MAX)
          ok = 1;

    if (!ok) {
      reportDataError(file, "invalid key number: %.*" PRIws, suffixLength, suffix);
      return 0;
    }

    if (value->key != KTB_KEY_ANY) {
      reportDataError(file, "not a key set: %.*" PRIws, prefixLength, characters);
      return 0;
    }

    value->key = number;
  }

  return 1;
}

static int
getKeyOperand (DataFile *file, KeyValue *value, KeyTableData *ktd) {
  DataString name;

  if (getDataString(file, &name, 1, "key name")) {
    if (parseKeyName(file, value, name.characters, name.length, ktd)) {
      return 1;
    }
  }

  return 0;
}

static int
newModifierPosition (const KeyCombination *combination, const KeyValue *modifier, unsigned int *position) {
  int found = findKeyValue(combination->modifierKeys, combination->modifierCount, modifier, position);
  return found && (modifier->key != KTB_KEY_ANY);
}

static int
insertModifier (DataFile *file, KeyCombination *combination, unsigned int position, const KeyValue *value) {
  if (combination->modifierCount == MAX_MODIFIERS_PER_COMBINATION) {
    reportDataError(file, "too many modifier keys");
    return 0;
  }

  {
    int index = combination->modifierCount;

    while (index--) {
      if (index >= position) {
        combination->modifierKeys[index+1] = combination->modifierKeys[index];
      }

      if (combination->modifierPositions[index] >= position) {
        combination->modifierPositions[index] += 1;
      }
    }
  }

  combination->modifierKeys[position] = *value;
  combination->modifierPositions[combination->modifierCount++] = position;
  return 1;
}

static int
parseKeyCombination (DataFile *file, KeyCombination *combination, const wchar_t *characters, int length, KeyTableData *ktd) {
  KeyValue value;

  memset(combination, 0, sizeof(*combination));
  combination->modifierCount = 0;

  while (1) {
    const wchar_t *end = wmemchr(characters, WC_C('+'), length);
    if (!end) break;

    {
      int count = end - characters;

      if (!count) {
        reportDataError(file, "missing modifier key");
        return 0;
      }
      if (!parseKeyName(file, &value, characters, count, ktd)) return 0;

      {
        unsigned int position;

        if (newModifierPosition(combination, &value, &position)) {
          reportDataError(file, "duplicate modifier key: %.*" PRIws, count, characters);
          return 0;
        }

        if (!insertModifier(file, combination, position, &value)) return 0;
        if (value.key == KTB_KEY_ANY) combination->anyKeyCount += 1;
      }

      length -= count + 1;
      characters = end + 1;
    }
  }

  if (length) {
    if (*characters == WC_C('!')) {
      characters += 1, length -= 1;
      combination->flags |= KCF_IMMEDIATE_KEY;
    }
  }

  if (!length) {
    reportDataError(file, "missing key");
    return 0;
  }
  if (!parseKeyName(file, &value, characters, length, ktd)) return 0;

  {
    unsigned int position;

    if (newModifierPosition(combination, &value, &position)) {
      reportDataError(file, "duplicate key: %.*" PRIws, length, characters);
      return 0;
    }

    if (combination->flags & KCF_IMMEDIATE_KEY) {
      combination->immediateKey = value;
    } else if (!insertModifier(file, combination, position, &value)) {
      return 0;
    }
    if (value.key == KTB_KEY_ANY) combination->anyKeyCount += 1;
  }

  return 1;
}

static int
getKeysOperand (DataFile *file, KeyCombination *combination, KeyTableData *ktd) {
  DataString names;

  if (getDataString(file, &names, 1, "key combination")) {
    if (parseKeyCombination(file, combination, names.characters, names.length, ktd)) return 1;
  }

  return 0;
}

static int
sortKeyboardFunctionNames (const void *element1, const void *element2) {
  const KeyboardFunction *const *kbf1 = element1;
  const KeyboardFunction *const *kbf2 = element2;
  return strcasecmp((*kbf1)->name, (*kbf2)->name);
}

static int
searchKeyboardFunctionName (const void *target, const void *element) {
  const DataOperand *name = target;
  const KeyboardFunction *const *kbf = element;
  return compareToName(name->characters, name->length, (*kbf)->name);
}

static int
parseKeyboardFunctionName (DataFile *file, const KeyboardFunction **keyboardFunction, const wchar_t *characters, int length, KeyTableData *ktd) {
  static const KeyboardFunction **sortedKeyboardFunctions = NULL;

  if (!sortedKeyboardFunctions) {
    const KeyboardFunction **newTable = malloc(ARRAY_SIZE(newTable, keyboardFunctionCount));

    if (!newTable) {
      logMallocError();
      return 0;
    }

    {
      const KeyboardFunction *source = keyboardFunctionTable;
      const KeyboardFunction **target = newTable;
      unsigned int count = keyboardFunctionCount;

      do {
        *target++ = source++;
      } while (--count);

      qsort(newTable, keyboardFunctionCount, sizeof(*newTable), sortKeyboardFunctionNames);
    }

    sortedKeyboardFunctions = newTable;
  }

  {
    const DataOperand name = {
      .characters = characters,
      .length = length
    };
    const KeyboardFunction *const *kbf = bsearch(&name, sortedKeyboardFunctions, keyboardFunctionCount, sizeof(*sortedKeyboardFunctions), searchKeyboardFunctionName);

    if (kbf) {
      *keyboardFunction = *kbf;
      return 1;
    }
  }

  reportDataError(file, "unknown keyboard function: %.*" PRIws, length, characters);
  return 0;
}

static int
getKeyboardFunctionOperand (DataFile *file, const KeyboardFunction **keyboardFunction, KeyTableData *ktd) {
  DataOperand name;

  if (getDataOperand(file, &name, "keyboard function name")) {
    if (parseKeyboardFunctionName(file, keyboardFunction, name.characters, name.length, ktd)) return 1;
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
applyCommandModifier (int *command, const CommandModifierEntry *modifiers, const DataOperand *name) {
  const CommandModifierEntry *modifier = modifiers;

  while (modifier->name) {
    if (!(*command & modifier->bit)) {
      if (compareToName(name->characters, name->length, modifier->name) == 0) {
        *command |= modifier->bit;
        return 1;
      }
    }

    modifier += 1;
  }

  return 0;
}

static const CommandEntry *
parseCommandOperand (DataFile *file, int *value, const wchar_t *characters, int length, KeyTableData *ktd) {
  int offsetDone = 0;
  int unicodeDone = 0;

  const wchar_t *end = wmemchr(characters, WC_C('+'), length);
  const CommandEntry *const *command;

  {
    const DataOperand name = {
      .characters = characters,
      .length = end? end-characters: length
    };

    if (!name.length) {
      reportDataError(file, "missing command name");
      return NULL;
    }

    if (!(command = bsearch(&name, ktd->commandTable, ktd->commandCount, sizeof(*ktd->commandTable), searchCommandName))) {
      reportDataError(file, "unknown command name: %.*" PRIws, name.length, name.characters);
      return NULL;
    }
  }
  *value = (*command)->code;

  while (end) {
    DataOperand modifier;

    if (!(length -= end - characters + 1)) {
      reportDataError(file, "missing command modifier");
      return NULL;
    }

    characters = end + 1;
    end = wmemchr(characters, WC_C('+'), length);

    modifier.characters = characters;
    modifier.length = end? end-characters: length;

    if ((*command)->isToggle && !(*value & BRL_FLG_TOGGLE_MASK)) {
      if (applyCommandModifier(value, commandModifierTable_toggle, &modifier)) continue;
    }

    if ((*command)->isMotion) {
      if (applyCommandModifier(value, commandModifierTable_motion, &modifier)) continue;

      if ((*command)->isRow) {
        if (applyCommandModifier(value, commandModifierTable_line, &modifier)) continue;
      }
    }

    if ((*command)->isInput) {
      if (applyCommandModifier(value, commandModifierTable_input, &modifier)) continue;
    }

    if ((*command)->isCharacter) {
      if (applyCommandModifier(value, commandModifierTable_character, &modifier)) continue;

      if (!unicodeDone) {
        if (modifier.length == 1) {
          *value |= BRL_ARG_SET(modifier.characters[0]);
          unicodeDone = 1;
          continue;
        }
      }
    }

    if ((*command)->isBraille) {
      if (applyCommandModifier(value, commandModifierTable_braille, &modifier)) continue;
      if (applyCommandModifier(value, commandModifierTable_character, &modifier)) continue;
    }

    if ((*command)->isKeyboard) {
      if (applyCommandModifier(value, commandModifierTable_keyboard, &modifier)) continue;
    }

    if (((*command)->isOffset || (*command)->isColumn) && !offsetDone) {
      int maximum = BRL_MSK_ARG - ((*command)->code & BRL_MSK_ARG);
      int offset;

      if (isNumber(&offset, modifier.characters, modifier.length)) {
        if ((offset >= 0) && (offset <= maximum)) {
          *value += offset;
          offsetDone = 1;
          continue;
        }
      }
    }

    reportDataError(file, "unknown command modifier: %.*" PRIws, modifier.length, modifier.characters);
    return NULL;
  }

  return *command;
}

static const CommandEntry *
getCommandOperand (DataFile *file, int *value, KeyTableData *ktd) {
  DataString name;

  if (getDataString(file, &name, 1, "command name")) {
    const CommandEntry *cmd;

    if ((cmd = parseCommandOperand(file, value, name.characters, name.length, ktd))) return cmd;
  }

  return NULL;
}

static int
addKeyBinding (KeyContext *ctx, const KeyBinding *binding) {
  if (ctx->keyBindings.count == ctx->keyBindings.size) {
    unsigned int newSize = ctx->keyBindings.size? ctx->keyBindings.size<<1: 0X10;
    KeyBinding *newTable = realloc(ctx->keyBindings.table, ARRAY_SIZE(newTable, newSize));

    if (!newTable) {
      logSystemError("realloc");
      return 0;
    }

    ctx->keyBindings.table = newTable;
    ctx->keyBindings.size = newSize;
  }

  ctx->keyBindings.table[ctx->keyBindings.count++] = *binding;
  return 1;
}

static int processKeyTableLine (DataFile *file, void *data);

static int
processBindOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  KeyContext *ctx = getCurrentKeyContext(ktd);
  KeyBinding binding;

  if (!ctx) return 0;

  memset(&binding, 0, sizeof(binding));
  if (hideBindings(ktd)) binding.flags |= KBF_HIDDEN;

  if (getKeysOperand(file, &binding.combination, ktd)) {
    const CommandEntry *cmd;

    if ((cmd = getCommandOperand(file, &binding.command, ktd))) {
      if (cmd->isOffset) binding.flags |= KBF_OFFSET;
      if (cmd->isColumn) binding.flags |= KBF_COLUMN;
      if (cmd->isRow) binding.flags |= KBF_ROW;
      if (cmd->isRange) binding.flags |= KBF_RANGE;
      if (cmd->isRouting) binding.flags |= KBF_ROUTE;
      if (cmd->isKeyboard) binding.flags |= KBF_KEYBOARD;
      if (!addKeyBinding(ctx, &binding)) return 0;
    }
  }

  return 1;
}

static int
processContextOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  DataString context;

  if (getDataString(file, &context, 1, "context name or number")) {
    int ok = 0;

    if (isKeyword(WS_C("default"), context.characters, context.length)) {
      ktd->context = KTB_CTX_DEFAULT;
      ok = 1;
    } else if (isKeyword(WS_C("menu"), context.characters, context.length)) {
      ktd->context = KTB_CTX_MENU;
      ok = 1;
    } else {
      int number;

      if (!isNumber(&number, context.characters, context.length)) {
        reportDataError(file, "unknown context name: %.*" PRIws, context.length, context.characters);
      } else if ((number < 0) || (number > (BRL_MSK_ARG - KTB_CTX_DEFAULT))) {
        reportDataError(file, "invalid context number: %.*" PRIws, context.length, context.characters);
      } else {
        ktd->context = KTB_CTX_DEFAULT + number;
        ok = 1;
      }
    }

    if (ok) {
      KeyContext *ctx = getCurrentKeyContext(ktd);

      if (ctx) {
        DataOperand title;

        if (getDataText(file, &title, NULL)) {
          if (ctx->title) {
            if ((title.length != wcslen(ctx->title)) ||
                (wmemcmp(title.characters, ctx->title, title.length) != 0)) {
              reportDataError(file, "context title redefined");
              return 0;
            }
          } else if (!setKeyContextTitle(ctx, title.characters, title.length)) {
            return 0;
          }
        }
      }
    }
  }

  return 1;
}

static int
processHideOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  DataString state;

  if (getDataString(file, &state, 1, "hide state")) {
    if (isKeyword(WS_C("on"), state.characters, state.length)) {
      ktd->hideRequested = 1;
    } else if (isKeyword(WS_C("off"), state.characters, state.length)) {
      ktd->hideRequested = 0;
    } else {
      reportDataError(file, "unknown hide state: %.*" PRIws, state.length, state.characters);
    }
  }

  return 1;
}

static int
processHotkeyOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  KeyContext *ctx = getCurrentKeyContext(ktd);
  HotkeyEntry hotkey;

  if (!ctx) return 0;
  memset(&hotkey, 0, sizeof(hotkey));
  if (hideBindings(ktd)) hotkey.flags |= HKF_HIDDEN;

  if (getKeyOperand(file, &hotkey.keyValue, ktd)) {
    if (getCommandOperand(file, &hotkey.pressCommand, ktd)) {
      if (getCommandOperand(file, &hotkey.releaseCommand, ktd)) {
        unsigned int newCount = ctx->hotkeys.count + 1;
        HotkeyEntry *newTable = realloc(ctx->hotkeys.table, ARRAY_SIZE(newTable, newCount));

        if (!newTable) {
          logSystemError("realloc");
          return 0;
        }

        ctx->hotkeys.table = newTable;
        ctx->hotkeys.table[ctx->hotkeys.count++] = hotkey;
        return 1;
      }
    }
  }

  return 1;
}

static int
processIfKeyOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  DataString name;

  if (getDataString(file, &name, 1, "key name")) {
    if (findKeyName(name.characters, name.length, ktd)) {
      return processKeyTableLine(file, ktd);
    }
  }

  return 1;
}

static int
processIncludeWrapper (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  int result;

  unsigned int hideRequested = ktd->hideRequested;
  unsigned int hideImposed = ktd->hideImposed;

  if (ktd->hideRequested) ktd->hideImposed = 1;
  result = processIncludeOperands(file, data);

  ktd->hideRequested = hideRequested;
  ktd->hideImposed = hideImposed;
  return result;
}

static int
processMapOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  KeyContext *ctx = getCurrentKeyContext(ktd);
  MappedKeyEntry map;

  if (!ctx) return 0;
  memset(&map, 0, sizeof(map));

  if (getKeyOperand(file, &map.keyValue, ktd)) {
    if (map.keyValue.key != KTB_KEY_ANY) {
      if (getKeyboardFunctionOperand(file, &map.keyboardFunction, ktd)) {
        unsigned int newCount = ctx->mappedKeys.count + 1;
        MappedKeyEntry *newTable = realloc(ctx->mappedKeys.table, ARRAY_SIZE(newTable, newCount));

        if (!newTable) {
          logSystemError("realloc");
          return 0;
        }

        ctx->mappedKeys.table = newTable;
        ctx->mappedKeys.table[ctx->mappedKeys.count++] = map;
        return 1;
      }
    } else {
      reportDataError(file, "cannot map key set");
    }
  }

  return 1;
}

static int
processNoteOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  DataOperand operand;

  if (getDataText(file, &operand, "note text")) {
    if (!hideBindings(ktd)) {
      DataString string;

      if (parseDataString(file, &string, operand.characters, operand.length, 0)) {
        unsigned int newCount = ktd->table->noteCount + 1;
        wchar_t **newTable = realloc(ktd->table->noteTable, ARRAY_SIZE(newTable, newCount));

        if (!newTable) {
          logSystemError("realloc");
          return 0;
        }

        ktd->table->noteTable = newTable;

        {
          wchar_t *noteString = malloc(ARRAY_SIZE(*ktd->table->noteTable, string.length+1));

          if (!noteString) {
            logMallocError();
            return 0;
          }

          wmemcpy(noteString, string.characters, string.length);
          noteString[string.length] = 0;
          ktd->table->noteTable[ktd->table->noteCount++] = noteString;
          return 1;
        }
      }
    }
  }

  return 1;
}

static int
processSuperimposeOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  KeyContext *ctx = getCurrentKeyContext(ktd);
  if (!ctx) return 0;

  {
    const KeyboardFunction *kbf;

    if (getKeyboardFunctionOperand(file, &kbf, ktd)) {
      ctx->mappedKeys.superimpose |= kbf->bit;
      return 1;
    }
  }

  return 1;
}

static int
processTitleOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  DataOperand title;

  if (getDataText(file, &title, "title text")) {
    if (ktd->table->title) {
      reportDataError(file, "table title specified more than once");
    } else if (!(ktd->table->title = malloc(ARRAY_SIZE(ktd->table->title, title.length+1)))) {
      logMallocError();
      return 0;
    } else {
      wmemcpy(ktd->table->title, title.characters, title.length);
      ktd->table->title[title.length] = 0;
      return 1;
    }
  }

  return 1;
}

static int
processKeyTableLine (DataFile *file, void *data) {
  static const DataProperty properties[] = {
    {.name=WS_C("assign"), .processor=processAssignOperands},
    {.name=WS_C("bind"), .processor=processBindOperands},
    {.name=WS_C("context"), .processor=processContextOperands},
    {.name=WS_C("hide"), .processor=processHideOperands},
    {.name=WS_C("hotkey"), .processor=processHotkeyOperands},
    {.name=WS_C("ifkey"), .processor=processIfKeyOperands},
    {.name=WS_C("include"), .processor=processIncludeWrapper},
    {.name=WS_C("map"), .processor=processMapOperands},
    {.name=WS_C("note"), .processor=processNoteOperands},
    {.name=WS_C("superimpose"), .processor=processSuperimposeOperands},
    {.name=WS_C("title"), .processor=processTitleOperands},
    {.name=NULL, .processor=NULL}
  };

  return processPropertyOperand(file, properties, "key table directive", data);
}

void
resetKeyTable (KeyTable *table) {
  table->context.current = table->context.next = table->context.persistent = KTB_CTX_DEFAULT;
  table->pressedCount = 0;
  table->command = EOF;
  table->immediate = 0;
}

static int
compareKeyCombinations (const KeyCombination *combination1, const KeyCombination *combination2) {
  if (combination1->flags & KCF_IMMEDIATE_KEY) {
    if (combination2->flags & KCF_IMMEDIATE_KEY) {
      int relation = compareKeyValues(&combination1->immediateKey, &combination2->immediateKey);
      if (relation) return relation;
    } else {
      return -1;
    }
  } else if (combination2->flags & KCF_IMMEDIATE_KEY) {
    return 1;
  }

  return compareKeyArrays(combination1->modifierCount, combination1->modifierKeys,
                          combination2->modifierCount, combination2->modifierKeys);
}

int
compareKeyBindings (const KeyBinding *binding1, const KeyBinding *binding2) {
  return compareKeyCombinations(&binding1->combination, &binding2->combination);
}

static int
sortKeyBindings (const void *element1, const void *element2) {
  const KeyBinding *const *binding1 = element1;
  const KeyBinding *const *binding2 = element2;
  return compareKeyBindings(*binding1, *binding2);
}

typedef struct {
  unsigned int *indexTable;
  unsigned int indexSize;
  unsigned int indexCount;
} IncompleteBindingData;

static int
addBindingIndex (KeyContext *ctx, const KeyValue *keys, unsigned char count, unsigned int index, IncompleteBindingData *ibd) {
  int first = 0;
  int last = ibd->indexCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    const KeyCombination *combination = &ctx->keyBindings.table[ibd->indexTable[current]].combination;
    int relation = compareKeyArrays(count, keys, combination->modifierCount, combination->modifierKeys);
    if (!relation) return 1;

    if (relation < 0) {
      last = current - 1;
    } else {
      first = current + 1;
    }
  }

  if (ibd->indexCount == ibd->indexSize) {
    unsigned int newSize = ibd->indexSize? ibd->indexSize<<1: 0X40;
    unsigned int *newTable = realloc(ibd->indexTable, ARRAY_SIZE(newTable, newSize));

    if (!newTable) {
      logSystemError("realloc");
      return 0;
    }

    ibd->indexTable = newTable;
    ibd->indexSize = newSize;
  }

  if (index == ctx->keyBindings.count) {
    KeyBinding binding = {
      .flags = KBF_HIDDEN,
      .command = EOF,

      .combination = {
        .modifierCount = count
      }
    };

    copyKeyValues(binding.combination.modifierKeys, keys, count);

    if (!addKeyBinding(ctx, &binding)) return 0;
  }

  memmove(&ibd->indexTable[first+1], &ibd->indexTable[first],
          (ibd->indexCount++ - first) * sizeof(*ibd->indexTable));
  ibd->indexTable[first] = index;
  return 1;
}

static int
addSubbindingIndexes (KeyContext *ctx, const KeyValue *keys, unsigned char count, IncompleteBindingData *ibd) {
  if (count > 1) {
    KeyValue values[--count];
    int index = 0;

    copyKeyValues(values, &keys[1], count);

    while (1) {
      if (!addBindingIndex(ctx, values, count, ctx->keyBindings.count, ibd)) return 0;
      if (!addSubbindingIndexes(ctx, values, count, ibd)) return 0;
      if (index == count) break;
      values[index] = keys[index];
      index += 1;
    }
  }

  return 1;
}

static int
addIncompleteBindings (KeyContext *ctx) {
  int ok = 1;
  IncompleteBindingData ibd = {
    .indexTable = NULL,
    .indexSize = 0,
    .indexCount = 0
  };

  {
    int index;

    for (index=0; index<ctx->keyBindings.count; index+=1) {
      const KeyCombination *combination = &ctx->keyBindings.table[index].combination;

      if (!(combination->flags & KCF_IMMEDIATE_KEY)) {
        if (!addBindingIndex(ctx, combination->modifierKeys, combination->modifierCount, index, &ibd)) {
          ok = 0;
          goto done;
        }
      }
    }
  }

  {
    int count = ctx->keyBindings.count;
    int index;

    for (index=0; index<count; index+=1) {
      const KeyCombination *combination = &ctx->keyBindings.table[index].combination;

      if ((combination->flags & KCF_IMMEDIATE_KEY)) {
        if (!addBindingIndex(ctx, combination->modifierKeys, combination->modifierCount, ctx->keyBindings.count, &ibd)) {
          ok = 0;
          goto done;
        }
      }

      if (!addSubbindingIndexes(ctx, combination->modifierKeys, combination->modifierCount, &ibd)) {
        ok = 0;
        goto done;
      }
    }
  }

done:
  if (ibd.indexTable) free(ibd.indexTable);
  return ok;
}

static int
prepareKeyBindings (KeyContext *ctx) {
  if (!addIncompleteBindings(ctx)) return 0;

  if (ctx->keyBindings.count < ctx->keyBindings.size) {
    if (ctx->keyBindings.count) {
      KeyBinding *newTable = realloc(ctx->keyBindings.table, ARRAY_SIZE(newTable, ctx->keyBindings.count));

      if (!newTable) {
        logSystemError("realloc");
        return 0;
      }

      ctx->keyBindings.table = newTable;
    } else {
      free(ctx->keyBindings.table);
      ctx->keyBindings.table = NULL;
    }

    ctx->keyBindings.size = ctx->keyBindings.count;
  }

  if (ctx->keyBindings.count) {
    if (!(ctx->keyBindings.sorted = malloc(ARRAY_SIZE(ctx->keyBindings.sorted, ctx->keyBindings.count)))) {
      logMallocError();
      return 0;
    }

    {
      KeyBinding *source = ctx->keyBindings.table;
      const KeyBinding **target = ctx->keyBindings.sorted;
      unsigned int count = ctx->keyBindings.count;

      while (count) {
        *target++ = source++;
        count -= 1;
      }
    }

    qsort(ctx->keyBindings.sorted, ctx->keyBindings.count, sizeof(*ctx->keyBindings.sorted), sortKeyBindings);
  }

  return 1;
}

static int
sortHotkeyEntries (const void *element1, const void *element2) {
  const HotkeyEntry *const *hotkey1 = element1;
  const HotkeyEntry *const *hotkey2 = element2;

  return compareKeyValues(&(*hotkey1)->keyValue, &(*hotkey2)->keyValue);
}

static int
prepareHotkeyEntries (KeyContext *ctx) {
  if (ctx->hotkeys.count) {
    if (!(ctx->hotkeys.sorted = malloc(ARRAY_SIZE(ctx->hotkeys.sorted, ctx->hotkeys.count)))) {
      logMallocError();
      return 0;
    }

    {
      const HotkeyEntry *source = ctx->hotkeys.table;
      const HotkeyEntry **target = ctx->hotkeys.sorted;
      unsigned int count = ctx->hotkeys.count;

      while (count) {
        *target++ = source++;
        count -= 1;
      }
    }

    qsort(ctx->hotkeys.sorted, ctx->hotkeys.count, sizeof(*ctx->hotkeys.sorted), sortHotkeyEntries);
  }

  return 1;
}

static int
sortMappedKeyEntries (const void *element1, const void *element2) {
  const MappedKeyEntry *const *map1 = element1;
  const MappedKeyEntry *const *map2 = element2;

  return compareKeyValues(&(*map1)->keyValue, &(*map2)->keyValue);
}

static int
prepareMappedKeyEntries (KeyContext *ctx) {
  if (ctx->mappedKeys.count) {
    if (!(ctx->mappedKeys.sorted = malloc(ARRAY_SIZE(ctx->mappedKeys.sorted, ctx->mappedKeys.count)))) {
      logMallocError();
      return 0;
    }

    {
      const MappedKeyEntry *source = ctx->mappedKeys.table;
      const MappedKeyEntry **target = ctx->mappedKeys.sorted;
      unsigned int count = ctx->mappedKeys.count;

      while (count) {
        *target++ = source++;
        count -= 1;
      }
    }

    qsort(ctx->mappedKeys.sorted, ctx->mappedKeys.count, sizeof(*ctx->mappedKeys.sorted), sortMappedKeyEntries);
  }

  return 1;
}

int
finishKeyTable (KeyTableData *ktd) {
  {
    unsigned int context;

    for (context=0; context<ktd->table->keyContextCount; context+=1) {
      KeyContext *ctx = &ktd->table->keyContextTable[context];

      if (!prepareKeyBindings(ctx)) return 0;
      if (!prepareHotkeyEntries(ctx)) return 0;
      if (!prepareMappedKeyEntries(ctx)) return 0;
    }
  }

  if (!setDefaultKeyContextProperties(ktd)) return 0;
  qsort(ktd->table->keyNameTable, ktd->table->keyNameCount, sizeof(*ktd->table->keyNameTable), sortKeyValues);
  resetKeyTable(ktd->table);
  return 1;
}

KeyTable *
compileKeyTable (const char *name, KEY_NAME_TABLES_REFERENCE keys) {
  KeyTable *table = NULL;

  if (setGlobalTableVariables(KEY_TABLE_EXTENSION, KEY_SUBTABLE_EXTENSION)) {
    KeyTableData ktd;
    memset(&ktd, 0, sizeof(ktd));

    ktd.context = KTB_CTX_DEFAULT;

    if ((ktd.table = malloc(sizeof(*ktd.table)))) {
      ktd.table->title = NULL;

      ktd.table->noteTable = NULL;
      ktd.table->noteCount = 0;

      ktd.table->keyNameTable = NULL;
      ktd.table->keyNameCount = 0;

      ktd.table->keyContextTable = NULL;
      ktd.table->keyContextCount = 0;

      ktd.table->pressedKeys = NULL;
      ktd.table->pressedSize = 0;
      ktd.table->pressedCount = 0;

      if (allocateKeyNameTable(&ktd, keys)) {
        if (allocateCommandTable(&ktd)) {
          if (processDataFile(name, processKeyTableLine, &ktd)) {
            if (finishKeyTable(&ktd)) {
              table = ktd.table;
              ktd.table = NULL;
            }
          }

          if (ktd.commandTable) free(ktd.commandTable);
        }
      }

      if (ktd.table) destroyKeyTable(ktd.table);
    } else {
      logMallocError();
    }
  }

  return table;
}

void
destroyKeyTable (KeyTable *table) {
  while (table->noteCount) free(table->noteTable[--table->noteCount]);

  while (table->keyContextCount) {
    KeyContext *ctx = &table->keyContextTable[--table->keyContextCount];

    if (ctx->title) free(ctx->title);

    if (ctx->keyBindings.table) free(ctx->keyBindings.table);
    if (ctx->keyBindings.sorted) free(ctx->keyBindings.sorted);

    if (ctx->hotkeys.table) free(ctx->hotkeys.table);
    if (ctx->hotkeys.sorted) free(ctx->hotkeys.sorted);

    if (ctx->mappedKeys.table) free(ctx->mappedKeys.table);
    if (ctx->mappedKeys.sorted) free(ctx->mappedKeys.sorted);
  }

  if (table->keyContextTable) free(table->keyContextTable);
  if (table->keyNameTable) free(table->keyNameTable);
  if (table->title) free(table->title);
  if (table->pressedKeys) free(table->pressedKeys);
  free(table);
}

char *
ensureKeyTableExtension (const char *path) {
  return ensureFileExtension(path, KEY_TABLE_EXTENSION);
}

char *
makeKeyTablePath (const char *directory, const char *name) {
  return makeFilePath(directory, name, KEY_TABLE_EXTENSION);
}
