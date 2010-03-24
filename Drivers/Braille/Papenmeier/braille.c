/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

/* This Driver was written as a project in the
 *   HTL W1, Abteilung Elektrotechnik, Wien - Österreich
 *   (Technical High School, Department for electrical engineering,
 *     Vienna, Austria)  http://www.ee.htlw16.ac.at
 *  by
 *   Tibor Becker
 *   Michael Burger
 *   Herbert Gruber
 *   Heimo Schön
 * Teacher:
 *   August Hörandl <august.hoerandl@gmx.at>
 */
/*
 * Support for all Papenmeier Terminal + config file
 *   Heimo.Schön <heimo.schoen@gmx.at>
 *   August Hörandl <august.hoerandl@gmx.at>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"
#include "bitfield.h"
#include "timing.h"

#define BRL_STATUS_FIELDS sfGeneric
#define BRL_HAVE_STATUS_CELLS
#define BRL_HAVE_FIRMNESS
#include "brl_driver.h"
#include "brldefs-pm.h"
#include "models.h"
#include "braille.h"

static const ModelEntry *model = NULL;
static TranslationTable outputTable;
 
/*--- Input/Output Operations ---*/

typedef struct {
  const int *bauds;
  unsigned char protocol1;
  unsigned char protocol2;
  int (*openPort) (char **parameters, const char *device);
  int (*preparePort) (void);
  void (*closePort) (void);
  void (*flushPort) (BrailleDisplay *brl);
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, size_t *offset, size_t length, int timeout);
  int (*writeBytes) (const unsigned char *buffer, int length);
} InputOutputOperations;

static const InputOutputOperations *io;
static const int *baud;
static int charactersPerSecond;

/*--- Serial Operations ---*/

#include "io_serial.h"
static SerialDevice *serialDevice = NULL;
static const int serialBauds[] = {19200, 38400, 0};

static int
openSerialPort (char **parameters, const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, *baud)) return 1;

    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
  return 0;
}

static int
prepareSerialPort (void) {
  return serialSetFlowControl(serialDevice, SERIAL_FLOW_HARDWARE);
}

static void
closeSerialPort (void) {
  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
}

static void
flushSerialPort (BrailleDisplay *brl) {
  serialDiscardOutput(serialDevice);
  drainBrailleOutput(brl, 100);
  serialDiscardInput(serialDevice);
}

static int
awaitSerialInput (int milliseconds) {
  return serialAwaitInput(serialDevice, milliseconds);
}

static int
readSerialBytes (unsigned char *buffer, size_t *offset, size_t length, int timeout) {
  return serialReadChunk(serialDevice, buffer, offset, length, 0, timeout);
}

static int
writeSerialBytes (const unsigned char *buffer, int length) {
  return serialWriteData(serialDevice, buffer, length);
}

static const InputOutputOperations serialOperations = {
  serialBauds, 1, 1,
  openSerialPort, prepareSerialPort, closeSerialPort, flushSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialBytes
};

/*--- USB Operations ---*/

#include "io_usb.h"
static UsbChannel *usb = NULL;
static const int usbBauds[] = {115200, 57600, 0};

static int
openUsbPort (char **parameters, const char *device) {
  const SerialParameters serial = {
    .baud = *baud,
    .flow = SERIAL_FLOW_NONE,
    .data = 8,
    .stop = 1,
    .parity = SERIAL_PARITY_NONE
  };

  const UsbChannelDefinition definitions[] = {
    { .vendor=0X0403, .product=0Xf208,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .serial = &serial
    }
    ,
    { .vendor=0 }
  };

  if ((usb = usbFindChannel(definitions, (void *)device))) {
    return 1;
  }
  return 0;
}

static int
prepareUsbPort (void) {
  return 1;
}

static void
closeUsbPort (void) {
  if (usb) {
    usbCloseChannel(usb);
    usb = NULL;
  }
}

static void
flushUsbPort (BrailleDisplay *brl) {
}

static int
awaitUsbInput (int milliseconds) {
  return usbAwaitInput(usb->device, usb->definition.inputEndpoint, milliseconds);
}

static int
readUsbBytes (unsigned char *buffer, size_t *offset, size_t length, int timeout) {
  int count = usbReapInput(usb->device, usb->definition.inputEndpoint, buffer+*offset, length, 
                           (*offset? timeout: 0), timeout);
  if (count == -1) return 0;
  *offset += count;
  return 1;
}

static int
writeUsbBytes (const unsigned char *buffer, int length) {
  return usbWriteEndpoint(usb->device, usb->definition.outputEndpoint, buffer, length, 1000);
}

static const InputOutputOperations usbOperations = {
  usbBauds, 0, 3,
  openUsbPort, prepareUsbPort, closeUsbPort, flushUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes
};

/*--- Bluetooth Operations ---*/

#include "io_bluetooth.h"
#include "io_misc.h"

static int bluetoothConnection = -1;
static const int bluetoothBauds[] = {115200, 0};

