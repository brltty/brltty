/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>

#include "at2.h"
#include "brldefs.h"

typedef enum {
  AT2_RELEASE,
  AT2_SCROLL_LOCK,
  AT2_NUMBER_LOCK,
  AT2_NUMBER_SHIFT,
  AT2_CAPS_LOCK,
  AT2_LEFT_SHIFT,
  AT2_RIGHT_SHIFT,
  AT2_LEFT_CONTROL,
  AT2_RIGHT_CONTROL,
  AT2_LEFT_ALT,
  AT2_RIGHT_ALT,
  AT2_LEFT_WINDOWS,
  AT2_RIGHT_WINDOWS,
  AT2_MENU
} At2Modifier;

typedef struct {
  int command;
  int alternate;
} At2KeyEntry;
typedef At2KeyEntry At2KeyTable[0X100];

static const At2KeyTable at2KeysOriginal = {
  [0X76] = {VAL_PASSKEY+VPK_ESCAPE},
  [0X05] = {VAL_PASSKEY+VPK_FUNCTION+0},
  [0X06] = {VAL_PASSKEY+VPK_FUNCTION+1},
  [0X04] = {VAL_PASSKEY+VPK_FUNCTION+2},
  [0X0C] = {VAL_PASSKEY+VPK_FUNCTION+3},
  [0X03] = {VAL_PASSKEY+VPK_FUNCTION+4},
  [0X0B] = {VAL_PASSKEY+VPK_FUNCTION+5},
  [0X83] = {VAL_PASSKEY+VPK_FUNCTION+6},
  [0X0A] = {VAL_PASSKEY+VPK_FUNCTION+7},
  [0X01] = {VAL_PASSKEY+VPK_FUNCTION+8},
  [0X09] = {VAL_PASSKEY+VPK_FUNCTION+9},
  [0X78] = {VAL_PASSKEY+VPK_FUNCTION+10},
  [0X07] = {VAL_PASSKEY+VPK_FUNCTION+11},
  [0X7E] = {AT2_SCROLL_LOCK},
  [0X0E] = {VAL_PASSCHAR+'`', VAL_PASSCHAR+'~'},
  [0X16] = {VAL_PASSCHAR+'1', VAL_PASSCHAR+'!'},
  [0X1E] = {VAL_PASSCHAR+'2', VAL_PASSCHAR+'@'},
  [0X26] = {VAL_PASSCHAR+'3', VAL_PASSCHAR+'#'},
  [0X25] = {VAL_PASSCHAR+'4', VAL_PASSCHAR+'$'},
  [0X2E] = {VAL_PASSCHAR+'5', VAL_PASSCHAR+'%'},
  [0X36] = {VAL_PASSCHAR+'6', VAL_PASSCHAR+'^'},
  [0X3D] = {VAL_PASSCHAR+'7', VAL_PASSCHAR+'&'},
  [0X3E] = {VAL_PASSCHAR+'8', VAL_PASSCHAR+'*'},
  [0X46] = {VAL_PASSCHAR+'9', VAL_PASSCHAR+'('},
  [0X45] = {VAL_PASSCHAR+'0', VAL_PASSCHAR+')'},
  [0X4E] = {VAL_PASSCHAR+'-', VAL_PASSCHAR+'_'},
  [0X55] = {VAL_PASSCHAR+'=', VAL_PASSCHAR+'+'},
  [0X66] = {VAL_PASSKEY+VPK_BACKSPACE},
  [0X0D] = {VAL_PASSKEY+VPK_TAB},
  [0X15] = {VAL_PASSCHAR+'q', VAL_PASSCHAR+'Q'},
  [0X1D] = {VAL_PASSCHAR+'w', VAL_PASSCHAR+'W'},
  [0X24] = {VAL_PASSCHAR+'e', VAL_PASSCHAR+'E'},
  [0X2D] = {VAL_PASSCHAR+'r', VAL_PASSCHAR+'R'},
  [0X2C] = {VAL_PASSCHAR+'t', VAL_PASSCHAR+'T'},
  [0X35] = {VAL_PASSCHAR+'y', VAL_PASSCHAR+'Y'},
  [0X3C] = {VAL_PASSCHAR+'u', VAL_PASSCHAR+'U'},
  [0X43] = {VAL_PASSCHAR+'i', VAL_PASSCHAR+'I'},
  [0X44] = {VAL_PASSCHAR+'o', VAL_PASSCHAR+'O'},
  [0X4D] = {VAL_PASSCHAR+'p', VAL_PASSCHAR+'P'},
  [0X54] = {VAL_PASSCHAR+'[', VAL_PASSCHAR+'{'},
  [0X5B] = {VAL_PASSCHAR+']', VAL_PASSCHAR+'}'},
  [0X5D] = {VAL_PASSCHAR+'\\', VAL_PASSCHAR+'|'},
  [0X58] = {AT2_CAPS_LOCK},
  [0X1C] = {VAL_PASSCHAR+'a', VAL_PASSCHAR+'A'},
  [0X1B] = {VAL_PASSCHAR+'s', VAL_PASSCHAR+'S'},
  [0X23] = {VAL_PASSCHAR+'d', VAL_PASSCHAR+'D'},
  [0X2B] = {VAL_PASSCHAR+'f', VAL_PASSCHAR+'F'},
  [0X34] = {VAL_PASSCHAR+'g', VAL_PASSCHAR+'G'},
  [0X33] = {VAL_PASSCHAR+'h', VAL_PASSCHAR+'H'},
  [0X3B] = {VAL_PASSCHAR+'j', VAL_PASSCHAR+'J'},
  [0X42] = {VAL_PASSCHAR+'k', VAL_PASSCHAR+'K'},
  [0X4B] = {VAL_PASSCHAR+'l', VAL_PASSCHAR+'L'},
  [0X4C] = {VAL_PASSCHAR+';', VAL_PASSCHAR+':'},
  [0X52] = {VAL_PASSCHAR+'\'', VAL_PASSCHAR+'"'},
  [0X5A] = {VAL_PASSKEY+VPK_RETURN},
  [0X12] = {AT2_LEFT_SHIFT},
  [0X1A] = {VAL_PASSCHAR+'z', VAL_PASSCHAR+'Z'},
  [0X22] = {VAL_PASSCHAR+'x', VAL_PASSCHAR+'X'},
  [0X21] = {VAL_PASSCHAR+'c', VAL_PASSCHAR+'C'},
  [0X2A] = {VAL_PASSCHAR+'v', VAL_PASSCHAR+'V'},
  [0X32] = {VAL_PASSCHAR+'b', VAL_PASSCHAR+'B'},
  [0X31] = {VAL_PASSCHAR+'n', VAL_PASSCHAR+'N'},
  [0X3A] = {VAL_PASSCHAR+'m', VAL_PASSCHAR+'M'},
  [0X41] = {VAL_PASSCHAR+',', VAL_PASSCHAR+'<'},
  [0X49] = {VAL_PASSCHAR+'.', VAL_PASSCHAR+'>'},
  [0X4A] = {VAL_PASSCHAR+'/', VAL_PASSCHAR+'?'},
  [0X59] = {AT2_RIGHT_SHIFT},
  [0X14] = {AT2_LEFT_CONTROL},
  [0X11] = {AT2_LEFT_ALT},
  [0X29] = {VAL_PASSCHAR+' '},
  [0X77] = {AT2_NUMBER_LOCK},
  [0X7C] = {VAL_PASSCHAR+'*'},
  [0X7B] = {VAL_PASSCHAR+'-'},
  [0X79] = {VAL_PASSCHAR+'+'},
  [0X6C] = {VAL_PASSKEY+VPK_HOME, VAL_PASSCHAR+'7'},
  [0X75] = {VAL_PASSKEY+VPK_CURSOR_UP, VAL_PASSCHAR+'8'},
  [0X7D] = {VAL_PASSKEY+VPK_PAGE_UP, VAL_PASSCHAR+'9'},
  [0X6B] = {VAL_PASSKEY+VPK_CURSOR_LEFT, VAL_PASSCHAR+'4'},
  [0X73] = {VAL_PASSCHAR+'5'},
  [0X74] = {VAL_PASSKEY+VPK_CURSOR_RIGHT, VAL_PASSCHAR+'6'},
  [0X69] = {VAL_PASSKEY+VPK_END, VAL_PASSCHAR+'1'},
  [0X72] = {VAL_PASSKEY+VPK_CURSOR_DOWN, VAL_PASSCHAR+'2'},
  [0X7A] = {VAL_PASSKEY+VPK_PAGE_DOWN, VAL_PASSCHAR+'3'},
  [0X70] = {VAL_PASSKEY+VPK_INSERT, VAL_PASSCHAR+'0'},
  [0X71] = {VAL_PASSKEY+VPK_DELETE, VAL_PASSCHAR+'.'}
};

