/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#include "hidkeys.h"
#include "keycodes.h"
#include "bitmask.h"
#include "brl_cmds.h"
#include "cmd_enqueue.h"

typedef struct {
  uint16_t xtCode;
  uint16_t atCode;
} HidKeyEntry;

static const HidKeyEntry hidKeyTable[] = {
  /* aA */
  [HID_KEY_A] = {
    .xtCode = XT_KEY(00, A),
    .atCode = AT_KEY(00, A)
  },

  /* bB */
  [HID_KEY_B] = {
    .xtCode = XT_KEY(00, B),
    .atCode = AT_KEY(00, B)
  },

  /* cC */
  [HID_KEY_C] = {
    .xtCode = XT_KEY(00, C),
    .atCode = AT_KEY(00, C)
  },

  /* dD */
  [HID_KEY_D] = {
    .xtCode = XT_KEY(00, D),
    .atCode = AT_KEY(00, D)
  },

  /* eE */
  [HID_KEY_E] = {
    .xtCode = XT_KEY(00, E),
    .atCode = AT_KEY(00, E)
  },

  /* fF */
  [HID_KEY_F] = {
    .xtCode = XT_KEY(00, F),
    .atCode = AT_KEY(00, F)
  },

  /* gG */
  [HID_KEY_G] = {
    .xtCode = XT_KEY(00, G),
    .atCode = AT_KEY(00, G)
  },

  /* hH */
  [HID_KEY_H] = {
    .xtCode = XT_KEY(00, H),
    .atCode = AT_KEY(00, H)
  },

  /* iI */
  [HID_KEY_I] = {
    .xtCode = XT_KEY(00, I),
    .atCode = AT_KEY(00, I)
  },

  /* jJ */
  [HID_KEY_J] = {
    .xtCode = XT_KEY(00, J),
    .atCode = AT_KEY(00, J)
  },

  /* kK */
  [HID_KEY_K] = {
    .xtCode = XT_KEY(00, K),
    .atCode = AT_KEY(00, K)
  },

  /* lL */
  [HID_KEY_L] = {
    .xtCode = XT_KEY(00, L),
    .atCode = AT_KEY(00, L)
  },

  /* mM */
  [HID_KEY_M] = {
    .xtCode = XT_KEY(00, M),
    .atCode = AT_KEY(00, M)
  },

  /* nN */
  [HID_KEY_N] = {
    .xtCode = XT_KEY(00, N),
    .atCode = AT_KEY(00, N)
  },

  /* oO */
  [HID_KEY_O] = {
    .xtCode = XT_KEY(00, O),
    .atCode = AT_KEY(00, O)
  },

  /* pP */
  [HID_KEY_P] = {
    .xtCode = XT_KEY(00, P),
    .atCode = AT_KEY(00, P)
  },

  /* qQ */
  [HID_KEY_Q] = {
    .xtCode = XT_KEY(00, Q),
    .atCode = AT_KEY(00, Q)
  },

  /* rR */
  [HID_KEY_R] = {
    .xtCode = XT_KEY(00, R),
    .atCode = AT_KEY(00, R)
  },

  /* sS */
  [HID_KEY_S] = {
    .xtCode = XT_KEY(00, S),
    .atCode = AT_KEY(00, S)
  },

  /* tT */
  [HID_KEY_T] = {
    .xtCode = XT_KEY(00, T),
    .atCode = AT_KEY(00, T)
  },

  /* uU */
  [HID_KEY_U] = {
    .xtCode = XT_KEY(00, U),
    .atCode = AT_KEY(00, U)
  },

  /* vV */
  [HID_KEY_V] = {
    .xtCode = XT_KEY(00, V),
    .atCode = AT_KEY(00, V)
  },

  /* wW */
  [HID_KEY_W] = {
    .xtCode = XT_KEY(00, W),
    .atCode = AT_KEY(00, W)
  },

  /* xX */
  [HID_KEY_X] = {
    .xtCode = XT_KEY(00, X),
    .atCode = AT_KEY(00, X)
  },

  /* yY */
  [HID_KEY_Y] = {
    .xtCode = XT_KEY(00, Y),
    .atCode = AT_KEY(00, Y)
  },

  /* zZ */
  [HID_KEY_Z] = {
    .xtCode = XT_KEY(00, Z),
    .atCode = AT_KEY(00, Z)
  },

  /* 1! */
  [HID_KEY_One] = {
    .xtCode = XT_KEY(00, One),
    .atCode = AT_KEY(00, One)
  },

  /* 2@ */
  [HID_KEY_Two] = {
    .xtCode = XT_KEY(00, Two),
    .atCode = AT_KEY(00, Two)
  },

  /* 3# */
  [HID_KEY_Three] = {
    .xtCode = XT_KEY(00, Three),
    .atCode = AT_KEY(00, Three)
  },

  /* 4$ */
  [HID_KEY_Four] = {
    .xtCode = XT_KEY(00, Four),
    .atCode = AT_KEY(00, Four)
  },

  /* 5% */
  [HID_KEY_Five] = {
    .xtCode = XT_KEY(00, Five),
    .atCode = AT_KEY(00, Five)
  },

  /* 6^ */
  [HID_KEY_Six] = {
    .xtCode = XT_KEY(00, Six),
    .atCode = AT_KEY(00, Six)
  },

  /* 7& */
  [HID_KEY_Seven] = {
    .xtCode = XT_KEY(00, Seven),
    .atCode = AT_KEY(00, Seven)
  },

  /* 8* */
  [HID_KEY_Eight] = {
    .xtCode = XT_KEY(00, Eight),
    .atCode = AT_KEY(00, Eight)
  },

  /* 9( */
  [HID_KEY_Nine] = {
    .xtCode = XT_KEY(00, Nine),
    .atCode = AT_KEY(00, Nine)
  },

  /* 0) */
  [HID_KEY_Zero] = {
    .xtCode = XT_KEY(00, Zero),
    .atCode = AT_KEY(00, Zero)
  },

  /* Return */
  [HID_KEY_Return] = {
    .xtCode = XT_KEY(00, Return),
    .atCode = AT_KEY(00, Return)
  },

  /* Escape */
  [HID_KEY_Escape] = {
    .xtCode = XT_KEY(00, Escape),
    .atCode = AT_KEY(00, Escape)
  },

  /* Backspace */
  [HID_KEY_Backspace] = {
    .xtCode = XT_KEY(00, Backspace),
    .atCode = AT_KEY(00, Backspace)
  },

  /* Tab */
  [HID_KEY_Tab] = {
    .xtCode = XT_KEY(00, Tab),
    .atCode = AT_KEY(00, Tab)
  },

  /* Space */
  [HID_KEY_Space] = {
    .xtCode = XT_KEY(00, Space),
    .atCode = AT_KEY(00, Space)
  },

  /* -_ */
  [HID_KEY_Minus] = {
    .xtCode = XT_KEY(00, Minus),
    .atCode = AT_KEY(00, Minus)
  },

  /* =+ */
  [HID_KEY_Equals] = {
    .xtCode = XT_KEY(00, Equals),
    .atCode = AT_KEY(00, Equals)
  },

  /* [{ */
  [HID_KEY_LeftBracket] = {
    .xtCode = XT_KEY(00, LeftBracket),
    .atCode = AT_KEY(00, LeftBracket)
  },

  /* ]} */
  [HID_KEY_RightBracket] = {
    .xtCode = XT_KEY(00, RightBracket),
    .atCode = AT_KEY(00, RightBracket)
  },

  /* \| */
  [HID_KEY_Backslash] = {
    .xtCode = XT_KEY(00, Backslash),
    .atCode = AT_KEY(00, Backslash)
  },

  /* Europe 1 (Note 2) */
  [HID_KEY_Europe1] = {
    .xtCode = XT_KEY(00, Backslash),
    .atCode = AT_KEY(00, Backslash)
  },

  /* ;: */
  [HID_KEY_Semicolon] = {
    .xtCode = XT_KEY(00, Semicolon),
    .atCode = AT_KEY(00, Semicolon)
  },

  /* '" */
  [HID_KEY_Apostrophe] = {
    .xtCode = XT_KEY(00, Apostrophe),
    .atCode = AT_KEY(00, Apostrophe)
  },

  /* `~ */
  [HID_KEY_Grave] = {
    .xtCode = XT_KEY(00, Grave),
    .atCode = AT_KEY(00, Grave)
  },

  /* ,< */
  [HID_KEY_Comma] = {
    .xtCode = XT_KEY(00, Comma),
    .atCode = AT_KEY(00, Comma)
  },

  /* .> */
  [HID_KEY_Period] = {
    .xtCode = XT_KEY(00, Period),
    .atCode = AT_KEY(00, Period)
  },

  /* /? */
  [HID_KEY_Slash] = {
    .xtCode = XT_KEY(00, Slash),
    .atCode = AT_KEY(00, Slash)
  },

  /* Caps Lock */
  [HID_KEY_CapsLock] = {
    .xtCode = XT_KEY(00, CapsLock),
    .atCode = AT_KEY(00, CapsLock)
  },

  /* F1 */
  [HID_KEY_F1] = {
    .xtCode = XT_KEY(00, F1),
    .atCode = AT_KEY(00, F1)
  },

  /* F2 */
  [HID_KEY_F2] = {
    .xtCode = XT_KEY(00, F2),
    .atCode = AT_KEY(00, F2)
  },

  /* F3 */
  [HID_KEY_F3] = {
    .xtCode = XT_KEY(00, F3),
    .atCode = AT_KEY(00, F3)
  },

  /* F4 */
  [HID_KEY_F4] = {
    .xtCode = XT_KEY(00, F4),
    .atCode = AT_KEY(00, F4)
  },

  /* F5 */
  [HID_KEY_F5] = {
    .xtCode = XT_KEY(00, F5),
    .atCode = AT_KEY(00, F5)
  },

  /* F6 */
  [HID_KEY_F6] = {
    .xtCode = XT_KEY(00, F6),
    .atCode = AT_KEY(00, F6)
  },

  /* F7 */
  [HID_KEY_F7] = {
    .xtCode = XT_KEY(00, F7),
    .atCode = AT_KEY(00, F7)
  },

  /* F8 */
  [HID_KEY_F8] = {
    .xtCode = XT_KEY(00, F8),
    .atCode = AT_KEY(00, F8)
  },

  /* F9 */
  [HID_KEY_F9] = {
    .xtCode = XT_KEY(00, F9),
    .atCode = AT_KEY(00, F9)
  },

  /* F10 */
  [HID_KEY_F10] = {
    .xtCode = XT_KEY(00, F10),
    .atCode = AT_KEY(00, F10)
  },

  /* F11 */
  [HID_KEY_F11] = {
    .xtCode = XT_KEY(00, F11),
    .atCode = AT_KEY(00, F11)
  },

  /* F12 */
  [HID_KEY_F12] = {
    .xtCode = XT_KEY(00, F12),
    .atCode = AT_KEY(00, F12)
  },

  /* Print Screen (Note 1) */
  [HID_KEY_PrintScreen] = {
    .xtCode = XT_KEY(E0, PrintScreen),
    .atCode = AT_KEY(E0, PrintScreen)
  },

  /* Scroll Lock */
  [HID_KEY_ScrollLock] = {
    .xtCode = XT_KEY(00, ScrollLock),
    .atCode = AT_KEY(00, ScrollLock)
  },

  /* Insert (Note 1) */
  [HID_KEY_Insert] = {
    .xtCode = XT_KEY(E0, Insert),
    .atCode = AT_KEY(E0, Insert)
  },

  /* Home (Note 1) */
  [HID_KEY_Home] = {
    .xtCode = XT_KEY(E0, Home),
    .atCode = AT_KEY(E0, Home)
  },

  /* Page Up (Note 1) */
  [HID_KEY_PageUp] = {
    .xtCode = XT_KEY(E0, PageUp),
    .atCode = AT_KEY(E0, PageUp)
  },

  /* Delete (Note 1) */
  [HID_KEY_Delete] = {
    .xtCode = XT_KEY(E0, Delete),
    .atCode = AT_KEY(E0, Delete)
  },

  /* End (Note 1) */
  [HID_KEY_End] = {
    .xtCode = XT_KEY(E0, End),
    .atCode = AT_KEY(E0, End)
  },

  /* Page Down (Note 1) */
  [HID_KEY_PageDown] = {
    .xtCode = XT_KEY(E0, PageDown),
    .atCode = AT_KEY(E0, PageDown)
  },

  /* Right Arrow (Note 1) */
  [HID_KEY_ArrowRight] = {
    .xtCode = XT_KEY(E0, ArrowRight),
    .atCode = AT_KEY(E0, ArrowRight)
  },

  /* Left Arrow (Note 1) */
  [HID_KEY_ArrowLeft] = {
    .xtCode = XT_KEY(E0, ArrowLeft),
    .atCode = AT_KEY(E0, ArrowLeft)
  },

  /* Down Arrow (Note 1) */
  [HID_KEY_ArrowDown] = {
    .xtCode = XT_KEY(E0, ArrowDown),
    .atCode = AT_KEY(E0, ArrowDown)
  },

  /* Up Arrow (Note 1) */
  [HID_KEY_ArrowUp] = {
    .xtCode = XT_KEY(E0, ArrowUp),
    .atCode = AT_KEY(E0, ArrowUp)
  },

  /* Num Lock */
  [HID_KEY_NumLock] = {
    .xtCode = XT_KEY(00, NumLock),
    .atCode = AT_KEY(00, NumLock)
  },

  /* Keypad / (Note 1) */
  [HID_KEY_KeypadDivide] = {
    .xtCode = XT_KEY(E0, KeypadDivide),
    .atCode = AT_KEY(E0, KeypadDivide)
  },

  /* Keypad * */
  [HID_KEY_KeypadTimes] = {
    .xtCode = XT_KEY(00, KeypadTimes),
    .atCode = AT_KEY(00, KeypadTimes)
  },

  /* Keypad - */
  [HID_KEY_KeypadMinus] = {
    .xtCode = XT_KEY(00, KeypadMinus),
    .atCode = AT_KEY(00, KeypadMinus)
  },

  /* Keypad + */
  [HID_KEY_KeypadPlus] = {
    .xtCode = XT_KEY(00, KeypadPlus),
    .atCode = AT_KEY(00, KeypadPlus)
  },

  /* Keypad Enter */
  [HID_KEY_KeypadEnter] = {
    .xtCode = XT_KEY(E0, KeypadEnter),
    .atCode = AT_KEY(E0, KeypadEnter)
  },

  /* Keypad 1 End */
  [HID_KEY_KeypadOne] = {
    .xtCode = XT_KEY(00, KeypadOne),
    .atCode = AT_KEY(00, KeypadOne)
  },

  /* Keypad 2 Down */
  [HID_KEY_KeypadTwo] = {
    .xtCode = XT_KEY(00, KeypadTwo),
    .atCode = AT_KEY(00, KeypadTwo)
  },

  /* Keypad 3 PageDn */
  [HID_KEY_KeypadThree] = {
    .xtCode = XT_KEY(00, KeypadThree),
    .atCode = AT_KEY(00, KeypadThree)
  },

  /* Keypad 4 Left */
  [HID_KEY_KeypadFour] = {
    .xtCode = XT_KEY(00, KeypadFour),
    .atCode = AT_KEY(00, KeypadFour)
  },

  /* Keypad 5 */
  [HID_KEY_KeypadFive] = {
    .xtCode = XT_KEY(00, KeypadFive),
    .atCode = AT_KEY(00, KeypadFive)
  },

  /* Keypad 6 Right */
  [HID_KEY_KeypadSix] = {
    .xtCode = XT_KEY(00, KeypadSix),
    .atCode = AT_KEY(00, KeypadSix)
  },

  /* Keypad 7 Home */
  [HID_KEY_KeypadSeven] = {
    .xtCode = XT_KEY(00, KeypadSeven),
    .atCode = AT_KEY(00, KeypadSeven)
  },

  /* Keypad 8 Up */
  [HID_KEY_KeypadEight] = {
    .xtCode = XT_KEY(00, KeypadEight),
    .atCode = AT_KEY(00, KeypadEight)
  },

  /* Keypad 9 PageUp */
  [HID_KEY_KeypadNine] = {
    .xtCode = XT_KEY(00, KeypadNine),
    .atCode = AT_KEY(00, KeypadNine)
  },

  /* Keypad 0 Insert */
  [HID_KEY_KeypadZero] = {
    .xtCode = XT_KEY(00, KeypadZero),
    .atCode = AT_KEY(00, KeypadZero)
  },

  /* Keypad . Delete */
  [HID_KEY_KeypadPeriod] = {
    .xtCode = XT_KEY(00, KeypadPeriod),
    .atCode = AT_KEY(00, KeypadPeriod)
  },

  /* Europe 2 (Note 2) */
  [HID_KEY_Europe2] = {
    .xtCode = XT_KEY(00, Europe2),
    .atCode = AT_KEY(00, Europe2)
  },

  /* App */
  [HID_KEY_App] = {
    .xtCode = XT_KEY(E0, App),
    .atCode = AT_KEY(E0, App)
  },

  /* Keyboard Power */
  [HID_KEY_Power] = {
    .xtCode = XT_KEY(E0, Power),
    .atCode = AT_KEY(E0, Power)
  },

  /* Keypad = */
  [HID_KEY_KeypadEquals] = {
    .xtCode = XT_KEY(00, KeypadEquals),
    .atCode = AT_KEY(00, KeypadEquals)
  },

  /* F13 */
  [HID_KEY_F13] = {
    .xtCode = XT_KEY(00, F13),
    .atCode = AT_KEY(00, F13)
  },

  /* F14 */
  [HID_KEY_F14] = {
    .xtCode = XT_KEY(00, F14),
    .atCode = AT_KEY(00, F14)
  },

  /* F15 */
  [HID_KEY_F15] = {
    .xtCode = XT_KEY(00, F15),
    .atCode = AT_KEY(00, F15)
  },

  /* F16 */
  [HID_KEY_F16] = {
    .xtCode = XT_KEY(00, F16),
    .atCode = AT_KEY(00, F16)
  },

  /* F17 */
  [HID_KEY_F17] = {
    .xtCode = XT_KEY(00, F17),
    .atCode = AT_KEY(00, F17)
  },

  /* F18 */
  [HID_KEY_F18] = {
    .xtCode = XT_KEY(00, F18),
    .atCode = AT_KEY(00, F18)
  },

  /* F19 */
  [HID_KEY_F19] = {
    .xtCode = XT_KEY(00, F19),
    .atCode = AT_KEY(00, F19)
  },

  /* F20 */
  [HID_KEY_F20] = {
    .xtCode = XT_KEY(00, F20),
    .atCode = AT_KEY(00, F20)
  },

  /* F21 */
  [HID_KEY_F21] = {
    .xtCode = XT_KEY(00, F21),
    .atCode = AT_KEY(00, F21)
  },

  /* F22 */
  [HID_KEY_F22] = {
    .xtCode = XT_KEY(00, F22),
    .atCode = AT_KEY(00, F22)
  },

  /* F23 */
  [HID_KEY_F23] = {
    .xtCode = XT_KEY(00, F23),
    .atCode = AT_KEY(00, F23)
  },

  /* F24 */
  [HID_KEY_F24] = {
    .xtCode = XT_KEY(00, F24),
    .atCode = AT_KEY(00, F24)
  },

  /* Keyboard Execute */
  [HID_KEY_Execute] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Help */
  [HID_KEY_Help] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Menu */
  [HID_KEY_Menu] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Select */
  [HID_KEY_Select] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Stop */
  [HID_KEY_Stop] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Again */
  [HID_KEY_Again] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Undo */
  [HID_KEY_Undo] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Cut */
  [HID_KEY_Cut] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Copy */
  [HID_KEY_Copy] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Paste */
  [HID_KEY_Paste] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Find */
  [HID_KEY_Find] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Mute */
  [HID_KEY_Mute] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Volume Up */
  [HID_KEY_VolumeUp] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Volume Dn */
  [HID_KEY_VolumeDown] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Caps Lock */
  [HID_KEY_CapsLockXXX] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Num Lock */
  [HID_KEY_NumLockXXX] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Scroll Lock */
  [HID_KEY_ScrollLockXXX] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keypad , (Brazilian Keypad .) */
  [HID_KEY_KeypadComma] = {
    .xtCode = XT_KEY(00, KeypadComma),
    .atCode = AT_KEY(00, KeypadComma)
  },

  /* Keyboard Equal Sign */
  [HID_KEY_EqualsXXX] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Int'l 1 (Ro) */
  [HID_KEY_International1] = {
    .xtCode = XT_KEY(00, International1),
    .atCode = AT_KEY(00, International1)
  },

  /* Keyboard Intl'2 (Katakana/Hiragana) */
  [HID_KEY_International2] = {
    .xtCode = XT_KEY(00, International2),
    .atCode = AT_KEY(00, International2)
  },

  /* Keyboard Int'l 3 (Yen) */
  [HID_KEY_International3] = {
    .xtCode = XT_KEY(00, International3),
    .atCode = AT_KEY(00, International3)
  },

  /* Keyboard Int'l 4 (Henkan) */
  [HID_KEY_International4] = {
    .xtCode = XT_KEY(00, International4),
    .atCode = AT_KEY(00, International4)
  },

  /* Keyboard Int'l 5 (Muhenkan) */
  [HID_KEY_International5] = {
    .xtCode = XT_KEY(00, International5),
    .atCode = AT_KEY(00, International5)
  },

  /* Keyboard Int'l 6 (PC9800 Keypad ,) */
  [HID_KEY_International6] = {
    .xtCode = XT_KEY(00, International6),
    .atCode = AT_KEY(00, International6)
  },

  /* Keyboard Int'l 7 */
  [HID_KEY_International7] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Int'l 8 */
  [HID_KEY_International8] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Int'l 9 */
  [HID_KEY_International9] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Lang 1 (Hanguel/English) */
  [HID_KEY_Language1] = {
    .xtCode = XT_KEY(00, Language1),
    .atCode = AT_KEY(00, Language1)
  },

  /* Keyboard Lang 2 (Hanja) */
  [HID_KEY_Language2] = {
    .xtCode = XT_KEY(00, Language2),
    .atCode = AT_KEY(00, Language2)
  },

  /* Keyboard Lang 3 (Katakana) */
  [HID_KEY_Language3] = {
    .xtCode = XT_KEY(00, Language3),
    .atCode = AT_KEY(00, Language3)
  },

  /* Keyboard Lang 4 (Hiragana) */
  [HID_KEY_Language4] = {
    .xtCode = XT_KEY(00, Language4),
    .atCode = AT_KEY(00, Language4)
  },

  /* Keyboard Lang 5 (Zenkaku/Hankaku) */
  [HID_KEY_Language5] = {
    .xtCode = XT_KEY(00, F24),
    .atCode = AT_KEY(00, F24)
  },

  /* Keyboard Lang 6 */
  [HID_KEY_Language6] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Lang 7 */
  [HID_KEY_Language7] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Lang 8 */
  [HID_KEY_Language8] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Lang 9 */
  [HID_KEY_Language9] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Alternate Erase */
  [HID_KEY_AlternateErase] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard SysReq/Attention */
  [HID_KEY_SystemReequest] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Cancel */
  [HID_KEY_Cancel] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Clear */
  [HID_KEY_Clear] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Prior */
  [HID_KEY_Prior] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Return */
  [HID_KEY_ReturnXXX] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Separator */
  [HID_KEY_Separator] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Out */
  [HID_KEY_Out] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Oper */
  [HID_KEY_Oper] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard Clear/Again */
  [HID_KEY_ClearXXX] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard CrSel/Props */
  [HID_KEY_CrSel] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Keyboard ExSel */
  [HID_KEY_ExSel] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  },

  /* Left Control */
  [HID_KEY_LeftControl] = {
    .xtCode = XT_KEY(00, LeftControl),
    .atCode = AT_KEY(00, LeftControl)
  },

  /* Left Shift */
  [HID_KEY_LeftShift] = {
    .xtCode = XT_KEY(00, LeftShift),
    .atCode = AT_KEY(00, LeftShift)
  },

  /* Left Alt */
  [HID_KEY_LeftAlt] = {
    .xtCode = XT_KEY(00, LeftAlt),
    .atCode = AT_KEY(00, LeftAlt)
  },

  /* Left GUI */
  [HID_KEY_LeftGUI] = {
    .xtCode = XT_KEY(E0, LeftGUI),
    .atCode = AT_KEY(E0, LeftGUI)
  },

  /* Right Control */
  [HID_KEY_RightControl] = {
    .xtCode = XT_KEY(E0, RightControl),
    .atCode = AT_KEY(E0, RightControl)
  },

  /* Right Shift */
  [HID_KEY_RightShift] = {
    .xtCode = XT_KEY(00, RightShift),
    .atCode = AT_KEY(00, RightShift)
  },

  /* Right Alt */
  [HID_KEY_RightAlt] = {
    .xtCode = XT_KEY(E0, RightAlt),
    .atCode = AT_KEY(E0, RightAlt)
  },

  /* Right GUI */
  [HID_KEY_RightGUI] = {
    .xtCode = XT_KEY(E0, RightGUI),
    .atCode = AT_KEY(E0, RightGUI)
  },
};

