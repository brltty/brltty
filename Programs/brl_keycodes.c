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

#include "brl_keycodes.h"
#include "kbd_keycodes.h"
#include "brl_cmds.h"

typedef enum {
  MOD_RELEASE = 0, /* must be first */
  MOD_WINDOWS_LEFT,
  MOD_WINDOWS_RIGHT,
  MOD_APP,
  MOD_CAPS_LOCK,
  MOD_SCROLL_LOCK,
  MOD_NUMBER_LOCK,
  MOD_SHIFT_LEFT,
  MOD_SHIFT_RIGHT,
  MOD_CONTROL_LEFT,
  MOD_CONTROL_RIGHT,
  MOD_ALT_LEFT,
  MOD_ALT_RIGHT
} Modifier;

#define MOD_BIT(number) (1 << (number))
#define MOD_SET(number, bits) ((bits) |= MOD_BIT((number)))
#define MOD_CLR(number, bits) ((bits) &= ~MOD_BIT((number)))
#define MOD_TST(number, bits) ((bits) & MOD_BIT((number)))

#define USE_SCAN_CODES(mode,type) (mode##_scanCodesSize = (mode##_scanCodes = mode##_##type##ScanCodes)? (sizeof(mode##_##type##ScanCodes) / sizeof(*mode##_scanCodes)): 0)

typedef struct {
  int command;
  int alternate;
} KeyEntry;

#define CMD_CHAR(wc) (BRL_BLK_PASSCHAR | BRL_ARG_SET((wc)))

static const KeyEntry keyEntry_Escape = {BRL_BLK_PASSKEY+BRL_KEY_ESCAPE};
static const KeyEntry keyEntry_F1 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+0};
static const KeyEntry keyEntry_F2 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+1};
static const KeyEntry keyEntry_F3 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+2};
static const KeyEntry keyEntry_F4 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+3};
static const KeyEntry keyEntry_F5 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+4};
static const KeyEntry keyEntry_F6 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+5};
static const KeyEntry keyEntry_F7 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+6};
static const KeyEntry keyEntry_F8 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+7};
static const KeyEntry keyEntry_F9 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+8};
static const KeyEntry keyEntry_F10 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+9};
static const KeyEntry keyEntry_F11 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+10};
static const KeyEntry keyEntry_F12 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+11};
static const KeyEntry keyEntry_ScrollLock = {MOD_SCROLL_LOCK};

static const KeyEntry keyEntry_F13 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+12};
static const KeyEntry keyEntry_F14 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+13};
static const KeyEntry keyEntry_F15 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+14};
static const KeyEntry keyEntry_F16 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+15};
static const KeyEntry keyEntry_F17 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+16};
static const KeyEntry keyEntry_F18 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+17};
static const KeyEntry keyEntry_F19 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+18};
static const KeyEntry keyEntry_F20 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+19};
static const KeyEntry keyEntry_F21 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+20};
static const KeyEntry keyEntry_F22 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+21};
static const KeyEntry keyEntry_F23 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+22};
static const KeyEntry keyEntry_F24 = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+23};

static const KeyEntry keyEntry_Grave = {CMD_CHAR(WC_C('`')), CMD_CHAR(WC_C('~'))};
static const KeyEntry keyEntry_1 = {CMD_CHAR(WC_C('1')), CMD_CHAR(WC_C('!'))};
static const KeyEntry keyEntry_2 = {CMD_CHAR(WC_C('2')), CMD_CHAR(WC_C('@'))};
static const KeyEntry keyEntry_3 = {CMD_CHAR(WC_C('3')), CMD_CHAR(WC_C('#'))};
static const KeyEntry keyEntry_4 = {CMD_CHAR(WC_C('4')), CMD_CHAR(WC_C('$'))};
static const KeyEntry keyEntry_5 = {CMD_CHAR(WC_C('5')), CMD_CHAR(WC_C('%'))};
static const KeyEntry keyEntry_6 = {CMD_CHAR(WC_C('6')), CMD_CHAR(WC_C('^'))};
static const KeyEntry keyEntry_7 = {CMD_CHAR(WC_C('7')), CMD_CHAR(WC_C('&'))};
static const KeyEntry keyEntry_8 = {CMD_CHAR(WC_C('8')), CMD_CHAR(WC_C('*'))};
static const KeyEntry keyEntry_9 = {CMD_CHAR(WC_C('9')), CMD_CHAR(WC_C('('))};
static const KeyEntry keyEntry_0 = {CMD_CHAR(WC_C('0')), CMD_CHAR(WC_C(')'))};
static const KeyEntry keyEntry_Minus = {CMD_CHAR(WC_C('-')), CMD_CHAR(WC_C('_'))};
static const KeyEntry keyEntry_Equal = {CMD_CHAR(WC_C('=')), CMD_CHAR(WC_C('+'))};
static const KeyEntry keyEntry_Backspace = {BRL_BLK_PASSKEY+BRL_KEY_BACKSPACE};

