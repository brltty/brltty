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
  KeyTable *table;

  const CommandEntry **commandTable;
  unsigned int commandCount;

  unsigned char context;

  unsigned hideRequested:1;
  unsigned hideImposed:1;
} KeyTableData;

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
      LogError("realloc");
      return NULL;
    }
    ktd->table->keyContextTable = newTable;

    while (ktd->table->keyContextCount < newCount) {
      KeyContext *ctx = &ktd->table->keyContextTable[ktd->table->keyContextCount++];
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

  return &ktd->table->keyContextTable[context];
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

static const KeyNameEntry **
findKeyName (const wchar_t *characters, int length, KeyTableData *ktd) {
  const DataOperand name = {
    .characters = characters,
    .length = length
  };

  return bsearch(&name, ktd->table->keyNameTable, ktd->table->keyNameCount, sizeof(*ktd->table->keyNameTable), searchKeyName);
}

static int
parseKeyName (DataFile *file, unsigned char *set, unsigned char *key, const wchar_t *characters, int length, KeyTableData *ktd) {
  const KeyNameEntry **kne = findKeyName(characters, length, ktd);

  if (kne) {
    *set = (*kne)->set;
    *key = (*kne)->key;
    return 1;
  }

  reportDataError(file, "unknown key: %.*" PRIws, length, characters);
  return 0;
}

static int
parseKeyCombination (DataFile *file, KeyCombination *combination, const wchar_t *characters, int length, KeyTableData *ktd) {
  KeySet modifiers;
  int immediate;

  removeAllKeys(&modifiers);
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

      if (!addKey(&modifiers, key)) {
        reportDataError(file, "duplicate modifier key: %.*" PRIws, count, characters);
        return 0;
      }

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
  if (!parseKeyName(file, &combination->set, &combination->key, characters, length, ktd)) return 0;

  if (!combination->set) {
    if (testKey(&modifiers, combination->key)) {
      reportDataError(file, "duplicate key: %.*" PRIws, length, characters);
      return 0;
    }

    if (!immediate) {
      addKey(&modifiers, combination->key);
      combination->key = 0;
    }
  }

  if (modifiers.count > ARRAY_COUNT(combination->modifiers.keys)) {
    reportDataError(file, "too many modifier keys");
    return 0;
  }

  copyKeySetMask(combination->modifiers.mask, modifiers.mask);
  memcpy(combination->modifiers.keys, modifiers.keys,
         (combination->modifiers.count = modifiers.count));

  return 1;
}

static int
getKeysOperand (DataFile *file, KeyCombination *key, KeyTableData *ktd) {
  DataString names;

  if (getDataString(file, &names, 1, "key combination")) {
    if (parseKeyCombination(file, key, names.characters, names.length, ktd)) return 1;
  }

  return 0;
}

static int
getMappedKeyOperand (DataFile *file, unsigned char *key, KeyTableData *ktd) {
  DataString name;

  if (getDataString(file, &name, 1, "mapped key name")) {
    unsigned char set;

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
parseCommandOperand (DataFile *file, int *value, const wchar_t *characters, int length, KeyTableData *ktd) {
  int toggleDone = 0;
  int offsetDone = 0;

  const wchar_t *end = wmemchr(characters, WC_C('+'), length);
  const CommandEntry **command;

  {
    const DataOperand name = {
      .characters = characters,
      .length = end? end-characters: length
    };

    if (!name.length) {
      reportDataError(file, "missing command name");
      return 0;
    }

    if (!(command = bsearch(&name, ktd->commandTable, ktd->commandCount, sizeof(*ktd->commandTable), searchCommandName))) {
      reportDataError(file, "unknown command name: %.*" PRIws, name.length, name.characters);
      return 0;
    }
  }
  *value = (*command)->code;

  while (end) {
    DataOperand modifier;

    if (!(length -= end - characters + 1)) {
      reportDataError(file, "missing command modifier");
      return 0;
    }

    characters = end + 1;
    end = wmemchr(characters, WC_C('+'), length);

    modifier.characters = characters;
    modifier.length = end? end-characters: length;

    if ((*command)->isToggle && !toggleDone) {
      if (isKeyword(WS_C("on"), modifier.characters, modifier.length)) {
        *value |= BRL_FLG_TOGGLE_ON;
      }  else if (isKeyword(WS_C("off"), modifier.characters, modifier.length)) {
        *value |= BRL_FLG_TOGGLE_OFF;
      } else {
        goto notToggle;
      }

      toggleDone = 1;
      continue;
    }
  notToggle:

    if ((*command)->isMotion) {
      if (isKeyword(WS_C("route"), modifier.characters, modifier.length) &&
          !(*value & BRL_FLG_MOTION_ROUTE)) {
        *value |= BRL_FLG_MOTION_ROUTE;
        continue;
      }

      if ((*command)->isRow) {
        if (isKeyword(WS_C("scaled"), modifier.characters, modifier.length) &&
            !(*value & BRL_FLG_LINE_SCALED)) {
          *value |= BRL_FLG_LINE_SCALED;
          continue;
        }

        if (isKeyword(WS_C("toleft"), modifier.characters, modifier.length) &&
            !(*value & BRL_FLG_LINE_TOLEFT)) {
          *value |= BRL_FLG_LINE_TOLEFT;
          continue;
        }
      }
    }

    if ((*command)->isOffset && !offsetDone) {
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
    return 0;
  }

  return 1;
}

static int
getCommandOperand (DataFile *file, int *value, KeyTableData *ktd) {
  DataString name;

  if (getDataString(file, &name, 1, "command name")) {
    if (parseCommandOperand(file, value, name.characters, name.length, ktd)) return 1;
  }

  return 0;
}

static int processKeyTableLine (DataFile *file, void *data);

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
    binding->hidden = hideBindings(ktd);

    if (getKeysOperand(file, &binding->keys, ktd)) {
      if (getCommandOperand(file, &binding->command, ktd)) {
        if (!binding->keys.set)
          if (getCommandEntry(binding->command)->isColumn)
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
  DataString context;

  if (getDataString(file, &context, 1, "context name or number")) {
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
  if (!ctx) return 0;

  {
    unsigned char key;

    if (getMappedKeyOperand(file, &key, ktd)) {
      unsigned char function;

      if (getKeyboardFunctionOperand(file, &function, ktd)) {
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
processNoteOperands (DataFile *file, void *data) {
  KeyTableData *ktd = data;
  DataOperand note;

  if (getDataText(file, &note, "note text")) {
    if (!hideBindings(ktd)) {
      unsigned int newCount = ktd->table->noteCount + 1;
      wchar_t **newTable = realloc(ktd->table->noteTable, ARRAY_SIZE(newTable, newCount));

      if (!newTable) {
        LogError("realloc");
        return 0;
      }

      ktd->table->noteTable = newTable;

      {
        wchar_t *noteString = malloc(ARRAY_SIZE(*ktd->table->noteTable, note.length+1));

        if (!noteString) {
          LogError("malloc");
          return 0;
        }

        wmemcpy(noteString, note.characters, note.length);
        noteString[note.length] = 0;
        ktd->table->noteTable[ktd->table->noteCount++] = noteString;
        return 1;
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
    unsigned char function;

    if (getKeyboardFunctionOperand(file, &function, ktd)) {
      ctx->superimposedBits |= keyboardFunctionTable[function].bit;
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
      LogError("malloc");
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

static int
sortKeyBindings (const void *element1, const void *element2) {
  const KeyBinding *const *binding1 = element1;
  const KeyBinding *const *binding2 = element2;
  return compareKeyBindings(*binding1, *binding2);
}

static int
prepareKeyBindings (KeyContext *ctx) {
  if (ctx->keyBindingCount < ctx->keyBindingsSize) {
    if (ctx->keyBindingCount) {
      KeyBinding *newTable = realloc(ctx->keyBindingTable, ARRAY_SIZE(newTable, ctx->keyBindingCount));

      if (!newTable) {
        LogError("realloc");
        return 0;
      }

      ctx->keyBindingTable = newTable;
    } else {
      free(ctx->keyBindingTable);
      ctx->keyBindingTable = NULL;
    }

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

int
finishKeyTable (KeyTableData *ktd) {
  {
    unsigned int context;

    for (context=0; context<ktd->table->keyContextCount; context+=1) {
      KeyContext *ctx = &ktd->table->keyContextTable[context];

      if (!prepareKeyBindings(ctx)) return 0;
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
  KeyTableData ktd;

  memset(&ktd, 0, sizeof(ktd));
  ktd.context = BRL_CTX_DEFAULT;

  if ((ktd.table = malloc(sizeof(*ktd.table)))) {
    ktd.table->title = NULL;
    ktd.table->noteTable = NULL;
    ktd.table->noteCount = 0;
    ktd.table->keyNameTable = NULL;
    ktd.table->keyNameCount = 0;
    ktd.table->keyContextTable = NULL;
    ktd.table->keyContextCount = 0;

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
    LogError("malloc");
  }

  return table;
}

void
destroyKeyTable (KeyTable *table) {
  while (table->noteCount) free(table->noteTable[--table->noteCount]);

  while (table->keyContextCount) {
    KeyContext *ctx = &table->keyContextTable[--table->keyContextCount];

    if (ctx->name) free(ctx->name);
    if (ctx->keyBindingTable) free(ctx->keyBindingTable);
    if (ctx->sortedKeyBindings) free(ctx->sortedKeyBindings);
    if (ctx->keyMap) free(ctx->keyMap);
  }

  if (table->keyContextTable) free(table->keyContextTable);
  if (table->keyNameTable) free(table->keyNameTable);
  if (table->title) free(table->title);
  free(table);
}

char *
ensureKeyTableExtension (const char *path) {
  return ensureExtension(path, KEY_TABLE_EXTENSION);
}
