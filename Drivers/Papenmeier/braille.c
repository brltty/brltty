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
#include "Programs/serial.h"

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

/*
 * change bits for the papenmeier terminal
 *                             1 2           1 4
 * dot number -> bit number    3 4   we get  2 5 
 *                             5 6           3 6
 *                             7 8           7 8
 */
static TranslationTable outputTable;

static int brl_fd = -1;			/* file descriptor for Braille display */
static int chars_per_sec;			/* file descriptor for Braille display */
static unsigned int debug_keys = 0;
static unsigned int debug_reads = 0;
static unsigned int debug_writes = 0;
static struct termios oldtio;		/* old terminal settings */

static unsigned char currentStatus[PMSC];
static unsigned char currentLine[BRLCOLSMAX];

/* ------------------------------------------------------------ */

static const TerminalDefinition *the_terminal = NULL;

static int code_status_first = -1;
static int code_status_last  = -1;
static int code_cursor_first = -1;
static int code_cursor_last  = -1;
static int code_front_first = -1;
static int code_front_last  = -1;
static int code_easy_first = -1;
static int code_easy_last  = -1;
static int code_switch_first = -1;
static int code_switch_last  = -1;

static int addr_status = -1;
static int addr_text = -1;

static unsigned int pressed_modifiers = 0;
static unsigned int saved_modifiers = 0;
static int input_mode = 0;
 
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
  qsort(the_terminal->commands, the_terminal->commandCount, sizeof(*the_terminal->commands), compareCommands);
}

