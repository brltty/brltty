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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "misc.h"
#include "io_defs.h"

typedef enum {
  PARM_PROTOCOLS,
  PARM_VARIOKEYS
} DriverParameter;
#define BRLPARMS "protocols", "variokeys"

#define BRLSTAT ST_TiemanStyle
#define BRL_HAVE_STATUS_CELLS
#define BRL_HAVE_PACKET_IO
#include "brl_driver.h"
#include "brldefs-bm.h"

BEGIN_KEY_NAME_TABLE(display)
  KEY_NAME_ENTRY(BM_KEY_DISPLAY+0, "Display1"),
  KEY_NAME_ENTRY(BM_KEY_DISPLAY+1, "Display2"),
  KEY_NAME_ENTRY(BM_KEY_DISPLAY+2, "Display3"),
  KEY_NAME_ENTRY(BM_KEY_DISPLAY+3, "Display4"),
  KEY_NAME_ENTRY(BM_KEY_DISPLAY+4, "Display5"),
  KEY_NAME_ENTRY(BM_KEY_DISPLAY+5, "Display6"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(command)
  KEY_NAME_ENTRY(BM_KEY_COMMAND+0, "Command1"),
  KEY_NAME_ENTRY(BM_KEY_COMMAND+1, "Command2"),
  KEY_NAME_ENTRY(BM_KEY_COMMAND+2, "Command3"),
  KEY_NAME_ENTRY(BM_KEY_COMMAND+3, "Command4"),
  KEY_NAME_ENTRY(BM_KEY_COMMAND+4, "Command5"),
  KEY_NAME_ENTRY(BM_KEY_COMMAND+5, "Command6"),
  KEY_NAME_ENTRY(BM_KEY_COMMAND+6, "Command7"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(rockers)
  KEY_NAME_ENTRY(BM_KEY_FRONT_ROCKERS+0, "FrontRightDown"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_ROCKERS+1, "FrontRightUp"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_ROCKERS+2, "FrontMiddleDown"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_ROCKERS+3, "FrontMiddleUp"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_ROCKERS+4, "FrontLeftDown"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_ROCKERS+5, "FrontLeftUp"),

  KEY_NAME_ENTRY(BM_KEY_BACK_ROCKERS+0, "BackRightDown"),
  KEY_NAME_ENTRY(BM_KEY_BACK_ROCKERS+1, "BackRightUp"),
  KEY_NAME_ENTRY(BM_KEY_BACK_ROCKERS+2, "BackMiddleDown"),
  KEY_NAME_ENTRY(BM_KEY_BACK_ROCKERS+3, "BackMiddleUp"),
  KEY_NAME_ENTRY(BM_KEY_BACK_ROCKERS+4, "BackLeftDown"),
  KEY_NAME_ENTRY(BM_KEY_BACK_ROCKERS+5, "BackLeftUp"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(buttons)
  KEY_NAME_ENTRY(BM_KEY_FRONT_BUTTONS+0, "Front10"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_BUTTONS+1, "Front9"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_BUTTONS+2, "Front8"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_BUTTONS+3, "Front7"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_BUTTONS+4, "Front6"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_BUTTONS+5, "Front5"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_BUTTONS+6, "Front4"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_BUTTONS+7, "Front3"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_BUTTONS+8, "Front2"),
  KEY_NAME_ENTRY(BM_KEY_FRONT_BUTTONS+9, "Front1"),

  KEY_NAME_ENTRY(BM_KEY_BACK_BUTTONS+0, "Back10"),
  KEY_NAME_ENTRY(BM_KEY_BACK_BUTTONS+1, "Back9"),
  KEY_NAME_ENTRY(BM_KEY_BACK_BUTTONS+2, "Back8"),
  KEY_NAME_ENTRY(BM_KEY_BACK_BUTTONS+3, "Back7"),
  KEY_NAME_ENTRY(BM_KEY_BACK_BUTTONS+4, "Back6"),
  KEY_NAME_ENTRY(BM_KEY_BACK_BUTTONS+5, "Back5"),
  KEY_NAME_ENTRY(BM_KEY_BACK_BUTTONS+6, "Back4"),
  KEY_NAME_ENTRY(BM_KEY_BACK_BUTTONS+7, "Back3"),
  KEY_NAME_ENTRY(BM_KEY_BACK_BUTTONS+8, "Back2"),
  KEY_NAME_ENTRY(BM_KEY_BACK_BUTTONS+9, "Back1"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(entry)
  KEY_NAME_ENTRY(BM_KEY_ENTRY+0, "B9"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+1, "B10"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+2, "B11"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+4, "F1"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+5, "F2"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+6, "F3"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+7, "F4"),

  KEY_NAME_ENTRY(BM_KEY_ENTRY+8, "Dot1"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+9, "Dot2"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+10, "Dot3"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+11, "Dot4"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+12, "Dot5"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+13, "Dot6"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+14, "Dot7"),
  KEY_NAME_ENTRY(BM_KEY_ENTRY+15, "Dot8"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(joystick)
  KEY_NAME_ENTRY(BM_KEY_JOYSTICK+0, "Up"),
  KEY_NAME_ENTRY(BM_KEY_JOYSTICK+1, "Left"),
  KEY_NAME_ENTRY(BM_KEY_JOYSTICK+2, "Down"),
  KEY_NAME_ENTRY(BM_KEY_JOYSTICK+3, "Right"),
  KEY_NAME_ENTRY(BM_KEY_JOYSTICK+4, "Press"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routing)
  KEY_SET_ENTRY(BM_SET_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(horizontal)
  KEY_SET_ENTRY(BM_SET_HorizontalSensors, "HorizontalSensor"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(vertical)
  KEY_SET_ENTRY(BM_SET_LeftSensors, "LeftSensor"),
  KEY_SET_ENTRY(BM_SET_RightSensors, "RightSensor"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(display),
  KEY_NAME_TABLE(command),
  KEY_NAME_TABLE(rockers),
  KEY_NAME_TABLE(buttons),
  KEY_NAME_TABLE(entry),
  KEY_NAME_TABLE(joystick),
  KEY_NAME_TABLE(routing),
  KEY_NAME_TABLE(horizontal),
  KEY_NAME_TABLE(vertical),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(vario)
  KEY_NAME_TABLE(display),
  KEY_NAME_TABLE(command),
  KEY_NAME_TABLE(rockers),
  KEY_NAME_TABLE(routing),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)
DEFINE_KEY_TABLE(vario)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
  &KEY_TABLE_DEFINITION(vario),
END_KEY_TABLE_LIST

/* Global Definitions */

static unsigned int useVarioKeys;
static const int probeLimit = 2;
static const int probeTimeout = 200;

#define KEY_GROUP_SIZE(count) (((count) + 7) / 8)
#define MAXIMUM_CELL_COUNT 84
#define VERTICAL_SENSOR_COUNT 27

static int cellCount;
static int cellsUpdated;
static unsigned char internalCells[MAXIMUM_CELL_COUNT];
static unsigned char externalCells[MAXIMUM_CELL_COUNT];
static TranslationTable outputTable;

typedef struct {
  unsigned char navigationKeys[KEY_GROUP_SIZE(BM_KEY_COUNT)];
  unsigned char routingKeys[KEY_GROUP_SIZE(MAXIMUM_CELL_COUNT)];
  unsigned char horizontalSensors[KEY_GROUP_SIZE(MAXIMUM_CELL_COUNT)];
  unsigned char leftSensors[KEY_GROUP_SIZE(VERTICAL_SENSOR_COUNT)];
  unsigned char rightSensors[KEY_GROUP_SIZE(VERTICAL_SENSOR_COUNT)];
} KeysState;

static KeysState keysState;
static unsigned char switchSettings;

typedef struct {
  const char *name;
  int serialBaud;
  SerialParity serialParity;
  DotsTable dotsTable;
  int (*readPacket) (BrailleDisplay *brl, unsigned char *packet, int size);
  int (*writePacket) (BrailleDisplay *brl, const unsigned char *packet, int length);
  int (*probeDisplay) (BrailleDisplay *brl);
  void (*updateKeys) (BrailleDisplay *brl);
  int (*writeCells) (BrailleDisplay *brl);
} ProtocolOperations;

typedef struct {
  const ProtocolOperations *const *protocols;
  int (*openPort) (const char *device);
  int (*configurePort) (void);
  void (*closePort) ();
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, int length, int wait);
  int (*writeBytes) (const unsigned char *buffer, int length);
} InputOutputOperations;

static const InputOutputOperations *io;
static const ProtocolOperations *protocol;
static int charactersPerSecond;

/* Internal Routines */

static void
logTextField (const char *name, const char *address, int length) {
  while (length > 0) {
    const char byte = address[length - 1];
    if (byte && (byte != ' ')) break;
    --length;
  }
  LogPrint(LOG_INFO, "%s: %.*s", name, length, address);
}

static int
readByte (unsigned char *byte, int wait) {
  int count = io->readBytes(byte, 1, wait);
  if (count > 0) return 1;

  if (count == 0) errno = EAGAIN;
  return 0;
}

static int
flushInput (void) {
  unsigned char byte;
  while (readByte(&byte, 0));
  return errno == EAGAIN;
}

static void
adjustWriteDelay (BrailleDisplay *brl, int bytes) {
  brl->writeDelay += (bytes * 1000 / charactersPerSecond) + 1;
}

static int
setGroupedKey (unsigned char *group, unsigned char key, int press) {
  unsigned char *byte = &group[key / 8];
  unsigned char bit = 1 << (key % 8);

  if (!(*byte & bit) == !press) return 0;

  if (press) {
    *byte |= bit;
  } else {
    *byte &= ~bit;
  }
  return 1;
}

static void
clearKeyGroup (unsigned char *group, unsigned char count) {
  memset(group, 0, KEY_GROUP_SIZE(count));
}

static void
resetKeyGroup (unsigned char *group, unsigned char count, unsigned char key) {
  clearKeyGroup(group, count);
  if (key > 0) setGroupedKey(group, key-1, 1);
}

static void
updateKeyGroup (
  unsigned char *group, const unsigned char *new,
  unsigned char set, unsigned char base, unsigned char count
) {
  unsigned char pressTable[count];
  unsigned char pressCount = 0;
  unsigned char offset;

  for (offset=0; offset<count; offset+=1) {
    unsigned char key = base + offset;
    int press = (new[offset / 8] & (1 << (offset % 8))) != 0;

    if (setGroupedKey(group, key, press)) {
      if (press) {
        pressTable[pressCount++] = key;
      } else {
        enqueueKeyEvent(set, key, 0);
      }
    }
  }

  while (pressCount) enqueueKeyEvent(set, pressTable[--pressCount], 1);
}

static void
updateNavigationKeys (const unsigned char *new, unsigned char base, unsigned char count) {
  updateKeyGroup(keysState.navigationKeys, new, BM_SET_NavigationKeys, base, count);
}

static void
updateDisplayKeys (unsigned char new) {
  updateNavigationKeys(&new, BM_KEY_DISPLAY, BM_KEYS_DISPLAY);
}

static void
updateCommandKeys (unsigned char new) {
  updateNavigationKeys(&new, BM_KEY_COMMAND, BM_KEYS_COMMAND);
}

static void
updateRoutingKeys (const unsigned char *new, unsigned char count) {
  updateKeyGroup(keysState.routingKeys, new, BM_SET_RoutingKeys, 0, count);
}

static int
updateCells (BrailleDisplay *brl) {
  if (cellsUpdated) {
    if (!protocol->writeCells(brl)) return 0;
    cellsUpdated = 0;
  }
  return 1;
}

static void
translateCells (int start, int count) {
  while (count-- > 0) {
    externalCells[start] = outputTable[internalCells[start]];
    ++start;
    cellsUpdated = 1;
  }
}

static void
clearCells (int start, int count) {
  memset(&internalCells[start], 0, count);
  translateCells(start, count);
}

static void
logCellCount (BrailleDisplay *brl) {
  switch ((brl->textColumns = cellCount)) {
    case 44:
    case 68:
    case 84:
      brl->textColumns -= 4;
      break;

    case 56:
      brl->textColumns -= 16;
      break;
  }
  brl->textRows = 1;
  brl->statusRows = (brl->statusColumns = cellCount - brl->textColumns)? 1: 0;

  LogPrint(LOG_INFO, "Cell Count: %d (%d text, %d status)",
           cellCount, brl->textColumns, brl->statusColumns);
}

static void
changeCellCount (BrailleDisplay *brl, int count) {
  if (count != cellCount) {
    if (count > cellCount) {
      clearCells(cellCount, count-cellCount);

      {
        int number;
        for (number=cellCount; number<count; number+=1) {
          setGroupedKey(keysState.routingKeys, number, 0);
          setGroupedKey(keysState.horizontalSensors, number, 0);
        }
      }
    }

    cellCount = count;
    logCellCount(brl);
    brl->resizeRequired = 1;
  }
}

/* Baum Protocol */

#define ESCAPE 0X1B

typedef unsigned char BaumInteger[2];
#define MAKE_BAUM_INTEGER_FIRST(i) ((i) & 0XFF)
#define MAKE_BAUM_INTEGER_SECOND(i) (((i) >> 8) & 0XFF)
#define MAKE_BAUM_INTEGER(i) MAKE_BAUM_INTEGER_FIRST((i)), MAKE_BAUM_INTEGER_SECOND((i))

static inline uint16_t
getBaumInteger (const BaumInteger integer) {
  return (integer[1] << 8) | integer[0];
}

typedef enum {
  BAUM_REQ_DisplayData             = 0X01,
  BAUM_REQ_GetVersionNumber        = 0X05,
  BAUM_REQ_GetKeys                 = 0X08,
  BAUM_REQ_GetMode                 = 0X11,
  BAUM_REQ_SetMode                 = 0X12,
  BAUM_REQ_SetProtocolState        = 0X15,
  BAUM_REQ_SetCommunicationChannel = 0X16,
  BAUM_REQ_CausePowerdown          = 0X17,
  BAUM_REQ_ModuleRegistration      = 0X50,
  BAUM_REQ_DataRegisters           = 0X51,
  BAUM_REQ_ServiceRegisters        = 0X52,
  BAUM_REQ_GetDeviceIdentity       = 0X84,
  BAUM_REQ_GetSerialNumber         = 0X8A,
  BAUM_REQ_GetBluetoothName        = 0X8C,
  BAUM_REQ_SetBluetoothName        = 0X8D,
  BAUM_REQ_SetBluetoothPin         = 0X8E
} BaumRequestCode;

typedef enum {
  BAUM_RSP_CellCount            = 0X01,
  BAUM_RSP_VersionNumber        = 0X05,
  BAUM_RSP_ModeSetting          = 0X11,
  BAUM_RSP_CommunicationChannel = 0X16,
  BAUM_RSP_PowerdownSignal      = 0X17,
  BAUM_RSP_HorizontalSensors    = 0X20,
  BAUM_RSP_VerticalSensors      = 0X21,
  BAUM_RSP_RoutingKeys          = 0X22,
  BAUM_RSP_Switches             = 0X23,
  BAUM_RSP_DisplayKeys          = 0X24,
  BAUM_RSP_HorizontalSensor     = 0X25,
  BAUM_RSP_VerticalSensor       = 0X26,
  BAUM_RSP_RoutingKey           = 0X27,
  BAUM_RSP_FrontRockers         = 0X28,
  BAUM_RSP_BackRockers          = 0X29,
  BAUM_RSP_CommandKeys          = 0X2B,
  BAUM_RSP_FrontButtons         = 0X2C,
  BAUM_RSP_BackButtons          = 0X2D,
  BAUM_RSP_EntryKeys            = 0X33,
  BAUM_RSP_Joystick             = 0X34,
  BAUM_RSP_ErrorCode            = 0X40,
  BAUM_RSP_ModuleRegistration   = 0X50,
  BAUM_RSP_DataRegisters        = 0X51,
  BAUM_RSP_ServiceRegisters     = 0X52,
  BAUM_RSP_DeviceIdentity       = 0X84,
  BAUM_RSP_SerialNumber         = 0X8A,
  BAUM_RSP_BluetoothName        = 0X8C
} BaumResponseCode;

typedef enum {
  BAUM_MODE_KeyGroupCompressed          = 0X01,
  BAUM_MODE_HorizontalSensorsEnabled    = 0X06,
  BAUM_MODE_LeftSensorsEnabled  = 0X07,
  BAUM_MODE_RoutingKeysEnabled          = 0X08,
  BAUM_MODE_RightSensorsEnabled = 0X09,
  BAUM_MODE_BackKeysEnabled             = 0X0A,
  BAUM_MODE_DisplayRotated              = 0X10,
  BAUM_MODE_DisplayEnabled              = 0X20,
  BAUM_MODE_PowerdownEnabled            = 0X21,
  BAUM_MODE_PowerdownTime               = 0X22,
  BAUM_MODE_BluetoothEnabled            = 0X23,
  BAUM_MODE_UsbCharge                   = 0X24
} BaumMode;

typedef enum {
  BAUM_PDT_5Minutes  = 1,
  BAUM_PDT_10Minutes = 2,
  BAUM_PDT_1Hour     = 3,
  BAUM_PDT_2Hours    = 4
} BaumPowerdownTime;

typedef enum {
  BAUM_PDR_ProtocolRequested = 0X01,
  BAUM_PDR_PowerSwitch       = 0X02,
  BAUM_PDR_AutoPowerOff      = 0X04,
  BAUM_PDR_BatteryLow        = 0X08,
  BAUM_PDR_Charging          = 0X80
} BaumPowerdownReason;

typedef enum {
  BAUM_SWT_DisableSensors  = 0X01,
  BAUM_SWT_ScaledVertical  = 0X02,
  BAUM_SWT_ShowSensor      = 0X40,
  BAUM_SWT_BrailleKeyboard = 0X80
} BaumSwitch;

typedef enum {
  BAUM_ERR_BluetoothSupport       = 0X0A,
  BAUM_ERR_TransmitOverrun        = 0X10,
  BAUM_ERR_ReceiveOverrun         = 0X11,
  BAUM_ERR_TransmitTimeout        = 0X12,
  BAUM_ERR_ReceiveTimeout         = 0X13,
  BAUM_ERR_PacketType             = 0X14,
  BAUM_ERR_PacketChecksum         = 0X15,
  BAUM_ERR_PacketData             = 0X16,
  BAUM_ERR_Test                   = 0X18,
  BAUM_ERR_FlashWrite             = 0X19,
  BAUM_ERR_CommunicationChannel   = 0X1F,
  BAUM_ERR_SerialNumber           = 0X20,
  BAUM_ERR_SerialParity           = 0X21,
  BAUM_ERR_SerialOverrun          = 0X22,
  BAUM_ERR_SerialFrame            = 0X24,
  BAUM_ERR_LocalizationIdentifier = 0X25,
  BAUM_ERR_LocalizationIndex      = 0X26,
  BAUM_ERR_LanguageIdentifier     = 0X27,
  BAUM_ERR_LanguageIndex          = 0X28,
  BAUM_ERR_BrailleTableIdentifier = 0X29,
  BAUM_ERR_BrailleTableIndex      = 0X2A
} BaumError;

#define BAUM_LENGTH_DeviceIdentity 16
#define BAUM_LENGTH_SerialNumber 8
#define BAUM_LENGTH_BluetoothName 14

typedef enum {
  BAUM_MRC_Acknowledge = 0X01,
  BAUM_MRC_Query       = 0X04
} BaumModuleRegistrationCommand;

typedef enum {
  BAUM_MRE_Addition  = 1,
  BAUM_MRE_Removal   = 2,
  BAUM_MRE_Rejection = 3
} BaumModuleRegistrationEvent;

typedef enum {
  BAUM_DRC_Write = 0X00,
  BAUM_DRC_Read  = 0X01,
  BAUM_DRC_Reset = 0X80
} BaumDataRegistersCommand;

typedef enum {
  BAUM_DRF_WheelsChanged  = 0X01,
  BAUM_DRF_ButtonsChanged = 0X02,
  BAUM_DRF_KeysChanged    = 0X04,
  BAUM_DRF_PotsChanged    = 0X04,
  BAUM_DRF_SensorsChanged = 0X08,
  BAUM_DRF_ErrorOccurred  = 0X80
} BaumDataRegistersFlag;

typedef enum {
  BAUM_DRE_WheelsNotConnected = 0X01,
  BAUM_DRE_WheelsNotAdjusted  = 0X02,
  BAUM_DRE_KeyBufferFull      = 0X04,
  BAUM_DRE_SerialError        = 0X80
} BaumDataRegistersError;

typedef enum {
  BAUM_SRC_Write = 0X00,
  BAUM_SRC_Read  = 0X01
} BaumServiceRegistersCommand;

typedef union {
  unsigned char bytes[2 + 0XFF];

  struct {
    unsigned char code;

    union {
      unsigned char cellCount;
      unsigned char versionNumber;

      struct {
        unsigned char identifier;
        unsigned char setting;
      } PACKED mode;

      unsigned char communicationChannel;
      unsigned char powerdownReason;
      unsigned char horizontalSensors[KEY_GROUP_SIZE(MAXIMUM_CELL_COUNT)];

      struct {
        unsigned char left[KEY_GROUP_SIZE(VERTICAL_SENSOR_COUNT)];
        unsigned char right[KEY_GROUP_SIZE(VERTICAL_SENSOR_COUNT)];
      } PACKED verticalSensors;

      unsigned char routingKeys[KEY_GROUP_SIZE(MAXIMUM_CELL_COUNT)];
      unsigned char switches;
      unsigned char displayKeys;
      unsigned char horizontalSensor;

      union {
        unsigned char left;
        unsigned char right;
      } PACKED verticalSensor;

      unsigned char routingKey;
      unsigned char frontRockers[1];
      unsigned char backRockers[1];
      unsigned char commandKeys;
      unsigned char frontButtons[2];
      unsigned char backButtons[2];
      unsigned char entryKeys[2];
      unsigned char joystick[1];
      unsigned char errorCode;

      struct {
        unsigned char length;
        BaumInteger moduleIdentifier;
        BaumInteger serialNumber;

        union {
          struct {
            BaumInteger hardwareVersion;
            BaumInteger firmwareVersion;
            unsigned char event;
          } PACKED registration;

          union {
            struct {
              unsigned char flags;
              unsigned char errors;
              signed char wheels[4];
              unsigned char buttons;
              unsigned char keys;
              unsigned char sensors[KEY_GROUP_SIZE(80)];
            } PACKED display80;

            struct {
              unsigned char flags;
              unsigned char errors;
              signed char wheels[3];
              unsigned char buttons;
              unsigned char keys;
              unsigned char sensors[KEY_GROUP_SIZE(64)];
            } PACKED display64;

            struct {
              unsigned char flags;
              unsigned char errors;
              unsigned char buttons;
            } PACKED status;

            struct {
              unsigned char flags;
              unsigned char errors;
              signed char wheel;
              unsigned char buttons;
              unsigned char keypad[2];
            } PACKED phone;

            struct {
              unsigned char flags;
              unsigned char errors;
              signed char wheel;
              unsigned char keys;
              unsigned char pots[6];
            } PACKED audio;

            struct {
              unsigned char flags;
              unsigned char errors;
              unsigned char buttons;
              unsigned char cursor;
              unsigned char keys;
              unsigned char pots[4];
            } PACKED voice;
          } registers;
        } data;
      } PACKED modular;

      char deviceIdentity[BAUM_LENGTH_DeviceIdentity];
      char serialNumber[BAUM_LENGTH_SerialNumber];
      char bluetoothName[BAUM_LENGTH_BluetoothName];
    } PACKED values;
  } PACKED data;
} PACKED BaumResponsePacket;

typedef enum {
  BAUM_DEVICE_Inka,
  BAUM_DEVICE_DM80P,
  BAUM_DEVICE_Modular,
  BAUM_DEVICE_Generic
} BaumDeviceType;
static BaumDeviceType baumDeviceType;

typedef enum {
  BAUM_MODULE_Display80,
  BAUM_MODULE_Display64,
  BAUM_MODULE_Status,
  BAUM_MODULE_Phone,
  BAUM_MODULE_Audio,
  BAUM_MODULE_Voice
} BaumModuleType;

typedef struct {
  uint16_t identifier;
  unsigned char type;
  unsigned char cellCount;
  unsigned char keyCount;
  unsigned char buttonCount;
  unsigned char wheelCount;
  unsigned char potCount;
  unsigned isDisplay:1;
  unsigned hasCursorKeys:1;
  unsigned hasKeypad:1;
} BaumModuleDescription;

static const BaumModuleDescription baumModuleDescriptions[] = {
  { .identifier = 0X4180,
    .type = BAUM_MODULE_Display80,
    .cellCount = 80,
    .wheelCount = 4,
    .isDisplay = 1
  }
  ,
  { .identifier = 0X4181,
    .type = BAUM_MODULE_Display64,
    .cellCount = 64,
    .wheelCount = 3,
    .isDisplay = 1
  }
  ,
  { .identifier = 0X4190,
    .type = BAUM_MODULE_Status,
    .cellCount = 4,
    .buttonCount = 4
  }
  ,
  { .identifier = 0X4191,
    .type = BAUM_MODULE_Phone,
    .cellCount = 12,
    .buttonCount = 4,
    .wheelCount = 1,
    .hasKeypad = 1
  }
  ,
  { .identifier = 0X4192,
    .type = BAUM_MODULE_Audio,
    .keyCount = 5,
    .wheelCount = 1,
    .potCount = 6
  }
  ,
  { .identifier = 0X4193,
    .type = BAUM_MODULE_Voice,
    .keyCount = 4,
    .buttonCount = 3,
    .potCount = 4,
    .hasCursorKeys = 1
  }
  ,
  { .identifier = 0 }
};

static const BaumModuleDescription *
getBaumModuleDescription (uint16_t identifier) {
  const BaumModuleDescription *bmd = baumModuleDescriptions;

  while (bmd->identifier) {
    if (bmd->identifier == identifier) return bmd;
    bmd += 1;
  }

  LogPrint(LOG_DEBUG, "unknown module identifier: %04X", identifier);
  return NULL;
}

typedef struct {
  const BaumModuleDescription *description;
  uint16_t serialNumber;
  uint16_t hardwareVersion;
  uint16_t firmwareVersion;
} BaumModuleRegistration;

static void
clearBaumModuleRegistration (BaumModuleRegistration *bmr) {
  bmr->description = NULL;
  bmr->serialNumber = 0;
  bmr->hardwareVersion = 0;
  bmr->firmwareVersion = 0;
}

static BaumModuleRegistration baumDisplayModule;
static BaumModuleRegistration baumStatusModule;

static BaumModuleRegistration *const baumModules[] = {
  &baumDisplayModule,
  &baumStatusModule,
  NULL
};

static BaumModuleRegistration *
getBaumModuleRegistration (const BaumModuleDescription *bmd, uint16_t serialNumber) {
  if (bmd) {
    BaumModuleRegistration *const *bmr = baumModules;

    while (*bmr) {
      if (((*bmr)->description == bmd) && ((*bmr)->serialNumber == serialNumber)) return *bmr;
      bmr += 1;
    }
  }

  return NULL;
}

static int
getBaumModuleCellCount (void) {
  int count = 0;

  {
    BaumModuleRegistration *const *bmr = baumModules;

    while (*bmr) {
      const BaumModuleDescription *bmd = (*bmr++)->description;
      if (bmd) count += bmd->cellCount;
    }
  }

  return count;
}

static void
assumeBaumDeviceIdentity (const char *identity) {
  LogPrint(LOG_INFO, "Baum Device Identity: %s", identity);
}

static void
logBaumDeviceIdentity (const BaumResponsePacket *packet) {
  logTextField("Baum Device Identity",
               packet->data.values.deviceIdentity,
               sizeof(packet->data.values.deviceIdentity));
}

static void
logBaumSerialNumber (const BaumResponsePacket *packet) {
  logTextField("Baum Serial Number",
               packet->data.values.serialNumber,
               sizeof(packet->data.values.serialNumber));
}

static int
logBaumPowerdownReason (BaumPowerdownReason reason) {
  typedef struct {
    BaumPowerdownReason bit;
    const char *explanation;
  } ReasonEntry;
  static const ReasonEntry reasonTable[] = {
    {BAUM_PDR_ProtocolRequested, "driver request"},
    {BAUM_PDR_PowerSwitch      , "power switch"},
    {BAUM_PDR_AutoPowerOff     , "idle timeout"},
    {BAUM_PDR_BatteryLow       , "battery low"},
    {0}
  };
  const ReasonEntry *entry;

  char buffer[0X100];
  int length = 0;
  char delimiter = ':';

  sprintf(&buffer[length], "Baum Powerdown%n", &length);
  for (entry=reasonTable; entry->bit; ++entry) {
    if (reason & entry->bit) {
      int count;
      sprintf(&buffer[length], "%c %s%n", delimiter, entry->explanation, &count);
      length += count;
      delimiter = ',';
    }
  }

  LogPrint(LOG_WARNING, "%.*s", length, buffer);
  return 1;
}

static int
readBaumPacket (BrailleDisplay *brl, unsigned char *packet, int size) {
  int started = 0;
  int escape = 0;
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!readByte(&byte, (started || escape))) {
      if (offset > 0) logPartialPacket(packet, offset);
      return 0;
    }

    if (byte == ESCAPE) {
      if ((escape = !escape)) continue;
    } else if (escape) {
      escape = 0;

      if (offset > 0) {
        logShortPacket(packet, offset);
        offset = 0;
        length = 0;
      } else {
        started = 1;
      }
    }

    if (!started) {
      logIgnoredByte(byte);
      continue;
    }

    if (offset < size) {
      if (offset == 0) {
        switch (byte) {
          case BAUM_RSP_Switches:
            if (!cellCount) {
              assumeBaumDeviceIdentity("DM80P");
              baumDeviceType = BAUM_DEVICE_DM80P;
              cellCount = 84;
            }

          case BAUM_RSP_CellCount:
          case BAUM_RSP_VersionNumber:
          case BAUM_RSP_CommunicationChannel:
          case BAUM_RSP_PowerdownSignal:
          case BAUM_RSP_DisplayKeys:
          case BAUM_RSP_HorizontalSensor:
          case BAUM_RSP_RoutingKey:
          case BAUM_RSP_FrontRockers:
          case BAUM_RSP_BackRockers:
          case BAUM_RSP_CommandKeys:
          case BAUM_RSP_Joystick:
          case BAUM_RSP_ErrorCode:
          case BAUM_RSP_ModuleRegistration:
          case BAUM_RSP_DataRegisters:
          case BAUM_RSP_ServiceRegisters:
            length = 2;
            break;

          case BAUM_RSP_ModeSetting:
          case BAUM_RSP_FrontButtons:
          case BAUM_RSP_BackButtons:
          case BAUM_RSP_EntryKeys:
            length = 3;
            break;

          case BAUM_RSP_VerticalSensor:
            length = (baumDeviceType == BAUM_DEVICE_Inka)? 2: 3;
            break;

          case BAUM_RSP_VerticalSensors:
          case BAUM_RSP_SerialNumber:
            length = 9;
            break;

          case BAUM_RSP_BluetoothName:
            length = 15;
            break;

          case BAUM_RSP_DeviceIdentity:
            length = 17;
            break;

          case BAUM_RSP_RoutingKeys:
            if (!cellCount) {
              assumeBaumDeviceIdentity("Inka");
              baumDeviceType = BAUM_DEVICE_Inka;
              cellCount = 56;
            }

            if (baumDeviceType == BAUM_DEVICE_Inka) {
              length = 2;
              break;
            }

            length = KEY_GROUP_SIZE(cellCount) + 1;
            break;

          case BAUM_RSP_HorizontalSensors:
            length = KEY_GROUP_SIZE(brl->textColumns) + 1;
            break;

          default:
            logUnknownPacket(byte);
            started = 0;
            continue;
        }
      } else if (offset == 1) {
        switch (packet[0]) {
          case BAUM_RSP_ModuleRegistration:
          case BAUM_RSP_DataRegisters:
          case BAUM_RSP_ServiceRegisters:
            length += byte;
            break;

          default:
            break;
        }
      }

      packet[offset] = byte;
    } else {
      if (offset == size) logTruncatedPacket(packet, offset);
      logDiscardedByte(byte);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        started = 0;
        continue;
      }

      logInputPacket(packet, offset);
      return length;
    }
  }
}

static int
getBaumPacket (BrailleDisplay *brl, BaumResponsePacket *packet) {
  return readBaumPacket(brl, packet->bytes, sizeof(*packet));
}

static int
writeBaumPacket (BrailleDisplay *brl, const unsigned char *packet, int length) {
  unsigned char buffer[1 + (length * 2)];
  unsigned char *byte = buffer;
  *byte++ = ESCAPE;

  {
    int index = 0;
    while (index < length)
      if ((*byte++ = packet[index++]) == ESCAPE)
        *byte++ = ESCAPE;
  }

  {
    int count = byte - buffer;
    logOutputPacket(buffer, count);

    {
      int ok = io->writeBytes(buffer, count) != -1;
      if (ok) adjustWriteDelay(brl, count);
      return ok;
    }
  }
}

static int
setBaumMode (BrailleDisplay *brl, unsigned char mode, unsigned char setting) {
  const unsigned char request[] = {BAUM_REQ_SetMode, mode, setting};
  return writeBaumPacket(brl, request, sizeof(request));
}

static void
setBaumSwitches (BrailleDisplay *brl, unsigned char newSettings, int initialize) {
  unsigned char changedSettings = newSettings ^ switchSettings;
  switchSettings = newSettings;

  {
    typedef struct {
      unsigned char switchBit;
      unsigned char modeNumber;
      unsigned char offValue;
      unsigned char onValue;
    } SwitchEntry;

    static const SwitchEntry switchTable[] = {
      {BAUM_SWT_ShowSensor, 0X01, 0, 2},
      {BAUM_SWT_BrailleKeyboard, 0X03, 0, 3},
      {0}
    };
    const SwitchEntry *entry = switchTable;

    while (entry->switchBit) {
      if (initialize || (changedSettings & entry->switchBit))
        setBaumMode(brl, entry->modeNumber,
                    ((switchSettings & entry->switchBit)? entry->onValue:
                                                          entry->offValue));
      ++entry;
    }
  }
}

static void
setInkaSwitches (BrailleDisplay *brl, unsigned char newSettings, int initialize) {
  newSettings ^= 0X0F;
  setBaumSwitches(brl, ((newSettings & 0X03) | ((newSettings & 0X0C) << 4)), initialize);
}

static int
writeBaumModuleRegistrationCommand (
  BrailleDisplay *brl,
  uint16_t moduleIdentifier, uint16_t serialNumber,
  BaumModuleRegistrationCommand command
) {
  const unsigned char request[] = {
    BAUM_REQ_ModuleRegistration,
    5, /* data length */
    MAKE_BAUM_INTEGER(moduleIdentifier),
    MAKE_BAUM_INTEGER(serialNumber),
    command
  };

  return writeBaumPacket(brl, request, sizeof(request));
}

static int
writeBaumDataRegisters (
  BrailleDisplay *brl,
  const BaumModuleRegistration *bmr,
  const unsigned char *registers, unsigned char count
) {
  const BaumModuleDescription *bmd = bmr->description;

  if (bmd) {
    if (count < bmd->cellCount) count = bmd->cellCount;

    if (count) {
      unsigned char packet[2 + 7 + count];
      unsigned char *byte = packet;

      *byte++ = BAUM_REQ_DataRegisters;
      *byte++ = 7 + count;

      *byte++ = MAKE_BAUM_INTEGER_FIRST(bmd->identifier);
      *byte++ = MAKE_BAUM_INTEGER_SECOND(bmd->identifier);

      *byte++ = MAKE_BAUM_INTEGER_FIRST(bmr->serialNumber);
      *byte++ = MAKE_BAUM_INTEGER_SECOND(bmr->serialNumber);

      *byte++ = BAUM_DRC_Write;
      *byte++ = 0; /* index of the first register to write */
      *byte++ = count;

      memcpy(byte, registers, count);
      byte += count;

      if (!writeBaumPacket(brl, packet, byte-packet)) return 0;
    }
  }

  return 1;
}

static int
handleBaumModuleRegistrationEvent (BrailleDisplay *brl, const BaumResponsePacket *packet) {
  uint16_t moduleIdentifier = getBaumInteger(packet->data.values.modular.moduleIdentifier);
  uint16_t serialNumber = getBaumInteger(packet->data.values.modular.serialNumber);
  const BaumModuleDescription *bmd = getBaumModuleDescription(moduleIdentifier);

  if (packet->data.values.modular.data.registration.event == BAUM_MRE_Addition) {
    if (!writeBaumModuleRegistrationCommand(brl,
                                            moduleIdentifier, serialNumber,
                                            BAUM_MRC_Acknowledge))
      return 0;

    if (bmd) {
      BaumModuleRegistration *bmr;

      if (bmd->isDisplay) {
        bmr = &baumDisplayModule;
      } else if (bmd->type == BAUM_MODULE_Status) {
        bmr = &baumStatusModule;
      } else {
        bmr = NULL;
      }

      if (bmr) {
        if (bmr->description) clearBaumModuleRegistration(bmr);

        bmr->description = bmd;
        bmr->serialNumber = serialNumber;
        bmr->hardwareVersion = getBaumInteger(packet->data.values.modular.data.registration.hardwareVersion);
        bmr->firmwareVersion = getBaumInteger(packet->data.values.modular.data.registration.firmwareVersion);
      }
    }
  } else {
    BaumModuleRegistration *bmr = getBaumModuleRegistration(bmd, serialNumber);
    if (bmr) clearBaumModuleRegistration(bmr);
  }

  return 1;
}

static void
handleBaumDataRegistersEvent (BrailleDisplay *brl, const BaumResponsePacket *packet) {
  const BaumModuleDescription *bmd = getBaumModuleDescription(getBaumInteger(packet->data.values.modular.moduleIdentifier));
  const BaumModuleRegistration *bmr = getBaumModuleRegistration(bmd, getBaumInteger(packet->data.values.modular.serialNumber));

  if (bmr) {
    switch (bmd->type) {
      {
        unsigned char flags;
        unsigned char errors;
        const signed char *wheel;
        unsigned char wheels;
        unsigned char buttons;
        unsigned char keys;
        const unsigned char *sensors;

      case BAUM_MODULE_Display80:
        flags = packet->data.values.modular.data.registers.display80.flags;
        errors = packet->data.values.modular.data.registers.display80.errors;
        wheel = packet->data.values.modular.data.registers.display80.wheels;
        wheels = ARRAY_COUNT(packet->data.values.modular.data.registers.display80.wheels);
        buttons = packet->data.values.modular.data.registers.display80.buttons;
        keys = packet->data.values.modular.data.registers.display80.keys;
        sensors = packet->data.values.modular.data.registers.display80.sensors;
        goto doDisplay;

      case BAUM_MODULE_Display64:
        flags = packet->data.values.modular.data.registers.display64.flags;
        errors = packet->data.values.modular.data.registers.display64.errors;
        wheel = packet->data.values.modular.data.registers.display64.wheels;
        wheels = ARRAY_COUNT(packet->data.values.modular.data.registers.display64.wheels);
        buttons = packet->data.values.modular.data.registers.display64.buttons;
        keys = packet->data.values.modular.data.registers.display64.keys;
        sensors = packet->data.values.modular.data.registers.display64.sensors;
        goto doDisplay;

      doDisplay:
        if (flags & BAUM_DRF_WheelsChanged) {
        }

        if (flags & BAUM_DRF_ButtonsChanged) {
        }

        if (flags & BAUM_DRF_KeysChanged) {
          updateDisplayKeys(keys);
        }

        if (flags & BAUM_DRF_SensorsChanged) {
          updateRoutingKeys(sensors, brl->textColumns);
        }

        break;
      }

      case BAUM_MODULE_Status:
        if (packet->data.values.modular.data.registers.status.flags & BAUM_DRF_ButtonsChanged) {
          updateCommandKeys(packet->data.values.modular.data.registers.status.buttons);
        }

        break;

      default:
        LogPrint(LOG_WARNING, "unsupported data register configuration: %u", bmd->type);
        break;
    }
  }
}

static int
probeBaumDisplay (BrailleDisplay *brl) {
  int probes = 0;

  do {
    int identityCellCount = 0;

    baumDeviceType = BAUM_DEVICE_Generic;
    cellCount = 0;

    {
      BaumModuleRegistration *const *bmr = baumModules;
      while (*bmr) clearBaumModuleRegistration(*bmr++);
    }

    /* newer models return an identity string which contains the cell count */
    {
      static const unsigned char request[] = {BAUM_REQ_GetDeviceIdentity};
      if (!writeBaumPacket(brl, request, sizeof(request))) break;
    }

    /* get the serial number for the log */
    {
      static const unsigned char request[] = {BAUM_REQ_GetSerialNumber};
      if (!writeBaumPacket(brl, request, sizeof(request))) break;
    }

    /* try explicitly asking for the cell count */
    {
      static const unsigned char request[] = {BAUM_REQ_DisplayData, 0};
      if (!writeBaumPacket(brl, request, sizeof(request))) break;
    }

    /* enqueue a request to get the initial key states */
    {
      static const unsigned char request[] = {BAUM_REQ_GetKeys};
      if (!writeBaumPacket(brl, request, sizeof(request))) break;
    }

    /* the modular models need to be probed with a general call */
    if (!writeBaumModuleRegistrationCommand(brl, 0, 0, BAUM_MRC_Query)) break;

    while (io->awaitInput(probeTimeout)) {
      BaumResponsePacket response;
      int size = getBaumPacket(brl, &response);

      if (size) {
        switch (response.data.code) {
          case BAUM_RSP_RoutingKeys: /* Inka */
            setInkaSwitches(brl, response.data.values.switches, 1);
            return 1;

          case BAUM_RSP_Switches: /* DM80P */
            setBaumSwitches(brl, response.data.values.switches, 1);
            return 1;

          case BAUM_RSP_CellCount: { /* newer models */
            unsigned char count = response.data.values.cellCount;

            if ((count > 0) && (count <= MAXIMUM_CELL_COUNT)) {
              cellCount = count;
              return 1;
            }

            LogPrint(LOG_DEBUG, "unexpected cell count: %u", count);
            continue;
          }

          case BAUM_RSP_ModuleRegistration: /* modular models */
            if (!handleBaumModuleRegistrationEvent(brl, &response)) return 0;
            if (!baumDisplayModule.description) continue;
            baumDeviceType = BAUM_DEVICE_Modular;
            cellCount = getBaumModuleCellCount();
            return 1;

          case BAUM_RSP_DeviceIdentity: /* should contain fallback cell count */
            logBaumDeviceIdentity(&response);

            {
              const int length = sizeof(response.data.values.deviceIdentity);
              char buffer[length+1];

              memcpy(buffer, response.data.values.deviceIdentity, length);
              buffer[length] = 0;

              {
                char *digits = strpbrk(buffer, "123456789");

                if (digits) {
                  int count = atoi(digits);
                  if ((count > 0) && (count <= MAXIMUM_CELL_COUNT)) identityCellCount = count;
                }
              }
            }
            continue;

          case BAUM_RSP_SerialNumber:
            logBaumSerialNumber(&response);
            continue;

          case BAUM_RSP_ErrorCode:
            if (response.data.values.errorCode != BAUM_ERR_PacketType) goto unexpectedPacket;
            LogPrint(LOG_DEBUG, "unsupported request");
            continue;

          default:
          unexpectedPacket:
            logUnexpectedPacket(response.bytes, size);
            continue;
        }
      } else if (errno != EAGAIN) {
        break;
      }
    }
    if (errno != EAGAIN) break;

    if (identityCellCount) {
      cellCount = identityCellCount;
      return 1;
    }
  } while (++probes < probeLimit);

  return 0;
}

static void
updateBaumKeys (BrailleDisplay *brl) {
  BaumResponsePacket packet;
  int size;

  while ((size = getBaumPacket(brl, &packet))) {
    switch (packet.data.code) {
      case BAUM_RSP_CellCount:
        changeCellCount(brl, packet.data.values.cellCount);
        continue;

      case BAUM_RSP_DeviceIdentity:
        logBaumDeviceIdentity(&packet);
        continue;

      case BAUM_RSP_SerialNumber:
        logBaumSerialNumber(&packet);
        continue;

      case BAUM_RSP_CommunicationChannel:
        continue;

      case BAUM_RSP_PowerdownSignal:
        if (!logBaumPowerdownReason(packet.data.values.powerdownReason)) continue;
        errno = ENODEV;
        return;

      case BAUM_RSP_DisplayKeys: {
        unsigned char keys;

        switch (baumDeviceType) {
          case BAUM_DEVICE_Inka:
            keys = 0;
#define KEY(inka,baum) if (!(packet.data.values.displayKeys & (inka))) keys |= (baum)
            KEY(004, 001);
            KEY(002, 002);
            KEY(001, 004);
            KEY(040, 010);
            KEY(020, 020);
            KEY(010, 040);
#undef KEY
            break;

          case BAUM_DEVICE_DM80P:
            keys = packet.data.values.displayKeys ^ 0X7F;
            break;

          default:
            keys = packet.data.values.displayKeys;
            break;
        }

        updateDisplayKeys(keys);
        continue;
      }

      case BAUM_RSP_CommandKeys:
        updateCommandKeys(packet.data.values.commandKeys);
        continue;

      case BAUM_RSP_FrontRockers:
        updateNavigationKeys(packet.data.values.frontRockers,
                             BM_KEY_FRONT_ROCKERS, BM_KEYS_ROCKER);
        continue;

      case BAUM_RSP_BackRockers:
        updateNavigationKeys(packet.data.values.backRockers,
                             BM_KEY_BACK_ROCKERS, BM_KEYS_ROCKER);
        continue;

      case BAUM_RSP_FrontButtons:
        updateNavigationKeys(packet.data.values.frontButtons,
                             BM_KEY_FRONT_BUTTONS, BM_KEYS_BUTTON);
        continue;

      case BAUM_RSP_BackButtons:
        updateNavigationKeys(packet.data.values.backButtons,
                             BM_KEY_BACK_BUTTONS, BM_KEYS_BUTTON);
        continue;

      case BAUM_RSP_EntryKeys:
        updateNavigationKeys(packet.data.values.entryKeys,
                             BM_KEY_ENTRY, BM_KEYS_ENTRY);
        continue;

      case BAUM_RSP_Joystick:
        updateNavigationKeys(packet.data.values.joystick,
                             BM_KEY_JOYSTICK, BM_KEYS_JOYSTICK);
        continue;

      case BAUM_RSP_HorizontalSensor:
        resetKeyGroup(packet.data.values.horizontalSensors, brl->textColumns, packet.data.values.horizontalSensor);
      case BAUM_RSP_HorizontalSensors:
        if (!(switchSettings & BAUM_SWT_DisableSensors)) {
          updateKeyGroup(keysState.horizontalSensors, packet.data.values.horizontalSensors,
                         BM_SET_HorizontalSensors, 0, brl->textColumns);
        }
        continue;

      case BAUM_RSP_VerticalSensor: {
        unsigned char left = packet.data.values.verticalSensor.left;
        unsigned char right;

        if (baumDeviceType != BAUM_DEVICE_Inka) {
          right = packet.data.values.verticalSensor.right;
        } else if (left & 0X40) {
          left -= 0X40;
          right = 0;
        } else {
          right = left;
          left = 0;
        }

        resetKeyGroup(packet.data.values.verticalSensors.left, VERTICAL_SENSOR_COUNT, left);
        resetKeyGroup(packet.data.values.verticalSensors.right, VERTICAL_SENSOR_COUNT, right);
      }

      case BAUM_RSP_VerticalSensors:
        if (!(switchSettings & BAUM_SWT_DisableSensors)) {
          updateKeyGroup(keysState.leftSensors, packet.data.values.verticalSensors.left,
                         BM_SET_LeftSensors, 0, VERTICAL_SENSOR_COUNT);
          updateKeyGroup(keysState.rightSensors, packet.data.values.verticalSensors.right,
                         BM_SET_RightSensors, 0, VERTICAL_SENSOR_COUNT);
        }
        continue;

      case BAUM_RSP_RoutingKey:
        resetKeyGroup(packet.data.values.routingKeys, cellCount, packet.data.values.routingKey);
        goto doRoutingKeys;

      case BAUM_RSP_RoutingKeys:
        if (baumDeviceType == BAUM_DEVICE_Inka) {
          setInkaSwitches(brl, packet.data.values.switches, 0);
          continue;
        }

      doRoutingKeys:
        updateKeyGroup(keysState.routingKeys, packet.data.values.routingKeys,
                       BM_SET_RoutingKeys, 0, cellCount);
        continue;

      case BAUM_RSP_Switches:
        setBaumSwitches(brl, packet.data.values.switches, 0);
        continue;

      case BAUM_RSP_ModuleRegistration:
        if (handleBaumModuleRegistrationEvent(brl, &packet)) {
        }

        changeCellCount(brl, getBaumModuleCellCount());
        continue;

      case BAUM_RSP_DataRegisters:
        handleBaumDataRegistersEvent(brl, &packet);
        continue;

      case BAUM_RSP_ErrorCode:
        if (packet.data.values.errorCode != BAUM_ERR_PacketType) goto unexpectedPacket;
        LogPrint(LOG_DEBUG, "unsupported request");
        continue;

      default:
      unexpectedPacket:
        logUnexpectedPacket(packet.bytes, size);
        continue;
    }
  }
}

static int
writeBaumCells (BrailleDisplay *brl) {
  if (baumDeviceType == BAUM_DEVICE_Modular) {
    if (!writeBaumDataRegisters(brl, &baumDisplayModule, &externalCells[0], brl->textColumns)) return 0;
    if (!writeBaumDataRegisters(brl, &baumStatusModule, &externalCells[brl->textColumns], brl->statusColumns)) return 0;
    return 1;
  }

  {
    unsigned char packet[1 + 1 + cellCount];
    unsigned char *byte = packet;

    *byte++ = BAUM_REQ_DisplayData;
    if ((baumDeviceType == BAUM_DEVICE_Inka) || (baumDeviceType == BAUM_DEVICE_DM80P))
      *byte++ = 0;

    memcpy(byte, externalCells, cellCount);
    byte += cellCount;

    return writeBaumPacket(brl, packet, byte-packet);
  }
}

static const ProtocolOperations baumOperations = {
  "Baum",
  19200, SERIAL_PARITY_NONE,
  {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80},
  readBaumPacket, writeBaumPacket,
  probeBaumDisplay, updateBaumKeys, writeBaumCells
};

/* HandyTech Protocol */

typedef enum {
  HT_REQ_WRITE = 0X01,
  HT_REQ_RESET = 0XFF
} HandyTechRequestCode;

typedef enum {
  HT_RSP_KEY_B1    = 0X03,
  HT_RSP_KEY_Up    = 0X04,
  HT_RSP_KEY_B2    = 0X07,
  HT_RSP_KEY_Dn    = 0X08,
  HT_RSP_KEY_B3    = 0X0B,
  HT_RSP_KEY_B4    = 0X0F,
  HT_RSP_KEY_CR1   = 0X20,
  HT_RSP_WRITE_ACK = 0X7E,
  HT_RSP_RELEASE   = 0X80,
  HT_RSP_IDENTITY  = 0XFE
} HandyTechResponseCode;
#define HT_IS_ROUTING_KEY(code) (((code) >= HT_RSP_KEY_CR1) && ((code) < (HT_RSP_KEY_CR1 + brl->textColumns)))

typedef union {
  unsigned char bytes[2];

  struct {
    unsigned char code;

    union {
      unsigned char identity;
    } PACKED values;
  } PACKED data;
} PACKED HandyTechResponsePacket;

typedef struct {
  const char *name;
  unsigned char identity;
  unsigned char textCount;
  unsigned char statusCount;
} HandyTechModelEntry;

static const HandyTechModelEntry handyTechModelTable[] = {
  { "Modular 80",
    0X88, 80, 4
  }
  ,
  { "Modular 40",
    0X89, 40, 4
  }
  ,
  {NULL}        
};
static const HandyTechModelEntry *ht;

static int
readHandyTechPacket (BrailleDisplay *brl, unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!readByte(&byte, offset>0)) {
      if (offset > 0) logPartialPacket(packet, offset);
      return 0;
    }

    if (offset < size) {
      if (offset == 0) {
        switch (byte) {
          case HT_RSP_IDENTITY:
            length = 2;
            break;

          case HT_RSP_WRITE_ACK:
            length = 1;
            break;

          default: {
            unsigned char key = byte & ~HT_RSP_RELEASE;
            switch (key) {
              default:
                if (!HT_IS_ROUTING_KEY(key)) {
                  logUnknownPacket(byte);
                  continue;
                }

              case HT_RSP_KEY_Up:
              case HT_RSP_KEY_Dn:
              case HT_RSP_KEY_B1:
              case HT_RSP_KEY_B2:
              case HT_RSP_KEY_B3:
              case HT_RSP_KEY_B4:
                length = 1;
                break;
            }
            break;
          }
        }
      }

      packet[offset] = byte;
    } else {
      if (offset == size) logTruncatedPacket(packet, offset);
      logDiscardedByte(byte);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      logInputPacket(packet, offset);
      return length;
    }
  }
}

static int
getHandyTechPacket (BrailleDisplay *brl, HandyTechResponsePacket *packet) {
  return readHandyTechPacket(brl, packet->bytes, sizeof(*packet));
}

static int
writeHandyTechPacket (BrailleDisplay *brl, const unsigned char *packet, int length) {
  logOutputPacket(packet, length);

  {
    int ok = io->writeBytes(packet, length) != -1;
    if (ok) adjustWriteDelay(brl, length);
    return ok;
  }
}

static const HandyTechModelEntry *
findHandyTechModel (unsigned char identity) {
  const HandyTechModelEntry *model;

  for (model=handyTechModelTable; model->name; ++model) {
    if (identity == model->identity) {
      LogPrint(LOG_INFO, "Baum emulation: HandyTech Model: %02X -> %s", identity, model->name);
      return model;
    }
  }

  LogPrint(LOG_WARNING, "Baum emulation: unknown HandyTech identity code: %02X", identity);
  return NULL;
}

static int
probeHandyTechDisplay (BrailleDisplay *brl) {
  int probes = 0;
  static const unsigned char request[] = {HT_REQ_RESET};
  while (writeHandyTechPacket(brl, request, sizeof(request))) {
    while (io->awaitInput(probeTimeout)) {
      HandyTechResponsePacket response;
      if (getHandyTechPacket(brl, &response)) {
        if (response.data.code == HT_RSP_IDENTITY) {
          if (!(ht = findHandyTechModel(response.data.values.identity))) return 0;
          cellCount = ht->textCount;
          return 1;
        }
      }
    }
    if (errno != EAGAIN) break;
    if (++probes == probeLimit) break;
  }

  return 0;
}

static void
updateHandyTechKeys (BrailleDisplay *brl) {
  HandyTechResponsePacket packet;
  int size;

  while ((size = getHandyTechPacket(brl, &packet))) {
    unsigned char code = packet.data.code;

    switch (code) {
      case HT_RSP_IDENTITY: {
        const HandyTechModelEntry *model = findHandyTechModel(packet.data.values.identity);
        if (model && (model != ht)) {
          ht = model;
          changeCellCount(brl, ht->textCount);
        }
        continue;
      }

      case HT_RSP_WRITE_ACK:
        continue;
    }

    {
      unsigned char *group;
      unsigned char set;
      unsigned char key = code & ~HT_RSP_RELEASE;
      int press = (code & HT_RSP_RELEASE) == 0;

      if (HT_IS_ROUTING_KEY(key)) {
        group = keysState.routingKeys;
        set = BM_SET_RoutingKeys;
        key = key - HT_RSP_KEY_CR1;
      } else {
        group = keysState.navigationKeys;
        set = BM_SET_NavigationKeys;

        switch (key) {
#define KEY(ht,baum) case HT_RSP_KEY_##ht: key = BM_KEY_DISPLAY + baum; break
          KEY(Up, 0);
          KEY(B1, 1);
          KEY(Dn, 2);
          KEY(B2, 3);
          KEY(B3, 4);
          KEY(B4, 5);
#undef KEY

          default:
            logUnexpectedPacket(packet.bytes, size);
            continue;
        }
      }

      if (setGroupedKey(group, key, press)) enqueueKeyEvent(set, key, press);
    }
  }
}

static int
writeHandyTechCells (BrailleDisplay *brl) {
  unsigned char packet[1 + ht->statusCount + ht->textCount];
  unsigned char *byte = packet;

  *byte++ = HT_REQ_WRITE;

  {
    int count = ht->statusCount;
    while (count-- > 0) *byte++ = 0;
  }

  memcpy(byte, externalCells, ht->textCount);
  byte += ht->textCount;

  return writeHandyTechPacket(brl, packet, byte-packet);
}

static const ProtocolOperations handyTechOperations = {
  "HandyTech",
  19200, SERIAL_PARITY_ODD,
  {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80},
  readHandyTechPacket, writeHandyTechPacket,
  probeHandyTechDisplay, updateHandyTechKeys, writeHandyTechCells
};

/* PowerBraille Protocol */

#define PB_BUTTONS0_MARKER    0X60
#define PB1_BUTTONS0_Display6 0X08
#define PB1_BUTTONS0_Display5 0X04
#define PB1_BUTTONS0_Display4 0X02
#define PB1_BUTTONS0_Display2 0X01
#define PB2_BUTTONS0_Display3 0X08
#define PB2_BUTTONS0_Display5 0X04
#define PB2_BUTTONS0_Display1 0X02
#define PB2_BUTTONS0_Display2 0X01

#define PB_BUTTONS1_MARKER    0XE0
#define PB1_BUTTONS1_Display3 0X08
#define PB1_BUTTONS1_Display1 0X02
#define PB2_BUTTONS1_Display6 0X04
#define PB2_BUTTONS1_Display4 0X02

typedef enum {
  PB_REQ_WRITE = 0X04,
  PB_REQ_RESET = 0X0A
} PowerBrailleRequestCode;

typedef enum {
  PB_RSP_IDENTITY = 0X05,
  PB_RSP_SENSORS  = 0X08
} PowerBrailleResponseCode;

typedef union {
  unsigned char bytes[11];

  unsigned char buttons[2];

  struct {
    unsigned char zero;
    unsigned char code;

    union {
      struct {
        unsigned char cells;
        unsigned char dots;
        unsigned char version[4];
        unsigned char checksum[4];
      } PACKED identity;

      struct {
        unsigned char count;
        unsigned char vertical[4];
        unsigned char horizontal[10];
      } PACKED sensors;
    } PACKED values;
  } PACKED data;
} PACKED PowerBrailleResponsePacket;

static int
readPowerBraillePacket (BrailleDisplay *brl, unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!readByte(&byte, offset>0)) {
      if (offset > 0) logPartialPacket(packet, offset);
      return 0;
    }
  haveByte:

    if (offset == 0) {
      if (!byte) {
        length = 2;
      } else if ((byte & PB_BUTTONS0_MARKER) == PB_BUTTONS0_MARKER) {
        length = 2;
      } else {
        logIgnoredByte(byte);
        continue;
      }
    } else if (packet[0]) {
      if ((byte & PB_BUTTONS1_MARKER) != PB_BUTTONS1_MARKER) {
        logShortPacket(packet, offset);
        offset = 0;
        length = 0;
        goto haveByte;
      }
    } else {
      if (offset == 1) {
        switch (byte) {
          case PB_RSP_IDENTITY:
            length = 12;
            break;

          case PB_RSP_SENSORS:
            length = 3;
            break;

          default:
            logUnknownPacket(byte);
            offset = 0;
            length = 0;
            continue;
        }
      } else if ((offset == 2) && (packet[1] == PB_RSP_SENSORS)) {
        length += byte;
      }
    }

    if (offset < length) {
      packet[offset] = byte;
    } else {
      if (offset == size) logTruncatedPacket(packet, offset);
      logDiscardedByte(byte);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      logInputPacket(packet, offset);
      return length;
    }
  }
}

