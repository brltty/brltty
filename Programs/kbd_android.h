/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_KBD_ANDROID
#define BRLTTY_INCLUDED_KBD_ANDROID

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  ANDROID_KEY_0 = 7,
  ANDROID_KEY_1 = 8,
  ANDROID_KEY_2 = 9,
  ANDROID_KEY_3 = 10,
  ANDROID_KEY_3D_MODE = 206,
  ANDROID_KEY_4 = 11,
  ANDROID_KEY_5 = 12,
  ANDROID_KEY_6 = 13,
  ANDROID_KEY_7 = 14,
  ANDROID_KEY_8 = 15,
  ANDROID_KEY_9 = 16,

  ANDROID_KEY_A = 29,
  ANDROID_KEY_ALT_LEFT = 57,
  ANDROID_KEY_ALT_RIGHT = 58,
  ANDROID_KEY_APOSTROPHE = 75,
  ANDROID_KEY_APP_SWITCH = 187,
  ANDROID_KEY_ASSIST = 219,
  ANDROID_KEY_AT = 77,
  ANDROID_KEY_AVR_INPUT = 182,
  ANDROID_KEY_AVR_POWER = 181,

  ANDROID_KEY_B = 30,
  ANDROID_KEY_BACK = 4,
  ANDROID_KEY_BACKSLASH = 73,
  ANDROID_KEY_BOOKMARK = 174,
  ANDROID_KEY_BREAK = 121,
  ANDROID_KEY_BUTTON_1 = 188,
  ANDROID_KEY_BUTTON_10 = 197,
  ANDROID_KEY_BUTTON_11 = 198,
  ANDROID_KEY_BUTTON_12 = 199,
  ANDROID_KEY_BUTTON_13 = 200,
  ANDROID_KEY_BUTTON_14 = 201,
  ANDROID_KEY_BUTTON_15 = 202,
  ANDROID_KEY_BUTTON_16 = 203,
  ANDROID_KEY_BUTTON_2 = 189,
  ANDROID_KEY_BUTTON_3 = 190,
  ANDROID_KEY_BUTTON_4 = 191,
  ANDROID_KEY_BUTTON_5 = 192,
  ANDROID_KEY_BUTTON_6 = 193,
  ANDROID_KEY_BUTTON_7 = 194,
  ANDROID_KEY_BUTTON_8 = 195,
  ANDROID_KEY_BUTTON_9 = 196,
  ANDROID_KEY_BUTTON_A = 96,
  ANDROID_KEY_BUTTON_B = 97,
  ANDROID_KEY_BUTTON_C = 98,
  ANDROID_KEY_BUTTON_L1 = 102,
  ANDROID_KEY_BUTTON_L2 = 104,
  ANDROID_KEY_BUTTON_MODE = 110,
  ANDROID_KEY_BUTTON_R1 = 103,
  ANDROID_KEY_BUTTON_R2 = 105,
  ANDROID_KEY_BUTTON_SELECT = 109,
  ANDROID_KEY_BUTTON_START = 108,
  ANDROID_KEY_BUTTON_THUMBL = 106,
  ANDROID_KEY_BUTTON_THUMBR = 107,
  ANDROID_KEY_BUTTON_X = 99,
  ANDROID_KEY_BUTTON_Y = 100,
  ANDROID_KEY_BUTTON_Z = 101,

  ANDROID_KEY_C = 31,
  ANDROID_KEY_CALCULATOR = 210,
  ANDROID_KEY_CALENDAR = 208,
  ANDROID_KEY_CALL = 5,
  ANDROID_KEY_CAMERA = 27,
  ANDROID_KEY_CAPS_LOCK = 115,
  ANDROID_KEY_CAPTIONS = 175,
  ANDROID_KEY_CHANNEL_DOWN = 167,
  ANDROID_KEY_CHANNEL_UP = 166,
  ANDROID_KEY_CLEAR = 28,
  ANDROID_KEY_COMMA = 55,
  ANDROID_KEY_CONTACTS = 207,
  ANDROID_KEY_CTRL_LEFT = 113,
  ANDROID_KEY_CTRL_RIGHT = 114,

  ANDROID_KEY_D = 32,
  ANDROID_KEY_DEL = 67,
  ANDROID_KEY_DPAD_CENTER = 23,
  ANDROID_KEY_DPAD_DOWN = 20,
  ANDROID_KEY_DPAD_LEFT = 21,
  ANDROID_KEY_DPAD_RIGHT = 22,
  ANDROID_KEY_DPAD_UP = 19,
  ANDROID_KEY_DVR = 173,

  ANDROID_KEY_E = 33,
  ANDROID_KEY_EISU = 212,
  ANDROID_KEY_ENDCALL = 6,
  ANDROID_KEY_ENTER = 66,
  ANDROID_KEY_ENVELOPE = 65,
  ANDROID_KEY_EQUALS = 70,
  ANDROID_KEY_ESCAPE = 111,
  ANDROID_KEY_EXPLORER = 64,

  ANDROID_KEY_F = 34,
  ANDROID_KEY_F1 = 131,
  ANDROID_KEY_F10 = 140,
  ANDROID_KEY_F11 = 141,
  ANDROID_KEY_F12 = 142,
  ANDROID_KEY_F2 = 132,
  ANDROID_KEY_F3 = 133,
  ANDROID_KEY_F4 = 134,
  ANDROID_KEY_F5 = 135,
  ANDROID_KEY_F6 = 136,
  ANDROID_KEY_F7 = 137,
  ANDROID_KEY_F8 = 138,
  ANDROID_KEY_F9 = 139,
  ANDROID_KEY_FOCUS = 80,
  ANDROID_KEY_FORWARD = 125,
  ANDROID_KEY_FORWARD_DEL = 112,
  ANDROID_KEY_FUNCTION = 119,

  ANDROID_KEY_G = 35,
  ANDROID_KEY_GRAVE = 68,
  ANDROID_KEY_GUIDE = 172,

  ANDROID_KEY_H = 36,
  ANDROID_KEY_HEADSETHOOK = 79,
  ANDROID_KEY_HENKAN = 214,
  ANDROID_KEY_HOME = 3,

  ANDROID_KEY_I = 37,
  ANDROID_KEY_INFO = 165,
  ANDROID_KEY_INSERT = 124,

  ANDROID_KEY_J = 38,

  ANDROID_KEY_K = 39,
  ANDROID_KEY_KANA = 218,
  ANDROID_KEY_KATAKANA_HIRAGANA = 215,

  ANDROID_KEY_L = 40,
  ANDROID_KEY_LANGUAGE_SWITCH = 204,
  ANDROID_KEY_LEFT_BRACKET = 71,

  ANDROID_KEY_M = 41,
  ANDROID_KEY_MANNER_MODE = 205,
  ANDROID_KEY_MEDIA_CLOSE = 128,
  ANDROID_KEY_MEDIA_EJECT = 129,
  ANDROID_KEY_MEDIA_FAST_FORWARD = 90,
  ANDROID_KEY_MEDIA_NEXT = 87,
  ANDROID_KEY_MEDIA_PAUSE = 127,
  ANDROID_KEY_MEDIA_PLAY = 126,
  ANDROID_KEY_MEDIA_PLAY_PAUSE = 85,
  ANDROID_KEY_MEDIA_PREVIOUS = 88,
  ANDROID_KEY_MEDIA_RECORD = 130,
  ANDROID_KEY_MEDIA_REWIND = 89,
  ANDROID_KEY_MEDIA_STOP = 86,
  ANDROID_KEY_MENU = 82,
  ANDROID_KEY_META_LEFT = 117,
  ANDROID_KEY_META_RIGHT = 118,
  ANDROID_KEY_MINUS = 69,
  ANDROID_KEY_MOVE_END = 123,
  ANDROID_KEY_MOVE_HOME = 122,
  ANDROID_KEY_MUHENKAN = 213,
  ANDROID_KEY_MUSIC = 209,
  ANDROID_KEY_MUTE = 91,

  ANDROID_KEY_N = 42,
  ANDROID_KEY_NOTIFICATION = 83,
  ANDROID_KEY_NUM = 78,
  ANDROID_KEY_NUMPAD_0 = 144,
  ANDROID_KEY_NUMPAD_1 = 145,
  ANDROID_KEY_NUMPAD_2 = 146,
  ANDROID_KEY_NUMPAD_3 = 147,
  ANDROID_KEY_NUMPAD_4 = 148,
  ANDROID_KEY_NUMPAD_5 = 149,
  ANDROID_KEY_NUMPAD_6 = 150,
  ANDROID_KEY_NUMPAD_7 = 151,
  ANDROID_KEY_NUMPAD_8 = 152,
  ANDROID_KEY_NUMPAD_9 = 153,
  ANDROID_KEY_NUMPAD_ADD = 157,
  ANDROID_KEY_NUMPAD_COMMA = 159,
  ANDROID_KEY_NUMPAD_DIVIDE = 154,
  ANDROID_KEY_NUMPAD_DOT = 158,
  ANDROID_KEY_NUMPAD_ENTER = 160,
  ANDROID_KEY_NUMPAD_EQUALS = 161,
  ANDROID_KEY_NUMPAD_LEFT_PAREN = 162,
  ANDROID_KEY_NUMPAD_MULTIPLY = 155,
  ANDROID_KEY_NUMPAD_RIGHT_PAREN = 163,
  ANDROID_KEY_NUMPAD_SUBTRACT = 156,
  ANDROID_KEY_NUM_LOCK = 143,

  ANDROID_KEY_O = 43,

  ANDROID_KEY_P = 44,
  ANDROID_KEY_PAGE_DOWN = 93,
  ANDROID_KEY_PAGE_UP = 92,
  ANDROID_KEY_PERIOD = 56,
  ANDROID_KEY_PICTSYMBOLS = 94,
  ANDROID_KEY_PLUS = 81,
  ANDROID_KEY_POUND = 18,
  ANDROID_KEY_POWER = 26,
  ANDROID_KEY_PROG_BLUE = 186,
  ANDROID_KEY_PROG_GREEN = 184,
  ANDROID_KEY_PROG_RED = 183,
  ANDROID_KEY_PROG_YELLOW = 185,

  ANDROID_KEY_Q = 45,

  ANDROID_KEY_R = 46,
  ANDROID_KEY_RIGHT_BRACKET = 72,
  ANDROID_KEY_RO = 217,

  ANDROID_KEY_S = 47,
  ANDROID_KEY_SCROLL_LOCK = 116,
  ANDROID_KEY_SEARCH = 84,
  ANDROID_KEY_SEMICOLON = 74,
  ANDROID_KEY_SETTINGS = 176,
  ANDROID_KEY_SHIFT_LEFT = 59,
  ANDROID_KEY_SHIFT_RIGHT = 60,
  ANDROID_KEY_SLASH = 76,
  ANDROID_KEY_SOFT_LEFT = 1,
  ANDROID_KEY_SOFT_RIGHT = 2,
  ANDROID_KEY_SPACE = 62,
  ANDROID_KEY_STAR = 17,
  ANDROID_KEY_STB_INPUT = 180,
  ANDROID_KEY_STB_POWER = 179,
  ANDROID_KEY_SWITCH_CHARSET = 95,
  ANDROID_KEY_SYM = 63,
  ANDROID_KEY_SYSRQ = 120,

  ANDROID_KEY_T = 48,
  ANDROID_KEY_TAB = 61,
  ANDROID_KEY_TV = 170,
  ANDROID_KEY_TV_INPUT = 178,
  ANDROID_KEY_TV_POWER = 177,

  ANDROID_KEY_U = 49,
  ANDROID_KEY_UNKNOWN = 0,

  ANDROID_KEY_V = 50,
  ANDROID_KEY_VOLUME_DOWN = 25,
  ANDROID_KEY_VOLUME_MUTE = 164,
  ANDROID_KEY_VOLUME_UP = 24,

  ANDROID_KEY_W = 51,
  ANDROID_KEY_WINDOW = 171,

  ANDROID_KEY_X = 52,

  ANDROID_KEY_Y = 53,
  ANDROID_KEY_YEN = 216,

  ANDROID_KEY_Z = 54,
  ANDROID_KEY_ZENKAKU_HANKAKU = 211,
  ANDROID_KEY_ZOOM_IN = 168,
  ANDROID_KEY_ZOOM_OUT = 169,

  B2G_KEY_CHARACTERS = 0X100,
  B2G_KEY_CHORDS     = 0X200,

  B2G_KEY_DOT1 = 769,
  B2G_KEY_DOT2 = 770,
  B2G_KEY_DOT3 = 771,
  B2G_KEY_DOT4 = 772,
  B2G_KEY_DOT5 = 773,
  B2G_KEY_DOT6 = 774,
  B2G_KEY_DOT7 = 775,
  B2G_KEY_DOT8 = 776,
  B2G_KEY_DOT9 = 777,

  B2G_KEY_CURSOR0  = 778,
  B2G_KEY_CURSOR1  = 779,
  B2G_KEY_CURSOR2  = 780,
  B2G_KEY_CURSOR3  = 781,
  B2G_KEY_CURSOR4  = 782,
  B2G_KEY_CURSOR5  = 783,
  B2G_KEY_CURSOR6  = 784,
  B2G_KEY_CURSOR7  = 785,
  B2G_KEY_CURSOR8  = 786,
  B2G_KEY_CURSOR9  = 787,
  B2G_KEY_CURSOR10 = 788,
  B2G_KEY_CURSOR11 = 789,
  B2G_KEY_CURSOR12 = 790,
  B2G_KEY_CURSOR13 = 791,
  B2G_KEY_CURSOR14 = 792,
  B2G_KEY_CURSOR15 = 793,
  B2G_KEY_CURSOR16 = 794,
  B2G_KEY_CURSOR17 = 795,
  B2G_KEY_CURSOR18 = 796,
  B2G_KEY_CURSOR19 = 797,
  B2G_KEY_CURSOR20 = 798,
  B2G_KEY_CURSOR21 = 799,
  B2G_KEY_CURSOR22 = 800,
  B2G_KEY_CURSOR23 = 801,
  B2G_KEY_CURSOR24 = 802,
  B2G_KEY_CURSOR25 = 803,
  B2G_KEY_CURSOR26 = 804,
  B2G_KEY_CURSOR27 = 805,
  B2G_KEY_CURSOR28 = 806,
  B2G_KEY_CURSOR29 = 807,
  B2G_KEY_CURSOR30 = 808,
  B2G_KEY_CURSOR31 = 809,
  B2G_KEY_CURSOR32 = 810,
  B2G_KEY_CURSOR33 = 811,
  B2G_KEY_CURSOR34 = 812,
  B2G_KEY_CURSOR35 = 813,
  B2G_KEY_CURSOR36 = 814,
  B2G_KEY_CURSOR37 = 815,
  B2G_KEY_CURSOR38 = 816,
  B2G_KEY_CURSOR39 = 817,

  B2G_KEY_BACK    = 818,
  B2G_KEY_FORWARD = 819,
} AndroidKeyCode;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KBD_ANDROID */
