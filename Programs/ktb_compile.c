/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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
#include "ktb.h"
#include "ktb_internal.h"

typedef struct {
  const char *name;
  KeyCode code;
} KeyNameEntry;

static const KeyNameEntry keyNameTable[] = {
  {"LETTER_A", KEY_LETTER_A},
  {"LETTER_B", KEY_LETTER_B},
  {"LETTER_C", KEY_LETTER_C},
  {"LETTER_D", KEY_LETTER_D},
  {"LETTER_E", KEY_LETTER_E},
  {"LETTER_F", KEY_LETTER_F},
  {"LETTER_G", KEY_LETTER_G},
  {"LETTER_H", KEY_LETTER_H},
  {"LETTER_I", KEY_LETTER_I},
  {"LETTER_J", KEY_LETTER_J},
  {"LETTER_K", KEY_LETTER_K},
  {"LETTER_L", KEY_LETTER_L},
  {"LETTER_M", KEY_LETTER_M},
  {"LETTER_N", KEY_LETTER_N},
  {"LETTER_O", KEY_LETTER_O},
  {"LETTER_P", KEY_LETTER_P},
  {"LETTER_Q", KEY_LETTER_Q},
  {"LETTER_R", KEY_LETTER_R},
  {"LETTER_S", KEY_LETTER_S},
  {"LETTER_T", KEY_LETTER_T},
  {"LETTER_U", KEY_LETTER_U},
  {"LETTER_V", KEY_LETTER_V},
  {"LETTER_W", KEY_LETTER_W},
  {"LETTER_X", KEY_LETTER_X},
  {"LETTER_Y", KEY_LETTER_Y},
  {"LETTER_Z", KEY_LETTER_Z},

  {"SYMBOL_One_Exclamation", KEY_SYMBOL_One_Exclamation},
  {"SYMBOL_Two_At", KEY_SYMBOL_Two_At},
  {"SYMBOL_Three_Number", KEY_SYMBOL_Three_Number},
  {"SYMBOL_Four_Dollar", KEY_SYMBOL_Four_Dollar},
  {"SYMBOL_Five_Percent", KEY_SYMBOL_Five_Percent},
  {"SYMBOL_Six_Circumflex", KEY_SYMBOL_Six_Circumflex},
  {"SYMBOL_Seven_Ampersand", KEY_SYMBOL_Seven_Ampersand},
  {"SYMBOL_Eight_Asterisk", KEY_SYMBOL_Eight_Asterisk},
  {"SYMBOL_Nine_LeftParenthesis", KEY_SYMBOL_Nine_LeftParenthesis},
  {"SYMBOL_Zero_RightParenthesis", KEY_SYMBOL_Zero_RightParenthesis},

  {"SYMBOL_Grave_Tilde", KEY_SYMBOL_Grave_Tilde},
  {"SYMBOL_Backslash_Bar", KEY_SYMBOL_Backslash_Bar},
  {"SYMBOL_Minus_Underscore", KEY_SYMBOL_Minus_Underscore},
  {"SYMBOL_Equals_Plus", KEY_SYMBOL_Equals_Plus},
  {"SYMBOL_LeftBracket_LeftBrace", KEY_SYMBOL_LeftBracket_LeftBrace},
  {"SYMBOL_RightBracket_RightBrace", KEY_SYMBOL_RightBracket_RightBrace},
  {"SYMBOL_Semicolon_Colon", KEY_SYMBOL_Semicolon_Colon},
  {"SYMBOL_Apostrophe_Quote", KEY_SYMBOL_Apostrophe_Quote},
  {"SYMBOL_Comma_Less", KEY_SYMBOL_Comma_Less},
  {"SYMBOL_Period_Greater", KEY_SYMBOL_Period_Greater},
  {"SYMBOL_Slash_Question", KEY_SYMBOL_Slash_Question},

  {"FUNCTION_Escape", KEY_FUNCTION_Escape},
  {"FUNCTION_Enter", KEY_FUNCTION_Enter},
  {"FUNCTION_Space", KEY_FUNCTION_Space},
  {"FUNCTION_Tab", KEY_FUNCTION_Tab},
  {"FUNCTION_DeleteBackward", KEY_FUNCTION_DeleteBackward},

  {"FUNCTION_F1", KEY_FUNCTION_F1},
  {"FUNCTION_F2", KEY_FUNCTION_F2},
  {"FUNCTION_F3", KEY_FUNCTION_F3},
  {"FUNCTION_F4", KEY_FUNCTION_F4},
  {"FUNCTION_F5", KEY_FUNCTION_F5},
  {"FUNCTION_F6", KEY_FUNCTION_F6},
  {"FUNCTION_F7", KEY_FUNCTION_F7},
  {"FUNCTION_F8", KEY_FUNCTION_F8},
  {"FUNCTION_F9", KEY_FUNCTION_F9},
  {"FUNCTION_F10", KEY_FUNCTION_F10},
  {"FUNCTION_F11", KEY_FUNCTION_F11},
  {"FUNCTION_F12", KEY_FUNCTION_F12},
  {"FUNCTION_F13", KEY_FUNCTION_F13},
  {"FUNCTION_F14", KEY_FUNCTION_F14},
  {"FUNCTION_F15", KEY_FUNCTION_F15},
  {"FUNCTION_F16", KEY_FUNCTION_F16},
  {"FUNCTION_F17", KEY_FUNCTION_F17},
  {"FUNCTION_F18", KEY_FUNCTION_F18},
  {"FUNCTION_F19", KEY_FUNCTION_F19},
  {"FUNCTION_F20", KEY_FUNCTION_F20},
  {"FUNCTION_F21", KEY_FUNCTION_F21},
  {"FUNCTION_F22", KEY_FUNCTION_F22},
  {"FUNCTION_F23", KEY_FUNCTION_F23},
  {"FUNCTION_F24", KEY_FUNCTION_F24},

  {"FUNCTION_Insert", KEY_FUNCTION_Insert},
  {"FUNCTION_DeleteForward", KEY_FUNCTION_DeleteForward},
  {"FUNCTION_Home", KEY_FUNCTION_Home},
  {"FUNCTION_End", KEY_FUNCTION_End},
  {"FUNCTION_PageUp", KEY_FUNCTION_PageUp},
  {"FUNCTION_PageDown", KEY_FUNCTION_PageDown},

  {"FUNCTION_ArrowUp", KEY_FUNCTION_ArrowUp},
  {"FUNCTION_ArrowDown", KEY_FUNCTION_ArrowDown},
  {"FUNCTION_ArrowLeft", KEY_FUNCTION_ArrowLeft},
  {"FUNCTION_ArrowRight", KEY_FUNCTION_ArrowRight},

  {"FUNCTION_PrintScreen", KEY_FUNCTION_PrintScreen},
  {"FUNCTION_SystemRequest", KEY_FUNCTION_SystemRequest},
  {"FUNCTION_Pause", KEY_FUNCTION_Pause},

  {"FUNCTION_ShiftLeft", KEY_FUNCTION_ShiftLeft},
  {"FUNCTION_ShiftRight", KEY_FUNCTION_ShiftRight},
  {"FUNCTION_ControlLeft", KEY_FUNCTION_ControlLeft},
  {"FUNCTION_ControlRight", KEY_FUNCTION_ControlRight},
  {"FUNCTION_AltLeft", KEY_FUNCTION_AltLeft},
  {"FUNCTION_AltRight", KEY_FUNCTION_AltRight},
  {"FUNCTION_GuiLeft", KEY_FUNCTION_GuiLeft},
  {"FUNCTION_GuiRight", KEY_FUNCTION_GuiRight},
  {"FUNCTION_Application", KEY_FUNCTION_Application},

  {"LOCK_Capitals", KEY_LOCK_Capitals},
  {"LOCK_Scroll", KEY_LOCK_Scroll},

  {"LOCKING_Capitals", KEY_LOCKING_Capitals},
  {"LOCKING_Scroll", KEY_LOCKING_Scroll},
  {"LOCKING_Numbers", KEY_LOCKING_Numbers},

  {"KEYPAD_NumLock_Clear", KEY_KEYPAD_NumLock_Clear},
  {"KEYPAD_Slash", KEY_KEYPAD_Slash},
  {"KEYPAD_Asterisk", KEY_KEYPAD_Asterisk},
  {"KEYPAD_Minus", KEY_KEYPAD_Minus},
  {"KEYPAD_Plus", KEY_KEYPAD_Plus},
  {"KEYPAD_Enter", KEY_KEYPAD_Enter},
  {"KEYPAD_One_End", KEY_KEYPAD_One_End},
  {"KEYPAD_Two_ArrowDown", KEY_KEYPAD_Two_ArrowDown},
  {"KEYPAD_Three_PageDown", KEY_KEYPAD_Three_PageDown},
  {"KEYPAD_Four_ArrowLeft", KEY_KEYPAD_Four_ArrowLeft},
  {"KEYPAD_Five", KEY_KEYPAD_Five},
  {"KEYPAD_Six_ArrowRight", KEY_KEYPAD_Six_ArrowRight},
  {"KEYPAD_Seven_Home", KEY_KEYPAD_Seven_Home},
  {"KEYPAD_Eight_ArrowUp", KEY_KEYPAD_Eight_ArrowUp},
  {"KEYPAD_Nine_PageUp", KEY_KEYPAD_Nine_PageUp},
  {"KEYPAD_Zero_Insert", KEY_KEYPAD_Zero_Insert},
  {"KEYPAD_Period_Delete", KEY_KEYPAD_Period_Delete},

  {"KEYPAD_Equals", KEY_KEYPAD_Equals},
  {"KEYPAD_LeftParenthesis", KEY_KEYPAD_LeftParenthesis},
  {"KEYPAD_RightParenthesis", KEY_KEYPAD_RightParenthesis},
  {"KEYPAD_LeftBrace", KEY_KEYPAD_LeftBrace},
  {"KEYPAD_RightBrace", KEY_KEYPAD_RightBrace},
  {"KEYPAD_Modulo", KEY_KEYPAD_Modulo},
  {"KEYPAD_BitwiseAnd", KEY_KEYPAD_BitwiseAnd},
  {"KEYPAD_BitwiseOr", KEY_KEYPAD_BitwiseOr},
  {"KEYPAD_BitwiseXor", KEY_KEYPAD_BitwiseXor},
  {"KEYPAD_Less", KEY_KEYPAD_Less},
  {"KEYPAD_Greater", KEY_KEYPAD_Greater},
  {"KEYPAD_BooleanAnd", KEY_KEYPAD_BooleanAnd},
  {"KEYPAD_BooleanOr", KEY_KEYPAD_BooleanOr},
  {"KEYPAD_BooleanXor", KEY_KEYPAD_BooleanXor},
  {"KEYPAD_BooleanNot", KEY_KEYPAD_BooleanNot},

  {"KEYPAD_Backspace", KEY_KEYPAD_Backspace},
  {"KEYPAD_Space", KEY_KEYPAD_Space},
  {"KEYPAD_Tab", KEY_KEYPAD_Tab},
  {"KEYPAD_Comma", KEY_KEYPAD_Comma},
  {"KEYPAD_Colon", KEY_KEYPAD_Colon},
  {"KEYPAD_Number", KEY_KEYPAD_Number},
  {"KEYPAD_At", KEY_KEYPAD_At},

  {"KEYPAD_A", KEY_KEYPAD_A},
  {"KEYPAD_B", KEY_KEYPAD_B},
  {"KEYPAD_C", KEY_KEYPAD_C},
  {"KEYPAD_D", KEY_KEYPAD_D},
  {"KEYPAD_E", KEY_KEYPAD_E},
  {"KEYPAD_F", KEY_KEYPAD_F},

  {"KEYPAD_00", KEY_KEYPAD_00},
  {"KEYPAD_000", KEY_KEYPAD_000},
  {"KEYPAD_ThousandsSeparator", KEY_KEYPAD_ThousandsSeparator},
  {"KEYPAD_DecimalSeparator", KEY_KEYPAD_DecimalSeparator},
  {"KEYPAD_CurrencyUnit", KEY_KEYPAD_CurrencyUnit},
  {"KEYPAD_CurrencySubunit", KEY_KEYPAD_CurrencySubunit},

  {"FUNCTION_Power", KEY_FUNCTION_Power},
  {"FUNCTION_Sleep", KEY_FUNCTION_Sleep},
  {"FUNCTION_Wakeup", KEY_FUNCTION_Wakeup},
  {"FUNCTION_Stop", KEY_FUNCTION_Stop},

  {"FUNCTION_Help", KEY_FUNCTION_Help},
  {"FUNCTION_Find", KEY_FUNCTION_Find},

  {"FUNCTION_Menu", KEY_FUNCTION_Menu},
  {"FUNCTION_Select", KEY_FUNCTION_Select},
  {"FUNCTION_Again", KEY_FUNCTION_Again},
  {"FUNCTION_Execute", KEY_FUNCTION_Execute},

  {"FUNCTION_Copy", KEY_FUNCTION_Copy},
  {"FUNCTION_Cut", KEY_FUNCTION_Cut},
  {"FUNCTION_Paste", KEY_FUNCTION_Paste},
  {"FUNCTION_Undo", KEY_FUNCTION_Undo},

  {"FUNCTION_Mute", KEY_FUNCTION_Mute},
  {"FUNCTION_VolumeUp", KEY_FUNCTION_VolumeUp},
  {"FUNCTION_VolumeDown", KEY_FUNCTION_VolumeDown},

  {"KEYPAD_Clear", KEY_KEYPAD_Clear},
  {"KEYPAD_ClearEntry", KEY_KEYPAD_ClearEntry},
  {"KEYPAD_PlusMinus", KEY_KEYPAD_PlusMinus},

  {"KEYPAD_MemoryClear", KEY_KEYPAD_MemoryClear},
  {"KEYPAD_MemoryStore", KEY_KEYPAD_MemoryStore},
  {"KEYPAD_MemoryRecall", KEY_KEYPAD_MemoryRecall},
  {"KEYPAD_MemoryAdd", KEY_KEYPAD_MemoryAdd},
  {"KEYPAD_MemorySubtract", KEY_KEYPAD_MemorySubtract},
  {"KEYPAD_MemoryMultiply", KEY_KEYPAD_MemoryMultiply},
  {"KEYPAD_MemoryDivide", KEY_KEYPAD_MemoryDivide},

  {"KEYPAD_Binary", KEY_KEYPAD_Binary},
  {"KEYPAD_Octal", KEY_KEYPAD_Octal},
  {"KEYPAD_Decimal", KEY_KEYPAD_Decimal},
  {"KEYPAD_Hexadecimal", KEY_KEYPAD_Hexadecimal},

  {"FUNCTION_Cancel", KEY_FUNCTION_Cancel},
  {"FUNCTION_Clear", KEY_FUNCTION_Clear},
  {"FUNCTION_Prior", KEY_FUNCTION_Prior},
  {"FUNCTION_Return", KEY_FUNCTION_Return},
  {"FUNCTION_Separator", KEY_FUNCTION_Separator},
  {"FUNCTION_Out", KEY_FUNCTION_Out},
  {"FUNCTION_Oper", KEY_FUNCTION_Oper},
  {"FUNCTION_Clear_Again", KEY_FUNCTION_Clear_Again},
  {"FUNCTION_CrSel_Props", KEY_FUNCTION_CrSel_Props},
  {"FUNCTION_ExSel", KEY_FUNCTION_ExSel},
          
  {NULL, KEY_SPECIAL_None}
};

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
  DataOperand name = {
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
      if (!parseKeyName(file, &code, characters, count,ktd)) return 0;

      if (BITMASK_TEST(key->modifiers, code)) {
        reportDataError(file, "duplicate modifier key name: %.*" PRIws, count, characters);
        return 0;
      }
      BITMASK_SET(key->modifiers, code);

      length -= count + 1;
      characters = end + 1;
    }
  }

  if (!length) {
    reportDataError(file, "missing key name");
    return 0;
  }
  if (!parseKeyName(file, &key->code, characters, length, ktd)) return 0;

  if (BITMASK_TEST(key->modifiers, key->code)) {
    reportDataError(file, "duplicate key name: %.*" PRIws, length, characters);
    return 0;
  }

  return 1;
}

