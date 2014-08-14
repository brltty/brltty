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

#include "scancodes.h"
#include "keycodes.h"
#include "brl_cmds.h"

const unsigned char at2Xt[0X80] = {
  [0X00] = 0X00,
  [AT_KEY_00_Escape] = XT_KEY_00_Escape,
  [AT_KEY_00_1] = XT_KEY_00_1,
  [AT_KEY_00_2] = XT_KEY_00_2,
  [AT_KEY_00_3] = XT_KEY_00_3,
  [AT_KEY_00_4] = XT_KEY_00_4,
  [AT_KEY_00_5] = XT_KEY_00_5,
  [AT_KEY_00_6] = XT_KEY_00_6,
  [AT_KEY_00_7] = XT_KEY_00_7,
  [AT_KEY_00_8] = XT_KEY_00_8,
  [AT_KEY_00_9] = XT_KEY_00_9,
  [AT_KEY_00_0] = XT_KEY_00_0,
  [AT_KEY_00_Minus] = XT_KEY_00_Minus,
  [AT_KEY_00_Equal] = XT_KEY_00_Equal,
  [AT_KEY_00_Backspace] = XT_KEY_00_Backspace,
  [AT_KEY_00_Tab] = XT_KEY_00_Tab,
  [AT_KEY_00_Q] = XT_KEY_00_Q,
  [AT_KEY_00_W] = XT_KEY_00_W,
  [AT_KEY_00_E] = XT_KEY_00_E,
  [AT_KEY_00_R] = XT_KEY_00_R,
  [AT_KEY_00_T] = XT_KEY_00_T,
  [AT_KEY_00_Y] = XT_KEY_00_Y,
  [AT_KEY_00_U] = XT_KEY_00_U,
  [AT_KEY_00_I] = XT_KEY_00_I,
  [AT_KEY_00_O] = XT_KEY_00_O,
  [AT_KEY_00_P] = XT_KEY_00_P,
  [AT_KEY_00_LeftBracket] = XT_KEY_00_LeftBracket,
  [AT_KEY_00_RightBracket] = XT_KEY_00_RightBracket,
  [AT_KEY_00_Enter] = XT_KEY_00_Enter,
  [AT_KEY_00_LeftControl] = XT_KEY_00_LeftControl,
  [AT_KEY_00_A] = XT_KEY_00_A,
  [AT_KEY_00_S] = XT_KEY_00_S,
  [AT_KEY_00_D] = XT_KEY_00_D,
  [AT_KEY_00_F] = XT_KEY_00_F,
  [AT_KEY_00_G] = XT_KEY_00_G,
  [AT_KEY_00_H] = XT_KEY_00_H,
  [AT_KEY_00_J] = XT_KEY_00_J,
  [AT_KEY_00_K] = XT_KEY_00_K,
  [AT_KEY_00_L] = XT_KEY_00_L,
  [AT_KEY_00_Semicolon] = XT_KEY_00_Semicolon,
  [AT_KEY_00_Apostrophe] = XT_KEY_00_Apostrophe,
  [AT_KEY_00_Grave] = XT_KEY_00_Grave,
  [AT_KEY_00_LeftShift] = XT_KEY_00_LeftShift,
  [AT_KEY_00_Backslash] = XT_KEY_00_Backslash,
  [AT_KEY_00_Z] = XT_KEY_00_Z,
  [AT_KEY_00_X] = XT_KEY_00_X,
  [AT_KEY_00_C] = XT_KEY_00_C,
  [AT_KEY_00_V] = XT_KEY_00_V,
  [AT_KEY_00_B] = XT_KEY_00_B,
  [AT_KEY_00_N] = XT_KEY_00_N,
  [AT_KEY_00_M] = XT_KEY_00_M,
  [AT_KEY_00_Comma] = XT_KEY_00_Comma,
  [AT_KEY_00_Period] = XT_KEY_00_Period,
  [AT_KEY_00_Slash] = XT_KEY_00_Slash,
  [AT_KEY_00_RightShift] = XT_KEY_00_RightShift,
  [AT_KEY_00_KPAsterisk] = XT_KEY_00_KPAsterisk,
  [AT_KEY_00_LeftAlt] = XT_KEY_00_LeftAlt,
  [AT_KEY_00_Space] = XT_KEY_00_Space,
  [AT_KEY_00_CapsLock] = XT_KEY_00_CapsLock,
  [AT_KEY_00_F1] = XT_KEY_00_F1,
  [AT_KEY_00_F2] = XT_KEY_00_F2,
  [AT_KEY_00_F3] = XT_KEY_00_F3,
  [AT_KEY_00_F4] = XT_KEY_00_F4,
  [AT_KEY_00_F5] = XT_KEY_00_F5,
  [AT_KEY_00_F6] = XT_KEY_00_F6,
  [AT_KEY_00_F7_X1] = XT_KEY_00_F7,
  [AT_KEY_00_F8] = XT_KEY_00_F8,
  [AT_KEY_00_F9] = XT_KEY_00_F9,
  [AT_KEY_00_F10] = XT_KEY_00_F10,
  [AT_KEY_00_NumLock] = XT_KEY_00_NumLock,
  [AT_KEY_00_ScrollLock] = XT_KEY_00_ScrollLock,
  [AT_KEY_00_KP7] = XT_KEY_00_KP7,
  [AT_KEY_00_KP8] = XT_KEY_00_KP8,
  [AT_KEY_00_KP9] = XT_KEY_00_KP9,
  [AT_KEY_00_KPMinus] = XT_KEY_00_KPMinus,
  [AT_KEY_00_KP4] = XT_KEY_00_KP4,
  [AT_KEY_00_KP5] = XT_KEY_00_KP5,
  [AT_KEY_00_KP6] = XT_KEY_00_KP6,
  [AT_KEY_00_KPPlus] = XT_KEY_00_KPPlus,
  [AT_KEY_00_KP1] = XT_KEY_00_KP1,
  [AT_KEY_00_KP2] = XT_KEY_00_KP2,
  [AT_KEY_00_KP3] = XT_KEY_00_KP3,
  [AT_KEY_00_KP0] = XT_KEY_00_KP0,
  [AT_KEY_00_KPPeriod] = XT_KEY_00_KPPeriod,
  [0X7F] = 0X54,
  [0X60] = 0X55,
  [AT_KEY_00_Europe2] = XT_KEY_00_Europe2,
  [AT_KEY_00_F11] = XT_KEY_00_F11,
  [AT_KEY_00_F12] = XT_KEY_00_F12,
  [AT_KEY_00_KPEqual] = XT_KEY_00_KPEqual,
  [0X17] = 0X5A,
  [0X1F] = 0X5B,
  [AT_KEY_00_International6] = XT_KEY_00_International6,
  [0X2F] = 0X5D,
  [0X37] = 0X5E,
  [0X3F] = 0X5F,
  [0X47] = 0X60,
  [0X4F] = 0X61,
  [0X56] = 0X62,
  [0X5E] = 0X63,
  [AT_KEY_00_F13] = XT_KEY_00_F13,
  [AT_KEY_00_F14] = XT_KEY_00_F14,
  [AT_KEY_00_F15] = XT_KEY_00_F15,
  [AT_KEY_00_F16] = XT_KEY_00_F16,
  [AT_KEY_00_F17] = XT_KEY_00_F17,
  [AT_KEY_00_F18] = XT_KEY_00_F18,
  [AT_KEY_00_F19] = XT_KEY_00_F19,
  [AT_KEY_00_F20] = XT_KEY_00_F20,
  [AT_KEY_00_F21] = XT_KEY_00_F21,
  [AT_KEY_00_F22] = XT_KEY_00_F22,
  [AT_KEY_00_F23] = XT_KEY_00_F23,
  [0X6F] = 0X6F,
  [AT_KEY_00_International2] = XT_KEY_00_International2,
  [0X19] = 0X71,
  [AT_KEY_00_CrSel] = XT_KEY_00_CrSel,
  [AT_KEY_00_International1] = XT_KEY_00_International1,
  [AT_KEY_00_ExSel] = XT_KEY_00_ExSel,
  [AT_KEY_00_EnlHelp] = XT_KEY_00_EnlHelp,
  [AT_KEY_00_F24] = XT_KEY_00_F24,
  [AT_KEY_00_Language4] = XT_KEY_00_Language4,
  [AT_KEY_00_Language3] = XT_KEY_00_Language3,
  [AT_KEY_00_International4] = XT_KEY_00_International4,
  [0X65] = 0X7A,
  [AT_KEY_00_International5] = XT_KEY_00_International5,
  [0X68] = 0X7C,
  [AT_KEY_00_International3] = XT_KEY_00_International3,
  [AT_KEY_00_KPComma] = XT_KEY_00_KPComma,
  [0X6E] = 0X7F
};

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

