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

/* Arguments for readbrl() */
#define TBL_CMD 0		/* Interpret as command */
#define TBL_ARG 1		/* Interpret as argument */

/* The following define argument codes: */
#define ARG_YES 'y'		/* yes */
#define ARG_NO 'n'		/* no */
#define ARG_RET ' '		/* data entry terminator (space) */
#define ARG_QUIT 'q'		/* cancel command */


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

#define CMD_NOOP '\0'		/* do nothing - a blank keystroke */

/* braille window movement */
#define CMD_LNUP 'u'		/* go up one line */
#define CMD_PRDIFLN '-'		/* go to prev different screen line */
#define CMD_LNDN 'd'		/* go down one line */
#define CMD_NXDIFLN '+'		/* go to next different screen line */
#define CMD_ATTRUP 3            /* go to previous line with differing attributes */
#define CMD_ATTRDN 4            /* go to next line with differing attributes */
#define CMD_NXBLNKLN 12		/* look for blank line, go to first non-blank
				   line after that. */
#define CMD_PRBLNKLN 13		/* look for previous blank line, go to first
				   non-blank line before that. */
#define CMD_NXSEARCH 14         /* search screen for cut_buffer */
#define CMD_PRSEARCH 15
#define CMD_WINUP '<'		/* go up one window */
#define CMD_WINDN '>'		/* go down one window */
#define CMD_TOP 't'		/* go to top of screen */
#define CMD_BOT 'b'		/* go to bottom of screen */
#define CMD_HWINLT '['		/* go left one half window */
#define CMD_HWINRT ']'		/* go right one half window */
#define CMD_FWINLT '{'		/* go left one full window */
#define CMD_FWINLTSKIP 9	/* go left one full window, skipping blanks */
#define CMD_FWINRT '}'		/* go right one full window */
#define CMD_FWINRTSKIP 11	/* go right one full window, skipping blanks */
#define CMD_LNBEG 's'		/* go to beginning (start) of line */
#define CMD_LNEND 'e'		/* go to end of line */
#define CMD_CHRLT '('		/* go left one character */
#define CMD_CHRRT ')'		/* go right one character */
#define CMD_TOP_LEFT 'T'	/* go to top of screen */
#define CMD_BOT_LEFT 'B'	/* go to bottom of screen */

/* misc */
#define CMD_HOME 'h'		/* go back to cursor */
#define CMD_CSRTRK 'c'		/* toggle cursor tracking */
#define CMD_DISPMD 'a'		/* Toggle attribute display */
#define CMD_FREEZE 'f'		/* freeze the screen */
#define CMD_HELP '?'		/* display help */
#define CMD_INFO 'i'		/* get status information */
#define CMD_RESTARTBRL 5        /* reinitialize braille display */
#define CMD_RESTARTSPEECH 124   /* reinitialize speech driver */
#define CMD_SWITCHVT_PREV 16 /* switch to previous VT */
#define CMD_SWITCHVT_NEXT 17 /* switch to next VT */

/* Cursor routing */
#define CMD_CSRJMP 'j'		/* jump cursor to window (cursor routing) */
#define CMD_CSRJMP_VERT ';'	/* jump cursor to window's line (routing) */
/* Cursor routing key offset values */
#define	CR_ROUTEOFFSET 0x100	/* normal cursor routing */

/* Cut and paste */
#define CMD_CUT_BEG 'C'		/* cut text begin - use routing keys */
#define CMD_CUT_END 'E'		/* cut text end - use routing keys */
#define CMD_PASTE 'P'		/* insert text */
/* Cursor routing key offset values to be used to define a block */
#define	CR_BEGBLKOFFSET	0x200	/* to define the beginning of a block */
#define	CR_ENDBLKOFFSET 0x300	/* to define the end of the block */

#define	CR_SWITCHVT 0x400	/* switch virtual console */

#define	CR_NXINDENT 0x500	/* find next line not more indented
				   than routing key indicates. */
#define	CR_PRINDENT 0x600

/* Configuration commands */
#define CMD_CONFMENU 'x'	/* enter configuration menu */
#define CMD_SAVECONF '='	/* save brltty configuration */
#define CMD_RESET 'r'		/* restore saved (or default) settings */

/* Configuration options */
#define CMD_CSRVIS 'v'		/* toggle cursor visibility */
#define CMD_CSRHIDE_QK 7		/* quick hide cursor (toggle) */
#define CMD_CSRSIZE 'z'		/* toggle cursor size */
#define CMD_CSRBLINK '#'	/* toggle cursor blink */
#define CMD_CAPBLINK '*'	/* toggle capital letter blink */
#define CMD_ATTRVIS 1           /* toggle attribute underlining */
#define CMD_ATTRBLINK 2         /* toggle blinking of attribute underlining */
#define CMD_SIXDOTS '6'		/* toggle six-dot mode */
#define CMD_SLIDEWIN 'w'	/* toggle sliding window */
#define CMD_SKPIDLNS 'I'	/* toggle skipping of identical lines */
#define CMD_SKPBLNKWINS 8	/* toggle skipping of blank windows */
#define CMD_SND 'S'		/* toggle sound on/off */

/* Key mappings: keys on the braille device are mapped to keyboard keys */
#define CMD_KEY_UP 'U'
#define CMD_KEY_DOWN 'D'
#define CMD_KEY_RIGHT 'R'
#define CMD_KEY_LEFT 'L'
#define CMD_KEY_RETURN 'N'


/* For speech devices: */
#define CMD_SAY 'Y'		/* speak current braille line */
#define CMD_SAYALL 127		/* speak text continuously */
#define CMD_SPKHOME 126		/* goto current/last speech position */
#define CMD_MUTE 'm'		/* stop speech */

/* For specifically turning on/off toggle commands */
#define VAL_SWITCHMASK  0x30000
#define VAL_SWITCHON    0x10000
#define VAL_SWITCHOFF   0x20000

/* For typing character -- pass through */
#define	VAL_PASSTHRU	0x700

/* For typing character -- Using current translation table */
#define VAL_BRLKEY      0x800

/* Braille information structure */
typedef struct
  {
    unsigned char *disp;	/* contents of the display */
    int x, y;			/* size of display */
  }
brldim;				/* used for writing to a braille display */


/* status cells modes */
#define ST_None 0
#define ST_AlvaStyle 1
#define ST_TiemanStyle 2
#define ST_PB80Style 3
#define ST_Papenmeier 4
#define ST_MDVStyle 5
#define NB_STCELLSTYLES 5

/* Routines provided by the braille driver library */
/* these are load dynamically at runtime into this structure with pointers to 
   all the functions and vars */

typedef struct 
{
  char* name;			/* name of driver */
  char* identifier;		/* name of driver */
  char* helpfile;		/* name of help file */
  int pref_style;		/* prefered status cells mode */

  /* Routines provided by the braille driver library: */
  void (*identify) (void);	/* print start-up messages */
  void (*initialize) (brldim *, const char *);	/* initialise Braille display */
  void (*close) (brldim *);		/* close braille display */
  void (*write) (brldim *);		/* write to braille display */
  int (*read) (int);		/* get key press from braille display */
  void (*setstatus) (const unsigned char *);	/* set status cells */

} braille_driver;

extern braille_driver *braille;	/* filled by dynamic libs */
extern char* braille_libname;	/* name of library */

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
