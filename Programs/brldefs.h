/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file LGPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef _BRLDEFS_H
#define _BRLDEFS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Argument for brl_readCommand() */
typedef enum {
  BRL_CTX_SCREEN,
  BRL_CTX_HELP,
  BRL_CTX_STATUS,
  BRL_CTX_PREFS,
  BRL_CTX_MESSAGE
} BRL_DriverCommandContext;

/* The following define command codes, which are return values for
 * readbrl().  The CMD_* codes are guaranteed to be between 1 and 127
 * inclusive, with the exception of CMD_NOOP = 0.
 *
 * readbrl() should return EOF if there are no keys to return.  If,
 * however, it has masked out the key for internal use, it may return
 * CMD_NOOP - in which case it will be called again immediately, rather
 * than waiting for the next cycle.  This can save internal nesting ...
 */

/*
 * Please comment all CMD_* definitions. They are
 * used during automatic help file generation.
 */
typedef enum {
  /* special commands which must be first and remain in order */
  BRL_CMD_NOOP = 0 /* do nothing */,

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
#define VAL_ARG_MASK 0XFF
#define VAL_BLK_MASK 0XFF00
#define VAL_FLG_MASK 0XFF0000
#define VAL_CMD_MASK (VAL_BLK_MASK | VAL_ARG_MASK)
  
/* For explicitly setting toggles on/off. */
#define VAL_TOGGLE_ON   0x010000
#define VAL_TOGGLE_OFF  0x020000
#define VAL_TOGGLE_MASK (VAL_TOGGLE_ON | VAL_TOGGLE_OFF)

#define VAL_REPEAT_INITIAL 0x800000
#define VAL_REPEAT_DELAY   0x400000
#define VAL_REPEAT_MASK    (VAL_REPEAT_INITIAL | VAL_REPEAT_DELAY)
#define IS_DELAYED_COMMAND(cmd) (((cmd) & VAL_REPEAT_DELAY) && !((cmd) & VAL_REPEAT_INITIAL))
  
/* cursor routing keys block offset values */
/*
 * Please comment all CR_* definitions. They are
 * used during automatic help file generation.
 */
#define CR_ROUTE     0X100   /* bring cursor to character */
#define CR_CUTBEGIN  0X200 /* start new cut buffer at character */
#define CR_CUTAPPEND 0X300 /* append to existing cut buffer from character */
#define CR_CUTRECT   0X400 /* rectangular cut to character */
#define CR_CUTLINE   0X500 /* linear cut to character */
#define CR_SWITCHVT  0X600 /* switch to virtual terminal */
#define CR_PRINDENT  0X700 /* go up to nearest line without greater indent */
#define CR_NXINDENT  0X800 /* go down to nearest line without greater indent */
#define CR_DESCCHAR  0X900 /* describe character */
#define CR_SETLEFT   0XA00 /* position left end of window at character */
#define CR_SETMARK   0XB00 /* remember current window position */
#define CR_GOTOMARK  0XC00 /* go to remembered window position */

/* For entering a special key. */
#define VAL_PASSKEY 0X2000
typedef enum {
  VPK_RETURN,
  VPK_TAB,
  VPK_BACKSPACE,
  VPK_ESCAPE,
  VPK_CURSOR_LEFT,
  VPK_CURSOR_RIGHT,
  VPK_CURSOR_UP,
  VPK_CURSOR_DOWN,
  VPK_PAGE_UP,
  VPK_PAGE_DOWN,
  VPK_HOME,
  VPK_END,
  VPK_INSERT,
  VPK_DELETE,
  VPK_FUNCTION
} Key;

#define VAL_PASSCHAR 0X2100 /* input character by value */
#define VAL_PASSDOTS 0X2200 /* input character as braille dots */

/* For modifying a character to be typed. */
#define VPC_CONTROL 0X010000
#define VPC_META    0X020000
#define VPC_UPPER   0X040000
#define VPC_SHIFT   0X080000

#define VAL_PASSAT2 0X2300 /* input AT set 2 keyboard scan code */

/* status cell styles */
typedef enum {
  ST_None,
  ST_AlvaStyle,
  ST_TiemanStyle,
  ST_PB80Style,
  ST_Generic,
  ST_MDVStyle,
  ST_VoyagerStyle
} StatusCellStyles;

/*
 * Please comment all STAT_* definitions. They are
 * used during automatic help file generation.
 */
typedef enum {
  FirstStatusCell = 0,
  #define FSC_GENERIC 0XFF

  /* numbers */
  STAT_BRLCOL /* screen column where left of braille window is */,
  STAT_BRLROW /* screen row where top of braille window is */,
  STAT_CSRCOL /* screen column where cursor is */,
  STAT_CSRROW /* screen row where cursor is */,
  STAT_SCRNUM /* virtual screen number */,

  /* flags */
  STAT_FREEZE /* frozen screen */,
  STAT_DISPMD /* attributes display */,
  STAT_SIXDOTS /* six-dot braille */,
  STAT_SLIDEWIN /* sliding window */,
  STAT_SKPIDLNS /* skip identical lines */,
  STAT_SKPBLNKWINS /* skip blank windows */,
  STAT_CSRVIS /* visible cursor */,
  STAT_CSRHIDE /* hidden cursor */,
  STAT_CSRTRK /* cursor tracking */,
  STAT_CSRSIZE /* block cursor */,
  STAT_CSRBLINK /* blinking cursor */,
  STAT_ATTRVIS /* visible attributes underline */,
  STAT_ATTRBLINK /* blinking attributes underline */,
  STAT_CAPBLINK /* blinking capital letters */,
  STAT_TUNES /* alert tunes */,
  STAT_HELP /* help mode */,
  STAT_INFO /* info mode */,
  STAT_AUTOREPEAT /* autorepeat */,
  STAT_AUTOSPEAK /* autospeak */,

  StatusCellCount /* must be last */
} StatusCell;

/* The bits for each braille dot.
 *
 * DotNumber  BitNumber  ActualBit
 *    1 4        0 1     0X01 0X02
 *    2 5        2 3     0X04 0X08
 *    3 6        4 5     0X10 0X20
 *    7 8        6 7     0X40 0X80
 *
 * This dot mapping was chosen for BRLTTY's internal dot representation
 * because it makes programming prettier and more efficient for things such
 * as underlining, cursor drawing, and vertical braille character shifting.
 *
 * Bx: x from dot number, value from actual bit.
 */
#define B1 0X01
#define B2 0X04
#define B3 0X10
#define B4 0X02
#define B5 0X08
#define B6 0X20
#define B7 0X40
#define B8 0X80

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BRLDEFS_H */