static int
getPowerBraillePacket (BrailleDisplay *brl, PowerBrailleResponsePacket *packet) {
  return readPowerBraillePacket(brl, packet->bytes, sizeof(*packet));
}

static int
writePowerBraillePacket (BrailleDisplay *brl, const unsigned char *packet, int length) {
  unsigned char buffer[2 + length];
  unsigned char *byte = buffer;

  *byte++ = 0XFF;
  *byte++ = 0XFF;

  memcpy(byte, packet, length);
  byte += length;

  {
    int count = byte - buffer;
    logOutputPacket(buffer, count);

    {
      int ok = io->writeBytes(buffer, count) != -1;
      if (ok) adjustWriteDelay(brl, count);
      return ok;
    }
  }
}

static int
probePowerBrailleDisplay (BrailleDisplay *brl) {
  int probes = 0;
  static const unsigned char request[] = {PB_REQ_RESET};
  while (writePowerBraillePacket(brl, request, sizeof(request))) {
    while (io->awaitInput(probeTimeout)) {
      PowerBrailleResponsePacket response;
      if (getPowerBraillePacket(brl, &response)) {
        if (response.data.code == PB_RSP_IDENTITY) {
          const unsigned char *version = response.data.values.identity.version;
          LogPrint(LOG_INFO, "Baum emulation: PowerBraille Version: %c%c%c%c",
                   version[0], version[1], version[2], version[3]);
          cellCount = response.data.values.identity.cells;
          return 1;
        }
      }
    }
    if (errno != EAGAIN) break;
    if (++probes == probeLimit) break;
  }

  return 0;
}

