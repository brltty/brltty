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
#include <inttypes.h>

#include "at2.h"
#include "brldefs.h"

typedef enum {
  AT2_RELEASE = 0, /* must be first */
  AT2_LEFT_WINDOWS,
  AT2_RIGHT_WINDOWS,
  AT2_MENU,
  AT2_CAPS_LOCK,
  AT2_SCROLL_LOCK,
  AT2_NUMBER_LOCK,
  AT2_NUMBER_SHIFT,
  AT2_LEFT_SHIFT,
  AT2_RIGHT_SHIFT,
  AT2_LEFT_CONTROL,
  AT2_RIGHT_CONTROL,
  AT2_LEFT_ALT,
  AT2_RIGHT_ALT
} At2Modifier;

typedef struct {
  uint16_t command;
  uint16_t alternate;
} At2KeyEntry;
typedef At2KeyEntry At2KeyTable[0X100];

static const At2KeyTable at2KeysOriginal = {
  [0X76] = {BRL_BLK_PASSKEY+BRL_KEY_ESCAPE},
  [0X05] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+0},
  [0X06] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+1},
  [0X04] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+2},
  [0X0C] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+3},
  [0X03] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+4},
  [0X0B] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+5},
  [0X83] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+6},
  [0X0A] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+7},
  [0X01] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+8},
  [0X09] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+9},
  [0X78] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+10},
  [0X07] = {BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+11},
  [0X7E] = {AT2_SCROLL_LOCK},

  [0X0E] = {BRL_BLK_PASSCHAR+'`', BRL_BLK_PASSCHAR+'~'},
  [0X16] = {BRL_BLK_PASSCHAR+'1', BRL_BLK_PASSCHAR+'!'},
  [0X1E] = {BRL_BLK_PASSCHAR+'2', BRL_BLK_PASSCHAR+'@'},
  [0X26] = {BRL_BLK_PASSCHAR+'3', BRL_BLK_PASSCHAR+'#'},
  [0X25] = {BRL_BLK_PASSCHAR+'4', BRL_BLK_PASSCHAR+'$'},
  [0X2E] = {BRL_BLK_PASSCHAR+'5', BRL_BLK_PASSCHAR+'%'},
  [0X36] = {BRL_BLK_PASSCHAR+'6', BRL_BLK_PASSCHAR+'^'},
  [0X3D] = {BRL_BLK_PASSCHAR+'7', BRL_BLK_PASSCHAR+'&'},
  [0X3E] = {BRL_BLK_PASSCHAR+'8', BRL_BLK_PASSCHAR+'*'},
  [0X46] = {BRL_BLK_PASSCHAR+'9', BRL_BLK_PASSCHAR+'('},
  [0X45] = {BRL_BLK_PASSCHAR+'0', BRL_BLK_PASSCHAR+')'},
  [0X4E] = {BRL_BLK_PASSCHAR+'-', BRL_BLK_PASSCHAR+'_'},
  [0X55] = {BRL_BLK_PASSCHAR+'=', BRL_BLK_PASSCHAR+'+'},
  [0X66] = {BRL_BLK_PASSKEY+BRL_KEY_BACKSPACE},

  [0X0D] = {BRL_BLK_PASSKEY+BRL_KEY_TAB},
  [0X15] = {BRL_BLK_PASSCHAR+'q', BRL_BLK_PASSCHAR+'Q'},
  [0X1D] = {BRL_BLK_PASSCHAR+'w', BRL_BLK_PASSCHAR+'W'},
  [0X24] = {BRL_BLK_PASSCHAR+'e', BRL_BLK_PASSCHAR+'E'},
  [0X2D] = {BRL_BLK_PASSCHAR+'r', BRL_BLK_PASSCHAR+'R'},
  [0X2C] = {BRL_BLK_PASSCHAR+'t', BRL_BLK_PASSCHAR+'T'},
  [0X35] = {BRL_BLK_PASSCHAR+'y', BRL_BLK_PASSCHAR+'Y'},
  [0X3C] = {BRL_BLK_PASSCHAR+'u', BRL_BLK_PASSCHAR+'U'},
  [0X43] = {BRL_BLK_PASSCHAR+'i', BRL_BLK_PASSCHAR+'I'},
  [0X44] = {BRL_BLK_PASSCHAR+'o', BRL_BLK_PASSCHAR+'O'},
  [0X4D] = {BRL_BLK_PASSCHAR+'p', BRL_BLK_PASSCHAR+'P'},
  [0X54] = {BRL_BLK_PASSCHAR+'[', BRL_BLK_PASSCHAR+'{'},
  [0X5B] = {BRL_BLK_PASSCHAR+']', BRL_BLK_PASSCHAR+'}'},
  [0X5D] = {BRL_BLK_PASSCHAR+'\\', BRL_BLK_PASSCHAR+'|'},

  [0X58] = {AT2_CAPS_LOCK},
  [0X61] = {BRL_BLK_PASSCHAR+'<', BRL_BLK_PASSCHAR+'>'},
  [0X1C] = {BRL_BLK_PASSCHAR+'a', BRL_BLK_PASSCHAR+'A'},
  [0X1B] = {BRL_BLK_PASSCHAR+'s', BRL_BLK_PASSCHAR+'S'},
  [0X23] = {BRL_BLK_PASSCHAR+'d', BRL_BLK_PASSCHAR+'D'},
  [0X2B] = {BRL_BLK_PASSCHAR+'f', BRL_BLK_PASSCHAR+'F'},
  [0X34] = {BRL_BLK_PASSCHAR+'g', BRL_BLK_PASSCHAR+'G'},
  [0X33] = {BRL_BLK_PASSCHAR+'h', BRL_BLK_PASSCHAR+'H'},
  [0X3B] = {BRL_BLK_PASSCHAR+'j', BRL_BLK_PASSCHAR+'J'},
  [0X42] = {BRL_BLK_PASSCHAR+'k', BRL_BLK_PASSCHAR+'K'},
  [0X4B] = {BRL_BLK_PASSCHAR+'l', BRL_BLK_PASSCHAR+'L'},
  [0X4C] = {BRL_BLK_PASSCHAR+';', BRL_BLK_PASSCHAR+':'},
  [0X52] = {BRL_BLK_PASSCHAR+'\'', BRL_BLK_PASSCHAR+'"'},
  [0X5A] = {BRL_BLK_PASSKEY+BRL_KEY_ENTER},

  [0X12] = {AT2_LEFT_SHIFT},
  [0X1A] = {BRL_BLK_PASSCHAR+'z', BRL_BLK_PASSCHAR+'Z'},
  [0X22] = {BRL_BLK_PASSCHAR+'x', BRL_BLK_PASSCHAR+'X'},
  [0X21] = {BRL_BLK_PASSCHAR+'c', BRL_BLK_PASSCHAR+'C'},
  [0X2A] = {BRL_BLK_PASSCHAR+'v', BRL_BLK_PASSCHAR+'V'},
  [0X32] = {BRL_BLK_PASSCHAR+'b', BRL_BLK_PASSCHAR+'B'},
  [0X31] = {BRL_BLK_PASSCHAR+'n', BRL_BLK_PASSCHAR+'N'},
  [0X3A] = {BRL_BLK_PASSCHAR+'m', BRL_BLK_PASSCHAR+'M'},
  [0X41] = {BRL_BLK_PASSCHAR+',', BRL_BLK_PASSCHAR+'<'},
  [0X49] = {BRL_BLK_PASSCHAR+'.', BRL_BLK_PASSCHAR+'>'},
  [0X4A] = {BRL_BLK_PASSCHAR+'/', BRL_BLK_PASSCHAR+'?'},
  [0X59] = {AT2_RIGHT_SHIFT},

  [0X14] = {AT2_LEFT_CONTROL},
  [0X11] = {AT2_LEFT_ALT},
  [0X29] = {BRL_BLK_PASSCHAR+' '},

  [0X77] = {AT2_NUMBER_LOCK},
  [0X7C] = {BRL_BLK_PASSCHAR+'*'},
  [0X7B] = {BRL_BLK_PASSCHAR+'-'},
  [0X79] = {BRL_BLK_PASSCHAR+'+'},

  [0X6C] = {BRL_BLK_PASSKEY+BRL_KEY_HOME, BRL_BLK_PASSCHAR+'7'},
  [0X75] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP, BRL_BLK_PASSCHAR+'8'},
  [0X7D] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP, BRL_BLK_PASSCHAR+'9'},

  [0X6B] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT, BRL_BLK_PASSCHAR+'4'},
  [0X73] = {BRL_BLK_PASSCHAR+'5'},
  [0X74] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, BRL_BLK_PASSCHAR+'6'},

  [0X69] = {BRL_BLK_PASSKEY+BRL_KEY_END, BRL_BLK_PASSCHAR+'1'},
  [0X72] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN, BRL_BLK_PASSCHAR+'2'},
  [0X7A] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN, BRL_BLK_PASSCHAR+'3'},

  [0X70] = {BRL_BLK_PASSKEY+BRL_KEY_INSERT, BRL_BLK_PASSCHAR+'0'},
  [0X71] = {BRL_BLK_PASSKEY+BRL_KEY_DELETE, BRL_BLK_PASSCHAR+'.'}
};