static int
openBluetoothPort (char **parameters, const char *device) {
  return (bluetoothConnection = btOpenConnection(device, 1, 0)) != -1;
}

static int
prepareBluetoothPort (void) {
  return 1;
}

static void
closeBluetoothPort (void) {
  close(bluetoothConnection);
  bluetoothConnection = -1;
}

static void
flushBluetoothPort (BrailleDisplay *brl) {
}

static int
awaitBluetoothInput (int milliseconds) {
  return awaitInput(bluetoothConnection, milliseconds);
}

static int
readBluetoothBytes (unsigned char *buffer, size_t *offset, size_t length, int timeout) {
  return readChunk(bluetoothConnection, buffer, offset, length, 0, timeout);
}

static int
writeBluetoothBytes (const unsigned char *buffer, int length) {
  int count = writeData(bluetoothConnection, buffer, length);
  if (count != length) {
    if (count == -1) {
      LogError("Papenmeier Bluetooth write");
    } else {
      LogPrint(LOG_WARNING, "Trunccated bluetooth write: %d < %d", count, length);
    }
  }
  return count;
}

static const InputOutputOperations bluetoothOperations = {
  bluetoothBauds, 0, 3,
  openBluetoothPort, prepareBluetoothPort, closeBluetoothPort, flushBluetoothPort,
  awaitBluetoothInput, readBluetoothBytes, writeBluetoothBytes
};

/*--- Protocol Operation Utilities ---*/

typedef struct {
  void (*initializeTerminal) (BrailleDisplay *brl);
  void (*releaseResources) (void);
  int (*readCommand) (BrailleDisplay *brl, BRL_DriverCommandContext context);
  void (*writeText) (BrailleDisplay *brl, int start, int count);
  void (*writeStatus) (BrailleDisplay *brl, int start, int count);
  void (*flushCells) (BrailleDisplay *brl);
  void (*setFirmness) (BrailleDisplay *brl, BrailleFirmness setting);
} ProtocolOperations;

static const ProtocolOperations *protocol;
static unsigned char currentStatus[PMSC];
static unsigned char currentText[BRLCOLSMAX];


static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, int count) {
  logOutputPacket(bytes, count);
  if (io->writeBytes(bytes, count) != -1) {
    brl->writeDelay += (count * 1000 / charactersPerSecond) + 1;
    return 1;
  } else {
    LogError("Write");
    return 0;
  }
}

static int
interpretIdentity (BrailleDisplay *brl, unsigned char id, int major, int minor) {
  int modelIndex;
  LogPrint(LOG_INFO, "Papenmeier ID: %d  Version: %d.%02d", id, major, minor);

  for (modelIndex=0; modelIndex<modelCount; modelIndex++) {
    if (modelTable[modelIndex].modelIdentifier == id) {
      model = &modelTable[modelIndex];
      LogPrint(LOG_INFO, "%s  Size: %d", model->modelName, model->textColumns);

      brl->textColumns = model->textColumns;
      brl->textRows = 1;
      brl->statusRows = (brl->statusColumns = model->statusCount)? 1: 0;
      brl->keyBindings = model->keyTableDefinition->bindings;
      brl->keyNameTables = model->keyTableDefinition->names;

      return 1;
    }
  }

  LogPrint(LOG_WARNING, "Unknown Papenmeier ID: %d", id);
  return 0;
}

/*--- Protocol 1 Operations ---*/

#define cIdSend 'S'
#define cIdIdentify 'I'
#define cIdReceive 'K'
#define PRESSED 1
#define IDENTITY_LENGTH 10

/* offsets within input data structure */
#define RCV_KEYFUNC  0X0000 /* physical and logical function keys */
#define RCV_KEYROUTE 0X0300 /* routing keys */
#define RCV_SENSOR   0X0600 /* sensors or secondary routing keys */

/* offsets within output data structure */
#define XMT_BRLDATA  0X0000 /* data for braille display */
#define XMT_LCDDATA  0X0100 /* data for LCD */
#define XMT_BRLWRITE 0X0200 /* how to write each braille cell:
                             * 0 = convert data according to braille table (default)
                             * 1 = write directly
                             * 2 = mark end of braille display
                             */
#define XMT_BRLCELL  0X0300 /* description of eadch braille cell:
                             * 0 = has cursor routing key
                             * 1 = has cursor routing key and sensor
                             */
#define XMT_ASC2BRL  0X0400 /* ASCII to braille translation table */
#define XMT_LCDUSAGE 0X0500 /* source of LCD data:
                             * 0 = same as braille display
                             * 1 = not same as braille display
                             */
