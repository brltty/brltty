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

#include "Programs/misc.h"

typedef enum {
   PARM_CONFIGFILE,
   PARM_DEBUGKEYS,
   PARM_DEBUGREADS,
   PARM_DEBUGWRITES
} DriverParameter;
#define BRLPARMS "configfile", "debugkeys", "debugreads", "debugwrites"

#define BRLSTAT ST_Generic
#define BRLCONST
#include "Programs/brl_driver.h"
#include "braille.h"

#ifdef ENABLE_PM_CONFIGURATION_FILE
#include "config.tab.c"
 
static int
yyerror (char *problem)  /* Called by yyparse on error */
{
  LogPrint(LOG_CRIT, "Papenmeier configuration error: line %d: %s",
           lineNumber, problem);
  return 0;
}

static void
loadConfigurationFile (const char *path) {
  LogPrint(LOG_DEBUG, "Loading Papenmeier configuration file: %s", path);
  if ((configurationFile = fopen(path, "r")) != NULL) {
    parseConfigurationFile();
    fclose(configurationFile);
    configurationFile = NULL;
  } else {
    LogPrint((errno == ENOENT)? LOG_DEBUG: LOG_ERR,
             "Cannot open Papenmeier configuration file '%s': %s",
             path, strerror(errno));
  }
}
#else /* ENABLE_PM_CONFIGURATION_FILE */
#include "brl-cfg.h"
#endif /* ENABLE_PM_CONFIGURATION_FILE */

static unsigned int debugKeys = 0;
static unsigned int debugReads = 0;
static unsigned int debugWrites = 0;

/*--- Command Determination ---*/

static const TerminalDefinition *terminal = NULL;
static TranslationTable outputTable;
static unsigned int currentModifiers = 0;
static unsigned int activeModifiers = 0;
static int inputMode = 0;
 
static void
resetState (void) {
  currentModifiers = 0;
  activeModifiers = 0;
  inputMode = 0;
}

static int
compareCommands (const void *item1, const void *item2) {
  const CommandDefinition *cmd1 = item1;
  const CommandDefinition *cmd2 = item2;
  if (cmd1->key < cmd2->key) return -1;
  if (cmd1->key > cmd2->key) return 1;
  if (cmd1->modifiers < cmd2->modifiers) return -1;
  if (cmd1->modifiers > cmd2->modifiers) return 1;
  return 0;
}

static void
sortCommands (void) {
  qsort(terminal->commands, terminal->commandCount, sizeof(*terminal->commands), compareCommands);
}

static int
findCommand (int *command, int key, int modifiers) {
  int first = 0;
  int last = terminal->commandCount - 1;
  CommandDefinition ref;
  ref.key = key;
  ref.modifiers = modifiers;
  while (first <= last) {
    int current = (first + last) / 2;
    CommandDefinition *cmd = &terminal->commands[current];
    int relation = compareCommands(cmd, &ref);

    if (!relation) {
      *command = cmd->code;
      return 1;
    }

    if (relation > 0) {
      last = current - 1;
    } else {
      first = current + 1;
    }
  }
  return 0;
}

static void
logModifiers (void) {
  if (debugKeys) {
    LogPrint(LOG_DEBUG, "modifiers: %04X [%04X]",
             currentModifiers, activeModifiers);
  }
}

static int
changeModifiers (int remove, int add) {
  int originalModifiers = currentModifiers;

  currentModifiers &= ~remove;
  currentModifiers |= add;

  if (currentModifiers != originalModifiers) {
    activeModifiers = (currentModifiers & ~originalModifiers)? currentModifiers: 0;
    logModifiers();
  }

  return BRL_CMD_NOOP;
}

