/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

#ifndef _BRL_H
#define _BRL_H

/* brl.h - Header file for the Braille display library
 */

/* Argument for readbrl() */
typedef enum {
   CMDS_SCREEN,
   CMDS_HELP,
   CMDS_STATUS,
   CMDS_PREFS,
   CMDS_MESSAGE
} DriverCommandContext;


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
   CMD_NOOP = 0 /* do nothing */,

   /* vertical motion */
   CMD_LNUP /* go up one line */,
   CMD_LNDN /* go down one line */,
   CMD_WINUP /* go up several lines */,
   CMD_WINDN /* go down several lines */,
   CMD_PRDIFLN /* go up to line with different content */,
   CMD_NXDIFLN /* go down to line with different content */,
   CMD_ATTRUP /* go up to line with different attributes */,
   CMD_ATTRDN /* go down to line with different attributes */,
   CMD_TOP /* go to top line */,
   CMD_BOT /* go to bottom line */,
   CMD_TOP_LEFT /* go to top-left corner */,
   CMD_BOT_LEFT /* go to bottom-left corner */,
   CMD_PRBLNKLN /* go to last line of previous paragraph */,
   CMD_NXBLNKLN /* go to first line of next paragraph */,
   CMD_PRSEARCH /* search up for content of cut buffer */,
   CMD_NXSEARCH /* search down for content of cut buffer */,

   /* horizontal motion */
   CMD_CHRLT /* go left one character */,
   CMD_CHRRT /* go right one character */,
   CMD_HWINLT /* go left one half window */,
   CMD_HWINRT /* go right one half window */,
   CMD_FWINLT /* go left one full window */,
   CMD_FWINRT /* go right one full window */,
   CMD_FWINLTSKIP /* go left to non-blank window */,
   CMD_FWINRTSKIP /* go right to non-blank window */,
   CMD_LNBEG /* go to beginning of line */,
   CMD_LNEND /* go to end of line */,

   /* implicit motion */
   CMD_HOME /* go to cursor */,
   CMD_BACK /* go to last motion */,

   /* feature activation and deactivation */
   CMD_FREEZE /* freeze/unfreeze screen */,
   CMD_DISPMD /* toggle display attributes/text */,
   CMD_SIXDOTS /* toggle text style 6-dot/8-dot */,
   CMD_SLIDEWIN /* toggle sliding window on/off */,
   CMD_SKPIDLNS /* toggle skipping of identical lines on/off */,
   CMD_SKPBLNKWINS /* toggle skipping of blank windows on/off */,
   CMD_CSRVIS /* toggle cursor visibility on/off */,
   CMD_CSRHIDE /* toggle quick hide of cursor */,
   CMD_CSRTRK /* toggle cursor tracking on/off */,
   CMD_CSRSIZE /* toggle cursor style underline/block */,
   CMD_CSRBLINK /* toggle cursor blinking on/off */,
   CMD_ATTRVIS /* toggle attribute underlining on/off */,
   CMD_ATTRBLINK /* toggle attribute blinking on/off */,
   CMD_CAPBLINK /* toggle capital letter blinking on/off */,
   CMD_SND /* toggle sound on/off */,

   /* mode selection */
   CMD_HELP /* display driver help */,
   CMD_INFO /* display status summary */,

   /* preference setting */
   CMD_PREFMENU /* present preferences menu */,
   CMD_PREFSAVE /* save preferences */,
   CMD_PREFLOAD /* reload preferences */,

   /* menu navigation */
   CMD_MENU_FIRST_ITEM /* go to first item in menu */,
   CMD_MENU_LAST_ITEM /* go to last item in menu */,
   CMD_MENU_PREV_ITEM /* go to previous item in menu */,
   CMD_MENU_NEXT_ITEM /* go to next item in menu */,
   CMD_MENU_PREV_SETTING /* change value to previous choice */,
   CMD_MENU_NEXT_SETTING /* change value to next choice */,

   /* speech */
   CMD_SAY /* speak current line */,
   CMD_SAYALL /* speak rest of screen */,
   CMD_MUTE /* stop speaking immediately */,
   CMD_SPKHOME /* goto current/last speech position */,

   /* virtual terminal switching */
   CMD_SWITCHVT_PREV /* switch to previous virtual terminal */,
   CMD_SWITCHVT_NEXT /* switch to next virtual terminal */,

   /* miscellaneous */
   CMD_CSRJMP_VERT /* route cursor to top line of window */,
   CMD_PASTE /* insert cut buffer at cursor */,
   CMD_RESTARTBRL /* reinitialize braille driver */,
   CMD_RESTARTSPEECH /* reinitialize speech driver */,

   /* for displays without routing keys */
   CMD_CSRJMP /* route cursor to top-left corner of braille window */,
   CMD_CUT_BEG /* cut text from top-left corner of braille window */,
   CMD_CUT_END /* cut text to bottom-right corner of braille window */,

   /* internal (to the driver) use only */
   CMD_INPUTMODE /* toggle input mode */,

   DriverCommandCount /* must be last */
} DriverCommand;
#define VAL_ARG_MASK 0XFF
#define VAL_BLK_MASK 0XFF00
#define VAL_FLG_MASK 0XFF0000

