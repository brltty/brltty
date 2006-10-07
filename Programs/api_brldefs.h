/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file COPYING-API for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_BRLDEFS
#define BRLTTY_INCLUDED_BRLDEFS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "api.h"

typedef enum {
  BRL_CMD_NOOP = BRLAPI_KEY_TYPE_CMD,

  /* vertical motion */
  BRL_CMD_LNUP /* go up one line */,
  BRL_CMD_LNDN /* go down one line */,
  BRL_CMD_WINUP /* go up several lines */,
  BRL_CMD_WINDN /* go down several lines */,
  BRL_CMD_PRDIFLN /* go up to nearest line with different content */,
  BRL_CMD_NXDIFLN /* go down to nearest line with different content */,
  BRL_CMD_ATTRUP /* go up to nearest line with different highlighting */,
  BRL_CMD_ATTRDN /* go down to nearest line with different highlighting */,
  BRL_CMD_TOP /* go to top line */,
  BRL_CMD_BOT /* go to bottom line */,
  BRL_CMD_TOP_LEFT /* go to top-left corner */,
  BRL_CMD_BOT_LEFT /* go to bottom-left corner */,
  BRL_CMD_PRPGRPH /* go up to last line of previous paragraph */,
  BRL_CMD_NXPGRPH /* go down to first line of next paragraph */,
  BRL_CMD_PRPROMPT /* go up to previous command prompt */,
  BRL_CMD_NXPROMPT /* go down to next command prompt */,
  BRL_CMD_PRSEARCH /* search backward for content of cut buffer */,
  BRL_CMD_NXSEARCH /* search forward for content of cut buffer */,
  
  /* horizontal motion */
  BRL_CMD_CHRLT /* go left one character */,
  BRL_CMD_CHRRT /* go right one character */,
  BRL_CMD_HWINLT /* go left half a window */,
  BRL_CMD_HWINRT /* go right half a window */,
  BRL_CMD_FWINLT /* go left one window */,
  BRL_CMD_FWINRT /* go right one window */,
  BRL_CMD_FWINLTSKIP /* go left to nearest non-blank window */,
  BRL_CMD_FWINRTSKIP /* go right to nearest non-blank window */,
  BRL_CMD_LNBEG /* go to beginning of line */,
  BRL_CMD_LNEND /* go to end of line */,
  
  /* implicit motion */
  BRL_CMD_HOME /* go to cursor */,
  BRL_CMD_BACK /* go back (undo unexpected cursor tracking motion) */,
  BRL_CMD_RETURN /* go back (after cursor tracking) or to cursor (cursor not in window) */,
  
  /* feature activation and deactivation */
  BRL_CMD_FREEZE /* toggle screen mode frozen/live */,
  BRL_CMD_DISPMD /* toggle display mode attributes/text */,
  BRL_CMD_SIXDOTS /* toggle text style 6-dot/8-dot */,
  BRL_CMD_SLIDEWIN /* toggle sliding window on/off */,
  BRL_CMD_SKPIDLNS /* toggle skipping of lines with identical content on/off */,
  BRL_CMD_SKPBLNKWINS /* toggle skipping of blank windows on/off */,
  BRL_CMD_CSRVIS /* toggle cursor visibility on/off */,
  BRL_CMD_CSRHIDE /* toggle hidden cursor on/off */,
  BRL_CMD_CSRTRK /* toggle cursor tracking on/off */,
  BRL_CMD_CSRSIZE /* toggle cursor style block/underline */,
  BRL_CMD_CSRBLINK /* toggle cursor blinking on/off */,
  BRL_CMD_ATTRVIS /* toggle attribute underlining on/off */,
  BRL_CMD_ATTRBLINK /* toggle attribute blinking on/off */,
  BRL_CMD_CAPBLINK /* toggle capital letter blinking on/off */,
  BRL_CMD_TUNES /* toggle alert tunes on/off */,
  BRL_CMD_AUTOREPEAT /* toggle autorepeat on/off */,
  BRL_CMD_AUTOSPEAK /* toggle autospeak on/off */,
 
  /* mode selection */
  BRL_CMD_HELP /* enter/leave help display */,
  BRL_CMD_INFO /* enter/leave status display */,
  BRL_CMD_LEARN /* enter/leave command learn mode */,
  
  /* preference setting */
  BRL_CMD_PREFMENU /* enter/leave preferences menu */,
  BRL_CMD_PREFSAVE /* save current preferences */,
  BRL_CMD_PREFLOAD /* restore saved preferences */,
  
  /* menu navigation */
  BRL_CMD_MENU_FIRST_ITEM /* go to first item in menu */,
  BRL_CMD_MENU_LAST_ITEM /* go to last item in menu */,
  BRL_CMD_MENU_PREV_ITEM /* go to previous item in menu */,
  BRL_CMD_MENU_NEXT_ITEM /* go to next item in menu */,
  BRL_CMD_MENU_PREV_SETTING /* change current item in menu to previous choice */,
  BRL_CMD_MENU_NEXT_SETTING /* change current item in menu to next choice */,
 
  /* speech controls */
  BRL_CMD_MUTE /* stop speaking immediately */,
  BRL_CMD_SPKHOME /* go to current (most recent) speech position */,
  BRL_CMD_SAY_LINE /* speak current line */,
  BRL_CMD_SAY_ABOVE /* speak from top of screen through current line */,
  BRL_CMD_SAY_BELOW /* speak from current line through bottom of screen */,
  BRL_CMD_SAY_SLOWER /* decrease speech rate */,
  BRL_CMD_SAY_FASTER /* increase speech rate */,
  BRL_CMD_SAY_SOFTER /* decrease speech volume */,
  BRL_CMD_SAY_LOUDER /* increase speech volume */,
  
  /* virtual terminal switching */
  BRL_CMD_SWITCHVT_PREV /* switch to previous virtual terminal */,
  BRL_CMD_SWITCHVT_NEXT /* switch to next virtual terminal */,
  
  /* miscellaneous */
  BRL_CMD_CSRJMP_VERT /* bring cursor to line (no horizontal motion) */,
  BRL_CMD_PASTE /* insert cut buffer at cursor */,
  BRL_CMD_RESTARTBRL /* reinitialize braille driver */,
  BRL_CMD_RESTARTSPEECH /* reinitialize speech driver */,
  
  BRL_driverCommandCount /* must be last */
} BRL_DriverCommand;