static void
updatePowerBrailleKeys (BrailleDisplay *brl) {
  PowerBrailleResponsePacket packet;
  int size;

  while ((size = getPowerBraillePacket(brl, &packet))) {
    if (!packet.data.zero) {
      switch (packet.data.code) {
        case PB_RSP_IDENTITY:
          changeCellCount(brl, packet.data.values.identity.cells);
          continue;

        case PB_RSP_SENSORS:
          updateKeyGroup(keysState.routingKeys, packet.data.values.sensors.horizontal,
                         BM_SET_RoutingKeys, 0, brl->textColumns);
          continue;

        default:
          break;
      }
    } else {
      unsigned char keys = 0;

#define KEY(key,index) if (packet.buttons[index] & PB2_BUTTONS##index##_Display##key) keys |= 1 << (key - 1)
      KEY(1, 0);
      KEY(2, 0);
      KEY(3, 0);
      KEY(4, 1);
      KEY(5, 0);
      KEY(6, 1);
#undef KEY

      /*
       * The PB emulation is deficient as the protocol doesn't report any
       * key status when all keys are released.  The ability to act on
       * released keys as needed for multiple key combinations is,
       * therefore, an unsolvable problem.  The TSI driver works around
       * this limitation by guessing the "key held" state based on the fact
       * that native Navigator/PowerBraille displays send repeated key
       * status for as long as there is at least one key pressed. Baum's PB
       * emulation, however, doesn't do this.
       *
       * Let's make basic functions act on key presses then.  The limited
       * set of single key bindings will work just fine.  If one is quick
       * enough to press, then release, combined keys all at the same time
       * then combined key functions might even work.  The Brailliant display
       * on which this was tested appears to delay any key update, possibly to
       * mitigate the issue, making combined keys somewhat usable.
       *
       * This is far from perfect, but that's the best we can do. The PB
       * emulation modes (either PB1 or PB2) should simply be avoided
       * whenever possible, and BAUM or HT should be used instead.
       */
      updateDisplayKeys(keys);
      continue;
    }

    logUnexpectedPacket(packet.bytes, size);
  }
}

