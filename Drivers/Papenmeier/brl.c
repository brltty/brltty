/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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
 * papenmeier/brl.c - Braille display library for Papenmeier Screen 2D
 *
 * watch out:
 *  the file read_config.c is included - HACK, but this gives easier test handling
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>

#include "brlconf.h"
#include "../brl.h"
#include "../scr.h"
#include "../misc.h"

typedef enum {
   PARM_CONFIGFILE,
   PARM_DEBUGKEYS,
   PARM_DEBUGREADS,
   PARM_DEBUGWRITES
} DriverParameter;
#define BRLPARMS "configfile", "debugkeys", "debugreads", "debugwrites"
#include "../brl_driver.h"

#ifdef READ_CONFIG
#include "config.tab.c"
static void read_config(char *path);
int yyerror (char* s)  /* Called by yyparse on error */
{
  LogPrint(LOG_CRIT, "Error: line %d: %s", linenumber, s);
  exit(99);
}
#else
#include "brl-cfg.h"
#endif

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
static unsigned char change_bits[] = {
  0x00, 0x01, 0x08, 0x09, 0x02, 0x03, 0x0a, 0x0b,
  0x10, 0x11, 0x18, 0x19, 0x12, 0x13, 0x1a, 0x1b,
  0x04, 0x05, 0x0c, 0x0d, 0x06, 0x07, 0x0e, 0x0f,
  0x14, 0x15, 0x1c, 0x1d, 0x16, 0x17, 0x1e, 0x1f,
  0x20, 0x21, 0x28, 0x29, 0x22, 0x23, 0x2a, 0x2b,
  0x30, 0x31, 0x38, 0x39, 0x32, 0x33, 0x3a, 0x3b,
  0x24, 0x25, 0x2c, 0x2d, 0x26, 0x27, 0x2e, 0x2f,
  0x34, 0x35, 0x3c, 0x3d, 0x36, 0x37, 0x3e, 0x3f,
  0x40, 0x41, 0x48, 0x49, 0x42, 0x43, 0x4a, 0x4b,
  0x50, 0x51, 0x58, 0x59, 0x52, 0x53, 0x5a, 0x5b,
  0x44, 0x45, 0x4c, 0x4d, 0x46, 0x47, 0x4e, 0x4f,
  0x54, 0x55, 0x5c, 0x5d, 0x56, 0x57, 0x5e, 0x5f,
  0x60, 0x61, 0x68, 0x69, 0x62, 0x63, 0x6a, 0x6b,
  0x70, 0x71, 0x78, 0x79, 0x72, 0x73, 0x7a, 0x7b,
  0x64, 0x65, 0x6c, 0x6d, 0x66, 0x67, 0x6e, 0x6f,
  0x74, 0x75, 0x7c, 0x7d, 0x76, 0x77, 0x7e, 0x7f,
  0x80, 0x81, 0x88, 0x89, 0x82, 0x83, 0x8a, 0x8b,
  0x90, 0x91, 0x98, 0x99, 0x92, 0x93, 0x9a, 0x9b,
  0x84, 0x85, 0x8c, 0x8d, 0x86, 0x87, 0x8e, 0x8f,
  0x94, 0x95, 0x9c, 0x9d, 0x96, 0x97, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa8, 0xa9, 0xa2, 0xa3, 0xaa, 0xab,
  0xb0, 0xb1, 0xb8, 0xb9, 0xb2, 0xb3, 0xba, 0xbb,
  0xa4, 0xa5, 0xac, 0xad, 0xa6, 0xa7, 0xae, 0xaf,
  0xb4, 0xb5, 0xbc, 0xbd, 0xb6, 0xb7, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc8, 0xc9, 0xc2, 0xc3, 0xca, 0xcb,
  0xd0, 0xd1, 0xd8, 0xd9, 0xd2, 0xd3, 0xda, 0xdb,
  0xc4, 0xc5, 0xcc, 0xcd, 0xc6, 0xc7, 0xce, 0xcf,
  0xd4, 0xd5, 0xdc, 0xdd, 0xd6, 0xd7, 0xde, 0xdf,
  0xe0, 0xe1, 0xe8, 0xe9, 0xe2, 0xe3, 0xea, 0xeb,
  0xf0, 0xf1, 0xf8, 0xf9, 0xf2, 0xf3, 0xfa, 0xfb,
  0xe4, 0xe5, 0xec, 0xed, 0xe6, 0xe7, 0xee, 0xef,
  0xf4, 0xf5, 0xfc, 0xfd, 0xf6, 0xf7, 0xfe, 0xff
};

