/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.0, 26 July 1996
 *
 * Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
 */

/* brl.h - Header file for the Braille display library
 * N. Nair, 25 January 1996
 */

/* Arguments for readbrl() */
#define TBL_CMD 0		/* Interpret as command */
#define TBL_ARG 1		/* Interpret as argument */

/* The following define argument codes: */
#define ARG_YES 'y'		/* yes */
#define ARG_NO 'n'		/* no */
#define ARG_RET ' '		/* data entry terminator (space) */
#define ARG_QUIT 'q'		/* cancel command */


/* The following define command codes: */

#define CMD_ERR '\0'		/* invalid code */

/* braille window movement */
#define CMD_LNUP 'u'		/* go up one line */
#define CMD_LNDN 'd'		/* go down one line */
#define CMD_WINUP '<'		/* go up one window */
#define CMD_WINDN '>'		/* go down one window */
#define CMD_TOP 't'		/* go to top of screen */
#define CMD_BOT 'b'		/* go to bottom of screen */
#define CMD_HWINLT '['		/* go left one half window */
#define CMD_HWINRT ']'		/* go right one half window */
#define CMD_FWINLT '{'		/* go left one full window */
#define CMD_FWINRT '}'		/* go right one full window */
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

/* Cursor routing */
#define CMD_CSRJMP 'j'		/* jump cursor to window (cursor routing) */
/* Cursor routing offset values */
#define	CR_ROUTEOFFSET 0x080	/* normal cursor routing */

/* Cut and paste */
#define CMD_CUT_BEG 'C'
#define CMD_CUT_END 'E'
#define	CR_BEGBLKOFFSET	0x100	/* to define the beginning of a block */
#define	CR_ENDBLKOFFSET 0x180	/* to define the end of the block */
#define CMD_PASTE 'P'

/* Configuration commands */
#define CMD_CONFMENU 'x'	/* enter configuration menu */
#define CMD_SAVECONF '='	/* save brltty configuration */
#define CMD_RESET 'r'		/* restore saved (or default) settings */

/* Configuration options */
#define CMD_CSRVIS 'v'		/* toggle cursor visibility */
#define CMD_CSRSIZE 'z'		/* toggle cursor size */
#define CMD_CSRBLINK '#'	/* toggle cursor blink */
#define CMD_CAPBLINK '*'	/* toggle capital letter blink */
#define CMD_SIXDOTS '6'		/* toggle six-dot mode */
#define CMD_SLIDEWIN 'w'	/* toggle sliding window */
#define CMD_SND 'S'		/* toggle sound on/off */


/* Key mappings: keys on the braille device are mapped to keyboard keys */
#define CMD_KEY_UP 'U'
#define CMD_KEY_DOWN 'D'
#define CMD_KEY_RIGHT 'R'
#define CMD_KEY_LEFT 'L'
#define CMD_KEY_RETURN 'N'

/* For external `say' command: */
#define CMD_SAY 'Y'


typedef struct
  {
    short x, y;			/* size of display */
    unsigned char *disp;	/* contents of the display */
  }
brldim;				/* used for writing to a braille display */

/* Routines provided by this library: */
void identbrl (const char *);	/* print start-up messages */
brldim initbrl (const char *);	/* initialise Braille display */
void closebrl (brldim);		/* close braille display */
void writebrl (brldim);		/* write to braille display */
int readbrl (int);		/* get key press from braille display */
#if defined (Alva_ABT3)
void WriteBrlStatus (unsigned char *);	/* write to status cells */
#elif defined (CombiBraille)
void setbrlstat (const unsigned char *);	/* set status cells */
#endif
