/*
 * BrlTty - A daemon providing access to the Linux console (when in text
 *          mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
 *
 * BrlTty comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/*
 * struct definition of settings that are saveable.
 * ENV_MAGICNUM has to be bumped whenever the definition of
 * struct brltty_env is modified otherwise this structure could be
 * filled with incompatible data from disk.
 */
#define ENV_MAGICNUM 0x4005

struct brltty_env {
	short magicnum;
	short csrvis;
	short attrvis;
	short csrblink;
	short capblink;
	short attrblink;
	short csrsize;
	short csroncnt;
	short csroffcnt;
	short caponcnt;
	short capoffcnt;
	short attroncnt;
	short attroffcnt;
	short sixdots;
	short slidewin;
	short sound;
	short skpidlns;
	short skpblnkeol;
	short skpblnkwins;
	short stcellstyle;
};

/* 
 * struct definition for volatile parameters
 */
struct brltty_param {
	short csrtrk;		/* tracking mode */
	short csrhide;		/* For temporarily hiding cursor */
	short dispmode;		/* text or attributes display */
	short winx, winy;	/* Start row and column of Braille window */
	short cox, coy;		/* Old cursor position */
};


#define TBL_TEXT 0		/* use text translation table */
#define TBL_ATTRIB 1		/* use attribute translation table */


/*
 * Shared variables
 */

extern char VERSION[];			/* BrlTty version string */
extern char COPYRIGHT[];		/* BrlTty copyright banner */
extern short opt_q;			/* quiet mode */
extern volatile int keep_going;		/* zero after reception of SIGTERM */

extern struct brltty_param initparam;	/* defaults for new brltty_param */
extern struct brltty_env env;		/* current env parameters */
extern brldim brl;			/* braille driver reference */

extern short fwinshift;			/* Full window horizontal distance */
extern short hwinshift;			/* Half window horizontal distance */
extern short vwinshift;			/* Window vertical distance */
extern short csr_offright;		/* used for sliding window */

extern unsigned char texttrans[256];	/* current text to braille translation table */
extern unsigned char untexttrans[256];	/* current braille to text translation table */
extern unsigned char statcells[22];	/* status cell buffer */


/*
 * Shared functions
 */

void startup(int argc, char *argv[]);
void startbrl(void);
void loadconfig(void);
void saveconfig(void);
void configmenu(void);
void clrbrlstat(void);