/* For specifically turning on/off toggle commands */
#define VAL_SWITCHON    0x10000
#define VAL_SWITCHOFF   0x20000
#define VAL_SWITCHMASK  (VAL_SWITCHON | VAL_SWITCHOFF)

/* cursor routing keys block offset values */
/*
 * Please comment all CR_* definitions. They are
 * used during automatic help file generation.
 */
#define CR_ROUTEOFFSET 0x100	/* route cursor to character */
#define CR_BEGBLKOFFSET	0x200	/* define the beginning of a block */
#define CR_ENDBLKOFFSET 0x300	/* define the end of a block */
#define CR_SWITCHVT 0x400	/* switch virtual terminal */
#define CR_NXINDENT 0x500	/* find next line not more indented than routing key indicates */
#define CR_PRINDENT 0x600       /* find previous line not more indented than routing key indicates */
#define CR_MSGATTRIB 0x700	/* show attributes of character */
#define CR_SETLEFT 0x800	/* bring left of window to character */

/* For entering a special key. */
#define VAL_PASSKEY	0xD00
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

/* For typing a character -- pass through. */
#define VAL_PASSCHAR	0xE00
#define VPC_CONTROL 0X010000
#define VPC_META    0X020000
#define VPC_UPPER   0X040000
#define VPC_SHIFT   0X080000

/* For typing a character -- use current translation table. */
#define VAL_PASSDOTS      0xF00

/* Braille information structure */
typedef struct
  {
    unsigned char *disp;	/* contents of the display */
    int x, y;			/* size of display */
  }
brldim;				/* used for writing to a braille display */


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

/* Routines provided by the braille driver library */
/* these are load dynamically at runtime into this structure with pointers to 
   all the functions and vars */

typedef struct 
{
  char *name;			/* name of driver */
  char *identifier;		/* name of driver */
  char **parameters;		/* user-supplied driver parameters */
  char *help_file;		/* name of help file */
  int status_style;		/* prefered status cells mode */

  /* Routines provided by the braille driver library: */
  void (*identify) (void);	/* print start-up messages */
  void (*initialize) (char **parameters, brldim *, const char *);	/* initialise Braille display */
  void (*close) (brldim *);		/* close braille display */
  void (*write) (brldim *);		/* write to braille display */
  int (*read) (DriverCommandContext);		/* get key press from braille display */
  void (*setstatus) (const unsigned char *);	/* set status cells */

} braille_driver;

extern braille_driver *braille;	/* filled by dynamic libs */
extern int loadBrailleDriver (const char **libraryName);
extern int listBrailleDrivers (void);

/*
 * Please comment all STAT_* definitions. They are
 * used during automatic help file generation.
 */
typedef enum {
   FirstStatusCell = 0,
   #define FSC_GENERIC 0XFF

   /* numbers */
   STAT_brlcol /* current line number */,
   STAT_brlrow /* current line number */,
   STAT_csrcol /* cursor position - column */,
   STAT_csrrow /* cursor position - row */,
   STAT_scrnum /* virtual screen number */,

   /* flags */
   STAT_tracking /* cursor tracking */,
   STAT_dispmode /* dispmode (text / attribut) */,
   STAT_frozen /* screen frozen */,
   STAT_visible /* cursor visible */,
   STAT_size /* cursor size */,
   STAT_blink /* cursor blink */,
   STAT_capitalblink /* capital letter blink */,
   STAT_dots /* 6 or 8 dots */,
   STAT_sound /* sound */,
   STAT_skip /* skip identical lines */,
   STAT_underline /* attribute underlining */,
   STAT_blinkattr /* blinking of attribute underlining */,

   /* internal (to the driver) use only */
   STAT_input /* input mode */,

   StatusCellCount /* must be last */
} StatusCell;

#endif /* !defined(_BRL_H) */
