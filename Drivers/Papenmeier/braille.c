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

/*
 * papenmeier/braille.c - Braille display library for Papenmeier Screen 2D
 *
 * watch out:
 *  the file read_config.c is included - HACK, but this gives easier test handling
 *
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
 
int yyerror (char* s)  /* Called by yyparse on error */
{
  LogPrint(LOG_CRIT, "Error: line %d: %s", linenumber, s);
  exit(99);
}

static void read_file(char* name)
{
  LogPrint(LOG_DEBUG, "Opening config file: %s", name);
  if ((configfile = fopen(name, "r")) != NULL) {
    LogPrint(LOG_DEBUG, "Reading config file: %s", name);
    parse();
    fclose(configfile);
    configfile = NULL;
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

/* HACK - send all data twice - HACK */
/* see README for details */
/* #define SEND_TWICE_HACK */

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

static one_terminal* the_terminal = NULL;

static int curr_cols = -1;
static int curr_stats = -1;

static int code_status_first = -1;
static int code_status_last  = -1;
static int code_route_first = -1;
static int code_route_last  = -1;
static int code_front_first = -1;
static int code_front_last  = -1;
static int code_easy_first = -1;
static int code_easy_last  = -1;
static int code_switch_first = -1;
static int code_switch_last  = -1;

static int addr_status = -1;
static int addr_display = -1;

static unsigned int pressed_modifiers = 0;
static int saved_command = EOF;

static int input_mode = 0;
static unsigned char input_dots = 0;
 
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
resetTerminal (BrailleDisplay *brl) {
  const unsigned char sequence[] = {cSTX, 0X01, cETX};
  LogPrint(LOG_WARNING, "Resetting terminal.");
  flushTerminal(brl);
  if (writeBytes(brl, sequence, sizeof(sequence))) {
    pressed_modifiers = 0;
    saved_command = EOF;
    input_mode = 0;
    input_dots = 0;
  }
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
      writeData(brl, offset, size, buffer);
    }
  }
}

static void
initializeTable (BrailleDisplay *brl) {
  char line[curr_cols];
  char status[curr_stats];

  /* don´t use the internal table for the status cells */
  memset(status, 1, sizeof(status));
  writeData(brl, XMT_BRLWRITE+addr_status, sizeof(status), status);

  /* don´t use the internal table for the line */
  memset(line, 1, sizeof(line));
  writeData(brl, XMT_BRLWRITE+addr_display, sizeof(line), line);
}

static void
writeLine (BrailleDisplay *brl) {
  writeData(brl, addr_display, curr_cols, currentLine);
}

static void
writeStatus (BrailleDisplay *brl) {
  writeData(brl, addr_status, curr_stats, currentStatus);
}