static int
handleCommand (BrailleDisplay *brl, int cmd, int repeat) {
  if (cmd == BRL_CMD_INPUT) {
    /* translate toggle -> ON/OFF */
    cmd |= inputMode? BRL_FLG_TOGGLE_OFF: BRL_FLG_TOGGLE_ON;
  }

  if (!IS_DELAYED_COMMAND(repeat)) {
    switch (cmd) {
      case BRL_CMD_INPUT | BRL_FLG_TOGGLE_ON:
        inputMode = 1;
        cmd = BRL_CMD_NOOP | BRL_FLG_TOGGLE_ON;
        if (debugKeys) {
          LogPrint(LOG_DEBUG, "input mode on"); 
        }
        break;

      case BRL_CMD_INPUT | BRL_FLG_TOGGLE_OFF:
        inputMode = 0;
        cmd = BRL_CMD_NOOP | BRL_FLG_TOGGLE_OFF;
        if (debugKeys) {
          LogPrint(LOG_DEBUG, "input mode off"); 
        }
        break;

      case BRL_CMD_SWSIM_LC:
        return changeModifiers(MOD_BAR_SLR|MOD_BAR_SLF, MOD_BAR_SLC);
      case BRL_CMD_SWSIM_LR:
        return changeModifiers(MOD_BAR_SLF, MOD_BAR_SLR);
      case BRL_CMD_SWSIM_LF:
        return changeModifiers(MOD_BAR_SLR, MOD_BAR_SLF);
      case BRL_CMD_SWSIM_RC:
        return changeModifiers(MOD_BAR_SRR|MOD_BAR_SRF, MOD_BAR_SRC);
      case BRL_CMD_SWSIM_RR:
        return changeModifiers(MOD_BAR_SRF, MOD_BAR_SRR);
      case BRL_CMD_SWSIM_RF:
        return changeModifiers(MOD_BAR_SRR, MOD_BAR_SRF);
      case BRL_CMD_SWSIM_BC:
        return changeModifiers(MOD_BAR_SLR|MOD_BAR_SLF|MOD_BAR_SRR|MOD_BAR_SRF, MOD_BAR_SLC|MOD_BAR_SRC);
      case BRL_CMD_SWSIM_BQ: {
        static const char *const states[] = {"center", "rear", "front", "both"};
        const char *left = states[currentModifiers & 0X3];
        const char *right = states[(currentModifiers >> 2) & 0X3];
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%-6s %-6s", left, right);
        showBrailleString(brl, buffer, 2000);
        return BRL_CMD_NOOP;
      }
    }
  }

  return cmd | repeat;
}

static int
handleModifier (BrailleDisplay *brl, int bit, int press) {
  int command = BRL_CMD_NOOP;
  int modifiers;

  if (press) {
    activeModifiers = (currentModifiers |= bit);
    modifiers = activeModifiers;
  } else {
    currentModifiers &= ~bit;
    modifiers = activeModifiers;
    activeModifiers = 0;
  }
  logModifiers();

  if (modifiers) {
    if (inputMode && !(modifiers & ~0XFF)) {
      static const unsigned char dots[] = {BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4, BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8};
      const unsigned char *dot = dots;
      int mod;
      command = BRL_BLK_PASSDOTS;
      for (mod=1; mod<0X100; ++dot, mod<<=1)
        if (modifiers & mod)
          command |= *dot;
      if (debugKeys)
        LogPrint(LOG_DEBUG, "cmd: [%02X]->%04X", modifiers, command); 
    } else if (findCommand(&command, NOKEY, modifiers)) {
      if (debugKeys)
        LogPrint(LOG_DEBUG, "cmd: [%04X]->%04X",
                 modifiers, command); 
    }
  }

  return handleCommand(brl, command, (press? BRL_FLG_REPEAT_DELAY: 0));
}

static int
handleKey (BrailleDisplay *brl, int code, int press, int offset) {
  int i;
  int cmd;

  /* look for modfier keys */
  for (i=0; i<terminal->modifierCount; i++)
    if (terminal->modifiers[i] == code)
      return handleModifier(brl, 1<<i, press);

  /* must be a "normal key" - search for cmd on key press */
  if (press) {
    int command;
    activeModifiers = 0;
    if (findCommand(&command, code, currentModifiers)) {
      if (debugKeys)
        LogPrint(LOG_DEBUG, "cmd: %d[%04X]->%04X (+%d)", 
                 code, currentModifiers, command, offset); 
      return handleCommand(brl, command+offset,
                           (BRL_FLG_REPEAT_INITIAL | BRL_FLG_REPEAT_DELAY));
    }

    /* no command found */
    LogPrint(LOG_DEBUG, "cmd: %d[%04X] ??", code, currentModifiers); 
  }
  return BRL_CMD_NOOP;
}

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