#define XMT_CSRPOSN  0X0501 /* cursor position (0 for no cursor) */
#define XMT_CSRDOTS  0X0502 /* cursor represenation in braille dots */
#define XMT_BRL2ASC  0X0503 /* braille to ASCII translation table */
#define XMT_LENFBSEQ 0X0603 /* length of feedback sequence for speech synthesizer */
#define XMT_LENKPSEQ 0X0604 /* length of keypad sequence */
#define XMT_TIMEK1K2 0X0605 /* key code suppression time for moving from K1 to K2 (left) */
#define XMT_TIMEK3K4 0X0606 /* key code suppression time for moving from K3 to K4 (up) */
#define XMT_TIMEK5K6 0X0607 /* key code suppression time for moving from K5 to K6 (right) */
#define XMT_TIMEK7K8 0X0608 /* key code suppression time for moving from K7 to K8 (down) */
#define XMT_TIMEROUT 0X0609 /* routing time interval */
#define XMT_TIMEOPPO 0X060A /* key code suppression time for opposite movements */

static int rcvStatusFirst;
static int rcvStatusLast;
static int rcvCursorFirst;
static int rcvCursorLast;
static int rcvFrontFirst;
static int rcvFrontLast;
static int rcvBarFirst;
static int rcvBarLast;
static int rcvSwitchFirst;
static int rcvSwitchLast;

static unsigned char xmtStatusOffset;
static unsigned char xmtTextOffset;

static void
resetTerminal1 (BrailleDisplay *brl) {
  static const unsigned char sequence[] = {cSTX, 0X01, cETX};
  LogPrint(LOG_WARNING, "Resetting terminal.");
  io->flushPort(brl);
  writeBytes(brl, sequence, sizeof(sequence));
}

#define RBF_ETX 1
#define RBF_RESET 2
static int
readBytes1 (BrailleDisplay *brl, unsigned char *buffer, size_t offset, size_t count, int flags) {
  if (io->readBytes(buffer, &offset, count, 1000)) {
    if (!(flags & RBF_ETX)) return 1;
    if (*(buffer+offset-1) == cETX) return 1;
    logCorruptPacket(buffer, offset);
  }
  if ((offset > 0) && (flags & RBF_RESET)) resetTerminal1(brl);
  return 0;
}

static int
writePacket1 (BrailleDisplay *brl, int xmtAddress, int count, const unsigned char *data) {
  if (count) {
    unsigned char header[] = {
      cSTX,
      cIdSend,
      0, 0, /* big endian data offset */
      0, 0  /* big endian packet length */
    };
    unsigned char trailer[] = {cETX};
    int size = sizeof(header) + count + sizeof(trailer);
    unsigned char buffer[size];
    int index = 0;

    header[2] = xmtAddress >> 8;
    header[3] = xmtAddress & 0XFF;
    header[4] = size >> 8;
    header[5] = size & 0XFF;
    memcpy(&buffer[index], header, sizeof(header));
    index += sizeof(header);

    if (count) {
      memcpy(&buffer[index], data, count);
      index += count;
    }
    
    memcpy(&buffer[index], trailer, sizeof(trailer));
    index += sizeof(trailer);
    
    if (!writeBytes(brl, buffer, index)) return 0;
  }
  return 1;
}

static int
interpretIdentity1 (BrailleDisplay *brl, const unsigned char *identity) {
  {
    unsigned char id = identity[2];
    unsigned char major = identity[3];
    unsigned char minor = ((identity[4] * 10) + identity[5]);
    if (!interpretIdentity(brl, id, major, minor)) return 0;
  }

  /* routing key codes: 0X300 -> status -> cursor */
  rcvStatusFirst = RCV_KEYROUTE;
  rcvStatusLast  = rcvStatusFirst + 3 * (model->statusCount - 1);
  rcvCursorFirst = rcvStatusLast + 3;
  rcvCursorLast  = rcvCursorFirst + 3 * (model->textColumns - 1);
  LogPrint(LOG_DEBUG, "Routing Keys: status=%03X-%03X cursor=%03X-%03X",
           rcvStatusFirst, rcvStatusLast,
           rcvCursorFirst, rcvCursorLast);

  /* function key codes: 0X000 -> front -> bar -> switches */
  rcvFrontFirst = RCV_KEYFUNC + 3;
  rcvFrontLast  = rcvFrontFirst + 3 * (model->frontKeys - 1);
  rcvBarFirst = rcvFrontLast + 3;
  rcvBarLast  = rcvBarFirst + 3 * ((model->hasBar? 8: 0) - 1);
  rcvSwitchFirst = rcvBarLast + 3;
  rcvSwitchLast  = rcvSwitchFirst + 3 * ((model->hasBar? 8: 0) - 1);
  LogPrint(LOG_DEBUG, "Function Keys: front=%03X-%03X bar=%03X-%03X switches=%03X-%03X",
           rcvFrontFirst, rcvFrontLast,
           rcvBarFirst, rcvBarLast,
           rcvSwitchFirst, rcvSwitchLast);

  /* cell offsets: 0X00 -> status -> text */
  xmtStatusOffset = 0;
  xmtTextOffset = xmtStatusOffset + model->statusCount;
  LogPrint(LOG_DEBUG, "Cell Offsets: status=%02X text=%02X",
           xmtStatusOffset, xmtTextOffset);

  return 1;
}