#define BRL_MSK_ARG		BRLAPI_KEY_CMD_ARG_MASK
#define BRL_MSK_BLK		(BRLAPI_KEY_TYPE_MASK | BRLAPI_KEY_CMD_BLK_MASK)
#define BRL_MSK_FLG		BRLAPI_KEY_FLAGS_MASK
#define BRL_MSK_CMD		(BRL_MSK_BLK | BRL_MSK_ARG)
  
/* For explicitly setting toggles on/off. */
#define BRL_FLG_TOGGLE_ON	BRLAPI_KEY_FLG_TOGGLE_ON
#define BRL_FLG_TOGGLE_OFF	BRLAPI_KEY_FLG_TOGGLE_OFF
#define BRL_FLG_TOGGLE_MASK	(BRL_FLG_TOGGLE_ON | BRL_FLG_TOGGLE_OFF)

/* For automatic cursor routing. */
#define BRL_FLG_ROUTE		BRLAPI_KEY_FLG_ROUTE

#define BRL_FLG_REPEAT_INITIAL	BRLAPI_KEY_FLG_REPEAT_INITIAL
#define BRL_FLG_REPEAT_DELAY	BRLAPI_KEY_FLG_REPEAT_DELAY
#define BRL_FLG_REPEAT_MASK	(BRL_FLG_REPEAT_INITIAL | BRL_FLG_REPEAT_DELAY)
#define IS_DELAYED_COMMAND(cmd)	(((cmd) & BRL_FLG_REPEAT_DELAY) && !((cmd) & BRL_FLG_REPEAT_INITIAL))
  
/* cursor routing keys block offset values */
/*
 * Please comment all BRL_BLK_* definitions. They are
 * used during automatic help file generation.
 */
