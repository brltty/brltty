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

#ifndef BRLTTY_INCLUDED_KTB_KEYBOARD
#define BRLTTY_INCLUDED_KTB_KEYBOARD

#include "ktbdefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  KBD_KEY_SPECIAL_Unmapped = 0 /* for KBD_KEY_UNMAPPED */,
  KBD_KEY_SPECIAL_Ignore,
} KBD_SpecialKey;

typedef enum {
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
} KBD_LetterKey;

typedef enum {
  KBD_KEY_NUMBER_Zero,
  KBD_KEY_NUMBER_One,
  KBD_KEY_NUMBER_Two,
  KBD_KEY_NUMBER_Three,
  KBD_KEY_NUMBER_Four,
  KBD_KEY_NUMBER_Five,
  KBD_KEY_NUMBER_Six,
  KBD_KEY_NUMBER_Seven,
  KBD_KEY_NUMBER_Eight,
  KBD_KEY_NUMBER_Nine,
} KBD_NumberKey;

typedef enum {
  KBD_KEY_SYMBOL_Grave,
  KBD_KEY_SYMBOL_Backslash,
  KBD_KEY_SYMBOL_Minus,
  KBD_KEY_SYMBOL_Equals,
  KBD_KEY_SYMBOL_LeftBracket,
  KBD_KEY_SYMBOL_RightBracket,
  KBD_KEY_SYMBOL_Semicolon,
  KBD_KEY_SYMBOL_Apostrophe,
  KBD_KEY_SYMBOL_Comma,
  KBD_KEY_SYMBOL_Period,
  KBD_KEY_SYMBOL_Slash,

  KBD_KEY_SYMBOL_Europe2,
} KBD_SymbolKey;

typedef enum {
  KBD_KEY_ACTION_Escape,
  KBD_KEY_ACTION_Enter,
  KBD_KEY_ACTION_Space,
  KBD_KEY_ACTION_Tab,
  KBD_KEY_ACTION_DeleteBackward,

  KBD_KEY_ACTION_Insert,
  KBD_KEY_ACTION_DeleteForward,
  KBD_KEY_ACTION_Home,
  KBD_KEY_ACTION_End,
  KBD_KEY_ACTION_PageUp,
  KBD_KEY_ACTION_PageDown,

  KBD_KEY_ACTION_ArrowUp,
  KBD_KEY_ACTION_ArrowDown,
  KBD_KEY_ACTION_ArrowLeft,
  KBD_KEY_ACTION_ArrowRight,

  KBD_KEY_ACTION_PrintScreen,
  KBD_KEY_ACTION_SystemRequest,
  KBD_KEY_ACTION_Pause,

  KBD_KEY_ACTION_ShiftLeft,
  KBD_KEY_ACTION_ShiftRight,
  KBD_KEY_ACTION_ControlLeft,
  KBD_KEY_ACTION_ControlRight,
  KBD_KEY_ACTION_AltLeft,
  KBD_KEY_ACTION_AltRight,
  KBD_KEY_ACTION_GuiLeft,
  KBD_KEY_ACTION_GuiRight,
  KBD_KEY_ACTION_Application,

  KBD_KEY_ACTION_Help,
  KBD_KEY_ACTION_Stop,
  KBD_KEY_ACTION_Props,
  KBD_KEY_ACTION_Front,
  KBD_KEY_ACTION_Open,
  KBD_KEY_ACTION_Find,
  KBD_KEY_ACTION_Again,
  KBD_KEY_ACTION_Undo,
  KBD_KEY_ACTION_Copy,
  KBD_KEY_ACTION_Paste,
  KBD_KEY_ACTION_Cut,

  KBD_KEY_ACTION_Power,
  KBD_KEY_ACTION_Sleep,
  KBD_KEY_ACTION_Wakeup,

  KBD_KEY_ACTION_Menu,
  KBD_KEY_ACTION_Select,

  KBD_KEY_ACTION_Cancel,
  KBD_KEY_ACTION_Clear,
  KBD_KEY_ACTION_Prior,
  KBD_KEY_ACTION_Return,
  KBD_KEY_ACTION_Separator,
  KBD_KEY_ACTION_Out,
  KBD_KEY_ACTION_Oper,
  KBD_KEY_ACTION_Clear_Again,
  KBD_KEY_ACTION_CrSel_Props,
  KBD_KEY_ACTION_ExSel,
} KBD_ActionKey;

typedef enum {
  KBD_KEY_MEDIA_Mute,
  KBD_KEY_MEDIA_VolumeDown,
  KBD_KEY_MEDIA_VolumeUp,

  KBD_KEY_MEDIA_Stop,
  KBD_KEY_MEDIA_Play,
  KBD_KEY_MEDIA_Record,
  KBD_KEY_MEDIA_Pause,
  KBD_KEY_MEDIA_PlayPause,

  KBD_KEY_MEDIA_Previous,
  KBD_KEY_MEDIA_Next,
  KBD_KEY_MEDIA_Backward,
  KBD_KEY_MEDIA_Forward,

  KBD_KEY_MEDIA_Eject,
  KBD_KEY_MEDIA_Close,
  KBD_KEY_MEDIA_EjectClose,
} KBD_MediaKey;

typedef enum {
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
} KBD_FunctionKey;

typedef enum {
  KBD_KEY_LOCK_Capitals,
  KBD_KEY_LOCK_Scroll,
  KBD_KEY_LOCK_Numbers,
} KBD_LockKey;

