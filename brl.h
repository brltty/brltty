/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nicolas Pitre <nico@cam.org>
 * Stéphane Doyon <s.doyon@videotron.ca>
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
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

#define CMD_NOOP '\0'		/* do nothing - a blank keystroke */

/* braille window movement */
#define CMD_LNUP 'u'		/* go up one line */
#define CMD_PRDIFLN '-'		/* go to prev different screen line */
#define CMD_LNDN 'd'		/* go down one line */
#define CMD_NXDIFLN '+'		/* go to next different screen line */
#define CMD_ATTRUP 3            /* previous line we differing attributes */
#define CMD_ATTRDN 4
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
#define CMD_RESTARTBRL 5        /* reinitialize braille display */

/* Cursor routing */
#define CMD_CSRJMP 'j'		/* jump cursor to window (cursor routing) */
#define CMD_CSRJMP_VERT ';'	/* jump cursor to window's line (routing) */
/* Cursor routing key offset values */
#define	CR_ROUTEOFFSET 0x080	/* normal cursor routing */

/* Cut and paste */
#define CMD_CUT_BEG 'C'
#define CMD_CUT_END 'E'
#define CMD_PASTE 'P'
/* Cursor routing key offset values to be used to define a block */
#define	CR_BEGBLKOFFSET	0x100	/* to define the beginning of a block */
#define	CR_ENDBLKOFFSET 0x180	/* to define the end of the block */

/* Configuration commands */
#define CMD_CONFMENU 'x'	/* enter configuration menu */
#define CMD_SAVECONF '='	/* save brltty configuration */
#define CMD_RESET 'r'		/* restore saved (or default) settings */

/* Configuration options */
#define CMD_CSRVIS 'v'		/* toggle cursor visibility */
#define CMD_CSRSIZE 'z'		/* toggle cursor size */
#define CMD_CSRBLINK '#'	/* toggle cursor blink */
#define CMD_CAPBLINK '*'	/* toggle capital letter blink */
#define CMD_ATTRVIS 1           /* toggle attribute underlining */
#define CMD_ATTRBLINK 2         /* toggle blinking of attribute underlining */
#define CMD_SIXDOTS '6'		/* toggle six-dot mode */
#define CMD_SLIDEWIN 'w'	/* toggle sliding window */
#define CMD_SKPIDLNS 'I'	/* toggle skipping of identical lines */
#define CMD_SND 'S'		/* toggle sound on/off */

/* Key mappings: keys on the braille device are mapped to keyboard keys */
#define CMD_KEY_UP 'U'
#define CMD_KEY_DOWN 'D'
#define CMD_KEY_RIGHT 'R'
#define CMD_KEY_LEFT 'L'
#define CMD_KEY_RETURN 'N'

/* For speech devices: */
#define CMD_SAY 'Y'
#define CMD_MUTE 'm'

/* For specifically turning on/off toggle commands */
#define VAL_SWITCHMASK  0x600
#define VAL_SWITCHON    0x200
#define VAL_SWITCHOFF   0x400

/* For typing character -- pass through */
#define	VAL_PASSTHRU	0x800

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

/* Routines provided by the braille driver library: */
void identbrl (const char *);	/* print start-up messages */
void initbrl (brldim *, const char *);	/* initialise Braille display */
void closebrl (brldim *);		/* close braille display */
void writebrl (brldim *);		/* write to braille display */
int readbrl (int);		/* get key press from braille display */
void setbrlstat (const unsigned char *);	/* set status cells */

#endif /* !defined(_BRL_H) */