static const At2KeyTable at2KeysE0 = {
  [0X12] = {AT2_NUMBER_SHIFT},
  [0X1F] = {AT2_LEFT_WINDOWS},
  [0X11] = {AT2_RIGHT_ALT},
  [0X27] = {AT2_RIGHT_WINDOWS},
  [0X2F] = {AT2_MENU},
  [0X14] = {AT2_RIGHT_CONTROL},
  [0X70] = {BRL_BLK_PASSKEY+BRL_KEY_INSERT},
  [0X71] = {BRL_BLK_PASSKEY+BRL_KEY_DELETE},
  [0X6C] = {BRL_BLK_PASSKEY+BRL_KEY_HOME},
  [0X69] = {BRL_BLK_PASSKEY+BRL_KEY_END},
  [0X7D] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP},
  [0X7A] = {BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN},
  [0X75] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP},
  [0X6B] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT},
  [0X72] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN},
  [0X74] = {BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT},
  [0X4A] = {BRL_BLK_PASSCHAR+'/'},
  [0X5A] = {BRL_BLK_PASSKEY+BRL_KEY_ENTER}
};

static const At2KeyEntry *at2Keys;
static int at2Modifiers;

#define AT2_BIT(modifier) (1 << (modifier))
#define AT2_SET(modifier) (at2Modifiers |= AT2_BIT((modifier)))
#define AT2_CLR(modifier) (at2Modifiers &= ~AT2_BIT((modifier)))
#define AT2_TST(modifier) (at2Modifiers & AT2_BIT((modifier)))

