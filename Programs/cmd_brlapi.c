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

 #include <stdio.h>

#include "cmd_brlapi.h"
#include "brldefs.h"
#include "ttb.h"

#ifdef ENABLE_API
static brlapi_keyCode_t
cmdWCharToBrlapi (wchar_t wc) {
  if (iswLatin1(wc)) {
    /* latin1 character */
    return BRLAPI_KEY_TYPE_SYM | wc;
  } else {
    /* unicode character */
    return BRLAPI_KEY_TYPE_SYM | BRLAPI_KEY_SYM_UNICODE | wc;
  }
}

brlapi_keyCode_t
cmdBrlttyToBrlapi (int command, int retainDots) {
  brlapi_keyCode_t code;
  int arg = BRL_ARG_GET(command);
  int blk = command & BRL_MSK_BLK;
  switch (blk) {
  case BRL_BLK_PASSCHAR:
    code = cmdWCharToBrlapi(arg);
    break;
  case BRL_BLK_PASSDOTS:
    if (retainDots) {
      if (arg == (BRLAPI_DOTC >> BRLAPI_KEY_CMD_ARG_SHIFT)) arg = 0;
      goto doDefault;
    }
    code = cmdWCharToBrlapi(convertDotsToCharacter(textTable, arg));
    break;
  case BRL_BLK_PASSKEY:
    switch (arg) {
    case BRL_KEY_ENTER:		code = BRLAPI_KEY_SYM_LINEFEED;	 break;
    case BRL_KEY_TAB:		code = BRLAPI_KEY_SYM_TAB;	 break;
    case BRL_KEY_BACKSPACE:	code = BRLAPI_KEY_SYM_BACKSPACE; break;
    case BRL_KEY_ESCAPE:	code = BRLAPI_KEY_SYM_ESCAPE;	 break;
    case BRL_KEY_CURSOR_LEFT:	code = BRLAPI_KEY_SYM_LEFT;	 break;
    case BRL_KEY_CURSOR_RIGHT:	code = BRLAPI_KEY_SYM_RIGHT;	 break;
    case BRL_KEY_CURSOR_UP:	code = BRLAPI_KEY_SYM_UP;	 break;
    case BRL_KEY_CURSOR_DOWN:	code = BRLAPI_KEY_SYM_DOWN;	 break;
    case BRL_KEY_PAGE_UP:	code = BRLAPI_KEY_SYM_PAGE_UP;	 break;
    case BRL_KEY_PAGE_DOWN:	code = BRLAPI_KEY_SYM_PAGE_DOWN; break;
    case BRL_KEY_HOME:		code = BRLAPI_KEY_SYM_HOME;	 break;
    case BRL_KEY_END:		code = BRLAPI_KEY_SYM_END;	 break;
    case BRL_KEY_INSERT:	code = BRLAPI_KEY_SYM_INSERT;	 break;
    case BRL_KEY_DELETE:	code = BRLAPI_KEY_SYM_DELETE;	 break;
    default: code = BRLAPI_KEY_SYM_FUNCTION + arg - BRL_KEY_FUNCTION; break;
    }
    break;
  default:
  doDefault:
    code = BRLAPI_KEY_TYPE_CMD
         | (blk >> BRL_SHIFT_BLK << BRLAPI_KEY_CMD_BLK_SHIFT)
         | (arg                  << BRLAPI_KEY_CMD_ARG_SHIFT)
         ;
    break;
  }
  if (blk == BRL_BLK_GOTOLINE)
    code = code
    | (command & BRL_FLG_LINE_SCALED	? BRLAPI_KEY_FLG_LINE_SCALED	: 0)
    | (command & BRL_FLG_LINE_TOLEFT	? BRLAPI_KEY_FLG_LINE_TOLEFT	: 0)
      ;
  if (blk == BRL_BLK_PASSCHAR
   || blk == BRL_BLK_PASSKEY
   || blk == BRL_BLK_PASSDOTS)
    code = code
    | (command & BRL_FLG_CHAR_CONTROL	? BRLAPI_KEY_FLG_CONTROL	: 0)
    | (command & BRL_FLG_CHAR_META	? BRLAPI_KEY_FLG_META		: 0)
    | (command & BRL_FLG_CHAR_UPPER	? BRLAPI_KEY_FLG_UPPER		: 0)
    | (command & BRL_FLG_CHAR_SHIFT	? BRLAPI_KEY_FLG_SHIFT		: 0)
      ;
  else
    code = code
    | (command & BRL_FLG_TOGGLE_ON	? BRLAPI_KEY_FLG_TOGGLE_ON	: 0)
    | (command & BRL_FLG_TOGGLE_OFF	? BRLAPI_KEY_FLG_TOGGLE_OFF	: 0)
    | (command & BRL_FLG_MOTION_ROUTE	? BRLAPI_KEY_FLG_MOTION_ROUTE	: 0)
      ;
  return code
    ;
}

