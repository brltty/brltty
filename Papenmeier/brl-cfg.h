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

/*
 * Support for all Papenmeier Terminal + config file
 *   Heimo.Schön <heimo.schoen@gmx.at>
 *   August Hörandl <august.hoerandl@gmx.at>
 *
 * Papenmeier/brl-cfg.h
 *  some defines and the big config table
 */

 /* supported terminals:
   id   name                     help  x  y  stat frnt easy
    0, "BRAILLEX Compact 486", 	  '1', 40, 1,  0,  9,  0
    1, "BRAILLEX 2D Lite (plus)", '2', 40, 1, 13,  9,  0
    2, "BRAILLEX Compact/Tiny",	  '3', 40, 1,  0,  9,  0
    3, "BRAILLEX 2D Screen Soft", '4', 80, 1, 22, 13,  0
    6, "BRAILLEX IB 80 cr soft",  '5', 80, 1,  4,  9,  0
   64, "BRAILLEX EL 2D-40",	  '6', 40, 1, 13,  0,  1
   65, "BRAILLEX EL 2D-66",	  '7', 66, 1, 13,  0,  1
   66, "BRAILLEX EL-80",	  '8', 80, 1,  2,  0,  1
 */

#define STAT_empty     0
/* other STAT_* moved to ../brl.h */

#define OFFS_FRONT      0
#define OFFS_STAT    1000
#define OFFS_ROUTE   2000  
#define OFFS_EASY    3000
#define OFFS_SWITCH  4000
#define OFFS_CR      5000

#define ROUTINGKEY      -9999  /* virtual routing key */
#define NOKEY           -1

#define KEYMAX 8

#define  STATMAX   22
#define  FRONTMAX  13
#define  EASYMAX   8
#define  NAMEMAX   80
#define  MODMAX    (KEYMAX-1)
#define  CMDMAX    50

#define  HELPLEN   80

#define OFFS_HORIZ    1000	/* added to status show */
#define OFFS_FLAG     2000
#define OFFS_NUMBER   3000

/* easy bar - code as delivered by the terminals */
#define EASY_LE   1
#define EASY_LE2  2    
#define EASY_UP   3 
#define EASY_UP2  4
#define EASY_RI   5 
#define EASY_RI2  6  
#define EASY_DO   7 
#define EASY_DO2  8  

typedef struct {
  int code;			/* the code to sent */
  int keycode;			/* the key to press */
  unsigned char modifiers;	/* modifiers (bitfield) */
} commands;

typedef struct {
  unsigned char ident;		/* identity of terminal */
  char name[NAMEMAX];		/* name of terminal */
  char helpfile[HELPLEN];	/* filename of local helpfile */

  int x, y;			/* size of display */
  int statcells;		/* number of status cells */
  int frontkeys;		/* number of frontkeys */
  int haseasybar;		/* terminal has an easy bar */

  int statshow[STATMAX];	/* status cells: info to show */
  int modifiers[MODMAX];	/* keys used as modifier */
  commands cmds[CMDMAX];
} one_terminal; 

/* some makros for terminals with the same layout -
 * named after there usage
 */

 /* commands for 9 front keys */
#define MOD_FRONT_9 \
     OFFS_FRONT + 1, \
     OFFS_FRONT + 9, \
     OFFS_FRONT + 2, \
     OFFS_FRONT + 8

#define CMD_FRONT_9 \
     { CR_BEGBLKOFFSET,  ROUTINGKEY, 4 }, \
     { CMD_WINUP,   OFFS_FRONT +  3, 0 }, \
     { CMD_TOP,     OFFS_FRONT +  3, 1 }, \
     { CMD_TOP,     OFFS_FRONT +  3, 2 }, \
     { CMD_LNUP,    OFFS_FRONT +  4, 0 }, \
     { CMD_PRDIFLN, OFFS_FRONT +  4, 1 }, \
     { CMD_ATTRUP,  OFFS_FRONT +  4, 2 }, \
     { CMD_HWINLT,  OFFS_FRONT +  5, 1 }, \
     { CMD_HOME,    OFFS_FRONT +  5, 0 }, \
     { CMD_HWINRT,  OFFS_FRONT +  5, 2 }, \
     { CMD_LNDN,    OFFS_FRONT +  6, 0 }, \
     { CMD_NXDIFLN, OFFS_FRONT + 6, 1 },  \
     { CMD_ATTRDN,  OFFS_FRONT +  6, 2 }, \
     { CMD_WINDN,   OFFS_FRONT +  7, 0 }, \
     { CMD_BOT,     OFFS_FRONT +  7, 1 }, \
     { CMD_BOT,     OFFS_FRONT +  7, 2 }, \
     { CR_ENDBLKOFFSET,  ROUTINGKEY, 8 }, \
     { CR_ROUTEOFFSET,   ROUTINGKEY, 0 }


