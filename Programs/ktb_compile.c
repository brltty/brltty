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
  DataArea *area;

  KeyBinding *bindingsTable;
  unsigned int bindingsSize;
  unsigned int bindingsCount;
} KeyTableData;

static inline KeyTableHeader *
getKeyTableHeader (KeyTableData *ktd) {
  return getDataItem(ktd->area, 0);
}

static int
parseKeyOperand (DataFile *file, KeyCode *code, const wchar_t *characters, int length) {
  typedef struct {
    const wchar_t *name;
    KeyCode code;
  } KeyEntry;

  static const KeyEntry keyTable[] = {
    {WS_C("LETTER_A"), KEY_LETTER_A},
    {WS_C("LETTER_B"), KEY_LETTER_B},
    {WS_C("LETTER_C"), KEY_LETTER_C},
    {WS_C("LETTER_D"), KEY_LETTER_D},
    {WS_C("LETTER_E"), KEY_LETTER_E},
    {WS_C("LETTER_F"), KEY_LETTER_F},
    {WS_C("LETTER_G"), KEY_LETTER_G},
    {WS_C("LETTER_H"), KEY_LETTER_H},
    {WS_C("LETTER_I"), KEY_LETTER_I},
    {WS_C("LETTER_J"), KEY_LETTER_J},
    {WS_C("LETTER_K"), KEY_LETTER_K},
    {WS_C("LETTER_L"), KEY_LETTER_L},
    {WS_C("LETTER_M"), KEY_LETTER_M},
    {WS_C("LETTER_N"), KEY_LETTER_N},
    {WS_C("LETTER_O"), KEY_LETTER_O},
    {WS_C("LETTER_P"), KEY_LETTER_P},
    {WS_C("LETTER_Q"), KEY_LETTER_Q},
    {WS_C("LETTER_R"), KEY_LETTER_R},
    {WS_C("LETTER_S"), KEY_LETTER_S},
    {WS_C("LETTER_T"), KEY_LETTER_T},
    {WS_C("LETTER_U"), KEY_LETTER_U},
    {WS_C("LETTER_V"), KEY_LETTER_V},
    {WS_C("LETTER_W"), KEY_LETTER_W},
    {WS_C("LETTER_X"), KEY_LETTER_X},
    {WS_C("LETTER_Y"), KEY_LETTER_Y},
    {WS_C("LETTER_Z"), KEY_LETTER_Z},

    {WS_C("SYMBOL_One_Exclamation"), KEY_SYMBOL_One_Exclamation},
    {WS_C("SYMBOL_Two_At"), KEY_SYMBOL_Two_At},
    {WS_C("SYMBOL_Three_Number"), KEY_SYMBOL_Three_Number},
    {WS_C("SYMBOL_Four_Dollar"), KEY_SYMBOL_Four_Dollar},
    {WS_C("SYMBOL_Five_Percent"), KEY_SYMBOL_Five_Percent},
    {WS_C("SYMBOL_Six_Circumflex"), KEY_SYMBOL_Six_Circumflex},
    {WS_C("SYMBOL_Seven_Ampersand"), KEY_SYMBOL_Seven_Ampersand},
    {WS_C("SYMBOL_Eight_Asterisk"), KEY_SYMBOL_Eight_Asterisk},
    {WS_C("SYMBOL_Nine_LeftParenthesis"), KEY_SYMBOL_Nine_LeftParenthesis},
    {WS_C("SYMBOL_Zero_RightParenthesis"), KEY_SYMBOL_Zero_RightParenthesis},

    {WS_C("SYMBOL_Grave_Tilde"), KEY_SYMBOL_Grave_Tilde},
    {WS_C("SYMBOL_Backslash_Bar"), KEY_SYMBOL_Backslash_Bar},
    {WS_C("SYMBOL_Minus_Underscore"), KEY_SYMBOL_Minus_Underscore},
    {WS_C("SYMBOL_Equals_Plus"), KEY_SYMBOL_Equals_Plus},
    {WS_C("SYMBOL_LeftBracket_LeftBrace"), KEY_SYMBOL_LeftBracket_LeftBrace},
    {WS_C("SYMBOL_RightBracket_RightBrace"), KEY_SYMBOL_RightBracket_RightBrace},
    {WS_C("SYMBOL_Semicolon_Colon"), KEY_SYMBOL_Semicolon_Colon},
    {WS_C("SYMBOL_Apostrophe_Quote"), KEY_SYMBOL_Apostrophe_Quote},
    {WS_C("SYMBOL_Comma_Less"), KEY_SYMBOL_Comma_Less},
    {WS_C("SYMBOL_Period_Greater"), KEY_SYMBOL_Period_Greater},
    {WS_C("SYMBOL_Slash_Question"), KEY_SYMBOL_Slash_Question},

    {WS_C("FUNCTION_Escape"), KEY_FUNCTION_Escape},
    {WS_C("FUNCTION_Enter"), KEY_FUNCTION_Enter},
    {WS_C("FUNCTION_Space"), KEY_FUNCTION_Space},
    {WS_C("FUNCTION_Tab"), KEY_FUNCTION_Tab},
    {WS_C("FUNCTION_DeleteBackward"), KEY_FUNCTION_DeleteBackward},

    {WS_C("FUNCTION_F1"), KEY_FUNCTION_F1},
    {WS_C("FUNCTION_F2"), KEY_FUNCTION_F2},
    {WS_C("FUNCTION_F3"), KEY_FUNCTION_F3},
    {WS_C("FUNCTION_F4"), KEY_FUNCTION_F4},
    {WS_C("FUNCTION_F5"), KEY_FUNCTION_F5},
    {WS_C("FUNCTION_F6"), KEY_FUNCTION_F6},
    {WS_C("FUNCTION_F7"), KEY_FUNCTION_F7},
    {WS_C("FUNCTION_F8"), KEY_FUNCTION_F8},
    {WS_C("FUNCTION_F9"), KEY_FUNCTION_F9},
    {WS_C("FUNCTION_F10"), KEY_FUNCTION_F10},
    {WS_C("FUNCTION_F11"), KEY_FUNCTION_F11},
    {WS_C("FUNCTION_F12"), KEY_FUNCTION_F12},
    {WS_C("FUNCTION_F13"), KEY_FUNCTION_F13},
    {WS_C("FUNCTION_F14"), KEY_FUNCTION_F14},
    {WS_C("FUNCTION_F15"), KEY_FUNCTION_F15},
    {WS_C("FUNCTION_F16"), KEY_FUNCTION_F16},
    {WS_C("FUNCTION_F17"), KEY_FUNCTION_F17},
    {WS_C("FUNCTION_F18"), KEY_FUNCTION_F18},
    {WS_C("FUNCTION_F19"), KEY_FUNCTION_F19},
    {WS_C("FUNCTION_F20"), KEY_FUNCTION_F20},
    {WS_C("FUNCTION_F21"), KEY_FUNCTION_F21},
    {WS_C("FUNCTION_F22"), KEY_FUNCTION_F22},
    {WS_C("FUNCTION_F23"), KEY_FUNCTION_F23},
    {WS_C("FUNCTION_F24"), KEY_FUNCTION_F24},

    {WS_C("FUNCTION_Insert"), KEY_FUNCTION_Insert},
    {WS_C("FUNCTION_DeleteForward"), KEY_FUNCTION_DeleteForward},
    {WS_C("FUNCTION_Home"), KEY_FUNCTION_Home},
    {WS_C("FUNCTION_End"), KEY_FUNCTION_End},
    {WS_C("FUNCTION_PageUp"), KEY_FUNCTION_PageUp},
    {WS_C("FUNCTION_PageDown"), KEY_FUNCTION_PageDown},

    {WS_C("FUNCTION_ArrowUp"), KEY_FUNCTION_ArrowUp},
    {WS_C("FUNCTION_ArrowDown"), KEY_FUNCTION_ArrowDown},
    {WS_C("FUNCTION_ArrowLeft"), KEY_FUNCTION_ArrowLeft},
    {WS_C("FUNCTION_ArrowRight"), KEY_FUNCTION_ArrowRight},

    {WS_C("FUNCTION_PrintScreen"), KEY_FUNCTION_PrintScreen},
    {WS_C("FUNCTION_SystemRequest"), KEY_FUNCTION_SystemRequest},
    {WS_C("FUNCTION_Pause"), KEY_FUNCTION_Pause},

    {WS_C("FUNCTION_ShiftLeft"), KEY_FUNCTION_ShiftLeft},
    {WS_C("FUNCTION_ShiftRight"), KEY_FUNCTION_ShiftRight},
    {WS_C("FUNCTION_ControlLeft"), KEY_FUNCTION_ControlLeft},
    {WS_C("FUNCTION_ControlRight"), KEY_FUNCTION_ControlRight},
    {WS_C("FUNCTION_AltLeft"), KEY_FUNCTION_AltLeft},
    {WS_C("FUNCTION_AltRight"), KEY_FUNCTION_AltRight},
    {WS_C("FUNCTION_GuiLeft"), KEY_FUNCTION_GuiLeft},
    {WS_C("FUNCTION_GuiRight"), KEY_FUNCTION_GuiRight},
    {WS_C("FUNCTION_Application"), KEY_FUNCTION_Application},

    {WS_C("LOCK_Capitals"), KEY_LOCK_Capitals},
    {WS_C("LOCK_Scroll"), KEY_LOCK_Scroll},

    {WS_C("LOCKING_Capitals"), KEY_LOCKING_Capitals},
    {WS_C("LOCKING_Scroll"), KEY_LOCKING_Scroll},
    {WS_C("LOCKING_Numbers"), KEY_LOCKING_Numbers},

    {WS_C("KEYPAD_NumLock_Clear"), KEY_KEYPAD_NumLock_Clear},
    {WS_C("KEYPAD_Slash"), KEY_KEYPAD_Slash},
    {WS_C("KEYPAD_Asterisk"), KEY_KEYPAD_Asterisk},
    {WS_C("KEYPAD_Minus"), KEY_KEYPAD_Minus},
    {WS_C("KEYPAD_Plus"), KEY_KEYPAD_Plus},
    {WS_C("KEYPAD_Enter"), KEY_KEYPAD_Enter},
    {WS_C("KEYPAD_One_End"), KEY_KEYPAD_One_End},
    {WS_C("KEYPAD_Two_ArrowDown"), KEY_KEYPAD_Two_ArrowDown},
    {WS_C("KEYPAD_Three_PageDown"), KEY_KEYPAD_Three_PageDown},
    {WS_C("KEYPAD_Four_ArrowLeft"), KEY_KEYPAD_Four_ArrowLeft},
    {WS_C("KEYPAD_Five"), KEY_KEYPAD_Five},
    {WS_C("KEYPAD_Six_ArrowRight"), KEY_KEYPAD_Six_ArrowRight},
    {WS_C("KEYPAD_Seven_Home"), KEY_KEYPAD_Seven_Home},
    {WS_C("KEYPAD_Eight_ArrowUp"), KEY_KEYPAD_Eight_ArrowUp},
    {WS_C("KEYPAD_Nine_PageUp"), KEY_KEYPAD_Nine_PageUp},
    {WS_C("KEYPAD_Zero_Insert"), KEY_KEYPAD_Zero_Insert},
    {WS_C("KEYPAD_Period_Delete"), KEY_KEYPAD_Period_Delete},

    {WS_C("KEYPAD_Equals"), KEY_KEYPAD_Equals},
    {WS_C("KEYPAD_LeftParenthesis"), KEY_KEYPAD_LeftParenthesis},
    {WS_C("KEYPAD_RightParenthesis"), KEY_KEYPAD_RightParenthesis},
    {WS_C("KEYPAD_LeftBrace"), KEY_KEYPAD_LeftBrace},
    {WS_C("KEYPAD_RightBrace"), KEY_KEYPAD_RightBrace},
    {WS_C("KEYPAD_Modulo"), KEY_KEYPAD_Modulo},
    {WS_C("KEYPAD_BitwiseAnd"), KEY_KEYPAD_BitwiseAnd},
    {WS_C("KEYPAD_BitwiseOr"), KEY_KEYPAD_BitwiseOr},
    {WS_C("KEYPAD_BitwiseXor"), KEY_KEYPAD_BitwiseXor},
    {WS_C("KEYPAD_Less"), KEY_KEYPAD_Less},
    {WS_C("KEYPAD_Greater"), KEY_KEYPAD_Greater},
    {WS_C("KEYPAD_BooleanAnd"), KEY_KEYPAD_BooleanAnd},
    {WS_C("KEYPAD_BooleanOr"), KEY_KEYPAD_BooleanOr},
    {WS_C("KEYPAD_BooleanXor"), KEY_KEYPAD_BooleanXor},
    {WS_C("KEYPAD_BooleanNot"), KEY_KEYPAD_BooleanNot},

    {WS_C("KEYPAD_Backspace"), KEY_KEYPAD_Backspace},
    {WS_C("KEYPAD_Space"), KEY_KEYPAD_Space},
    {WS_C("KEYPAD_Tab"), KEY_KEYPAD_Tab},
    {WS_C("KEYPAD_Comma"), KEY_KEYPAD_Comma},
    {WS_C("KEYPAD_Colon"), KEY_KEYPAD_Colon},
    {WS_C("KEYPAD_Number"), KEY_KEYPAD_Number},
    {WS_C("KEYPAD_At"), KEY_KEYPAD_At},

    {WS_C("KEYPAD_A"), KEY_KEYPAD_A},
    {WS_C("KEYPAD_B"), KEY_KEYPAD_B},
    {WS_C("KEYPAD_C"), KEY_KEYPAD_C},
    {WS_C("KEYPAD_D"), KEY_KEYPAD_D},
    {WS_C("KEYPAD_E"), KEY_KEYPAD_E},
    {WS_C("KEYPAD_F"), KEY_KEYPAD_F},

    {WS_C("KEYPAD_00"), KEY_KEYPAD_00},
    {WS_C("KEYPAD_000"), KEY_KEYPAD_000},
    {WS_C("KEYPAD_ThousandsSeparator"), KEY_KEYPAD_ThousandsSeparator},
    {WS_C("KEYPAD_DecimalSeparator"), KEY_KEYPAD_DecimalSeparator},
    {WS_C("KEYPAD_CurrencyUnit"), KEY_KEYPAD_CurrencyUnit},
    {WS_C("KEYPAD_CurrencySubunit"), KEY_KEYPAD_CurrencySubunit},

    {WS_C("FUNCTION_Power"), KEY_FUNCTION_Power},
    {WS_C("FUNCTION_Sleep"), KEY_FUNCTION_Sleep},
    {WS_C("FUNCTION_Wakeup"), KEY_FUNCTION_Wakeup},
    {WS_C("FUNCTION_Stop"), KEY_FUNCTION_Stop},

    {WS_C("FUNCTION_Help"), KEY_FUNCTION_Help},
    {WS_C("FUNCTION_Find"), KEY_FUNCTION_Find},

    {WS_C("FUNCTION_Menu"), KEY_FUNCTION_Menu},
    {WS_C("FUNCTION_Select"), KEY_FUNCTION_Select},
    {WS_C("FUNCTION_Again"), KEY_FUNCTION_Again},
    {WS_C("FUNCTION_Execute"), KEY_FUNCTION_Execute},

    {WS_C("FUNCTION_Copy"), KEY_FUNCTION_Copy},
    {WS_C("FUNCTION_Cut"), KEY_FUNCTION_Cut},
    {WS_C("FUNCTION_Paste"), KEY_FUNCTION_Paste},
    {WS_C("FUNCTION_Undo"), KEY_FUNCTION_Undo},

    {WS_C("FUNCTION_Mute"), KEY_FUNCTION_Mute},
    {WS_C("FUNCTION_VolumeUp"), KEY_FUNCTION_VolumeUp},
    {WS_C("FUNCTION_VolumeDown"), KEY_FUNCTION_VolumeDown},

    {WS_C("KEYPAD_Clear"), KEY_KEYPAD_Clear},
    {WS_C("KEYPAD_ClearEntry"), KEY_KEYPAD_ClearEntry},
    {WS_C("KEYPAD_PlusMinus"), KEY_KEYPAD_PlusMinus},

    {WS_C("KEYPAD_MemoryClear"), KEY_KEYPAD_MemoryClear},
    {WS_C("KEYPAD_MemoryStore"), KEY_KEYPAD_MemoryStore},
    {WS_C("KEYPAD_MemoryRecall"), KEY_KEYPAD_MemoryRecall},
    {WS_C("KEYPAD_MemoryAdd"), KEY_KEYPAD_MemoryAdd},
    {WS_C("KEYPAD_MemorySubtract"), KEY_KEYPAD_MemorySubtract},
    {WS_C("KEYPAD_MemoryMultiply"), KEY_KEYPAD_MemoryMultiply},
    {WS_C("KEYPAD_MemoryDivide"), KEY_KEYPAD_MemoryDivide},

    {WS_C("KEYPAD_Binary"), KEY_KEYPAD_Binary},
    {WS_C("KEYPAD_Octal"), KEY_KEYPAD_Octal},
    {WS_C("KEYPAD_Decimal"), KEY_KEYPAD_Decimal},
    {WS_C("KEYPAD_Hexadecimal"), KEY_KEYPAD_Hexadecimal},

    {WS_C("FUNCTION_Cancel"), KEY_FUNCTION_Cancel},
    {WS_C("FUNCTION_Clear"), KEY_FUNCTION_Clear},
    {WS_C("FUNCTION_Prior"), KEY_FUNCTION_Prior},
    {WS_C("FUNCTION_Return"), KEY_FUNCTION_Return},
    {WS_C("FUNCTION_Separator"), KEY_FUNCTION_Separator},
    {WS_C("FUNCTION_Out"), KEY_FUNCTION_Out},
    {WS_C("FUNCTION_Oper"), KEY_FUNCTION_Oper},
    {WS_C("FUNCTION_Clear_Again"), KEY_FUNCTION_Clear_Again},
    {WS_C("FUNCTION_CrSel_Props"), KEY_FUNCTION_CrSel_Props},
    {WS_C("FUNCTION_ExSel"), KEY_FUNCTION_ExSel},
            
    {NULL, 0X00}
  };

  const KeyEntry *key = keyTable;

  while (key->name) {
    if (isKeyword(key->name, characters, length)) {
      *code = key->code;
      return 1;
    }

    key += 1;
  }

  reportDataError(file, "unknown key name: %.*" PRIws, length, characters);
  return 0;
}

static int
getKeyOperand (DataFile *file, KeyCode *code) {
  DataOperand name;

  if (getDataOperand(file, &name, "key name")) {
    if (parseKeyOperand(file, code, name.characters, name.length)) return 1;
  }

  return 0;
}

static int
parseCommandOperand (DataFile *file, int *value, const wchar_t *characters, int length) {
  const CommandEntry *command = commandTable;

  while (command->name) {
    if (length == strlen(command->name)) {
      int index;
      for (
        index = 0;
        (index < length) && (towlower(characters[index]) == tolower(command->name[index]));
        index += 1
      );

      if (index == length) {
	*value = command->code;
	return 1;
      }
    }

    command += 1;
  }

  reportDataError(file, "unknown command name: %.*" PRIws, length, characters);
  return 0;
}

static int
getCommandOperand (DataFile *file, int *value) {
  DataOperand name;

  if (getDataOperand(file, &name, "command name")) {
    if (parseCommandOperand(file, value, name.characters, name.length)) return 1;
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

  if (getKeyOperand(file, &binding->key)) {
    if (getCommandOperand(file, &binding->command)) {
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
