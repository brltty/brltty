/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#include <inttypes.h>

#include "Programs/brl.h"
typedef enum {
  CMD_INPUT = DriverCommandCount /* toggle input mode */,
  CMD_SWSIM_LC /* simulate left switch centred */,
  CMD_SWSIM_LR /* simulate left switch rear */,
  CMD_SWSIM_LF /* simulate left switch front */,
  CMD_SWSIM_RC /* simulate right switch centred */,
  CMD_SWSIM_RR /* simulate right switch rear */,
  CMD_SWSIM_RF /* simulate right switch front */,
  CMD_SWSIM_BC /* simulate both switches centred */,
  CMD_SWSIM_BQ /* show positions of both switches */,
  CMD_NODOTS = VAL_PASSDOTS /* input character corresponding to no braille dots */
} InternalDriverCommands;

typedef enum {
  STAT_INPUT = StatusCellCount /* input mode */,
  InternalStatusCellCount
} InternalStatusCell;

#define OFFS_FRONT      0
#define OFFS_STAT    1000
#define OFFS_ROUTE   2000  
#define OFFS_EASY    3000
#define OFFS_SWITCH  4000
#define OFFS_CR      5000

#define ROUTINGKEY      -9999  /* virtual routing key */
#define NOKEY           -1

#define MODMAX    16
#define KEYMAX     8

#define OFFS_EMPTY       0
#define OFFS_HORIZ    1000	/* added to status show */
#define OFFS_FLAG     2000
#define OFFS_NUMBER   3000

/* easy bar - code as delivered by the terminals */
#define EASY_L1 1
#define EASY_L2 2    
#define EASY_U1 3 
#define EASY_U2 4
#define EASY_R1 5 
#define EASY_R2 6  
#define EASY_D1 7 
#define EASY_D2 8  

#define SWITCH_LEFT_REAR   1
#define SWITCH_LEFT_FRONT  2
#define KEY_LEFT_REAR      3
#define KEY_LEFT_FRONT     4
#define KEY_RIGHT_REAR     5
#define KEY_RIGHT_FRONT    6
#define SWITCH_RIGHT_REAR  7
#define SWITCH_RIGHT_FRONT 8

typedef struct {
  int code;		/* the code to sent */
  int16_t key;	/* the key to press */
  uint16_t modifiers;	/* modifiers (bitfield) */
} CommandDefinition;

typedef struct {
  unsigned char identifier;		/* identity of terminal */
  char *name;		/* name of terminal */
  char *helpFile;		/* filename of local helpfile */

  uint8_t columns;		/* width of display */
  uint8_t rows;			/* height of display */
  uint8_t frontKeys;		/* number of front keys */
  uint8_t hasEasyBar;		/* has an easy access bar */
  uint8_t leftSwitches;		/* number of switches on the left side */
  uint8_t rightSwitches;		/* number of switches on the right side */
  uint8_t leftKeys;		/* number of keys on the left side */
  uint8_t rightKeys;		/* number of keys on the right side */
  uint8_t statusCount;		/* number of status cells */
  uint8_t modifierCount;		/* number of modifier keys */
  uint16_t commandCount;		/* number of commands */

  uint16_t *statusCells;	/* status cells: info to show */
  int16_t *modifiers;	/* keys used as modifier */
  CommandDefinition *commands;
} TerminalDefinition; 