static const KeyEntry AT_basicScanCodes[] = {
  [AT_KEY_00_Escape] = {BRL_BLK_PASSKEY+BRL_KEY_ESCAPE},
  [AT_KEY_00_F1] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+0},
  [AT_KEY_00_F2] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+1},
  [AT_KEY_00_F3] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+2},
  [AT_KEY_00_F4] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+3},
  [AT_KEY_00_F5] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+4},
  [AT_KEY_00_F6] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+5},
  [AT_KEY_00_F7] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+6},
  [AT_KEY_00_F8] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+7},
  [AT_KEY_00_F9] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+8},
  [AT_KEY_00_F10] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+9},
  [AT_KEY_00_F11] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+10},
  [AT_KEY_00_F12] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+11},
  [AT_KEY_00_ScrollLock] = {MOD_SCROLL_LOCK},

  [AT_KEY_00_F13] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+12},
  [AT_KEY_00_F14] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+13},
  [AT_KEY_00_F15] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+14},
  [AT_KEY_00_F16] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+15},
  [AT_KEY_00_F17] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+16},
  [AT_KEY_00_F18] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+17},
  [AT_KEY_00_F19] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+18},
  [AT_KEY_00_F20] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+19},
  [AT_KEY_00_F21] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+20},
  [AT_KEY_00_F22] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+21},
  [AT_KEY_00_F23] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+22},
  [AT_KEY_00_F24] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+23},

  [AT_KEY_00_Grave] = {CMD_CHAR(WC_C('`')), CMD_CHAR(WC_C('~'))},
  [AT_KEY_00_1] = {CMD_CHAR(WC_C('1')), CMD_CHAR(WC_C('!'))},
  [AT_KEY_00_2] = {CMD_CHAR(WC_C('2')), CMD_CHAR(WC_C('@'))},
  [AT_KEY_00_3] = {CMD_CHAR(WC_C('3')), CMD_CHAR(WC_C('#'))},
  [AT_KEY_00_4] = {CMD_CHAR(WC_C('4')), CMD_CHAR(WC_C('$'))},
  [AT_KEY_00_5] = {CMD_CHAR(WC_C('5')), CMD_CHAR(WC_C('%'))},
  [AT_KEY_00_6] = {CMD_CHAR(WC_C('6')), CMD_CHAR(WC_C('^'))},
  [AT_KEY_00_7] = {CMD_CHAR(WC_C('7')), CMD_CHAR(WC_C('&'))},
  [AT_KEY_00_8] = {CMD_CHAR(WC_C('8')), CMD_CHAR(WC_C('*'))},
  [AT_KEY_00_9] = {CMD_CHAR(WC_C('9')), CMD_CHAR(WC_C('('))},
  [AT_KEY_00_0] = {CMD_CHAR(WC_C('0')), CMD_CHAR(WC_C(')'))},
  [AT_KEY_00_Minus] = {CMD_CHAR(WC_C('-')), CMD_CHAR(WC_C('_'))},
  [AT_KEY_00_Equal] = {CMD_CHAR(WC_C('=')), CMD_CHAR(WC_C('+'))},
  [AT_KEY_00_Backspace] = {BRL_BLK_PASSKEY+BRL_KEY_BACKSPACE},

  [AT_KEY_00_Tab] = {BRL_BLK_PASSKEY+BRL_KEY_TAB},
  [AT_KEY_00_Q] = {CMD_CHAR(WC_C('q')), CMD_CHAR(WC_C('Q'))},
  [AT_KEY_00_W] = {CMD_CHAR(WC_C('w')), CMD_CHAR(WC_C('W'))},
  [AT_KEY_00_E] = {CMD_CHAR(WC_C('e')), CMD_CHAR(WC_C('E'))},
  [AT_KEY_00_R] = {CMD_CHAR(WC_C('r')), CMD_CHAR(WC_C('R'))},
  [AT_KEY_00_T] = {CMD_CHAR(WC_C('t')), CMD_CHAR(WC_C('T'))},
  [AT_KEY_00_Y] = {CMD_CHAR(WC_C('y')), CMD_CHAR(WC_C('Y'))},
  [AT_KEY_00_U] = {CMD_CHAR(WC_C('u')), CMD_CHAR(WC_C('U'))},
  [AT_KEY_00_I] = {CMD_CHAR(WC_C('i')), CMD_CHAR(WC_C('I'))},
  [AT_KEY_00_O] = {CMD_CHAR(WC_C('o')), CMD_CHAR(WC_C('O'))},
  [AT_KEY_00_P] = {CMD_CHAR(WC_C('p')), CMD_CHAR(WC_C('P'))},
  [AT_KEY_00_LeftBracket] = {CMD_CHAR(WC_C('[')), CMD_CHAR(WC_C('{'))},
  [AT_KEY_00_RightBracket] = {CMD_CHAR(WC_C(']')), CMD_CHAR(WC_C('}'))},
  [AT_KEY_00_Backslash] = {CMD_CHAR('\\'), CMD_CHAR(WC_C('|'))},

  [AT_KEY_00_CapsLock] = {MOD_CAPS_LOCK},
  [AT_KEY_00_A] = {CMD_CHAR(WC_C('a')), CMD_CHAR(WC_C('A'))},
  [AT_KEY_00_S] = {CMD_CHAR(WC_C('s')), CMD_CHAR(WC_C('S'))},
  [AT_KEY_00_D] = {CMD_CHAR(WC_C('d')), CMD_CHAR(WC_C('D'))},
  [AT_KEY_00_F] = {CMD_CHAR(WC_C('f')), CMD_CHAR(WC_C('F'))},
  [AT_KEY_00_G] = {CMD_CHAR(WC_C('g')), CMD_CHAR(WC_C('G'))},
  [AT_KEY_00_H] = {CMD_CHAR(WC_C('h')), CMD_CHAR(WC_C('H'))},
  [AT_KEY_00_J] = {CMD_CHAR(WC_C('j')), CMD_CHAR(WC_C('J'))},
  [AT_KEY_00_K] = {CMD_CHAR(WC_C('k')), CMD_CHAR(WC_C('K'))},
  [AT_KEY_00_L] = {CMD_CHAR(WC_C('l')), CMD_CHAR(WC_C('L'))},
  [AT_KEY_00_Semicolon] = {CMD_CHAR(WC_C(';')), CMD_CHAR(WC_C(':'))},
  [AT_KEY_00_Apostrophe] = {CMD_CHAR(WC_C('\'')), CMD_CHAR(WC_C('"'))},
  [AT_KEY_00_Enter] = {BRL_BLK_PASSKEY+BRL_KEY_ENTER},

  [AT_KEY_00_LeftShift] = {MOD_SHIFT_LEFT},
  [AT_KEY_00_Europe2] = {CMD_CHAR(WC_C('<')), CMD_CHAR(WC_C('>'))},
  [AT_KEY_00_Z] = {CMD_CHAR(WC_C('z')), CMD_CHAR(WC_C('Z'))},
  [AT_KEY_00_X] = {CMD_CHAR(WC_C('x')), CMD_CHAR(WC_C('X'))},
  [AT_KEY_00_C] = {CMD_CHAR(WC_C('c')), CMD_CHAR(WC_C('C'))},
  [AT_KEY_00_V] = {CMD_CHAR(WC_C('v')), CMD_CHAR(WC_C('V'))},
  [AT_KEY_00_B] = {CMD_CHAR(WC_C('b')), CMD_CHAR(WC_C('B'))},
  [AT_KEY_00_N] = {CMD_CHAR(WC_C('n')), CMD_CHAR(WC_C('N'))},
  [AT_KEY_00_M] = {CMD_CHAR(WC_C('m')), CMD_CHAR(WC_C('M'))},
  [AT_KEY_00_Comma] = {CMD_CHAR(WC_C(',')), CMD_CHAR(WC_C('<'))},
  [AT_KEY_00_Period] = {CMD_CHAR(WC_C('.')), CMD_CHAR(WC_C('>'))},
  [AT_KEY_00_Slash] = {CMD_CHAR(WC_C('/')), CMD_CHAR(WC_C('?'))},
  [AT_KEY_00_RightShift] = {MOD_SHIFT_RIGHT},

  [AT_KEY_00_LeftControl] = {MOD_CONTROL_LEFT},
  [AT_KEY_00_LeftAlt] = {MOD_ALT_LEFT},
  [AT_KEY_00_Space] = {CMD_CHAR(WC_C(' '))},

  [AT_KEY_00_NumLock] = {MOD_NUMBER_LOCK},
  [AT_KEY_00_KPAsterisk] = {CMD_CHAR(WC_C('*'))},
  [AT_KEY_00_KPMinus] = {CMD_CHAR(WC_C('-'))},
  [AT_KEY_00_KPPlus] = {CMD_CHAR(WC_C('+'))},
  [AT_KEY_00_KPPeriod] = {BRL_BLK_PASSKEY+BRL_KEY_DELETE, CMD_CHAR(WC_C('.'))},
  [AT_KEY_00_KP0] = {BRL_BLK_PASSKEY+BRL_KEY_INSERT, CMD_CHAR(WC_C('0'))},
  [AT_KEY_00_KP1] = {BRL_BLK_PASSKEY+BRL_KEY_END, CMD_CHAR(WC_C('1'))},
  [AT_KEY_00_KP2] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN, CMD_CHAR(WC_C('2'))},
  [AT_KEY_00_KP3] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN, CMD_CHAR(WC_C('3'))},
  [AT_KEY_00_KP4] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT, CMD_CHAR(WC_C('4'))},
  [AT_KEY_00_KP5] = {CMD_CHAR(WC_C('5'))},
  [AT_KEY_00_KP6] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, CMD_CHAR(WC_C('6'))},
  [AT_KEY_00_KP7] = {BRL_BLK_PASSKEY+BRL_KEY_HOME, CMD_CHAR(WC_C('7'))},
  [AT_KEY_00_KP8] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP, CMD_CHAR(WC_C('8'))},
  [AT_KEY_00_KP9] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP, CMD_CHAR(WC_C('9'))}
};