void
AT2_resetState (void) {
  at2Keys = at2KeysOriginal;
  at2Modifiers = 0;
}

int
AT2_interpretCode (int *command, unsigned char byte) {
  if (byte == 0XF0) {
    AT2_SET(AT2_RELEASE);
  } else if (byte == 0XE0) {
    at2Keys = at2KeysE0;
  } else {
    const At2KeyEntry *key = &at2Keys[byte];
    int release = AT2_TST(AT2_RELEASE);

    int cmd = key->command;
    int blk = cmd & BRL_MSK_BLK;

    AT2_CLR(AT2_RELEASE);
    at2Keys = at2KeysOriginal;

    if (key->alternate) {
      int alternate = 0;

      if (blk == BRL_BLK_PASSCHAR) {
        if (AT2_TST(AT2_LEFT_SHIFT) || AT2_TST(AT2_RIGHT_SHIFT)) alternate = 1;
      } else {
        if (AT2_TST(AT2_NUMBER_LOCK) || AT2_TST(AT2_NUMBER_SHIFT)) alternate = 1;
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
            if (AT2_TST(AT2_CAPS_LOCK)) cmd |= BRL_FLG_CHAR_UPPER;
            if (AT2_TST(AT2_LEFT_ALT)) cmd |= BRL_FLG_CHAR_META;
            if (AT2_TST(AT2_LEFT_CONTROL) || AT2_TST(AT2_RIGHT_CONTROL)) cmd |= BRL_FLG_CHAR_CONTROL;
          }

          if ((blk == BRL_BLK_PASSKEY) && AT2_TST(AT2_LEFT_ALT)) {
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
  }
  return 0;
}