static int
handleKey1 (BrailleDisplay *brl, int code, int press, int time) {
  int key;

  if (rcvFrontFirst <= code && 
      code <= rcvFrontLast) { /* front key */
    key = (code - rcvFrontFirst) / 3;
    return enqueueKeyEvent(PM_SET_NavigationKeys, PM_KEY_FRONT+key, press);
  }

  if (rcvStatusFirst <= code && 
      code <= rcvStatusLast) { /* status key */
    key = (code - rcvStatusFirst) / 3;
    return enqueueKeyEvent(PM_SET_NavigationKeys, PM_KEY_STATUS+key, press);
  }

  if (rcvBarFirst <= code && 
      code <= rcvBarLast) { /* easy access bar */
    key = (code - rcvBarFirst) / 3;
    return enqueueKeyEvent(PM_SET_NavigationKeys, PM_KEY_BAR+key, press);
  }

  if (rcvSwitchFirst <= code && 
      code <= rcvSwitchLast) { /* easy access bar */
    key = (code - rcvSwitchFirst) / 3;
    return enqueueKeyEvent(PM_SET_NavigationKeys, PM_KEY_SWITCH+key, press);
  }

  if (rcvCursorFirst <= code && 
      code <= rcvCursorLast) { /* Routing Keys */ 
    key = (code - rcvCursorFirst) / 3;
    return enqueueKeyEvent(PM_SET_RoutingKeys1, key, press);
  }

  LogPrint(LOG_WARNING, "unexpected key: %04X", code);
  return 1;
}

static int
disableOutputTranslation1 (BrailleDisplay *brl, unsigned char xmtOffset, int count) {
  unsigned char buffer[count];
  memset(buffer, 1, sizeof(buffer));
  return writePacket1(brl, XMT_BRLWRITE+xmtOffset,
                      sizeof(buffer), buffer);
}

static void
initializeTable1 (BrailleDisplay *brl) {
  disableOutputTranslation1(brl, xmtStatusOffset, model->statusCount);
  disableOutputTranslation1(brl, xmtTextOffset, model->textColumns);
}

static void
writeText1 (BrailleDisplay *brl, int start, int count) {
  writePacket1(brl, XMT_BRLDATA+xmtTextOffset+start, count, currentText+start);
}

static void
writeStatus1 (BrailleDisplay *brl, int start, int count) {
  writePacket1(brl, XMT_BRLDATA+xmtStatusOffset+start, count, currentStatus+start);
}

static void
flushCells1 (BrailleDisplay *brl) {
}

static void
initializeTerminal1 (BrailleDisplay *brl) {
  initializeTable1(brl);
  drainBrailleOutput(brl, 0);

  writeStatus1(brl, 0, model->statusCount);
  drainBrailleOutput(brl, 0);

  writeText1(brl, 0, model->textColumns);
  drainBrailleOutput(brl, 0);
}

static void
restartTerminal1 (BrailleDisplay *brl) {
  initializeTerminal1(brl);
}

static int
readCommand1 (BrailleDisplay *brl, BRL_DriverCommandContext context) {
#define READ(offset,count,flags) { if (!readBytes1(brl, buf, offset, count, RBF_RESET|(flags))) return EOF; }
  while (1) {
    unsigned char buf[0X100];

    while (1) {
      READ(0, 1, 0);
      if (buf[0] == cSTX) break;
      logIgnoredByte(buf[0]);
    }

    READ(1, 1, 0);
    switch (buf[1]) {
      default: {
        int i;

        logUnknownPacket(buf[1]);
        for (i=2; i<sizeof(buf); i++) {
          READ(i, 1, 0);
          logDiscardedByte(buf[i]);
        }
        break;
      }

      case cIdIdentify: {
        const int length = 10;
        READ(2, length-2, RBF_ETX);
        logInputPacket(buf, length);
        if (interpretIdentity1(brl, buf)) brl->resizeRequired = 1;

        approximateDelay(200);
        restartTerminal1(brl);
        break;
      }

      case cIdReceive: {
        int length;

        READ(2, 4, 0);
        length = (buf[4] << 8) | buf[5];	/* packet size */

        if (length != 10) {
          LogPrint(LOG_WARNING, "Unexpected input packet length: %d", length);
          resetTerminal1(brl);
          return EOF;
        }

        READ(6, length-6, RBF_ETX);			/* Data */
        logInputPacket(buf, length);

        handleKey1(brl, ((buf[2] << 8) | buf[3]),
                   (buf[6] == PRESSED),
                   ((buf[7] << 8) | buf[8]));
        continue;
      }

      {
        const char *message;

      case 0X03:
        message = "missing identification byte";
        goto logError;

      case 0X04:
        message = "data too long";
        goto logError;

      case 0X05:
        message = "data starts beyond end of structure";
        goto logError;

      case 0X06:
        message = "data extends beyond end of structure";
        goto logError;

      case 0X07:
        message = "data framing error";
        goto logError;

      logError:
        READ(2, 1, RBF_ETX);
        logInputPacket(buf, 3);
        LogPrint(LOG_WARNING, "Output packet error: %02X: %s", buf[1], message);
        restartTerminal1(brl);
        break;
      }
    }
  }
#undef READ
}

