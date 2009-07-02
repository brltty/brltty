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

#define OFFS_EMPTY       0
#define OFFS_HORIZ    1000	/* added to status show */
#define OFFS_FLAG     2000
#define OFFS_NUMBER   3000

#define PM_BEGIN_STATUS(count) static const uint16_t pmStatus_##count[] = {
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
  OFFS_EMPTY,
  OFFS_FLAG  + BRL_GSC_AUTOSPEAK,
  OFFS_FLAG  + BRL_GSC_AUTOREPEAT,
  OFFS_EMPTY
PM_END_STATUS


typedef struct {
  unsigned char modelIdentifier;
  unsigned char protocolRevision;
  const char *modelName;
  const char *keyBindings;
  const KeyNameEntry *const *keyNameTables;

  uint8_t textColumns;
  uint8_t frontKeys;
  uint8_t hasBar;
  uint8_t leftSwitches;
  uint8_t rightSwitches;
  uint8_t leftKeys;
  uint8_t rightKeys;
  uint8_t statusCount;

  const uint16_t *statusCells;
} ModelEntry; 

#define PM_MODEL_IDENTITY(identifier, model, name) \
  .modelIdentifier = identifier, \
  .modelName = name, \
  .protocolRevision = 1, \
  .keyBindings = #model, \
  .keyNameTables = keyNameTables_##model

#define PM_CELL_COUNTS(columns, status) \
  .textColumns = columns, \
  .statusCells = pmStatus_##status, \
  .statusCount = status

#define PM_FRONT_KEYS(front) \
  .frontKeys = front

#define PM_BAR(ls, rs, lk, rk) \
  .hasBar = 1, \
  .leftSwitches = ls, \
  .rightSwitches = rs, \
  .leftKeys = lk, \
  .rightKeys = rk