static const KeyEntry AT_emul0ScanCodes[] = {
  [AT_KEY_E0_LeftGUI] = {MOD_WINDOWS_LEFT},
  [AT_KEY_E0_RightAlt] = {MOD_ALT_RIGHT},
  [AT_KEY_E0_RightGUI] = {MOD_WINDOWS_RIGHT},
  [AT_KEY_E0_App] = {MOD_APP},
  [AT_KEY_E0_RightControl] = {MOD_CONTROL_RIGHT},

  [AT_KEY_E0_Insert] = {BRL_BLK_PASSKEY+BRL_KEY_INSERT},
  [AT_KEY_E0_Delete] = {BRL_BLK_PASSKEY+BRL_KEY_DELETE},
  [AT_KEY_E0_Home] = {BRL_BLK_PASSKEY+BRL_KEY_HOME},
  [AT_KEY_E0_End] = {BRL_BLK_PASSKEY+BRL_KEY_END},
  [AT_KEY_E0_PageUp] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP},
  [AT_KEY_E0_PageDown] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN},

  [AT_KEY_E0_ArrowUp] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP},
  [AT_KEY_E0_ArrowLeft] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT},
  [AT_KEY_E0_ArrowDown] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN},
  [AT_KEY_E0_ArrowRight] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT},

  [AT_KEY_E0_KPSlash] = {CMD_CHAR(WC_C('/'))},
  [AT_KEY_E0_KPEnter] = {BRL_BLK_PASSKEY+BRL_KEY_ENTER}
};