static int
writePowerBrailleCells (BrailleDisplay *brl) {
  unsigned char packet[6 + (brl->textColumns * 2)];
  unsigned char *byte = packet;

  *byte++ = PB_REQ_WRITE;
  *byte++ = 0; /* cursor mode: disabled */
  *byte++ = 0; /* cursor position: nowhere */
  *byte++ = 1; /* cursor type: command */
  *byte++ = brl->textColumns * 2; /* attribute-data pairs */
  *byte++ = 0; /* start */

  {
    int i;
    for (i=0; i<brl->textColumns; ++i) {
      *byte++ = 0; /* attributes */
      *byte++ = externalCells[i]; /* data */
    }
  }

  return writePowerBraillePacket(brl, packet, byte-packet);
}

static const ProtocolOperations powerBrailleOperations = {
  "PowerBraille",
  9600, SERIAL_PARITY_NONE,
  {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80},
  readPowerBraillePacket, writePowerBraillePacket,
  probePowerBrailleDisplay, updatePowerBrailleKeys, writePowerBrailleCells
};

static const ProtocolOperations *const allProtocols[] = {
  &baumOperations,
  &handyTechOperations,
  &powerBrailleOperations,
  NULL
};

static const ProtocolOperations *const nativeProtocols[] = {
  &baumOperations,
  NULL
};

