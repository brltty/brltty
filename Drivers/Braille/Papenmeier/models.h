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
  const KeyTableDefinition *keyTableDefinition;

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
  .keyTableDefinition = &KEY_TABLE_DEFINITION(model)

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


BEGIN_KEY_NAME_TABLE(bar)
  KEY_NAME_ENTRY(PM_KEY_BarLeft1, "BarLeft1"),
  KEY_NAME_ENTRY(PM_KEY_BarLeft2, "BarLeft2"),
  KEY_NAME_ENTRY(PM_KEY_BarRight1, "BarRight1"),
  KEY_NAME_ENTRY(PM_KEY_BarRight2, "BarRight2"),
  KEY_NAME_ENTRY(PM_KEY_BarUp1, "BarUp1"),
  KEY_NAME_ENTRY(PM_KEY_BarUp2, "BarUp2"),
  KEY_NAME_ENTRY(PM_KEY_BarDown1, "BarDown1"),
  KEY_NAME_ENTRY(PM_KEY_BarDown2, "BarDown2"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(switches)
  KEY_NAME_ENTRY(PM_KEY_LeftSwitchRear, "LeftSwitchRear"),
  KEY_NAME_ENTRY(PM_KEY_LeftSwitchFront, "LeftSwitchFront"),
  KEY_NAME_ENTRY(PM_KEY_RightSwitchRear, "RightSwitchRear"),
  KEY_NAME_ENTRY(PM_KEY_RightSwitchFront, "RightSwitchFront"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(keys)
  KEY_NAME_ENTRY(PM_KEY_LeftKeyRear, "LeftKeyRear"),
  KEY_NAME_ENTRY(PM_KEY_LeftKeyFront, "LeftKeyFront"),
  KEY_NAME_ENTRY(PM_KEY_RightKeyRear, "RightKeyRear"),
  KEY_NAME_ENTRY(PM_KEY_RightKeyFront, "RightKeyFront"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(front9)
  KEY_NAME_ENTRY(PM_KEY_FRONT+0, "Function"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+1, "Cursor"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+2, "Backward"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+3, "Up"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+4, "Home"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+5, "Down"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+6, "Forward"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+7, "Braille"),
  KEY_NAME_ENTRY(PM_KEY_FRONT+8, "Attribute"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(front13)
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
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(status)
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
END_KEY_NAME_TABLE
#define STATUS_KEY_TABLE(count) (KEY_NAME_TABLE(status) + (ARRAY_COUNT(KEY_NAME_TABLE(status)) - 1 - (count)))

BEGIN_KEY_NAME_TABLE(keyboard)
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
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routingKeys1)
  KEY_SET_ENTRY(PM_SET_RoutingKeys1, "RoutingKey1"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routingKeys2)
  KEY_SET_ENTRY(PM_SET_RoutingKeys2, "RoutingKey2"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(statusKeys2)
  KEY_SET_ENTRY(PM_SET_StatusKeys2, "StatusKey2"),
END_KEY_NAME_TABLE


BEGIN_KEY_NAME_TABLES(c_486)
  KEY_NAME_TABLE(front9),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(2d_l)
  KEY_NAME_TABLE(front9),
  STATUS_KEY_TABLE(13),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(c)
  KEY_NAME_TABLE(front9),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(2d_s)
  KEY_NAME_TABLE(front13),
  STATUS_KEY_TABLE(22),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(ib_80)
  KEY_NAME_TABLE(front9),
  STATUS_KEY_TABLE(4),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el_2d_40)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(switches),
  KEY_NAME_TABLE(keys),
  STATUS_KEY_TABLE(13),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el_2d_66)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(switches),
  KEY_NAME_TABLE(keys),
  STATUS_KEY_TABLE(13),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el_80)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(switches),
  KEY_NAME_TABLE(keys),
  STATUS_KEY_TABLE(2),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el_2d_80)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(switches),
  KEY_NAME_TABLE(keys),
  STATUS_KEY_TABLE(20),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el_40_p)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(switches),
  KEY_NAME_TABLE(keys),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(elba_32)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(switches),
  KEY_NAME_TABLE(keys),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(elba_20)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(switches),
  KEY_NAME_TABLE(keys),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el40s)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(keys),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el80_ii)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(keys),
  STATUS_KEY_TABLE(2),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el66s)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(keys),
  KEY_NAME_TABLE(routingKeys1),
  KEY_NAME_TABLE(routingKeys2),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el80s)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(keys),
  KEY_NAME_TABLE(routingKeys1),
  KEY_NAME_TABLE(routingKeys2),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(trio)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(keys),
  KEY_NAME_TABLE(keyboard),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el70s)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(keys),
  KEY_NAME_TABLE(routingKeys1),
  KEY_NAME_TABLE(routingKeys2),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(el2d_80s)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(keys),
  STATUS_KEY_TABLE(20),
  KEY_NAME_TABLE(routingKeys1),
  KEY_NAME_TABLE(routingKeys2),
  KEY_NAME_TABLE(statusKeys2),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(elba_trio_20)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(keys),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(elba_trio_32)
  KEY_NAME_TABLE(bar),
  KEY_NAME_TABLE(keys),
  KEY_NAME_TABLE(routingKeys1),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(c_486)
DEFINE_KEY_TABLE(2d_l)
DEFINE_KEY_TABLE(c)
DEFINE_KEY_TABLE(2d_s)
DEFINE_KEY_TABLE(ib_80)
DEFINE_KEY_TABLE(el_2d_40)
DEFINE_KEY_TABLE(el_2d_66)
DEFINE_KEY_TABLE(el_80)
DEFINE_KEY_TABLE(el_2d_80)
DEFINE_KEY_TABLE(el_40_p)
DEFINE_KEY_TABLE(elba_32)
DEFINE_KEY_TABLE(elba_20)
DEFINE_KEY_TABLE(el40s)
DEFINE_KEY_TABLE(el80_ii)
DEFINE_KEY_TABLE(el66s)
DEFINE_KEY_TABLE(el80s)
DEFINE_KEY_TABLE(trio)
DEFINE_KEY_TABLE(el70s)
DEFINE_KEY_TABLE(el2d_80s)
DEFINE_KEY_TABLE(elba_trio_20)
DEFINE_KEY_TABLE(elba_trio_32)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(c_486),
  &KEY_TABLE_DEFINITION(2d_l),
  &KEY_TABLE_DEFINITION(c),
  &KEY_TABLE_DEFINITION(2d_s),
  &KEY_TABLE_DEFINITION(ib_80),
  &KEY_TABLE_DEFINITION(el_2d_40),
  &KEY_TABLE_DEFINITION(el_2d_66),
  &KEY_TABLE_DEFINITION(el_80),
  &KEY_TABLE_DEFINITION(el_2d_80),
  &KEY_TABLE_DEFINITION(el_40_p),
  &KEY_TABLE_DEFINITION(elba_32),
  &KEY_TABLE_DEFINITION(elba_20),
  &KEY_TABLE_DEFINITION(el40s),
  &KEY_TABLE_DEFINITION(el80_ii),
  &KEY_TABLE_DEFINITION(el66s),
  &KEY_TABLE_DEFINITION(el80s),
  &KEY_TABLE_DEFINITION(trio),
  &KEY_TABLE_DEFINITION(el70s),
  &KEY_TABLE_DEFINITION(el2d_80s),
  &KEY_TABLE_DEFINITION(elba_trio_20),
  &KEY_TABLE_DEFINITION(elba_trio_32),
END_KEY_TABLE_LIST


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
