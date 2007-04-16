/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#include "prologue.h"

#include "Programs/brl.h"
typedef enum {
  BRL_CMD_INPUT = BRL_driverCommandCount /* toggle input mode */,
  BRL_CMD_SWSIM_LC /* simulate left switch centered */,
  BRL_CMD_SWSIM_LR /* simulate left switch rear */,
  BRL_CMD_SWSIM_LF /* simulate left switch front */,
  BRL_CMD_SWSIM_RC /* simulate right switch centered */,
  BRL_CMD_SWSIM_RR /* simulate right switch rear */,
  BRL_CMD_SWSIM_RF /* simulate right switch front */,
  BRL_CMD_SWSIM_BC /* simulate both switches centered */,
  BRL_CMD_SWSIM_BQ /* show positions of both switches */,
  BRL_CMD_NODOTS = BRL_BLK_PASSDOTS /* input character corresponding to no braille dots */
} InternalDriverCommands;

typedef enum {
  BRL_GSC_INPUT = BRL_genericStatusCellCount /* input mode */,
  InternalStatusCellCount
} InternalStatusCell;

#define OFFS_FRONT      0
#define OFFS_STAT    1000
#define OFFS_ROUTE   2000  
#define OFFS_BAR     3000
#define OFFS_SWITCH  4000
#define OFFS_CR      5000

#define ROUTINGKEY1     -9999  /* first row of  routing keys */
#define ROUTINGKEY2     -9998  /* second row of  routing keys */
#define NOKEY           -1

#define MODMAX    16
#define KEYMAX     8

#define OFFS_EMPTY       0
#define OFFS_HORIZ    1000	/* added to status show */
#define OFFS_FLAG     2000
#define OFFS_NUMBER   3000

/* easy access bar - code as delivered by the terminals */
#define BAR_L1 1
#define BAR_L2 2    
#define BAR_U1 3 
#define BAR_U2 4
#define BAR_R1 5 
#define BAR_R2 6  
#define BAR_D1 7 
#define BAR_D2 8  

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
  uint8_t hasBar;		/* has an easy access bar */
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

