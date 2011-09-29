/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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
  uint16_t set1;
  uint16_t set2;
} HidKeyEntry;

static const HidKeyEntry hidKeyTable[] = {
  [0X04] = {.set1=0X001E, .set2=0X001C} /* aA */,
  [0X05] = {.set1=0X0030, .set2=0X0032} /* bB */,
  [0X06] = {.set1=0X002E, .set2=0X0021} /* cC */,
  [0X07] = {.set1=0X0020, .set2=0X0023} /* dD */,
  [0X08] = {.set1=0X0012, .set2=0X0024} /* eE */,
  [0X09] = {.set1=0X0021, .set2=0X002B} /* fF */,
  [0X0A] = {.set1=0X0022, .set2=0X0034} /* gG */,
  [0X0B] = {.set1=0X0023, .set2=0X0033} /* hH */,
  [0X0C] = {.set1=0X0017, .set2=0X0043} /* iI */,
  [0X0D] = {.set1=0X0024, .set2=0X003B} /* jJ */,
  [0X0E] = {.set1=0X0025, .set2=0X0042} /* kK */,
  [0X0F] = {.set1=0X0026, .set2=0X004B} /* lL */,
  [0X10] = {.set1=0X0032, .set2=0X003A} /* mM */,
  [0X11] = {.set1=0X0031, .set2=0X0031} /* nN */,
  [0X12] = {.set1=0X0018, .set2=0X0044} /* oO */,
  [0X13] = {.set1=0X0019, .set2=0X004D} /* pP */,
  [0X14] = {.set1=0X0010, .set2=0X0015} /* qQ */,
  [0X15] = {.set1=0X0013, .set2=0X002D} /* rR */,
  [0X16] = {.set1=0X001F, .set2=0X001B} /* sS */,
  [0X17] = {.set1=0X0014, .set2=0X002C} /* tT */,
  [0X18] = {.set1=0X0016, .set2=0X003C} /* uU */,
  [0X19] = {.set1=0X002F, .set2=0X002A} /* vV */,
  [0X1A] = {.set1=0X0011, .set2=0X001D} /* wW */,
  [0X1B] = {.set1=0X002D, .set2=0X0022} /* xX */,
  [0X1C] = {.set1=0X0015, .set2=0X0035} /* yY */,
  [0X1D] = {.set1=0X002C, .set2=0X001A} /* zZ */,
  [0X1E] = {.set1=0X0002, .set2=0X0016} /* 1! */,
  [0X1F] = {.set1=0X0003, .set2=0X001E} /* 2@ */,
  [0X20] = {.set1=0X0004, .set2=0X0026} /* 3# */,
  [0X21] = {.set1=0X0005, .set2=0X0025} /* 4$ */,
  [0X22] = {.set1=0X0006, .set2=0X002E} /* 5% */,
  [0X23] = {.set1=0X0007, .set2=0X0036} /* 6^ */,
  [0X24] = {.set1=0X0008, .set2=0X003D} /* 7& */,
  [0X25] = {.set1=0X0009, .set2=0X003E} /* 8* */,
  [0X26] = {.set1=0X000A, .set2=0X0046} /* 9( */,
  [0X27] = {.set1=0X000B, .set2=0X0045} /* 0) */,
  [0X28] = {.set1=0X001C, .set2=0X005A} /* Return */,
  [0X29] = {.set1=0X0001, .set2=0X0076} /* Escape */,
  [0X2A] = {.set1=0X000E, .set2=0X0066} /* Backspace */,
  [0X2B] = {.set1=0X000F, .set2=0X000D} /* Tab */,
  [0X2C] = {.set1=0X0039, .set2=0X0029} /* Space */,
  [0X2D] = {.set1=0X000C, .set2=0X004E} /* -_ */,
  [0X2E] = {.set1=0X000D, .set2=0X0055} /* =+ */,
  [0X2F] = {.set1=0X001A, .set2=0X0054} /* [{ */,
  [0X30] = {.set1=0X001B, .set2=0X005B} /* ]} */,
  [0X31] = {.set1=0X002B, .set2=0X005D} /* \| */,
  [0X32] = {.set1=0X002B, .set2=0X005D} /* Europe 1 (Note 2) */,
  [0X33] = {.set1=0X0027, .set2=0X004C} /* ;: */,
  [0X34] = {.set1=0X0028, .set2=0X0052} /* '" */,
  [0X35] = {.set1=0X0029, .set2=0X000E} /* `~ */,
  [0X36] = {.set1=0X0033, .set2=0X0041} /* ,< */,
  [0X37] = {.set1=0X0034, .set2=0X0049} /* .> */,
  [0X38] = {.set1=0X0035, .set2=0X004A} /* /? */,
  [0X39] = {.set1=0X003A, .set2=0X0058} /* Caps Lock */,
  [0X3A] = {.set1=0X003B, .set2=0X0005} /* F1 */,
  [0X3B] = {.set1=0X003C, .set2=0X0006} /* F2 */,
  [0X3C] = {.set1=0X003D, .set2=0X0004} /* F3 */,
  [0X3D] = {.set1=0X003E, .set2=0X000C} /* F4 */,
  [0X3E] = {.set1=0X003F, .set2=0X0003} /* F5 */,
  [0X3F] = {.set1=0X0040, .set2=0X000B} /* F6 */,
  [0X40] = {.set1=0X0041, .set2=0X0083} /* F7 */,
  [0X41] = {.set1=0X0042, .set2=0X000A} /* F8 */,
  [0X42] = {.set1=0X0043, .set2=0X0001} /* F9 */,
  [0X43] = {.set1=0X0044, .set2=0X0009} /* F10 */,
  [0X44] = {.set1=0X0057, .set2=0X0078} /* F11 */,
  [0X45] = {.set1=0X0058, .set2=0X0007} /* F12 */,
  [0X46] = {.set1=0XE037, .set2=0XE07C} /* Print Screen (Note 1) */,
  [0X47] = {.set1=0X0046, .set2=0X007E} /* Scroll Lock */,

  [0X49] = {.set1=0XE052, .set2=0XE070} /* Insert (Note 1) */,
  [0X4A] = {.set1=0XE047, .set2=0XE06C} /* Home (Note 1) */,
  [0X4B] = {.set1=0XE049, .set2=0XE07D} /* Page Up (Note 1) */,
  [0X4C] = {.set1=0XE053, .set2=0XE071} /* Delete (Note 1) */,
  [0X4D] = {.set1=0XE04F, .set2=0XE069} /* End (Note 1) */,
  [0X4E] = {.set1=0XE051, .set2=0XE07A} /* Page Down (Note 1) */,
  [0X4F] = {.set1=0XE04D, .set2=0XE074} /* Right Arrow (Note 1) */,
  [0X50] = {.set1=0XE04B, .set2=0XE06B} /* Left Arrow (Note 1) */,
  [0X51] = {.set1=0XE050, .set2=0XE072} /* Down Arrow (Note 1) */,
  [0X52] = {.set1=0XE048, .set2=0XE075} /* Up Arrow (Note 1) */,
  [0X53] = {.set1=0X0045, .set2=0X0077} /* Num Lock */,
  [0X54] = {.set1=0XE035, .set2=0XE04A} /* Keypad / (Note 1) */,
  [0X55] = {.set1=0X0037, .set2=0X007C} /* Keypad * */,
  [0X56] = {.set1=0X004A, .set2=0X007B} /* Keypad - */,
  [0X57] = {.set1=0X004E, .set2=0X0079} /* Keypad + */,
  [0X58] = {.set1=0XE01C, .set2=0XE05A} /* Keypad Enter */,
  [0X59] = {.set1=0X004F, .set2=0X0069} /* Keypad 1 End */,
  [0X5A] = {.set1=0X0050, .set2=0X0072} /* Keypad 2 Down */,
  [0X5B] = {.set1=0X0051, .set2=0X007A} /* Keypad 3 PageDn */,
  [0X5C] = {.set1=0X004B, .set2=0X006B} /* Keypad 4 Left */,
  [0X5D] = {.set1=0X004C, .set2=0X0073} /* Keypad 5 */,
  [0X5E] = {.set1=0X004D, .set2=0X0074} /* Keypad 6 Right */,
  [0X5F] = {.set1=0X0047, .set2=0X006C} /* Keypad 7 Home */,
  [0X60] = {.set1=0X0048, .set2=0X0075} /* Keypad 8 Up */,
  [0X61] = {.set1=0X0049, .set2=0X007D} /* Keypad 9 PageUp */,
  [0X62] = {.set1=0X0052, .set2=0X0070} /* Keypad 0 Insert */,
  [0X63] = {.set1=0X0053, .set2=0X0071} /* Keypad . Delete */,
  [0X64] = {.set1=0X0056, .set2=0X0061} /* Europe 2 (Note 2) */,
  [0X65] = {.set1=0XE05D, .set2=0XE02F} /* App */,
  [0X66] = {.set1=0XE05E, .set2=0XE037} /* Keyboard Power */,
  [0X67] = {.set1=0X0059, .set2=0X000F} /* Keypad = */,
  [0X68] = {.set1=0X0064, .set2=0X0008} /* F13 */,
  [0X69] = {.set1=0X0065, .set2=0X0010} /* F14 */,
  [0X6A] = {.set1=0X0066, .set2=0X0018} /* F15 */,
  [0X6B] = {.set1=0X0067, .set2=0X0020} /* F16 */,
  [0X6C] = {.set1=0X0068, .set2=0X0028} /* F17 */,
  [0X6D] = {.set1=0X0069, .set2=0X0030} /* F18 */,
  [0X6E] = {.set1=0X006A, .set2=0X0038} /* F19 */,
  [0X6F] = {.set1=0X006B, .set2=0X0040} /* F20 */,
  [0X70] = {.set1=0X006C, .set2=0X0048} /* F21 */,
  [0X71] = {.set1=0X006D, .set2=0X0050} /* F22 */,
  [0X72] = {.set1=0X006E, .set2=0X0057} /* F23 */,
  [0X73] = {.set1=0X0076, .set2=0X005F} /* F24 */,
  [0X74] = {.set1=0X0000, .set2=0X0000} /* Keyboard Execute */,
  [0X75] = {.set1=0X0000, .set2=0X0000} /* Keyboard Help */,
  [0X76] = {.set1=0X0000, .set2=0X0000} /* Keyboard Menu */,
  [0X77] = {.set1=0X0000, .set2=0X0000} /* Keyboard Select */,
  [0X78] = {.set1=0X0000, .set2=0X0000} /* Keyboard Stop */,
  [0X79] = {.set1=0X0000, .set2=0X0000} /* Keyboard Again */,
  [0X7A] = {.set1=0X0000, .set2=0X0000} /* Keyboard Undo */,
  [0X7B] = {.set1=0X0000, .set2=0X0000} /* Keyboard Cut */,
  [0X7C] = {.set1=0X0000, .set2=0X0000} /* Keyboard Copy */,
  [0X7D] = {.set1=0X0000, .set2=0X0000} /* Keyboard Paste */,
  [0X7E] = {.set1=0X0000, .set2=0X0000} /* Keyboard Find */,
  [0X7F] = {.set1=0X0000, .set2=0X0000} /* Keyboard Mute */,
  [0X80] = {.set1=0X0000, .set2=0X0000} /* Keyboard Volume Up */,
  [0X81] = {.set1=0X0000, .set2=0X0000} /* Keyboard Volume Dn */,
  [0X82] = {.set1=0X0000, .set2=0X0000} /* Caps Lock */,
  [0X83] = {.set1=0X0000, .set2=0X0000} /* Num Lock */,
  [0X84] = {.set1=0X0000, .set2=0X0000} /* Scroll Lock */,
  [0X85] = {.set1=0X007E, .set2=0X006D} /* Keypad , (Brazilian Keypad .) */,
  [0X86] = {.set1=0X0000, .set2=0X0000} /* Keyboard Equal Sign */,
  [0X87] = {.set1=0X0073, .set2=0X0051} /* Keyboard Int'l 1 (Ro) */,
  [0X88] = {.set1=0X0070, .set2=0X0013} /* Keyboard Intl'2 (Katakana/Hiragana) */,
  [0X89] = {.set1=0X007D, .set2=0X006A} /* Keyboard Int'l 3 (Yen) */,
  [0X8A] = {.set1=0X0079, .set2=0X0064} /* Keyboard Int'l 4 (Henkan) */,
  [0X8B] = {.set1=0X007B, .set2=0X0067} /* Keyboard Int'l 5 (Muhenkan) */,
  [0X8C] = {.set1=0X005C, .set2=0X0027} /* Keyboard Int'l 6 (PC9800 Keypad ,) */,
  [0X8D] = {.set1=0X0000, .set2=0X0000} /* Keyboard Int'l 7 */,
  [0X8E] = {.set1=0X0000, .set2=0X0000} /* Keyboard Int'l 8 */,
  [0X8F] = {.set1=0X0000, .set2=0X0000} /* Keyboard Int'l 9 */,
  [0X90] = {.set1=0X00F2, .set2=0X00F2} /* Keyboard Lang 1 (Hanguel/English) */,
  [0X91] = {.set1=0X00F1, .set2=0X00F1} /* Keyboard Lang 2 (Hanja) */,
  [0X92] = {.set1=0X0078, .set2=0X0063} /* Keyboard Lang 3 (Katakana) */,
  [0X93] = {.set1=0X0077, .set2=0X0062} /* Keyboard Lang 4 (Hiragana) */,
  [0X94] = {.set1=0X0076, .set2=0X005F} /* Keyboard Lang 5 (Zenkaku/Hankaku) */,
  [0X95] = {.set1=0X0000, .set2=0X0000} /* Keyboard Lang 6 */,
  [0X96] = {.set1=0X0000, .set2=0X0000} /* Keyboard Lang 7 */,
  [0X97] = {.set1=0X0000, .set2=0X0000} /* Keyboard Lang 8 */,
  [0X98] = {.set1=0X0000, .set2=0X0000} /* Keyboard Lang 9 */,
  [0X99] = {.set1=0X0000, .set2=0X0000} /* Keyboard Alternate Erase */,
  [0X9A] = {.set1=0X0000, .set2=0X0000} /* Keyboard SysReq/Attention */,
  [0X9B] = {.set1=0X0000, .set2=0X0000} /* Keyboard Cancel */,
  [0X9C] = {.set1=0X0000, .set2=0X0000} /* Keyboard Clear */,
  [0X9D] = {.set1=0X0000, .set2=0X0000} /* Keyboard Prior */,
  [0X9E] = {.set1=0X0000, .set2=0X0000} /* Keyboard Return */,
  [0X9F] = {.set1=0X0000, .set2=0X0000} /* Keyboard Separator */,
  [0XA0] = {.set1=0X0000, .set2=0X0000} /* Keyboard Out */,
  [0XA1] = {.set1=0X0000, .set2=0X0000} /* Keyboard Oper */,
  [0XA2] = {.set1=0X0000, .set2=0X0000} /* Keyboard Clear/Again */,
  [0XA3] = {.set1=0X0000, .set2=0X0000} /* Keyboard CrSel/Props */,
  [0XA4] = {.set1=0X0000, .set2=0X0000} /* Keyboard ExSel */,

  [0XE0] = {.set1=0X001D, .set2=0X0014} /* Left Control */,
  [0XE1] = {.set1=0X002A, .set2=0X0012} /* Left Shift */,
  [0XE2] = {.set1=0X0038, .set2=0X0011} /* Left Alt */,
  [0XE3] = {.set1=0XE05B, .set2=0XE01F} /* Left GUI */,
  [0XE4] = {.set1=0XE01D, .set2=0XE014} /* Right Control */,
  [0XE5] = {.set1=0X0036, .set2=0X0059} /* Right Shift */,
  [0XE6] = {.set1=0XE038, .set2=0XE011} /* Right Alt */,
  [0XE7] = {.set1=0XE05C, .set2=0XE027} /* Right GUI */,
};