/* Serial IO */
#include "io_serial.h"

static SerialDevice *serialDevice = NULL;

static int
openSerialPort (const char *device) {
  if ((serialDevice = serialOpenDevice(device))) return 1;
  return 0;
}

static int
configureSerialPort (void) {
  if (!serialRestartDevice(serialDevice, protocol->serialBaud)) return 0;
  if (!serialSetParity(serialDevice, protocol->serialParity)) return 0;
  return 1;
}

static int
awaitSerialInput (int milliseconds) {
  return serialAwaitInput(serialDevice, milliseconds);
}

static int
readSerialBytes (unsigned char *buffer, int count, int wait) {
  const int timeout = 100;
  return serialReadData(serialDevice, buffer, count,
                        (wait? timeout: 0), timeout);
}

static int
writeSerialBytes (const unsigned char *buffer, int length) {
  return serialWriteData(serialDevice, buffer, length);
}

static void
closeSerialPort (void) {
  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
}

static const InputOutputOperations serialOperations = {
  nativeProtocols,
  openSerialPort, configureSerialPort, closeSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialBytes
};

#ifdef ENABLE_USB_SUPPORT
/* USB IO */
#include "io_usb.h"

static UsbChannel *usbChannel = NULL;

static int
openUsbPort (const char *device) {
  static const UsbChannelDefinition definitions[] = {
    { /* Vario40 (40 cells) */
      .vendor=0X0403, .product=0XFE70,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* PocketVario (24 cells) */
      .vendor=0X0403, .product=0XFE71,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* SuperVario 40 (40 cells) */
      .vendor=0X0403, .product=0XFE72,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* SuperVario 32 (32 cells) */
      .vendor=0X0403, .product=0XFE73,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* SuperVario 64 (64 cells) */
      .vendor=0X0403, .product=0XFE74,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* SuperVario 80 (80 cells) */
      .vendor=0X0403, .product=0XFE75,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* VarioPro 80 (80 cells) */
      .vendor=0X0403, .product=0XFE76,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* VarioPro 64 (64 cells) */
      .vendor=0X0403, .product=0XFE77,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* VarioPro 40 (40 cells) */
      .vendor=0X0904, .product=0X2000,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* EcoVario 24 (24 cells) */
      .vendor=0X0904, .product=0X2001,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* EcoVario 40 (40 cells) */
      .vendor=0X0904, .product=0X2002,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* VarioConnect 40 (40 cells) */
      .vendor=0X0904, .product=0X2007,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* VarioConnect 32 (32 cells) */
      .vendor=0X0904, .product=0X2008,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* VarioConnect 24 (24 cells) */
      .vendor=0X0904, .product=0X2009,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* VarioConnect 64 (64 cells) */
      .vendor=0X0904, .product=0X2010,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* VarioConnect 80 (80 cells) */
      .vendor=0X0904, .product=0X2011,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* EcoVario 32 (32 cells) */
      .vendor=0X0904, .product=0X2014,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* EcoVario 64 (64 cells) */
      .vendor=0X0904, .product=0X2015,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* EcoVario 80 (80 cells) */
      .vendor=0X0904, .product=0X2016,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { /* Refreshabraille 18 (18 cells) */
      .vendor=0X0904, .product=0X3000,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1
    }
    ,
    { .vendor=0 }
  };

  if ((usbChannel = usbFindChannel(definitions, (void *)device))) {
    return 1;
  }
  return 0;
}

static int
configureUsbPort (void) {
  const SerialParameters parameters = {
    .baud = protocol->serialBaud,
    .flow = SERIAL_FLOW_NONE,
    .data = 8,
    .stop = 1,
    .parity = protocol->serialParity
  };

  return usbSetSerialParameters(usbChannel->device, &parameters);
}

static int
awaitUsbInput (int milliseconds) {
  return usbAwaitInput(usbChannel->device,
                       usbChannel->definition.inputEndpoint,
                       milliseconds);
}

static int
readUsbBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  int count = usbReapInput(usbChannel->device,
                           usbChannel->definition.inputEndpoint,
                           buffer, length,
                           (wait? timeout: 0), timeout);
  if (count != -1) return count;
  if (errno == EAGAIN) return 0;
  return -1;
}