#define AT_emul1ScanCodes NULL

static const KeyEntry *AT_scanCodes;
static size_t AT_scanCodesSize;
static unsigned int AT_scanCodeModifiers;

int
atInterpretScanCode (int *command, unsigned char byte) {
  if (byte == 0XF0) {
    MOD_SET(MOD_RELEASE, AT_scanCodeModifiers);
  } else if (byte == 0XE0) {
    USE_SCAN_CODES(AT, emul0);
  } else if (byte == 0XE1) {
    USE_SCAN_CODES(AT, emul1);
  } else if (byte < AT_scanCodesSize) {
    const KeyEntry *key = &AT_scanCodes[byte];
    int release = MOD_TST(MOD_RELEASE, AT_scanCodeModifiers);

    MOD_CLR(MOD_RELEASE, AT_scanCodeModifiers);
    USE_SCAN_CODES(AT, basic);

    return interpretKey(command, key, release, &AT_scanCodeModifiers);
  }
  return 0;
}

static const KeyEntry XT_basicScanCodes[] = {
  [XT_KEY_00_Escape] = {BRL_BLK_PASSKEY+BRL_KEY_ESCAPE},
  [XT_KEY_00_F1] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+0},
  [XT_KEY_00_F2] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+1},
  [XT_KEY_00_F3] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+2},
  [XT_KEY_00_F4] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+3},
  [XT_KEY_00_F5] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+4},
  [XT_KEY_00_F6] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+5},
  [XT_KEY_00_F7] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+6},
  [XT_KEY_00_F8] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+7},
  [XT_KEY_00_F9] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+8},
  [XT_KEY_00_F10] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+9},
  [XT_KEY_00_F11] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+10},
  [XT_KEY_00_F12] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+11},
  [XT_KEY_00_ScrollLock] = {MOD_SCROLL_LOCK},

  [XT_KEY_00_F13] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+12},
  [XT_KEY_00_F14] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+13},
  [XT_KEY_00_F15] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+14},
  [XT_KEY_00_F16] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+15},
  [XT_KEY_00_F17] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+16},
  [XT_KEY_00_F18] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+17},
  [XT_KEY_00_F19] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+18},
  [XT_KEY_00_F20] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+19},
  [XT_KEY_00_F21] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+20},
  [XT_KEY_00_F22] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+21},
  [XT_KEY_00_F23] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+22},
  [XT_KEY_00_F24] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+23},

  [XT_KEY_00_Grave] = {CMD_CHAR(WC_C('`')), CMD_CHAR(WC_C('~'))},
  [XT_KEY_00_1] = {CMD_CHAR(WC_C('1')), CMD_CHAR(WC_C('!'))},
  [XT_KEY_00_2] = {CMD_CHAR(WC_C('2')), CMD_CHAR(WC_C('@'))},
  [XT_KEY_00_3] = {CMD_CHAR(WC_C('3')), CMD_CHAR(WC_C('#'))},
  [XT_KEY_00_4] = {CMD_CHAR(WC_C('4')), CMD_CHAR(WC_C('$'))},
  [XT_KEY_00_5] = {CMD_CHAR(WC_C('5')), CMD_CHAR(WC_C('%'))},
  [XT_KEY_00_6] = {CMD_CHAR(WC_C('6')), CMD_CHAR(WC_C('^'))},
  [XT_KEY_00_7] = {CMD_CHAR(WC_C('7')), CMD_CHAR(WC_C('&'))},
  [XT_KEY_00_8] = {CMD_CHAR(WC_C('8')), CMD_CHAR(WC_C('*'))},
  [XT_KEY_00_9] = {CMD_CHAR(WC_C('9')), CMD_CHAR(WC_C('('))},
  [XT_KEY_00_0] = {CMD_CHAR(WC_C('0')), CMD_CHAR(WC_C(')'))},
  [XT_KEY_00_Minus] = {CMD_CHAR(WC_C('-')), CMD_CHAR(WC_C('_'))},
  [XT_KEY_00_Equal] = {CMD_CHAR(WC_C('=')), CMD_CHAR(WC_C('+'))},
  [XT_KEY_00_Backspace] = {BRL_BLK_PASSKEY+BRL_KEY_BACKSPACE},

  [XT_KEY_00_Tab] = {BRL_BLK_PASSKEY+BRL_KEY_TAB},
  [XT_KEY_00_Q] = {CMD_CHAR(WC_C('q')), CMD_CHAR(WC_C('Q'))},
  [XT_KEY_00_W] = {CMD_CHAR(WC_C('w')), CMD_CHAR(WC_C('W'))},
  [XT_KEY_00_E] = {CMD_CHAR(WC_C('e')), CMD_CHAR(WC_C('E'))},
  [XT_KEY_00_R] = {CMD_CHAR(WC_C('r')), CMD_CHAR(WC_C('R'))},
  [XT_KEY_00_T] = {CMD_CHAR(WC_C('t')), CMD_CHAR(WC_C('T'))},
  [XT_KEY_00_Y] = {CMD_CHAR(WC_C('y')), CMD_CHAR(WC_C('Y'))},
  [XT_KEY_00_U] = {CMD_CHAR(WC_C('u')), CMD_CHAR(WC_C('U'))},
  [XT_KEY_00_I] = {CMD_CHAR(WC_C('i')), CMD_CHAR(WC_C('I'))},
  [XT_KEY_00_O] = {CMD_CHAR(WC_C('o')), CMD_CHAR(WC_C('O'))},
  [XT_KEY_00_P] = {CMD_CHAR(WC_C('p')), CMD_CHAR(WC_C('P'))},
  [XT_KEY_00_LeftBracket] = {CMD_CHAR(WC_C('[')), CMD_CHAR(WC_C('{'))},
  [XT_KEY_00_RightBracket] = {CMD_CHAR(WC_C(']')), CMD_CHAR(WC_C('}'))},
  [XT_KEY_00_Backslash] = {CMD_CHAR('\\'), CMD_CHAR(WC_C('|'))},

  [XT_KEY_00_CapsLock] = {MOD_CAPS_LOCK},
  [XT_KEY_00_A] = {CMD_CHAR(WC_C('a')), CMD_CHAR(WC_C('A'))},
  [XT_KEY_00_S] = {CMD_CHAR(WC_C('s')), CMD_CHAR(WC_C('S'))},
  [XT_KEY_00_D] = {CMD_CHAR(WC_C('d')), CMD_CHAR(WC_C('D'))},
  [XT_KEY_00_F] = {CMD_CHAR(WC_C('f')), CMD_CHAR(WC_C('F'))},
  [XT_KEY_00_G] = {CMD_CHAR(WC_C('g')), CMD_CHAR(WC_C('G'))},
  [XT_KEY_00_H] = {CMD_CHAR(WC_C('h')), CMD_CHAR(WC_C('H'))},
  [XT_KEY_00_J] = {CMD_CHAR(WC_C('j')), CMD_CHAR(WC_C('J'))},
  [XT_KEY_00_K] = {CMD_CHAR(WC_C('k')), CMD_CHAR(WC_C('K'))},
  [XT_KEY_00_L] = {CMD_CHAR(WC_C('l')), CMD_CHAR(WC_C('L'))},
  [XT_KEY_00_Semicolon] = {CMD_CHAR(WC_C(';')), CMD_CHAR(WC_C(':'))},
  [XT_KEY_00_Apostrophe] = {CMD_CHAR(WC_C('\'')), CMD_CHAR(WC_C('"'))},
  [XT_KEY_00_Enter] = {BRL_BLK_PASSKEY+BRL_KEY_ENTER},

  [XT_KEY_00_LeftShift] = {MOD_SHIFT_LEFT},
  [XT_KEY_00_Europe2] = {CMD_CHAR(WC_C('<')), CMD_CHAR(WC_C('>'))},
  [XT_KEY_00_Z] = {CMD_CHAR(WC_C('z')), CMD_CHAR(WC_C('Z'))},
  [XT_KEY_00_X] = {CMD_CHAR(WC_C('x')), CMD_CHAR(WC_C('X'))},
  [XT_KEY_00_C] = {CMD_CHAR(WC_C('c')), CMD_CHAR(WC_C('C'))},
  [XT_KEY_00_V] = {CMD_CHAR(WC_C('v')), CMD_CHAR(WC_C('V'))},
  [XT_KEY_00_B] = {CMD_CHAR(WC_C('b')), CMD_CHAR(WC_C('B'))},
  [XT_KEY_00_N] = {CMD_CHAR(WC_C('n')), CMD_CHAR(WC_C('N'))},
  [XT_KEY_00_M] = {CMD_CHAR(WC_C('m')), CMD_CHAR(WC_C('M'))},
  [XT_KEY_00_Comma] = {CMD_CHAR(WC_C(',')), CMD_CHAR(WC_C('<'))},
  [XT_KEY_00_Period] = {CMD_CHAR(WC_C('.')), CMD_CHAR(WC_C('>'))},
  [XT_KEY_00_Slash] = {CMD_CHAR(WC_C('/')), CMD_CHAR(WC_C('?'))},
  [XT_KEY_00_RightShift] = {MOD_SHIFT_RIGHT},

  [XT_KEY_00_LeftControl] = {MOD_CONTROL_LEFT},
  [XT_KEY_00_LeftAlt] = {MOD_ALT_LEFT},
  [XT_KEY_00_Space] = {CMD_CHAR(WC_C(' '))},

  [XT_KEY_00_NumLock] = {MOD_NUMBER_LOCK},
  [XT_KEY_00_KPAsterisk] = {CMD_CHAR(WC_C('*'))},
  [XT_KEY_00_KPMinus] = {CMD_CHAR(WC_C('-'))},
  [XT_KEY_00_KPPlus] = {CMD_CHAR(WC_C('+'))},
  [XT_KEY_00_KPPeriod] = {BRL_BLK_PASSKEY+BRL_KEY_DELETE, CMD_CHAR(WC_C('.'))},
  [XT_KEY_00_KP0] = {BRL_BLK_PASSKEY+BRL_KEY_INSERT, CMD_CHAR(WC_C('0'))},
  [XT_KEY_00_KP1] = {BRL_BLK_PASSKEY+BRL_KEY_END, CMD_CHAR(WC_C('1'))},
  [XT_KEY_00_KP2] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN, CMD_CHAR(WC_C('2'))},
  [XT_KEY_00_KP3] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN, CMD_CHAR(WC_C('3'))},
  [XT_KEY_00_KP4] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT, CMD_CHAR(WC_C('4'))},
  [XT_KEY_00_KP5] = {CMD_CHAR(WC_C('5'))},
  [XT_KEY_00_KP6] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, CMD_CHAR(WC_C('6'))},
  [XT_KEY_00_KP7] = {BRL_BLK_PASSKEY+BRL_KEY_HOME, CMD_CHAR(WC_C('7'))},
  [XT_KEY_00_KP8] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP, CMD_CHAR(WC_C('8'))},
  [XT_KEY_00_KP9] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP, CMD_CHAR(WC_C('9'))}
};