static int
findCommand (int *command, int key, int modifiers) {
  int first = 0;
  int last = the_terminal->commandCount - 1;
  CommandDefinition ref;
  ref.key = key;
  ref.modifiers = modifiers;
  while (first <= last) {
    int current = (first + last) / 2;
    CommandDefinition *cmd = &the_terminal->commands[current];
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
flushTerminal (BrailleDisplay *brl) {
  tcflush(brl_fd, TCOFLUSH);
  drainBrailleOutput(brl, 100);
  tcflush(brl_fd, TCIFLUSH);
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, int count) {
  if (debug_writes) LogBytes("Write", bytes, count);
  if (safe_write(brl_fd, bytes, count) != -1) return 1;
  LogError("Write");
  return 0;
}

static void
resetState (void) {
  pressed_modifiers = 0;
  saved_modifiers = 0;
  input_mode = 0;
}

static void
resetTerminal (BrailleDisplay *brl) {
  static const unsigned char sequence[] = {cSTX, 0X01, cETX};
  LogPrint(LOG_WARNING, "Resetting terminal.");
  flushTerminal(brl);
  if (writeBytes(brl, sequence, sizeof(sequence))) resetState();
}

static int
writeData (BrailleDisplay *brl, int offset, int count, const unsigned char *data) {
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

    header[2] = offset >> 8;
    header[3] = offset & 0XFF;
    header[4] = size >> 8;
    header[5] = size & 0XFF;
    memcpy(&buffer[index], header, sizeof(header));
    index += sizeof(header);

    memcpy(&buffer[index], data, count);
    index += count;
    
    memcpy(&buffer[index], trailer, sizeof(trailer));
    index += sizeof(trailer);
    
    brl->writeDelay += count * 1000 / chars_per_sec;
    if (!writeBytes(brl, buffer, index)) return 0;
  }
  return 1;
}

static void
updateData (BrailleDisplay *brl, int offset, int size, const unsigned char *data, unsigned char *buffer) {
  if (memcmp(buffer, data, size) != 0) {
    int index;
    while (size) {
      index = size - 1;
      if (buffer[index] != data[index]) break;
      size = index;
    }
    for (index=0; index<size; ++index) {
      if (buffer[index] != data[index]) break;
    }
    if ((size -= index)) {
      buffer += index;
      data += index;
      offset += index;
      memcpy(buffer, data, size);
      writeData(brl, XMT_BRLDATA+offset, size, buffer);
    }
  }
}

static int
disableOutputTranslation (BrailleDisplay *brl, int offset, int count) {
  unsigned char buffer[count];
  memset(buffer, 1, sizeof(buffer));
  return writeData(brl, XMT_BRLWRITE+offset,
                   sizeof(buffer), buffer);
}

static void
initializeTable (BrailleDisplay *brl) {
  disableOutputTranslation(brl, addr_status, the_terminal->statusCount);
  disableOutputTranslation(brl, addr_text, the_terminal->columns);
}

static void
writeLine (BrailleDisplay *brl) {
  writeData(brl, XMT_BRLDATA+addr_text, the_terminal->columns, currentLine);
}

static void
writeStatus (BrailleDisplay *brl) {
  writeData(brl, XMT_BRLDATA+addr_status, the_terminal->statusCount, currentStatus);
}

static void
restartTerminal (BrailleDisplay *brl) {
  initializeTable(brl);
  drainBrailleOutput(brl, 0);

  writeStatus(brl);
  drainBrailleOutput(brl, 0);

  writeLine(brl);
  drainBrailleOutput(brl, 0);

  resetState();
}

/* ------------------------------------------------------------ */

#define RBF_ETX 1
#define RBF_RESET 2
static int
readBytes (BrailleDisplay *brl, unsigned char *buffer, int offset, int count, int flags) {
  if (readChunk(brl_fd, buffer, &offset, count, 1000)) {
    if (!(flags & RBF_ETX)) return 1;
    if (*(buffer+offset-1) == cETX) return 1;
    LogPrint(LOG_WARNING, "Input packet not terminated by ETX.");
  }
  if ((offset > 0) && (flags & RBF_RESET)) resetTerminal(brl);
  return 0;
}

static int
interpretIdentity (const unsigned char *identity, BrailleDisplay *brl) {
  int tn;
  LogBytes("Identity", identity, IDENTITY_LENGTH);
  LogPrint(LOG_INFO, "Papenmeier ID: %d  Version: %d.%d%d (%02X%02X%02X)", 
           identity[2],
           identity[3], identity[4], identity[5],
           identity[6], identity[7], identity[8]);
  for (tn=0; tn<pmTerminalCount; tn++) {
    if (pmTerminals[tn].identifier == identity[2]) {
      the_terminal = &pmTerminals[tn];
      LogPrint(LOG_INFO, "%s  Size: %dx%d  HelpFile: %s", 
               the_terminal->name,
               the_terminal->columns, the_terminal->rows,
               the_terminal->helpFile);
      brl->x = the_terminal->columns;
      brl->y = the_terminal->rows;

      /* TODO: ?? HACK */
      BRLSYMBOL.helpFile = the_terminal->helpFile;

      /* routing key codes: 0X300 -> status -> cursor */
      code_status_first = RCV_KEYROUTE;
      code_status_last  = code_status_first + 3 * (the_terminal->statusCount - 1);
      code_cursor_first = code_status_last + 3;
      code_cursor_last  = code_cursor_first + 3 * (the_terminal->columns - 1);
      LogPrint(LOG_DEBUG, "Routing Keys: status=%03X-%03X cursor=%03X-%03X",
               code_status_first, code_status_last,
               code_cursor_first, code_cursor_last);

      /* function key codes: 0X000 -> front -> bar -> switches */
      code_front_first = RCV_KEYFUNC + 3;
      code_front_last  = code_front_first + 3 * (the_terminal->frontKeys - 1);
      code_easy_first = code_front_last + 3;
      code_easy_last  = code_easy_first + 3 * ((the_terminal->hasEasyBar? 8: 0) - 1);
      code_switch_first = code_easy_last + 3;
      code_switch_last  = code_switch_first + 3 * ((the_terminal->hasEasyBar? 8: 0) - 1);
      LogPrint(LOG_DEBUG, "Function Keys: front=%03X-%03X bar=%03X-%03X switches=%03X-%03X",
               code_front_first, code_front_last,
               code_easy_first, code_easy_last,
               code_switch_first, code_switch_last);

      /* cell block offsets: 0X00 -> status -> text */
      addr_status = 0;
      addr_text = addr_status + the_terminal->statusCount;
      LogPrint(LOG_DEBUG, "Cell Block Offsets: status=%02X text=%02X",
               addr_status, addr_text);

      sortCommands();
      return 1;
    }
  }
  LogPrint(LOG_WARNING, "Unknown Papenmeier ID: %d", identity[2]);
  return 0;
}

static int
identifyTerminal(BrailleDisplay *brl) {
  static const unsigned char badPacket[] = { 
    cSTX,
    cIdSend,
    0, 0,			/* position */
    0, 0,			/* wrong number of bytes */
    cETX
  };

  flushTerminal(brl);
  if (writeBytes(brl, badPacket, sizeof(badPacket))) {
    if (awaitInput(brl_fd, 1000)) {
      unsigned char identity[IDENTITY_LENGTH];			/* answer has 10 chars */
      if (readBytes(brl, identity, 0, 1, 0)) {
        if (identity[0] == cSTX) {
          if (readBytes(brl, identity, 1, sizeof(identity)-1, RBF_ETX)) {
            if (identity[1] == cIdIdentify) {
              if (interpretIdentity(identity, brl)) return 1;
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

/* ------------------------------------------------------------ */

static int 
initializeDisplay (BrailleDisplay *brl, const char *dev, speed_t baud) {
  if (openSerialDevice(dev, &brl_fd, &oldtio)) {
    struct termios newtio;	/* new terminal settings */
    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = CRTSCTS | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;		/* raw output */
    newtio.c_lflag = 0;		/* don't echo or generate signals */
    newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
    newtio.c_cc[VTIME] = 0;
    LogPrint(LOG_DEBUG, "Trying %d baud.", baud2integer(baud));
    if (resetSerialDevice(brl_fd, &newtio, baud)) {
      chars_per_sec = baud2integer(baud) / 10;
      if (identifyTerminal(brl)) {
        initializeTable(brl);

        memset(currentStatus, outputTable[0], the_terminal->statusCount);
        writeStatus(brl);

        memset(currentLine, outputTable[0], the_terminal->columns);
        writeLine(brl);

        resetState();
        return 1;
      }
    }
    close(brl_fd);
    brl_fd = -1;
  }
  return 0;
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  validateYesNo(&debug_keys, "debug keys flag", parameters[PARM_DEBUGKEYS]);
  validateYesNo(&debug_reads, "debug reads flag", parameters[PARM_DEBUGREADS]);
  validateYesNo(&debug_writes, "debug writes flag", parameters[PARM_DEBUGWRITES]);

  /* read the config file for individual configurations */
#ifdef ENABLE_PM_CONFIGURATION_FILE
  LogPrint(LOG_DEBUG, "Loading configuration file.");
  read_config(brl->dataDirectory, parameters[PARM_CONFIGFILE]);
#endif /* ENABLE_PM_CONFIGURATION_FILE */

  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(&dots, &outputTable);
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  while (1) {
    const static speed_t speeds[] = {B19200, B38400, B0};
    const static speed_t *speed = speeds;
    if (initializeDisplay(brl, device, *speed)) return 1;
    brl_close(brl);
    if (*++speed == B0) return 0;
  }
}

static void
brl_close (BrailleDisplay *brl) {
  if (brl_fd != -1) {
    tcsetattr(brl_fd, TCSADRAIN, &oldtio);	/* restore terminal settings */
    close(brl_fd);
    brl_fd = -1;
  }
}

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "Papenmeier Driver (compiled on %s at %s)", __DATE__, __TIME__);
  LogPrint(LOG_INFO, "   Copyright (C) 1998-2001 by The BRLTTY Team.");
  LogPrint(LOG_INFO, "                 August Hörandl <august.hoerandl@gmx.at>");
  LogPrint(LOG_INFO, "                 Heimo Schön <heimo.schoen@gmx.at>");
}

static void
brl_writeStatus(BrailleDisplay *brl, const unsigned char* s) {
  if (the_terminal->statusCount) {
    unsigned char cells[the_terminal->statusCount];
    if (s[FirstStatusCell] == FSC_GENERIC) {
      int i;

      unsigned char values[InternalStatusCellCount];
      memcpy(values, s, sizeof(values));
      values[STAT_INPUT] = input_mode;

      for (i=0; i<the_terminal->statusCount; i++) {
	int code = the_terminal->statusCells[i];
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
      if (debug_writes) LogBytes("Status", s, InternalStatusCellCount);
    } else {
      int i = 0;
      while (i < the_terminal->statusCount) {
	unsigned char dots = s[i];
	if (!dots) break;
        cells[i++] = outputTable[dots];
      }
      if (debug_writes) LogBytes("Status", s, i);
      while (i < the_terminal->statusCount) cells[i++] = outputTable[0];
    }
    updateData(brl, addr_status, the_terminal->statusCount, cells, currentStatus);
  }
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  int i;
  if (debug_writes) LogBytes("Window", brl->buffer, the_terminal->columns);
  for (i=0; i<the_terminal->columns; i++) brl->buffer[i] = outputTable[brl->buffer[i]];
  updateData(brl, addr_text, the_terminal->columns, brl->buffer, currentLine);
}

/* ------------------------------------------------------------ */

/* found command - some actions to be done within the driver */
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

/* one key is pressed or released */
static int
handleKey (int code, int press, int offsroute) {
  int i;
  int cmd;

  /* look for modfier keys */
  for (i=0; i<the_terminal->modifierCount; i++)
    if (the_terminal->modifiers[i] == code)
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

static int
handleCode (int code, int press, int time) {
  /* which key -> translate to OFFS_* + number */
  /* attn: number starts with 1 */
  int num;

  if (code_front_first <= code && 
      code <= code_front_last) { /* front key */
    num = 1 + (code - code_front_first) / 3;
    return handleKey(OFFS_FRONT + num, press, 0);
  }

  if (code_status_first <= code && 
      code <= code_status_last) { /* status key */
    num = 1+ (code - code_status_first) / 3;
    return handleKey(OFFS_STAT + num, press, 0);
  }

  if (code_easy_first <= code && 
      code <= code_easy_last) { /* easy bar */
    num = 1 + (code - code_easy_first) / 3;
    return handleKey(OFFS_EASY + num, press, 0);
  }

  if (code_switch_first <= code && 
      code <= code_switch_last) { /* easy bar */
    num = 1 + (code - code_switch_first) / 3;
    return handleKey(OFFS_SWITCH + num, press, 0);
  }

  if (code_cursor_first <= code && 
      code <= code_cursor_last) { /* Routing Keys */ 
    num = (code - code_cursor_first) / 3;
    return handleKey(ROUTINGKEY, press, num);
  }

  LogPrint(LOG_WARNING, "Unexpected key: %04X", code);
  return CMD_NOOP;
}

/* ------------------------------------------------------------ */

#define READ(offset,count,flags) { if (!readBytes(brl, buf, offset, count, RBF_RESET|(flags))) return EOF; }
static int 
brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds) {
  while (1) {
    unsigned char buf[0X100];

    do {
      READ(0, 1, 0);
    } while (buf[0] != cSTX);
    if (debug_reads) LogPrint(LOG_DEBUG, "read: STX");

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
        if (interpretIdentity(buf, brl)) brl->resizeRequired = 1;
        delay(200);
        restartTerminal(brl);
        break;
      }

      case cIdReceive: {
        int length;
        int i;

        READ(2, 4, 0);
        length = (buf[4] << 8) | buf[5];	/* packet size */
        if (length != 10) {
          LogPrint(LOG_WARNING, "Unexpected input packet length: %d", length);
          resetTerminal(brl);
          return CMD_ERR;
        }
        READ(6, length-6, RBF_ETX);			/* Data */
        if (debug_reads) LogBytes("read", buf, length);

        {
          int command = handleCode(((buf[2] << 8) | buf[3]),
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
        restartTerminal(brl);
        break;
      }
    }
  }
}
#undef READ