int
cmdBrlapiToBrltty (brlapi_keyCode_t code) {
  int cmd;
  switch (code & BRLAPI_KEY_TYPE_MASK) {
  case BRLAPI_KEY_TYPE_CMD:
    cmd = BRL_BLK((code&BRLAPI_KEY_CMD_BLK_MASK)>>BRLAPI_KEY_CMD_BLK_SHIFT)
	| BRL_ARG_SET((code&BRLAPI_KEY_CMD_ARG_MASK)>>BRLAPI_KEY_CMD_ARG_SHIFT);
    break;
  case BRLAPI_KEY_TYPE_SYM: {
      unsigned long keysym;
      keysym = code & BRLAPI_KEY_CODE_MASK;
      switch (keysym) {
      case BRLAPI_KEY_SYM_BACKSPACE:	cmd = BRL_BLK_PASSKEY|BRL_KEY_BACKSPACE;	break;
      case BRLAPI_KEY_SYM_TAB:		cmd = BRL_BLK_PASSKEY|BRL_KEY_TAB;	break;
      case BRLAPI_KEY_SYM_LINEFEED:	cmd = BRL_BLK_PASSKEY|BRL_KEY_ENTER;	break;
      case BRLAPI_KEY_SYM_ESCAPE:	cmd = BRL_BLK_PASSKEY|BRL_KEY_ESCAPE;	break;
      case BRLAPI_KEY_SYM_HOME:		cmd = BRL_BLK_PASSKEY|BRL_KEY_HOME;	break;
      case BRLAPI_KEY_SYM_LEFT:		cmd = BRL_BLK_PASSKEY|BRL_KEY_CURSOR_LEFT;	break;
      case BRLAPI_KEY_SYM_UP:		cmd = BRL_BLK_PASSKEY|BRL_KEY_CURSOR_UP;	break;
      case BRLAPI_KEY_SYM_RIGHT:	cmd = BRL_BLK_PASSKEY|BRL_KEY_CURSOR_RIGHT;	break;
      case BRLAPI_KEY_SYM_DOWN:		cmd = BRL_BLK_PASSKEY|BRL_KEY_CURSOR_DOWN;	break;
      case BRLAPI_KEY_SYM_PAGE_UP:	cmd = BRL_BLK_PASSKEY|BRL_KEY_PAGE_UP;	break;
      case BRLAPI_KEY_SYM_PAGE_DOWN:	cmd = BRL_BLK_PASSKEY|BRL_KEY_PAGE_DOWN;	break;
      case BRLAPI_KEY_SYM_END:		cmd = BRL_BLK_PASSKEY|BRL_KEY_END;	break;
      case BRLAPI_KEY_SYM_INSERT:	cmd = BRL_BLK_PASSKEY|BRL_KEY_INSERT;	break;
      case BRLAPI_KEY_SYM_DELETE:	cmd = BRL_BLK_PASSKEY|BRL_KEY_DELETE;	break;
      default:
	if (keysym >= BRLAPI_KEY_SYM_FUNCTION && keysym <= BRLAPI_KEY_SYM_FUNCTION + 34)
	  cmd = BRL_BLK_PASSKEY | (BRL_KEY_FUNCTION + keysym - BRLAPI_KEY_SYM_FUNCTION);
	else if (keysym < 0x100 || (keysym & 0x1F000000) == BRLAPI_KEY_SYM_UNICODE) {
	  wchar_t c = keysym & 0xFFFFFF;
	  cmd = BRL_BLK_PASSCHAR | BRL_ARG_SET(c);
	} else return EOF;
	break;
      }
      break;
    }
  default:
    return EOF;
  }
  return cmd
  | (code & BRLAPI_KEY_FLG_TOGGLE_ON		? BRL_FLG_TOGGLE_ON	: 0)
  | (code & BRLAPI_KEY_FLG_TOGGLE_OFF		? BRL_FLG_TOGGLE_OFF	: 0)
  | (code & BRLAPI_KEY_FLG_MOTION_ROUTE		? BRL_FLG_MOTION_ROUTE	: 0)
  | (code & BRLAPI_KEY_FLG_LINE_SCALED		? BRL_FLG_LINE_SCALED	: 0)
  | (code & BRLAPI_KEY_FLG_LINE_TOLEFT		? BRL_FLG_LINE_TOLEFT	: 0)
  | (code & BRLAPI_KEY_FLG_CONTROL		? BRL_FLG_CHAR_CONTROL	: 0)
  | (code & BRLAPI_KEY_FLG_META			? BRL_FLG_CHAR_META	: 0)
  | (code & BRLAPI_KEY_FLG_UPPER		? BRL_FLG_CHAR_UPPER	: 0)
  | (code & BRLAPI_KEY_FLG_SHIFT		? BRL_FLG_CHAR_SHIFT	: 0)
    ;
}
#endif /* ENABLE_API */
