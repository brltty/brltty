/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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


#ifndef __EU_BRAILLE_H__
#define __EU_BRAILLE_H__


/** Generic BrlTty Headers **/


#include "brl.h"

/* serial line baudrate 
 */


#define CNTX_INTERNAL	0x80


/**
 ** This union stores all keys from all models, used by the config file parser.
 */

enum	modelKeyDefinitions {
  KEY_UNKNOWN = 0,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_STAR,
  KEY_0,
  KEY_HASH,
  KEY_A,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,
  KEY_ABCD,

  /** Scriba / Iris Keys **/
  KEY_L1,
  KEY_L2,
  KEY_L3,
  KEY_L4,
  KEY_L5,
  KEY_L6,
  KEY_L7,
  KEY_L8,
  KEY_UA,
  KEY_LA,
  KEY_DA,
  KEY_RA,
  KEY_L12,
  KEY_L13,
  KEY_L14,
  KEY_L23,
  KEY_L24,
  KEY_L34,
  KEY_L56,
  KEY_L57,
  KEY_L58,
  KEY_L67,
  KEY_L68,
  KEY_L78,
  KEY_L123,
  KEY_L124,
  KEY_L134,
  KEY_L234,
  KEY_L567,
  KEY_L568,
  KEY_L578,
  KEY_L678,
  KEY_LDA,
  KEY_LRA,
  KEY_LUA,
  KEY_DRA,
  KEY_DUA,
  KEY_RUA,
  KEY_LUDA,

  /** Esys model keys **/
  KEY_J1U,
  KEY_J1D,
  KEY_J1L,
  KEY_J1R,
  KEY_J1C,
  KEY_J2U,
  KEY_J2D,
  KEY_jJ2L,
  KEY_J2R,
  KEY_J2C,
  KEY_M1L,
  KEY_M1R,
  KEY_M1C,
  KEY_M2L,
  KEY_M2R,
  KEY_M2C,
  KEY_M3L,
  KEY_M3R,
  KEY_M3C,
  KEY_M4L,
  KEY_M4R,
  KEY_M4C,

  KEY_LAST
};

typedef struct 
{
  enum modelKeyDefinitions	key;
  char*				str;
}				keyAlias;


/*
** main structure to alias braille keystrokes
*/

typedef struct	s_alias
{
  int		brl;
  int		key;
}		t_alias;

typedef struct	s_key
{
  short int	res;
  unsigned char	brl_key;
  int		(*f)(BrailleDisplay *);
}		t_key;

#endif /* __EU_BRAILLE_H__ */