static const KeyEntry keyEntry_Tab = {BRL_BLK_PASSKEY+BRL_KEY_TAB};
static const KeyEntry keyEntry_Q = {CMD_CHAR(WC_C('q')), CMD_CHAR(WC_C('Q'))};
static const KeyEntry keyEntry_W = {CMD_CHAR(WC_C('w')), CMD_CHAR(WC_C('W'))};
static const KeyEntry keyEntry_E = {CMD_CHAR(WC_C('e')), CMD_CHAR(WC_C('E'))};
static const KeyEntry keyEntry_R = {CMD_CHAR(WC_C('r')), CMD_CHAR(WC_C('R'))};
static const KeyEntry keyEntry_T = {CMD_CHAR(WC_C('t')), CMD_CHAR(WC_C('T'))};
static const KeyEntry keyEntry_Y = {CMD_CHAR(WC_C('y')), CMD_CHAR(WC_C('Y'))};
static const KeyEntry keyEntry_U = {CMD_CHAR(WC_C('u')), CMD_CHAR(WC_C('U'))};
static const KeyEntry keyEntry_I = {CMD_CHAR(WC_C('i')), CMD_CHAR(WC_C('I'))};
static const KeyEntry keyEntry_O = {CMD_CHAR(WC_C('o')), CMD_CHAR(WC_C('O'))};
static const KeyEntry keyEntry_P = {CMD_CHAR(WC_C('p')), CMD_CHAR(WC_C('P'))};
static const KeyEntry keyEntry_LeftBracket = {CMD_CHAR(WC_C('[')), CMD_CHAR(WC_C('{'))};
static const KeyEntry keyEntry_RightBracket = {CMD_CHAR(WC_C(']')), CMD_CHAR(WC_C('}'))};
static const KeyEntry keyEntry_Backslash = {CMD_CHAR('\\'), CMD_CHAR(WC_C('|'))};

static const KeyEntry keyEntry_CapsLock = {MOD_CAPS_LOCK};
static const KeyEntry keyEntry_A = {CMD_CHAR(WC_C('a')), CMD_CHAR(WC_C('A'))};
static const KeyEntry keyEntry_S = {CMD_CHAR(WC_C('s')), CMD_CHAR(WC_C('S'))};
static const KeyEntry keyEntry_D = {CMD_CHAR(WC_C('d')), CMD_CHAR(WC_C('D'))};
static const KeyEntry keyEntry_F = {CMD_CHAR(WC_C('f')), CMD_CHAR(WC_C('F'))};
static const KeyEntry keyEntry_G = {CMD_CHAR(WC_C('g')), CMD_CHAR(WC_C('G'))};
static const KeyEntry keyEntry_H = {CMD_CHAR(WC_C('h')), CMD_CHAR(WC_C('H'))};
static const KeyEntry keyEntry_J = {CMD_CHAR(WC_C('j')), CMD_CHAR(WC_C('J'))};
static const KeyEntry keyEntry_K = {CMD_CHAR(WC_C('k')), CMD_CHAR(WC_C('K'))};
static const KeyEntry keyEntry_L = {CMD_CHAR(WC_C('l')), CMD_CHAR(WC_C('L'))};
static const KeyEntry keyEntry_Semicolon = {CMD_CHAR(WC_C(';')), CMD_CHAR(WC_C(':'))};
static const KeyEntry keyEntry_Apostrophe = {CMD_CHAR(WC_C('\'')), CMD_CHAR(WC_C('"'))};
static const KeyEntry keyEntry_Enter = {BRL_BLK_PASSKEY+BRL_KEY_ENTER};