static KEY_NAME_TABLE(keyNames_bar) = {
  KEY_NAME_ENTRY(PM_KEY_BarLeft1, "BarLeft1"),
  KEY_NAME_ENTRY(PM_KEY_BarLeft2, "BarLeft2"),
  KEY_NAME_ENTRY(PM_KEY_BarRight1, "BarRight1"),
  KEY_NAME_ENTRY(PM_KEY_BarRight2, "BarRight2"),
  KEY_NAME_ENTRY(PM_KEY_BarUp1, "BarUp1"),
  KEY_NAME_ENTRY(PM_KEY_BarUp2, "BarUp2"),
  KEY_NAME_ENTRY(PM_KEY_BarDown1, "BarDown1"),
  KEY_NAME_ENTRY(PM_KEY_BarDown2, "BarDown2"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_switches) = {
  KEY_NAME_ENTRY(PM_KEY_LeftSwitchRear, "LeftSwitchRear"),
  KEY_NAME_ENTRY(PM_KEY_LeftSwitchFront, "LeftSwitchFront"),
  KEY_NAME_ENTRY(PM_KEY_RightSwitchRear, "RightSwitchRear"),
  KEY_NAME_ENTRY(PM_KEY_RightSwitchFront, "RightSwitchFront"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_keys) = {
  KEY_NAME_ENTRY(PM_KEY_LeftKeyRear, "LeftKeyRear"),
  KEY_NAME_ENTRY(PM_KEY_LeftKeyFront, "LeftKeyFront"),
  KEY_NAME_ENTRY(PM_KEY_RightKeyRear, "RightKeyRear"),
  KEY_NAME_ENTRY(PM_KEY_RightKeyFront, "RightKeyFront"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_front9) = {
  KEY_NAME_ENTRY(PM_KEY_FRONT+0, "Function"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+1, "Cursor"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+2, "Backward"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+3, "Up"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+4, "Home"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+5, "Down"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+6, "Forward"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+7, "Braille"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+8, "Attribute"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_front13) = {
  KEY_NAME_ENTRY(PM_KEY_FRONT+0, "Dot7"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+1, "Dot3"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+2, "Dot2"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+3, "Dot1"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+4, "Up"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+5, "Home"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+6, "Shift"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+7, "End"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+8, "Down"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+9, "Dot4"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+10, "Dot5"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+11, "Dot6"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+12, "Dot8"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_status) = {
  KEY_NAME_ENTRY(PM_KEY_STATUS+21, "Status22"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+20, "Status21"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+19, "Status20"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+18, "Status19"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+17, "Status18"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+16, "Status17"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+15, "Status16"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+14, "Status15"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+13, "Status14"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+12, "Status13"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+11, "Status12"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+10, "Status11"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+9, "Status10"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+8, "Status9"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+7, "Status8"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+6, "Status7"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+5, "Status6"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+4, "Status5"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+3, "Status4"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+2, "Status3"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+1, "Status2"),
  KEY_NAME_ENTRY(PM_KEY_STATUS+0, "Status1"),

  LAST_KEY_NAME_ENTRY
};

#define PM_STATUS_KEYS(count) (keyNames_status + (ARRAY_COUNT(keyNames_status) - 1 - (count)))
#define keyNames_status2 PM_STATUS_KEYS(2)
#define keyNames_status4 PM_STATUS_KEYS(4)
#define keyNames_status13 PM_STATUS_KEYS(13)
#define keyNames_status20 PM_STATUS_KEYS(20)
#define keyNames_status22 PM_STATUS_KEYS(22)

static KEY_NAME_TABLE(keyNames_keyboard) = {
  KEY_NAME_ENTRY(PM_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(PM_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(PM_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(PM_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(PM_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(PM_KEY_Dot6, "Dot6"),
  KEY_NAME_ENTRY(PM_KEY_Dot7, "Dot7"),
  KEY_NAME_ENTRY(PM_KEY_Dot8, "Dot8"),

  KEY_NAME_ENTRY(PM_KEY_Space, "Space"),
  KEY_NAME_ENTRY(PM_KEY_LeftSpace, "LeftSpace"),
  KEY_NAME_ENTRY(PM_KEY_RightSpace, "RightSpace"),
  KEY_NAME_ENTRY(PM_KEY_LeftThumb, "LeftThumb"),
  KEY_NAME_ENTRY(PM_KEY_RightThumb, "RightThumb"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_routingKeys1) = {
  KEY_SET_ENTRY(PM_SET_RoutingKeys1, "RoutingKey1"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_routingKeys2) = {
  KEY_SET_ENTRY(PM_SET_RoutingKeys2, "RoutingKey2"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_statusKeys2) = {
  KEY_SET_ENTRY(PM_SET_StatusKeys2, "StatusKey2"),

  LAST_KEY_NAME_ENTRY
};


static KEY_NAME_TABLE_LIST(keyNameTables_c_486) = {
  keyNames_front9,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_2d_l) = {
  keyNames_front9,
  keyNames_status13,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_c) = {
  keyNames_front9,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_2d_s) = {
  keyNames_front13,
  keyNames_status22,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_ib_80) = {
  keyNames_front9,
  keyNames_status4,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el_2d_40) = {
  keyNames_bar,
  keyNames_switches,
  keyNames_keys,
  keyNames_status13,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el_2d_66) = {
  keyNames_bar,
  keyNames_switches,
  keyNames_keys,
  keyNames_status13,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el_80) = {
  keyNames_bar,
  keyNames_switches,
  keyNames_keys,
  keyNames_status2,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el_2d_80) = {
  keyNames_bar,
  keyNames_switches,
  keyNames_keys,
  keyNames_status20,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el_40_p) = {
  keyNames_bar,
  keyNames_switches,
  keyNames_keys,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_elba_32) = {
  keyNames_bar,
  keyNames_switches,
  keyNames_keys,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_elba_20) = {
  keyNames_bar,
  keyNames_switches,
  keyNames_keys,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el40s) = {
  keyNames_bar,
  keyNames_keys,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el80_ii) = {
  keyNames_bar,
  keyNames_keys,
  keyNames_status2,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el66s) = {
  keyNames_bar,
  keyNames_keys,
  keyNames_routingKeys1,
  keyNames_routingKeys2,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el80s) = {
  keyNames_bar,
  keyNames_keys,
  keyNames_routingKeys1,
  keyNames_routingKeys2,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_trio) = {
  keyNames_bar,
  keyNames_keys,
  keyNames_keyboard,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el70s) = {
  keyNames_bar,
  keyNames_keys,
  keyNames_routingKeys1,
  keyNames_routingKeys2,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_el2d_80s) = {
  keyNames_bar,
  keyNames_keys,
  keyNames_status20,
  keyNames_routingKeys1,
  keyNames_routingKeys2,
  keyNames_statusKeys2,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_elba_trio_20) = {
  keyNames_bar,
  keyNames_keys,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_elba_trio_32) = {
  keyNames_bar,
  keyNames_keys,
  keyNames_routingKeys1,
  NULL
};


static const ModelEntry modelTable[] = {
  { PM_MODEL_IDENTITY(0, c_486, "BrailleX Compact 486"),
    PM_CELL_COUNTS(40, 0),
    PM_FRONT_KEYS(9)
  }
  ,
  { PM_MODEL_IDENTITY(1, 2d_l, "BrailleX 2D Lite (plus)"),
    PM_CELL_COUNTS(40, 13),
    PM_FRONT_KEYS(9)
  }
  ,
  { PM_MODEL_IDENTITY(2, c, "BrailleX Compact/Tiny"),
    PM_CELL_COUNTS(40, 0),
    PM_FRONT_KEYS(9)
  }
  ,
  { PM_MODEL_IDENTITY(3, 2d_s, "BrailleX 2D Screen Soft"),
    PM_CELL_COUNTS(80, 22),
    PM_FRONT_KEYS(13)
  }
  ,
  { PM_MODEL_IDENTITY(6, ib_80, "BrailleX IB 80 CR Soft"),
    PM_CELL_COUNTS(80, 4),
    PM_FRONT_KEYS(9)
  }
  ,
  { PM_MODEL_IDENTITY(64, el_2d_40, "BrailleX EL 2D-40"),
    PM_CELL_COUNTS(40, 13),
    PM_BAR(1, 1, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(65, el_2d_66, "BrailleX EL 2D-66"),
    PM_CELL_COUNTS(66, 13),
    PM_BAR(1, 1, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(66, el_80, "BrailleX EL 80"),
    PM_CELL_COUNTS(80, 2),
    PM_BAR(1, 1, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(67, el_2d_80, "BrailleX EL 2D-80"),
    PM_CELL_COUNTS(80, 20),
    PM_BAR(1, 1, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(68, el_40_p, "BrailleX EL 40 P"),
    PM_CELL_COUNTS(40, 0),
    PM_BAR(1, 1, 1, 0)
  }
  ,
  { PM_MODEL_IDENTITY(69, elba_32, "BrailleX Elba 32"),
    PM_CELL_COUNTS(32, 0),
    PM_BAR(1, 1, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(70, elba_20, "BrailleX Elba 20"),
    PM_CELL_COUNTS(20, 0),
    PM_BAR(1, 1, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(85, el40s, "BrailleX EL40s"),
    PM_CELL_COUNTS(40, 0),
    PM_BAR(0, 0, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(86, el80_ii, "BrailleX EL80-II"),
    PM_CELL_COUNTS(80, 2),
    PM_BAR(0, 0, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(87, el66s, "BrailleX EL66s"),
    PM_CELL_COUNTS(66, 0),
    PM_BAR(0, 0, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(88, el80s, "BrailleX EL80s"),
    PM_CELL_COUNTS(80, 0),
    PM_BAR(0, 0, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(89, trio, "BrailleX Trio"),
    .protocolRevision = 2,
    PM_CELL_COUNTS(40, 0),
    PM_BAR(0, 0, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(90, el70s, "BrailleX EL70s"),
    PM_CELL_COUNTS(70, 0),
    PM_BAR(0, 0, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(91, el2d_80s, "BrailleX EL2D-80s"),
    PM_CELL_COUNTS(80, 20),
    PM_BAR(0, 0, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(92, elba_trio_20, "BrailleX Elba (Trio 20)"),
    .protocolRevision = 2,
    PM_CELL_COUNTS(20, 0),
    PM_BAR(0, 0, 1, 1)
  }
  ,
  { PM_MODEL_IDENTITY(93, elba_trio_32, "BrailleX Elba (Trio 32)"),
    .protocolRevision = 2,
    PM_CELL_COUNTS(32, 0),
    PM_BAR(0, 0, 1, 1)
  }
};

static const unsigned int modelCount = ARRAY_COUNT(modelTable);