static int
getKeyOperand (DataFile *file, KeyCombination *key, KeyTableData *ktd) {
  DataOperand names;

  if (getDataOperand(file, &names, "key combination")) {
    if (parseKeyCombination(file, key, names.characters, names.length,ktd)) return 1;
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
  DataOperand name = {
    .characters = characters,
    .length = length
  };
  const CommandEntry **command = bsearch(&name, ktd->commandTable, ktd->commandCount, sizeof(*ktd->commandTable), compareToCommandName);

  if (command) {
    *value = (*command)->code;
    return 1;
  }

  reportDataError(file, "unknown command name: %.*" PRIws, length, characters);
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
  return strcmp((*key1)->name, (*key2)->name);
}

static int
allocateKeyNameTable (KeyTableData *ktd) {
  {
    const KeyNameEntry *key = keyNameTable;

    ktd->keyNameCount = 0;
    while (key->name) {
      ktd->keyNameCount += 1;
      key += 1;
    }
  }

  if ((ktd->keyNameTable = malloc(ktd->keyNameCount * sizeof(*ktd->keyNameTable)))) {
    {
      const KeyNameEntry *key = keyNameTable;
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
  return strcmp((*command1)->name, (*command2)->name);
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
compileKeyTable (const char *name) {
  KeyTable *table = NULL;
  KeyTableData ktd;

  memset(&ktd, 0, sizeof(ktd));

  ktd.bindingsTable = NULL;
  ktd.bindingsSize = 0;
  ktd.bindingsCount = 0;

  if (allocateKeyNameTable(&ktd)) {
    if (allocateCommandTable(&ktd)) {
      if ((ktd.area = newDataArea())) {
        if (allocateDataItem(ktd.area, NULL, sizeof(KeyTableHeader), __alignof__(KeyTableHeader))) {
          if (processDataFile(name, processKeyTableLine, &ktd)) {
            if (saveKeyBindings(&ktd)) {
              if ((table = malloc(sizeof(*table)))) {
                table->header.fields = getKeyTableHeader(&ktd);
                table->size = getDataSize(ktd.area);
                resetDataArea(ktd.area);
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