#define MOD_SWITCH    \
     OFFS_SWITCH + 1, \
     OFFS_SWITCH + 2, \
     OFFS_SWITCH + 7, \
     OFFS_SWITCH + 8

#define CMD_STAT_2 \
      { CMD_HELP,        OFFS_STAT +  1, 0 }, \
      { CMD_RESTARTBRL,  OFFS_STAT +  2, 0 }

/* commands for 13 status keys */
#define CMD_STAT_13 \
      { CMD_HELP,        OFFS_STAT +  1, 0 }, \
      { CMD_RESTARTBRL,  OFFS_STAT +  2, 0 }, \
      { CMD_CSRJMP_VERT, OFFS_STAT +  3, 0 }, \
      { CMD_BACK,        OFFS_STAT +  4, 0 }, \
      { CMD_CSRTRK,      OFFS_STAT +  5, 0 }, \
      { CMD_DISPMD,      OFFS_STAT +  6, 0 }, \
      { CMD_INFO,        OFFS_STAT +  7, 0 }, \
      { CMD_FREEZE,      OFFS_STAT +  8, 0 }, \
      { CMD_PREFMENU,    OFFS_STAT +  9, 0 }, \
      { CMD_PREFSAVE,    OFFS_STAT + 10, 0 }, \
      { CMD_CSRVIS,      OFFS_STAT + 11, 0 }, \
      { CMD_CSRSIZE,     OFFS_STAT + 12, 0 }, \
      { CMD_PASTE,       OFFS_STAT + 13, 0 }

/* easy bar + switches */
#define CMD_EASY \
     { CMD_LNUP,    OFFS_EASY + EASY_UP, 0 },  \
     { CMD_TOP,     OFFS_EASY + EASY_UP2, 0 }, \
     { CMD_LNDN,    OFFS_EASY + EASY_DO, 0 },  \
     { CMD_BOT,     OFFS_EASY + EASY_DO2, 0 }, \
     { CMD_PRDIFLN, OFFS_EASY + EASY_LE, 0 },  \
     { CMD_NXDIFLN, OFFS_EASY + EASY_RI, 0 },  \
     { CMD_HOME,    OFFS_EASY + EASY_LE2, 0 }, \
     { CMD_HOME,    OFFS_EASY + EASY_RI2, 0 }, \
     { CMD_PASTE,   OFFS_SWITCH + 6, 0 }, \
     { CR_BEGBLKOFFSET, ROUTINGKEY, 4 },     \
     { CR_ENDBLKOFFSET, ROUTINGKEY, 8 },  \
     { CR_ROUTEOFFSET,   ROUTINGKEY, 0 }

/* 13 status cells: info to show */
#define SHOW_STAT_13 \
      OFFS_HORIZ + STAT_current, \
      STAT_empty,                \
      OFFS_HORIZ + STAT_row,     \
      OFFS_HORIZ + STAT_col,     \
      OFFS_FLAG + STAT_tracking, \
      OFFS_FLAG + STAT_dispmode, \
      STAT_empty,                \
      OFFS_FLAG + STAT_frozen,   \
      STAT_empty,                \
      STAT_empty,                \
      OFFS_FLAG + STAT_visible,  \
      OFFS_FLAG + STAT_size,     \
      STAT_empty               