static int brl_fd = -1;			/* file descriptor for Braille display */
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

static unsigned char pressed_modifiers = 0;
static int saved_command = EOF;

static int input_mode = 0;
static unsigned char input_dots = 0;
 
static void
flushInput (void) {
  tcflush(brl_fd, TCIFLUSH);
}

static void
flushOutput (void) {
  tcflush(brl_fd, TCOFLUSH);
}

static int
writeBytes (const unsigned char *bytes, int count) {
  if (safe_write(brl_fd, bytes, count) != -1) return 1;
  LogError("Write");
  return 0;
}

static void
resetTerminal (void) {
  const unsigned char sequence[] = {cSTX, 0X01, cETX};
  LogPrint(LOG_WARNING, "Resetting terminal.");
  flushOutput();
  delay(500);
  flushInput();
  if (writeBytes(sequence, sizeof(sequence))) {
    pressed_modifiers = 0;
    saved_command = EOF;
    input_mode = 0;
    input_dots = 0;
  }
}

#define RBF_ETX 1
#define RBF_RESET 2
static int
readBytes (unsigned char *buffer, int offset, int count, int flags) {
  if (readChunk(brl_fd, buffer, &offset, count, 100)) {
    if (!(flags & RBF_ETX)) return 1;
    if (*(buffer+offset-1) == cETX) return 1;
    LogPrint(LOG_WARNING, "Input packet not terminated by ETX.");
  }
  if ((offset > 0) && (flags & RBF_RESET)) resetTerminal();
  return 0;
}

static int
writeData (int offset, int size, const unsigned char *data) {
  unsigned char header[] = {
    cSTX,
    cIdSend,
    0,
    0,
    0,
    0,
  };
  unsigned char trailer[] = {cETX};
  header[2] = offset >> 8;
  header[3] = offset & 0XFF;
  header[5] = (sizeof(header) + size + sizeof(trailer)) & 0XFF; 
  
  if (!writeBytes(header, sizeof(header))) return 0;
  if (!writeBytes(data, size)) return 0;
  if (!writeBytes(trailer, sizeof(trailer))) return 0;
#ifdef SEND_TWICE_HACK
  delay(100);
  if (!writeBytes(header, sizeof(header))) return 0;
  if (!writeBytes(data, size)) return 0;
  if (!writeBytes(trailer, sizeof(trailer))) return 0;
#endif
  return 1;
}

static void
updateData (int offset, int size, const unsigned char *data, unsigned char *buffer) {
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
      writeData(offset, size, buffer);
    }
  }
}

static void
initializeTable (void) {
  char line[curr_cols];
  char status[curr_stats];

  // don´t use the internal table for the status cells 
  memset(status, 1, sizeof(status));
  writeData(XMT_BRLWRITE+addr_status, sizeof(status), status);

  // don´t use the internal table for the line
  memset(line, 1, sizeof(line));
  writeData(XMT_BRLWRITE+addr_display, sizeof(line), line);
}

static void
writeLine (void) {
  writeData(addr_display, curr_cols, currentLine);
}

static void
writeStatus (void) {
  if (curr_stats > 0) writeData(addr_status, curr_stats, currentStatus);
}

/* ------------------------------------------------------------ */