static int
writeUsbBytes (const unsigned char *buffer, int length) {
  return usbWriteEndpoint(usbChannel->device,
                          usbChannel->definition.outputEndpoint,
                          buffer, length, 1000);
}

static void
closeUsbPort (void) {
  if (usbChannel) {
    usbCloseChannel(usbChannel);
    usbChannel = NULL;
  }
}

static const InputOutputOperations usbOperations = {
  allProtocols,
  openUsbPort, configureUsbPort, closeUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes
};
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
/* Bluetooth IO */
#include "io_bluetooth.h"
#include "io_misc.h"

static int bluetoothConnection = -1;

static int
openBluetoothPort (const char *device) {
  return (bluetoothConnection = btOpenConnection(device, 1, 0)) != -1;
}

static int
configureBluetoothPort (void) {
  return 1;
}

static int
awaitBluetoothInput (int milliseconds) {
  return awaitInput(bluetoothConnection, milliseconds);
}

static int
readBluetoothBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  size_t offset = 0;
  return readChunk(bluetoothConnection,
                   buffer, &offset, length,
                   (wait? timeout: 0), timeout);
}

static int
writeBluetoothBytes (const unsigned char *buffer, int length) {
  int count = writeData(bluetoothConnection, buffer, length);
  if (count != length) {
    if (count == -1) {
      LogError("Baum Bluetooth write");
    } else {
      LogPrint(LOG_WARNING, "Trunccated bluetooth write: %d < %d", count, length);
    }
  }
  return count;
}

static void
closeBluetoothPort (void) {
  close(bluetoothConnection);
  bluetoothConnection = -1;
}

static const InputOutputOperations bluetoothOperations = {
  allProtocols,
  openBluetoothPort, configureBluetoothPort, closeBluetoothPort,
  awaitBluetoothInput, readBluetoothBytes, writeBluetoothBytes
};
#endif /* ENABLE_BLUETOOTH_SUPPORT */

/* Driver Handlers */

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  const ProtocolOperations *const *requestedProtocols;

  {
    static const ProtocolOperations *const *const values[] = {
      NULL,
      allProtocols,
      nativeProtocols
    };
    static const char *choices[] = {"default", "all", "native", NULL};
    unsigned int index;

    if (!validateChoice(&index, parameters[PARM_PROTOCOLS], choices))
      LogPrint(LOG_WARNING, "%s: %s", "invalid protocols setting", parameters[PARM_PROTOCOLS]);
    requestedProtocols = values[index];
  }

  useVarioKeys = 0;
  if (!validateYesNo(&useVarioKeys, parameters[PARM_VARIOKEYS]))
    LogPrint(LOG_WARNING, "%s: %s", "invalid vario keys setting", parameters[PARM_VARIOKEYS]);

  if (isSerialDevice(&device)) {
    io = &serialOperations;
  } else

#ifdef ENABLE_USB_SUPPORT
  if (isUsbDevice(&device)) {
    io = &usbOperations;
  } else
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
  if (isBluetoothDevice(&device)) {
    io = &bluetoothOperations;
  } else
#endif /* ENABLE_BLUETOOTH_SUPPORT */

  {
    unsupportedDevice(device);
    return 0;
  }

  if (io->openPort(device)) {
    int attempts = 0;

    while (1) {
      const ProtocolOperations *const *protocolAddress = requestedProtocols;

      if (!protocolAddress) protocolAddress = io->protocols;

      while ((protocol = *(protocolAddress++))) {
        LogPrint(LOG_DEBUG, "probing with %s protocol", protocol->name);
        makeOutputTable(protocol->dotsTable, outputTable);

        {
          int bits = 10;
          if (protocol->serialParity != SERIAL_PARITY_NONE) ++bits;
          charactersPerSecond = protocol->serialBaud / bits;
        }

        if (io->configurePort()) {
          if (!flushInput()) goto failed;

          memset(&keysState, 0, sizeof(keysState));
          switchSettings = 0;

          if (protocol->probeDisplay(brl)) {
            logCellCount(brl);

            clearCells(0, cellCount);
            if (!updateCells(brl)) goto failed;

            {
              const KeyTableDefinition *ktd;

              if (useVarioKeys) {
                ktd = &KEY_TABLE_DEFINITION(vario);
              } else {
                ktd = &KEY_TABLE_DEFINITION(all);
              }

              brl->keyBindings = ktd->bindings;
              brl->keyNameTables = ktd->names;
            }

            return 1;
          }
        }
      }

      if (++attempts == 2) break;
      approximateDelay(700);
    }

  failed:
    io->closePort();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  io->closePort();
}

static ssize_t
brl_readPacket (BrailleDisplay *brl, void *buffer, size_t size) {
  int count = protocol->readPacket(brl, buffer, size);
  if (!count) count = -1;
  return count;
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const void *packet, size_t length) {
  return protocol->writePacket(brl, packet, length)? length: -1;
}

static int
brl_reset (BrailleDisplay *brl) {
  return 0;
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  int start = 0;
  int count = brl->textColumns;

  while (count > 0) {
    if (brl->buffer[count-1] != internalCells[count-1]) break;
    count -= 1;
  }

  while (start < count) {
    if (brl->buffer[start] != internalCells[start]) break;
    start += 1;
  }
  count -= start;

  memcpy(&internalCells[start], &brl->buffer[start], count);
  translateCells(start, count);

  return updateCells(brl);
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  if (memcmp(&internalCells[brl->textColumns], status, brl->statusColumns) != 0) {
    memcpy(&internalCells[brl->textColumns], status, brl->statusColumns);
    translateCells(brl->textColumns, brl->statusColumns);
  }
  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  protocol->updateKeys(brl);
  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