static const At2KeyTable at2KeysE0 = {
  [0X12] = {AT2_NUMBER_SHIFT},
  [0X1F] = {AT2_LEFT_WINDOWS},
  [0X11] = {AT2_RIGHT_ALT},
  [0X27] = {AT2_RIGHT_WINDOWS},
  [0X2F] = {AT2_MENU},
  [0X14] = {AT2_RIGHT_CONTROL},
  [0X70] = {VAL_PASSKEY+VPK_INSERT},
  [0X71] = {VAL_PASSKEY+VPK_DELETE},
  [0X6C] = {VAL_PASSKEY+VPK_HOME},
  [0X69] = {VAL_PASSKEY+VPK_END},
  [0X7D] = {VAL_PASSKEY+VPK_PAGE_UP},
  [0X7A] = {VAL_PASSKEY+VPK_PAGE_DOWN},
  [0X75] = {VAL_PASSKEY+VPK_CURSOR_UP},
  [0X6B] = {VAL_PASSKEY+VPK_CURSOR_LEFT},
  [0X72] = {VAL_PASSKEY+VPK_CURSOR_DOWN},
  [0X74] = {VAL_PASSKEY+VPK_CURSOR_RIGHT},
  [0X4A] = {VAL_PASSCHAR+'/'},
  [0X5A] = {VAL_PASSKEY+VPK_RETURN}
};