static void
releaseResources1 (void) {
}

static const ProtocolOperations protocolOperations1 = {
  initializeTerminal1, releaseResources1,
  readCommand1,
  writeText1, writeStatus1, flushCells1,
  NULL
};

static int
identifyTerminal1 (BrailleDisplay *brl) {
  static const unsigned char badPacket[] = { 
    cSTX,
    cIdSend,
    0, 0,			/* position */
    0, 0,			/* wrong number of bytes */
    cETX
  };

  io->flushPort(brl);
  if (writeBytes(brl, badPacket, sizeof(badPacket))) {
    if (io->awaitInput(1000)) {
      unsigned char identity[IDENTITY_LENGTH];			/* answer has 10 chars */
      if (readBytes1(brl, identity, 0, 1, 0)) {
        if (identity[0] == cSTX) {
          if (readBytes1(brl, identity, 1, sizeof(identity)-1, RBF_ETX)) {
            if (identity[1] == cIdIdentify) {
              if (interpretIdentity1(brl, identity)) {
                protocol = &protocolOperations1;

                {
                  static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
                  makeOutputTable(dots, outputTable);
                }

                return 1;
              }
            } else {
              LogPrint(LOG_WARNING, "Not an identification packet: %02X", identity[1]);
            }
          } else {
            LogPrint(LOG_WARNING, "Malformed identification packet.");
          }
        }
      }
    }
  }
  return 0;
}

/*--- Protocol 2 Operations ---*/

typedef struct {
  unsigned char type;
  unsigned char length;
  union {
    unsigned char bytes[0XFF];
  } data;
} Packet2;

#define PM2_MAKE_BYTE(high, low) ((LOW_NIBBLE((high)) << 4) | LOW_NIBBLE((low)))
#define PM2_MAKE_INTEGER2(tens,ones) ((LOW_NIBBLE((tens)) * 10) + LOW_NIBBLE((ones)))

typedef struct {
  unsigned char set;
  unsigned char key;
} InputMapping2;
static InputMapping2 *inputMap2 = NULL;
static int inputBytes2;
static int inputBits2;
static int inputKeySize2;

static unsigned char *inputState2 = NULL;
static int refreshRequired2;

static int
readPacket2 (BrailleDisplay *brl, Packet2 *packet) {
  unsigned char buffer[0X203];
  size_t offset = 0;

  volatile size_t size;
  volatile int identity;

  while (1) {
    if (!io->readBytes(buffer, &offset, 1, 1000)) {
      if (offset > 0) logPartialPacket(buffer, offset);
      return 0;
    }

    {
      unsigned char byte = buffer[offset-1];
      unsigned char type = HIGH_NIBBLE(byte);
      unsigned char value = LOW_NIBBLE(byte);

      switch (byte) {
        case cSTX:
          if (offset > 1) {
            logDiscardedBytes(buffer, offset-1);
            offset = 1;
          }
          continue;

        case cETX:
          if ((offset >= 5) && (offset == size)) {
            logInputPacket(buffer, offset);
            return 1;
          }

          logShortPacket(buffer, offset);
          offset = 0;
          continue;

        default:
          switch (offset) {
            case 1:
              logIgnoredByte(buffer[0]);
              offset = 0;
              continue;
    
            case 2:
              if (type != 0X40) break;
              packet->type = value;
              identity = value == 0X0A;
              continue;
    
            case 3:
              if (type != 0X50) break;
              packet->length = value << 4;
              continue;
    
            case 4:
              if (type != 0X50) break;
              packet->length |= value;

              size = packet->length;
              if (!identity) size *= 2;
              size += 5;
              continue;
    
            default:
              if (type != 0X30) break;

              if (offset == size) {
                logCorruptPacket(buffer, offset);
                offset = 0;
                continue;
              }

              {
                int index = offset - 5;
                if (identity) {
                  packet->data.bytes[index] = byte;
                } else {
                  int high = !(index % 2);
                  index /= 2;
                  if (high) {
                    packet->data.bytes[index] = value << 4;
                  } else {
                    packet->data.bytes[index] |= value;
                  }
                }
              }
              continue;
          }
          break;
      }
    }

    logCorruptPacket(buffer, offset);
    offset = 0;
  }
}

static int
writePacket2 (BrailleDisplay *brl, unsigned char command, unsigned char count, const unsigned char *data) {
  unsigned char buffer[(count * 2) + 5];
  unsigned char *byte = buffer;

  *byte++ = cSTX;
  *byte++ = 0X40 | command;
  *byte++ = 0X50 | (count >> 4);
  *byte++ = 0X50 | (count & 0XF);

  while (count-- > 0) {
    *byte++ = 0X30 | (*data >> 4);
    *byte++ = 0X30 | (*data & 0XF);
    data++;
  }

  *byte++ = cETX;
  return writeBytes(brl, buffer, byte-buffer);
}