static int
enqueueScanCode (uint8_t code) {
  return enqueueCommand(BRL_BLK_PASSXT | code);
}

static int
enqueueHidKeyEvent (unsigned char key, int press) {
  if (key < ARRAY_COUNT(hidKeyTable)) {
    uint16_t code = hidKeyTable[key].set1;

    if (code) {
      {
        uint8_t escape = (code >> 8) & 0XFF;

        if (escape)
          if (!enqueueScanCode(escape))
            return 0;
      }

      code &= 0XFF;
      if (!press) code |= 0X80;
      if (!enqueueScanCode(code)) return 0;
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
    if (!key) break;

    BITMASK_SET(pressedKeys, key);
  }

  for (index=0; index<oldCount; index+=1) {
    unsigned char key = oldKeys[index];
    if (!key) break;

    if (BITMASK_TEST(pressedKeys, key)) {
      BITMASK_CLEAR(pressedKeys, key);
    } else {
      enqueueHidKeyEvent(key, 0);
    }
  }

  for (index=0; index<newCount; index+=1) {
    unsigned char key = newKeys[index];
    if (!key) break;

    if (BITMASK_TEST(pressedKeys, key)) {
      enqueueHidKeyEvent(key, 1);
    }
  }

  *oldPacket = *newPacket;
}