static const KeyEntry keyEntry_LeftShift = {MOD_SHIFT_LEFT};
static const KeyEntry keyEntry_Europe2 = {CMD_CHAR(WC_C('<')), CMD_CHAR(WC_C('>'))};
static const KeyEntry keyEntry_Z = {CMD_CHAR(WC_C('z')), CMD_CHAR(WC_C('Z'))};
static const KeyEntry keyEntry_X = {CMD_CHAR(WC_C('x')), CMD_CHAR(WC_C('X'))};
static const KeyEntry keyEntry_C = {CMD_CHAR(WC_C('c')), CMD_CHAR(WC_C('C'))};
static const KeyEntry keyEntry_V = {CMD_CHAR(WC_C('v')), CMD_CHAR(WC_C('V'))};
static const KeyEntry keyEntry_B = {CMD_CHAR(WC_C('b')), CMD_CHAR(WC_C('B'))};
static const KeyEntry keyEntry_N = {CMD_CHAR(WC_C('n')), CMD_CHAR(WC_C('N'))};
static const KeyEntry keyEntry_M = {CMD_CHAR(WC_C('m')), CMD_CHAR(WC_C('M'))};
static const KeyEntry keyEntry_Comma = {CMD_CHAR(WC_C(',')), CMD_CHAR(WC_C('<'))};
static const KeyEntry keyEntry_Period = {CMD_CHAR(WC_C('.')), CMD_CHAR(WC_C('>'))};
static const KeyEntry keyEntry_Slash = {CMD_CHAR(WC_C('/')), CMD_CHAR(WC_C('?'))};
static const KeyEntry keyEntry_RightShift = {MOD_SHIFT_RIGHT};

static const KeyEntry keyEntry_LeftControl = {MOD_CONTROL_LEFT};
static const KeyEntry keyEntry_LeftGUI = {MOD_WINDOWS_LEFT};
static const KeyEntry keyEntry_LeftAlt = {MOD_ALT_LEFT};
static const KeyEntry keyEntry_Space = {CMD_CHAR(WC_C(' '))};
static const KeyEntry keyEntry_RightAlt = {MOD_ALT_RIGHT};
static const KeyEntry keyEntry_RightGUI = {MOD_WINDOWS_RIGHT};
static const KeyEntry keyEntry_App = {MOD_APP};
static const KeyEntry keyEntry_RightControl = {MOD_CONTROL_RIGHT};

static const KeyEntry keyEntry_Insert = {BRL_BLK_PASSKEY+BRL_KEY_INSERT};
static const KeyEntry keyEntry_Delete = {BRL_BLK_PASSKEY+BRL_KEY_DELETE};
static const KeyEntry keyEntry_Home = {BRL_BLK_PASSKEY+BRL_KEY_HOME};
static const KeyEntry keyEntry_End = {BRL_BLK_PASSKEY+BRL_KEY_END};
static const KeyEntry keyEntry_PageUp = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP};
static const KeyEntry keyEntry_PageDown = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN};

static const KeyEntry keyEntry_ArrowUp = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP};
static const KeyEntry keyEntry_ArrowLeft = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT};
static const KeyEntry keyEntry_ArrowDown = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN};
static const KeyEntry keyEntry_ArrowRight = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT};

static const KeyEntry keyEntry_NumLock = {MOD_NUMBER_LOCK};
static const KeyEntry keyEntry_KPSlash = {CMD_CHAR(WC_C('/'))};
static const KeyEntry keyEntry_KPAsterisk = {CMD_CHAR(WC_C('*'))};
static const KeyEntry keyEntry_KPMinus = {CMD_CHAR(WC_C('-'))};
static const KeyEntry keyEntry_KPPlus = {CMD_CHAR(WC_C('+'))};
static const KeyEntry keyEntry_KPEnter = {BRL_BLK_PASSKEY+BRL_KEY_ENTER};
static const KeyEntry keyEntry_KPPeriod = {BRL_BLK_PASSKEY+BRL_KEY_DELETE, CMD_CHAR(WC_C('.'))};
static const KeyEntry keyEntry_KP0 = {BRL_BLK_PASSKEY+BRL_KEY_INSERT, CMD_CHAR(WC_C('0'))};
static const KeyEntry keyEntry_KP1 = {BRL_BLK_PASSKEY+BRL_KEY_END, CMD_CHAR(WC_C('1'))};
static const KeyEntry keyEntry_KP2 = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN, CMD_CHAR(WC_C('2'))};
static const KeyEntry keyEntry_KP3 = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN, CMD_CHAR(WC_C('3'))};
static const KeyEntry keyEntry_KP4 = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT, CMD_CHAR(WC_C('4'))};
static const KeyEntry keyEntry_KP5 = {CMD_CHAR(WC_C('5'))};
static const KeyEntry keyEntry_KP6 = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, CMD_CHAR(WC_C('6'))};
static const KeyEntry keyEntry_KP7 = {BRL_BLK_PASSKEY+BRL_KEY_HOME, CMD_CHAR(WC_C('7'))};
static const KeyEntry keyEntry_KP8 = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP, CMD_CHAR(WC_C('8'))};
static const KeyEntry keyEntry_KP9 = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP, CMD_CHAR(WC_C('9'))};

