/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
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

#include "brl.h"
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

#define KEY_NONE     0X000
#define KEY_ROUTING1 0X001  /* first row of routing keys */
#define KEY_ROUTING2 0X002  /* second row of routing keys */
#define KEY_STATUS2  0X003  /* second row of status keys */
#define KEYS_FRONT   0X100
#define KEYS_STATUS  0X200
#define KEYS_BAR     0X300
#define KEYS_SWITCH  0X400


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
  unsigned char modelIdentifier;
  unsigned char protocolRevision;
  char *modelName;
  char *keyBindings;

  uint8_t textColumns;
  uint8_t textRows;
  uint8_t frontKeys;
  uint8_t hasBar;
  uint8_t leftSwitches;
  uint8_t rightSwitches;
  uint8_t leftKeys;
  uint8_t rightKeys;
  uint8_t statusCount;
  uint8_t modifierCount;
  uint16_t commandCount;

  uint16_t *statusCells;
  int16_t *modifierKeys;
  CommandDefinition *commandDefinitions;
} TerminalDefinition; 

#define PM_MODEL_IDENTITY(identifier, model, name) \
  .modelIdentifier = identifier, \
  .modelName = name, \
  .protocolRevision = 1, \
  .keyBindings = #model

#define PM_TEXT_CELLS(columns, rows) \
  .textColumns = columns, \
  .textRows = rows