static int
identifyTerminal(brldim *brl) {
  static const unsigned char badPacket[] = { 
    cSTX,
    cIdSend,
    0, 0,			/* position */
    0, 0,			/* wrong number of bytes */
    cETX
  };

  flushOutput();
  delay(100);
  flushInput();

  if (writeBytes(badPacket, sizeof(badPacket))) {
    if (awaitInput(brl_fd, 1000)) {
      unsigned char identity[10];			/* answer has 10 chars */
      if (readBytes(identity, 0, 1, 0)) {
        if (identity[0] == cSTX) {
          if (readBytes(identity, 1, sizeof(identity)-1, RBF_ETX)) {
            if (identity[1] == cIdIdentify) {
              int tn;
              LogBytes("Identification packet", identity, sizeof(identity));
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

                  // TODO: ?? HACK
                  brl_driver.helpFile = the_terminal->helpfile;

                  // key codes - starts at 0X300 
                  // status keys - routing keys - step 3
                  code_status_first = RCV_KEYROUTE;
                  code_status_last  = code_status_first + 3 * (curr_stats - 1);
                  code_route_first = code_status_last + 3;
                  code_route_last  = code_route_first + 3 * (curr_cols - 1);

                  if (the_terminal->frontkeys > 0) {
                    code_front_first = RCV_KEYFUNC + 3;
                    code_front_last  = code_front_first + 3 * (the_terminal->frontkeys - 1);
                  } else
                    code_front_first = code_front_last  = -1;

                  if (the_terminal->haseasybar) {
                    code_easy_first = RCV_KEYFUNC + 3;
                    code_easy_last  = 0X18;
                    code_switch_first = 0X1B;
                    code_switch_last = 0X30;
                  } else
                    code_easy_first = code_easy_last = code_switch_first = code_switch_last = -1;

                  LogPrint(LOG_DEBUG, "s=%03X-%03X r=%03X-%03X f=%03X-%03X e=%03X-%03X sw=%03X-%03X",
                           code_status_first, code_status_last,
                           code_route_first, code_route_last,
                           code_front_first, code_front_last,
                           code_easy_first, code_easy_last,
                           code_switch_first, code_switch_last);

                  // address of display
                  addr_status = XMT_BRLDATA;
                  addr_display = addr_status + the_terminal->statcells;
                  LogPrint(LOG_DEBUG, "addr: s=%d d=%d",
                           addr_status, addr_display);

                  return 1;
                }
              }
              LogPrint(LOG_WARNING, "Unknown Papenmeier ID: %d", identity[2]);
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
initializeDisplay (brldim *brl, const char *dev, speed_t baud) {
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
      brldim res;
      res.x = BRLCOLSMAX;		/* initialise size of display - unknownown yet */
      res.y = 1;
      res.disp = NULL;		/* clear pointers */

/* HACK - used with serial.c */
#ifdef _SERIAL_C_
      /* HACK - used with serial.c - 2d screen */
      the_terminal = &pm_terminals[3];
      addr_status = XMT_BRLDATA;
      addr_display = addr_status + the_terminal->statcells;
      if (1) {
#else
      if (identifyTerminal(&res)) {
#endif
        if ((res.disp = (unsigned char *)malloc(res.x * res.y))) {
          initializeTable();

          memset(currentStatus, change_bits[0], curr_stats);
          writeStatus();

          memset(currentLine, change_bits[0], curr_cols);
          writeLine();

          *brl = res;
          return 1;
        }
      }
    }
    close(brl_fd);
    brl_fd = -1;
  }
  return 0;
}

static void
brl_initialize (char **parameters, brldim *brl, const char *dev) {
  validateYesNo(&debug_keys, "debug keys flag", parameters[PARM_DEBUGKEYS]);
  validateYesNo(&debug_reads, "debug reads flag", parameters[PARM_DEBUGREADS]);
  validateYesNo(&debug_writes, "debug writes flag", parameters[PARM_DEBUGWRITES]);

  /* read the config file for individual configurations */
#ifdef READ_CONFIG
  LogPrint(LOG_DEBUG, "Loading config file.");
  read_config(parameters[PARM_CONFIGFILE]);
#endif

  while (1) {
    const static speed_t speeds[] = {B19200, B38400, B0};
    const static speed_t *speed = speeds;
    if (initializeDisplay(brl, dev, *speed)) break;
    brl_close(brl);
    if (*++speed == B0) {
      speed = speeds;
      delay(1000);
    }
  }
}

static void
brl_close (brldim *brl)
{
  if (brl->disp != NULL) {
    free(brl->disp);
    brl->disp = NULL;
  }
  brl->x = brl->y = -1;

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
brl_writeStatus(const unsigned char* s) {
  if (debug_writes)
    LogPrint(LOG_DEBUG, "setbrlstat %d", curr_stats);

  if (curr_stats) {
    unsigned char cells[curr_stats];
    if (s[FirstStatusCell] == FSC_GENERIC) {
      int i;

      unsigned char values[InternalStatusCellCount];
      memcpy(values, s, sizeof(values));
      values[STAT_INPUT] = input_mode;

      for (i=0; i < curr_stats; i++) {
	int code = the_terminal->statshow[i];
	if (code == STAT_EMPTY)
	  cells[i] = 0;
	else if (code >= OFFS_NUMBER)
	  cells[i] = change_bits[portraitNumber(values[code-OFFS_NUMBER])];
	else if (code >= OFFS_FLAG)
	  cells[i] = change_bits[seascapeFlag(i+1, values[code-OFFS_FLAG])];
	else if (code >= OFFS_HORIZ)
	  cells[i] = change_bits[seascapeNumber(values[code-OFFS_HORIZ])];
	else
	  cells[i] = change_bits[values[code]];
      }
    } else {
      int i = 0;
      while (i < curr_stats) {
	unsigned char dots = s[i];
	if (!dots) break;
        cells[i++] = change_bits[dots];
      }
      while (i < curr_stats)
        cells[i++] = change_bits[0];
    }
    updateData(addr_status, curr_stats, cells, currentStatus);
  }
}

static void
brl_writeWindow (brldim *brl) {
  int i;
  if (debug_writes) LogBytes("write", brl->disp, curr_cols);
  for (i=0; i<curr_cols; i++) brl->disp[i] = change_bits[brl->disp[i]];
  updateData(addr_display, curr_cols, brl->disp, currentLine);
}

/* ------------------------------------------------------------ */

/* found command - some actions to be done within the driver */
static int
handle_command(int cmd)
{
  if (cmd == CMD_INPUT) {
    // translate toggle -> ON/OFF
    cmd |= input_mode? VAL_SWITCHOFF: VAL_SWITCHON;
  }

  switch (cmd) {
    case CMD_INPUT | VAL_SWITCHON:
      input_mode = 1;
      input_dots = 0;
      cmd = VAL_SWITCHON;
      if (debug_keys) {
        LogPrint(LOG_DEBUG, "input mode on"); 
      }
      break;
    case CMD_INPUT | VAL_SWITCHOFF:
      input_mode = 0;
      cmd = VAL_SWITCHOFF;
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
modifier_pressed(int bit)
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
modifier_released(int bit)
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
      int bit = 1 << i;
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

#define READ(offset,count,flags) { if (!readBytes(buf, offset, count, RBF_RESET|(flags))) return EOF; }
static int 
brl_read (DriverCommandContext cmds) {
  while (1) {
    unsigned char buf[0X100];

    do {
      READ(0, 1, 0);
    } while (buf[0] != cSTX);
    if (debug_reads) LogPrint(LOG_DEBUG,  "read: STX");

    READ(1, 1, 0);
    switch (buf[1]) {
      default: {
        int i;
        LogPrint(LOG_WARNING, "unknown packet: %02X", buf[1]);
        for (i=2; i<=sizeof(buf); i++) {
          READ(i, 1, 0);
          LogPrint(LOG_WARNING, "packet byte %2d: %02X", i, buf[i]);
        }
      }
      case cIdIdentify: {
        const int length = 10;
        READ(2, length-2, RBF_ETX);
        LogBytes("identify", buf, length);
        initializeTable();
        delay(100);
        writeStatus();
        delay(100);
        writeLine();
        break;
      }
      case cIdReceive: {
        int length;
        int i;

        READ(2, 4, 0);
        length = (buf[4] << 8) | buf[5];	/* packet size */
        if (length != 10) {
          LogPrint(LOG_WARNING, "Unexpected input packet length: %d", length);
          resetTerminal();
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
        initializeTable();
        writeStatus();
        writeLine();
        continue;
      }
    }
  }
}
#undef READ


#ifdef READ_CONFIG

/* ------------------------------------------------------------ */
/* read config */

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

static void read_config(char *name)
{
  if (!*name) {
    if (!(name = getenv(PM_CONFIG_ENV))) {
      name = PM_CONFIG_FILE;
    }
  }
  LogPrint(LOG_INFO, "Papenmeier Configuration File: %s", name);
  read_file(name);
}

#endif

