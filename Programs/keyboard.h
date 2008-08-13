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

#ifndef BRLTTY_INCLUDED_KEYBOARD
#define BRLTTY_INCLUDED_KEYBOARD

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  KBD_TYPE_Any = 0,
  KBD_TYPE_PS2 = 0,
  KBD_TYPE_USB,
  KBD_TYPE_Bluetooth
} KeyboardType;

typedef struct {
  const char *device;
  KeyboardType type;
  int vendor;
  int product;
} KeyboardProperties;

extern const KeyboardProperties anyKeyboard;

extern int parseKeyboardProperties (KeyboardProperties *properties, const char *string);
extern int checkKeyboardProperties (const KeyboardProperties *actual, const KeyboardProperties *required);

typedef enum {
  KEY_ERROR_RollOver = 1,
  KEY_ERROR_Post = 2,
  KEY_ERROR_Undefined = 3,
  KEY_LETTER_A = 4,
  KEY_LETTER_B = 5,
  KEY_LETTER_C = 6,
  KEY_LETTER_D = 7,
  KEY_LETTER_E = 8,
  KEY_LETTER_F = 9,
  KEY_LETTER_G = 10,
  KEY_LETTER_H = 11,
  KEY_LETTER_I = 12,
  KEY_LETTER_J = 13,
  KEY_LETTER_K = 14,
  KEY_LETTER_L = 15,
  KEY_LETTER_M = 16,
  KEY_LETTER_N = 17,
  KEY_LETTER_O = 18,
  KEY_LETTER_P = 19,
  KEY_LETTER_Q = 20,
  KEY_LETTER_R = 21,
  KEY_LETTER_S = 22,
  KEY_LETTER_T = 23,
  KEY_LETTER_U = 24,
  KEY_LETTER_V = 25,
  KEY_LETTER_W = 26,
  KEY_LETTER_X = 27,
  KEY_LETTER_Y = 28,
  KEY_LETTER_Z = 29,
  KEY_SYMBOL_One_Exclamation = 30,
  KEY_SYMBOL_Two_At = 31,
  KEY_SYMBOL_Three_Number = 32,
  KEY_SYMBOL_Four_Dollar = 33,
  KEY_SYMBOL_Five_Percent = 34,
  KEY_SYMBOL_Six_Circumflex = 35,
  KEY_SYMBOL_Seven_Ampersand = 36,
  KEY_SYMBOL_Eight_Asterisk = 37,
  KEY_SYMBOL_Nine_LeftParenthesis = 38,
  KEY_SYMBOL_Zero_RightParenthesis = 39,
  KEY_FUNCTION_Enter = 40,
  KEY_FUNCTION_Escape = 41,
  KEY_FUNCTION_DeleteBackward = 42,
  KEY_FUNCTION_Tab = 43,
  KEY_FUNCTION_Space = 44,
  KEY_SYMBOL_Minus_Underscore = 45,
  KEY_SYMBOL_Equals_Plus = 46,
  KEY_SYMBOL_LeftBracket_LeftBrace = 47,
  KEY_SYMBOL_RightBracket_RightBrace = 48,
  KEY_SYMBOL_Backslash_Bar = 49,
  KEY_SYMBOL_Semicolon_Colon = 51,
  KEY_SYMBOL_Grave_Tilde = 53,
  KEY_SYMBOL_Comma_Less = 54,
  KEY_SYMBOL_Period_Greater = 55,
  KEY_SYMBOL_Slash_Question = 56,
  KEY_LOCK_Capitals = 57,
  KEY_FUNCTION_F1 = 58,
  KEY_FUNCTION_F2 = 59,
  KEY_FUNCTION_F3 = 60,
  KEY_FUNCTION_F4 = 61,
  KEY_FUNCTION_F5 = 62,
  KEY_FUNCTION_F6 = 63,
  KEY_FUNCTION_F7 = 64,
  KEY_FUNCTION_F8 = 65,
  KEY_FUNCTION_F9 = 66,
  KEY_FUNCTION_F10 = 67,
  KEY_FUNCTION_F11 = 68,
  KEY_FUNCTION_F12 = 69,
  KEY_FUNCTION_PrintScreen = 70,
  KEY_LOCK_Scroll = 71,
  KEY_FUNCTION_Pause = 72,
  KEY_FUNCTION_Insert = 73,
  KEY_FUNCTION_Home = 74,
  KEY_FUNCTION_PageUp = 75,
  KEY_FUNCTION_DeleteForward = 76,
  KEY_FUNCTION_End = 77,
  KEY_FUNCTION_PageDown = 78,
  KEY_FUNCTION_ArrowRight = 79,
  KEY_FUNCTION_ArrowLeft = 80,
  KEY_FUNCTION_ArrowDown = 81,
  KEY_FUNCTION_ArrowUp = 82,
  KEY_KEYPAD_NumLock_Clear = 83,
  KEY_KEYPAD_Slash = 84,
  KEY_KEYPAD_Asterisk = 85,
  KEY_KEYPAD_Minus = 86,
  KEY_KEYPAD_Plus = 87,
  KEY_KEYPAD_Enter = 88,
  KEY_KEYPAD_One_End = 89,
  KEY_KEYPAD_Two_ArrowDown = 90,
  KEY_KEYPAD_Three_PageDown = 91,
  KEY_KEYPAD_Four_ArrowLeft = 92,
  KEY_KEYPAD_Five = 93,
  KEY_KEYPAD_Six_ArrowRight = 94,
  KEY_KEYPAD_Seven_Home = 95,
  KEY_KEYPAD_Eight_ArrowUp = 96,
  KEY_KEYPAD_Nine_PageUp = 97,
  KEY_KEYPAD_Zero_Insert = 98,
  KEY_KEYPAD_Period_Delete = 99,
  KEY_FUNCTION_Application = 101,
  KEY_FUNCTION_Power = 102,
  KEY_KEYPAD_Equals = 103,
  KEY_FUNCTION_F13 = 104,
  KEY_FUNCTION_F14 = 105,
  KEY_FUNCTION_F15 = 106,
  KEY_FUNCTION_F16 = 107,
  KEY_FUNCTION_F17 = 108,
  KEY_FUNCTION_F18 = 109,
  KEY_FUNCTION_F19 = 110,
  KEY_FUNCTION_F20 = 111,
  KEY_FUNCTION_F21 = 112,
  KEY_FUNCTION_F22 = 113,
  KEY_FUNCTION_F23 = 114,
  KEY_FUNCTION_F24 = 115,
  KEY_FUNCTION_Execute = 116,
  KEY_FUNCTION_Help = 117,
  KEY_FUNCTION_Menu = 118,
  KEY_FUNCTION_Select = 119,
  KEY_FUNCTION_Stop = 120,
  KEY_FUNCTION_Again = 121,
  KEY_FUNCTION_Undo = 122,
  KEY_FUNCTION_Cut = 123,
  KEY_FUNCTION_Copy = 124,
  KEY_FUNCTION_Paste = 125,
  KEY_FUNCTION_Find = 126,
  KEY_FUNCTION_Mute = 127,
  KEY_FUNCTION_VolumeUp = 128,
  KEY_FUNCTION_VolumeDown = 129,
  KEY_LOCKING_Capitals = 130,
  KEY_LOCKING_Numbers = 131,
  KEY_LOCKING_Scroll = 132,
  KEY_KEYPAD_Comma = 133,
  KEY_FUNCTION_SystemRequest = 154,
  KEY_FUNCTION_Cancel = 155,
  KEY_FUNCTION_Clear = 156,
  KEY_FUNCTION_Prior = 157,
  KEY_FUNCTION_Return = 158,
  KEY_FUNCTION_Separator = 159,
  KEY_FUNCTION_Out = 160,
  KEY_FUNCTION_Oper = 161,
  KEY_FUNCTION_Clear_Again = 162,
  KEY_FUNCTION_CrSel_Props = 163,
  KEY_FUNCTION_ExSel = 164,
  KEY_KEYPAD_00 = 176,
  KEY_KEYPAD_000 = 177,
  KEY_KEYPAD_ThousandsSeparator = 178,
  KEY_KEYPAD_DecimalSeparator = 179,
  KEY_KEYPAD_CurrencyUnit = 180,
  KEY_KEYPAD_CurrencySubunit = 181,
  KEY_KEYPAD_LeftParenthesis = 182,
  KEY_KEYPAD_RightParenthesis = 183,
  KEY_KEYPAD_LeftBrace = 184,
  KEY_KEYPAD_RightBrace = 185,
  KEY_KEYPAD_Tab = 186,
  KEY_KEYPAD_Backspace = 187,
  KEY_KEYPAD_A = 188,
  KEY_KEYPAD_B = 189,
  KEY_KEYPAD_C = 190,
  KEY_KEYPAD_D = 191,
  KEY_KEYPAD_E = 192,
  KEY_KEYPAD_F = 193,
  KEY_KEYPAD_BooleanXor = 194,
  KEY_KEYPAD_BitwiseXor = 195,
  KEY_KEYPAD_Modulo = 196,
  KEY_KEYPAD_Less = 197,
  KEY_KEYPAD_Greater = 198,
  KEY_KEYPAD_BitwiseAnd = 199,
  KEY_KEYPAD_BooleanAnd = 200,
  KEY_KEYPAD_BitwiseOr = 201,
  KEY_KEYPAD_BooleanOr = 202,
  KEY_KEYPAD_Colon = 203,
  KEY_KEYPAD_Number = 204,
  KEY_KEYPAD_Space = 205,
  KEY_KEYPAD_At = 206,
  KEY_KEYPAD_BooleanNot = 207,
  KEY_KEYPAD_MemoryStore = 208,
  KEY_KEYPAD_MemoryRecall = 209,
  KEY_KEYPAD_MemoryClear = 210,
  KEY_KEYPAD_MemoryAdd = 211,
  KEY_KEYPAD_MemorySubtract = 212,
  KEY_KEYPAD_MemoryMultiply = 213,
  KEY_KEYPAD_MemoryDivide = 214,
  KEY_KEYPAD_PlusMinus = 215,
  KEY_KEYPAD_Clear = 216,
  KEY_KEYPAD_ClearEntry = 217,
  KEY_KEYPAD_Binary = 218,
  KEY_KEYPAD_Octal = 219,
  KEY_KEYPAD_Decimal = 220,
  KEY_KEYPAD_Hexadecimal = 221,
  KEY_FUNCTION_ControlLeft = 224,
  KEY_FUNCTION_ShiftLeft = 225,
  KEY_FUNCTION_AltLeft = 226,
  KEY_FUNCTION_GuiLeft = 227,
  KEY_FUNCTION_ControlRight = 228,
  KEY_FUNCTION_ShiftRight = 229,
  KEY_FUNCTION_AltRight = 230,
  KEY_FUNCTION_GuiRight = 231,
} KeyCode;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KEYBOARD */