#define PM_COUNT(array) (sizeof((array)) / sizeof((array)[0]))
#define PM_TERMINAL(identifier, signature, name, columns, rows, front, eab, ls, rs, lk, rk) \
{ \
  identifier, name, "brltty-pm-" #signature ".hlp", \
  columns, rows, front, eab, ls, rs, lk, rk, \
  PM_COUNT(pm_status_##signature), \
  PM_COUNT(pm_modifiers_##signature), \
  PM_COUNT(pm_commands_##signature), \
  pm_status_##signature, \
  pm_modifiers_##signature, \
  pm_commands_##signature \
}

/* some macros for terminals with the same layout -
 * named after there usage
 */

#define CHGONOFF(cmd, offs, on, off) \
      { cmd                 , offs, 0  }, \
      { cmd | VAL_TOGGLE_OFF, offs, off}, \
      { cmd | VAL_TOGGLE_ON , offs, on }


/* modifiers for 9 front keys */
#define MOD_FRONT_9 \
     OFFS_FRONT + 1, \
     OFFS_FRONT + 9, \
     OFFS_FRONT + 2, \
     OFFS_FRONT + 8

/* commands for 9 front keys */
#define CMDS_FRONT_9 \
     { CMD_FWINLT     , NOKEY         , 0X1 }, \
     { CMD_FWINRT     , NOKEY         , 0X2 }, \
     { CMD_HWINLT     , NOKEY         , 0X4 }, \
     { CMD_HWINRT     , NOKEY         , 0X8 }, \
                                               \
     { CMD_HOME       , OFFS_FRONT + 5, 0X0 }, \
     { CMD_LNBEG      , OFFS_FRONT + 5, 0X1 }, \
     { CMD_LNEND      , OFFS_FRONT + 5, 0X2 }, \
     { CMD_CHRLT      , OFFS_FRONT + 5, 0X4 }, \
     { CMD_CHRRT      , OFFS_FRONT + 5, 0X8 }, \
                                               \
     { CMD_WINUP      , OFFS_FRONT + 4, 0X0 }, \
     { CMD_PRDIFLN    , OFFS_FRONT + 4, 0X1 }, \
     { CMD_ATTRUP     , OFFS_FRONT + 4, 0X2 }, \
     { CMD_PRPGRPH    , OFFS_FRONT + 4, 0X4 }, \
     { CMD_PRSEARCH   , OFFS_FRONT + 4, 0X8 }, \
                                               \
     { CMD_WINDN      , OFFS_FRONT + 6, 0X0 }, \
     { CMD_NXDIFLN    , OFFS_FRONT + 6, 0X1 }, \
     { CMD_ATTRDN     , OFFS_FRONT + 6, 0X2 }, \
     { CMD_NXPGRPH    , OFFS_FRONT + 6, 0X4 }, \
     { CMD_NXSEARCH   , OFFS_FRONT + 6, 0X8 }, \
                                               \
     { CMD_LNUP       , OFFS_FRONT + 3, 0X0 }, \
     { CMD_TOP_LEFT   , OFFS_FRONT + 3, 0X1 }, \
     { CMD_TOP        , OFFS_FRONT + 3, 0X2 }, \
                                               \
     { CMD_LNDN       , OFFS_FRONT + 7, 0X0 }, \
     { CMD_BOT_LEFT   , OFFS_FRONT + 7, 0X1 }, \
     { CMD_BOT        , OFFS_FRONT + 7, 0X2 }, \
                                               \
     { CR_ROUTE       , ROUTINGKEY    , 0X0 }, \
     { CR_CUTBEGIN    , ROUTINGKEY    , 0X1 }, \
     { CR_CUTRECT     , ROUTINGKEY    , 0X2 }, \
     { CR_PRINDENT    , ROUTINGKEY    , 0X4 }, \
     { CR_NXINDENT    , ROUTINGKEY    , 0X8 }, \
     { CMD_PASTE      , NOKEY         , 0X3 }


/* modifiers for 13 front keys */
#define MOD_FRONT_13  \
     OFFS_FRONT +  4, \
     OFFS_FRONT +  3, \
     OFFS_FRONT +  2, \
     OFFS_FRONT + 10, \
     OFFS_FRONT + 11, \
     OFFS_FRONT + 12, \
     OFFS_FRONT +  1, \
     OFFS_FRONT + 13

/* commands for 13 front keys */
#define CMDS_FRONT_13 \
      { CMD_HOME                    , OFFS_FRONT +  7, 0000 }, \
      { CMD_TOP                     , OFFS_FRONT +  6, 0000 }, \
      { CMD_BOT                     , OFFS_FRONT +  8, 0000 }, \
      { CMD_LNUP                    , OFFS_FRONT +  5, 0000 }, \
      { CMD_LNDN                    , OFFS_FRONT +  9, 0000 }, \
                                                               \
      { CMD_PRDIFLN                 , NOKEY          , 0001 }, \
      { CMD_NXDIFLN                 , NOKEY          , 0010 }, \
      { CMD_ATTRUP                  , NOKEY          , 0002 }, \
      { CMD_ATTRDN                  , NOKEY          , 0020 }, \
      { CMD_PRPGRPH                 , NOKEY          , 0004 },         \
      { CMD_NXPGRPH                 , NOKEY          , 0040 },        \
      { CMD_PRPROMPT                , NOKEY          , 0100 },        \
      { CMD_NXPROMPT                , NOKEY          , 0200 },        \
                                                               \
      { CMD_WINUP                   , NOKEY          , 0003 },        \
      { CMD_WINDN                   , NOKEY          , 0030 },        \
      { CMD_PRSEARCH                , NOKEY          , 0104 },        \
      { CMD_NXSEARCH                , NOKEY          , 0240 },        \
      { CR_PRINDENT                 , ROUTINGKEY     , 0104 },        \
      { CR_NXINDENT                 , ROUTINGKEY     , 0240 },        \
                                                               \
      { CMD_LNBEG                   , OFFS_FRONT +  7, 0001 }, \
      { CMD_TOP_LEFT                , OFFS_FRONT +  6, 0001 }, \
      { CMD_BOT_LEFT                , OFFS_FRONT +  8, 0001 }, \
      { CMD_FWINLT                  , OFFS_FRONT +  5, 0001 }, \
      { CMD_FWINRT                  , OFFS_FRONT +  9, 0001 }, \
      { CR_DESCCHAR                 , ROUTINGKEY     , 0001 }, \
                                                               \
      { CMD_LNEND                   , OFFS_FRONT +  7, 0010 }, \
      { CMD_CHRLT                   , OFFS_FRONT +  6, 0010 }, \
      { CMD_CHRRT                   , OFFS_FRONT +  8, 0010 }, \
      { CMD_HWINLT                  , OFFS_FRONT +  5, 0010 }, \
      { CMD_HWINRT                  , OFFS_FRONT +  9, 0010 }, \
      { CR_SETLEFT                  , ROUTINGKEY     , 0010 }, \
                                                               \
      { VAL_PASSKEY+VPK_INSERT      , OFFS_FRONT +  7, 0002 }, \
      { VAL_PASSKEY+VPK_PAGE_UP     , OFFS_FRONT +  6, 0002 }, \
      { VAL_PASSKEY+VPK_PAGE_DOWN   , OFFS_FRONT +  8, 0002 }, \
      { VAL_PASSKEY+VPK_CURSOR_UP   , OFFS_FRONT +  5, 0002 }, \
      { VAL_PASSKEY+VPK_CURSOR_DOWN , OFFS_FRONT +  9, 0002 }, \
      { CR_SWITCHVT                 , ROUTINGKEY     , 0002 }, \
                                                               \
      { VAL_PASSKEY+VPK_DELETE      , OFFS_FRONT +  7, 0020 }, \
      { VAL_PASSKEY+VPK_HOME        , OFFS_FRONT +  6, 0020 }, \
      { VAL_PASSKEY+VPK_END         , OFFS_FRONT +  8, 0020 }, \
      { VAL_PASSKEY+VPK_CURSOR_LEFT , OFFS_FRONT +  5, 0020 }, \
      { VAL_PASSKEY+VPK_CURSOR_RIGHT, OFFS_FRONT +  9, 0020 }, \
      { VAL_PASSKEY+VPK_FUNCTION    , ROUTINGKEY     , 0020 }, \
                                                               \
      { CMD_NODOTS                  , OFFS_FRONT +  7, 0004 }, \
      { VAL_PASSKEY+VPK_ESCAPE      , OFFS_FRONT +  6, 0004 }, \
      { VAL_PASSKEY+VPK_TAB         , OFFS_FRONT +  8, 0004 }, \
      { VAL_PASSKEY+VPK_BACKSPACE   , OFFS_FRONT +  5, 0004 }, \
      { VAL_PASSKEY+VPK_RETURN      , OFFS_FRONT +  9, 0004 }, \
                                                               \
      { CMD_SPKHOME                 , OFFS_FRONT +  7, 0100 }, \
      { CMD_SAY_ABOVE               , OFFS_FRONT +  6, 0100 }, \
      { CMD_SAY_BELOW               , OFFS_FRONT +  8, 0100 }, \
      { CMD_MUTE                    , OFFS_FRONT +  5, 0100 }, \
      { CMD_SAY_LINE                , OFFS_FRONT +  9, 0100 }, \
                                                               \
      { CMD_RESTARTSPEECH           , OFFS_FRONT +  7, 0200 }, \
      { CMD_SAY_SLOWER              , OFFS_FRONT +  6, 0200 }, \
      { CMD_SAY_FASTER              , OFFS_FRONT +  8, 0200 }, \
      { CMD_SAY_SOFTER              , OFFS_FRONT +  5, 0200 }, \
      { CMD_SAY_LOUDER              , OFFS_FRONT +  9, 0200 }, \
                                                               \
      { CR_CUTBEGIN                 , ROUTINGKEY     , 0100 }, \
      { CR_CUTAPPEND                , ROUTINGKEY     , 0004 }, \
      { CR_CUTLINE                  , ROUTINGKEY     , 0040 }, \
      { CR_CUTRECT                  , ROUTINGKEY     , 0200 }, \
                                                               \
      { CR_ROUTE                    , ROUTINGKEY     , 0000 }
	

/* modifiers for switches */
#define MOD_EASY \
     OFFS_SWITCH + SWITCH_LEFT_REAR  , \
     OFFS_SWITCH + SWITCH_LEFT_FRONT , \
     OFFS_SWITCH + SWITCH_RIGHT_REAR , \
     OFFS_SWITCH + SWITCH_RIGHT_FRONT, \
     OFFS_SWITCH + KEY_LEFT_REAR     , \
     OFFS_SWITCH + KEY_LEFT_FRONT    , \
     OFFS_SWITCH + KEY_RIGHT_REAR    , \
     OFFS_SWITCH + KEY_RIGHT_FRONT   , \
     OFFS_EASY   + EASY_U1,            \
     OFFS_EASY   + EASY_D1,            \
     OFFS_EASY   + EASY_L1,            \
     OFFS_EASY   + EASY_R1,            \
     OFFS_EASY   + EASY_U2,            \
     OFFS_EASY   + EASY_D2,            \
     OFFS_EASY   + EASY_L2,            \
     OFFS_EASY   + EASY_R2

/* commands for easy bar + switches */
#define CMDS_EASY_X(dir, key, swt, cmd1, cmd2) \
  {cmd1, key, swt|(dir<<0X8)}, \
  {cmd2, key, swt|(dir<<0XC)}
#define CMDS_EASY_U(key, swt, cmd1, cmd2) CMDS_EASY_X(0X1, key, swt, cmd1, cmd2)
#define CMDS_EASY_D(key, swt, cmd1, cmd2) CMDS_EASY_X(0X2, key, swt, cmd1, cmd2)
#define CMDS_EASY_L(key, swt, cmd1, cmd2) CMDS_EASY_X(0X4, key, swt, cmd1, cmd2)
#define CMDS_EASY_R(key, swt, cmd1, cmd2) CMDS_EASY_X(0X8, key, swt, cmd1, cmd2)
#define CMDS_EASY_K(key, swt, u1, d1, u2, d2, l1, r1, l2, r2) \
  CMDS_EASY_U(key, swt, u1, u2), \
  CMDS_EASY_D(key, swt, d1, d2), \
  CMDS_EASY_L(key, swt, l1, l2), \
  CMDS_EASY_R(key, swt, r1, r2)
#define CMDS_EASY_C(swt) \
  {CMD_BACK        , NOKEY, 0X10|swt}, \
  {CMD_HOME        , NOKEY, 0X20|swt}, \
  {CMD_HELP        , NOKEY, 0X40|swt}, \
  {CMD_LEARN       , NOKEY, 0X80|swt}, \
  CMDS_EASY_K(NOKEY,        0X10|swt, \
              CMD_SIXDOTS, CMD_PASTE, CMD_CAPBLINK, CMD_CSRJMP_VERT, \
              CMD_DISPMD, CMD_CSRTRK, CMD_ATTRVIS, CMD_CSRVIS), \
  CMDS_EASY_K(NOKEY,        0X20|swt, \
              CMD_AUTOSPEAK, CMD_AUTOREPEAT, CMD_RESTARTBRL, CMD_FREEZE, \
              CMD_INFO, CMD_PREFMENU, CMD_PREFLOAD, CMD_PREFSAVE), \
  CMDS_EASY_K(NOKEY,        0X40|swt, \
              CMD_SAY_ABOVE, CMD_SAY_BELOW, CMD_SAY_LOUDER, CMD_SAY_SOFTER, \
              CMD_MUTE, CMD_SAY_LINE, CMD_SAY_SLOWER, CMD_SAY_FASTER), \
  {CR_ROUTE        , ROUTINGKEY, swt}, \
  CMDS_EASY_K(ROUTINGKEY,        swt, \
              CR_PRINDENT, CR_NXINDENT, CR_SETLEFT, CR_DESCCHAR, \
              CR_CUTAPPEND, CR_CUTLINE, CR_CUTBEGIN, CR_CUTRECT)
#define CMDS_EASY \
  CMDS_EASY_K(NOKEY, 0X00, \
              CMD_LNUP, CMD_LNDN, CMD_TOP, CMD_BOT, \
              CMD_FWINLT, CMD_FWINRT, CMD_LNBEG, CMD_LNEND), \
  CMDS_EASY_C(0X00), \
  CMDS_EASY_K(NOKEY, 0X01, \
              CMD_PRDIFLN, CMD_NXDIFLN, CMD_ATTRUP, CMD_ATTRDN, \
              CMD_PRPROMPT, CMD_NXPROMPT, CMD_PRPGRPH, CMD_NXPGRPH), \
  CMDS_EASY_C(0X01), \
  CMDS_EASY_K(NOKEY, 0X04, \
              VAL_PASSKEY+VPK_CURSOR_UP, VAL_PASSKEY+VPK_CURSOR_DOWN, VAL_PASSKEY+VPK_PAGE_UP, VAL_PASSKEY+VPK_PAGE_DOWN, \
              CMD_FWINLT, CMD_FWINRT, CMD_LNBEG, CMD_LNEND), \
  CMDS_EASY_C(0X04), \
  CMDS_EASY_K(NOKEY, 0X05, \
              VAL_PASSKEY+VPK_CURSOR_UP, VAL_PASSKEY+VPK_CURSOR_DOWN, VAL_PASSKEY+VPK_PAGE_UP, VAL_PASSKEY+VPK_PAGE_DOWN, \
              VAL_PASSKEY+VPK_CURSOR_LEFT, VAL_PASSKEY+VPK_CURSOR_RIGHT, VAL_PASSKEY+VPK_HOME, VAL_PASSKEY+VPK_END), \
  CMDS_EASY_C(0X05), \
  CMDS_EASY_K(NOKEY, 0X02, \
              CMD_PRSEARCH, CMD_NXSEARCH, CMD_HELP, CMD_LEARN, \
              CMD_CHRLT, CMD_CHRRT, CMD_HWINLT, CMD_HWINRT), \
  CMDS_EASY_C(0X02), \
  CMDS_EASY_K(NOKEY, 0X08, \
              CMD_MENU_PREV_ITEM, CMD_MENU_NEXT_ITEM, CMD_MENU_FIRST_ITEM, CMD_MENU_LAST_ITEM, \
              CMD_MENU_PREV_SETTING, CMD_MENU_NEXT_SETTING, CMD_PREFLOAD, CMD_PREFSAVE)
#define CMDS_EASY_SWSIM \
  CMDS_EASY_K(NOKEY, 0XA0, \
              CMD_SWSIM_LC, CMD_SWSIM_RC, CMD_SWSIM_BQ, CMD_SWSIM_BC, \
              CMD_SWSIM_LF, CMD_SWSIM_RF, CMD_SWSIM_LR, CMD_SWSIM_RR)


/* what to show for 2 status cells */
#define SHOW_STAT_2 \
      OFFS_NUMBER + STAT_BRLROW, \
      OFFS_NUMBER + STAT_CSRROW

/* commands for 2 status keys */
#define CMDS_STAT_2 \
      { CMD_HELP , OFFS_STAT + 1, 0 }, \
      { CMD_LEARN, OFFS_STAT + 2, 0 }


/* what to show for 4 status cells */
#define SHOW_STAT_4 \
      OFFS_NUMBER + STAT_BRLROW, \
      OFFS_NUMBER + STAT_CSRROW, \
      OFFS_NUMBER + STAT_CSRCOL, \
      OFFS_FLAG   + STAT_DISPMD

/* commands for 4 status keys */
#define CMDS_STAT_4 \
      { CMD_HELP       , OFFS_STAT + 1, 0 }, \
      { CMD_LEARN      , OFFS_STAT + 2, 0 }, \
      { CMD_CSRJMP_VERT, OFFS_STAT + 3, 0 }, \
      { CMD_DISPMD     , OFFS_STAT + 4, 0 }


/* what to show for 13 status cells */
#define SHOW_STAT_13 \
      OFFS_HORIZ + STAT_BRLROW  , \
      OFFS_EMPTY                , \
      OFFS_HORIZ + STAT_CSRROW  , \
      OFFS_HORIZ + STAT_CSRCOL  , \
      OFFS_EMPTY                , \
      OFFS_FLAG  + STAT_CSRTRK  , \
      OFFS_FLAG  + STAT_DISPMD  , \
      OFFS_FLAG  + STAT_FREEZE  , \
      OFFS_EMPTY                , \
      OFFS_EMPTY                , \
      OFFS_FLAG  + STAT_CSRVIS  , \
      OFFS_FLAG  + STAT_ATTRVIS , \
      OFFS_EMPTY

/* commands for 13 status keys */
#define CMDS_STAT_13(on, off) \
      CHGONOFF( CMD_HELP       , OFFS_STAT +  1, on, off), \
              { CMD_LEARN      , OFFS_STAT +  2, 0      }, \
              { CMD_CSRJMP_VERT, OFFS_STAT +  3, 0      }, \
              { CMD_BACK       , OFFS_STAT +  4, 0      }, \
      CHGONOFF( CMD_INFO       , OFFS_STAT +  5, on, off), \
      CHGONOFF( CMD_CSRTRK     , OFFS_STAT +  6, on, off), \
      CHGONOFF( CMD_DISPMD     , OFFS_STAT +  7, on, off), \
      CHGONOFF( CMD_FREEZE     , OFFS_STAT +  8, on, off), \
              { CMD_PREFMENU   , OFFS_STAT +  9, 0      }, \
              { CMD_PREFLOAD   , OFFS_STAT + 10, 0      }, \
      CHGONOFF( CMD_CSRVIS     , OFFS_STAT + 11, on, off), \
      CHGONOFF( CMD_ATTRVIS    , OFFS_STAT + 12, on, off), \
              { CMD_PASTE      , OFFS_STAT + 13, 0      }


/* what to show for 20 status cells */
#define SHOW_STAT_20 \
      OFFS_HORIZ + STAT_BRLROW    , \
      OFFS_EMPTY                  , \
      OFFS_HORIZ + STAT_CSRROW    , \
      OFFS_HORIZ + STAT_CSRCOL    , \
      OFFS_EMPTY                  , \
      OFFS_FLAG  + STAT_CSRTRK    , \
      OFFS_FLAG  + STAT_DISPMD    , \
      OFFS_FLAG  + STAT_FREEZE    , \
      OFFS_EMPTY                  , \
      OFFS_HORIZ + STAT_SCRNUM    , \
      OFFS_EMPTY                  , \
      OFFS_FLAG  + STAT_CSRVIS    , \
      OFFS_FLAG  + STAT_ATTRVIS   , \
      OFFS_FLAG  + STAT_CAPBLINK  , \
      OFFS_FLAG  + STAT_SIXDOTS   , \
      OFFS_FLAG  + STAT_SKPIDLNS  , \
      OFFS_FLAG  + STAT_TUNES     , \
      OFFS_FLAG  + STAT_AUTOSPEAK , \
      OFFS_FLAG  + STAT_AUTOREPEAT, \
      OFFS_EMPTY

/* commands for 20 status keys */
#define CMDS_STAT_20(on, off) \
      CHGONOFF( CMD_HELP       , OFFS_STAT +  1, on, off ), \
              { CMD_LEARN      , OFFS_STAT +  2, 0X0000  }, \
              { CMD_CSRJMP_VERT, OFFS_STAT +  3, 0X0000  }, \
              { CMD_BACK       , OFFS_STAT +  4, 0X0000  }, \
      CHGONOFF( CMD_INFO       , OFFS_STAT +  5, on, off ), \
      CHGONOFF( CMD_CSRTRK     , OFFS_STAT +  6, on, off ), \
      CHGONOFF( CMD_DISPMD     , OFFS_STAT +  7, on, off ), \
      CHGONOFF( CMD_FREEZE     , OFFS_STAT +  8, on, off ), \
              { CMD_PREFMENU   , OFFS_STAT +  9, 0X0000  }, \
              { CMD_PREFSAVE   , OFFS_STAT + 10, 0X0000  }, \
              { CMD_PREFLOAD   , OFFS_STAT + 11, 0X0000  }, \
      CHGONOFF( CMD_CSRVIS     , OFFS_STAT + 12, on, off ), \
      CHGONOFF( CMD_ATTRVIS    , OFFS_STAT + 13, on, off ), \
      CHGONOFF( CMD_CAPBLINK   , OFFS_STAT + 14, on, off ), \
      CHGONOFF( CMD_SIXDOTS    , OFFS_STAT + 15, on, off ), \
      CHGONOFF( CMD_SKPIDLNS   , OFFS_STAT + 16, on, off ), \
      CHGONOFF( CMD_TUNES      , OFFS_STAT + 17, on, off ), \
      CHGONOFF( CMD_AUTOSPEAK  , OFFS_STAT + 18, on, off ), \
      CHGONOFF( CMD_AUTOREPEAT , OFFS_STAT + 19, on, off ), \
              { CMD_PASTE      , OFFS_STAT + 20, 0X0000  }


/* what to show for 22 status cells */
#define SHOW_STAT_22 \
      OFFS_HORIZ + STAT_BRLROW    , \
      OFFS_EMPTY                  , \
      OFFS_HORIZ + STAT_CSRROW    , \
      OFFS_HORIZ + STAT_CSRCOL    , \
      OFFS_EMPTY                  , \
      OFFS_FLAG  + STAT_CSRTRK    , \
      OFFS_FLAG  + STAT_DISPMD    , \
      OFFS_FLAG  + STAT_FREEZE    , \
      OFFS_EMPTY                  , \
      OFFS_HORIZ + STAT_SCRNUM    , \
      OFFS_EMPTY                  , \
      OFFS_FLAG  + STAT_CSRVIS    , \
      OFFS_FLAG  + STAT_ATTRVIS   , \
      OFFS_FLAG  + STAT_CAPBLINK  , \
      OFFS_FLAG  + STAT_SIXDOTS   , \
      OFFS_FLAG  + STAT_SKPIDLNS  , \
      OFFS_FLAG  + STAT_TUNES     , \
      OFFS_EMPTY                  , \
      OFFS_FLAG  + STAT_INPUT     , \
      OFFS_FLAG  + STAT_AUTOSPEAK , \
      OFFS_FLAG  + STAT_AUTOREPEAT, \
      OFFS_EMPTY

/* commands for 22 status keys */
#define CMDS_STAT_22(on, off) \
      CHGONOFF( CMD_HELP       , OFFS_STAT +  1, on, off ), \
              { CMD_LEARN      , OFFS_STAT +  2, 0X0000  }, \
              { CMD_CSRJMP_VERT, OFFS_STAT +  3, 0X0000  }, \
              { CMD_BACK       , OFFS_STAT +  4, 0X0000  }, \
      CHGONOFF( CMD_INFO       , OFFS_STAT +  5, on, off ), \
      CHGONOFF( CMD_CSRTRK     , OFFS_STAT +  6, on, off ), \
      CHGONOFF( CMD_DISPMD     , OFFS_STAT +  7, on, off ), \
      CHGONOFF( CMD_FREEZE     , OFFS_STAT +  8, on, off ), \
              { CMD_PREFMENU   , OFFS_STAT +  9, 0X0000  }, \
              { CMD_PREFSAVE   , OFFS_STAT + 10, 0X0000  }, \
              { CMD_PREFLOAD   , OFFS_STAT + 11, 0X0000  }, \
      CHGONOFF( CMD_CSRVIS     , OFFS_STAT + 12, on, off ), \
      CHGONOFF( CMD_ATTRVIS    , OFFS_STAT + 13, on, off ), \
      CHGONOFF( CMD_CAPBLINK   , OFFS_STAT + 14, on, off ), \
      CHGONOFF( CMD_SIXDOTS    , OFFS_STAT + 15, on, off ), \
      CHGONOFF( CMD_SKPIDLNS   , OFFS_STAT + 16, on, off ), \
      CHGONOFF( CMD_TUNES      , OFFS_STAT + 17, on, off ), \
              { CMD_RESTARTBRL , OFFS_STAT + 18, 0X0000  }, \
      CHGONOFF( CMD_INPUT      , OFFS_STAT + 19, on, off ), \
      CHGONOFF( CMD_AUTOSPEAK  , OFFS_STAT + 20, on, off ), \
      CHGONOFF( CMD_AUTOREPEAT , OFFS_STAT + 21, on, off ), \
              { CMD_PASTE      , OFFS_STAT + 22, 0X0000  }

static uint16_t pm_status_c_486[] = {
};
static int16_t pm_modifiers_c_486[] = {
  MOD_FRONT_9
};
static CommandDefinition pm_commands_c_486[] = {
  CMDS_FRONT_9
};

static uint16_t pm_status_2d_l[] = {
  SHOW_STAT_13
};
static int16_t pm_modifiers_2d_l[] = {
  MOD_FRONT_9
};
static CommandDefinition pm_commands_2d_l[] = {
  CMDS_FRONT_9,
  CMDS_STAT_13(0X2, 0X1)
};

static uint16_t pm_status_c[] = {
};
static int16_t pm_modifiers_c[] = {
  MOD_FRONT_9
};
static CommandDefinition pm_commands_c[] = {
  CMDS_FRONT_9
};

static uint16_t pm_status_2d_s[] = {
  SHOW_STAT_22
};
static int16_t pm_modifiers_2d_s[] = {
  MOD_FRONT_13
};
static CommandDefinition pm_commands_2d_s[] = {
  CMDS_FRONT_13,
  CMDS_STAT_22(0X80, 0X40)
};

static uint16_t pm_status_ib_80[] = {
  SHOW_STAT_4
};
static int16_t pm_modifiers_ib_80[] = {
  MOD_FRONT_9
};
static CommandDefinition pm_commands_ib_80[] = {
  CMDS_FRONT_9,
  CMDS_STAT_4
};

static uint16_t pm_status_el_2d_40[] = {
  SHOW_STAT_13
};
static int16_t pm_modifiers_el_2d_40[] = {
  MOD_EASY
};
static CommandDefinition pm_commands_el_2d_40[] = {
  CMDS_EASY,
  CMDS_STAT_13(0X8000, 0X4000)
};

static uint16_t pm_status_el_2d_66[] = {
  SHOW_STAT_13
};
static int16_t pm_modifiers_el_2d_66[] = {
  MOD_EASY
};
static CommandDefinition pm_commands_el_2d_66[] = {
  CMDS_EASY,
  CMDS_STAT_13(0X8000, 0X4000)
};

static uint16_t pm_status_el_80[] = {
  SHOW_STAT_2
};
static int16_t pm_modifiers_el_80[] = {
  MOD_EASY
};
static CommandDefinition pm_commands_el_80[] = {
  CMDS_EASY,
  CMDS_STAT_2
};

static uint16_t pm_status_el_2d_80[] = {
  SHOW_STAT_20
};
static int16_t pm_modifiers_el_2d_80[] = {
  MOD_EASY
};
static CommandDefinition pm_commands_el_2d_80[] = {
  CMDS_EASY,
  CMDS_STAT_20(0X8000, 0X4000)
};

static uint16_t pm_status_el_40_p[] = {
};
static int16_t pm_modifiers_el_40_p[] = {
  MOD_EASY
};
static CommandDefinition pm_commands_el_40_p[] = {
  CMDS_EASY
};

static uint16_t pm_status_elba_32[] = {
};
static int16_t pm_modifiers_elba_32[] = {
  MOD_EASY
};
static CommandDefinition pm_commands_elba_32[] = {
  CMDS_EASY
};

static uint16_t pm_status_elba_20[] = {
};
static int16_t pm_modifiers_elba_20[] = {
  MOD_EASY
};
static CommandDefinition pm_commands_elba_20[] = {
  CMDS_EASY
};

static uint16_t pm_status_el_40_s[] = {
};
static int16_t pm_modifiers_el_40_s[] = {
  MOD_EASY
};
static CommandDefinition pm_commands_el_40_s[] = {
  CMDS_EASY_SWSIM,
  CMDS_EASY
};

static uint16_t pm_status_el_80_s[] = {
  SHOW_STAT_2
};
static int16_t pm_modifiers_el_80_s[] = {
  MOD_EASY
};
static CommandDefinition pm_commands_el_80_s[] = {
  CMDS_EASY_SWSIM,
  CMDS_EASY,
  CMDS_STAT_2
};

static uint16_t pm_status_el_66_s[] = {
};
static int16_t pm_modifiers_el_66_s[] = {
  MOD_EASY
};
static CommandDefinition pm_commands_el_66_s[] = {
  CMDS_EASY_SWSIM,
  CMDS_EASY
};


static TerminalDefinition pmTerminalTable[] = {
  PM_TERMINAL(
    0,				/* identity */
    c_486,		/* filename of local helpfile */
    "BrailleX Compact 486",	/* name of terminal */
    40, 1,			/* size of display */
    9,				/* number of front keys */
    0, 0, 0, 0, 0		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    1,				/* identity */
    2d_l,		/* filename of local helpfile */
    "BrailleX 2D Lite (plus)",	/* name of terminal */
    40, 1,			/* size of display */
    9,				/* number of front keys */
    0, 0, 0, 0, 0		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    2,				/* identity */
    c,		/* filename of local helpfile */
    "BrailleX Compact/Tiny",	/* name of terminal */
    40, 1,			/* size of display */
    9,				/* number of front keys */
    0, 0, 0, 0, 0		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    3,				/* identity */
    2d_s,		/* filename of local helpfile */
    "BrailleX 2D Screen Soft", /* name of terminal */
    80, 1,			/* size of display */
    13,				/* number of front keys */
    0, 0, 0, 0, 0		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    6,				/* identity */
    ib_80,		/* filename of local helpfile */
    "BrailleX IB 80 CR Soft",	/* name of terminal */
    80, 1,			/* size of display */
    9,				/* number of front keys */
    0, 0, 0, 0, 0		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    64,				/* identity */
    el_2d_40,		/* filename of local helpfile */
    "BrailleX EL 2D-40",	/* name of terminal */
    40, 1,			/* size of display */
    0,				/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    65,				/* identity */
    el_2d_66,		/* filename of local helpfile */
    "BrailleX EL 2D-66",	/* name of terminal */
    66, 1,			/* size of display */
    0,				/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    66,				/* identity */
    el_80,		/* filename of local helpfile */
    "BrailleX EL 80",		/* name of terminal */
    80, 1,			/* size of display */
    0,				/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    67,				/* identity */
    el_2d_80,		/* filename of local helpfile */
    "BrailleX EL 2D-80",		/* name of terminal */
    80, 1,			/* size of display */
    0,				/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    68,				/* identity */
    el_40_p,		/* filename of local helpfile */
    "BrailleX EL 40 P",		/* name of terminal */
    40, 1,			/* size of display */
    0,				/* number of front keys */
    1, 1, 1, 1, 0		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    69,				/* identity */
    elba_32,		/* filename of local helpfile */
    "BrailleX Elba 32",		/* name of terminal */
    32, 1,			/* size of display */
    0,				/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    70,				/* identity */
    elba_20,		/* filename of local helpfile */
    "BrailleX Elba 20",		/* name of terminal */
    20, 1,			/* size of display */
    0,				/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    85,				/* identity */
    el_40_s,		/* filename of local helpfile */
    "BrailleX II EL 40 S",		/* name of terminal */
    40, 1,			/* size of display */
    0,				/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    86,				/* identity */
    el_80_s,		/* filename of local helpfile */
    "BrailleX II EL 80 S",		/* name of terminal */
    80, 1,			/* size of display */
    0,				/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy bar */
  )
  ,
  PM_TERMINAL(
    87,				/* identity */
    el_66_s,		/* filename of local helpfile */
    "BrailleX II EL 66 S",		/* name of terminal */
    66, 1,			/* size of display */
    0,				/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy bar */
  )
};

static TerminalDefinition *pmTerminals = pmTerminalTable;
static int pmTerminalCount = PM_COUNT(pmTerminalTable);
static int pmTerminalsAllocated = 0;
