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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>

#include "Programs/brl.h"
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
read_file (const char *name) {
  LogPrint(LOG_DEBUG, "Opening config file: %s", name);
  if ((configurationFile = fopen(name, "r")) != NULL) {
    LogPrint(LOG_DEBUG, "Reading config file: %s", name);
    parse();
    fclose(configurationFile);
    configurationFile = NULL;
  } else {
    LogPrint((errno == ENOENT)? LOG_DEBUG: LOG_ERR,
             "Cannot open Papenmeier configuration file '%s': %s",
             name, strerror(errno));
  }
}

static void
read_config (const char *directory, const char *name) {
  if (!*name) {
    if (!(name = getenv(PM_CONFIG_ENV))) {
      name = PM_CONFIG_FILE;
    }
  }
  {
    char *path = makePath(directory, name);
    if (path) {
      LogPrint(LOG_INFO, "Papenmeier Configuration File: %s", path);
      read_file(path);
      free(path);
    }
  }
}
#else /* ENABLE_PM_CONFIGURATION_FILE */
#include "brl-cfg.h"
#endif /* ENABLE_PM_CONFIGURATION_FILE */

#define CMD_ERR	EOF

static unsigned int debug_keys = 0;
static unsigned int debug_reads = 0;
static unsigned int debug_writes = 0;

/*--- Command Determination ---*/

static const TerminalDefinition *terminal = NULL;
static TranslationTable outputTable;
static unsigned int pressed_modifiers = 0;
static unsigned int saved_modifiers = 0;
static int input_mode = 0;
 
