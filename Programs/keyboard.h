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

#ifndef BRLTTY_INCLUDED_KEYBOARD
#define BRLTTY_INCLUDED_KEYBOARD

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ktbdefs.h"

typedef enum {
  KBD_TYPE_Any = 0,
  KBD_TYPE_PS2 = 0,
  KBD_TYPE_USB,
  KBD_TYPE_Bluetooth
} KeyboardType;

typedef struct {
  KeyboardType type;
  int vendor;
  int product;
} KeyboardProperties;

extern const KeyboardProperties anyKeyboard;

extern int parseKeyboardProperties (KeyboardProperties *properties, const char *string);
extern int checkKeyboardProperties (const KeyboardProperties *actual, const KeyboardProperties *required);

typedef enum {
  KBD_KEY_SPECIAL_None = 0,

  KBD_KEY_LETTER_A,
  KBD_KEY_LETTER_B,
  KBD_KEY_LETTER_C,
  KBD_KEY_LETTER_D,
  KBD_KEY_LETTER_E,
  KBD_KEY_LETTER_F,
  KBD_KEY_LETTER_G,
  KBD_KEY_LETTER_H,
  KBD_KEY_LETTER_I,
  KBD_KEY_LETTER_J,
  KBD_KEY_LETTER_K,
  KBD_KEY_LETTER_L,
  KBD_KEY_LETTER_M,
  KBD_KEY_LETTER_N,
  KBD_KEY_LETTER_O,
  KBD_KEY_LETTER_P,
  KBD_KEY_LETTER_Q,
  KBD_KEY_LETTER_R,
  KBD_KEY_LETTER_S,
  KBD_KEY_LETTER_T,
  KBD_KEY_LETTER_U,
  KBD_KEY_LETTER_V,
  KBD_KEY_LETTER_W,
  KBD_KEY_LETTER_X,
  KBD_KEY_LETTER_Y,
  KBD_KEY_LETTER_Z,

  KBD_KEY_SYMBOL_One_Exclamation,
  KBD_KEY_SYMBOL_Two_At,
  KBD_KEY_SYMBOL_Three_Number,
  KBD_KEY_SYMBOL_Four_Dollar,
  KBD_KEY_SYMBOL_Five_Percent,
  KBD_KEY_SYMBOL_Six_Circumflex,
  KBD_KEY_SYMBOL_Seven_Ampersand,
  KBD_KEY_SYMBOL_Eight_Asterisk,
  KBD_KEY_SYMBOL_Nine_LeftParenthesis,
  KBD_KEY_SYMBOL_Zero_RightParenthesis,

  KBD_KEY_SYMBOL_Grave_Tilde,
  KBD_KEY_SYMBOL_Backslash_Bar,
  KBD_KEY_SYMBOL_Minus_Underscore,
  KBD_KEY_SYMBOL_Equals_Plus,
  KBD_KEY_SYMBOL_LeftBracket_LeftBrace,
  KBD_KEY_SYMBOL_RightBracket_RightBrace,
  KBD_KEY_SYMBOL_Semicolon_Colon,
  KBD_KEY_SYMBOL_Apostrophe_Quote,
  KBD_KEY_SYMBOL_Comma_Less,
  KBD_KEY_SYMBOL_Period_Greater,
  KBD_KEY_SYMBOL_Slash_Question,

  KBD_KEY_FUNCTION_Escape,
  KBD_KEY_FUNCTION_Enter,
  KBD_KEY_FUNCTION_Space,
  KBD_KEY_FUNCTION_Tab,
  KBD_KEY_FUNCTION_DeleteBackward,

  KBD_KEY_FUNCTION_F1,
  KBD_KEY_FUNCTION_F2,
  KBD_KEY_FUNCTION_F3,
  KBD_KEY_FUNCTION_F4,
  KBD_KEY_FUNCTION_F5,
  KBD_KEY_FUNCTION_F6,
  KBD_KEY_FUNCTION_F7,
  KBD_KEY_FUNCTION_F8,
  KBD_KEY_FUNCTION_F9,
  KBD_KEY_FUNCTION_F10,
  KBD_KEY_FUNCTION_F11,
  KBD_KEY_FUNCTION_F12,
  KBD_KEY_FUNCTION_F13,
  KBD_KEY_FUNCTION_F14,
  KBD_KEY_FUNCTION_F15,
  KBD_KEY_FUNCTION_F16,
  KBD_KEY_FUNCTION_F17,
  KBD_KEY_FUNCTION_F18,
  KBD_KEY_FUNCTION_F19,
  KBD_KEY_FUNCTION_F20,
  KBD_KEY_FUNCTION_F21,
  KBD_KEY_FUNCTION_F22,
  KBD_KEY_FUNCTION_F23,
  KBD_KEY_FUNCTION_F24,

  KBD_KEY_FUNCTION_Insert,
  KBD_KEY_FUNCTION_DeleteForward,
  KBD_KEY_FUNCTION_Home,
  KBD_KEY_FUNCTION_End,
  KBD_KEY_FUNCTION_PageUp,
  KBD_KEY_FUNCTION_PageDown,

  KBD_KEY_FUNCTION_ArrowUp,
  KBD_KEY_FUNCTION_ArrowDown,
  KBD_KEY_FUNCTION_ArrowLeft,
  KBD_KEY_FUNCTION_ArrowRight,

  KBD_KEY_FUNCTION_PrintScreen,
  KBD_KEY_FUNCTION_SystemRequest,
  KBD_KEY_FUNCTION_Pause,

  KBD_KEY_FUNCTION_ShiftLeft,
  KBD_KEY_FUNCTION_ShiftRight,
  KBD_KEY_FUNCTION_ControlLeft,
  KBD_KEY_FUNCTION_ControlRight,
  KBD_KEY_FUNCTION_AltLeft,
  KBD_KEY_FUNCTION_AltRight,
  KBD_KEY_FUNCTION_GuiLeft,
  KBD_KEY_FUNCTION_GuiRight,
  KBD_KEY_FUNCTION_Application,

  KBD_KEY_LOCK_Capitals,
  KBD_KEY_LOCK_Scroll,

  KBD_KEY_LOCKING_Capitals,
  KBD_KEY_LOCKING_Scroll,
  KBD_KEY_LOCKING_Numbers,

  KBD_KEY_KEYPAD_NumLock_Clear,
  KBD_KEY_KEYPAD_Slash,
  KBD_KEY_KEYPAD_Asterisk,
  KBD_KEY_KEYPAD_Minus,
  KBD_KEY_KEYPAD_Plus,
  KBD_KEY_KEYPAD_Enter,
  KBD_KEY_KEYPAD_One_End,
  KBD_KEY_KEYPAD_Two_ArrowDown,
  KBD_KEY_KEYPAD_Three_PageDown,
  KBD_KEY_KEYPAD_Four_ArrowLeft,
  KBD_KEY_KEYPAD_Five,
  KBD_KEY_KEYPAD_Six_ArrowRight,
  KBD_KEY_KEYPAD_Seven_Home,
  KBD_KEY_KEYPAD_Eight_ArrowUp,
  KBD_KEY_KEYPAD_Nine_PageUp,
  KBD_KEY_KEYPAD_Zero_Insert,
  KBD_KEY_KEYPAD_Period_Delete,

  KBD_KEY_KEYPAD_Equals,
  KBD_KEY_KEYPAD_LeftParenthesis,
  KBD_KEY_KEYPAD_RightParenthesis,
  KBD_KEY_KEYPAD_LeftBrace,
  KBD_KEY_KEYPAD_RightBrace,
  KBD_KEY_KEYPAD_Modulo,
  KBD_KEY_KEYPAD_BitwiseAnd,
  KBD_KEY_KEYPAD_BitwiseOr,
  KBD_KEY_KEYPAD_BitwiseXor,
  KBD_KEY_KEYPAD_Less,
  KBD_KEY_KEYPAD_Greater,
  KBD_KEY_KEYPAD_BooleanAnd,
  KBD_KEY_KEYPAD_BooleanOr,
  KBD_KEY_KEYPAD_BooleanXor,
  KBD_KEY_KEYPAD_BooleanNot,

  KBD_KEY_KEYPAD_Backspace,
  KBD_KEY_KEYPAD_Space,
  KBD_KEY_KEYPAD_Tab,
  KBD_KEY_KEYPAD_Comma,
  KBD_KEY_KEYPAD_Colon,
  KBD_KEY_KEYPAD_Number,
  KBD_KEY_KEYPAD_At,

  KBD_KEY_KEYPAD_A,
  KBD_KEY_KEYPAD_B,
  KBD_KEY_KEYPAD_C,
  KBD_KEY_KEYPAD_D,
  KBD_KEY_KEYPAD_E,
  KBD_KEY_KEYPAD_F,

  KBD_KEY_KEYPAD_00,
  KBD_KEY_KEYPAD_000,
  KBD_KEY_KEYPAD_ThousandsSeparator,
  KBD_KEY_KEYPAD_DecimalSeparator,
  KBD_KEY_KEYPAD_CurrencyUnit,
  KBD_KEY_KEYPAD_CurrencySubunit,

  KBD_KEY_FUNCTION_Power,
  KBD_KEY_FUNCTION_Sleep,
  KBD_KEY_FUNCTION_Wakeup,
  KBD_KEY_FUNCTION_Stop,

  KBD_KEY_FUNCTION_Help,
  KBD_KEY_FUNCTION_Find,

  KBD_KEY_FUNCTION_Menu,
  KBD_KEY_FUNCTION_Select,
  KBD_KEY_FUNCTION_Again,
  KBD_KEY_FUNCTION_Execute,

  KBD_KEY_FUNCTION_Copy,
  KBD_KEY_FUNCTION_Cut,
  KBD_KEY_FUNCTION_Paste,
  KBD_KEY_FUNCTION_Undo,

  KBD_KEY_FUNCTION_Mute,
  KBD_KEY_FUNCTION_VolumeUp,
  KBD_KEY_FUNCTION_VolumeDown,

  KBD_KEY_KEYPAD_Clear,
  KBD_KEY_KEYPAD_ClearEntry,
  KBD_KEY_KEYPAD_PlusMinus,

  KBD_KEY_KEYPAD_MemoryClear,
  KBD_KEY_KEYPAD_MemoryStore,
  KBD_KEY_KEYPAD_MemoryRecall,
  KBD_KEY_KEYPAD_MemoryAdd,
  KBD_KEY_KEYPAD_MemorySubtract,
  KBD_KEY_KEYPAD_MemoryMultiply,
  KBD_KEY_KEYPAD_MemoryDivide,

  KBD_KEY_KEYPAD_Binary,
  KBD_KEY_KEYPAD_Octal,
  KBD_KEY_KEYPAD_Decimal,
  KBD_KEY_KEYPAD_Hexadecimal,

  KBD_KEY_FUNCTION_Cancel,
  KBD_KEY_FUNCTION_Clear,
  KBD_KEY_FUNCTION_Prior,
  KBD_KEY_FUNCTION_Return,
  KBD_KEY_FUNCTION_Separator,
  KBD_KEY_FUNCTION_Out,
  KBD_KEY_FUNCTION_Oper,
  KBD_KEY_FUNCTION_Clear_Again,
  KBD_KEY_FUNCTION_CrSel_Props,
  KBD_KEY_FUNCTION_ExSel
} KeyboardKey;

extern KEY_NAME_TABLE_DECLARATION(keyboard);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KEYBOARD */
