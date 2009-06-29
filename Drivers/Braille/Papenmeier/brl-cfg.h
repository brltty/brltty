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


typedef struct {
  unsigned char modelIdentifier;
  unsigned char protocolRevision;
  char *modelName;
  char *keyBindings;
  const KeyNameEntry *const *keyNameTables;

  uint8_t textColumns;
  uint8_t textRows;
  uint8_t frontKeys;
  uint8_t hasBar;
  uint8_t leftSwitches;
  uint8_t rightSwitches;
  uint8_t leftKeys;
  uint8_t rightKeys;
  uint8_t statusCount;

  uint16_t *statusCells;
} ModelEntry; 

#define PM_MODEL_IDENTITY(identifier, model, name) \
  .modelIdentifier = identifier, \
  .modelName = name, \
  .protocolRevision = 1, \
  .keyBindings = #model

#define PM_TEXT_CELLS(columns, rows) \
  .textColumns = columns, \
  .textRows = rows

#define PM_FRONT_KEYS(f, s) \
  .frontKeys = f, \
  PM_MODEL_TABLES(front##f, s)

#define PM_BAR(ls, rs, lk, rk, s) \
  .hasBar = 1, \
  .leftSwitches = ls, \
  .rightSwitches = rs, \
  .leftKeys = lk, \
  .rightKeys = rk, \
  PM_MODEL_TABLES(bar, s)

#define PM_MODEL_TABLES(k, s) \
  .statusCells = pmStatus_##s, \
  .statusCount = s, \
  .keyNameTables = keyNameTables_##k##_status##s


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
  OFFS_EMPTY,
  OFFS_FLAG  + BRL_GSC_AUTOSPEAK,
  OFFS_FLAG  + BRL_GSC_AUTOREPEAT,
  OFFS_EMPTY
PM_END_STATUS


static KEY_NAME_TABLE(keyNames_bar) = {
  KEY_NAME_ENTRY(PM_KEY_BarLeft1, "BarLeft1"),
  KEY_NAME_ENTRY(PM_KEY_BarLeft2, "BarLeft2"),
  KEY_NAME_ENTRY(PM_KEY_BarRight1, "BarRight1"),
  KEY_NAME_ENTRY(PM_KEY_BarRight2, "BarRight2"),
  KEY_NAME_ENTRY(PM_KEY_BarUp1, "BarUp1"),
  KEY_NAME_ENTRY(PM_KEY_BarUp2, "BarUp2"),
  KEY_NAME_ENTRY(PM_KEY_BarDown1, "BarDown1"),
  KEY_NAME_ENTRY(PM_KEY_BarDown2, "BarDown2"),

  KEY_NAME_ENTRY(PM_KEY_LeftSwitchRear, "LeftSwitchRear"),
  KEY_NAME_ENTRY(PM_KEY_LeftSwitchFront, "LeftSwitchFront"),
  KEY_NAME_ENTRY(PM_KEY_RightSwitchRear, "RightSwitchRear"),
  KEY_NAME_ENTRY(PM_KEY_RightSwitchFront, "RightSwitchFront"),
  KEY_NAME_ENTRY(PM_KEY_LeftKeyRear, "LeftKeyRear"),
  KEY_NAME_ENTRY(PM_KEY_LeftKeyFront, "LeftKeyFront"),
  KEY_NAME_ENTRY(PM_KEY_RightKeyRear, "RightKeyRear"),
  KEY_NAME_ENTRY(PM_KEY_RightKeyFront, "RightKeyFront"),

  KEY_SET_ENTRY(PM_SET_RoutingKeys2, "RoutingKey2"),
  KEY_SET_ENTRY(PM_SET_StatusKeys2, "StatusKey2"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE(keyNames_front9) = {
  KEY_NAME_ENTRY(PM_KEY_FRONT+0, "Function"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+1, "Cursor"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+2, "Back"),
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

static KEY_NAME_TABLE(keyNames_routingKeys1) = {
  KEY_SET_ENTRY(PM_SET_RoutingKeys1, "RoutingKey1"),

  LAST_KEY_NAME_ENTRY
};

static KEY_NAME_TABLE_LIST(keyNameTables_front9_status0) = {
  keyNames_front9,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_front9_status4) = {
  keyNames_front9,
  keyNames_status4,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_front9_status13) = {
  keyNames_front9,
  keyNames_status13,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_front13_status22) = {
  keyNames_front13,
  keyNames_status22,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_bar_status0) = {
  keyNames_bar,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_bar_status2) = {
  keyNames_bar,
  keyNames_status2,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_bar_status13) = {
  keyNames_bar,
  keyNames_status13,
  keyNames_routingKeys1,
  NULL
};

static KEY_NAME_TABLE_LIST(keyNameTables_bar_status20) = {
  keyNames_bar,
  keyNames_status20,
  keyNames_routingKeys1,
  NULL
};


static const ModelEntry modelTable[] = {
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
    PM_BAR(1, 1, 1, 1, 13)
  }
  ,
  { PM_MODEL_IDENTITY(65, el_2d_66, "BrailleX EL 2D-66"),
    PM_TEXT_CELLS(66, 1),
    PM_BAR(1, 1, 1, 1, 13)
  }
  ,
  { PM_MODEL_IDENTITY(66, el_80, "BrailleX EL 80"),
    PM_TEXT_CELLS(80, 1),
    PM_BAR(1, 1, 1, 1, 2)
  }
  ,
  { PM_MODEL_IDENTITY(67, el_2d_80, "BrailleX EL 2D-80"),
    PM_TEXT_CELLS(80, 1),
    PM_BAR(1, 1, 1, 1, 20)
  }
  ,
  { PM_MODEL_IDENTITY(68, el_40_p, "BrailleX EL 40 P"),
    PM_TEXT_CELLS(40, 1),
    PM_BAR(1, 1, 1, 0, 0)
  }
  ,
  { PM_MODEL_IDENTITY(69, elba_32, "BrailleX Elba 32"),
    PM_TEXT_CELLS(32, 1),
    PM_BAR(1, 1, 1, 1, 0)
  }
  ,
  { PM_MODEL_IDENTITY(70, elba_20, "BrailleX Elba 20"),
    PM_TEXT_CELLS(20, 1),
    PM_BAR(1, 1, 1, 1, 0)
  }
  ,
  { PM_MODEL_IDENTITY(85, el40s, "BrailleX EL40s"),
    PM_TEXT_CELLS(40, 1),
    PM_BAR(0, 0, 1, 1, 0)
  }
  ,
  { PM_MODEL_IDENTITY(86, el80_ii, "BrailleX EL80-II"),
    PM_TEXT_CELLS(80, 1),
    PM_BAR(0, 0, 1, 1, 2)
  }
  ,
  { PM_MODEL_IDENTITY(87, el66s, "BrailleX EL66s"),
    PM_TEXT_CELLS(66, 1),
    PM_BAR(0, 0, 1, 1, 0)
  }
  ,
  { PM_MODEL_IDENTITY(88, el80s, "BrailleX EL80s"),
    PM_TEXT_CELLS(80, 1),
    PM_BAR(0, 0, 1, 1, 0)
  }
  ,
  { PM_MODEL_IDENTITY(89, trio, "BrailleX Trio"),
    .protocolRevision = 2,
    PM_TEXT_CELLS(40, 1),
    PM_BAR(0, 0, 1, 1, 0)
  }
  ,
  { PM_MODEL_IDENTITY(90, el70s, "BrailleX EL70s"),
    PM_TEXT_CELLS(70, 1),
    PM_BAR(0, 0, 1, 1, 0)
  }
  ,
  { PM_MODEL_IDENTITY(91, el2d_80s, "BrailleX EL2D-80s"),
    PM_TEXT_CELLS(80, 1),
    PM_BAR(0, 0, 1, 1, 20)
  }
  ,
  { PM_MODEL_IDENTITY(92, elba_trio_20, "BrailleX Elba (Trio 20)"),
    .protocolRevision = 2,
    PM_TEXT_CELLS(20, 1),
    PM_BAR(0, 0, 1, 1, 0)
  }
  ,
  { PM_MODEL_IDENTITY(93, elba_trio_32, "BrailleX Elba (Trio 32)"),
    .protocolRevision = 2,
    PM_TEXT_CELLS(32, 1),
    PM_BAR(0, 0, 1, 1, 0)
  }
};

static const unsigned int modelCount = ARRAY_COUNT(modelTable);