static int
enqueueXtCode (uint8_t code) {
  return enqueueCommand(BRL_BLK_PASSXT | code);
}

static int
enqueueHidKeyEvent (unsigned char key, int press) {
  if (key < ARRAY_COUNT(hidKeyTable)) {
    uint16_t code = hidKeyTable[key].xtCode;

    if (code) {
      {
        uint8_t escape = (code >> 8) & 0XFF;

        if (escape)
          if (!enqueueXtCode(escape))
            return 0;
      }

      code &= 0XFF;

      if (!press) {
        if (code & 0X80) return 1;
        code |= 0X80;
      }

      if (!enqueueXtCode(code)) return 0;
    }
  }

  return 1;
}

static unsigned char
getPressedKeys (const HidKeyboardPacket *packet, unsigned char *keys) {
  unsigned char count = 0;

  {
    static const unsigned char modifiers[] = {0XE0, 0XE1, 0XE2, 0XE3, 0XE4, 0XE5, 0XE6, 0XE7};
    const unsigned char *modifier = modifiers;
    uint8_t bit = 0X1;

    while (bit) {
      if (packet->modifiers & bit) keys[count++] = *modifier;
      modifier += 1;
      bit <<= 1;
    }
  }

  {
    int index;

    for (index=0; index<6; index+=1) {
      unsigned char key = packet->keys[index];
      if (!key) break;
      keys[count++] = key;
    }
  }

  return count;
}

