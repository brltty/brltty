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
 * Please: comment all CMD_* - this info is used for the Papenmeier helpfile
 *
 */
typedef enum {
   /* special commands which must be first and remain in order */
   CMD_NOOP /* do nothing */,

   /* vertical motion */
   CMD_LNUP /* go up one line */,
   CMD_LNDN /* go down one line */,
   CMD_WINUP /* go up several lines */,
   CMD_WINDN /* go down several lines */,
   CMD_PRDIFLN /* go up to line with different content */,
   CMD_NXDIFLN /* go down to line with different content */,
   CMD_ATTRUP /* go up to line with different attributes */,
   CMD_ATTRDN /* go down to line with different attributes */,
   CMD_PRBLNKLN /* go to last line of previous paragraph */,
   CMD_NXBLNKLN /* go to first line of next paragraph */,
   CMD_PRSEARCH /* search up for content of cut buffer */,
   CMD_NXSEARCH /* search down for content of cut buffer */,
   CMD_TOP /* go to top line */,
   CMD_BOT /* go to bottom line */,
   CMD_TOP_LEFT /* go to top-left corner */,
   CMD_BOT_LEFT /* go to bottom-left corner */,

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

   /* cursor related */
   CMD_HOME /* go to cursor */,
   CMD_BACK /* go to last motion */,
   CMD_CSRJMP /* route cursor to top-left corner of braille window */,
   CMD_CSRJMP_VERT /* route cursor to top line of window */,

   /* cut and paste */
   CMD_CUT_BEG /* cut text from top-left corner of braille window */,
   CMD_CUT_END /* cut text to bottom-right corner of braille window */,
   CMD_PASTE /* insert cut buffer at cursor */,

   /* driver options */
   CMD_FREEZE /* freeze/unfreeze screen */,
   CMD_DISPMD /* toggle display attributes/text */,
   CMD_SIXDOTS /* toggle text style 6-dot/8-dot */,
   CMD_SLIDEWIN /* toggle sliding window on/off */,
   CMD_SKPIDLNS /* toggle skipping of identical lines on/off */,
   CMD_SKPBLNKWINS /* toggle skipping of blank windows on/off */,
   CMD_CSRVIS /* toggle cursor visibility on/off */,
   CMD_CSRHIDE_QK /* toggle quick hide of cursor */,
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
   CMD_PREFMENU /* present preferences menu */,

   /* preferences maintenance */
   CMD_PREFSAVE /* save preferences */,
   CMD_PREFLOAD /* reload preferences */,

   /* speech control */
   CMD_SAY /* speak current line */,
   CMD_SAYALL /* speak rest of screen */,
   CMD_MUTE /* stop speaking immediately */,
   CMD_SPKHOME /* goto current/last speech position */,

   /* virtual terminal selection */
   CMD_SWITCHVT_PREV /* switch to previous virtual terminal */,
   CMD_SWITCHVT_NEXT /* switch to next virtual terminal */,

   /* general control */
   CMD_RESTARTBRL /* reinitialize braille driver */,
   CMD_RESTARTSPEECH /* reinitialize speech driver */,
} DriverCommand;

/* Cursor routing key offset values */
#define	CR_ROUTEOFFSET 0x100	/* route cursor to character */
#define	CR_BEGBLKOFFSET	0x200	/* define the beginning of a block */
#define	CR_ENDBLKOFFSET 0x300	/* define the end of a block */
#define	CR_SWITCHVT 0x400	/* switch virtual terminal */
#define	CR_NXINDENT 0x500	/* find next line */
#define	CR_PRINDENT 0x600       /* or previous line */
				/* not more indented than routing key indicates */
#define	CR_MSGATTRIB 0x700	/* message attributes of character */

/* For entering a special key. */
#define	VAL_PASSKEY	0xD00
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
#define	VAL_PASSCHAR	0xE00

/* For typing a character -- use current translation table. */
#define VAL_PASSDOTS      0xF00

/* For specifically turning on/off toggle commands */
#define VAL_SWITCHMASK  0x30000
#define VAL_SWITCHON    0x10000
#define VAL_SWITCHOFF   0x20000

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
   ST_Papenmeier,
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
extern char *braille_libraryName;	/* name of library */

int load_braille_driver(void);
int list_braille_drivers(void);

/* Status display Papenmeier - comment 
 * Please: comment all STAT_* - this info is used for the Papenmeier helpfile
 */
#define STAT_current   1	/* current line number */
#define STAT_row       2	/* cursor position - row */
#define STAT_col       3	/* cursor position - column */
#define STAT_tracking  5	/* cursor tracking */
#define STAT_dispmode  6	/* dispmode (text / attribut) */
#define STAT_frozen    8	/* screen frozen */
#define STAT_visible  12	/* cursor visible */
#define STAT_size     13	/* cursor size */
#define STAT_blink    14	/* cursor blink */
#define STAT_capitalblink 15	/* capital letter blink */
#define STAT_dots     16	/* 6 or 8 dots */
#define STAT_sound    17	/* sound */
#define STAT_skip     18	/* skip identical lines */
#define STAT_underline 19	/* attribute underlining */
#define STAT_blinkattr 20	/* blinking of attribute underlining */

#endif /* !defined(_BRL_H) */