#define PM_TERMINAL(identifier, model, name, columns, rows, status, front, modifiers, eab, ls, rs, lk, rk) \
{ \
  identifier, name, "brltty-pm-" #model ".hlp", \
  columns, rows, front, eab, ls, rs, lk, rk, \
  status, \
  ARRAY_COUNT(pmModifiers_##modifiers), \
  ARRAY_COUNT(pmCommands_##modifiers##_Status##status), \
  pmStatus_##status, \
  pmModifiers_##modifiers, \
  pmCommands_##modifiers##_Status##status \
}

/* some macros for terminals with the same layout -
 * named after there usage
 */


#define PM_BEGIN_STATUS(count) static uint16_t pmStatus_##count[] = {
#define PM_END_STATUS };

#define pmStatus_0 NULL

PM_BEGIN_STATUS(2)
  OFFS_NUMBER + BRL_GSC_BRLROW,
  OFFS_NUMBER + BRL_GSC_CSRROW
PM_END_STATUS

PM_BEGIN_STATUS(4)
  OFFS_NUMBER + BRL_GSC_BRLROW,
  OFFS_NUMBER + BRL_GSC_CSRROW,
  OFFS_NUMBER + BRL_GSC_CSRCOL,
  OFFS_FLAG   + BRL_GSC_DISPMD
PM_END_STATUS

PM_BEGIN_STATUS(13)
  OFFS_HORIZ + BRL_GSC_BRLROW,
  OFFS_EMPTY,
  OFFS_HORIZ + BRL_GSC_CSRROW,
  OFFS_HORIZ + BRL_GSC_CSRCOL,
  OFFS_EMPTY,
  OFFS_FLAG  + BRL_GSC_CSRTRK,
  OFFS_FLAG  + BRL_GSC_DISPMD,
  OFFS_FLAG  + BRL_GSC_FREEZE,
  OFFS_EMPTY,
  OFFS_EMPTY,
  OFFS_FLAG  + BRL_GSC_CSRVIS,
  OFFS_FLAG  + BRL_GSC_ATTRVIS,
  OFFS_EMPTY
PM_END_STATUS

PM_BEGIN_STATUS(20)
  OFFS_HORIZ + BRL_GSC_BRLROW,
  OFFS_EMPTY,
  OFFS_HORIZ + BRL_GSC_CSRROW,
  OFFS_HORIZ + BRL_GSC_CSRCOL,
  OFFS_EMPTY,
  OFFS_FLAG  + BRL_GSC_CSRTRK,
  OFFS_FLAG  + BRL_GSC_DISPMD,
  OFFS_FLAG  + BRL_GSC_FREEZE,
  OFFS_EMPTY,
  OFFS_HORIZ + BRL_GSC_SCRNUM,
  OFFS_EMPTY,
  OFFS_FLAG  + BRL_GSC_CSRVIS,
  OFFS_FLAG  + BRL_GSC_ATTRVIS,
  OFFS_FLAG  + BRL_GSC_CAPBLINK,
  OFFS_FLAG  + BRL_GSC_SIXDOTS,
  OFFS_FLAG  + BRL_GSC_SKPIDLNS,
  OFFS_FLAG  + BRL_GSC_TUNES,
  OFFS_FLAG  + BRL_GSC_AUTOSPEAK,
  OFFS_FLAG  + BRL_GSC_AUTOREPEAT,
  OFFS_EMPTY
PM_END_STATUS

PM_BEGIN_STATUS(22)
  OFFS_HORIZ + BRL_GSC_BRLROW,
  OFFS_EMPTY,
  OFFS_HORIZ + BRL_GSC_CSRROW,
  OFFS_HORIZ + BRL_GSC_CSRCOL,
  OFFS_EMPTY,
  OFFS_FLAG  + BRL_GSC_CSRTRK,
  OFFS_FLAG  + BRL_GSC_DISPMD,
  OFFS_FLAG  + BRL_GSC_FREEZE,
  OFFS_EMPTY,
  OFFS_HORIZ + BRL_GSC_SCRNUM,
  OFFS_EMPTY,
  OFFS_FLAG  + BRL_GSC_CSRVIS,
  OFFS_FLAG  + BRL_GSC_ATTRVIS,
  OFFS_FLAG  + BRL_GSC_CAPBLINK,
  OFFS_FLAG  + BRL_GSC_SIXDOTS,
  OFFS_FLAG  + BRL_GSC_SKPIDLNS,
  OFFS_FLAG  + BRL_GSC_TUNES,
  OFFS_EMPTY,
  OFFS_FLAG  + BRL_GSC_INPUT,
  OFFS_FLAG  + BRL_GSC_AUTOSPEAK,
  OFFS_FLAG  + BRL_GSC_AUTOREPEAT,
  OFFS_EMPTY
PM_END_STATUS


#define PM_BEGIN_MODIFIERS(name) static int16_t pmModifiers_##name[] = {
#define PM_END_MODIFIERS };

PM_BEGIN_MODIFIERS(Front9)
  OFFS_FRONT + 1,
  OFFS_FRONT + 9,
  OFFS_FRONT + 2,
  OFFS_FRONT + 8
PM_END_MODIFIERS

PM_BEGIN_MODIFIERS(Front13)
  OFFS_FRONT +  4,
  OFFS_FRONT +  3,
  OFFS_FRONT +  2,
  OFFS_FRONT + 10,
  OFFS_FRONT + 11,
  OFFS_FRONT + 12,
  OFFS_FRONT +  1,
  OFFS_FRONT + 13
PM_END_MODIFIERS

PM_BEGIN_MODIFIERS(Bar)
  OFFS_SWITCH + SWITCH_LEFT_REAR,
  OFFS_SWITCH + SWITCH_LEFT_FRONT,
  OFFS_SWITCH + SWITCH_RIGHT_REAR,
  OFFS_SWITCH + SWITCH_RIGHT_FRONT,
  OFFS_SWITCH + KEY_LEFT_REAR,
  OFFS_SWITCH + KEY_LEFT_FRONT,
  OFFS_SWITCH + KEY_RIGHT_REAR,
  OFFS_SWITCH + KEY_RIGHT_FRONT,
  OFFS_BAR    + BAR_U1,
  OFFS_BAR    + BAR_D1,
  OFFS_BAR    + BAR_L1,
  OFFS_BAR    + BAR_R1,
  OFFS_BAR    + BAR_U2,
  OFFS_BAR    + BAR_D2,
  OFFS_BAR    + BAR_L2,
  OFFS_BAR    + BAR_R2
PM_END_MODIFIERS
#define pmModifiers_BarSim pmModifiers_Bar

#define MOD_BAR_SLC 0X0000
#define MOD_BAR_SLR 0X0001
#define MOD_BAR_SLF 0X0002
#define MOD_BAR_SRC 0X0000
#define MOD_BAR_SRR 0X0004
#define MOD_BAR_SRF 0X0008
#define MOD_BAR_KLR 0X0010
#define MOD_BAR_KLF 0X0020
#define MOD_BAR_KRR 0X0040
#define MOD_BAR_KRF 0X0080
#define MOD_BAR_BU1 0X0100
#define MOD_BAR_BD1 0X0200
#define MOD_BAR_BL1 0X0400
#define MOD_BAR_BR1 0X0800
#define MOD_BAR_BU2 0X1000
#define MOD_BAR_BD2 0X2000
#define MOD_BAR_BL2 0X4000
#define MOD_BAR_BR2 0X8000


#define CHGONOFF(cmd, offs, on, off) \
      { cmd                 , offs, 0  }, \
      { cmd | BRL_FLG_TOGGLE_OFF, offs, off}, \
      { cmd | BRL_FLG_TOGGLE_ON , offs, on }


/* commands for 9 front keys */
#define CMDS_FRONT_9 \
     { BRL_CMD_FWINLT     , NOKEY         , 0X1 }, \
     { BRL_CMD_FWINRT     , NOKEY         , 0X2 }, \
     { BRL_CMD_HWINLT     , NOKEY         , 0X4 }, \
     { BRL_CMD_HWINRT     , NOKEY         , 0X8 }, \
                                               \
     { BRL_CMD_HOME       , OFFS_FRONT + 5, 0X0 }, \
     { BRL_CMD_LNBEG      , OFFS_FRONT + 5, 0X1 }, \
     { BRL_CMD_LNEND      , OFFS_FRONT + 5, 0X2 }, \
     { BRL_CMD_CHRLT      , OFFS_FRONT + 5, 0X4 }, \
     { BRL_CMD_CHRRT      , OFFS_FRONT + 5, 0X8 }, \
                                               \
     { BRL_CMD_WINUP      , OFFS_FRONT + 4, 0X0 }, \
     { BRL_CMD_PRDIFLN    , OFFS_FRONT + 4, 0X1 }, \
     { BRL_CMD_ATTRUP     , OFFS_FRONT + 4, 0X2 }, \
     { BRL_CMD_PRPGRPH    , OFFS_FRONT + 4, 0X4 }, \
     { BRL_CMD_PRSEARCH   , OFFS_FRONT + 4, 0X8 }, \
                                               \
     { BRL_CMD_WINDN      , OFFS_FRONT + 6, 0X0 }, \
     { BRL_CMD_NXDIFLN    , OFFS_FRONT + 6, 0X1 }, \
     { BRL_CMD_ATTRDN     , OFFS_FRONT + 6, 0X2 }, \
     { BRL_CMD_NXPGRPH    , OFFS_FRONT + 6, 0X4 }, \
     { BRL_CMD_NXSEARCH   , OFFS_FRONT + 6, 0X8 }, \
                                               \
     { BRL_CMD_LNUP       , OFFS_FRONT + 3, 0X0 }, \
     { BRL_CMD_TOP_LEFT   , OFFS_FRONT + 3, 0X1 }, \
     { BRL_CMD_TOP        , OFFS_FRONT + 3, 0X2 }, \
                                               \
     { BRL_CMD_LNDN       , OFFS_FRONT + 7, 0X0 }, \
     { BRL_CMD_BOT_LEFT   , OFFS_FRONT + 7, 0X1 }, \
     { BRL_CMD_BOT        , OFFS_FRONT + 7, 0X2 }, \
                                               \
     { BRL_BLK_ROUTE       , ROUTINGKEY1   , 0X0 }, \
     { BRL_BLK_CUTBEGIN    , ROUTINGKEY1   , 0X1 }, \
     { BRL_BLK_CUTRECT     , ROUTINGKEY1   , 0X2 }, \
     { BRL_BLK_PRINDENT    , ROUTINGKEY1   , 0X4 }, \
     { BRL_BLK_NXINDENT    , ROUTINGKEY1   , 0X8 }, \
     { BRL_CMD_PASTE      , NOKEY         , 0X3 }


/* commands for 13 front keys */
#define CMDS_FRONT_13 \
      { BRL_CMD_HOME                    , OFFS_FRONT +  7, 0000 }, \
      { BRL_CMD_TOP                     , OFFS_FRONT +  6, 0000 }, \
      { BRL_CMD_BOT                     , OFFS_FRONT +  8, 0000 }, \
      { BRL_CMD_LNUP                    , OFFS_FRONT +  5, 0000 }, \
      { BRL_CMD_LNDN                    , OFFS_FRONT +  9, 0000 }, \
                                                               \
      { BRL_CMD_PRDIFLN                 , NOKEY          , 0001 }, \
      { BRL_CMD_NXDIFLN                 , NOKEY          , 0010 }, \
      { BRL_CMD_ATTRUP                  , NOKEY          , 0002 }, \
      { BRL_CMD_ATTRDN                  , NOKEY          , 0020 }, \
      { BRL_CMD_PRPGRPH                 , NOKEY          , 0004 },         \
      { BRL_CMD_NXPGRPH                 , NOKEY          , 0040 },        \
      { BRL_CMD_PRPROMPT                , NOKEY          , 0100 },        \
      { BRL_CMD_NXPROMPT                , NOKEY          , 0200 },        \
                                                               \
      { BRL_CMD_WINUP                   , NOKEY          , 0003 },        \
      { BRL_CMD_WINDN                   , NOKEY          , 0030 },        \
      { BRL_CMD_PRSEARCH                , NOKEY          , 0104 },        \
      { BRL_CMD_NXSEARCH                , NOKEY          , 0240 },        \
      { BRL_BLK_PRINDENT                 , ROUTINGKEY1    , 0104 },        \
      { BRL_BLK_NXINDENT                 , ROUTINGKEY1    , 0240 },        \
                                                               \
      { BRL_CMD_LNBEG                   , OFFS_FRONT +  7, 0001 }, \
      { BRL_CMD_TOP_LEFT                , OFFS_FRONT +  6, 0001 }, \
      { BRL_CMD_BOT_LEFT                , OFFS_FRONT +  8, 0001 }, \
      { BRL_CMD_FWINLT                  , OFFS_FRONT +  5, 0001 }, \
      { BRL_CMD_FWINRT                  , OFFS_FRONT +  9, 0001 }, \
      { BRL_BLK_DESCCHAR                 , ROUTINGKEY1    , 0001 }, \
                                                               \
      { BRL_CMD_LNEND                   , OFFS_FRONT +  7, 0010 }, \
      { BRL_CMD_CHRLT                   , OFFS_FRONT +  6, 0010 }, \
      { BRL_CMD_CHRRT                   , OFFS_FRONT +  8, 0010 }, \
      { BRL_CMD_HWINLT                  , OFFS_FRONT +  5, 0010 }, \
      { BRL_CMD_HWINRT                  , OFFS_FRONT +  9, 0010 }, \
      { BRL_BLK_SETLEFT                  , ROUTINGKEY1    , 0010 }, \
                                                               \
      { BRL_BLK_PASSKEY+BRL_KEY_INSERT      , OFFS_FRONT +  7, 0002 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP     , OFFS_FRONT +  6, 0002 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN   , OFFS_FRONT +  8, 0002 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP   , OFFS_FRONT +  5, 0002 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN , OFFS_FRONT +  9, 0002 }, \
      { BRL_BLK_SWITCHVT                 , ROUTINGKEY1    , 0002 }, \
                                                               \
      { BRL_BLK_PASSKEY+BRL_KEY_DELETE      , OFFS_FRONT +  7, 0020 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_HOME        , OFFS_FRONT +  6, 0020 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_END         , OFFS_FRONT +  8, 0020 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT , OFFS_FRONT +  5, 0020 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, OFFS_FRONT +  9, 0020 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_FUNCTION    , ROUTINGKEY1    , 0020 }, \
                                                               \
      { BRL_CMD_NODOTS                  , OFFS_FRONT +  7, 0004 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_ESCAPE      , OFFS_FRONT +  6, 0004 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_TAB         , OFFS_FRONT +  8, 0004 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_BACKSPACE   , OFFS_FRONT +  5, 0004 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_ENTER      , OFFS_FRONT +  9, 0004 }, \
                                                               \
      { BRL_CMD_SPKHOME                 , OFFS_FRONT +  7, 0100 }, \
      { BRL_CMD_SAY_ABOVE               , OFFS_FRONT +  6, 0100 }, \
      { BRL_CMD_SAY_BELOW               , OFFS_FRONT +  8, 0100 }, \
      { BRL_CMD_MUTE                    , OFFS_FRONT +  5, 0100 }, \
      { BRL_CMD_SAY_LINE                , OFFS_FRONT +  9, 0100 }, \
                                                               \
      { BRL_CMD_RESTARTSPEECH           , OFFS_FRONT +  7, 0200 }, \
      { BRL_CMD_SAY_SLOWER              , OFFS_FRONT +  6, 0200 }, \
      { BRL_CMD_SAY_FASTER              , OFFS_FRONT +  8, 0200 }, \
      { BRL_CMD_SAY_SOFTER              , OFFS_FRONT +  5, 0200 }, \
      { BRL_CMD_SAY_LOUDER              , OFFS_FRONT +  9, 0200 }, \
                                                               \
      { BRL_BLK_CUTBEGIN                 , ROUTINGKEY1    , 0100 }, \
      { BRL_BLK_CUTAPPEND                , ROUTINGKEY1    , 0004 }, \
      { BRL_BLK_CUTLINE                  , ROUTINGKEY1    , 0040 }, \
      { BRL_BLK_CUTRECT                  , ROUTINGKEY1    , 0200 }, \
                                                               \
      { BRL_BLK_ROUTE                    , ROUTINGKEY1    , 0000 }
	

/* commands for easy access bar + switches/keys */
#define CMDS_BAR_COMMON(common) \
  common(MOD_BAR_SLC|MOD_BAR_SRC), \
  common(MOD_BAR_SLC|MOD_BAR_SRR), \
  common(MOD_BAR_SLC|MOD_BAR_SRF), \
  common(MOD_BAR_SLR|MOD_BAR_SRC), \
  common(MOD_BAR_SLR|MOD_BAR_SRR), \
  common(MOD_BAR_SLR|MOD_BAR_SRF), \
  common(MOD_BAR_SLF|MOD_BAR_SRC), \
  common(MOD_BAR_SLF|MOD_BAR_SRR), \
  common(MOD_BAR_SLF|MOD_BAR_SRF)
#define CMDS_BAR(code, mod, u1, d1, u2, d2, l1, r1, l2, r2) \
  {(u1), (code), MOD_BAR_BU1|(mod)}, \
  {(d1), (code), MOD_BAR_BD1|(mod)}, \
  {(u2), (code), MOD_BAR_BU2|(mod)}, \
  {(d2), (code), MOD_BAR_BD2|(mod)}, \
  {(l1), (code), MOD_BAR_BL1|(mod)}, \
  {(r1), (code), MOD_BAR_BR1|(mod)}, \
  {(l2), (code), MOD_BAR_BL2|(mod)}, \
  {(r2), (code), MOD_BAR_BR2|(mod)}
#define CMDS_BAR_COMMON_ALL(mod) \
  {BRL_CMD_BACK , NOKEY, MOD_BAR_KLR|(mod)}, \
  {BRL_CMD_HOME , NOKEY, MOD_BAR_KLF|(mod)}, \
  {BRL_CMD_HELP , NOKEY, MOD_BAR_KRR|(mod)}, \
  {BRL_CMD_LEARN, NOKEY, MOD_BAR_KRF|(mod)}, \
  CMDS_BAR(NOKEY, MOD_BAR_KLR|(mod), \
           BRL_CMD_SIXDOTS, BRL_CMD_PASTE, BRL_CMD_CAPBLINK, BRL_CMD_CSRJMP_VERT, \
                BRL_CMD_DISPMD, BRL_CMD_CSRTRK, BRL_CMD_ATTRVIS, BRL_CMD_CSRVIS), \
  CMDS_BAR(NOKEY, MOD_BAR_KLF|(mod), \
           BRL_CMD_AUTOSPEAK, BRL_CMD_AUTOREPEAT, BRL_CMD_RESTARTBRL, BRL_CMD_FREEZE, \
                BRL_CMD_INFO, BRL_CMD_PREFMENU, BRL_CMD_PREFLOAD, BRL_CMD_PREFSAVE), \
  CMDS_BAR(NOKEY, MOD_BAR_KRR|(mod), \
           BRL_CMD_SAY_ABOVE, BRL_CMD_SAY_BELOW, BRL_CMD_SAY_LOUDER, BRL_CMD_SAY_SOFTER, \
                BRL_CMD_MUTE, BRL_CMD_SAY_LINE, BRL_CMD_SAY_SLOWER, BRL_CMD_SAY_FASTER), \
  CMDS_BAR(NOKEY, MOD_BAR_KRF|(mod), \
           BRL_CMD_SPKHOME, BRL_CMD_TUNES, BRL_CMD_RESTARTSPEECH, BRL_CMD_SWSIM_BQ, \
                BRL_CMD_SKPIDLNS, BRL_CMD_SKPBLNKWINS, BRL_CMD_NOOP, BRL_CMD_SLIDEWIN), \
  {BRL_BLK_ROUTE, ROUTINGKEY1, (mod)}, \
  CMDS_BAR(ROUTINGKEY1, (mod), \
           BRL_BLK_PRINDENT, BRL_BLK_NXINDENT, BRL_BLK_SETLEFT, BRL_BLK_DESCCHAR, \
                BRL_BLK_CUTAPPEND, BRL_BLK_CUTLINE, BRL_BLK_CUTBEGIN, BRL_BLK_CUTRECT), \
  {BRL_BLK_DESCCHAR, ROUTINGKEY2, (mod)}
#define CMDS_BAR_ALL \
  CMDS_BAR(NOKEY, MOD_BAR_SLC|MOD_BAR_SRC, \
           BRL_CMD_LNUP, BRL_CMD_LNDN, BRL_CMD_TOP, BRL_CMD_BOT, \
                BRL_CMD_FWINLT, BRL_CMD_FWINRT, BRL_CMD_LNBEG, BRL_CMD_LNEND), \
  CMDS_BAR(NOKEY, MOD_BAR_SLR|MOD_BAR_SRC, \
           BRL_CMD_PRDIFLN, BRL_CMD_NXDIFLN, BRL_CMD_ATTRUP, BRL_CMD_ATTRDN, \
                BRL_CMD_PRPROMPT, BRL_CMD_NXPROMPT, BRL_CMD_PRPGRPH, BRL_CMD_NXPGRPH), \
  CMDS_BAR(NOKEY, MOD_BAR_SLC|MOD_BAR_SRR, \
           BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN, BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP, BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN, \
                BRL_CMD_FWINLT|BRL_FLG_ROUTE, BRL_CMD_FWINRT|BRL_FLG_ROUTE, BRL_CMD_LNBEG|BRL_FLG_ROUTE, BRL_CMD_LNEND|BRL_FLG_ROUTE), \
  CMDS_BAR(NOKEY, MOD_BAR_SLR|MOD_BAR_SRR, \
           BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN, BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP, BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN, \
                BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, BRL_BLK_PASSKEY+BRL_KEY_HOME, BRL_BLK_PASSKEY+BRL_KEY_END), \
  CMDS_BAR(NOKEY, MOD_BAR_SLF|MOD_BAR_SRC, \
           BRL_CMD_PRSEARCH, BRL_CMD_NXSEARCH, BRL_CMD_HELP, BRL_CMD_LEARN, \
                BRL_CMD_CHRLT, BRL_CMD_CHRRT, BRL_CMD_HWINLT, BRL_CMD_HWINRT), \
  CMDS_BAR(NOKEY, MOD_BAR_SLC|MOD_BAR_SRF, \
           BRL_CMD_MENU_PREV_ITEM, BRL_CMD_MENU_NEXT_ITEM, BRL_CMD_MENU_FIRST_ITEM, BRL_CMD_MENU_LAST_ITEM, \
                BRL_CMD_MENU_PREV_SETTING, BRL_CMD_MENU_NEXT_SETTING, BRL_CMD_PREFLOAD, BRL_CMD_PREFSAVE), \
  CMDS_BAR_COMMON(CMDS_BAR_COMMON_ALL)

#define CMDS_BAR_COMMON_SWSIM(mod) \
  CMDS_BAR(NOKEY, MOD_BAR_KLF|MOD_BAR_KRF|(mod), \
           BRL_CMD_SWSIM_LC, BRL_CMD_SWSIM_RC, BRL_CMD_SWSIM_BQ, BRL_CMD_SWSIM_BC, \
                BRL_CMD_SWSIM_LR, BRL_CMD_SWSIM_RR, BRL_CMD_SWSIM_LF, BRL_CMD_SWSIM_RF)
#define CMDS_BAR_SWSIM CMDS_BAR_COMMON(CMDS_BAR_COMMON_SWSIM)


/* commands for 2 status keys */
#define CMDS_STAT_2 \
      { BRL_CMD_HELP , OFFS_STAT + 1, 0 }, \
      { BRL_CMD_LEARN, OFFS_STAT + 2, 0 }


/* commands for 4 status keys */
#define CMDS_STAT_4 \
      { BRL_CMD_HELP       , OFFS_STAT + 1, 0 }, \
      { BRL_CMD_LEARN      , OFFS_STAT + 2, 0 }, \
      { BRL_CMD_CSRJMP_VERT, OFFS_STAT + 3, 0 }, \
      { BRL_CMD_DISPMD     , OFFS_STAT + 4, 0 }


/* commands for 13 status keys */
#define CMDS_STAT_13(on, off) \
      CHGONOFF( BRL_CMD_HELP       , OFFS_STAT +  1, on, off), \
              { BRL_CMD_LEARN      , OFFS_STAT +  2, 0      }, \
              { BRL_CMD_CSRJMP_VERT, OFFS_STAT +  3, 0      }, \
              { BRL_CMD_BACK       , OFFS_STAT +  4, 0      }, \
      CHGONOFF( BRL_CMD_INFO       , OFFS_STAT +  5, on, off), \
      CHGONOFF( BRL_CMD_CSRTRK     , OFFS_STAT +  6, on, off), \
      CHGONOFF( BRL_CMD_DISPMD     , OFFS_STAT +  7, on, off), \
      CHGONOFF( BRL_CMD_FREEZE     , OFFS_STAT +  8, on, off), \
              { BRL_CMD_PREFMENU   , OFFS_STAT +  9, 0      }, \
              { BRL_CMD_PREFLOAD   , OFFS_STAT + 10, 0      }, \
      CHGONOFF( BRL_CMD_CSRVIS     , OFFS_STAT + 11, on, off), \
      CHGONOFF( BRL_CMD_ATTRVIS    , OFFS_STAT + 12, on, off), \
              { BRL_CMD_PASTE      , OFFS_STAT + 13, 0      }


/* commands for 20 status keys */
#define CMDS_STAT_20(on, off) \
      CHGONOFF( BRL_CMD_HELP       , OFFS_STAT +  1, on, off ), \
              { BRL_CMD_LEARN      , OFFS_STAT +  2, 0X0000  }, \
              { BRL_CMD_CSRJMP_VERT, OFFS_STAT +  3, 0X0000  }, \
              { BRL_CMD_BACK       , OFFS_STAT +  4, 0X0000  }, \
      CHGONOFF( BRL_CMD_INFO       , OFFS_STAT +  5, on, off ), \
      CHGONOFF( BRL_CMD_CSRTRK     , OFFS_STAT +  6, on, off ), \
      CHGONOFF( BRL_CMD_DISPMD     , OFFS_STAT +  7, on, off ), \
      CHGONOFF( BRL_CMD_FREEZE     , OFFS_STAT +  8, on, off ), \
              { BRL_CMD_PREFMENU   , OFFS_STAT +  9, 0X0000  }, \
              { BRL_CMD_PREFSAVE   , OFFS_STAT + 10, 0X0000  }, \
              { BRL_CMD_PREFLOAD   , OFFS_STAT + 11, 0X0000  }, \
      CHGONOFF( BRL_CMD_CSRVIS     , OFFS_STAT + 12, on, off ), \
      CHGONOFF( BRL_CMD_ATTRVIS    , OFFS_STAT + 13, on, off ), \
      CHGONOFF( BRL_CMD_CAPBLINK   , OFFS_STAT + 14, on, off ), \
      CHGONOFF( BRL_CMD_SIXDOTS    , OFFS_STAT + 15, on, off ), \
      CHGONOFF( BRL_CMD_SKPIDLNS   , OFFS_STAT + 16, on, off ), \
      CHGONOFF( BRL_CMD_TUNES      , OFFS_STAT + 17, on, off ), \
      CHGONOFF( BRL_CMD_AUTOSPEAK  , OFFS_STAT + 18, on, off ), \
      CHGONOFF( BRL_CMD_AUTOREPEAT , OFFS_STAT + 19, on, off ), \
              { BRL_CMD_PASTE      , OFFS_STAT + 20, 0X0000  }


/* commands for 22 status keys */
#define CMDS_STAT_22(on, off) \
      CHGONOFF( BRL_CMD_HELP       , OFFS_STAT +  1, on, off ), \
              { BRL_CMD_LEARN      , OFFS_STAT +  2, 0X0000  }, \
              { BRL_CMD_CSRJMP_VERT, OFFS_STAT +  3, 0X0000  }, \
              { BRL_CMD_BACK       , OFFS_STAT +  4, 0X0000  }, \
      CHGONOFF( BRL_CMD_INFO       , OFFS_STAT +  5, on, off ), \
      CHGONOFF( BRL_CMD_CSRTRK     , OFFS_STAT +  6, on, off ), \
      CHGONOFF( BRL_CMD_DISPMD     , OFFS_STAT +  7, on, off ), \
      CHGONOFF( BRL_CMD_FREEZE     , OFFS_STAT +  8, on, off ), \
              { BRL_CMD_PREFMENU   , OFFS_STAT +  9, 0X0000  }, \
              { BRL_CMD_PREFSAVE   , OFFS_STAT + 10, 0X0000  }, \
              { BRL_CMD_PREFLOAD   , OFFS_STAT + 11, 0X0000  }, \
      CHGONOFF( BRL_CMD_CSRVIS     , OFFS_STAT + 12, on, off ), \
      CHGONOFF( BRL_CMD_ATTRVIS    , OFFS_STAT + 13, on, off ), \
      CHGONOFF( BRL_CMD_CAPBLINK   , OFFS_STAT + 14, on, off ), \
      CHGONOFF( BRL_CMD_SIXDOTS    , OFFS_STAT + 15, on, off ), \
      CHGONOFF( BRL_CMD_SKPIDLNS   , OFFS_STAT + 16, on, off ), \
      CHGONOFF( BRL_CMD_TUNES      , OFFS_STAT + 17, on, off ), \
              { BRL_CMD_RESTARTBRL , OFFS_STAT + 18, 0X0000  }, \
      CHGONOFF( BRL_CMD_INPUT      , OFFS_STAT + 19, on, off ), \
      CHGONOFF( BRL_CMD_AUTOSPEAK  , OFFS_STAT + 20, on, off ), \
      CHGONOFF( BRL_CMD_AUTOREPEAT , OFFS_STAT + 21, on, off ), \
              { BRL_CMD_PASTE      , OFFS_STAT + 22, 0X0000  }


#define PM_BEGIN_COMMANDS(name) static CommandDefinition pmCommands_##name[] = {
#define PM_END_COMMANDS };

PM_BEGIN_COMMANDS(Front9_Status0)
  CMDS_FRONT_9
PM_END_COMMANDS

PM_BEGIN_COMMANDS(Front9_Status4)
  CMDS_FRONT_9,
  CMDS_STAT_4
PM_END_COMMANDS

PM_BEGIN_COMMANDS(Front9_Status13)
  CMDS_FRONT_9,
  CMDS_STAT_13(0X2, 0X1)
PM_END_COMMANDS

PM_BEGIN_COMMANDS(Front13_Status22)
  CMDS_FRONT_13,
  CMDS_STAT_22(0X80, 0X40)
PM_END_COMMANDS

PM_BEGIN_COMMANDS(Bar_Status0)
  CMDS_BAR_ALL
PM_END_COMMANDS

PM_BEGIN_COMMANDS(Bar_Status2)
  CMDS_BAR_ALL,
  CMDS_STAT_2
PM_END_COMMANDS

PM_BEGIN_COMMANDS(Bar_Status13)
  CMDS_BAR_ALL,
  CMDS_STAT_13(0X8000, 0X4000)
PM_END_COMMANDS

PM_BEGIN_COMMANDS(Bar_Status20)
  CMDS_BAR_ALL,
  CMDS_STAT_20(0X8000, 0X4000)
PM_END_COMMANDS

PM_BEGIN_COMMANDS(BarSim_Status0)
  CMDS_BAR_ALL,
  CMDS_BAR_SWSIM
PM_END_COMMANDS

PM_BEGIN_COMMANDS(BarSim_Status2)
  CMDS_BAR_ALL,
  CMDS_STAT_2,
  CMDS_BAR_SWSIM
PM_END_COMMANDS

PM_BEGIN_COMMANDS(BarSim_Status20)
  CMDS_BAR_ALL,
  CMDS_STAT_20(0X8000, 0X4000),
  CMDS_BAR_SWSIM
PM_END_COMMANDS


static TerminalDefinition pmTerminalTable[] = {
  PM_TERMINAL(
    0,				/* identity */
    c_486,		/* filename of local helpfile */
    "BrailleX Compact 486",	/* name of terminal */
    40, 1, 0,			/* size of display */
    9, Front9,			/* number of front keys */
    0, 0, 0, 0, 0		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    1,				/* identity */
    2d_l,		/* filename of local helpfile */
    "BrailleX 2D Lite (plus)",	/* name of terminal */
    40, 1, 13,			/* size of display */
    9, Front9,			/* number of front keys */
    0, 0, 0, 0, 0		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    2,				/* identity */
    c,		/* filename of local helpfile */
    "BrailleX Compact/Tiny",	/* name of terminal */
    40, 1, 0,			/* size of display */
    9, Front9,			/* number of front keys */
    0, 0, 0, 0, 0		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    3,				/* identity */
    2d_s,		/* filename of local helpfile */
    "BrailleX 2D Screen Soft", /* name of terminal */
    80, 1, 22,			/* size of display */
    13, Front13,		/* number of front keys */
    0, 0, 0, 0, 0		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    6,				/* identity */
    ib_80,		/* filename of local helpfile */
    "BrailleX IB 80 CR Soft",	/* name of terminal */
    80, 1, 4,			/* size of display */
    9, Front9,			/* number of front keys */
    0, 0, 0, 0, 0		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    64,				/* identity */
    el_2d_40,		/* filename of local helpfile */
    "BrailleX EL 2D-40",	/* name of terminal */
    40, 1, 13,			/* size of display */
    0, Bar, 			/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    65,				/* identity */
    el_2d_66,		/* filename of local helpfile */
    "BrailleX EL 2D-66",	/* name of terminal */
    66, 1, 13,			/* size of display */
    0, Bar, 			/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    66,				/* identity */
    el_80,		/* filename of local helpfile */
    "BrailleX EL 80",		/* name of terminal */
    80, 1, 2,			/* size of display */
    0, Bar, 			/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    67,				/* identity */
    el_2d_80,		/* filename of local helpfile */
    "BrailleX EL 2D-80",		/* name of terminal */
    80, 1, 20,			/* size of display */
    0, Bar, 			/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    68,				/* identity */
    el_40_p,		/* filename of local helpfile */
    "BrailleX EL 40 P",		/* name of terminal */
    40, 1, 0,			/* size of display */
    0, Bar, 			/* number of front keys */
    1, 1, 1, 1, 0		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    69,				/* identity */
    elba_32,		/* filename of local helpfile */
    "BrailleX Elba 32",		/* name of terminal */
    32, 1, 0,			/* size of display */
    0, Bar, 			/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    70,				/* identity */
    elba_20,		/* filename of local helpfile */
    "BrailleX Elba 20",		/* name of terminal */
    20, 1, 0,			/* size of display */
    0, Bar, 			/* number of front keys */
    1, 1, 1, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    85,				/* identity */
    el40s,		/* filename of local helpfile */
    "BrailleX EL40s",		/* name of terminal */
    40, 1, 0,			/* size of display */
    0, BarSim, 			/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    86,				/* identity */
    el80_ii,		/* filename of local helpfile */
    "BrailleX EL80-II",		/* name of terminal */
    80, 1, 2,			/* size of display */
    0, BarSim, 			/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    87,				/* identity */
    el66s,		/* filename of local helpfile */
    "BrailleX EL66s",		/* name of terminal */
    66, 1, 0,			/* size of display */
    0, BarSim, 			/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    88,				/* identity */
    el80s,		/* filename of local helpfile */
    "BrailleX EL80s",		/* name of terminal */
    80, 1, 0,			/* size of display */
    0, BarSim, 			/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    89,				/* identity */
    trio,		/* filename of local helpfile */
    "BrailleX Trio",		/* name of terminal */
    40, 1, 0,			/* size of display */
    0, BarSim, 			/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    90,				/* identity */
    el70s,		/* filename of local helpfile */
    "BrailleX EL70s",		/* name of terminal */
    70, 1, 0,			/* size of display */
    0, BarSim, 			/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    91,				/* identity */
    el2d_80s,		/* filename of local helpfile */
    "BrailleX EL2D-80s",		/* name of terminal */
    80, 1, 20,			/* size of display */
    0, BarSim, 			/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    92,				/* identity */
    elba_trio_20,		/* filename of local helpfile */
    "BrailleX Elba (Trio 20)",		/* name of terminal */
    20, 1, 0,			/* size of display */
    0, BarSim, 			/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy access bar */
  )
  ,
  PM_TERMINAL(
    93,				/* identity */
    elba_trio_32,		/* filename of local helpfile */
    "BrailleX Elba (Trio 32)",		/* name of terminal */
    32, 1, 0,			/* size of display */
    0, BarSim, 			/* number of front keys */
    1, 0, 0, 1, 1		/* terminal has an easy access bar */
  )
};

static TerminalDefinition *pmTerminals = pmTerminalTable;
static int pmTerminalCount = ARRAY_COUNT(pmTerminalTable);
static int pmTerminalsAllocated = 0;