static const At2KeyEntry *at2Keys;
static int at2Modifiers;

#define AT2_BIT(modifier) (1 << (modifier))
#define AT2_SET(modifier) (at2Modifiers |= AT2_BIT((modifier)))
#define AT2_CLR(modifier) (at2Modifiers &= ~AT2_BIT((modifier)))
#define AT2_TST(modifier) (at2Modifiers & AT2_BIT((modifier)))

void
at2Reset (void) {
  at2Keys = at2KeysOriginal;
  at2Modifiers = 0;
}

int
at2Process (int *command, unsigned char byte) {
  if (byte == 0XF0) {
    AT2_SET(AT2_RELEASE);
  } else if (byte == 0XE0) {
    at2Keys = at2KeysE0;
  } else {
    const At2KeyEntry *key = &at2Keys[byte];
    int release = AT2_TST(AT2_RELEASE);

    int cmd = key->command;
    int blk = cmd & VAL_BLK_MASK;

    AT2_CLR(AT2_RELEASE);
    at2Keys = at2KeysOriginal;

    if (key->alternate) {
      int alternate = 0;

      if (blk == VAL_PASSCHAR) {
        if (AT2_TST(AT2_LEFT_SHIFT) || AT2_TST(AT2_RIGHT_SHIFT)) alternate = 1;
      } else {
        if (AT2_TST(AT2_NUMBER_LOCK) || AT2_TST(AT2_NUMBER_SHIFT)) alternate = 1;
      }

      if (alternate) {
        cmd = key->alternate;
        blk = cmd & VAL_BLK_MASK;
      }
    }

    if (cmd) {
      if (blk) {
        if (!release) {
          if (blk == VAL_PASSCHAR) {
            if (AT2_TST(AT2_CAPS_LOCK)) cmd |= VPC_UPPER;
            if (AT2_TST(AT2_LEFT_ALT)) cmd |= VPC_META;
            if (AT2_TST(AT2_LEFT_CONTROL) || AT2_TST(AT2_RIGHT_CONTROL)) cmd |= VPC_CONTROL;
          }

          *command = cmd;
          return 1;
        }
      }

      switch (cmd) {
        case AT2_SCROLL_LOCK:
        case AT2_NUMBER_LOCK:
        case AT2_CAPS_LOCK:
          if (!release) {
            if (AT2_TST(cmd)) {
              AT2_CLR(cmd);
            } else {
              AT2_SET(cmd);
            }
          }
          break;

        case AT2_NUMBER_SHIFT:
        case AT2_LEFT_SHIFT:
        case AT2_RIGHT_SHIFT:
        case AT2_LEFT_CONTROL:
        case AT2_RIGHT_CONTROL:
        case AT2_LEFT_ALT:
        case AT2_RIGHT_ALT:
          if (release) {
            AT2_CLR(cmd);
          } else {
            AT2_SET(cmd);
          }
          break;
      }
    }
  }
  return 0;
}