static int
interpretKey (int *command, const KeyEntry *key, int release, unsigned int *modifiers) {
  int cmd = key->command;
  int blk = cmd & BRL_MSK_BLK;

  if (key->alternate) {
    int alternate = 0;

    if (blk == BRL_BLK_PASSCHAR) {
      if (MOD_TST(MOD_SHIFT_LEFT, *modifiers) || MOD_TST(MOD_SHIFT_RIGHT, *modifiers)) alternate = 1;
    } else {
      if (MOD_TST(MOD_NUMBER_LOCK, *modifiers)) alternate = 1;
    }

    if (alternate) {
      cmd = key->alternate;
      blk = cmd & BRL_MSK_BLK;
    }
  }

  if (cmd) {
    if (blk) {
      if (!release) {
        if (blk == BRL_BLK_PASSCHAR) {
          if (MOD_TST(MOD_CAPS_LOCK, *modifiers)) cmd |= BRL_FLG_CHAR_UPPER;
          if (MOD_TST(MOD_ALT_LEFT, *modifiers)) cmd |= BRL_FLG_CHAR_META;
          if (MOD_TST(MOD_CONTROL_LEFT, *modifiers) || MOD_TST(MOD_CONTROL_RIGHT, *modifiers)) cmd |= BRL_FLG_CHAR_CONTROL;
        }

        if ((blk == BRL_BLK_PASSKEY) && MOD_TST(MOD_ALT_LEFT, *modifiers)) {
          int arg = cmd & BRL_MSK_ARG;
          switch (arg) {
            case BRL_KEY_CURSOR_LEFT:
              cmd = BRL_CMD_SWITCHVT_PREV;
              break;

            case BRL_KEY_CURSOR_RIGHT:
              cmd = BRL_CMD_SWITCHVT_NEXT;
              break;

            default:
              if (arg >= BRL_KEY_FUNCTION) {
                cmd = BRL_BLK_SWITCHVT + (arg - BRL_KEY_FUNCTION);
              }
              break;
          }
        }

        *command = cmd;
        return 1;
      }
    } else {
      switch (cmd) {
        case MOD_SCROLL_LOCK:
        case MOD_NUMBER_LOCK:
        case MOD_CAPS_LOCK:
          if (!release) {
            if (MOD_TST(cmd, *modifiers)) {
              MOD_CLR(cmd, *modifiers);
            } else {
              MOD_SET(cmd, *modifiers);
            }
          }
          break;

        case MOD_SHIFT_LEFT:
        case MOD_SHIFT_RIGHT:
        case MOD_CONTROL_LEFT:
        case MOD_CONTROL_RIGHT:
        case MOD_ALT_LEFT:
        case MOD_ALT_RIGHT:
          if (release) {
            MOD_CLR(cmd, *modifiers);
          } else {
            MOD_SET(cmd, *modifiers);
          }
          break;
      }
    }
  }

  return 0;
}

