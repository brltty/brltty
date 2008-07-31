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
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/** EuroBraille/eu_protocol.c 
 ** Protocol base stuff for Clio/scriba and Esys/Iris.
 ** (C) Yannick PLASSIARD <yan@mistigri.org>
 */

#include "prologue.h"

#include "misc.h"
#include "eu_protocol.h"

t_eubrl_protocol	esysirisProtocol = {
  .init = esysiris_init,
  .reset = esysiris_reset,
  .readPacket = esysiris_readPacket,
  .writePacket = esysiris_writePacket,
  .readCommand = esysiris_readCommand,
  .readKey = esysiris_readKey,
  .keyToCommand = esysiris_keyToCommand,
  .writeWindow = esysiris_writeWindow,
  .hasLcdSupport = esysiris_hasLcdSupport,
  .writeVisual = esysiris_writeVisual,
  .protocolType = ESYSIRIS_PROTOCOL
};

/** Eurobraille Clio protocol **/
t_eubrl_protocol	clioProtocol = {
  .init = clio_init,
  .reset = clio_reset,
  .readPacket = clio_readPacket,
  .writePacket = clio_writePacket,
  .readCommand = clio_readCommand,
  .readKey = clio_readKey,
  .keyToCommand = clio_keyToCommand,
  .writeWindow = clio_writeWindow,
  .hasLcdSupport = clio_hasLcdSupport,
  .writeVisual = clio_writeVisual,
  .protocolType = CLIO_PROTOCOL
};

/*
** Handles a braille key : converts it to a BRLTTY Command
*/
unsigned int		protocol_handleBrailleKey(unsigned int key)
{
  unsigned int res = EOF;
  unsigned int dots = key & 0x000003ff;
  static char altFlag = 0, controlFlag = 0, shiftFlag = 0;

  switch (dots)
    {
      /** Alt, Control, and shift handling **/
    case 0x280:
      if (altFlag)
	altFlag = 0;
      else
	altFlag = 1;
      break;
    case 0x2c0:
      if (controlFlag)
	controlFlag = 0;
      else
	controlFlag = 1;
      break;
    case 0x240:
      if (shiftFlag)
	shiftFlag = 0;
      else
	shiftFlag = 1;
      break;
      
      /** Space, backspace and enter handling */
    case 0x200:
      res = BRL_BLK_PASSCHAR | ' ';
      break;
    case 0x100:
      res = BRL_BLK_PASSKEY + BRL_KEY_BACKSPACE;
      break;
    case 0x300:
      res = BRL_BLK_PASSKEY + BRL_KEY_ENTER;
      break;
      
      /** Other function keys (home, end, f1-f12, ... ) **/
    case 0x232:	res = BRL_BLK_PASSKEY + BRL_KEY_TAB; break;
    case 0x216: 
      res = BRL_FLG_CHAR_SHIFT | (BRL_BLK_PASSKEY + BRL_KEY_TAB); 
      break;
    case 0x208:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP; break;
    case 0x220:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN; break;
    case 0x210:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT; break;
    case 0x202:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT; break;
    case 0x205:	res = BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP; break;
    case 0x228:	res = BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN; break;
    case 0x207:	res = BRL_BLK_PASSKEY + BRL_KEY_HOME; break;
    case 0x238:	res = BRL_BLK_PASSKEY + BRL_KEY_END; break;
    case 0x224:	res = BRL_BLK_PASSKEY + BRL_KEY_DELETE; break;
    case 0x21b:	res = BRL_BLK_PASSKEY + BRL_KEY_ESCAPE; break;
    case 0x215:	res = BRL_BLK_PASSKEY + BRL_KEY_INSERT; break;
    case 0x101:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION; break;
    case 0x103:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 1; break;
    case 0x109:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 2; break;
    case 0x119:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 3; break;
    case 0x111:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 4; break;
    case 0x10b:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 5; break;
    case 0x11b:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 6; break;
    case 0x113:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 7; break;
    case 0x10a:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 8; break;
    case 0x11a:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 9; break;
    case 0x105:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 10; break;
    case 0x107:	res = BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 11; break;
      
    default:
      res = BRL_BLK_PASSDOTS | dots;
      break;
    }
  if (res != EOF)
    { 
      if (altFlag)
	{
	  res |= BRL_FLG_CHAR_META;
	  altFlag = 0;
	}
      if (controlFlag)
	{
	  res |= BRL_FLG_CHAR_CONTROL;
	  controlFlag = 0;
	}
      if (shiftFlag)
	{
	  res |= BRL_FLG_CHAR_SHIFT;
	  shiftFlag = 0;
	}
    }
  return res;
}