static int
interpretIdentity2 (BrailleDisplay *brl, const unsigned char *identity) {
  {
    unsigned char id = PM2_MAKE_BYTE(identity[0], identity[1]);
    unsigned char major = LOW_NIBBLE(identity[2]);
    unsigned char minor = PM2_MAKE_INTEGER2(identity[3], identity[4]);
    if (!interpretIdentity(brl, id, major, minor)) return 0;
  }

  return 1;
}

static void
writeCells2 (BrailleDisplay *brl, int start, int count) {
  refreshRequired2 = 1;
}

static void
flushCells2 (BrailleDisplay *brl) {
  if (refreshRequired2) {
    unsigned char buffer[0XFF];
    unsigned int size = 0;

    /* The status cells. */
    memcpy(&buffer[size], currentStatus, model->statusCount);
    size += model->statusCount;

    /* Two dummy cells for each key on the left side. */
    if (model->protocolRevision < 2) {
      int count = model->leftKeys;
      while (count-- > 0) {
        buffer[size++] = 0;
        buffer[size++] = 0;
      }
    }

    /* The text cells. */
    memcpy(&buffer[size], currentText, model->textColumns);
    size += model->textColumns;

    /* Two dummy cells for each key on the right side. */
    if (model->protocolRevision < 2) {
      int count = model->rightKeys;
      while (count-- > 0) {
        buffer[size++] = 0;
        buffer[size++] = 0;
      }
    }

    writePacket2(brl, 3, size, buffer);
    refreshRequired2 = 0;
  }
}

static void
initializeTerminal2 (BrailleDisplay *brl) {
  memset(inputState2, 0, inputBytes2);
  refreshRequired2 = 1;

  /* Don't send the init packet by default as that was done at the factory
   * and shouldn't need to be done again. We'll keep the code, though,
   * just in case it's ever needed. Perhaps there should be a driver
   * parameter to control it.
   */
  if (0) {
    unsigned char data[13];
    unsigned char size = 0;

    data[size++] = model->modelIdentifier; /* device identification code */

    /* serial baud (bcd-encoded, six digits, one per nibble) */
    /* set to zero for default (57,600) */
    data[size++] = 0;
    data[size++] = 0;
    data[size++] = 0;

    data[size++] = model->statusCount; /* number of vertical braille cells */
    data[size++] = model->leftKeys; /* number of left keys and switches */
    data[size++] = model->textColumns; /* number of horizontal braille cells */
    data[size++] = model->rightKeys; /* number of right keys and switches */

    data[size++] = 2; /* number of routing keys per braille cell */
    data[size++] = 0; /* size of LCD */

    data[size++] = 1; /* keys and switches mixed into braille data stream */
    data[size++] = 0; /* easy access bar mixed into braille data stream */
    data[size++] = 1; /* routing keys mixed into braille data stream */

    LogBytes(LOG_DEBUG, "Init Packet", data, size);
    writePacket2(brl, 1, size, data);
  }
}

static int 
readCommand2 (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  Packet2 packet;

  while (readPacket2(brl, &packet)) {
    switch (packet.type) {
      default:
        LogPrint(LOG_DEBUG, "Packet ignored: %02X", packet.type);
        break;

      case 0X0B: {
        int bytes = MIN(packet.length, inputBytes2);
        int byte;

        /* Find out which keys have been released. */
        for (byte=0; byte<bytes; byte+=1) {
          unsigned char old = inputState2[byte];
          unsigned char new = packet.data.bytes[byte];

          if (new != old) {
            InputMapping2 *mapping = &inputMap2[byte * 8];
            unsigned char bit = 0X01;

            while (bit) {
              if (!(new & bit) && (old & bit)) {
                enqueueKeyEvent(mapping->set, mapping->key, 0);
                if ((inputState2[byte] &= ~bit) == new) break;
              }

              mapping += 1;
              bit <<= 1;
            }
          }
        }

        /* Find out which keys have been pressed. */
        for (byte=0; byte<bytes; byte+=1) {
          unsigned char old = inputState2[byte];
          unsigned char new = packet.data.bytes[byte];

          if (new != old) {
            InputMapping2 *mapping = &inputMap2[byte * 8];
            unsigned char bit = 0X01;

            while (bit) {
              if ((new & bit) && !(old & bit)) {
                enqueueKeyEvent(mapping->set, mapping->key, 1);
                if ((inputState2[byte] |= bit) == new) break;
              }

              mapping += 1;
              bit <<= 1;
            }
          }
        }

        continue;
      }

      case 0X0C: {
        unsigned char modifierKeys = packet.data.bytes[0];
        unsigned char dotKeys = packet.data.bytes[1];
        uint16_t allKeys = (modifierKeys << 8) | dotKeys;
        PM_NavigationKey pressedKeys[0X10];
        unsigned char pressedCount = 0;
        const PM_KeySet set = PM_SET_NavigationKeys;
        unsigned char keyOffset;

#define BIT(key) (1 << ((key) - PM_KEY_KEYBOARD))
        if (allKeys & (BIT(PM_KEY_LeftSpace) | BIT(PM_KEY_RightSpace))) allKeys &= ~BIT(PM_KEY_Space);
#undef BIT

        for (keyOffset=0; keyOffset<13; keyOffset+=1) {
          if (allKeys & (1 << keyOffset)) {
            PM_NavigationKey key = PM_KEY_KEYBOARD + keyOffset;
            enqueueKeyEvent(set, key, 1);
            pressedKeys[pressedCount++] = key;
          }
        }

        while (pressedCount) enqueueKeyEvent(set, pressedKeys[--pressedCount], 0);
        continue;
      }
    }
  }

  if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;
  return EOF;
}