static const KeyEntry *const XT_basicScanCodes[] = {
  [XT_KEY_00_Escape] = &keyEntry_Escape,
  [XT_KEY_00_F1] = &keyEntry_F1,
  [XT_KEY_00_F2] = &keyEntry_F2,
  [XT_KEY_00_F3] = &keyEntry_F3,
  [XT_KEY_00_F4] = &keyEntry_F4,
  [XT_KEY_00_F5] = &keyEntry_F5,
  [XT_KEY_00_F6] = &keyEntry_F6,
  [XT_KEY_00_F7] = &keyEntry_F7,
  [XT_KEY_00_F8] = &keyEntry_F8,
  [XT_KEY_00_F9] = &keyEntry_F9,
  [XT_KEY_00_F10] = &keyEntry_F10,
  [XT_KEY_00_F11] = &keyEntry_F11,
  [XT_KEY_00_F12] = &keyEntry_F12,
  [XT_KEY_00_ScrollLock] = &keyEntry_ScrollLock,

  [XT_KEY_00_F13] = &keyEntry_F13,
  [XT_KEY_00_F14] = &keyEntry_F14,
  [XT_KEY_00_F15] = &keyEntry_F15,
  [XT_KEY_00_F16] = &keyEntry_F16,
  [XT_KEY_00_F17] = &keyEntry_F17,
  [XT_KEY_00_F18] = &keyEntry_F18,
  [XT_KEY_00_F19] = &keyEntry_F19,
  [XT_KEY_00_F20] = &keyEntry_F20,
  [XT_KEY_00_F21] = &keyEntry_F21,
  [XT_KEY_00_F22] = &keyEntry_F22,
  [XT_KEY_00_F23] = &keyEntry_F23,
  [XT_KEY_00_F24] = &keyEntry_F24,

  [XT_KEY_00_Grave] = &keyEntry_Grave,
  [XT_KEY_00_1] = &keyEntry_1,
  [XT_KEY_00_2] = &keyEntry_2,
  [XT_KEY_00_3] = &keyEntry_3,
  [XT_KEY_00_4] = &keyEntry_4,
  [XT_KEY_00_5] = &keyEntry_5,
  [XT_KEY_00_6] = &keyEntry_6,
  [XT_KEY_00_7] = &keyEntry_7,
  [XT_KEY_00_8] = &keyEntry_8,
  [XT_KEY_00_9] = &keyEntry_9,
  [XT_KEY_00_0] = &keyEntry_0,
  [XT_KEY_00_Minus] = &keyEntry_Minus,
  [XT_KEY_00_Equal] = &keyEntry_Equal,
  [XT_KEY_00_Backspace] = &keyEntry_Backspace,

  [XT_KEY_00_Tab] = &keyEntry_Tab,
  [XT_KEY_00_Q] = &keyEntry_Q,
  [XT_KEY_00_W] = &keyEntry_W,
  [XT_KEY_00_E] = &keyEntry_E,
  [XT_KEY_00_R] = &keyEntry_R,
  [XT_KEY_00_T] = &keyEntry_T,
  [XT_KEY_00_Y] = &keyEntry_Y,
  [XT_KEY_00_U] = &keyEntry_U,
  [XT_KEY_00_I] = &keyEntry_I,
  [XT_KEY_00_O] = &keyEntry_O,
  [XT_KEY_00_P] = &keyEntry_P,
  [XT_KEY_00_LeftBracket] = &keyEntry_LeftBracket,
  [XT_KEY_00_RightBracket] = &keyEntry_RightBracket,
  [XT_KEY_00_Backslash] = &keyEntry_Backslash,

  [XT_KEY_00_CapsLock] = &keyEntry_CapsLock,
  [XT_KEY_00_A] = &keyEntry_A,
  [XT_KEY_00_S] = &keyEntry_S,
  [XT_KEY_00_D] = &keyEntry_D,
  [XT_KEY_00_F] = &keyEntry_F,
  [XT_KEY_00_G] = &keyEntry_G,
  [XT_KEY_00_H] = &keyEntry_H,
  [XT_KEY_00_J] = &keyEntry_J,
  [XT_KEY_00_K] = &keyEntry_K,
  [XT_KEY_00_L] = &keyEntry_L,
  [XT_KEY_00_Semicolon] = &keyEntry_Semicolon,
  [XT_KEY_00_Apostrophe] = &keyEntry_Apostrophe,
  [XT_KEY_00_Enter] = &keyEntry_Enter,

  [XT_KEY_00_LeftShift] = &keyEntry_LeftShift,
  [XT_KEY_00_Europe2] = &keyEntry_Europe2,
  [XT_KEY_00_Z] = &keyEntry_Z,
  [XT_KEY_00_X] = &keyEntry_X,
  [XT_KEY_00_C] = &keyEntry_C,
  [XT_KEY_00_V] = &keyEntry_V,
  [XT_KEY_00_B] = &keyEntry_B,
  [XT_KEY_00_N] = &keyEntry_N,
  [XT_KEY_00_M] = &keyEntry_M,
  [XT_KEY_00_Comma] = &keyEntry_Comma,
  [XT_KEY_00_Period] = &keyEntry_Period,
  [XT_KEY_00_Slash] = &keyEntry_Slash,
  [XT_KEY_00_RightShift] = &keyEntry_RightShift,

  [XT_KEY_00_LeftControl] = &keyEntry_LeftControl,
  [XT_KEY_00_LeftAlt] = &keyEntry_LeftAlt,
  [XT_KEY_00_Space] = &keyEntry_Space,

  [XT_KEY_00_NumLock] = &keyEntry_NumLock,
  [XT_KEY_00_KPAsterisk] = &keyEntry_KPAsterisk,
  [XT_KEY_00_KPMinus] = &keyEntry_KPMinus,
  [XT_KEY_00_KPPlus] = &keyEntry_KPPlus,
  [XT_KEY_00_KPPeriod] = &keyEntry_KPPeriod,
  [XT_KEY_00_KP0] = &keyEntry_KP0,
  [XT_KEY_00_KP1] = &keyEntry_KP1,
  [XT_KEY_00_KP2] = &keyEntry_KP2,
  [XT_KEY_00_KP3] = &keyEntry_KP3,
  [XT_KEY_00_KP4] = &keyEntry_KP4,
  [XT_KEY_00_KP5] = &keyEntry_KP5,
  [XT_KEY_00_KP6] = &keyEntry_KP6,
  [XT_KEY_00_KP7] = &keyEntry_KP7,
  [XT_KEY_00_KP8] = &keyEntry_KP8,
  [XT_KEY_00_KP9] = &keyEntry_KP9,
};

