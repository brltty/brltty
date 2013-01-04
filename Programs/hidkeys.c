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

#include "prologue.h"

#include <string.h>

#include "hidkeys.h"
#include "bitmask.h"
#include "brldefs.h"
#include "brl.h"

typedef struct {
  uint16_t xtCode;
  uint16_t atCode;
} HidKeyEntry;

static const HidKeyEntry hidKeyTable[] = {
  /* aA */
  [0X04] = {
    .xtCode = 0X001E,
    .atCode = 0X001C
  }
  ,
  /* bB */
  [0X05] = {
    .xtCode = 0X0030,
    .atCode = 0X0032
  }
  ,
  /* cC */
  [0X06] = {
    .xtCode = 0X002E,
    .atCode = 0X0021
  }
  ,
  /* dD */
  [0X07] = {
    .xtCode = 0X0020,
    .atCode = 0X0023
  }
  ,
  /* eE */
  [0X08] = {
    .xtCode = 0X0012,
    .atCode = 0X0024
  }
  ,
  /* fF */
  [0X09] = {
    .xtCode = 0X0021,
    .atCode = 0X002B
  }
  ,
  /* gG */
  [0X0A] = {
    .xtCode = 0X0022,
    .atCode = 0X0034
  }
  ,
  /* hH */
  [0X0B] = {
    .xtCode = 0X0023,
    .atCode = 0X0033
  }
  ,
  /* iI */
  [0X0C] = {
    .xtCode = 0X0017,
    .atCode = 0X0043
  }
  ,
  /* jJ */
  [0X0D] = {
    .xtCode = 0X0024,
    .atCode = 0X003B
  }
  ,
  /* kK */
  [0X0E] = {
    .xtCode = 0X0025,
    .atCode = 0X0042
  }
  ,
  /* lL */
  [0X0F] = {
    .xtCode = 0X0026,
    .atCode = 0X004B
  }
  ,
  /* mM */
  [0X10] = {
    .xtCode = 0X0032,
    .atCode = 0X003A
  }
  ,
  /* nN */
  [0X11] = {
    .xtCode = 0X0031,
    .atCode = 0X0031
  }
  ,
  /* oO */
  [0X12] = {
    .xtCode = 0X0018,
    .atCode = 0X0044
  }
  ,
  /* pP */
  [0X13] = {
    .xtCode = 0X0019,
    .atCode = 0X004D
  }
  ,
  /* qQ */
  [0X14] = {
    .xtCode = 0X0010,
    .atCode = 0X0015
  }
  ,
  /* rR */
  [0X15] = {
    .xtCode = 0X0013,
    .atCode = 0X002D
  }
  ,
  /* sS */
  [0X16] = {
    .xtCode = 0X001F,
    .atCode = 0X001B
  }
  ,
  /* tT */
  [0X17] = {
    .xtCode = 0X0014,
    .atCode = 0X002C
  }
  ,
  /* uU */
  [0X18] = {
    .xtCode = 0X0016,
    .atCode = 0X003C
  }
  ,
  /* vV */
  [0X19] = {
    .xtCode = 0X002F,
    .atCode = 0X002A
  }
  ,
  /* wW */
  [0X1A] = {
    .xtCode = 0X0011,
    .atCode = 0X001D
  }
  ,
  /* xX */
  [0X1B] = {
    .xtCode = 0X002D,
    .atCode = 0X0022
  }
  ,
  /* yY */
  [0X1C] = {
    .xtCode = 0X0015,
    .atCode = 0X0035
  }
  ,
  /* zZ */
  [0X1D] = {
    .xtCode = 0X002C,
    .atCode = 0X001A
  }
  ,
  /* 1! */
  [0X1E] = {
    .xtCode = 0X0002,
    .atCode = 0X0016
  }
  ,
  /* 2@ */
  [0X1F] = {
    .xtCode = 0X0003,
    .atCode = 0X001E
  }
  ,
  /* 3# */
  [0X20] = {
    .xtCode = 0X0004,
    .atCode = 0X0026
  }
  ,
  /* 4$ */
  [0X21] = {
    .xtCode = 0X0005,
    .atCode = 0X0025
  }
  ,
  /* 5% */
  [0X22] = {
    .xtCode = 0X0006,
    .atCode = 0X002E
  }
  ,
  /* 6^ */
  [0X23] = {
    .xtCode = 0X0007,
    .atCode = 0X0036
  }
  ,
  /* 7& */
  [0X24] = {
    .xtCode = 0X0008,
    .atCode = 0X003D
  }
  ,
  /* 8* */
  [0X25] = {
    .xtCode = 0X0009,
    .atCode = 0X003E
  }
  ,
  /* 9( */
  [0X26] = {
    .xtCode = 0X000A,
    .atCode = 0X0046
  }
  ,
  /* 0) */
  [0X27] = {
    .xtCode = 0X000B,
    .atCode = 0X0045
  }
  ,
  /* Return */
  [0X28] = {
    .xtCode = 0X001C,
    .atCode = 0X005A
  }
  ,
  /* Escape */
  [0X29] = {
    .xtCode = 0X0001,
    .atCode = 0X0076
  }
  ,
  /* Backspace */
  [0X2A] = {
    .xtCode = 0X000E,
    .atCode = 0X0066
  }
  ,
  /* Tab */
  [0X2B] = {
    .xtCode = 0X000F,
    .atCode = 0X000D
  }
  ,
  /* Space */
  [0X2C] = {
    .xtCode = 0X0039,
    .atCode = 0X0029
  }
  ,
  /* -_ */
  [0X2D] = {
    .xtCode = 0X000C,
    .atCode = 0X004E
  }
  ,
  /* =+ */
  [0X2E] = {
    .xtCode = 0X000D,
    .atCode = 0X0055
  }
  ,
  /* [{ */
  [0X2F] = {
    .xtCode = 0X001A,
    .atCode = 0X0054
  }
  ,
  /* ]} */
  [0X30] = {
    .xtCode = 0X001B,
    .atCode = 0X005B
  }
  ,
  /* \| */
  [0X31] = {
    .xtCode = 0X002B,
    .atCode = 0X005D
  }
  ,
  /* Europe 1 (Note 2) */
  [0X32] = {
    .xtCode = 0X002B,
    .atCode = 0X005D
  }
  ,
  /* ;: */
  [0X33] = {
    .xtCode = 0X0027,
    .atCode = 0X004C
  }
  ,
  /* '" */
  [0X34] = {
    .xtCode = 0X0028,
    .atCode = 0X0052
  }
  ,
  /* `~ */
  [0X35] = {
    .xtCode = 0X0029,
    .atCode = 0X000E
  }
  ,
  /* ,< */
  [0X36] = {
    .xtCode = 0X0033,
    .atCode = 0X0041
  }
  ,
  /* .> */
  [0X37] = {
    .xtCode = 0X0034,
    .atCode = 0X0049
  }
  ,
  /* /? */
  [0X38] = {
    .xtCode = 0X0035,
    .atCode = 0X004A
  }
  ,
  /* Caps Lock */
  [0X39] = {
    .xtCode = 0X003A,
    .atCode = 0X0058
  }
  ,
  /* F1 */
  [0X3A] = {
    .xtCode = 0X003B,
    .atCode = 0X0005
  }
  ,
  /* F2 */
  [0X3B] = {
    .xtCode = 0X003C,
    .atCode = 0X0006
  }
  ,
  /* F3 */
  [0X3C] = {
    .xtCode = 0X003D,
    .atCode = 0X0004
  }
  ,
  /* F4 */
  [0X3D] = {
    .xtCode = 0X003E,
    .atCode = 0X000C
  }
  ,
  /* F5 */
  [0X3E] = {
    .xtCode = 0X003F,
    .atCode = 0X0003
  }
  ,
  /* F6 */
  [0X3F] = {
    .xtCode = 0X0040,
    .atCode = 0X000B
  }
  ,
  /* F7 */
  [0X40] = {
    .xtCode = 0X0041,
    .atCode = 0X0083
  }
  ,
  /* F8 */
  [0X41] = {
    .xtCode = 0X0042,
    .atCode = 0X000A
  }
  ,
  /* F9 */
  [0X42] = {
    .xtCode = 0X0043,
    .atCode = 0X0001
  }
  ,
  /* F10 */
  [0X43] = {
    .xtCode = 0X0044,
    .atCode = 0X0009
  }
  ,
  /* F11 */
  [0X44] = {
    .xtCode = 0X0057,
    .atCode = 0X0078
  }
  ,
  /* F12 */
  [0X45] = {
    .xtCode = 0X0058,
    .atCode = 0X0007
  }
  ,
  /* Print Screen (Note 1) */
  [0X46] = {
    .xtCode = 0XE037,
    .atCode = 0XE07C
  }
  ,
  /* Scroll Lock */
  [0X47] = {
    .xtCode = 0X0046,
    .atCode = 0X007E
  }
  ,

  /* Insert (Note 1) */
  [0X49] = {
    .xtCode = 0XE052,
    .atCode = 0XE070
  }
  ,
  /* Home (Note 1) */
  [0X4A] = {
    .xtCode = 0XE047,
    .atCode = 0XE06C
  }
  ,
  /* Page Up (Note 1) */
  [0X4B] = {
    .xtCode = 0XE049,
    .atCode = 0XE07D
  }
  ,
  /* Delete (Note 1) */
  [0X4C] = {
    .xtCode = 0XE053,
    .atCode = 0XE071
  }
  ,
  /* End (Note 1) */
  [0X4D] = {
    .xtCode = 0XE04F,
    .atCode = 0XE069
  }
  ,
  /* Page Down (Note 1) */
  [0X4E] = {
    .xtCode = 0XE051,
    .atCode = 0XE07A
  }
  ,
  /* Right Arrow (Note 1) */
  [0X4F] = {
    .xtCode = 0XE04D,
    .atCode = 0XE074
  }
  ,
  /* Left Arrow (Note 1) */
  [0X50] = {
    .xtCode = 0XE04B,
    .atCode = 0XE06B
  }
  ,
  /* Down Arrow (Note 1) */
  [0X51] = {
    .xtCode = 0XE050,
    .atCode = 0XE072
  }
  ,
  /* Up Arrow (Note 1) */
  [0X52] = {
    .xtCode = 0XE048,
    .atCode = 0XE075
  }
  ,
  /* Num Lock */
  [0X53] = {
    .xtCode = 0X0045,
    .atCode = 0X0077
  }
  ,
  /* Keypad / (Note 1) */
  [0X54] = {
    .xtCode = 0XE035,
    .atCode = 0XE04A
  }
  ,
  /* Keypad * */
  [0X55] = {
    .xtCode = 0X0037,
    .atCode = 0X007C
  }
  ,
  /* Keypad - */
  [0X56] = {
    .xtCode = 0X004A,
    .atCode = 0X007B
  }
  ,
  /* Keypad + */
  [0X57] = {
    .xtCode = 0X004E,
    .atCode = 0X0079
  }
  ,
  /* Keypad Enter */
  [0X58] = {
    .xtCode = 0XE01C,
    .atCode = 0XE05A
  }
  ,
  /* Keypad 1 End */
  [0X59] = {
    .xtCode = 0X004F,
    .atCode = 0X0069
  }
  ,
  /* Keypad 2 Down */
  [0X5A] = {
    .xtCode = 0X0050,
    .atCode = 0X0072
  }
  ,
  /* Keypad 3 PageDn */
  [0X5B] = {
    .xtCode = 0X0051,
    .atCode = 0X007A
  }
  ,
  /* Keypad 4 Left */
  [0X5C] = {
    .xtCode = 0X004B,
    .atCode = 0X006B
  }
  ,
  /* Keypad 5 */
  [0X5D] = {
    .xtCode = 0X004C,
    .atCode = 0X0073
  }
  ,
  /* Keypad 6 Right */
  [0X5E] = {
    .xtCode = 0X004D,
    .atCode = 0X0074
  }
  ,
  /* Keypad 7 Home */
  [0X5F] = {
    .xtCode = 0X0047,
    .atCode = 0X006C
  }
  ,
  /* Keypad 8 Up */
  [0X60] = {
    .xtCode = 0X0048,
    .atCode = 0X0075
  }
  ,
  /* Keypad 9 PageUp */
  [0X61] = {
    .xtCode = 0X0049,
    .atCode = 0X007D
  }
  ,
  /* Keypad 0 Insert */
  [0X62] = {
    .xtCode = 0X0052,
    .atCode = 0X0070
  }
  ,
  /* Keypad . Delete */
  [0X63] = {
    .xtCode = 0X0053,
    .atCode = 0X0071
  }
  ,
  /* Europe 2 (Note 2) */
  [0X64] = {
    .xtCode = 0X0056,
    .atCode = 0X0061
  }
  ,
  /* App */
  [0X65] = {
    .xtCode = 0XE05D,
    .atCode = 0XE02F
  }
  ,
  /* Keyboard Power */
  [0X66] = {
    .xtCode = 0XE05E,
    .atCode = 0XE037
  }
  ,
  /* Keypad = */
  [0X67] = {
    .xtCode = 0X0059,
    .atCode = 0X000F
  }
  ,
  /* F13 */
  [0X68] = {
    .xtCode = 0X0064,
    .atCode = 0X0008
  }
  ,
  /* F14 */
  [0X69] = {
    .xtCode = 0X0065,
    .atCode = 0X0010
  }
  ,
  /* F15 */
  [0X6A] = {
    .xtCode = 0X0066,
    .atCode = 0X0018
  }
  ,
  /* F16 */
  [0X6B] = {
    .xtCode = 0X0067,
    .atCode = 0X0020
  }
  ,
  /* F17 */
  [0X6C] = {
    .xtCode = 0X0068,
    .atCode = 0X0028
  }
  ,
  /* F18 */
  [0X6D] = {
    .xtCode = 0X0069,
    .atCode = 0X0030
  }
  ,
  /* F19 */
  [0X6E] = {
    .xtCode = 0X006A,
    .atCode = 0X0038
  }
  ,
  /* F20 */
  [0X6F] = {
    .xtCode = 0X006B,
    .atCode = 0X0040
  }
  ,
  /* F21 */
  [0X70] = {
    .xtCode = 0X006C,
    .atCode = 0X0048
  }
  ,
  /* F22 */
  [0X71] = {
    .xtCode = 0X006D,
    .atCode = 0X0050
  }
  ,
  /* F23 */
  [0X72] = {
    .xtCode = 0X006E,
    .atCode = 0X0057
  }
  ,
  /* F24 */
  [0X73] = {
    .xtCode = 0X0076,
    .atCode = 0X005F
  }
  ,
  /* Keyboard Execute */
  [0X74] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Help */
  [0X75] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Menu */
  [0X76] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Select */
  [0X77] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Stop */
  [0X78] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Again */
  [0X79] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Undo */
  [0X7A] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Cut */
  [0X7B] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Copy */
  [0X7C] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Paste */
  [0X7D] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Find */
  [0X7E] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Mute */
  [0X7F] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Volume Up */
  [0X80] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Volume Dn */
  [0X81] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Caps Lock */
  [0X82] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Num Lock */
  [0X83] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Scroll Lock */
  [0X84] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keypad , (Brazilian Keypad .) */
  [0X85] = {
    .xtCode = 0X007E,
    .atCode = 0X006D
  }
  ,
  /* Keyboard Equal Sign */
  [0X86] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Int'l 1 (Ro) */
  [0X87] = {
    .xtCode = 0X0073,
    .atCode = 0X0051
  }
  ,
  /* Keyboard Intl'2 (Katakana/Hiragana) */
  [0X88] = {
    .xtCode = 0X0070,
    .atCode = 0X0013
  }
  ,
  /* Keyboard Int'l 3 (Yen) */
  [0X89] = {
    .xtCode = 0X007D,
    .atCode = 0X006A
  }
  ,
  /* Keyboard Int'l 4 (Henkan) */
  [0X8A] = {
    .xtCode = 0X0079,
    .atCode = 0X0064
  }
  ,
  /* Keyboard Int'l 5 (Muhenkan) */
  [0X8B] = {
    .xtCode = 0X007B,
    .atCode = 0X0067
  }
  ,
  /* Keyboard Int'l 6 (PC9800 Keypad ,) */
  [0X8C] = {
    .xtCode = 0X005C,
    .atCode = 0X0027
  }
  ,
  /* Keyboard Int'l 7 */
  [0X8D] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Int'l 8 */
  [0X8E] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Int'l 9 */
  [0X8F] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Lang 1 (Hanguel/English) */
  [0X90] = {
    .xtCode = 0X00F2,
    .atCode = 0X00F2
  }
  ,
  /* Keyboard Lang 2 (Hanja) */
  [0X91] = {
    .xtCode = 0X00F1,
    .atCode = 0X00F1
  }
  ,
  /* Keyboard Lang 3 (Katakana) */
  [0X92] = {
    .xtCode = 0X0078,
    .atCode = 0X0063
  }
  ,
  /* Keyboard Lang 4 (Hiragana) */
  [0X93] = {
    .xtCode = 0X0077,
    .atCode = 0X0062
  }
  ,
  /* Keyboard Lang 5 (Zenkaku/Hankaku) */
  [0X94] = {
    .xtCode = 0X0076,
    .atCode = 0X005F
  }
  ,
  /* Keyboard Lang 6 */
  [0X95] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Lang 7 */
  [0X96] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Lang 8 */
  [0X97] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Lang 9 */
  [0X98] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Alternate Erase */
  [0X99] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard SysReq/Attention */
  [0X9A] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Cancel */
  [0X9B] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Clear */
  [0X9C] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Prior */
  [0X9D] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Return */
  [0X9E] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Separator */
  [0X9F] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Out */
  [0XA0] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Oper */
  [0XA1] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard Clear/Again */
  [0XA2] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard CrSel/Props */
  [0XA3] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,
  /* Keyboard ExSel */
  [0XA4] = {
    .xtCode = 0X0000,
    .atCode = 0X0000
  }
  ,

  /* Left Control */
  [0XE0] = {
    .xtCode = 0X001D,
    .atCode = 0X0014
  }
  ,
  /* Left Shift */
  [0XE1] = {
    .xtCode = 0X002A,
    .atCode = 0X0012
  }
  ,
  /* Left Alt */
  [0XE2] = {
    .xtCode = 0X0038,
    .atCode = 0X0011
  }
  ,
  /* Left GUI */
  [0XE3] = {
    .xtCode = 0XE05B,
    .atCode = 0XE01F
  }
  ,
  /* Right Control */
  [0XE4] = {
    .xtCode = 0XE01D,
    .atCode = 0XE014
  }
  ,
  /* Right Shift */
  [0XE5] = {
    .xtCode = 0X0036,
    .atCode = 0X0059
  }
  ,
  /* Right Alt */
  [0XE6] = {
    .xtCode = 0XE038,
    .atCode = 0XE011
  }
  ,
  /* Right GUI */
  [0XE7] = {
    .xtCode = 0XE05C,
    .atCode = 0XE027
  }
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