static void
restartTerminal (BrailleDisplay *brl) {
  initializeTable(brl);
  drainBrailleOutput(brl, 0);

  writeStatus(brl);
  drainBrailleOutput(brl, 0);

  writeLine(brl);
  drainBrailleOutput(brl, 0);
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
  for (tn=0; tn<num_terminals; tn++) {
    if (pm_terminals[tn].ident == identity[2]) {
      the_terminal = &pm_terminals[tn];
      LogPrint(LOG_INFO, "%s  Size: %dx%d  HelpFile: %s", 
               the_terminal->name,
               the_terminal->x, the_terminal->y,
               the_terminal->helpfile);
      brl->x = the_terminal->x;
      brl->y = the_terminal->y;

      curr_cols = the_terminal->x;
      curr_stats = the_terminal->statcells;

      /* TODO: ?? HACK */
      BRLSYMBOL.helpFile = the_terminal->helpfile;

      /* key codes - starts at 0X300  */
      /* status keys - routing keys - step 3 */
      code_status_first = RCV_KEYROUTE;
      code_status_last  = code_status_first + 3 * (curr_stats - 1);
      code_route_first = code_status_last + 3;
      code_route_last  = code_route_first + 3 * (curr_cols - 1);
      LogPrint(LOG_DEBUG, "Keys: status=%03X-%03X routing=%03X-%03X",
               code_status_first, code_status_last,
               code_route_first, code_route_last);

      if (the_terminal->frontkeys > 0) {
        code_front_first = RCV_KEYFUNC + 3;
        code_front_last  = code_front_first + 3 * (the_terminal->frontkeys - 1);
        LogPrint(LOG_DEBUG, "Keys: front=%03X-%03X",
                 code_front_first, code_front_last);
      } else
        code_front_first = code_front_last  = -1;

      if (the_terminal->haseasybar) {
        code_easy_first = RCV_KEYFUNC + 3;
        code_easy_last  = 0X18;
        code_switch_first = 0X1B;
        code_switch_last = 0X30;
        LogPrint(LOG_DEBUG, "Keys: bar=%03X-%03X switches=%03X-%03X",
                 code_easy_first, code_easy_last,
                 code_switch_first, code_switch_last);
      } else
        code_easy_first = code_easy_last = code_switch_first = code_switch_last = -1;

      /* address of display */
      addr_status = XMT_BRLDATA;
      addr_display = addr_status + the_terminal->statcells;
      LogPrint(LOG_DEBUG, "Cells: status=%04X display=%04X",
               addr_status, addr_display);

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
/* HACK - used with serial.c */
#ifdef _SERIAL_C_
      /* HACK - used with serial.c - 2d screen */
      the_terminal = &pm_terminals[3];
      addr_status = XMT_BRLDATA;
      addr_display = addr_status + the_terminal->statcells;
      if (1) {
#else /* _SERIAL_C_ */
      if (identifyTerminal(brl)) {
#endif /* _SERIAL_C_ */
        initializeTable(brl);

        memset(currentStatus, outputTable[0], curr_stats);
        writeStatus(brl);

        memset(currentLine, outputTable[0], curr_cols);
        writeLine(brl);

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
  LogPrint(LOG_DEBUG, "Loading config file.");
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
    if (*++speed == B0) {
      speed = speeds;
      delay(1000);
    }
  }
}

static void
brl_close (BrailleDisplay *brl)
{
  if (brl_fd != -1) {
    tcsetattr(brl_fd, TCSADRAIN, &oldtio);	/* restore terminal settings */
    close(brl_fd);
    brl_fd = -1;
  }
}

static void
brl_identify (void)
{
  LogPrint(LOG_NOTICE, "Papenmeier Driver (compiled on %s at %s)", __DATE__, __TIME__);
  LogPrint(LOG_INFO, "   Copyright (C) 1998-2001 by The BRLTTY Team.");
  LogPrint(LOG_INFO, "                 August Hörandl <august.hoerandl@gmx.at>");
  LogPrint(LOG_INFO, "                 Heimo Schön <heimo.schoen@gmx.at>");
}

static void
brl_writeStatus(BrailleDisplay *brl, const unsigned char* s) {
  if (curr_stats) {
    unsigned char cells[curr_stats];
    if (s[FirstStatusCell] == FSC_GENERIC) {
      int i;

      unsigned char values[InternalStatusCellCount];
      memcpy(values, s, sizeof(values));
      values[STAT_INPUT] = input_mode;

      for (i=0; i < curr_stats; i++) {
	int code = the_terminal->statshow[i];
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
      while (i < curr_stats) {
	unsigned char dots = s[i];
	if (!dots) break;
        cells[i++] = outputTable[dots];
      }
      if (debug_writes) LogBytes("Status", s, i);
      while (i < curr_stats) cells[i++] = outputTable[0];
    }
    updateData(brl, addr_status, curr_stats, cells, currentStatus);
  }
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  int i;
  if (debug_writes) LogBytes("Window", brl->buffer, curr_cols);
  for (i=0; i<curr_cols; i++) brl->buffer[i] = outputTable[brl->buffer[i]];
  updateData(brl, addr_display, curr_cols, brl->buffer, currentLine);
}

/* ------------------------------------------------------------ */

/* found command - some actions to be done within the driver */
static int
handle_command(int cmd)
{
  if (cmd == CMD_INPUT) {
    /* translate toggle -> ON/OFF */
    cmd |= input_mode? VAL_TOGGLE_OFF: VAL_TOGGLE_ON;
  }

  switch (cmd) {
    case CMD_INPUT | VAL_TOGGLE_ON:
      input_mode = 1;
      input_dots = 0;
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

  saved_command = EOF;
  input_dots = 0;
  return cmd;
}

/*
 * Handling of Modifiers
 * command bound to modifiers only are remembered on press 
 * and send on last modifier released
 */

static void
log_modifiers(void)
{
  if (debug_keys) {
    LogPrint(LOG_DEBUG, "modifiers: %04x", pressed_modifiers);
  }
}

/* handle modifier pressed - includes dots in input mode */
static int
modifier_pressed(unsigned int bit)
{
  pressed_modifiers |= bit;
  log_modifiers();

  saved_command = EOF;
  if (input_mode && !(pressed_modifiers & ~0XFF)) {
    input_dots = pressed_modifiers;
    if (debug_keys) {
      LogPrint(LOG_DEBUG, "input dots: %02x", input_dots); 
    }
  } else {
    int i;
    input_dots = 0;
    for (i=0; i<CMDMAX; i++)
      if ((the_terminal->cmds[i].modifiers == pressed_modifiers) &&
	  (the_terminal->cmds[i].keycode == NOKEY)) {
	saved_command = the_terminal->cmds[i].code;
	LogPrint(LOG_DEBUG, "saving cmd: %d", saved_command); 
	break;
      }
  }
  return CMD_NOOP;
}
 
/* handle modifier release */
static int
modifier_released(unsigned int bit)
{
  pressed_modifiers &= ~bit;
  log_modifiers();

  if (saved_command != EOF) {
    if (debug_keys) {
      LogPrint(LOG_DEBUG, "saved cmd: %d", saved_command); 
    }
    return handle_command(saved_command);
  }

  if (input_mode && (input_dots != 0)) {
    int cmd = VAL_PASSDOTS;
    static unsigned char mod_to_dot[] = {B1, B2, B3, B4, B5, B6, B7, B8};
    unsigned char *dot = mod_to_dot;
    int mod;
    for (mod=1; mod<0X100; ++dot, mod<<=1)
      if (input_dots & mod)
        cmd |= *dot;
    if (debug_keys) {
      LogPrint(LOG_DEBUG, "dots=%02X cmd=%04X", input_dots, cmd); 
    }
    return handle_command(cmd);
  }

  return EOF;
}

/* one key is pressed or released */
static int
handle_key (int code, int ispressed, int offsroute)
{
  int i;
  int cmd;

  /* look for modfier keys */
  for (i=0; i<MODMAX; i++) 
    if (the_terminal->modifiers[i] == code) {
      /* found modifier: update bitfield */
      unsigned int bit = 1 << i;
      return ispressed? modifier_pressed(bit): modifier_released(bit);
    }

  /* must be a "normal key" - search for cmd on keypress */
  if (!ispressed) return EOF;
  input_dots = 0;
  for (i=0; i<CMDMAX; i++)
    if ((the_terminal->cmds[i].keycode == code) &&
	(the_terminal->cmds[i].modifiers == pressed_modifiers)) {
      if (debug_keys)
        LogPrint(LOG_DEBUG, "cmd: %d->%d (+%d)", 
                 code, the_terminal->cmds[i].code, offsroute); 
      return handle_command(the_terminal->cmds[i].code + offsroute);
    }

  /* no command found */
  LogPrint(LOG_DEBUG, "cmd: %d[%04x] ??", code, pressed_modifiers); 
  return CMD_NOOP;
}

static int
handle_code (int code, int press, int time) {
  /* which key -> translate to OFFS_* + number */
  /* attn: number starts with 1 */
  int num;

  if (code_front_first <= code && 
      code <= code_front_last) { /* front key */
    num = 1 + (code - code_front_first) / 3;
    return handle_key(OFFS_FRONT + num, press, 0);
  }

  if (code_status_first <= code && 
      code <= code_status_last) { /* status key */
    num = 1+ (code - code_status_first) / 3;
    return handle_key(OFFS_STAT + num, press, 0);
  }

  if (code_easy_first <= code && 
      code <= code_easy_last) { /* easy bar */
    num = 1 + (code - code_easy_first) / 3;
    return handle_key(OFFS_EASY + num, press, 0);
  }

  if (code_switch_first <= code && 
      code <= code_switch_last) { /* easy bar */
    num = 1 + (code - code_switch_first) / 3;
    return handle_key(OFFS_SWITCH + num, press, 0);
  }

  if (code_route_first <= code && 
      code <= code_route_last) { /* Routing Keys */ 
    num = (code - code_route_first) / 3;
    return handle_key(ROUTINGKEY, press, num);
  }

  LogPrint(LOG_WARNING, "Unexpected key: %04X", code);
  return press? CMD_NOOP: EOF;
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
          int command = handle_code(((buf[2] << 8) | buf[3]),
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