#define PM_FRONT_KEYS(front, status) \
  .frontKeys = front, \
  PM_TERMINAL_TABLES(status, Front##front)

#define PM_BAR(ls, rs, lk, rk, modifiers, status) \
  .hasBar = 1, \
  .leftSwitches = ls, \
  .rightSwitches = rs, \
  .leftKeys = lk, \
  .rightKeys = rk, \
  PM_TERMINAL_TABLES(status, Bar##modifiers)

#define PM_TERMINAL_TABLES(status, modifiers) \
  .statusCells = pmStatus_##status, \
  .statusCount = status, \
  .modifierKeys = pmModifiers_##modifiers, \
  .modifierCount = ARRAY_COUNT(pmModifiers_##modifiers), \
  .commandDefinitions = pmCommands_##modifiers##_Status##status, \
  .commandCount = ARRAY_COUNT(pmCommands_##modifiers##_Status##status)

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
  KEYS_FRONT + 1,
  KEYS_FRONT + 9,
  KEYS_FRONT + 2,
  KEYS_FRONT + 8
PM_END_MODIFIERS

PM_BEGIN_MODIFIERS(Front13)
  KEYS_FRONT +  4,
  KEYS_FRONT +  3,
  KEYS_FRONT +  2,
  KEYS_FRONT + 10,
  KEYS_FRONT + 11,
  KEYS_FRONT + 12,
  KEYS_FRONT +  1,
  KEYS_FRONT + 13
PM_END_MODIFIERS

PM_BEGIN_MODIFIERS(BarBasic)
  KEYS_SWITCH + SWITCH_LEFT_REAR,
  KEYS_SWITCH + SWITCH_LEFT_FRONT,
  KEYS_SWITCH + SWITCH_RIGHT_REAR,
  KEYS_SWITCH + SWITCH_RIGHT_FRONT,
  KEYS_SWITCH + KEY_LEFT_REAR,
  KEYS_SWITCH + KEY_LEFT_FRONT,
  KEYS_SWITCH + KEY_RIGHT_REAR,
  KEYS_SWITCH + KEY_RIGHT_FRONT,
  KEYS_BAR    + BAR_U1,
  KEYS_BAR    + BAR_D1,
  KEYS_BAR    + BAR_L1,
  KEYS_BAR    + BAR_R1,
  KEYS_BAR    + BAR_U2,
  KEYS_BAR    + BAR_D2,
  KEYS_BAR    + BAR_L2,
  KEYS_BAR    + BAR_R2
PM_END_MODIFIERS
#define pmModifiers_BarSimulate pmModifiers_BarBasic

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
     { BRL_CMD_FWINLT     , KEY_NONE      , 0X1 }, \
     { BRL_CMD_FWINRT     , KEY_NONE      , 0X2 }, \
     { BRL_CMD_HWINLT     , KEY_NONE      , 0X4 }, \
     { BRL_CMD_HWINRT     , KEY_NONE      , 0X8 }, \
                                               \
     { BRL_CMD_HOME       , KEYS_FRONT + 5, 0X0 }, \
     { BRL_CMD_LNBEG      , KEYS_FRONT + 5, 0X1 }, \
     { BRL_CMD_LNEND      , KEYS_FRONT + 5, 0X2 }, \
     { BRL_CMD_CHRLT      , KEYS_FRONT + 5, 0X4 }, \
     { BRL_CMD_CHRRT      , KEYS_FRONT + 5, 0X8 }, \
                                               \
     { BRL_CMD_WINUP      , KEYS_FRONT + 4, 0X0 }, \
     { BRL_CMD_PRDIFLN    , KEYS_FRONT + 4, 0X1 }, \
     { BRL_CMD_ATTRUP     , KEYS_FRONT + 4, 0X2 }, \
     { BRL_CMD_PRPGRPH    , KEYS_FRONT + 4, 0X4 }, \
     { BRL_CMD_PRSEARCH   , KEYS_FRONT + 4, 0X8 }, \
                                               \
     { BRL_CMD_WINDN      , KEYS_FRONT + 6, 0X0 }, \
     { BRL_CMD_NXDIFLN    , KEYS_FRONT + 6, 0X1 }, \
     { BRL_CMD_ATTRDN     , KEYS_FRONT + 6, 0X2 }, \
     { BRL_CMD_NXPGRPH    , KEYS_FRONT + 6, 0X4 }, \
     { BRL_CMD_NXSEARCH   , KEYS_FRONT + 6, 0X8 }, \
                                               \
     { BRL_CMD_LNUP       , KEYS_FRONT + 3, 0X0 }, \
     { BRL_CMD_TOP_LEFT   , KEYS_FRONT + 3, 0X1 }, \
     { BRL_CMD_TOP        , KEYS_FRONT + 3, 0X2 }, \
                                               \
     { BRL_CMD_LNDN       , KEYS_FRONT + 7, 0X0 }, \
     { BRL_CMD_BOT_LEFT   , KEYS_FRONT + 7, 0X1 }, \
     { BRL_CMD_BOT        , KEYS_FRONT + 7, 0X2 }, \
                                               \
     { BRL_BLK_ROUTE       , KEY_ROUTING1  , 0X0 }, \
     { BRL_BLK_CUTBEGIN    , KEY_ROUTING1  , 0X1 }, \
     { BRL_BLK_CUTRECT     , KEY_ROUTING1  , 0X2 }, \
     { BRL_BLK_PRINDENT    , KEY_ROUTING1  , 0X4 }, \
     { BRL_BLK_NXINDENT    , KEY_ROUTING1  , 0X8 }, \
     { BRL_CMD_PASTE      , KEY_NONE      , 0X3 }


/* commands for 13 front keys */
#define CMDS_FRONT_13 \
      { BRL_CMD_HOME                    , KEYS_FRONT +  7, 0000 }, \
      { BRL_CMD_TOP                     , KEYS_FRONT +  6, 0000 }, \
      { BRL_CMD_BOT                     , KEYS_FRONT +  8, 0000 }, \
      { BRL_CMD_LNUP                    , KEYS_FRONT +  5, 0000 }, \
      { BRL_CMD_LNDN                    , KEYS_FRONT +  9, 0000 }, \
                                                               \
      { BRL_CMD_PRDIFLN                 , KEY_NONE       , 0001 }, \
      { BRL_CMD_NXDIFLN                 , KEY_NONE       , 0010 }, \
      { BRL_CMD_ATTRUP                  , KEY_NONE       , 0002 }, \
      { BRL_CMD_ATTRDN                  , KEY_NONE       , 0020 }, \
      { BRL_CMD_PRPGRPH                 , KEY_NONE       , 0004 },         \
      { BRL_CMD_NXPGRPH                 , KEY_NONE       , 0040 },        \
      { BRL_CMD_PRPROMPT                , KEY_NONE       , 0100 },        \
      { BRL_CMD_NXPROMPT                , KEY_NONE       , 0200 },        \
                                                               \
      { BRL_CMD_WINUP                   , KEY_NONE       , 0003 },        \
      { BRL_CMD_WINDN                   , KEY_NONE       , 0030 },        \
      { BRL_CMD_PRSEARCH                , KEY_NONE       , 0104 },        \
      { BRL_CMD_NXSEARCH                , KEY_NONE       , 0240 },        \
      { BRL_BLK_PRINDENT                 , KEY_ROUTING1   , 0104 },        \
      { BRL_BLK_NXINDENT                 , KEY_ROUTING1   , 0240 },        \
                                                               \
      { BRL_CMD_LNBEG                   , KEYS_FRONT +  7, 0001 }, \
      { BRL_CMD_TOP_LEFT                , KEYS_FRONT +  6, 0001 }, \
      { BRL_CMD_BOT_LEFT                , KEYS_FRONT +  8, 0001 }, \
      { BRL_CMD_FWINLT                  , KEYS_FRONT +  5, 0001 }, \
      { BRL_CMD_FWINRT                  , KEYS_FRONT +  9, 0001 }, \
      { BRL_BLK_DESCCHAR                 , KEY_ROUTING1   , 0001 }, \
                                                               \
      { BRL_CMD_LNEND                   , KEYS_FRONT +  7, 0010 }, \
      { BRL_CMD_CHRLT                   , KEYS_FRONT +  6, 0010 }, \
      { BRL_CMD_CHRRT                   , KEYS_FRONT +  8, 0010 }, \
      { BRL_CMD_HWINLT                  , KEYS_FRONT +  5, 0010 }, \
      { BRL_CMD_HWINRT                  , KEYS_FRONT +  9, 0010 }, \
      { BRL_BLK_SETLEFT                  , KEY_ROUTING1   , 0010 }, \
                                                               \
      { BRL_BLK_PASSKEY+BRL_KEY_INSERT      , KEYS_FRONT +  7, 0002 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP     , KEYS_FRONT +  6, 0002 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN   , KEYS_FRONT +  8, 0002 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP   , KEYS_FRONT +  5, 0002 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN , KEYS_FRONT +  9, 0002 }, \
      { BRL_BLK_SWITCHVT                 , KEY_ROUTING1   , 0002 }, \
                                                               \
      { BRL_BLK_PASSKEY+BRL_KEY_DELETE      , KEYS_FRONT +  7, 0020 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_HOME        , KEYS_FRONT +  6, 0020 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_END         , KEYS_FRONT +  8, 0020 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT , KEYS_FRONT +  5, 0020 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, KEYS_FRONT +  9, 0020 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_FUNCTION    , KEY_ROUTING1   , 0020 }, \
                                                               \
      { BRL_CMD_NODOTS                  , KEYS_FRONT +  7, 0004 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_ESCAPE      , KEYS_FRONT +  6, 0004 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_TAB         , KEYS_FRONT +  8, 0004 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_BACKSPACE   , KEYS_FRONT +  5, 0004 }, \
      { BRL_BLK_PASSKEY+BRL_KEY_ENTER      , KEYS_FRONT +  9, 0004 }, \
                                                               \
      { BRL_CMD_SPKHOME                 , KEYS_FRONT +  7, 0100 }, \
      { BRL_CMD_SAY_ABOVE               , KEYS_FRONT +  6, 0100 }, \
      { BRL_CMD_SAY_BELOW               , KEYS_FRONT +  8, 0100 }, \
      { BRL_CMD_MUTE                    , KEYS_FRONT +  5, 0100 }, \
      { BRL_CMD_SAY_LINE                , KEYS_FRONT +  9, 0100 }, \
                                                               \
      { BRL_CMD_RESTARTSPEECH           , KEYS_FRONT +  7, 0200 }, \
      { BRL_CMD_SAY_SLOWER              , KEYS_FRONT +  6, 0200 }, \
      { BRL_CMD_SAY_FASTER              , KEYS_FRONT +  8, 0200 }, \
      { BRL_CMD_SAY_SOFTER              , KEYS_FRONT +  5, 0200 }, \
      { BRL_CMD_SAY_LOUDER              , KEYS_FRONT +  9, 0200 }, \
                                                               \
      { BRL_BLK_CUTBEGIN                 , KEY_ROUTING1   , 0100 }, \
      { BRL_BLK_CUTAPPEND                , KEY_ROUTING1   , 0004 }, \
      { BRL_BLK_CUTLINE                  , KEY_ROUTING1   , 0040 }, \
      { BRL_BLK_CUTRECT                  , KEY_ROUTING1   , 0200 }, \
                                                               \
      { BRL_BLK_ROUTE                    , KEY_ROUTING1   , 0000 }
	

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
  {BRL_CMD_BACK , KEY_NONE, MOD_BAR_KLR|(mod)}, \
  {BRL_CMD_HOME , KEY_NONE, MOD_BAR_KLF|(mod)}, \
  {BRL_CMD_HELP , KEY_NONE, MOD_BAR_KRR|(mod)}, \
  {BRL_CMD_LEARN, KEY_NONE, MOD_BAR_KRF|(mod)}, \
  CMDS_BAR(KEY_NONE, MOD_BAR_KLR|(mod), \
           BRL_CMD_SIXDOTS, BRL_CMD_PASTE, BRL_CMD_CAPBLINK, BRL_CMD_CSRJMP_VERT, \
           BRL_CMD_DISPMD, BRL_CMD_CSRTRK, BRL_CMD_ATTRVIS, BRL_CMD_CSRVIS), \
  CMDS_BAR(KEY_NONE, MOD_BAR_KLF|(mod), \
           BRL_CMD_AUTOSPEAK, BRL_CMD_AUTOREPEAT, BRL_CMD_RESTARTBRL, BRL_CMD_FREEZE, \
           BRL_CMD_INFO, BRL_CMD_PREFMENU, BRL_CMD_PREFLOAD, BRL_CMD_PREFSAVE), \
  CMDS_BAR(KEY_NONE, MOD_BAR_KRR|(mod), \
           BRL_CMD_SAY_ABOVE, BRL_CMD_SAY_BELOW, BRL_CMD_SAY_LOUDER, BRL_CMD_SAY_SOFTER, \
           BRL_CMD_MUTE, BRL_CMD_SAY_LINE, BRL_CMD_SAY_SLOWER, BRL_CMD_SAY_FASTER), \
  CMDS_BAR(KEY_NONE, MOD_BAR_KRF|(mod), \
           BRL_CMD_SPKHOME, BRL_CMD_TUNES, BRL_CMD_RESTARTSPEECH, BRL_CMD_SWSIM_BQ, \
           BRL_CMD_SKPIDLNS, BRL_CMD_SKPBLNKWINS, BRL_CMD_NOOP, BRL_CMD_SLIDEWIN), \
  {BRL_BLK_ROUTE, KEY_ROUTING1, (mod)}, \
  CMDS_BAR(KEY_ROUTING1, (mod), \
           BRL_BLK_PRINDENT, BRL_BLK_NXINDENT, BRL_BLK_SETLEFT, BRL_BLK_DESCCHAR, \
                BRL_BLK_CUTAPPEND, BRL_BLK_CUTLINE, BRL_BLK_CUTBEGIN, BRL_BLK_CUTRECT), \
  {BRL_BLK_DESCCHAR, KEY_ROUTING2, (mod)}, \
  {BRL_BLK_GOTOLINE|BRL_FLG_LINE_SCALED, KEY_STATUS2, (mod)}
#define CMDS_BAR_ALL \
  CMDS_BAR(KEY_NONE, MOD_BAR_SLC|MOD_BAR_SRC, \
           BRL_CMD_LNUP, BRL_CMD_LNDN, BRL_CMD_TOP, BRL_CMD_BOT, \
           BRL_CMD_FWINLT, BRL_CMD_FWINRT, BRL_CMD_LNBEG, BRL_CMD_LNEND), \
  CMDS_BAR(KEY_NONE, MOD_BAR_SLR|MOD_BAR_SRC, \
           BRL_CMD_PRDIFLN, BRL_CMD_NXDIFLN, BRL_CMD_ATTRUP, BRL_CMD_ATTRDN, \
           BRL_CMD_PRPROMPT, BRL_CMD_NXPROMPT, BRL_CMD_PRPGRPH, BRL_CMD_NXPGRPH), \
  CMDS_BAR(KEY_NONE, MOD_BAR_SLC|MOD_BAR_SRR, \
           BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN, BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP, BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN, \
           BRL_CMD_FWINLT|BRL_FLG_ROUTE, BRL_CMD_FWINRT|BRL_FLG_ROUTE, BRL_CMD_LNBEG|BRL_FLG_ROUTE, BRL_CMD_LNEND|BRL_FLG_ROUTE), \
  CMDS_BAR(KEY_NONE, MOD_BAR_SLR|MOD_BAR_SRR, \
           BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN, BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP, BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN, \
           BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, BRL_BLK_PASSKEY+BRL_KEY_HOME, BRL_BLK_PASSKEY+BRL_KEY_END), \
  CMDS_BAR(KEY_NONE, MOD_BAR_SLF|MOD_BAR_SRC, \
           BRL_CMD_PRSEARCH, BRL_CMD_NXSEARCH, BRL_CMD_HELP, BRL_CMD_LEARN, \
           BRL_CMD_CHRLT, BRL_CMD_CHRRT, BRL_CMD_HWINLT, BRL_CMD_HWINRT), \
  CMDS_BAR(KEY_NONE, MOD_BAR_SLC|MOD_BAR_SRF, \
           BRL_CMD_MENU_PREV_ITEM, BRL_CMD_MENU_NEXT_ITEM, BRL_CMD_MENU_FIRST_ITEM, BRL_CMD_MENU_LAST_ITEM, \
           BRL_CMD_MENU_PREV_SETTING, BRL_CMD_MENU_NEXT_SETTING, BRL_CMD_PREFLOAD, BRL_CMD_PREFSAVE), \
  CMDS_BAR_COMMON(CMDS_BAR_COMMON_ALL)

#define CMDS_BAR_COMMON_SWSIM(mod) \
  CMDS_BAR(KEY_NONE, MOD_BAR_KLF|MOD_BAR_KRF|(mod), \
           BRL_CMD_SWSIM_LC, BRL_CMD_SWSIM_RC, BRL_CMD_SWSIM_BQ, BRL_CMD_SWSIM_BC, \
           BRL_CMD_SWSIM_LR, BRL_CMD_SWSIM_RR, BRL_CMD_SWSIM_LF, BRL_CMD_SWSIM_RF)
#define CMDS_BAR_SWSIM CMDS_BAR_COMMON(CMDS_BAR_COMMON_SWSIM)


/* commands for 2 status keys */
#define CMDS_STAT_2 \
      { BRL_CMD_HELP , KEYS_STATUS + 1, 0 }, \
      { BRL_CMD_LEARN, KEYS_STATUS + 2, 0 }


/* commands for 4 status keys */
#define CMDS_STAT_4 \
      { BRL_CMD_HELP       , KEYS_STATUS + 1, 0 }, \
      { BRL_CMD_LEARN      , KEYS_STATUS + 2, 0 }, \
      { BRL_CMD_CSRJMP_VERT, KEYS_STATUS + 3, 0 }, \
      { BRL_CMD_DISPMD     , KEYS_STATUS + 4, 0 }


/* commands for 13 status keys */
#define CMDS_STAT_13(on, off) \
      CHGONOFF( BRL_CMD_HELP       , KEYS_STATUS +  1, on, off), \
              { BRL_CMD_LEARN      , KEYS_STATUS +  2, 0      }, \
              { BRL_CMD_CSRJMP_VERT, KEYS_STATUS +  3, 0      }, \
              { BRL_CMD_BACK       , KEYS_STATUS +  4, 0      }, \
      CHGONOFF( BRL_CMD_INFO       , KEYS_STATUS +  5, on, off), \
      CHGONOFF( BRL_CMD_CSRTRK     , KEYS_STATUS +  6, on, off), \
      CHGONOFF( BRL_CMD_DISPMD     , KEYS_STATUS +  7, on, off), \
      CHGONOFF( BRL_CMD_FREEZE     , KEYS_STATUS +  8, on, off), \
              { BRL_CMD_PREFMENU   , KEYS_STATUS +  9, 0      }, \
              { BRL_CMD_PREFLOAD   , KEYS_STATUS + 10, 0      }, \
      CHGONOFF( BRL_CMD_CSRVIS     , KEYS_STATUS + 11, on, off), \
      CHGONOFF( BRL_CMD_ATTRVIS    , KEYS_STATUS + 12, on, off), \
              { BRL_CMD_PASTE      , KEYS_STATUS + 13, 0      }


/* commands for 20 status keys */
#define CMDS_STAT_20(on, off) \
      CHGONOFF( BRL_CMD_HELP       , KEYS_STATUS +  1, on, off ), \
              { BRL_CMD_LEARN      , KEYS_STATUS +  2, 0X0000  }, \
              { BRL_CMD_CSRJMP_VERT, KEYS_STATUS +  3, 0X0000  }, \
              { BRL_CMD_BACK       , KEYS_STATUS +  4, 0X0000  }, \
      CHGONOFF( BRL_CMD_INFO       , KEYS_STATUS +  5, on, off ), \
      CHGONOFF( BRL_CMD_CSRTRK     , KEYS_STATUS +  6, on, off ), \
      CHGONOFF( BRL_CMD_DISPMD     , KEYS_STATUS +  7, on, off ), \
      CHGONOFF( BRL_CMD_FREEZE     , KEYS_STATUS +  8, on, off ), \
              { BRL_CMD_PREFMENU   , KEYS_STATUS +  9, 0X0000  }, \
              { BRL_CMD_PREFSAVE   , KEYS_STATUS + 10, 0X0000  }, \
              { BRL_CMD_PREFLOAD   , KEYS_STATUS + 11, 0X0000  }, \
      CHGONOFF( BRL_CMD_CSRVIS     , KEYS_STATUS + 12, on, off ), \
      CHGONOFF( BRL_CMD_ATTRVIS    , KEYS_STATUS + 13, on, off ), \
      CHGONOFF( BRL_CMD_CAPBLINK   , KEYS_STATUS + 14, on, off ), \
      CHGONOFF( BRL_CMD_SIXDOTS    , KEYS_STATUS + 15, on, off ), \
      CHGONOFF( BRL_CMD_SKPIDLNS   , KEYS_STATUS + 16, on, off ), \
      CHGONOFF( BRL_CMD_TUNES      , KEYS_STATUS + 17, on, off ), \
      CHGONOFF( BRL_CMD_AUTOSPEAK  , KEYS_STATUS + 18, on, off ), \
      CHGONOFF( BRL_CMD_AUTOREPEAT , KEYS_STATUS + 19, on, off ), \
              { BRL_CMD_PASTE      , KEYS_STATUS + 20, 0X0000  }


/* commands for 22 status keys */
#define CMDS_STAT_22(on, off) \
      CHGONOFF( BRL_CMD_HELP       , KEYS_STATUS +  1, on, off ), \
              { BRL_CMD_LEARN      , KEYS_STATUS +  2, 0X0000  }, \
              { BRL_CMD_CSRJMP_VERT, KEYS_STATUS +  3, 0X0000  }, \
              { BRL_CMD_BACK       , KEYS_STATUS +  4, 0X0000  }, \
      CHGONOFF( BRL_CMD_INFO       , KEYS_STATUS +  5, on, off ), \
      CHGONOFF( BRL_CMD_CSRTRK     , KEYS_STATUS +  6, on, off ), \
      CHGONOFF( BRL_CMD_DISPMD     , KEYS_STATUS +  7, on, off ), \
      CHGONOFF( BRL_CMD_FREEZE     , KEYS_STATUS +  8, on, off ), \
              { BRL_CMD_PREFMENU   , KEYS_STATUS +  9, 0X0000  }, \
              { BRL_CMD_PREFSAVE   , KEYS_STATUS + 10, 0X0000  }, \
              { BRL_CMD_PREFLOAD   , KEYS_STATUS + 11, 0X0000  }, \
      CHGONOFF( BRL_CMD_CSRVIS     , KEYS_STATUS + 12, on, off ), \
      CHGONOFF( BRL_CMD_ATTRVIS    , KEYS_STATUS + 13, on, off ), \
      CHGONOFF( BRL_CMD_CAPBLINK   , KEYS_STATUS + 14, on, off ), \
      CHGONOFF( BRL_CMD_SIXDOTS    , KEYS_STATUS + 15, on, off ), \
      CHGONOFF( BRL_CMD_SKPIDLNS   , KEYS_STATUS + 16, on, off ), \
      CHGONOFF( BRL_CMD_TUNES      , KEYS_STATUS + 17, on, off ), \
              { BRL_CMD_RESTARTBRL , KEYS_STATUS + 18, 0X0000  }, \
      CHGONOFF( BRL_CMD_INPUT      , KEYS_STATUS + 19, on, off ), \
      CHGONOFF( BRL_CMD_AUTOSPEAK  , KEYS_STATUS + 20, on, off ), \
      CHGONOFF( BRL_CMD_AUTOREPEAT , KEYS_STATUS + 21, on, off ), \
              { BRL_CMD_PASTE      , KEYS_STATUS + 22, 0X0000  }


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

PM_BEGIN_COMMANDS(BarBasic_Status0)
  CMDS_BAR_ALL
PM_END_COMMANDS

PM_BEGIN_COMMANDS(BarBasic_Status2)
  CMDS_BAR_ALL,
  CMDS_STAT_2
PM_END_COMMANDS

PM_BEGIN_COMMANDS(BarBasic_Status13)
  CMDS_BAR_ALL,
  CMDS_STAT_13(0X8000, 0X4000)
PM_END_COMMANDS

PM_BEGIN_COMMANDS(BarBasic_Status20)
  CMDS_BAR_ALL,
  CMDS_STAT_20(0X8000, 0X4000)
PM_END_COMMANDS

PM_BEGIN_COMMANDS(BarSimulate_Status0)
  CMDS_BAR_ALL,
  CMDS_BAR_SWSIM
PM_END_COMMANDS

PM_BEGIN_COMMANDS(BarSimulate_Status2)
  CMDS_BAR_ALL,
  CMDS_STAT_2,
  CMDS_BAR_SWSIM
PM_END_COMMANDS

PM_BEGIN_COMMANDS(BarSimulate_Status20)
  CMDS_BAR_ALL,
  CMDS_STAT_20(0X8000, 0X4000),
  CMDS_BAR_SWSIM
PM_END_COMMANDS


static TerminalDefinition pmTerminalTable[] = {
  { PM_MODEL_IDENTITY(0, c_486, "BrailleX Compact 486"),
    PM_TEXT_CELLS(40, 1),
    PM_FRONT_KEYS(9, 0)
  }
  ,
  { PM_MODEL_IDENTITY(1, 2d_l, "BrailleX 2D Lite (plus)"),
    PM_TEXT_CELLS(40, 1),
    PM_FRONT_KEYS(9, 13)
  }
  ,
  { PM_MODEL_IDENTITY(2, c, "BrailleX Compact/Tiny"),
    PM_TEXT_CELLS(40, 1),
    PM_FRONT_KEYS(9, 0)
  }
  ,
  { PM_MODEL_IDENTITY(3, 2d_s, "BrailleX 2D Screen Soft"),
    PM_TEXT_CELLS(80, 1),
    PM_FRONT_KEYS(13, 22)
  }
  ,
  { PM_MODEL_IDENTITY(6, ib_80, "BrailleX IB 80 CR Soft"),
    PM_TEXT_CELLS(80, 1),
    PM_FRONT_KEYS(9, 4)
  }
  ,
  { PM_MODEL_IDENTITY(64, el_2d_40, "BrailleX EL 2D-40"),
    PM_TEXT_CELLS(40, 1),
    PM_BAR(1, 1, 1, 1, Basic, 13)
  }
  ,
  { PM_MODEL_IDENTITY(65, el_2d_66, "BrailleX EL 2D-66"),
    PM_TEXT_CELLS(66, 1),
    PM_BAR(1, 1, 1, 1, Basic, 13)
  }
  ,
  { PM_MODEL_IDENTITY(66, el_80, "BrailleX EL 80"),
    PM_TEXT_CELLS(80, 1),
    PM_BAR(1, 1, 1, 1, Basic, 2)
  }
  ,
  { PM_MODEL_IDENTITY(67, el_2d_80, "BrailleX EL 2D-80"),
    PM_TEXT_CELLS(80, 1),
    PM_BAR(1, 1, 1, 1, Basic, 20)
  }
  ,
  { PM_MODEL_IDENTITY(68, el_40_p, "BrailleX EL 40 P"),
    PM_TEXT_CELLS(40, 1),
    PM_BAR(1, 1, 1, 0, Basic, 0)
  }
  ,
  { PM_MODEL_IDENTITY(69, elba_32, "BrailleX Elba 32"),
    PM_TEXT_CELLS(32, 1),
    PM_BAR(1, 1, 1, 1, Basic, 0)
  }
  ,
  { PM_MODEL_IDENTITY(70, elba_20, "BrailleX Elba 20"),
    PM_TEXT_CELLS(20, 1),
    PM_BAR(1, 1, 1, 1, Basic, 0)
  }
  ,
  { PM_MODEL_IDENTITY(85, el40s, "BrailleX EL40s"),
    PM_TEXT_CELLS(40, 1),
    PM_BAR(0, 0, 1, 1, Simulate, 0)
  }
  ,
  { PM_MODEL_IDENTITY(86, el80_ii, "BrailleX EL80-II"),
    PM_TEXT_CELLS(80, 1),
    PM_BAR(0, 0, 1, 1, Simulate, 2)
  }
  ,
  { PM_MODEL_IDENTITY(87, el66s, "BrailleX EL66s"),
    PM_TEXT_CELLS(66, 1),
    PM_BAR(0, 0, 1, 1, Simulate, 0)
  }
  ,
  { PM_MODEL_IDENTITY(88, el80s, "BrailleX EL80s"),
    PM_TEXT_CELLS(80, 1),
    PM_BAR(0, 0, 1, 1, Simulate, 0)
  }
  ,
  { PM_MODEL_IDENTITY(89, trio, "BrailleX Trio"),
    .protocolRevision = 2,
    PM_TEXT_CELLS(40, 1),
    PM_BAR(0, 0, 1, 1, Simulate, 0)
  }
  ,
  { PM_MODEL_IDENTITY(90, el70s, "BrailleX EL70s"),
    PM_TEXT_CELLS(70, 1),
    PM_BAR(0, 0, 1, 1, Simulate, 0)
  }
  ,
  { PM_MODEL_IDENTITY(91, el2d_80s, "BrailleX EL2D-80s"),
    PM_TEXT_CELLS(80, 1),
    PM_BAR(0, 0, 1, 1, Simulate, 20)
  }
  ,
  { PM_MODEL_IDENTITY(92, elba_trio_20, "BrailleX Elba (Trio 20)"),
    .protocolRevision = 2,
    PM_TEXT_CELLS(20, 1),
    PM_BAR(0, 0, 1, 1, Simulate, 0)
  }
  ,
  { PM_MODEL_IDENTITY(93, elba_trio_32, "BrailleX Elba (Trio 32)"),
    .protocolRevision = 2,
    PM_TEXT_CELLS(32, 1),
    PM_BAR(0, 0, 1, 1, Simulate, 0)
  }
};

static TerminalDefinition *pmTerminals = pmTerminalTable;
static int pmTerminalCount = ARRAY_COUNT(pmTerminalTable);
static int pmTerminalsAllocated = 0;