static const KeyEntry *const XT_emul0ScanCodes[] = {
  [XT_KEY_E0_LeftGUI] = &keyEntry_LeftGUI,
  [XT_KEY_E0_RightAlt] = &keyEntry_RightAlt,
  [XT_KEY_E0_RightGUI] = &keyEntry_RightGUI,
  [XT_KEY_E0_App] = &keyEntry_App,
  [XT_KEY_E0_RightControl] = &keyEntry_RightControl,

  [XT_KEY_E0_Insert] = &keyEntry_Insert,
  [XT_KEY_E0_Delete] = &keyEntry_Delete,
  [XT_KEY_E0_Home] = &keyEntry_Home,
  [XT_KEY_E0_End] = &keyEntry_End,
  [XT_KEY_E0_PageUp] = &keyEntry_PageUp,
  [XT_KEY_E0_PageDown] = &keyEntry_PageDown,

  [XT_KEY_E0_ArrowUp] = &keyEntry_ArrowUp,
  [XT_KEY_E0_ArrowLeft] = &keyEntry_ArrowLeft,
  [XT_KEY_E0_ArrowDown] = &keyEntry_ArrowDown,
  [XT_KEY_E0_ArrowRight] = &keyEntry_ArrowRight,

  [XT_KEY_E0_KPSlash] = &keyEntry_KPSlash,
  [XT_KEY_E0_KPEnter] = &keyEntry_KPEnter,
};

#define XT_emul1ScanCodes NULL

static const KeyEntry *const *XT_scanCodes;
static size_t XT_scanCodesSize;
static unsigned int XT_scanCodeModifiers;

int
xtInterpretScanCode (int *command, unsigned char byte) {
  if (byte == XT_MOD_E0) {
    USE_SCAN_CODES(XT, emul0);
  } else if (byte == XT_MOD_E1) {
    USE_SCAN_CODES(XT, emul1);
  } else if (byte < XT_scanCodesSize) {
    const KeyEntry *key = XT_scanCodes[byte & 0X7F];
    int release = (byte & XT_BIT_RELEASE) != 0;

    USE_SCAN_CODES(XT, basic);

    return interpretKey(command, key, release, &XT_scanCodeModifiers);
  }
  return 0;
}