void
initializeHidKeyboardPacket (HidKeyboardPacket *packet) {
  memset(packet, 0, sizeof(*packet));
}

void
processHidKeyboardPacket (
  HidKeyboardPacket *oldPacket,
  const HidKeyboardPacket *newPacket
) {
  unsigned char oldKeys[14];
  unsigned char oldCount = getPressedKeys(oldPacket, oldKeys);

  unsigned char newKeys[14];
  unsigned char newCount = getPressedKeys(newPacket, newKeys);

  BITMASK(pressedKeys, 0X100, char);
  unsigned char index;

  memset(pressedKeys, 0, sizeof(pressedKeys));

  for (index=0; index<newCount; index+=1) {
    unsigned char key = newKeys[index];

    BITMASK_SET(pressedKeys, key);
  }

  for (index=0; index<oldCount; index+=1) {
    unsigned char key = oldKeys[index];

    if (BITMASK_TEST(pressedKeys, key)) {
      BITMASK_CLEAR(pressedKeys, key);
    } else {
      enqueueHidKeyEvent(key, 0);
    }
  }

  for (index=0; index<newCount; index+=1) {
    unsigned char key = newKeys[index];

    if (BITMASK_TEST(pressedKeys, key)) {
      enqueueHidKeyEvent(key, 1);
    }
  }

  *oldPacket = *newPacket;
}