#include "Programs/io_serial.h"
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

#ifdef ENABLE_USB_SUPPORT
#include "Programs/io_usb.h"
static UsbChannel *usb = NULL;
static const int usbBauds[] = {57600, 0};

static int
openUsbPort (char **parameters, const char *device) {
  const UsbChannelDefinition definitions[] = {
    {0X0403, 0Xf208, 1, 0, 0, 1, 2, *baud, 0, 8, 1, SERIAL_PARITY_NONE},
    {0}
  };

  if ((usb = usbFindChannel(definitions, (void *)device))) {
    usbBeginInput(usb->device, usb->definition.inputEndpoint, 8);
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
#endif /* ENABLE_USB_SUPPORT */

/*--- Protocol Operation Utilities ---*/

typedef struct {
  void (*initializeTerminal) (BrailleDisplay *brl);
  void (*releaseResources) (void);
  int (*readCommand) (BrailleDisplay *brl, BRL_DriverCommandContext context);
  void (*writeText) (BrailleDisplay *brl, int start, int count);
  void (*writeStatus) (BrailleDisplay *brl, int start, int count);
  void (*flushCells) (BrailleDisplay *brl);
} ProtocolOperations;

static const ProtocolOperations *protocol;
static unsigned char currentStatus[PMSC];
static unsigned char currentText[BRLCOLSMAX];


static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, int count) {
  if (debugWrites) LogBytes(LOG_DEBUG, "Write", bytes, count);
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
  int tn;
  LogPrint(LOG_INFO, "Papenmeier ID: %d  Version: %d.%02d", 
           id, major, minor);
  for (tn=0; tn<pmTerminalCount; tn++) {
    if (pmTerminals[tn].identifier == id) {
      terminal = &pmTerminals[tn];
      LogPrint(LOG_INFO, "%s  Size: %dx%d  HelpFile: %s", 
               terminal->name,
               terminal->columns, terminal->rows,
               terminal->helpFile);
      brl->x = terminal->columns;
      brl->y = terminal->rows;

      /* TODO: ?? HACK */
      BRLSYMBOL.helpFile = terminal->helpFile;

      sortCommands();
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
  if (writeBytes(brl, sequence, sizeof(sequence))) resetState();
}

#define RBF_ETX 1
#define RBF_RESET 2
static int
readBytes1 (BrailleDisplay *brl, unsigned char *buffer, size_t offset, size_t count, int flags) {
  if (io->readBytes(buffer, &offset, count, 1000)) {
    if (!(flags & RBF_ETX)) return 1;
    if (*(buffer+offset-1) == cETX) return 1;
    LogBytes(LOG_WARNING, "Corrupt Packet", buffer, offset);
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
  rcvStatusLast  = rcvStatusFirst + 3 * (terminal->statusCount - 1);
  rcvCursorFirst = rcvStatusLast + 3;
  rcvCursorLast  = rcvCursorFirst + 3 * (terminal->columns - 1);
  LogPrint(LOG_DEBUG, "Routing Keys: status=%03X-%03X cursor=%03X-%03X",
           rcvStatusFirst, rcvStatusLast,
           rcvCursorFirst, rcvCursorLast);

  /* function key codes: 0X000 -> front -> bar -> switches */
  rcvFrontFirst = RCV_KEYFUNC + 3;
  rcvFrontLast  = rcvFrontFirst + 3 * (terminal->frontKeys - 1);
  rcvBarFirst = rcvFrontLast + 3;
  rcvBarLast  = rcvBarFirst + 3 * ((terminal->hasBar? 8: 0) - 1);
  rcvSwitchFirst = rcvBarLast + 3;
  rcvSwitchLast  = rcvSwitchFirst + 3 * ((terminal->hasBar? 8: 0) - 1);
  LogPrint(LOG_DEBUG, "Function Keys: front=%03X-%03X bar=%03X-%03X switches=%03X-%03X",
           rcvFrontFirst, rcvFrontLast,
           rcvBarFirst, rcvBarLast,
           rcvSwitchFirst, rcvSwitchLast);

  /* cell offsets: 0X00 -> status -> text */
  xmtStatusOffset = 0;
  xmtTextOffset = xmtStatusOffset + terminal->statusCount;
  LogPrint(LOG_DEBUG, "Cell Offsets: status=%02X text=%02X",
           xmtStatusOffset, xmtTextOffset);

  return 1;
}

static int
handleKey1 (BrailleDisplay *brl, int code, int press, int time) {
  /* which key -> translate to OFFS_* + number */
  /* attn: number starts with 1 */
  int num;

  if (rcvFrontFirst <= code && 
      code <= rcvFrontLast) { /* front key */
    num = 1 + (code - rcvFrontFirst) / 3;
    return handleKey(brl, OFFS_FRONT+num, press, 0);
  }

  if (rcvStatusFirst <= code && 
      code <= rcvStatusLast) { /* status key */
    num = 1 + (code - rcvStatusFirst) / 3;
    return handleKey(brl, OFFS_STAT+num, press, 0);
  }

  if (rcvBarFirst <= code && 
      code <= rcvBarLast) { /* easy access bar */
    {
      static const int modifiers[] = {
        MOD_BAR_SLR, MOD_BAR_SLF,
        MOD_BAR_KLR, MOD_BAR_KLF,
        MOD_BAR_KRR, MOD_BAR_KRF,
        MOD_BAR_SRR, MOD_BAR_SRF,
        0
      };
      const int *modifier = modifiers;

      int remove = 0;
      int add = 0;
      int bit = 1;

      while (*modifier) {
        if (time & bit) {
          add |= *modifier;
        } else {
          remove |= *modifier;
        }

        bit <<= 1;
        ++modifier;
      }

      changeModifiers(remove, add);
    }

    num = 1 + (code - rcvBarFirst) / 3;
    return handleKey(brl, OFFS_BAR+num, press, 0);
  }

  if (rcvSwitchFirst <= code && 
      code <= rcvSwitchLast) { /* easy access bar */
    num = 1 + (code - rcvSwitchFirst) / 3;
    return handleKey(brl, OFFS_SWITCH+num, press, 0);
  }

  if (rcvCursorFirst <= code && 
      code <= rcvCursorLast) { /* Routing Keys */ 
    num = (code - rcvCursorFirst) / 3;
    return handleKey(brl, ROUTINGKEY1, press, num);
  }

  LogPrint(LOG_WARNING, "Unexpected key: %04X", code);
  return BRL_CMD_NOOP;
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
  disableOutputTranslation1(brl, xmtStatusOffset, terminal->statusCount);
  disableOutputTranslation1(brl, xmtTextOffset, terminal->columns);
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

  writeStatus1(brl, 0, terminal->statusCount);
  drainBrailleOutput(brl, 0);

  writeText1(brl, 0, terminal->columns);
  drainBrailleOutput(brl, 0);
}

static void
restartTerminal1 (BrailleDisplay *brl) {
  initializeTerminal1(brl);
  resetState();
}

static int
readCommand1 (BrailleDisplay *brl, BRL_DriverCommandContext context) {
#define READ(offset,count,flags) { if (!readBytes1(brl, buf, offset, count, RBF_RESET|(flags))) return EOF; }
  while (1) {
    unsigned char buf[0X100];

    while (1) {
      READ(0, 1, 0);
      if (buf[0] == cSTX) break;
      LogBytes(LOG_WARNING, "Discarded Byte", buf, 1);
    }

    READ(1, 1, 0);
    switch (buf[1]) {
      default: {
        int i;
        LogPrint(LOG_WARNING, "unknown packet: %02X", buf[1]);
        for (i=2; i<sizeof(buf); i++) {
          READ(i, 1, 0);
          LogPrint(LOG_WARNING, "packet byte %2d: %02X", i, buf[i]);
        }
        break;
      }

      case cIdIdentify: {
        const int length = 10;
        READ(2, length-2, RBF_ETX);
        if (debugReads) LogBytes(LOG_DEBUG, "Identity Packet", buf, length);
        if (interpretIdentity1(brl, buf)) brl->resizeRequired = 1;
        approximateDelay(200);
        restartTerminal1(brl);
        break;
      }

      case cIdReceive: {
        int length;
        int i;

        READ(2, 4, 0);
        length = (buf[4] << 8) | buf[5];	/* packet size */
        if (length != 10) {
          LogPrint(LOG_WARNING, "Unexpected input packet length: %d", length);
          resetTerminal1(brl);
          return EOF;
        }
        READ(6, length-6, RBF_ETX);			/* Data */
        if (debugReads) LogBytes(LOG_DEBUG, "Input Packet", buf, length);

        {
          int command = handleKey1(brl, ((buf[2] << 8) | buf[3]),
                                   (buf[6] == PRESSED),
                                   ((buf[7] << 8) | buf[8]));
          if (command != EOF) return command;
        }
        break;
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
      logError:
        READ(2, 1, RBF_ETX);
        if (debugReads) LogBytes(LOG_DEBUG, "Error Packet", buf, 3);
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
  writeText1, writeStatus1, flushCells1
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
  int code;
  int offset;
} InputMapping2;
static InputMapping2 *inputMap2 = NULL;
static int inputBytes2;
static int inputBits2;

static unsigned char *inputState2 = NULL;
static int refreshRequired2;

static int
readPacket2 (BrailleDisplay *brl, Packet2 *packet) {
  unsigned char buffer[0X203];
  size_t offset = 0;
  size_t size;
  int identity;

  while (1) {
    if (!io->readBytes(buffer, &offset, 1, 1000)) {
      LogBytes(LOG_WARNING, "Partial Packet", buffer, offset);
      return 0;
    }

    {
      unsigned char byte = buffer[offset-1];
      unsigned char type = HIGH_NIBBLE(byte);
      unsigned char value = LOW_NIBBLE(byte);

      switch (byte) {
        case cSTX:
          if (offset > 1) {
            LogBytes(LOG_WARNING, "Incomplete Packet", buffer, offset);
            offset = 1;
          }
          continue;

        case cETX:
          if ((offset >= 5) && (offset == size)) {
            if (debugReads) LogBytes(LOG_DEBUG, "Input Packet", buffer, offset);
            return 1;
          }
          LogBytes(LOG_WARNING, "Short Packet", buffer, offset);
          offset = 0;
          continue;

        default:
          switch (offset) {
            case 1:
              LogBytes(LOG_WARNING, "Discarded Byte", buffer, offset);
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
                LogBytes(LOG_WARNING, "Long Packet", buffer, offset);
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

    LogBytes(LOG_WARNING, "Corrupt Packet", buffer, offset);
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

    /* Two dummy cells for each key on the left side. */
    {
      int count = terminal->leftKeys;
      while (count-- > 0) {
        buffer[size++] = 0;
        buffer[size++] = 0;
      }
    }

    /* The text cells. */
    memcpy(&buffer[size], currentText, terminal->columns);
    size += terminal->columns;

    /* The status cells. */
    memcpy(&buffer[size], currentStatus, terminal->statusCount);
    size += terminal->statusCount;

    /* Two dummy cells for each key on the right side. */
    {
      int count = terminal->rightKeys;
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

  {
    unsigned char data[13];
    unsigned char size = 0;

    data[size++] = terminal->identifier;

    /* Set serial baud (bcd encoded) to default (57,600). */
    data[size++] = 0;
    data[size++] = 0;
    data[size++] = 0;

    data[size++] = 0; /* vertical modules */
    data[size++] = terminal->leftKeys;
    data[size++] = terminal->columns + terminal->statusCount;
    data[size++] = terminal->rightKeys;

    data[size++] = 2; /* routing keys per cell */
    data[size++] = 0; /* size of LCD */

    data[size++] = 1; /* keys mixed into braille data stream */
    data[size++] = 0; /* EAB mixed into braille data stream */
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
        int command = BRL_CMD_NOOP;
        int bytes = MIN(packet.length, inputBytes2);
        int byte;

        /* Find out which keys have been released.
         * The first one determines the command to be executed.
         */
        {
          int release = 0;
          for (byte=0; byte<bytes; ++byte) {
            unsigned char old = inputState2[byte];
            unsigned char new = packet.data.bytes[byte];

            if (new != old) {
              InputMapping2 *mapping = &inputMap2[byte * 8];
              unsigned char bit = 0X01;

              while (bit) {
                if (!(new & bit) && (old & bit)) {
                  if (mapping->code != NOKEY) {
                    int cmd = handleKey(brl, mapping->code, 0, mapping->offset);
                    if (!release) {
                      command = cmd;
                      release = 1;
                    }
                  }

                  if ((inputState2[byte] &= ~bit) == new) break;
                }

                mapping++;
                bit <<= 1;
              }
            }
          }
        }

        /* Find out which keys have been pressed.
         * The last one determines the command to be executed.
         */
        for (byte=0; byte<bytes; ++byte) {
          unsigned char old = inputState2[byte];
          unsigned char new = packet.data.bytes[byte];

          if (new != old) {
            InputMapping2 *mapping = &inputMap2[byte * 8];
            unsigned char bit = 0X01;

            while (bit) {
              if ((new & bit) && !(old & bit)) {
                if (mapping->code != NOKEY) {
                  command = handleKey(brl, mapping->code, 1, mapping->offset);
                }

                if ((inputState2[byte] |= bit) == new) break;
              }

              mapping++;
              bit <<= 1;
            }
          }
        }

        return command;
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

static const ProtocolOperations protocolOperations2 = {
  initializeTerminal2, releaseResources2,
  readCommand2,
  writeCells2, writeCells2, flushCells2
};

static void
addInputMapping2 (int byte, int bit, int code, int offset) {
  InputMapping2 *mapping = &inputMap2[(byte * 8) + bit];
  mapping->code = code;
  mapping->offset = offset;
}

static void
nextInputModule2 (int *byte, int *bit) {
  if (!*bit) *bit = 8, *byte -= 1;
  *bit -= 2;
}

static void
mapKey2 (int count, int *byte, int *bit, int rear, int front) {
  while (count--) {
    nextInputModule2(byte, bit);
    nextInputModule2(byte, bit);
    addInputMapping2(*byte, *bit+1, front, 0);
    addInputMapping2(*byte, *bit, rear, 0);
  }
}

static void
mapInputModules2 (void) {
  int byte = inputBytes2;
  int bit = 0;

  {
    int i;
    for (i=0; i<inputBits2; ++i) {
      InputMapping2 *mapping = &inputMap2[i];
      mapping->code = NOKEY;
      mapping->offset = 0;
    }
  }

  mapKey2(terminal->rightKeys, &byte, &bit,
          OFFS_SWITCH+KEY_RIGHT_REAR,
          OFFS_SWITCH+KEY_RIGHT_FRONT);

  {
    unsigned char cell = terminal->statusCount;
    while (cell) {
      nextInputModule2(&byte, &bit);
      addInputMapping2(byte, bit, OFFS_STAT+cell--, 0);
    }
  }

  {
    unsigned char column = terminal->columns;
    while (column) {
      nextInputModule2(&byte, &bit);
      addInputMapping2(byte, bit, ROUTINGKEY1, column-1);
      addInputMapping2(byte, bit+1, ROUTINGKEY2, --column);
    }
  }

  mapKey2(terminal->leftKeys, &byte, &bit,
          OFFS_SWITCH+KEY_LEFT_REAR,
          OFFS_SWITCH+KEY_LEFT_FRONT);

  byte--;
  addInputMapping2(byte, 7, OFFS_BAR+BAR_L2, 0);
  addInputMapping2(byte, 6, OFFS_BAR+BAR_R2, 0);
  addInputMapping2(byte, 5, OFFS_BAR+BAR_L1, 0);
  addInputMapping2(byte, 4, OFFS_BAR+BAR_R1, 0);
  addInputMapping2(byte, 3, OFFS_BAR+BAR_D2, 0);
  addInputMapping2(byte, 2, OFFS_BAR+BAR_D1, 0);
  addInputMapping2(byte, 1, OFFS_BAR+BAR_U1, 0);
  addInputMapping2(byte, 0, OFFS_BAR+BAR_U2, 0);
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

            {
              int modules = terminal->leftKeys + terminal->rightKeys;
              inputBytes2 = modules + 1 + ((((modules * 4) + ((terminal->columns + terminal->statusCount) * 2)) + 7) / 8);
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

static void
resetTerminalTable (void) {
  if (pmTerminalsAllocated) {
#ifdef ENABLE_PM_CONFIGURATION_FILE
    deallocateTerminalTable();
#endif /* ENABLE_PM_CONFIGURATION_FILE */
    pmTerminals = pmTerminalTable;
    pmTerminalCount = ARRAY_COUNT(pmTerminalTable);
    pmTerminalsAllocated = 0;
  }
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  if (!validateYesNo(&debugKeys, parameters[PARM_DEBUGKEYS]))
    LogPrint(LOG_WARNING, "%s: %s", "invalid debug keys setting", parameters[PARM_DEBUGKEYS]);

  if (!validateYesNo(&debugReads, parameters[PARM_DEBUGREADS]))
    LogPrint(LOG_WARNING, "%s: %s", "invalid debug reads setting", parameters[PARM_DEBUGREADS]);

  if (!validateYesNo(&debugWrites, parameters[PARM_DEBUGWRITES]))
    LogPrint(LOG_WARNING, "%s: %s", "invalid debug writes setting", parameters[PARM_DEBUGWRITES]);

#ifdef ENABLE_PM_CONFIGURATION_FILE
  {
    const char *name = parameters[PARM_CONFIGFILE];
    if (!*name) {
      if (!(name = getenv(PM_CONFIG_ENV))) {
        name = PM_CONFIG_FILE;
      }
    }

    {
      char *path = makePath(brl->dataDirectory, name);
      if (path) {
        loadConfigurationFile(path);
        free(path);
      }
    }
  }
#endif /* ENABLE_PM_CONFIGURATION_FILE */

  if (isSerialDevice(&device)) {
    io = &serialOperations;
  } else

#ifdef ENABLE_USB_SUPPORT
  if (isUsbDevice(&device)) {
    io = &usbOperations;
  } else
#endif /* ENABLE_USB_SUPPORT */

  {
    unsupportedDevice(device);
    goto failed;
  }

  baud = io->bauds;
  while (*baud) {
    LogPrint(LOG_DEBUG, "Probing Papenmeier display at %d baud.", *baud);
    charactersPerSecond = *baud / 10;

    if (io->openPort(parameters, device)) {
      if (identifyTerminal(brl)) {
        memset(currentText, outputTable[0], terminal->columns);
        memset(currentStatus, outputTable[0], terminal->statusCount);
        resetState();
        protocol->initializeTerminal(brl);
        if (io->preparePort()) return 1;
      }
      io->closePort();
    }

    ++baud;
  }

failed:
  resetTerminalTable();
  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  io->closePort();
  protocol->releaseResources();
  resetTerminalTable();
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

static void
brl_writeWindow (BrailleDisplay *brl) {
  int i;
  for (i=0; i<terminal->columns; i++) brl->buffer[i] = outputTable[brl->buffer[i]];
  updateCells(brl, terminal->columns, brl->buffer, currentText, protocol->writeText);
  protocol->flushCells(brl);
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char* s) {
  if (terminal->statusCount) {
    unsigned char cells[terminal->statusCount];
    if (s[BRL_firstStatusCell] == BRL_STATUS_CELLS_GENERIC) {
      int i;

      unsigned char values[InternalStatusCellCount];
      memcpy(values, s, BRL_genericStatusCellCount);
      values[BRL_GSC_INPUT] = inputMode;

      for (i=0; i<terminal->statusCount; i++) {
        int code = terminal->statusCells[i];
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
      while (i < terminal->statusCount) {
        unsigned char dots = s[i];
        if (!dots) break;
        cells[i++] = outputTable[dots];
      }
      while (i < terminal->statusCount) cells[i++] = outputTable[0];
    }
    updateCells(brl, terminal->statusCount, cells, currentStatus, protocol->writeStatus);
  }
}

static int 
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return protocol->readCommand(brl, context);
}