typedef enum {
  KBD_KEY_KPNUMBER_Zero,
  KBD_KEY_KPNUMBER_One,
  KBD_KEY_KPNUMBER_Two,
  KBD_KEY_KPNUMBER_Three,
  KBD_KEY_KPNUMBER_Four,
  KBD_KEY_KPNUMBER_Five,
  KBD_KEY_KPNUMBER_Six,
  KBD_KEY_KPNUMBER_Seven,
  KBD_KEY_KPNUMBER_Eight,
  KBD_KEY_KPNUMBER_Nine,

  KBD_KEY_KPNUMBER_A,
  KBD_KEY_KPNUMBER_B,
  KBD_KEY_KPNUMBER_C,
  KBD_KEY_KPNUMBER_D,
  KBD_KEY_KPNUMBER_E,
  KBD_KEY_KPNUMBER_F,
} KBD_KPNumberKey;

typedef enum {
  KBD_KEY_KEYPAD_Slash,
  KBD_KEY_KEYPAD_Asterisk,
  KBD_KEY_KEYPAD_Minus,
  KBD_KEY_KEYPAD_Plus,
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

  KBD_KEY_KEYPAD_Space,
  KBD_KEY_KEYPAD_Comma,
  KBD_KEY_KEYPAD_Colon,
  KBD_KEY_KEYPAD_Number,
  KBD_KEY_KEYPAD_At,

  KBD_KEY_KEYPAD_00,
  KBD_KEY_KEYPAD_000,
  KBD_KEY_KEYPAD_ThousandsSeparator,
  KBD_KEY_KEYPAD_DecimalSeparator,
  KBD_KEY_KEYPAD_CurrencyUnit,
  KBD_KEY_KEYPAD_CurrencySubunit,

  KBD_KEY_KEYPAD_PlusMinus,
} KBD_KeypadKey;

typedef enum {
  KBD_KEY_KPACTION_Enter,
  KBD_KEY_KPACTION_Backspace,
  KBD_KEY_KPACTION_Tab,

  KBD_KEY_KPACTION_Clear,
  KBD_KEY_KPACTION_ClearEntry,

  KBD_KEY_KPACTION_MemoryClear,
  KBD_KEY_KPACTION_MemoryStore,
  KBD_KEY_KPACTION_MemoryRecall,
  KBD_KEY_KPACTION_MemoryAdd,
  KBD_KEY_KPACTION_MemorySubtract,
  KBD_KEY_KPACTION_MemoryMultiply,
  KBD_KEY_KPACTION_MemoryDivide,

  KBD_KEY_KPACTION_Binary,
  KBD_KEY_KPACTION_Octal,
  KBD_KEY_KPACTION_Decimal,
  KBD_KEY_KPACTION_Hexadecimal,
} KBD_KPActionKey;

typedef enum {
  KBD_KEY_BRAILLE_Space,
  KBD_KEY_BRAILLE_Dot1,
  KBD_KEY_BRAILLE_Dot2,
  KBD_KEY_BRAILLE_Dot3,
  KBD_KEY_BRAILLE_Dot4,
  KBD_KEY_BRAILLE_Dot5,
  KBD_KEY_BRAILLE_Dot6,
  KBD_KEY_BRAILLE_Dot7,
  KBD_KEY_BRAILLE_Dot8,

  KBD_KEY_BRAILLE_Backward,
  KBD_KEY_BRAILLE_Forward,
} KBD_BrailleKey;

typedef enum {
  KBD_SET_SPECIAL = 0 /* for KBD_KEY_UNMAPPED */,
  KBD_SET_LETTER,
  KBD_SET_NUMBER,
  KBD_SET_SYMBOL,
  KBD_SET_ACTION,
  KBD_SET_MEDIA,
  KBD_SET_FUNCTION,
  KBD_SET_LOCK,
  KBD_SET_KPNUMBER,
  KBD_SET_KEYPAD,
  KBD_SET_KPACTION,
  KBD_SET_BRAILLE,

  KBD_SET_ROUTE,
} KBD_KeySet;

#define KBD_KEY_VALUE(s,k) {.set=KBD_SET_##s, .key=KBD_KEY_##s##_##k}
#define KBD_NAME_ENTRY(s,k,n) {.value=KBD_KEY_VALUE(s, k), .name=n}

#define KBD_KEY_SPECIAL(name) KBD_KEY_VALUE(SPECIAL, name)
#define KBD_KEY_LETTER(name) KBD_KEY_VALUE(LETTER, name)
#define KBD_KEY_NUMBER(name) KBD_KEY_VALUE(NUMBER, name)
#define KBD_KEY_SYMBOL(name) KBD_KEY_VALUE(SYMBOL, name)
#define KBD_KEY_ACTION(name) KBD_KEY_VALUE(ACTION, name)
#define KBD_KEY_MEDIA(name) KBD_KEY_VALUE(MEDIA, name)
#define KBD_KEY_FUNCTION(name) KBD_KEY_VALUE(FUNCTION, name)
#define KBD_KEY_LOCK(name) KBD_KEY_VALUE(LOCK, name)
#define KBD_KEY_KPNUMBER(name) KBD_KEY_VALUE(KPNUMBER, name)
#define KBD_KEY_KEYPAD(name) KBD_KEY_VALUE(KEYPAD, name)
#define KBD_KEY_KPACTION(name) KBD_KEY_VALUE(KPACTION, name)
#define KBD_KEY_BRAILLE(name) KBD_KEY_VALUE(BRAILLE, name)
#define KBD_KEY_ROUTE(offset) {.set=KBD_SET_ROUTE, .key=(offset)}

#define KBD_KEY_UNMAPPED KBD_KEY_SPECIAL(Unmapped)
#define KBD_KEY_IGNORE KBD_KEY_SPECIAL(Ignore)

extern KEY_NAME_TABLES_DECLARATION(keyboard);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KTB_KEYBOARD */