static void
releaseResources2 (void) {
  if (inputState2) {
    free(inputState2);
    inputState2 = NULL;
  }

  if (inputMap2) {
    free(inputMap2);
    inputMap2 = NULL;
  }
}

static void
setFirmness2 (BrailleDisplay *brl, BrailleFirmness setting) {
  unsigned char data[] = {(setting * 98 / BRL_FIRMNESS_MAXIMUM) + 2, 0X99};
  writePacket2(brl, 6, sizeof(data), data);
}

static const ProtocolOperations protocolOperations2 = {
  initializeTerminal2, releaseResources2,
  readCommand2,
  writeCells2, writeCells2, flushCells2,
  setFirmness2
};

typedef struct {
  unsigned char byte;
  unsigned char bit;
  unsigned char size;
} InputModule2;

static void
addInputMapping2 (const InputModule2 *module, unsigned char bit, unsigned char set, unsigned char key) {
  if (model->protocolRevision < 2) {
    bit += module->bit;
  } else {
    bit += 8 - module->bit - module->size;
  }

  {
    InputMapping2 *mapping = &inputMap2[(module->byte * 8) + bit];
    mapping->set = set;
    mapping->key = key;
  }
}

static int
nextInputModule2 (InputModule2 *module, unsigned char size) {
  if (!module->bit) {
    if (!module->byte) return 0;
    module->byte -= 1;
    module->bit = 8;
  }
  module->bit -= module->size = size;
  return 1;
}

static void
mapInputKey2 (int count, InputModule2 *module, int rear, int front) {
  while (count--) {
    nextInputModule2(module, inputKeySize2);
    addInputMapping2(module, 0, PM_SET_NavigationKeys, rear);
    addInputMapping2(module, 1, PM_SET_NavigationKeys, front);
  }
}

static void
mapInputModules2 (void) {
  InputModule2 module;
  module.byte = inputBytes2;
  module.bit = 0;

  {
    int i;
    for (i=0; i<inputBits2; ++i) {
      InputMapping2 *mapping = &inputMap2[i];
      mapping->set = 0;
      mapping->key = 0;
    }
  }

  mapInputKey2(model->rightKeys, &module, PM_KEY_RightKeyRear, PM_KEY_RightKeyFront);

  {
    unsigned char column = model->textColumns;
    while (column) {
      nextInputModule2(&module, 1);
      addInputMapping2(&module, 0, PM_SET_RoutingKeys2, --column);

      nextInputModule2(&module, 1);
      addInputMapping2(&module, 0, PM_SET_RoutingKeys1, column);
    }
  }

  mapInputKey2(model->leftKeys, &module, PM_KEY_LeftKeyRear, PM_KEY_LeftKeyFront);

  {
    unsigned char cell = model->statusCount;
    while (cell) {
      nextInputModule2(&module, 1);
      addInputMapping2(&module, 0, PM_SET_StatusKeys2, cell-1);

      nextInputModule2(&module, 1);
      addInputMapping2(&module, 0, PM_SET_NavigationKeys, PM_KEY_STATUS+cell--);
    }
  }

  module.bit = 0;
  nextInputModule2(&module, 8);
  addInputMapping2(&module, 0, PM_SET_NavigationKeys, PM_KEY_BarUp2);
  addInputMapping2(&module, 1, PM_SET_NavigationKeys, PM_KEY_BarUp1);
  addInputMapping2(&module, 2, PM_SET_NavigationKeys, PM_KEY_BarDown1);
  addInputMapping2(&module, 3, PM_SET_NavigationKeys, PM_KEY_BarDown2);
  addInputMapping2(&module, 4, PM_SET_NavigationKeys, PM_KEY_BarRight1);
  addInputMapping2(&module, 5, PM_SET_NavigationKeys, PM_KEY_BarLeft1);
  addInputMapping2(&module, 6, PM_SET_NavigationKeys, PM_KEY_BarRight2);
  addInputMapping2(&module, 7, PM_SET_NavigationKeys, PM_KEY_BarLeft2);
}

