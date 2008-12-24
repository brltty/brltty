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

#ifndef BRLTTY_INCLUDED_KEYCODES
#define BRLTTY_INCLUDED_KEYCODES

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "bitmask.h"

typedef enum {
  KEY_SPECIAL_None = 0,

  KEY_LETTER_A,
  KEY_LETTER_B,
  KEY_LETTER_C,
  KEY_LETTER_D,
  KEY_LETTER_E,
  KEY_LETTER_F,
  KEY_LETTER_G,
  KEY_LETTER_H,
  KEY_LETTER_I,
  KEY_LETTER_J,
  KEY_LETTER_K,
  KEY_LETTER_L,
  KEY_LETTER_M,
  KEY_LETTER_N,
  KEY_LETTER_O,
  KEY_LETTER_P,
  KEY_LETTER_Q,
  KEY_LETTER_R,
  KEY_LETTER_S,
  KEY_LETTER_T,
  KEY_LETTER_U,
  KEY_LETTER_V,
  KEY_LETTER_W,
  KEY_LETTER_X,
  KEY_LETTER_Y,
  KEY_LETTER_Z,

  KEY_SYMBOL_One_Exclamation,
  KEY_SYMBOL_Two_At,
  KEY_SYMBOL_Three_Number,
  KEY_SYMBOL_Four_Dollar,
  KEY_SYMBOL_Five_Percent,
  KEY_SYMBOL_Six_Circumflex,
  KEY_SYMBOL_Seven_Ampersand,
  KEY_SYMBOL_Eight_Asterisk,
  KEY_SYMBOL_Nine_LeftParenthesis,
  KEY_SYMBOL_Zero_RightParenthesis,

  KEY_SYMBOL_Grave_Tilde,
  KEY_SYMBOL_Backslash_Bar,
  KEY_SYMBOL_Minus_Underscore,
  KEY_SYMBOL_Equals_Plus,
  KEY_SYMBOL_LeftBracket_LeftBrace,
  KEY_SYMBOL_RightBracket_RightBrace,
  KEY_SYMBOL_Semicolon_Colon,
  KEY_SYMBOL_Apostrophe_Quote,
  KEY_SYMBOL_Comma_Less,
  KEY_SYMBOL_Period_Greater,
  KEY_SYMBOL_Slash_Question,

  KEY_FUNCTION_Escape,
  KEY_FUNCTION_Enter,
  KEY_FUNCTION_Space,
  KEY_FUNCTION_Tab,
  KEY_FUNCTION_DeleteBackward,

  KEY_FUNCTION_F1,
  KEY_FUNCTION_F2,
  KEY_FUNCTION_F3,
  KEY_FUNCTION_F4,
  KEY_FUNCTION_F5,
  KEY_FUNCTION_F6,
  KEY_FUNCTION_F7,
  KEY_FUNCTION_F8,
  KEY_FUNCTION_F9,
  KEY_FUNCTION_F10,
  KEY_FUNCTION_F11,
  KEY_FUNCTION_F12,
  KEY_FUNCTION_F13,
  KEY_FUNCTION_F14,
  KEY_FUNCTION_F15,
  KEY_FUNCTION_F16,
  KEY_FUNCTION_F17,
  KEY_FUNCTION_F18,
  KEY_FUNCTION_F19,
  KEY_FUNCTION_F20,
  KEY_FUNCTION_F21,
  KEY_FUNCTION_F22,
  KEY_FUNCTION_F23,
  KEY_FUNCTION_F24,

  KEY_FUNCTION_Insert,
  KEY_FUNCTION_DeleteForward,
  KEY_FUNCTION_Home,
  KEY_FUNCTION_End,
  KEY_FUNCTION_PageUp,
  KEY_FUNCTION_PageDown,

  KEY_FUNCTION_ArrowUp,
  KEY_FUNCTION_ArrowDown,
  KEY_FUNCTION_ArrowLeft,
  KEY_FUNCTION_ArrowRight,

  KEY_FUNCTION_PrintScreen,
  KEY_FUNCTION_SystemRequest,
  KEY_FUNCTION_Pause,

  KEY_FUNCTION_ShiftLeft,
  KEY_FUNCTION_ShiftRight,
  KEY_FUNCTION_ControlLeft,
  KEY_FUNCTION_ControlRight,
  KEY_FUNCTION_AltLeft,
  KEY_FUNCTION_AltRight,
  KEY_FUNCTION_GuiLeft,
  KEY_FUNCTION_GuiRight,
  KEY_FUNCTION_Application,

  KEY_LOCK_Capitals,
  KEY_LOCK_Scroll,

  KEY_LOCKING_Capitals,
  KEY_LOCKING_Scroll,
  KEY_LOCKING_Numbers,

  KEY_KEYPAD_NumLock_Clear,
  KEY_KEYPAD_Slash,
  KEY_KEYPAD_Asterisk,
  KEY_KEYPAD_Minus,
  KEY_KEYPAD_Plus,
  KEY_KEYPAD_Enter,
  KEY_KEYPAD_One_End,
  KEY_KEYPAD_Two_ArrowDown,
  KEY_KEYPAD_Three_PageDown,
  KEY_KEYPAD_Four_ArrowLeft,
  KEY_KEYPAD_Five,
  KEY_KEYPAD_Six_ArrowRight,
  KEY_KEYPAD_Seven_Home,
  KEY_KEYPAD_Eight_ArrowUp,
  KEY_KEYPAD_Nine_PageUp,
  KEY_KEYPAD_Zero_Insert,
  KEY_KEYPAD_Period_Delete,

  KEY_KEYPAD_Equals,
  KEY_KEYPAD_LeftParenthesis,
  KEY_KEYPAD_RightParenthesis,
  KEY_KEYPAD_LeftBrace,
  KEY_KEYPAD_RightBrace,
  KEY_KEYPAD_Modulo,
  KEY_KEYPAD_BitwiseAnd,
  KEY_KEYPAD_BitwiseOr,
  KEY_KEYPAD_BitwiseXor,
  KEY_KEYPAD_Less,
  KEY_KEYPAD_Greater,
  KEY_KEYPAD_BooleanAnd,
  KEY_KEYPAD_BooleanOr,
  KEY_KEYPAD_BooleanXor,
  KEY_KEYPAD_BooleanNot,

  KEY_KEYPAD_Backspace,
  KEY_KEYPAD_Space,
  KEY_KEYPAD_Tab,
  KEY_KEYPAD_Comma,
  KEY_KEYPAD_Colon,
  KEY_KEYPAD_Number,
  KEY_KEYPAD_At,

  KEY_KEYPAD_A,
  KEY_KEYPAD_B,
  KEY_KEYPAD_C,
  KEY_KEYPAD_D,
  KEY_KEYPAD_E,
  KEY_KEYPAD_F,

  KEY_KEYPAD_00,
  KEY_KEYPAD_000,
  KEY_KEYPAD_ThousandsSeparator,
  KEY_KEYPAD_DecimalSeparator,
  KEY_KEYPAD_CurrencyUnit,
  KEY_KEYPAD_CurrencySubunit,

  KEY_FUNCTION_Power,
  KEY_FUNCTION_Sleep,
  KEY_FUNCTION_Wakeup,
  KEY_FUNCTION_Stop,

  KEY_FUNCTION_Help,
  KEY_FUNCTION_Find,

  KEY_FUNCTION_Menu,
  KEY_FUNCTION_Select,
  KEY_FUNCTION_Again,
  KEY_FUNCTION_Execute,

  KEY_FUNCTION_Copy,
  KEY_FUNCTION_Cut,
  KEY_FUNCTION_Paste,
  KEY_FUNCTION_Undo,

  KEY_FUNCTION_Mute,
  KEY_FUNCTION_VolumeUp,
  KEY_FUNCTION_VolumeDown,

  KEY_KEYPAD_Clear,
  KEY_KEYPAD_ClearEntry,
  KEY_KEYPAD_PlusMinus,

  KEY_KEYPAD_MemoryClear,
  KEY_KEYPAD_MemoryStore,
  KEY_KEYPAD_MemoryRecall,
  KEY_KEYPAD_MemoryAdd,
  KEY_KEYPAD_MemorySubtract,
  KEY_KEYPAD_MemoryMultiply,
  KEY_KEYPAD_MemoryDivide,

  KEY_KEYPAD_Binary,
  KEY_KEYPAD_Octal,
  KEY_KEYPAD_Decimal,
  KEY_KEYPAD_Hexadecimal,

  KEY_FUNCTION_Cancel,
  KEY_FUNCTION_Clear,
  KEY_FUNCTION_Prior,
  KEY_FUNCTION_Return,
  KEY_FUNCTION_Separator,
  KEY_FUNCTION_Out,
  KEY_FUNCTION_Oper,
  KEY_FUNCTION_Clear_Again,
  KEY_FUNCTION_CrSel_Props,
  KEY_FUNCTION_ExSel,

  KeyCodeCount
} KeyCode;

#define KEY_CODE_MASK_ELEMENT_TYPE char
typedef BITMASK(KeyCodeMask, KeyCodeCount, KEY_CODE_MASK_ELEMENT_TYPE);
#define KEY_CODE_MASK_SIZE sizeof(KeyCodeMask)
#define KEY_CODE_MASK_ELEMENT_COUNT (KEY_CODE_MASK_SIZE / sizeof(KEY_CODE_MASK_ELEMENT_TYPE))

extern void copyKeyCodeMask (KeyCodeMask to, const KeyCodeMask from);
extern int sameKeyCodeMasks (const KeyCodeMask mask1, const KeyCodeMask mask2);
extern int isKeySubset (const KeyCodeMask set, const KeyCodeMask subset);

typedef struct {
  KeyCodeMask mask;
  unsigned int count;
  KeyCode codes[KeyCodeCount];
} KeyCodeSet;

extern int addKeyCode (KeyCodeSet *set, KeyCode code);
extern int removeKeyCode (KeyCodeSet *set, KeyCode code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KEYCODES */