#define BRL_BLK_ROUTE     	BRLAPI_KEY_CMD_ROUTE
#define BRL_BLK_CUTBEGIN	BRLAPI_KEY_CMD_CUTBEGIN
#define BRL_BLK_CUTAPPEND	BRLAPI_KEY_CMD_CUTAPPEND
#define BRL_BLK_CUTRECT		BRLAPI_KEY_CMD_CUTRECT
#define BRL_BLK_CUTLINE		BRLAPI_KEY_CMD_CUTLINE
#define BRL_BLK_SWITCHVT	BRLAPI_KEY_CMD_SWITCHVT
#define BRL_BLK_PRINDENT	BRLAPI_KEY_CMD_PRINDENT
#define BRL_BLK_NXINDENT	BRLAPI_KEY_CMD_NXINDENT
#define BRL_BLK_DESCCHAR	BRLAPI_KEY_CMD_DESCCHAR
#define BRL_BLK_SETLEFT		BRLAPI_KEY_CMD_SETLEFT
#define BRL_BLK_SETMARK		BRLAPI_KEY_CMD_SETMARK
#define BRL_BLK_GOTOMARK	BRLAPI_KEY_CMD_GOTOMARK

#define BRL_BLK_GOTOLINE	BRLAPI_KEY_CMD_GOTOLINE
#define BRL_FLG_LINE_SCALED	BRLAPI_KEY_FLG_LINE_SCALED
#define BRL_FLG_LINE_TOLEFT	BRLAPI_KEY_FLG_LINE_TOLEFT

#define BRL_BLK_PRDIFCHAR	BRLAPI_KEY_CMD_PRDIFCHAR
#define BRL_BLK_NXDIFCHAR	BRLAPI_KEY_CMD_NXDIFCHAR

/* For entering a special key. */
#define BRL_BLK_PASSKEY 0X0000FF00
enum {
  BRL_KEY_ENTER = 0x0d,
  BRL_KEY_TAB = 0x09,
  BRL_KEY_BACKSPACE = 0x08,
  BRL_KEY_ESCAPE = 0x01d,
  BRL_KEY_CURSOR_LEFT = 0x51,
  BRL_KEY_CURSOR_RIGHT = 0x53,
  BRL_KEY_CURSOR_UP = 0x52,
  BRL_KEY_CURSOR_DOWN = 0x54,
  BRL_KEY_PAGE_UP = 0x55,
  BRL_KEY_PAGE_DOWN = 0x56,
  BRL_KEY_HOME = 0x50,
  BRL_KEY_END = 0x57,
  BRL_KEY_INSERT = 0x63,
  BRL_KEY_DELETE = 0xff,
  BRL_KEY_FUNCTION = 0xbe,
};

#define BRL_BLK_PASSCHAR	0X00000000ULL /* input character by value */
#define BRL_BLK_PASSDOTS	BRLAPI_KEY_CMD_PASSDOTS /* input character as braille dots */

/* For modifying a character to be typed. */
#define BRL_FLG_CHAR_SHIFT	BRLAPI_KEY_FLG_SHIFT
#define BRL_FLG_CHAR_UPPER	BRLAPI_KEY_FLG_UPPER
#define BRL_FLG_CHAR_CONTROL	BRLAPI_KEY_FLG_CONTROL
#define BRL_FLG_CHAR_META	BRLAPI_KEY_FLG_META
#define BRL_FLT_CHAR_ALL	(BRLAPI_KEY_FLG_SHIFT|BRLAPI_KEY_FLG_UPPER|BRLAPI_KEY_FLG_CONTROL|BRLAPI_KEY_FLG_META)

#define BRL_BLK_PASSAT2		BRLAPI_KEY_CMD_PASSAT2

#define BRL_DOT1 BRLAPI_DOT1
#define BRL_DOT2 BRLAPI_DOT2
#define BRL_DOT3 BRLAPI_DOT3
#define BRL_DOT4 BRLAPI_DOT4
#define BRL_DOT5 BRLAPI_DOT5
#define BRL_DOT6 BRLAPI_DOT6
#define BRL_DOT7 BRLAPI_DOT7
#define BRL_DOT8 BRLAPI_DOT8

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRLDEFS */
