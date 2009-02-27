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

/* Global Definitions */

static unsigned int useVarioKeys;
static const unsigned int logInputPackets = 0;
static const unsigned int logOutputPackets = 0;
static const int probeLimit = 2;
static const int probeTimeout = 200;

#define KEY_GROUP_SIZE(count) (((count) + 7) / 8)
#define MAXIMUM_CELL_COUNT 84
#define VERTICAL_SENSOR_COUNT 27

static int cellCount;
static int textCount;
static int statusCount;
static int cellsUpdated;
static unsigned char internalCells[MAXIMUM_CELL_COUNT];
static unsigned char externalCells[MAXIMUM_CELL_COUNT];
static TranslationTable outputTable;

typedef struct {
  uint64_t functionKeys;
  unsigned char routingKeys[KEY_GROUP_SIZE(MAXIMUM_CELL_COUNT)];
  unsigned char horizontalSensors[KEY_GROUP_SIZE(MAXIMUM_CELL_COUNT)];
  unsigned char leftVerticalSensors[KEY_GROUP_SIZE(VERTICAL_SENSOR_COUNT)];
  unsigned char rightVerticalSensors[KEY_GROUP_SIZE(VERTICAL_SENSOR_COUNT)];
} Keys;

static Keys activeKeys;
static Keys pressedKeys;
static unsigned char switchSettings;
static int pendingCommand;

static int currentModifiers;
#define MOD_INPUT      0X0001
#define MOD_INPUT_ONCE 0X0002
#define MOD_CHORD      0X0004
#define MOD_DOT7       0X0010
#define MOD_DOT7_LOCK  0X0020
#define MOD_DOT8       0X0040
#define MOD_DOT8_LOCK  0X0080
#define MOD_UPPER      0X0100
#define MOD_UPPER_LOCK 0X0200
#define MOD_META       0X0400
#define MOD_CNTRL      0X0800

typedef struct {
  const char *name;
  int serialBaud;
  SerialParity serialParity;
  int (*readPacket) (unsigned char *packet, int size);
  int (*writePacket) (BrailleDisplay *brl, const unsigned char *packet, int length);
  int (*probeDisplay) (BrailleDisplay *brl);
  int (*updateKeys) (BrailleDisplay *brl, int *keyPressed);
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
updateFunctionKeys (uint64_t keys, uint64_t mask, unsigned int shift, int *pressed) {
  if (shift) {
    keys <<= shift;
    mask <<= shift;
  }

  keys |= pressedKeys.functionKeys & ~mask;
  if (keys == pressedKeys.functionKeys) return 0;

  if (keys & ~pressedKeys.functionKeys) *pressed = 1;
  pressedKeys.functionKeys = keys;
  return 1;
}

static int
updateFunctionKeyByte (unsigned char keys, unsigned int shift, unsigned int width, int *pressed) {
  return updateFunctionKeys(keys, ((1 << width) - 1), shift, pressed);
}

static int
setGroupedKey (unsigned char *keys, int number, int press) {
  unsigned char *byte = &keys[number / 8];
  unsigned char bit = 1 << (number % 8);

  if (!(*byte & bit) == !press) return 0;

  if (press) {
    *byte |= bit;
  } else {
    *byte &= ~bit;
  }
  return 1;
}

static void
clearKeyGroup (unsigned char *keys, int count) {
  memset(keys, 0, KEY_GROUP_SIZE(count));
}

static void
resetKeyGroup (unsigned char *keys, int count, unsigned char key) {
  clearKeyGroup(keys, count);
  if (key > 0) setGroupedKey(keys, key-1, 1);
}

static int
updateKeyGroup (unsigned char *keys, const unsigned char *new, int count, int *pressed) {
  int size = KEY_GROUP_SIZE(count);
  int changed = 0;

  while (size-- > 0) {
    if (*new != *keys) {
      changed = 1;
      if (*new & ~*keys) *pressed = 1;
      *keys = *new;
    }

    ++new;
    ++keys;
  }

  return changed;
}

static int
getKeyNumbers (const unsigned char *keys, int count, unsigned char *numbers) {
  int size = KEY_GROUP_SIZE(count);
  unsigned char *next = numbers;
  unsigned char number = 0;
  int index;
  for (index=0; index<size; ++index) {
    unsigned char byte = keys[index];
    if (byte) {
      unsigned char bit;
      for (bit=1; bit; bit<<=1) {
        if (byte & bit) *next++ = number;
        ++number;
      }
    } else {
      number += 8;
    }
  }
  return next - numbers;
}

static signed char
getSensorNumber (const unsigned char *keys, int count) {
  unsigned char numbers[count];
  int pressed = getKeyNumbers(keys, count, numbers);
  return pressed? numbers[0]: -1;
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
logCellCount (void) {
  switch ((textCount = cellCount)) {
    case 44:
    case 84:
      textCount -= 4;
      break;

    case 56:
      textCount -= 16;
      break;
  }
  statusCount = cellCount - textCount;

  LogPrint(LOG_INFO, "Cell Count: %d (%d text, %d status)",
           cellCount, textCount, statusCount);
}

static void
changeCellCount (BrailleDisplay *brl, int count) {
  if (count != cellCount) {
    if (count > cellCount) {
      clearCells(cellCount, count-cellCount);

      {
        int number;
        for (number=cellCount; number<count; ++number) {
          setGroupedKey(pressedKeys.routingKeys, number, 0);
          setGroupedKey(pressedKeys.horizontalSensors, number, 0);
        }
      }
    }

    cellCount = count;
    logCellCount();

    brl->textColumns = textCount;
    brl->resizeRequired = 1;
  }
}

/* Baum Protocol */

#define ESCAPE 0X1B

typedef enum {
  BAUM_REQ_DisplayData             = 0X01,
  BAUM_REQ_GetVersionNumber        = 0X05,
  BAUM_REQ_GetKeys                 = 0X08,
  BAUM_REQ_GetMode                 = 0X11,
  BAUM_REQ_SetMode                 = 0X12,
  BAUM_REQ_SetProtocolState        = 0X15,
  BAUM_REQ_SetCommunicationChannel = 0X16,
  BAUM_REQ_CausePowerdown          = 0X17,
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
  BAUM_RSP_TopKeys              = 0X24,
  BAUM_RSP_HorizontalSensor     = 0X25,
  BAUM_RSP_VerticalSensor       = 0X26,
  BAUM_RSP_RoutingKey           = 0X27,
  BAUM_RSP_FrontKeys6           = 0X28,
  BAUM_RSP_BackKeys6            = 0X29,
  BAUM_RSP_CommandKeys          = 0X2B,
  BAUM_RSP_FrontKeys10          = 0X2C,
  BAUM_RSP_BackKeys10           = 0X2D,
  BAUM_RSP_EntryKeys            = 0X33,
  BAUM_RSP_JoyStick             = 0X34,
  BAUM_RSP_ErrorCode            = 0X40,
  BAUM_RSP_DeviceIdentity       = 0X84,
  BAUM_RSP_SerialNumber         = 0X8A,
  BAUM_RSP_BluetoothName        = 0X8C
} BaumResponseCode;

typedef enum {
  BAUM_MODE_KeyGroupCompressed          = 0X01,
  BAUM_MODE_HorizontalSensorsEnabled    = 0X06,
  BAUM_MODE_LeftVerticalSensorsEnabled  = 0X07,
  BAUM_MODE_RoutingKeysEnabled          = 0X08,
  BAUM_MODE_RightVerticalSensorsEnabled = 0X09,
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

#define BAUM_KEY(bit,type) (UINT64_C(bit) << BAUM_SHIFT_##type)

#define BAUM_SHIFT_TOP 0
#define BAUM_WIDTH_TOP 6
#define BAUM_KEY_TL1 BAUM_KEY(0X01, TOP)
#define BAUM_KEY_TL2 BAUM_KEY(0X02, TOP)
#define BAUM_KEY_TL3 BAUM_KEY(0X04, TOP)
#define BAUM_KEY_TR1 BAUM_KEY(0X08, TOP)
#define BAUM_KEY_TR2 BAUM_KEY(0X10, TOP)
#define BAUM_KEY_TR3 BAUM_KEY(0X20, TOP)

#define BAUM_SHIFT_COMMAND (BAUM_SHIFT_TOP + BAUM_WIDTH_TOP)
#define BAUM_WIDTH_COMMAND 7
#define BAUM_KEY_CK1 BAUM_KEY(0X01, COMMAND)
#define BAUM_KEY_CK2 BAUM_KEY(0X02, COMMAND)
#define BAUM_KEY_CK3 BAUM_KEY(0X04, COMMAND)
#define BAUM_KEY_CK4 BAUM_KEY(0X08, COMMAND)
#define BAUM_KEY_CK5 BAUM_KEY(0X10, COMMAND)
#define BAUM_KEY_CK6 BAUM_KEY(0X20, COMMAND)
#define BAUM_KEY_CK7 BAUM_KEY(0X40, COMMAND)

#define BAUM_SHIFT_FRONT10B (BAUM_SHIFT_COMMAND + BAUM_WIDTH_COMMAND)
#define BAUM_WIDTH_FRONT10B 8
#define BAUM_KEY_FK10 BAUM_KEY(0X01, FRONT10B)
#define BAUM_KEY_FK9  BAUM_KEY(0X02, FRONT10B)
#define BAUM_KEY_FK8  BAUM_KEY(0X04, FRONT10B)
#define BAUM_KEY_FK7  BAUM_KEY(0X08, FRONT10B)
#define BAUM_KEY_FK6  BAUM_KEY(0X10, FRONT10B)
#define BAUM_KEY_FK5  BAUM_KEY(0X20, FRONT10B)
#define BAUM_KEY_FK4  BAUM_KEY(0X40, FRONT10B)
#define BAUM_KEY_FK3  BAUM_KEY(0X80, FRONT10B)

#define BAUM_SHIFT_FRONT10A (BAUM_SHIFT_FRONT10B + BAUM_WIDTH_FRONT10B)
#define BAUM_WIDTH_FRONT10A 2
#define BAUM_KEY_FK2  BAUM_KEY(0X01, FRONT10A)
#define BAUM_KEY_FK1  BAUM_KEY(0X02, FRONT10A)

#define BAUM_SHIFT_FRONT6 (BAUM_SHIFT_FRONT10B + 2)
#define BAUM_WIDTH_FRONT6 6
#define BAUM_KEY_FRD BAUM_KEY(0X01, FRONT6)
#define BAUM_KEY_FRU BAUM_KEY(0X02, FRONT6)
#define BAUM_KEY_FMD BAUM_KEY(0X04, FRONT6)
#define BAUM_KEY_FMU BAUM_KEY(0X08, FRONT6)
#define BAUM_KEY_FLD BAUM_KEY(0X10, FRONT6)
#define BAUM_KEY_FLU BAUM_KEY(0X20, FRONT6)

#define BAUM_SHIFT_BACK10B (BAUM_SHIFT_FRONT10A + BAUM_WIDTH_FRONT10A)
#define BAUM_WIDTH_BACK10B 8
#define BAUM_KEY_BK10 BAUM_KEY(0X01, BACK10B)
#define BAUM_KEY_BK9  BAUM_KEY(0X02, BACK10B)
#define BAUM_KEY_BK8  BAUM_KEY(0X04, BACK10B)
#define BAUM_KEY_BK7  BAUM_KEY(0X08, BACK10B)
#define BAUM_KEY_BK6  BAUM_KEY(0X10, BACK10B)
#define BAUM_KEY_BK5  BAUM_KEY(0X20, BACK10B)
#define BAUM_KEY_BK4  BAUM_KEY(0X40, BACK10B)
#define BAUM_KEY_BK3  BAUM_KEY(0X80, BACK10B)

#define BAUM_SHIFT_BACK10A (BAUM_SHIFT_BACK10B + BAUM_WIDTH_BACK10B)
#define BAUM_WIDTH_BACK10A 2
#define BAUM_KEY_BK2  BAUM_KEY(0X01, BACK10A)
#define BAUM_KEY_BK1  BAUM_KEY(0X02, BACK10A)

#define BAUM_SHIFT_BACK6 (BAUM_SHIFT_BACK10B + 2)
#define BAUM_WIDTH_BACK6 6
#define BAUM_KEY_BRD BAUM_KEY(0X01, BACK6)
#define BAUM_KEY_BRU BAUM_KEY(0X02, BACK6)
#define BAUM_KEY_BMD BAUM_KEY(0X04, BACK6)
#define BAUM_KEY_BMU BAUM_KEY(0X08, BACK6)
#define BAUM_KEY_BLD BAUM_KEY(0X10, BACK6)
#define BAUM_KEY_BLU BAUM_KEY(0X20, BACK6)

#define BAUM_SHIFT_DOT (BAUM_SHIFT_BACK10A + BAUM_WIDTH_BACK10A)
#define BAUM_WIDTH_DOT 8
#define BAUM_KEY_DOT1 BAUM_KEY(0X01, DOT)
#define BAUM_KEY_DOT2 BAUM_KEY(0X02, DOT)
#define BAUM_KEY_DOT3 BAUM_KEY(0X04, DOT)
#define BAUM_KEY_DOT4 BAUM_KEY(0X08, DOT)
#define BAUM_KEY_DOT5 BAUM_KEY(0X10, DOT)
#define BAUM_KEY_DOT6 BAUM_KEY(0X20, DOT)
#define BAUM_KEY_DOT7 BAUM_KEY(0X40, DOT)
#define BAUM_KEY_DOT8 BAUM_KEY(0X80, DOT)

#define BAUM_SHIFT_BUTTON (BAUM_SHIFT_DOT + BAUM_WIDTH_DOT)
#define BAUM_WIDTH_BUTTON 8
#define BAUM_KEY_B9  BAUM_KEY(0X01, BUTTON)
#define BAUM_KEY_B0  BAUM_KEY(0X02, BUTTON)
#define BAUM_KEY_B11 BAUM_KEY(0X04, BUTTON)
#define BAUM_KEY_F1  BAUM_KEY(0X10, BUTTON)
#define BAUM_KEY_F2  BAUM_KEY(0X20, BUTTON)
#define BAUM_KEY_F3  BAUM_KEY(0X40, BUTTON)
#define BAUM_KEY_F4  BAUM_KEY(0X80, BUTTON)

#define BAUM_SHIFT_JOYSTICK (BAUM_SHIFT_BUTTON + BAUM_WIDTH_BUTTON)
#define BAUM_WIDTH_JOYSTICK 5
#define BAUM_KEY_UP    BAUM_KEY(0X01, JOYSTICK)
#define BAUM_KEY_LEFT  BAUM_KEY(0X02, JOYSTICK)
#define BAUM_KEY_DOWN  BAUM_KEY(0X04, JOYSTICK)
#define BAUM_KEY_RIGHT BAUM_KEY(0X08, JOYSTICK)
#define BAUM_KEY_PRESS BAUM_KEY(0X10, JOYSTICK)

#define BAUM_SHIFT_SENSOR (BAUM_SHIFT_JOYSTICK + BAUM_WIDTH_JOYSTICK)
#define BAUM_WIDTH_SENSOR 3
#define BAUM_KEY_HRZ BAUM_KEY(0X1, SENSOR)
#define BAUM_KEY_VTL BAUM_KEY(0X2, SENSOR)
#define BAUM_KEY_VTR BAUM_KEY(0X4, SENSOR)

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

#define BAUM_DEVICE_IDENTITY_LENGTH 16
#define BAUM_SERIAL_NUMBER_LENGTH 8
#define BAUM_BLUETOOTH_NAME_LENGTH 14

typedef union {
  unsigned char bytes[17];

  struct {
    unsigned char code;

    union {
      unsigned char horizontalSensors[KEY_GROUP_SIZE(MAXIMUM_CELL_COUNT)];
      struct {
        unsigned char left[KEY_GROUP_SIZE(VERTICAL_SENSOR_COUNT)];
        unsigned char right[KEY_GROUP_SIZE(VERTICAL_SENSOR_COUNT)];
      } PACKED verticalSensors;
      unsigned char routingKeys[KEY_GROUP_SIZE(MAXIMUM_CELL_COUNT)];
      unsigned char switches;
      unsigned char topKeys;
      unsigned char horizontalSensor;
      union {
        unsigned char left;
        unsigned char right;
      } PACKED verticalSensor;
      unsigned char routingKey;
      unsigned char frontKeys6;
      unsigned char backKeys6;
      unsigned char commandKeys;
      unsigned char frontKeys10[2];
      unsigned char backKeys10[2];
      struct {
        unsigned char buttons;
        unsigned char dots;
      } PACKED entryKeys;
      unsigned char joyStick;

      unsigned char cellCount;
      unsigned char versionNumber;
      unsigned char communicationChannel;
      unsigned char powerdownReason;

      struct {
        unsigned char identifier;
        unsigned char setting;
      } PACKED mode;

      char deviceIdentity[BAUM_DEVICE_IDENTITY_LENGTH];
      char serialNumber[BAUM_SERIAL_NUMBER_LENGTH];
      char bluetoothName[BAUM_BLUETOOTH_NAME_LENGTH];
    } PACKED values;
  } PACKED data;
} PACKED BaumResponsePacket;

typedef enum {
  BAUM_TYPE_Inka,
  BAUM_TYPE_DM80P,
  BAUM_TYPE_Generic
} BaumDeviceType;
static BaumDeviceType baumDeviceType;

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
readBaumPacket (unsigned char *packet, int size) {
  int started = 0;
  int escape = 0;
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!readByte(&byte, (started || escape))) {
      if (offset > 0) LogBytes(LOG_WARNING, "Partial Packet", packet, offset);
      return 0;
    }

    if (byte == ESCAPE) {
      if ((escape = !escape)) continue;
    } else if (escape) {
      escape = 0;

      if (offset > 0) {
        LogBytes(LOG_WARNING, "Short Packet", packet, offset);
        offset = 0;
        length = 0;
      } else {
        started = 1;
      }
    }

    if (!started) {
      LogBytes(LOG_WARNING, "Ignored Byte", &byte, 1);
      continue;
    }

    if (offset < size) {
      if (offset == 0) {
        switch (byte) {
          case BAUM_RSP_Switches:
            if (!cellCount) {
              assumeBaumDeviceIdentity("DM80P");
              baumDeviceType = BAUM_TYPE_DM80P;
              cellCount = 84;
            }

          case BAUM_RSP_CellCount:
          case BAUM_RSP_VersionNumber:
          case BAUM_RSP_CommunicationChannel:
          case BAUM_RSP_PowerdownSignal:
          case BAUM_RSP_TopKeys:
          case BAUM_RSP_HorizontalSensor:
          case BAUM_RSP_RoutingKey:
          case BAUM_RSP_FrontKeys6:
          case BAUM_RSP_BackKeys6:
          case BAUM_RSP_CommandKeys:
          case BAUM_RSP_JoyStick:
          case BAUM_RSP_ErrorCode:
            length = 2;
            break;

          case BAUM_RSP_ModeSetting:
          case BAUM_RSP_FrontKeys10:
          case BAUM_RSP_BackKeys10:
          case BAUM_RSP_EntryKeys:
            length = 3;
            break;

          case BAUM_RSP_VerticalSensor:
            length = (baumDeviceType == BAUM_TYPE_Inka)? 2: 3;
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
              baumDeviceType = BAUM_TYPE_Inka;
              cellCount = 56;
            }

            if (baumDeviceType == BAUM_TYPE_Inka) {
              length = 2;
              break;
            }

            length = KEY_GROUP_SIZE(cellCount) + 1;
            break;

          case BAUM_RSP_HorizontalSensors:
            length = KEY_GROUP_SIZE(textCount) + 1;
            break;

          default:
            LogBytes(LOG_WARNING, "Unknown Packet", &byte, 1);
            started = 0;
            continue;
        }
      }

      packet[offset] = byte;
    } else {
      if (offset == size) LogBytes(LOG_WARNING, "Truncated Packet", packet, offset);
      LogBytes(LOG_WARNING, "Discarded Byte", &byte, 1);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        started = 0;
        continue;
      }

      if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", packet, offset);
      return length;
    }
  }
}

static int
getBaumPacket (BaumResponsePacket *packet) {
  return readBaumPacket(packet->bytes, sizeof(*packet));
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
    if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", buffer, count);

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
probeBaumDisplay (BrailleDisplay *brl) {
  int probes = 0;
  while (1) {
    int assumedCellCount = 0;

    {
      static const unsigned char request[] = {BAUM_REQ_GetDeviceIdentity};
      if (!writeBaumPacket(brl, request, sizeof(request))) break;
    }

    {
      static const unsigned char request[] = {BAUM_REQ_GetSerialNumber};
      if (!writeBaumPacket(brl, request, sizeof(request))) break;
    }

    {
      static const unsigned char request[] = {BAUM_REQ_DisplayData, 0};
      if (!writeBaumPacket(brl, request, sizeof(request))) break;
    }

    {
      static const unsigned char request[] = {BAUM_REQ_GetKeys};
      if (!writeBaumPacket(brl, request, sizeof(request))) break;
    }

    baumDeviceType = BAUM_TYPE_Generic;
    cellCount = 0;
    while (io->awaitInput(probeTimeout)) {
      BaumResponsePacket response;
      int size = getBaumPacket(&response);
      if (size) {
        switch (response.data.code) {
          case BAUM_RSP_RoutingKeys: /* Inka */
            setInkaSwitches(brl, response.data.values.switches, 1);
            return 1;

          case BAUM_RSP_Switches: /* DM80P */
            setBaumSwitches(brl, response.data.values.switches, 1);
            return 1;

          case BAUM_RSP_CellCount: /* newer models */
            if ((response.data.values.cellCount < 1) ||
                (response.data.values.cellCount > MAXIMUM_CELL_COUNT)) {
              LogPrint(LOG_DEBUG, "Unexpected Cell Count: %u",
                       response.data.values.cellCount);
              continue;
            }

            cellCount = response.data.values.cellCount;
            return 1;

          case BAUM_RSP_DeviceIdentity:
            logBaumDeviceIdentity(&response);
            {
              const int length = sizeof(response.data.values.deviceIdentity);
              char buffer[length+1];
              memcpy(buffer, response.data.values.deviceIdentity, length);
              buffer[length] = 0;

              {
                char *digits = strpbrk(buffer, "123456789");
                if (digits) assumedCellCount = atoi(digits);
              }
            }
            continue;

          case BAUM_RSP_SerialNumber:
            logBaumSerialNumber(&response);
            continue;

          default:
            LogBytes(LOG_WARNING, "Unexpected Packet", response.bytes, size);
            continue;
        }
      } else if (errno != EAGAIN) {
        break;
      }
    }
    if (errno != EAGAIN) break;
    if (assumedCellCount) {
      cellCount = assumedCellCount;
      return 1;
    }
    if (++probes == probeLimit) break;
  }

  return 0;
}

static int
updateBaumKeys (BrailleDisplay *brl, int *keyPressed) {
  BaumResponsePacket packet;
  int size;

  while ((size = getBaumPacket(&packet))) {
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
        return 0;

      case BAUM_RSP_TopKeys: {
        unsigned char keys;

        switch (baumDeviceType) {
          case BAUM_TYPE_Inka:
            keys = 0;
#define KEY(bit,name) if (!(packet.data.values.topKeys & (bit))) keys |= BAUM_KEY_##name
            KEY(004, TL1);
            KEY(002, TL2);
            KEY(001, TL3);
            KEY(040, TR1);
            KEY(020, TR2);
            KEY(010, TR3);
#undef KEY
            break;

          case BAUM_TYPE_DM80P:
            keys = packet.data.values.topKeys ^ 0X7F;
            break;

          default:
            keys = packet.data.values.topKeys;
            break;
        }

        if (updateFunctionKeyByte(keys, BAUM_SHIFT_TOP, BAUM_WIDTH_TOP, keyPressed)) return 1;
        continue;
      }

      case BAUM_RSP_CommandKeys:
        if (updateFunctionKeyByte(packet.data.values.commandKeys,
                                  BAUM_SHIFT_COMMAND, BAUM_WIDTH_COMMAND, keyPressed)) return 1;
        continue;

      case BAUM_RSP_FrontKeys6:
        if (updateFunctionKeyByte(packet.data.values.frontKeys6,
                                  BAUM_SHIFT_FRONT6, BAUM_WIDTH_FRONT6, keyPressed)) return 1;
        continue;

      case BAUM_RSP_BackKeys6:
        if (updateFunctionKeyByte(packet.data.values.backKeys6,
                                  BAUM_SHIFT_BACK6, BAUM_WIDTH_BACK6, keyPressed)) return 1;
        continue;

      case BAUM_RSP_FrontKeys10: {
        int changed = 0;
        if (updateFunctionKeyByte(packet.data.values.frontKeys10[0],
                                  BAUM_SHIFT_FRONT10A, BAUM_WIDTH_FRONT10A, keyPressed)) changed = 1;
        if (updateFunctionKeyByte(packet.data.values.frontKeys10[1],
                                  BAUM_SHIFT_FRONT10B, BAUM_WIDTH_FRONT10B, keyPressed)) changed = 1;
        if (changed) return 1;
        continue;
      }

      case BAUM_RSP_BackKeys10: {
        int changed = 0;
        if (updateFunctionKeyByte(packet.data.values.backKeys10[0],
                                  BAUM_SHIFT_BACK10A, BAUM_WIDTH_BACK10A, keyPressed)) changed = 1;
        if (updateFunctionKeyByte(packet.data.values.backKeys10[1],
                                  BAUM_SHIFT_BACK10B, BAUM_WIDTH_BACK10B, keyPressed)) changed = 1;
        if (changed) return 1;
        continue;
      }

      case BAUM_RSP_EntryKeys: {
        int changed = 0;
        if (updateFunctionKeyByte(packet.data.values.entryKeys.dots,
                                  BAUM_SHIFT_DOT, BAUM_WIDTH_DOT, keyPressed)) changed = 1;
        if (updateFunctionKeyByte(packet.data.values.entryKeys.buttons,
                                  BAUM_SHIFT_BUTTON, BAUM_WIDTH_BUTTON, keyPressed)) changed = 1;
        if (changed) return 1;
        continue;
      }

      case BAUM_RSP_JoyStick:
        if (updateFunctionKeyByte(packet.data.values.joyStick,
                                  BAUM_SHIFT_JOYSTICK, BAUM_WIDTH_JOYSTICK, keyPressed)) return 1;
        continue;

      case BAUM_RSP_HorizontalSensor:
        resetKeyGroup(packet.data.values.horizontalSensors, textCount, packet.data.values.horizontalSensor);
      case BAUM_RSP_HorizontalSensors:
        if (updateKeyGroup(pressedKeys.horizontalSensors, packet.data.values.horizontalSensors, textCount, keyPressed)) return 1;
        continue;

      case BAUM_RSP_VerticalSensor: {
        unsigned char left = packet.data.values.verticalSensor.left;
        unsigned char right;
        if (baumDeviceType != BAUM_TYPE_Inka) {
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
      case BAUM_RSP_VerticalSensors: {
        int changed = 0;
        if (updateKeyGroup(pressedKeys.leftVerticalSensors, packet.data.values.verticalSensors.left, VERTICAL_SENSOR_COUNT, keyPressed)) changed = 1;
        if (updateKeyGroup(pressedKeys.rightVerticalSensors, packet.data.values.verticalSensors.right, VERTICAL_SENSOR_COUNT, keyPressed)) changed = 1;
        if (changed) return 1;
        continue;
      }

      case BAUM_RSP_RoutingKey:
        resetKeyGroup(packet.data.values.routingKeys, cellCount, packet.data.values.routingKey);
        goto doRoutingKeys;
      case BAUM_RSP_RoutingKeys:
        if (baumDeviceType == BAUM_TYPE_Inka) {
          setInkaSwitches(brl, packet.data.values.switches, 0);
          continue;
        }

      doRoutingKeys:
        if (updateKeyGroup(pressedKeys.routingKeys, packet.data.values.routingKeys, cellCount, keyPressed)) return 1;
        continue;

      case BAUM_RSP_Switches:
        setBaumSwitches(brl, packet.data.values.switches, 0);
        continue;

      default:
        LogBytes(LOG_WARNING, "Unexpected Packet", packet.bytes, size);
        continue;
    }
  }

  return 0;
}

static int
writeBaumCells (BrailleDisplay *brl) {
  unsigned char packet[1 + 1 + cellCount];
  unsigned char *byte = packet;

  *byte++ = BAUM_REQ_DisplayData;
  if ((baumDeviceType == BAUM_TYPE_Inka) || (baumDeviceType == BAUM_TYPE_DM80P))
    *byte++ = 0;

  memcpy(byte, externalCells, cellCount);
  byte += cellCount;

  return writeBaumPacket(brl, packet, byte-packet);
}

static const ProtocolOperations baumOperations = {
  "Baum",
  19200, SERIAL_PARITY_NONE,
  readBaumPacket, writeBaumPacket,
  probeBaumDisplay, updateBaumKeys, writeBaumCells
};

/* HandyTech Protocol */

typedef enum {
  HT_REQ_WRITE = 0X01,
  HT_REQ_RESET = 0XFF
} HandyTechRequestCode;

typedef enum {
  HT_RSP_KEY_TL1   = 0X04, /* UP */
  HT_RSP_KEY_TL2   = 0X03, /* B1 */
  HT_RSP_KEY_TL3   = 0X08, /* DN */
  HT_RSP_KEY_TR1   = 0X07, /* B2 */
  HT_RSP_KEY_TR2   = 0X0B, /* B3 */
  HT_RSP_KEY_TR3   = 0X0F, /* B4 */
  HT_RSP_KEY_CR1   = 0X20,
  HT_RSP_WRITE_ACK = 0X7E,
  HT_RSP_RELEASE   = 0X80,
  HT_RSP_IDENTITY  = 0XFE
} HandyTechResponseCode;
#define HT_IS_ROUTING_KEY(code) (((code) >= HT_RSP_KEY_CR1) && ((code) < (HT_RSP_KEY_CR1 + textCount)))

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
readHandyTechPacket (unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!readByte(&byte, offset>0)) {
      if (offset > 0) LogBytes(LOG_WARNING, "Partial Packet", packet, offset);
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
                  LogBytes(LOG_WARNING, "Unknown Packet", &byte, 1);
                  continue;
                }

              case HT_RSP_KEY_TL1:
              case HT_RSP_KEY_TL2:
              case HT_RSP_KEY_TL3:
              case HT_RSP_KEY_TR1:
              case HT_RSP_KEY_TR2:
              case HT_RSP_KEY_TR3:
                length = 1;
                break;
            }
            break;
          }
        }
      }

      packet[offset] = byte;
    } else {
      if (offset == size) LogBytes(LOG_WARNING, "Truncated Packet", packet, offset);
      LogBytes(LOG_WARNING, "Discarded Byte", &byte, 1);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", packet, offset);
      return length;
    }
  }
}

static int
getHandyTechPacket (HandyTechResponsePacket *packet) {
  return readHandyTechPacket(packet->bytes, sizeof(*packet));
}

static int
writeHandyTechPacket (BrailleDisplay *brl, const unsigned char *packet, int length) {
  if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", packet, length);

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
      if (getHandyTechPacket(&response)) {
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

static int
updateHandyTechKeys (BrailleDisplay *brl, int *keyPressed) {
  HandyTechResponsePacket packet;
  int size;

  while ((size = getHandyTechPacket(&packet))) {
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
      unsigned char key = code & ~HT_RSP_RELEASE;
      int press = (code & HT_RSP_RELEASE) == 0;

      if (HT_IS_ROUTING_KEY(key)) {
        if (!setGroupedKey(pressedKeys.routingKeys, (key - HT_RSP_KEY_CR1), press)) continue;
        if (press) *keyPressed = 1;
      } else {
        uint64_t bit;
        switch (key) {
#define KEY(name) case HT_RSP_KEY_##name: bit = BAUM_KEY_##name; break
          KEY(TL1);
          KEY(TL2);
          KEY(TL3);
          KEY(TR1);
          KEY(TR2);
          KEY(TR3);
#undef KEY

          default:
            LogBytes(LOG_WARNING, "Unexpected Packet", packet.bytes, size);
            continue;
        }
        if (!updateFunctionKeys((press? bit: 0), bit, 0, keyPressed)) continue;
      }
      return 1;
    }
  }

  return 0;
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
  readHandyTechPacket, writeHandyTechPacket,
  probeHandyTechDisplay, updateHandyTechKeys, writeHandyTechCells
};

/* PowerBraille Protocol */

#define PB_BUTTONS0_MARKER 0X60
#define PB1_BUTTONS0_TR3   0X08
#define PB1_BUTTONS0_TR2   0X04
#define PB1_BUTTONS0_TR1   0X02
#define PB1_BUTTONS0_TL2   0X01
#define PB2_BUTTONS0_TL3   0X08
#define PB2_BUTTONS0_TR2   0X04
#define PB2_BUTTONS0_TL1   0X02
#define PB2_BUTTONS0_TL2   0X01

#define PB_BUTTONS1_MARKER 0XE0
#define PB1_BUTTONS1_TL3   0X08
#define PB1_BUTTONS1_TL1   0X02
#define PB2_BUTTONS1_TR3   0X04
#define PB2_BUTTONS1_TR1   0X02

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
readPowerBraillePacket (unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!readByte(&byte, offset>0)) {
      if (offset > 0) LogBytes(LOG_WARNING, "Partial Packet", packet, offset);
      return 0;
    }
  haveByte:

    if (offset == 0) {
      if (!byte) {
        length = 2;
      } else if ((byte & PB_BUTTONS0_MARKER) == PB_BUTTONS0_MARKER) {
        length = 2;
      } else {
        LogBytes(LOG_WARNING, "Ignored Byte", &byte, 1);
        continue;
      }
    } else if (packet[0]) {
      if ((byte & PB_BUTTONS1_MARKER) != PB_BUTTONS1_MARKER) {
        LogBytes(LOG_WARNING, "Short Packet", packet, offset);
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
            LogBytes(LOG_WARNING, "Unknown Packet", &byte, 1);
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
      if (offset == size) LogBytes(LOG_WARNING, "Truncated Packet", packet, offset);
      LogBytes(LOG_WARNING, "Discarded Byte", &byte, 1);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", packet, offset);
      return length;
    }
  }
}

static int
getPowerBraillePacket (PowerBrailleResponsePacket *packet) {
  return readPowerBraillePacket(packet->bytes, sizeof(*packet));
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
    if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", buffer, count);

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
      if (getPowerBraillePacket(&response)) {
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

static int
updatePowerBrailleKeys (BrailleDisplay *brl, int *keyPressed) {
  PowerBrailleResponsePacket packet;
  int size;

  while ((size = getPowerBraillePacket(&packet))) {
    if (!packet.data.zero) {
      switch (packet.data.code) {
        case PB_RSP_IDENTITY:
          changeCellCount(brl, packet.data.values.identity.cells);
          continue;

        case PB_RSP_SENSORS:
          if (updateKeyGroup(pressedKeys.routingKeys, packet.data.values.sensors.horizontal, textCount, keyPressed)) return 1;
          continue;

        default:
          LogBytes(LOG_WARNING, "Unexpected Packet", packet.bytes, size);
          continue;
      }
    } else {
      uint64_t keys = 0;
      if (packet.buttons[0] & PB2_BUTTONS0_TL1) keys |= BAUM_KEY_TL1;
      if (packet.buttons[0] & PB2_BUTTONS0_TL2) keys |= BAUM_KEY_TL2;
      if (packet.buttons[0] & PB2_BUTTONS0_TL3) keys |= BAUM_KEY_TL3;
      if (packet.buttons[1] & PB2_BUTTONS1_TR1) keys |= BAUM_KEY_TR1;
      if (packet.buttons[0] & PB2_BUTTONS0_TR2) keys |= BAUM_KEY_TR2;
      if (packet.buttons[1] & PB2_BUTTONS1_TR3) keys |= BAUM_KEY_TR3;

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
      if (updateFunctionKeys(keys, 0XFF, 0, keyPressed)) {
        if (!*keyPressed) continue;
        *keyPressed = 0;
      }
      activeKeys = pressedKeys;
      return 1;
    }
  }

  return 0;
}

static int
writePowerBrailleCells (BrailleDisplay *brl) {
  unsigned char packet[6 + (textCount * 2)];
  unsigned char *byte = packet;

  *byte++ = PB_REQ_WRITE;
  *byte++ = 0; /* cursor mode: disabled */
  *byte++ = 0; /* cursor position: nowhere */
  *byte++ = 1; /* cursor type: command */
  *byte++ = textCount * 2; /* attribute-data pairs */
  *byte++ = 0; /* start */

  {
    int i;
    for (i=0; i<textCount; ++i) {
      *byte++ = 0; /* attributes */
      *byte++ = externalCells[i]; /* data */
    }
  }

  return writePowerBraillePacket(brl, packet, byte-packet);
}

static const ProtocolOperations powerBrailleOperations = {
  "PowerBraille",
  9600, SERIAL_PARITY_NONE,
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
static const UsbSerialOperations *usbSerial = NULL;

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
    { /* VarioPro 40 (40 cells) */
      .vendor=0X0904, .product=0X2012,
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
    usbSerial = usbGetSerialOperations(usbChannel->device);
    usbBeginInput(usbChannel->device,
                  usbChannel->definition.inputEndpoint,
                  8);
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
    usbSerial = NULL;
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

  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
  }
  
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

        {
          int bits = 10;
          if (protocol->serialParity != SERIAL_PARITY_NONE) ++bits;
          charactersPerSecond = protocol->serialBaud / bits;
        }

        if (io->configurePort()) {
          if (!flushInput()) goto failed;

          memset(&pressedKeys, 0, sizeof(pressedKeys));
          switchSettings = 0;

          if (protocol->probeDisplay(brl)) {
            logCellCount();

            clearCells(0, cellCount);
            if (!updateCells(brl)) goto failed;

            brl->textColumns = textCount;
            brl->textRows = 1;
            brl->helpPage = useVarioKeys? 1: 0;

            activeKeys = pressedKeys;
            pendingCommand = EOF;
            currentModifiers = 0;
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
  int count = protocol->readPacket(buffer, size);
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
  int count = textCount;

  while (count > 0) {
    if (brl->buffer[count-1] != internalCells[count-1]) break;
    --count;
  }

  while (start < count) {
    if (brl->buffer[start] != internalCells[start]) break;
    ++start;
  }
  count -= start;

  memcpy(&internalCells[start], &brl->buffer[start], count);
  translateCells(start, count);
  return updateCells(brl);
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  if (memcmp(&internalCells[textCount], status, statusCount) != 0) {
    memcpy(&internalCells[textCount], status, statusCount);
    translateCells(textCount, statusCount);
  }
  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  uint64_t keys;
  int command;
  int keyPressed;
  int newModifiers = 0;

  unsigned char routingKeys[textCount];
  int routingKeyCount;
  signed char horizontalSensor;
  signed char leftVerticalSensor;
  signed char rightVerticalSensor;

  if (pendingCommand != EOF) {
    command = pendingCommand;
    pendingCommand = EOF;
    return command;
  }

  keyPressed = 0;
  if (!protocol->updateKeys(brl, &keyPressed)) {
    if (errno == EAGAIN) return EOF;
    return BRL_CMD_RESTARTBRL;
  }

  if (keyPressed) activeKeys = pressedKeys;
  keys = activeKeys.functionKeys;
  command = BRL_CMD_NOOP;

  routingKeyCount = getKeyNumbers(activeKeys.routingKeys, textCount, routingKeys);
  horizontalSensor = getSensorNumber(activeKeys.horizontalSensors, textCount);
  leftVerticalSensor = getSensorNumber(activeKeys.leftVerticalSensors, VERTICAL_SENSOR_COUNT);
  rightVerticalSensor = getSensorNumber(activeKeys.rightVerticalSensors, VERTICAL_SENSOR_COUNT);

  if (!(switchSettings & BAUM_SWT_DisableSensors)) {
    if (baumDeviceType == BAUM_TYPE_Inka) {
      if (horizontalSensor >= 0) {
        routingKeys[routingKeyCount++] = horizontalSensor;
        horizontalSensor = -1;
      }
    }

    if (horizontalSensor >= 0) keys |= BAUM_KEY_HRZ;
    if (leftVerticalSensor >= 0) keys |= BAUM_KEY_VTL;
    if (rightVerticalSensor >= 0) keys |= BAUM_KEY_VTR;
  }

#define KEY(key,cmd) case (key): command = (cmd); break
#define KEYv(key,baumCmd,varioCmd) KEY((key), (useVarioKeys? (varioCmd): (baumCmd)))
  if (currentModifiers & MOD_INPUT) {
#define DOT1 BAUM_KEY_TL1
#define DOT2 BAUM_KEY_TL2
#define DOT3 BAUM_KEY_TL3
#define DOT4 BAUM_KEY_TR1
#define DOT5 BAUM_KEY_TR2
#define DOT6 BAUM_KEY_TR3

    newModifiers = currentModifiers & (MOD_INPUT | MOD_INPUT_ONCE |
                                       MOD_DOT7_LOCK | MOD_DOT8_LOCK |
                                       MOD_UPPER_LOCK);
    if ((currentModifiers & MOD_INPUT_ONCE) && (keys || routingKeyCount))
      newModifiers &= ~MOD_INPUT;

    if ((routingKeyCount == 0) && keys) {
      if (currentModifiers & MOD_CHORD) {
      doChord:
        switch (keys) {
          KEY(DOT1, BRL_BLK_PASSKEY+BRL_KEY_BACKSPACE);
          KEY(DOT4|DOT6, BRL_BLK_PASSKEY+BRL_KEY_ENTER);

          KEY(DOT4, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP);
          KEY(DOT6, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN);

          KEY(DOT2, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT);
          KEY(DOT5, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT);

          KEY(DOT4|DOT5, BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP);
          KEY(DOT5|DOT6, BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN);

          KEY(DOT1|DOT2, BRL_BLK_PASSKEY+BRL_KEY_HOME);
          KEY(DOT2|DOT3, BRL_BLK_PASSKEY+BRL_KEY_END);

          KEY(DOT1|DOT4|DOT5, BRL_BLK_PASSKEY+BRL_KEY_DELETE);
          KEY(DOT1|DOT5, BRL_BLK_PASSKEY+BRL_KEY_ESCAPE);
          KEY(DOT2|DOT4, BRL_BLK_PASSKEY+BRL_KEY_INSERT);
          KEY(DOT2|DOT3|DOT4|DOT5, BRL_BLK_PASSKEY+BRL_KEY_TAB);
        }

        if (command != BRL_CMD_NOOP) {
          if (currentModifiers & (MOD_UPPER | MOD_UPPER_LOCK)) command |= BRL_FLG_CHAR_SHIFT;
          if (currentModifiers & MOD_META) command |= BRL_FLG_CHAR_META;
          if (currentModifiers & MOD_CNTRL) command |= BRL_FLG_CHAR_CONTROL;
        }
      } else {
      doCharacter:
        command = BRL_BLK_PASSDOTS;

        if (keys & DOT1) command |= BRL_DOT1;
        if (keys & DOT2) command |= BRL_DOT2;
        if (keys & DOT3) command |= BRL_DOT3;
        if (keys & DOT4) command |= BRL_DOT4;
        if (keys & DOT5) command |= BRL_DOT5;
        if (keys & DOT6) command |= BRL_DOT6;

        if (currentModifiers & (MOD_DOT7 | MOD_DOT7_LOCK)) command |= BRL_DOT7;
        if (currentModifiers & (MOD_DOT8 | MOD_DOT8_LOCK)) command |= BRL_DOT8;

        if (currentModifiers & (MOD_UPPER | MOD_UPPER_LOCK)) command |= BRL_FLG_CHAR_UPPER;
        if (currentModifiers & MOD_META) command |= BRL_FLG_CHAR_META;
        if (currentModifiers & MOD_CNTRL) command |= BRL_FLG_CHAR_CONTROL;
      }
    } else if (routingKeyCount == 1) {
      unsigned char key1 = routingKeys[0];

      if ((key1 == 0) || (key1 == textCount-1)) {
        if (keys) goto doChord;
        newModifiers |= MOD_CHORD;
      } else if (key1 == 1) {
        if (keys) {
          currentModifiers |= MOD_DOT7;
          goto doCharacter;
        }

        if (currentModifiers & MOD_DOT7_LOCK)
          newModifiers &= ~MOD_DOT7_LOCK;
        else if (currentModifiers & MOD_DOT7)
          newModifiers |= MOD_DOT7_LOCK | MOD_DOT7;
        else 
          newModifiers |= MOD_DOT7;
      } else if (key1 == textCount-2) {
        if (keys) {
          currentModifiers |= MOD_DOT8;
          goto doCharacter;
        }

        if (currentModifiers & MOD_DOT8_LOCK)
          newModifiers &= ~MOD_DOT8_LOCK;
        else if (currentModifiers & MOD_DOT8)
          newModifiers |= MOD_DOT8_LOCK | MOD_DOT8;
        else
          newModifiers |= MOD_DOT8;
      } else if (key1 == 2) {
        if (keys) {
          currentModifiers |= MOD_UPPER;
          goto doCharacter;
        }

        if (currentModifiers & MOD_UPPER_LOCK)
          newModifiers &= ~MOD_UPPER_LOCK;
        else if (currentModifiers & MOD_UPPER)
          newModifiers |= MOD_UPPER_LOCK | MOD_UPPER;
        else
          newModifiers |= MOD_UPPER;
      } else if (key1 == textCount-3) {
        if (keys) {
          currentModifiers |= MOD_META;
          goto doCharacter;
        }

        newModifiers |= MOD_META;
      }
    } else if (routingKeyCount == 2) {
      unsigned char key1 = routingKeys[0];
      unsigned char key2 = routingKeys[1];

      if (keys == 0) {
        if ((key1 == textCount-2) && (key2 == textCount-1))
          command = BRL_BLK_PASSDOTS; /* space */
        else if ((key1 == 0) && (key2 == textCount-1))
          newModifiers &= ~(MOD_CHORD |
                            MOD_DOT7 | MOD_DOT7_LOCK |
                            MOD_DOT8 | MOD_DOT8_LOCK |
                            MOD_UPPER | MOD_UPPER_LOCK |
                            MOD_META | MOD_CNTRL);
        else if ((key1 == 1) && (key2 == textCount-2))
          newModifiers |= MOD_DOT7 | MOD_DOT8;
        else if ((key1 == 2) && (key2 == textCount-3))
          newModifiers |= MOD_CNTRL;
      } else {
        if ((key1 == textCount-2) && (key2 == textCount-1)) {
          switch (keys) {
            case BAUM_KEY_TL1:
              /* already in input mode */
              command = BRL_CMD_NOOP | BRL_FLG_TOGGLE_ON;
              break;

            case BAUM_KEY_TL2:
              newModifiers |= MOD_INPUT_ONCE;
              break;

            case BAUM_KEY_TL3:
              newModifiers = 0;
              command = BRL_CMD_NOOP | BRL_FLG_TOGGLE_OFF;
              break;
          }
        }
      }
    }

    if ((currentModifiers & MOD_INPUT_ONCE) &&
        (newModifiers & (MOD_CHORD | MOD_DOT7 | MOD_DOT8 |
                         MOD_UPPER | MOD_META | MOD_CNTRL)))
      newModifiers |= MOD_INPUT;

#undef DOT1
#undef DOT2
#undef DOT3
#undef DOT4
#undef DOT5
#undef DOT6
  } else if (routingKeyCount == 0) {
    if (keys && (keys == (keys & (BAUM_KEY_DOT1 | BAUM_KEY_DOT2 | BAUM_KEY_DOT3 |
                                  BAUM_KEY_DOT4 | BAUM_KEY_DOT5 | BAUM_KEY_DOT6 |
                                  BAUM_KEY_DOT7 | BAUM_KEY_DOT8 |
                                  BAUM_KEY_F2 | BAUM_KEY_F3)))) {
      command = BRL_BLK_PASSDOTS;
#define DOT(dot,key) if (keys & BAUM_KEY_##key) command |= BRL_DOT##dot
      DOT(1, DOT1);
      DOT(2, DOT2);
      DOT(3, DOT3);
      DOT(4, DOT4);
      DOT(5, DOT5);
      DOT(6, DOT6);
      DOT(7, DOT7);
      DOT(8, DOT8);
      DOT(7, F2);
      DOT(8, F3);
#undef DOT
    } else {
      switch (keys) {
        KEY(BAUM_KEY_TL2, BRL_CMD_FWINLT);
        KEY(BAUM_KEY_TR2, BRL_CMD_FWINRT);

        KEYv(BAUM_KEY_TL1|BAUM_KEY_TL3, BRL_CMD_CHRLT, BRL_CMD_HOME);
        KEYv(BAUM_KEY_TR1|BAUM_KEY_TR3, BRL_CMD_CHRRT, BRL_CMD_CSRTRK);

        KEYv(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TL3, BRL_CMD_LNBEG, BRL_CMD_CHRLT);
        KEYv(BAUM_KEY_TR1|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_LNEND, BRL_CMD_CHRRT);

        KEYv(BAUM_KEY_TR1, BRL_CMD_LNUP, BRL_CMD_PRDIFLN);
        KEYv(BAUM_KEY_TR3, BRL_CMD_LNDN, BRL_CMD_NXDIFLN);

        KEY(BAUM_KEY_TL1|BAUM_KEY_TR1, BRL_CMD_TOP);
        KEY(BAUM_KEY_TL3|BAUM_KEY_TR3, BRL_CMD_BOT);

        KEYv(BAUM_KEY_TL2|BAUM_KEY_TR1, BRL_CMD_TOP_LEFT, BRL_CMD_INFO);
        KEYv(BAUM_KEY_TL2|BAUM_KEY_TR3, BRL_CMD_BOT_LEFT, BRL_CMD_NOOP);

        KEYv(BAUM_KEY_TR2|BAUM_KEY_TR1, BRL_CMD_PRDIFLN, BRL_CMD_ATTRUP);
        KEYv(BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_NXDIFLN, BRL_CMD_ATTRDN);

        KEYv(BAUM_KEY_TL2|BAUM_KEY_TL1, BRL_CMD_ATTRUP, BRL_CMD_TOP_LEFT);
        KEYv(BAUM_KEY_TL2|BAUM_KEY_TL3, BRL_CMD_ATTRDN, BRL_CMD_BOT_LEFT);

        KEY(BAUM_KEY_TL2|BAUM_KEY_TR2|BAUM_KEY_TL1|BAUM_KEY_TR1, BRL_CMD_PRPROMPT);
        KEY(BAUM_KEY_TL2|BAUM_KEY_TR2|BAUM_KEY_TL3|BAUM_KEY_TR3, BRL_CMD_NXPROMPT);

        KEY(BAUM_KEY_TL2|BAUM_KEY_TR2|BAUM_KEY_TR1, BRL_CMD_PRPGRPH);
        KEY(BAUM_KEY_TL2|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_NXPGRPH);

        KEY(BAUM_KEY_TL2|BAUM_KEY_TR1|BAUM_KEY_TR3|BAUM_KEY_TL1, BRL_CMD_PRSEARCH);
        KEY(BAUM_KEY_TL2|BAUM_KEY_TR1|BAUM_KEY_TR3|BAUM_KEY_TL3, BRL_CMD_NXSEARCH);

        KEYv(BAUM_KEY_TL1, BRL_CMD_CSRTRK|BRL_FLG_TOGGLE_ON, BRL_CMD_LNUP);
        KEYv(BAUM_KEY_TL3, BRL_CMD_CSRTRK|BRL_FLG_TOGGLE_OFF, BRL_CMD_LNDN);

	KEY(BAUM_KEY_TL1|BAUM_KEY_TR3, BRL_CMD_BACK);

        KEYv(BAUM_KEY_TL2|BAUM_KEY_TR1|BAUM_KEY_TR3, BRL_CMD_HOME, BRL_CMD_LNBEG);
        KEYv(BAUM_KEY_TL1|BAUM_KEY_TL3|BAUM_KEY_TR2, BRL_CMD_SPKHOME, BRL_CMD_LNEND);

        KEY(BAUM_KEY_TL1|BAUM_KEY_TL3|BAUM_KEY_TR1|BAUM_KEY_TR3, BRL_CMD_CSRJMP_VERT);
        KEYv(BAUM_KEY_TL2|BAUM_KEY_TR2, BRL_CMD_INFO, BRL_CMD_SKPBLNKWINS);

        KEY(BAUM_KEY_TL1|BAUM_KEY_TR1|BAUM_KEY_TR2, BRL_CMD_DISPMD);
        KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TR1, BRL_CMD_FREEZE);
        KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TR2, BRL_CMD_HELP);
        KEY(BAUM_KEY_TL1|BAUM_KEY_TL3|BAUM_KEY_TR1, BRL_CMD_PREFMENU);
        KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR1, BRL_CMD_PASTE);
        KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR2, BRL_CMD_PREFLOAD);
        KEY(BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR1, BRL_CMD_RESTARTSPEECH);
        KEY(BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR1|BAUM_KEY_TR2, BRL_CMD_ATTRVIS);
        KEY(BAUM_KEY_TL1|BAUM_KEY_TL3|BAUM_KEY_TR3, BRL_CMD_BACK);
        KEY(BAUM_KEY_TL2|BAUM_KEY_TR1|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_PREFSAVE);

        KEY(BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR2, BRL_CMD_SIXDOTS|BRL_FLG_TOGGLE_ON);
        KEY(BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR3, BRL_CMD_SIXDOTS|BRL_FLG_TOGGLE_OFF);

        KEY(BAUM_KEY_TL1|BAUM_KEY_TR1|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_LEARN);
        KEY(BAUM_KEY_TL1|BAUM_KEY_TL2|BAUM_KEY_TL3|BAUM_KEY_TR3, BRL_CMD_SWITCHVT_NEXT);
        KEY(BAUM_KEY_TL3|BAUM_KEY_TR1|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_SWITCHVT_PREV);

        KEY(BAUM_KEY_TL3|BAUM_KEY_TR1, BRL_CMD_MUTE);
        KEY(BAUM_KEY_TL3|BAUM_KEY_TR2, BRL_CMD_SAY_LINE);
        KEY(BAUM_KEY_TL3|BAUM_KEY_TR1|BAUM_KEY_TR2, BRL_CMD_SAY_ABOVE);
        KEY(BAUM_KEY_TL3|BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_CMD_SAY_BELOW);
        KEY(BAUM_KEY_TL3|BAUM_KEY_TR1|BAUM_KEY_TR3, BRL_CMD_AUTOSPEAK);

        KEY(BAUM_KEY_B11, BRL_BLK_PASSDOTS);
        KEY(BAUM_KEY_B9, BRL_BLK_PASSDOTS);
        KEY(BAUM_KEY_B0, BRL_BLK_PASSDOTS);

        KEY(BAUM_KEY_PRESS, BRL_BLK_PASSKEY+BRL_KEY_ENTER);
        KEY(BAUM_KEY_LEFT, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT);
        KEY(BAUM_KEY_RIGHT, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT);
        KEY(BAUM_KEY_UP, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP);
        KEY(BAUM_KEY_DOWN, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN);

        {
          int arg;
          int flags;

        case BAUM_KEY_VTL:
          arg = leftVerticalSensor;
          flags = BRL_FLG_LINE_TOLEFT;
          goto doVerticalSensor;

        case BAUM_KEY_VTR:
          arg = rightVerticalSensor;
          flags = 0;
          goto doVerticalSensor;

        doVerticalSensor:
          if (switchSettings & BAUM_SWT_ScaledVertical) {
            flags |= BRL_FLG_LINE_SCALED;
            arg = rescaleInteger(arg, VERTICAL_SENSOR_COUNT-1, BRL_MSK_ARG);
          } else if (arg > 0) {
            --arg;
          }
          command = BRL_BLK_GOTOLINE | arg | flags;
          break;
        }
      }
    }
  } else if (routingKeyCount == 1) {
    unsigned char key = routingKeys[0];
    switch (keys) {
      KEY(0, BRL_BLK_ROUTE+key);

      KEY(BAUM_KEY_TL1, BRL_BLK_CUTBEGIN+key);
      KEY(BAUM_KEY_TL2, BRL_BLK_CUTAPPEND+key);
      KEY(BAUM_KEY_TR1, BRL_BLK_CUTLINE+key);
      KEY(BAUM_KEY_TR2, BRL_BLK_CUTRECT+key);

      KEY(BAUM_KEY_TL3, BRL_BLK_DESCCHAR+key);
      KEY(BAUM_KEY_TR3, BRL_BLK_SETLEFT+key);

      KEY(BAUM_KEY_TL2|BAUM_KEY_TL1, BRL_BLK_PRINDENT+key);
      KEY(BAUM_KEY_TL2|BAUM_KEY_TL3, BRL_BLK_NXINDENT+key);

      KEY(BAUM_KEY_TR2|BAUM_KEY_TR1, BRL_BLK_PRDIFCHAR+key);
      KEY(BAUM_KEY_TR2|BAUM_KEY_TR3, BRL_BLK_NXDIFCHAR+key);

      KEY(BAUM_KEY_TL1|BAUM_KEY_TL3, BRL_BLK_SETMARK+key);
      KEY(BAUM_KEY_TR1|BAUM_KEY_TR3, BRL_BLK_GOTOMARK+key);
    }
  } else if (routingKeyCount == 2) {
    if (keys == 0) {
      command = BRL_BLK_CUTBEGIN + routingKeys[0];
      pendingCommand = BRL_BLK_CUTLINE + routingKeys[1];
    } else if (routingKeys[1] == routingKeys[0]+1) {
      switch (keys) {
        KEY(BAUM_KEY_TR1, BRL_BLK_PRINDENT+routingKeys[0]);
        KEY(BAUM_KEY_TR3, BRL_BLK_NXINDENT+routingKeys[0]);

        default:
          if (routingKeys[1] == textCount-1) {
            switch (keys) {
              case BAUM_KEY_TL1:
                newModifiers = MOD_INPUT;
                command = BRL_CMD_NOOP | BRL_FLG_TOGGLE_ON;
                break;

              case BAUM_KEY_TL2:
                newModifiers = MOD_INPUT | MOD_INPUT_ONCE;
                break;

              KEY(BAUM_KEY_TL3, BRL_CMD_NOOP|BRL_FLG_TOGGLE_OFF);
                /* already out of input mode */ 
            }
          }
      }
    }
  }
#undef KEYv
#undef KEY

  if (!keyPressed) {
    memset(&activeKeys, 0, sizeof(activeKeys));
    currentModifiers = newModifiers;
  } else if (pendingCommand != EOF) {
    command = BRL_CMD_NOOP;
    pendingCommand = EOF;
  } else {
    command |= BRL_FLG_REPEAT_DELAY;
  }

  return command;
}