/* 22 status cells: info to show */
#define SHOW_STAT_22 \
      OFFS_HORIZ + STAT_current, \
      STAT_empty,                \
      OFFS_HORIZ + STAT_row,     \
      OFFS_HORIZ + STAT_col,     \
      STAT_empty,                \
      OFFS_FLAG + STAT_tracking, \
      OFFS_FLAG + STAT_dispmode, \
      STAT_empty,                \
      OFFS_FLAG + STAT_frozen,   \
      STAT_empty,                \
      STAT_empty,                \
      STAT_empty,                \
      OFFS_FLAG + STAT_visible,  \
      OFFS_FLAG + STAT_size,     \
      OFFS_FLAG + STAT_blink,    \
      OFFS_FLAG + STAT_capitalblink,   \
      OFFS_FLAG + STAT_dots,     \
      OFFS_FLAG + STAT_sound,    \
      OFFS_FLAG + STAT_skip,     \
      OFFS_FLAG + STAT_underline, \
      OFFS_FLAG + STAT_blinkattr, 

	/* commands for 13 front keys */
#define MOD_FRONT_13 \
     OFFS_FRONT + 1, \
     OFFS_FRONT + 13

#define CMD_FRONT_13 \
      { CR_BEGBLKOFFSET, ROUTINGKEY, 1 },   \
      { CMD_ATTRUP,   OFFS_FRONT +  2, 0 }, \
      { CMD_WINUP,    OFFS_FRONT +  3, 0 }, \
      { CMD_PRDIFLN,  OFFS_FRONT +  4, 0 }, \
      { CMD_LNUP,     OFFS_FRONT +  5, 0 }, \
      { CMD_TOP,      OFFS_FRONT +  6, 0 }, \
      { CMD_HOME,     OFFS_FRONT +  7, 0 }, \
      { CMD_BOT,      OFFS_FRONT +  8, 0 }, \
      { CMD_LNDN,     OFFS_FRONT +  9, 0 }, \
      { CMD_NXDIFLN,  OFFS_FRONT + 10, 0 }, \
      { CMD_WINDN,    OFFS_FRONT + 11, 0 }, \
      { CMD_ATTRDN,   OFFS_FRONT + 12, 0 }, \
      { CMD_PASTE, NOKEY, 3 }, \
      { CR_ENDBLKOFFSET, ROUTINGKEY, 2 },   \
      { CR_ROUTEOFFSET,  ROUTINGKEY, 0 }
	
	/* commands for 22 status keys */
#define CMD_STAT_22 \
      { CMD_HELP,        OFFS_STAT +  1, 0 }, \
      { CMD_RESTARTBRL,  OFFS_STAT +  2, 0 }, \
      { CMD_CSRJMP_VERT, OFFS_STAT +  3, 0 }, \
      { CMD_BACK,        OFFS_STAT +  4, 0 }, \
                                              \
      { CMD_CSRTRK,      OFFS_STAT +  6, 0 }, \
      { CMD_DISPMD,      OFFS_STAT +  7, 0 }, \
      { CMD_INFO,        OFFS_STAT +  8, 0 }, \
      { CMD_FREEZE,      OFFS_STAT +  9, 0 }, \
                                              \
      { CMD_PREFMENU,    OFFS_STAT + 10, 0 }, \
      { CMD_PREFSAVE,    OFFS_STAT + 11, 0 }, \
      { CMD_PREFLOAD,    OFFS_STAT + 12, 0 }, \
      { CMD_CSRVIS,      OFFS_STAT + 13, 0 }, \
      { CMD_CSRSIZE,     OFFS_STAT + 14, 0 }, \
                                              \
      { CMD_CSRBLINK,    OFFS_STAT + 15, 0 }, \
      { CMD_CAPBLINK,    OFFS_STAT + 16, 0 }, \
      { CMD_SIXDOTS,     OFFS_STAT + 17, 0 }, \
      { CMD_SND,         OFFS_STAT + 18, 0 }, \
      { CMD_SKPIDLNS,    OFFS_STAT + 19, 0 }, \
                                              \
      { CMD_ATTRVIS,     OFFS_STAT + 20, 0 }, \
      { CMD_ATTRBLINK,   OFFS_STAT + 21, 0 }, \
      { CMD_PASTE,       OFFS_STAT + 22, 0 }