static const KeyEntry *const AT_basicScanCodes[] = {
  [AT_KEY_00_Escape] = &keyEntry_Escape,
  [AT_KEY_00_F1] = &keyEntry_F1,
  [AT_KEY_00_F2] = &keyEntry_F2,
  [AT_KEY_00_F3] = &keyEntry_F3,
  [AT_KEY_00_F4] = &keyEntry_F4,
  [AT_KEY_00_F5] = &keyEntry_F5,
  [AT_KEY_00_F6] = &keyEntry_F6,
  [AT_KEY_00_F7] = &keyEntry_F7,
  [AT_KEY_00_F8] = &keyEntry_F8,
  [AT_KEY_00_F9] = &keyEntry_F9,
  [AT_KEY_00_F10] = &keyEntry_F10,
  [AT_KEY_00_F11] = &keyEntry_F11,
  [AT_KEY_00_F12] = &keyEntry_F12,
  [AT_KEY_00_ScrollLock] = &keyEntry_ScrollLock,

  [AT_KEY_00_F13] = &keyEntry_F13,
  [AT_KEY_00_F14] = &keyEntry_F14,
  [AT_KEY_00_F15] = &keyEntry_F15,
  [AT_KEY_00_F16] = &keyEntry_F16,
  [AT_KEY_00_F17] = &keyEntry_F17,
  [AT_KEY_00_F18] = &keyEntry_F18,
  [AT_KEY_00_F19] = &keyEntry_F19,
  [AT_KEY_00_F20] = &keyEntry_F20,
  [AT_KEY_00_F21] = &keyEntry_F21,
  [AT_KEY_00_F22] = &keyEntry_F22,
  [AT_KEY_00_F23] = &keyEntry_F23,
  [AT_KEY_00_F24] = &keyEntry_F24,

  [AT_KEY_00_Grave] = &keyEntry_Grave,
  [AT_KEY_00_1] = &keyEntry_1,
  [AT_KEY_00_2] = &keyEntry_2,
  [AT_KEY_00_3] = &keyEntry_3,
  [AT_KEY_00_4] = &keyEntry_4,
  [AT_KEY_00_5] = &keyEntry_5,
  [AT_KEY_00_6] = &keyEntry_6,
  [AT_KEY_00_7] = &keyEntry_7,
  [AT_KEY_00_8] = &keyEntry_8,
  [AT_KEY_00_9] = &keyEntry_9,
  [AT_KEY_00_0] = &keyEntry_0,
  [AT_KEY_00_Minus] = &keyEntry_Minus,
  [AT_KEY_00_Equal] = &keyEntry_Equal,
  [AT_KEY_00_Backspace] = &keyEntry_Backspace,

  [AT_KEY_00_Tab] = &keyEntry_Tab,
  [AT_KEY_00_Q] = &keyEntry_Q,
  [AT_KEY_00_W] = &keyEntry_W,
  [AT_KEY_00_E] = &keyEntry_E,
  [AT_KEY_00_R] = &keyEntry_R,
  [AT_KEY_00_T] = &keyEntry_T,
  [AT_KEY_00_Y] = &keyEntry_Y,
  [AT_KEY_00_U] = &keyEntry_U,
  [AT_KEY_00_I] = &keyEntry_I,
  [AT_KEY_00_O] = &keyEntry_O,
  [AT_KEY_00_P] = &keyEntry_P,
  [AT_KEY_00_LeftBracket] = &keyEntry_LeftBracket,
  [AT_KEY_00_RightBracket] = &keyEntry_RightBracket,
  [AT_KEY_00_Backslash] = &keyEntry_Backslash,

  [AT_KEY_00_CapsLock] = &keyEntry_CapsLock,
  [AT_KEY_00_A] = &keyEntry_A,
  [AT_KEY_00_S] = &keyEntry_S,
  [AT_KEY_00_D] = &keyEntry_D,
  [AT_KEY_00_F] = &keyEntry_F,
  [AT_KEY_00_G] = &keyEntry_G,
  [AT_KEY_00_H] = &keyEntry_H,
  [AT_KEY_00_J] = &keyEntry_J,
  [AT_KEY_00_K] = &keyEntry_K,
  [AT_KEY_00_L] = &keyEntry_L,
  [AT_KEY_00_Semicolon] = &keyEntry_Semicolon,
  [AT_KEY_00_Apostrophe] = &keyEntry_Apostrophe,
  [AT_KEY_00_Enter] = &keyEntry_Enter,

  [AT_KEY_00_LeftShift] = &keyEntry_LeftShift,
  [AT_KEY_00_Europe2] = &keyEntry_Europe2,
  [AT_KEY_00_Z] = &keyEntry_Z,
  [AT_KEY_00_X] = &keyEntry_X,
  [AT_KEY_00_C] = &keyEntry_C,
  [AT_KEY_00_V] = &keyEntry_V,
  [AT_KEY_00_B] = &keyEntry_B,
  [AT_KEY_00_N] = &keyEntry_N,
  [AT_KEY_00_M] = &keyEntry_M,
  [AT_KEY_00_Comma] = &keyEntry_Comma,
  [AT_KEY_00_Period] = &keyEntry_Period,
  [AT_KEY_00_Slash] = &keyEntry_Slash,
  [AT_KEY_00_RightShift] = &keyEntry_RightShift,

  [AT_KEY_00_LeftControl] = &keyEntry_LeftControl,
  [AT_KEY_00_LeftAlt] = &keyEntry_LeftAlt,
  [AT_KEY_00_Space] = &keyEntry_Space,

  [AT_KEY_00_NumLock] = &keyEntry_NumLock,
  [AT_KEY_00_KPAsterisk] = &keyEntry_KPAsterisk,
  [AT_KEY_00_KPMinus] = &keyEntry_KPMinus,
  [AT_KEY_00_KPPlus] = &keyEntry_KPPlus,
  [AT_KEY_00_KPPeriod] = &keyEntry_KPPeriod,
  [AT_KEY_00_KP0] = &keyEntry_KP0,
  [AT_KEY_00_KP1] = &keyEntry_KP1,
  [AT_KEY_00_KP2] = &keyEntry_KP2,
  [AT_KEY_00_KP3] = &keyEntry_KP3,
  [AT_KEY_00_KP4] = &keyEntry_KP4,
  [AT_KEY_00_KP5] = &keyEntry_KP5,
  [AT_KEY_00_KP6] = &keyEntry_KP6,
  [AT_KEY_00_KP7] = &keyEntry_KP7,
  [AT_KEY_00_KP8] = &keyEntry_KP8,
  [AT_KEY_00_KP9] = &keyEntry_KP9,
};