static const KeyEntry XT_emul0ScanCodes[] = {
  [XT_KEY_E0_LeftGUI] = {MOD_WINDOWS_LEFT},
  [XT_KEY_E0_RightAlt] = {MOD_ALT_RIGHT},
  [XT_KEY_E0_RightGUI] = {MOD_WINDOWS_RIGHT},
  [XT_KEY_E0_App] = {MOD_APP},
  [XT_KEY_E0_RightControl] = {MOD_CONTROL_RIGHT},

  [XT_KEY_E0_Insert] = {BRL_BLK_PASSKEY+BRL_KEY_INSERT},
  [XT_KEY_E0_Delete] = {BRL_BLK_PASSKEY+BRL_KEY_DELETE},
  [XT_KEY_E0_Home] = {BRL_BLK_PASSKEY+BRL_KEY_HOME},
  [XT_KEY_E0_End] = {BRL_BLK_PASSKEY+BRL_KEY_END},
  [XT_KEY_E0_PageUp] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP},
  [XT_KEY_E0_PageDown] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN},

  [XT_KEY_E0_ArrowUp] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP},
  [XT_KEY_E0_ArrowLeft] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT},
  [XT_KEY_E0_ArrowDown] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN},
  [XT_KEY_E0_ArrowRight] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT},

  [XT_KEY_E0_KPSlash] = {CMD_CHAR(WC_C('/'))},
  [XT_KEY_E0_KPEnter] = {BRL_BLK_PASSKEY+BRL_KEY_ENTER}
};

#define XT_emul1ScanCodes NULL

static const KeyEntry *XT_scanCodes;
static size_t XT_scanCodesSize;
static unsigned int XT_scanCodeModifiers;

int
xtInterpretScanCode (int *command, unsigned char byte) {
  if (byte == 0XE0) {
    USE_SCAN_CODES(XT, emul0);
  } else if (byte == 0XE1) {
    USE_SCAN_CODES(XT, emul1);
  } else if (byte < XT_scanCodesSize) {
    const KeyEntry *key = &XT_scanCodes[byte & 0X7F];
    int release = (byte & 0X80) != 0;

    USE_SCAN_CODES(XT, basic);

    return interpretKey(command, key, release, &XT_scanCodeModifiers);
  }
  return 0;
}

void
resetScanCodes (void) {
  USE_SCAN_CODES(AT, basic);
  AT_scanCodeModifiers = 0;

  USE_SCAN_CODES(XT, basic);
  XT_scanCodeModifiers = 0;
}