static one_terminal pm_terminals[] =
{
  {
    0,				/* identity */
    "BrailleX Compact 486",	/* name of terminal */
    "brltty-pm-c-486.hlp",		/* filename of local helpfile */
    40, 1,			/* size of display */
    0,				/* number of status cells */
    9,				/* number of frontkeys */
    0,				/* terminal has an easy bar */
    {				/* status cells: info to show */
    },
    {				/* modifiers */
      MOD_FRONT_9
    },
    {				/* commands + keys */
      CMD_FRONT_9
    },
  },

  {
    1,				/* identity */
    "BrailleX 2D Lite (plus)",	/* name of terminal */
    "brltty-pm-2d-l.hlp",		/* filename of local helpfile */
    40, 1,			/* size of display */
    13,				/* number of status cells */
    9,				/* number of frontkeys */
    0,				/* terminal has an easy bar */
    {				/* status cells: info to show */
      SHOW_STAT_13,
    },
    {				/* modifiers */
      MOD_FRONT_9
    },
    {				/* commands + keys */
      CMD_FRONT_9,
      CMD_STAT_13
    },
  },

  {
    2,				/* identity */
    "BrailleX Compact/Tiny",	/* name of terminal */
    "brltty-pm-c.hlp",		/* filename of local helpfile */
    40, 1,			/* size of display */
    0,				/* number of status cells */
    9,				/* number of frontkeys */
    0,				/* terminal has an easy bar */
    {				/* status cells: info to show */
    },
    {				/* modifiers */
      MOD_FRONT_9
    },
    {				/* commands + keys */
      CMD_FRONT_9
    },
  },

  {
    3,				/* identity */
    "BrailleX 2D Screen Soft", /* name of terminal */
    "brltty-pm-2d-s.hlp",		/* filename of local helpfile */
    80, 1,			/* size of display */
    22,				/* number of status cells */
    13,				/* number of frontkeys */
    0,				/* terminal has an easy bar */
    { /* 22 */			/* status cells: info to show */
      SHOW_STAT_22
    },
    {				/* modifiers */
      MOD_FRONT_13
    },
    {				/* commands + keys */
      CMD_FRONT_13,
      CMD_STAT_22
    },
  },

  {
    6,				/* identity */
    "BrailleX IB 80 CR Soft",	/* name of terminal */
    "brltty-pm-ib-80.hlp",		/* filename of local helpfile */
    80, 1,			/* size of display */
    4,				/* number of status cells */
    9,				/* number of frontkeys */
    0,				/* terminal has an easy bar */
    { /* 4 */			/* status cells: info to show */
      OFFS_HORIZ + STAT_current,
      OFFS_HORIZ + STAT_row,
      OFFS_HORIZ + STAT_col,
      OFFS_FLAG + STAT_dispmode,
    },
    {				/* modifiers */
      MOD_FRONT_9
    },
    {				/* commands + keys */
      CMD_FRONT_9
    },
  },

  {
    64,				/* identity */
    "BrailleX EL 2D-40",	/* name of terminal */
    "brltty-pm-el-2d-40.hlp",		/* filename of local helpfile */
    40, 1,			/* size of display */
    13,				/* number of status cells */
    0,				/* number of frontkeys */
    1,				/* terminal has an easy bar */
    { /* 13 */			
      SHOW_STAT_13
    },
    {				/* modifiers */
      MOD_SWITCH
    },
    {				/* commands + keys */
      CMD_STAT_13,
      CMD_EASY
    },
  },

  {
    65,				/* identity */
    "BrailleX EL 2D-66",	/* name of terminal */
    "brltty-pm-el-2d-66.hlp",		/* filename of local helpfile */
    66, 1,			/* size of display */
    13,				/* number of status cells */
    0,				/* number of frontkeys */
    1,				/* terminal has an easy bar */
    {				/* status cells: info to show */
      SHOW_STAT_13
    },
    {				/* modifiers */
      MOD_SWITCH
    },
    {				/* commands + keys */
      CMD_STAT_13,
      CMD_EASY
    },
  },
  {
    66,				/* identity */
    "BrailleX EL-80",		/* name of terminal */
    "brltty-pm-el-80.hlp",		/* filename of local helpfile */
    80, 1,			/* size of display */
    2,				/* number of status cells */
    0,				/* number of frontkeys */
    1,				/* terminal has an easy bar */
    {				/* status cells: info to show */
      OFFS_NUMBER + STAT_current,
      OFFS_NUMBER + STAT_row,
    },
    {				/* modifiers */
      MOD_SWITCH
    },
    {				/* commands + keys */
      CMD_EASY,
      CMD_STAT_2
    },
  },
};

static const int num_terminals = sizeof(pm_terminals)/sizeof(pm_terminals[0]);