static const KeyEntry *const AT_emul0ScanCodes[] = {
  [AT_KEY_E0_LeftGUI] = &keyEntry_LeftGUI,
  [AT_KEY_E0_RightAlt] = &keyEntry_RightAlt,
  [AT_KEY_E0_RightGUI] = &keyEntry_RightGUI,
  [AT_KEY_E0_App] = &keyEntry_App,
  [AT_KEY_E0_RightControl] = &keyEntry_RightControl,

  [AT_KEY_E0_Insert] = &keyEntry_Insert,
  [AT_KEY_E0_Delete] = &keyEntry_Delete,
  [AT_KEY_E0_Home] = &keyEntry_Home,
  [AT_KEY_E0_End] = &keyEntry_End,
  [AT_KEY_E0_PageUp] = &keyEntry_PageUp,
  [AT_KEY_E0_PageDown] = &keyEntry_PageDown,

  [AT_KEY_E0_ArrowUp] = &keyEntry_ArrowUp,
  [AT_KEY_E0_ArrowLeft] = &keyEntry_ArrowLeft,
  [AT_KEY_E0_ArrowDown] = &keyEntry_ArrowDown,
  [AT_KEY_E0_ArrowRight] = &keyEntry_ArrowRight,

  [AT_KEY_E0_KPSlash] = &keyEntry_KPSlash,
  [AT_KEY_E0_KPEnter] = &keyEntry_KPEnter,
};

#define AT_emul1ScanCodes NULL

static const KeyEntry *const *AT_scanCodes;
static size_t AT_scanCodesSize;
static unsigned int AT_scanCodeModifiers;

int
atInterpretScanCode (int *command, unsigned char byte) {
  if (byte == AT_MOD_RELEASE) {
    MOD_SET(MOD_RELEASE, AT_scanCodeModifiers);
  } else if (byte == AT_MOD_E0) {
    USE_SCAN_CODES(AT, emul0);
  } else if (byte == AT_MOD_E1) {
    USE_SCAN_CODES(AT, emul1);
  } else if (byte < AT_scanCodesSize) {
    const KeyEntry *key = AT_scanCodes[byte];
    int release = MOD_TST(MOD_RELEASE, AT_scanCodeModifiers);

    MOD_CLR(MOD_RELEASE, AT_scanCodeModifiers);
    USE_SCAN_CODES(AT, basic);

    return interpretKey(command, key, release, &AT_scanCodeModifiers);
  }
  return 0;
}

void
resetScanCodes (void) {
  USE_SCAN_CODES(XT, basic);
  XT_scanCodeModifiers = 0;

  USE_SCAN_CODES(AT, basic);
  AT_scanCodeModifiers = 0;
}