static void
resetState (void) {
  pressed_modifiers = 0;
  saved_modifiers = 0;
  input_mode = 0;
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

static int
handleCommand (int cmd, int repeat) {
  if (cmd == CMD_INPUT) {
    /* translate toggle -> ON/OFF */
    cmd |= input_mode? VAL_TOGGLE_OFF: VAL_TOGGLE_ON;
  }

  switch (cmd) {
    case CMD_INPUT | VAL_TOGGLE_ON:
      input_mode = 1;
      cmd = VAL_TOGGLE_ON;
      if (debug_keys) {
        LogPrint(LOG_DEBUG, "input mode on"); 
      }
      break;
    case CMD_INPUT | VAL_TOGGLE_OFF:
      input_mode = 0;
      cmd = VAL_TOGGLE_OFF;
      if (debug_keys) {
        LogPrint(LOG_DEBUG, "input mode off"); 
      }
      break;
  }

  return cmd | repeat;
}

static int
handleModifier (int bit, int press) {
  int command = CMD_NOOP;
  int modifiers;

  if (press) {
    saved_modifiers = (pressed_modifiers |= bit);
    modifiers = saved_modifiers;
  } else {
    pressed_modifiers &= ~bit;
    modifiers = saved_modifiers;
    saved_modifiers = 0;
  }

  if (debug_keys) {
    LogPrint(LOG_DEBUG, "modifiers: %04X", pressed_modifiers);
  }

  if (modifiers) {
    if (input_mode && !(modifiers & ~0XFF)) {
      static const unsigned char dots[] = {B1, B2, B3, B4, B5, B6, B7, B8};
      const unsigned char *dot = dots;
      int mod;
      command = VAL_PASSDOTS;
      for (mod=1; mod<0X100; ++dot, mod<<=1)
        if (modifiers & mod)
          command |= *dot;
      if (debug_keys)
        LogPrint(LOG_DEBUG, "cmd: [%02X]->%04X", modifiers, command); 
    } else if (findCommand(&command, NOKEY, modifiers)) {
      if (debug_keys)
        LogPrint(LOG_DEBUG, "cmd: [%04X]->%04X",
                 modifiers, command); 
    }
  }

  return handleCommand(command, (press? VAL_REPEAT_DELAY: 0));
}

static int
handleKey (int code, int press, int offsroute) {
  int i;
  int cmd;

  /* look for modfier keys */
  for (i=0; i<terminal->modifierCount; i++)
    if (terminal->modifiers[i] == code)
      return handleModifier(1<<i, press);

  /* must be a "normal key" - search for cmd on key press */
  if (press) {
    int command;
    saved_modifiers = 0;
    if (findCommand(&command, code, pressed_modifiers)) {
      if (debug_keys)
        LogPrint(LOG_DEBUG, "cmd: %d[%04X]->%04X (+%d)", 
                 code, pressed_modifiers, command, offsroute); 
      return handleCommand(command + offsroute,
                           (VAL_REPEAT_INITIAL | VAL_REPEAT_DELAY));
    }

    /* no command found */
    LogPrint(LOG_DEBUG, "cmd: %d[%04X] ??", code, pressed_modifiers); 
  }
  return CMD_NOOP;
}

/*--- Input/Output Operations ---*/

typedef struct {
  const speed_t *speeds;
  unsigned char protocol1;
  unsigned char protocol2;
  int (*openPort) (char **parameters, const char *device);
  void (*closePort) (void);
  void (*flushPort) (BrailleDisplay *brl);
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (void *buffer, int *offset, int length, int timeout);
  int (*writeBytes) (const void *buffer, int length);
} InputOutputOperations;

static const InputOutputOperations *io;
static const speed_t *speed;
static int charactersPerSecond;

/*--- Serial Operations ---*/

#include "Programs/serial.h"
static int serialDevice = -1;
static struct termios oldSerialSettings;
static const speed_t serialSpeeds[] = {B19200, B38400, B0};

static int
openSerialPort (char **parameters, const char *device) {
  if (openSerialDevice(device, &serialDevice, &oldSerialSettings)) {
    struct termios newSerialSettings;

    memset(&newSerialSettings, 0, sizeof(newSerialSettings));
    newSerialSettings.c_cflag = CRTSCTS | CS8 | CLOCAL | CREAD;
    newSerialSettings.c_iflag = IGNPAR;
    newSerialSettings.c_oflag = 0;		/* raw output */
    newSerialSettings.c_lflag = 0;		/* don't echo or generate signals */
    newSerialSettings.c_cc[VMIN] = 0;	/* set nonblocking read */
    newSerialSettings.c_cc[VTIME] = 0;

    if (resetSerialDevice(serialDevice, &newSerialSettings, *speed)) return 1;

    close(serialDevice);
    serialDevice = -1;
  }
  return 0;
}

static void
closeSerialPort (void) {
  if (serialDevice != -1) {
    tcsetattr(serialDevice, TCSADRAIN, &oldSerialSettings);		/* restore terminal settings */
    close(serialDevice);
    serialDevice = -1;
  }
}

static void
flushSerialPort (BrailleDisplay *brl) {
  tcflush(serialDevice, TCOFLUSH);
  drainBrailleOutput(brl, 100);
  tcflush(serialDevice, TCIFLUSH);
}

static int
awaitSerialInput (int milliseconds) {
  return awaitInput(serialDevice, milliseconds);
}

static int
readSerialBytes (void *buffer, int *offset, int length, int timeout) {
  return readChunk(serialDevice, buffer, offset, length, timeout);
}

static int
writeSerialBytes (const void *buffer, int length) {
  int written = safe_write(serialDevice, buffer, length);
  if (written == -1) LogError("Serial write");
  return written;
}

static const InputOutputOperations serialOperations = {
  serialSpeeds, 1, 1,
  openSerialPort, closeSerialPort, flushSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialBytes
};

/*--- USB Operations ---*/

#ifdef ENABLE_USB
#include "Programs/usb.h"
static UsbChannel *usb = NULL;
static const speed_t usbSpeeds[] = {B57600, B0};

static int
openUsbPort (char **parameters, const char *device) {
  const int baud = baud2integer(*speed);
  const UsbChannelDefinition definitions[] = {
    {0X0403, 0Xf208, 1, 0, 0, 1, 2, baud, 0, 8, 1, USB_SERIAL_PARITY_NONE},
    {0}
  };

  if ((usb = usbFindChannel(definitions, (void *)device))) {
    usbBeginInput(usb->device, usb->definition.inputEndpoint, 8);
    return 1;
  }
  return 0;
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
readUsbBytes (void *buffer, int *offset, int length, int timeout) {
  int count = usbReapInput(usb->device, usb->definition.inputEndpoint, buffer+*offset, length, 
                           (offset? timeout: 0), timeout);
  if (count == -1)
    if (errno == EAGAIN)
      return 0;
  *offset += count;
  return 1;
}

static int
writeUsbBytes (const void *buffer, int length) {
  return usbWriteEndpoint(usb->device, usb->definition.outputEndpoint, buffer, length, 1000);
}

static const InputOutputOperations usbOperations = {
  usbSpeeds, 0, 5,
  openUsbPort, closeUsbPort, flushUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes
};
#endif /* ENABLE_USB */

/*--- Protocol Operation Utilities ---*/

typedef struct {
  void (*initializeTerminal) (BrailleDisplay *brl);
  void (*releaseResources) (void);
  int (*readCommand) (BrailleDisplay *brl, DriverCommandContext cmds);
  void (*writeText) (BrailleDisplay *brl, int start, int count);
  void (*writeStatus) (BrailleDisplay *brl, int start, int count);
  void (*flushCells) (BrailleDisplay *brl);
} ProtocolOperations;

static const ProtocolOperations *protocol;
static unsigned char currentStatus[PMSC];
static unsigned char currentText[BRLCOLSMAX];


static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, int count) {
  if (debug_writes) LogBytes("Write", bytes, count);
  if (io->writeBytes(bytes, count) != -1) {
    brl->writeDelay += count * 1000 / charactersPerSecond;
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
readBytes1 (BrailleDisplay *brl, unsigned char *buffer, int offset, int count, int flags) {
  if (io->readBytes(buffer, &offset, count, 1000)) {
    if (!(flags & RBF_ETX)) return 1;
    if (*(buffer+offset-1) == cETX) return 1;
    LogPrint(LOG_WARNING, "Input packet not terminated by ETX.");
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
  rcvBarLast  = rcvBarFirst + 3 * ((terminal->hasEasyBar? 8: 0) - 1);
  rcvSwitchFirst = rcvBarLast + 3;
  rcvSwitchLast  = rcvSwitchFirst + 3 * ((terminal->hasEasyBar? 8: 0) - 1);
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
handleKey1 (int code, int press, int time) {
  /* which key -> translate to OFFS_* + number */
  /* attn: number starts with 1 */
  int num;

  if (rcvFrontFirst <= code && 
      code <= rcvFrontLast) { /* front key */
    num = 1 + (code - rcvFrontFirst) / 3;
    return handleKey(OFFS_FRONT + num, press, 0);
  }

  if (rcvStatusFirst <= code && 
      code <= rcvStatusLast) { /* status key */
    num = 1 + (code - rcvStatusFirst) / 3;
    return handleKey(OFFS_STAT + num, press, 0);
  }

  if (rcvBarFirst <= code && 
      code <= rcvBarLast) { /* easy bar */
    num = 1 + (code - rcvBarFirst) / 3;
    return handleKey(OFFS_EASY + num, press, 0);
  }

  if (rcvSwitchFirst <= code && 
      code <= rcvSwitchLast) { /* easy bar */
    num = 1 + (code - rcvSwitchFirst) / 3;
    return handleKey(OFFS_SWITCH + num, press, 0);
  }

  if (rcvCursorFirst <= code && 
      code <= rcvCursorLast) { /* Routing Keys */ 
    num = (code - rcvCursorFirst) / 3;
    return handleKey(ROUTINGKEY, press, num);
  }

  LogPrint(LOG_WARNING, "Unexpected key: %04X", code);
  return CMD_NOOP;
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
readCommand1 (BrailleDisplay *brl, DriverCommandContext cmds) {
#define READ(offset,count,flags) { if (!readBytes1(brl, buf, offset, count, RBF_RESET|(flags))) return EOF; }
  while (1) {
    unsigned char buf[0X100];

    do {
      READ(0, 1, 0);
    } while (buf[0] != cSTX);
    if (debug_reads) LogPrint(LOG_DEBUG, "Read: STX");

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
        if (interpretIdentity1(brl, buf)) brl->resizeRequired = 1;
        delay(200);
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
          return CMD_ERR;
        }
        READ(6, length-6, RBF_ETX);			/* Data */
        if (debug_reads) LogBytes("Read", buf, length);

        {
          int command = handleKey1(((buf[2] << 8) | buf[3]),
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
                  makeOutputTable(&dots, &outputTable);
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

static int left2;
static int right2;

static int refresh2;
static Packet2 state2;

typedef struct {
  int code;
  int offset;
} InputMapping2;
static InputMapping2 *inputMap2 = NULL;
static int inputBytes2;
static int inputBits2;

static int
readPacket2 (BrailleDisplay *brl, Packet2 *packet) {
  char buffer[0X203];
  int offset = 0;
  int size;
  int identity;

  while (1) {
    if (!io->readBytes(buffer, &offset, 1, 1000)) {
      LogBytes("Partial Packet", buffer, offset);
      return 0;
    }

    {
      unsigned char byte = buffer[offset-1];
      unsigned char type = HIGH_NIBBLE(byte);
      unsigned char value = LOW_NIBBLE(byte);

      switch (byte) {
        case cSTX:
          if (offset > 1) {
            LogBytes("Incomplete Packet", buffer, offset);
            offset = 1;
          }
          continue;

        case cETX:
          if ((offset >= 5) && (offset == size)) {
            if (debug_reads) LogBytes("Input Packet", buffer, offset);
            return 1;
          }
          LogBytes("Short Packet", buffer, offset);
          offset = 0;
          continue;

        default:
          switch (offset) {
            case 1:
              LogBytes("Discarded Byte", buffer, offset);
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
                LogBytes("Long Packet", buffer, offset);
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

    LogBytes("Corrupt Packet", buffer, offset);
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
writeText2 (BrailleDisplay *brl, int start, int count) {
  refresh2 = 1;
}

static void
writeStatus2 (BrailleDisplay *brl, int start, int count) {
  refresh2 = 1;
}

static void
flushCells2 (BrailleDisplay *brl) {
  if (refresh2) {
    unsigned char buffer[0XFF];
    unsigned int size = 0;

    {
      int modules = left2;
      while (modules-- > 0) {
        buffer[size++] = 0;
        buffer[size++] = 0;
      }
    }

    memcpy(&buffer[size], currentStatus, terminal->statusCount);
    size += terminal->statusCount;

    memcpy(&buffer[size], currentText, terminal->columns);
    size += terminal->columns;

    {
      int modules = right2;
      while (modules-- > 0) {
        buffer[size++] = 0;
        buffer[size++] = 0;
      }
    }

    writePacket2(brl, 3, size, buffer);
    refresh2 = 0;
  }
}

static void
initializeTerminal2 (BrailleDisplay *brl) {
  refresh2 = 1;
  memset(&state2, 0, sizeof(state2));
}

static int 
readCommand2 (BrailleDisplay *brl, DriverCommandContext cmds) {
  Packet2 packet;

  while (readPacket2(brl, &packet)) {
    switch (packet.type) {
      default:
        LogPrint(LOG_DEBUG, "Packet ignored: %02X", packet.type);
        break;

      case 0X0B: {
        int command = CMD_NOOP;
        int bytes = MIN(packet.length, inputBytes2);
        int release = 0;
        int byte;

        for (byte=0; byte<bytes; ++byte) {
          unsigned char old = state2.data.bytes[byte];
          unsigned char new = packet.data.bytes[byte];

          if (new != old) {
            int index = byte * 8;
            unsigned char bit = 0X01;

            while (bit) {
              if (!(new & bit) && (old & bit)) {
                InputMapping2 *mapping = &inputMap2[index];
                int cmd = handleKey(mapping->code, 0, mapping->offset);

                if (!release) {
                  command = cmd;
                  release = 1;
                }

                state2.data.bytes[byte] &= ~bit;
              }

              index++;
              bit <<= 1;
            }
          }
        }

        for (byte=0; byte<bytes; ++byte) {
          unsigned char old = state2.data.bytes[byte];
          unsigned char new = packet.data.bytes[byte];

          if (new != old) {
            int index = byte * 8;
            unsigned char bit = 0X01;

            while (bit) {
              if ((new & bit) && !(old & bit)) {
                InputMapping2 *mapping = &inputMap2[index];
                command = handleKey(mapping->code, 1, mapping->offset);

                state2.data.bytes[byte] |= bit;
              }

              index++;
              bit <<= 1;
            }
          }
        }

        return command;
      }
    }
  }

  return EOF;
}

static void
releaseResources2 (void) {
  if (inputMap2) {
    free(inputMap2);
    inputMap2 = NULL;
  }
}

static const ProtocolOperations protocolOperations2 = {
  initializeTerminal2, releaseResources2,
  readCommand2,
  writeText2, writeStatus2, flushCells2
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
mapSwitchKey2 (int count, int *byte, int *bit, int rear, int front) {
  while (count--) {
    nextInputModule2(byte, bit);
    nextInputModule2(byte, bit);
    addInputMapping2(*byte, *bit+1, front, 0);
    addInputMapping2(*byte, *bit, rear, 0);
  }
}

static int
mapInputModules2 (void) {
  inputBytes2 = left2 + right2 + 1 + (((((left2 + right2) * 4) + ((terminal->statusCount + terminal->columns) * 2)) + 7) / 8);
  inputBits2 = inputBytes2 * 8;

  if ((inputMap2 = malloc(inputBits2 * sizeof(*inputMap2)))) {
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

    mapSwitchKey2(terminal->rightSwitches, &byte, &bit,
                  OFFS_SWITCH+SWITCH_RIGHT_REAR,
                  OFFS_SWITCH+SWITCH_RIGHT_FRONT);
    mapSwitchKey2(terminal->rightKeys, &byte, &bit,
                  OFFS_SWITCH+KEY_RIGHT_REAR,
                  OFFS_SWITCH+KEY_RIGHT_FRONT);

    {
      unsigned char column = terminal->columns;
      do {
        nextInputModule2(&byte, &bit);
        addInputMapping2(byte, bit, ROUTINGKEY, --column);
      } while (column);
    }

    mapSwitchKey2(terminal->leftKeys, &byte, &bit,
                  OFFS_SWITCH+KEY_LEFT_REAR,
                  OFFS_SWITCH+KEY_LEFT_FRONT);
    mapSwitchKey2(terminal->leftSwitches, &byte, &bit,
                  OFFS_SWITCH+SWITCH_LEFT_REAR,
                  OFFS_SWITCH+SWITCH_LEFT_FRONT);

    byte--;
    addInputMapping2(byte, 7, OFFS_EASY+EASY_L2, 0);
    addInputMapping2(byte, 6, OFFS_EASY+EASY_R2, 0);
    addInputMapping2(byte, 5, OFFS_EASY+EASY_L1, 0);
    addInputMapping2(byte, 4, OFFS_EASY+EASY_R1, 0);
    addInputMapping2(byte, 3, OFFS_EASY+EASY_D2, 0);
    addInputMapping2(byte, 2, OFFS_EASY+EASY_D1, 0);
    addInputMapping2(byte, 1, OFFS_EASY+EASY_U1, 0);
    addInputMapping2(byte, 0, OFFS_EASY+EASY_U2, 0);

    return 1;
  }
  return 0;
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
              makeOutputTable(&dots, &outputTable);
            }

            left2 = terminal->leftSwitches + terminal->leftKeys;
            right2 = terminal->rightSwitches + terminal->rightKeys;
            if (mapInputModules2()) {
              return 1;
            }
          }
        }
      }
    }
    if (errno != EAGAIN) break;

    if (++tries == io->protocol2) {
      LogPrint(LOG_WARNING, "No response from display.");
      break;
    }
  }
  return 0;
}

/*--- Driver Operations ---*/

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "Papenmeier Driver (compiled on %s at %s)", __DATE__, __TIME__);
  LogPrint(LOG_INFO, "   Copyright (C) 1998-2001 by The BRLTTY Team.");
  LogPrint(LOG_INFO, "                 August Hörandl <august.hoerandl@gmx.at>");
  LogPrint(LOG_INFO, "                 Heimo Schön <heimo.schoen@gmx.at>");
}

static int
identifyTerminal (BrailleDisplay *brl) {
  if (io->protocol1 && identifyTerminal1(brl)) return 1;
  if (io->protocol2 && identifyTerminal2(brl)) return 1;
  return 0;
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  validateYesNo(&debug_keys, "debug keys flag", parameters[PARM_DEBUGKEYS]);
  validateYesNo(&debug_reads, "debug reads flag", parameters[PARM_DEBUGREADS]);
  validateYesNo(&debug_writes, "debug writes flag", parameters[PARM_DEBUGWRITES]);

#ifdef ENABLE_PM_CONFIGURATION_FILE
  /* read the config file for individual configurations */
  LogPrint(LOG_DEBUG, "Loading configuration file.");
  read_config(brl->dataDirectory, parameters[PARM_CONFIGFILE]);
#endif /* ENABLE_PM_CONFIGURATION_FILE */

  if (isSerialDevice(&device)) {
    io = &serialOperations;

#ifdef ENABLE_USB
  } else if (isUsbDevice(&device)) {
    io = &usbOperations;
#endif /* ENABLE_USB */

  } else {
    unsupportedDevice(device);
    return 0;
  }

  speed = io->speeds;
  while (*speed != B0) {
    charactersPerSecond = baud2integer(*speed) / 10;

    if (io->openPort(parameters, device)) {
      if (identifyTerminal(brl)) {
        memset(currentText, outputTable[0], terminal->columns);
        memset(currentStatus, outputTable[0], terminal->statusCount);
        resetState();
        protocol->initializeTerminal(brl);
        return 1;
      }
      io->closePort();
    }

    ++speed;
  }

  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
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
    if (s[FirstStatusCell] == FSC_GENERIC) {
      int i;

      unsigned char values[InternalStatusCellCount];
      memcpy(values, s, sizeof(values));
      values[STAT_INPUT] = input_mode;

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
brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds) {
  return protocol->readCommand(brl, cmds);
}