static int
identifyTerminal2 (BrailleDisplay *brl) {
  int tries = 0;
  io->flushPort(brl);
  while (writePacket2(brl, 2, 0, NULL)) {
    while (io->awaitInput(100)) {
      Packet2 packet;			/* answer has 10 chars */
      if (readPacket2(brl, &packet)) {
        if (packet.type == 0X0A) {
          if (interpretIdentity2(brl, packet.data.bytes)) {
            protocol = &protocolOperations2;

            {
              static const DotsTable dots = {0X80, 0X40, 0X20, 0X10, 0X08, 0X04, 0X02, 0X01};
              makeOutputTable(dots, outputTable);
            }

            inputKeySize2 = (model->protocolRevision < 2)? 4: 8;
            {
              int keyCount = model->leftKeys + model->rightKeys;
              inputBytes2 = keyCount + 1 +
                            ((((keyCount * inputKeySize2) +
                               ((model->textColumns + model->statusCount) * 2)
                              ) + 7) / 8);
            }
            inputBits2 = inputBytes2 * 8;

            if ((inputMap2 = malloc(inputBits2 * sizeof(*inputMap2)))) {
              mapInputModules2();

              if ((inputState2 = malloc(inputBytes2))) {
                return 1;
              }

              free(inputMap2);
              inputMap2 = NULL;
            }
          }
        }
      }
    }
    if (errno != EAGAIN) break;

    if (++tries == io->protocol2) break;
  }
  return 0;
}

/*--- Driver Operations ---*/

static int
identifyTerminal (BrailleDisplay *brl) {
  if (io->protocol1 && identifyTerminal1(brl)) return 1;
  if (io->protocol2 && identifyTerminal2(brl)) return 1;
  return 0;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if (isSerialDevice(&device)) {
    io = &serialOperations;
  } else if (isUsbDevice(&device)) {
    io = &usbOperations;
  } else if (isBluetoothDevice(&device)) {
    io = &bluetoothOperations;
  } else {
    unsupportedDevice(device);
    goto failed;
  }

  baud = io->bauds;
  while (*baud) {
    LogPrint(LOG_DEBUG, "Probing Papenmeier display at %d baud.", *baud);
    charactersPerSecond = *baud / 10;

    if (io->openPort(parameters, device)) {
      if (identifyTerminal(brl)) {
        memset(currentText, outputTable[0], model->textColumns);
        memset(currentStatus, outputTable[0], model->statusCount);
        protocol->initializeTerminal(brl);
        if (io->preparePort()) return 1;
      }
      io->closePort();
    }

    ++baud;
  }

failed:
  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  io->closePort();
  protocol->releaseResources();
}

static void
updateCells (BrailleDisplay *brl, int size, const unsigned char *data, unsigned char *cells,
             void (*writeCells) (BrailleDisplay *brl, int start, int count)) {
  if (memcmp(cells, data, size) != 0) {
    int index;

    while (size) {
      index = size - 1;
      if (cells[index] != data[index]) break;
      size = index;
    }

    for (index=0; index<size; ++index) {
      if (cells[index] != data[index]) break;
    }

    if ((size -= index)) {
      memcpy(cells+index, data+index, size);
      writeCells(brl, index, size);
    }
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  int i;
  for (i=0; i<model->textColumns; i++) brl->buffer[i] = outputTable[brl->buffer[i]];
  updateCells(brl, model->textColumns, brl->buffer, currentText, protocol->writeText);
  protocol->flushCells(brl);
  return 1;
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char* s) {
  if (model->statusCount) {
    unsigned char cells[model->statusCount];
    if (s[BRL_firstStatusCell] == BRL_STATUS_CELLS_GENERIC) {
      int i;

      unsigned char values[BRL_genericStatusCellCount];
      memcpy(values, s, BRL_genericStatusCellCount);

      for (i=0; i<model->statusCount; i++) {
        int code = model->statusCells[i];
        if (code == OFFS_EMPTY)
          cells[i] = 0;
        else if (code >= OFFS_NUMBER)
          cells[i] = outputTable[portraitNumber(values[code-OFFS_NUMBER])];
        else if (code >= OFFS_FLAG)
          cells[i] = outputTable[seascapeFlag(i+1, values[code-OFFS_FLAG])];
        else if (code >= OFFS_HORIZ)
          cells[i] = outputTable[seascapeNumber(values[code-OFFS_HORIZ])];
        else
          cells[i] = outputTable[values[code]];
      }
    } else {
      int i = 0;
      while (i < model->statusCount) {
        unsigned char dots = s[i];
        if (!dots) break;
        cells[i++] = outputTable[dots];
      }
      while (i < model->statusCount) cells[i++] = outputTable[0];
    }
    updateCells(brl, model->statusCount, cells, currentStatus, protocol->writeStatus);
  }
  return 1;
}

static int 
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return protocol->readCommand(brl, context);
}

static void
brl_firmness (BrailleDisplay *brl, BrailleFirmness setting) {
  if (protocol->setFirmness) protocol->setFirmness(brl, setting);
}
