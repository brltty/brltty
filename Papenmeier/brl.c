/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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
 *     Vienna, Austria)
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#include "brlconf.h"
#include "../brl.h"
#include "../scr.h"
#include "../misc.h"
#include "../brl_driver.h"

#ifdef READ_CONFIG
#include "read_config.c"
static void read_config();
#else
#include "brl-cfg.h"
#endif

static void init_table();

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

static int brl_fd = 0;			/* file descriptor for Braille display */
static struct termios oldtio;		/* old terminal settings */

static unsigned char prevline[PMSC] = "";
static unsigned char prev[BRLCOLSMAX+1]= "";

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

/* ------------------------------------------------------------ */

//static char thehelpfilename[] = "brltty-pm@.hlp";

static void
identify_terminal(brldim *brl)
{
  char badline[] = { 
    cSTX,
    cIdSend,
    0, 0,			/* position */
    0, 0,			/* wrong number of bytes */
    cETX
  };

  unsigned char buf[10];			/* answer has 10 chars */
  int tries = 5;
  int len, tn;

  for( ; tries > 0; tries --) {
    LogPrint(LOG_DEBUG,"starting auto indentify - %d", tries);
    //    tcflush (brl_fd, TCIFLUSH);	/* clean line */
    write(brl_fd, badline, sizeof(badline));
    delay(100);
    len = read(brl_fd,buf,sizeof(buf));
    if (len != sizeof(buf)) {	/* bad len */
      delay(100);
      LogPrint(LOG_ERR,"len(%d) != size (%d)", len, sizeof(buf));
      continue;			/* try again */
    }
    LogPrint(LOG_INFO, "Papenmeier ID: %d  Version: %d.%d%d (%02X%02X%02X)", 
	     buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);
    for(tn=0; tn < num_terminals; tn++)
      if (pm_terminals[tn].ident == buf[2])
	{
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
	  braille->help_file = the_terminal->helpfile;

	  // key codes - starts at 0x300 
	  // status keys - routing keys - step 3
	  code_status_first = 0x300;
	  code_status_last  = code_status_first + 3 * (curr_stats - 1);
	  code_route_first = code_status_last + 3;
	  code_route_last  = code_route_first + 3 * (curr_cols - 1);

	  if (the_terminal->frontkeys > 0) {
	    code_front_first = 0x03;
	    code_front_last  = code_front_first + 
	      3 * (the_terminal->frontkeys - 1);
	  } else
	    code_front_first = code_front_last  = -1;

	  if (the_terminal->haseasybar) {
	    code_easy_first = 0x03;
	    code_easy_last  = 0x18;
	    code_switch_first = 0x1b;
	    code_switch_last = 0x30;
	  } else
	    code_easy_first = code_easy_last  = 
	      code_switch_first = code_switch_last = -1;

	  LogPrint(LOG_DEBUG, "s %x-%x r %x-%x f %x-%x e %x-%x sw %x-%x",
		   code_status_first,
		       code_status_last,
		       code_route_first,
		       code_route_last,
		       code_front_first,
		       code_front_last,
		       code_easy_first,
		       code_easy_last,
		       code_switch_first,
		       code_switch_last);

	  // address of display
	  addr_status = 0x0000;
	  addr_display = addr_status + the_terminal->statcells;
	  LogPrint(LOG_DEBUG, "addr: %d %d",
		   addr_status,
		       addr_display);
	  return;
	}
  }
}

/* ------------------------------------------------------------ */

static void initbrlerror(brldim *brl)
{

  /* printf replaced with LogPrint  (NP) */
  LogPrint(LOG_ERR, "Initbrl: failure at open\n");

  if (brl->disp)
    free (brl->disp);
  brl->x = -1;
}

static void 
try_init(brldim *brl, const char *dev, unsigned int baud)
{
  brldim res;			/* return result */
  struct termios newtio;	/* new terminal settings */

  res.x = BRLCOLSMAX;		/* initialise size of display - unknownown yet */
  res.y = 1;
  res.disp = NULL;		/* clear pointers */

  /* Now open the Braille display device for random access */
  brl_fd = open (dev, O_RDWR | O_NOCTTY);
  if (brl_fd < 0) {
    initbrlerror(brl);
    return;
  }

  tcgetattr (brl_fd, &oldtio);	/* save current settings */

  /* Set bps, flow control and 8n1, enable reading */
  newtio.c_cflag = baud | CRTSCTS | CS8 | CLOCAL | CREAD;
  //newtio.c_cflag = baud | CS8 | CLOCAL | CREAD;

  /* Ignore bytes with parity errors and make terminal raw and dumb */
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;		/* raw output */
  newtio.c_lflag = 0;		/* don't echo or generate signals */
  newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
  newtio.c_cc[VTIME] = 0;
  tcflush (brl_fd, TCIFLUSH);	/* clean line */
  tcsetattr (brl_fd, TCSANOW, &newtio);		/* activate new settings */

  /* HACK - used with serial.c */
#ifdef _SERIAL_C_
  /* HACK - used with serial.c */
  terminal_type = pm_2dscreen;
#else
  identify_terminal(&res);
#endif

  /* Allocate space for buffers */
  res.disp = (unsigned char *) malloc (res.x * res.y);
  if (!res.disp)
    initbrlerror(&res);

  init_table();

  *brl = res;
  return;
}


static void 
initbrl (char **parameters, brldim *brl, const char *dev)
{
  LogPrint(LOG_DEBUG,  "try 19200");
  try_init(brl, dev, B19200);
  if (the_terminal==NULL) {
    closebrl(brl);
    LogPrint(LOG_DEBUG,  "try 38400");
    try_init(brl, dev, B38400);
  }
  if (the_terminal==NULL) {
    closebrl(brl);
    LogPrint(LOG_ERR, "unknown braille terminal type");
    exit(9);
  }
}


static void
closebrl (brldim *brl)
{
  free (brl->disp);
  tcsetattr (brl_fd, TCSANOW, &oldtio);	/* restore terminal settings */
  close (brl_fd);
}

static void
identbrl (void)
{
  LogAndStderr(LOG_NOTICE, "Papenmeier Driver (%s %s)", __DATE__, __TIME__);
  LogAndStderr(LOG_INFO, "   Copyright (C) 1998-2000 by The BRLTTY Team.");
  LogAndStderr(LOG_INFO, "                 August Hörandl <august.hoerandl@gmx.at>");
  LogAndStderr(LOG_INFO, "                 Heimo Schön <heimo.schoen@gmx.at>");
# ifdef RD_DEBUG
  LogAndStderr(LOG_INFO, "   Input debugging enabled.");
# endif
# ifdef WR_DEBUG
  LogAndStderr(LOG_INFO, "   Output debugging enabled.");
# endif
#ifdef MOD_DEBUG
  LogAndStderr(LOG_INFO, "   Modifier Keys debugging enabled.");
# endif

  /* read the config file for individual configurations */
#ifdef READ_CONFIG
  LogPrint(LOG_DEBUG, "look for config file");
  read_config();
#endif
}

static void 
write_to_braille(int offset, int size, const char* data)
{
  unsigned char BrlHead[] = 
  { cSTX,
    cIdSend,
    0,
    0,
    0,
    0,
  };
  unsigned char BrlTrail[]={ cETX };
  BrlHead[2] = offset / 256;
  BrlHead[3] = offset % 256;
  BrlHead[5] = (sizeof(BrlHead) + sizeof(BrlTrail) +size) % 256; 
  
  safe_write(brl_fd,BrlHead, sizeof(BrlHead));
  safe_write(brl_fd, data, size);
  safe_write(brl_fd,BrlTrail, sizeof(BrlTrail));
#ifdef SEND_TWICE_HACK
  delay(100);
  safe_write(brl_fd,BrlHead, sizeof(BrlHead));
  safe_write(brl_fd, data, size);
  safe_write(brl_fd,BrlTrail, sizeof(BrlTrail));
#endif
}


static void 
init_table()
{
  char line[BRLCOLSMAX];
  char spalte[PMSC];
  int i;
  // don´t use the internal table for the status column 
  for(i=0; i < curr_stats; i++)
    spalte[i] = 1; /* 1 = no table */
  write_to_braille(offsetTable+addr_status, curr_stats, spalte);

  // don´t use the internal table for the line
  for(i=0; i < curr_cols; i++)
    line[i] = 1; // 1 = no table
  write_to_braille(offsetTable+addr_display, curr_cols, line);
}

static void
writebrlstat(const unsigned char* s, int size)
{
 if (memcmp(s, prevline, size) != 0)
   {
     memcpy(prevline, s, size);
     write_to_braille(addr_status, size, prevline);
   }
}

static void
setbrlstat(const unsigned char* s)
{
  int i;
  unsigned char statcells[PMSC] = { 0 };

  LogPrint(LOG_DEBUG,"setbrlstat %d", curr_stats);

  if (curr_stats==0)
    return;
  for (i=0; i < curr_stats; i++) {
    int code = the_terminal->statshow[i];
    if (code == STAT_empty)
      statcells[i] = 0;
    else if (code >= OFFS_NUMBER)
      statcells [i] = change_bits[portrait_number(s[code-OFFS_NUMBER])];
    else if (code >= OFFS_FLAG)
      statcells[i] = change_bits[seascape_flag(i+1, s[code-OFFS_FLAG])];
    else if (code >= OFFS_HORIZ)
      statcells [i] = change_bits[seascape_number(s[code-OFFS_HORIZ])];
    else
      statcells [i] = change_bits[s[code]];
  }
  writebrlstat(statcells, curr_stats);
}

static void
writebrl (brldim *brl)
{
  int i;
#ifdef WR_DEBUG
  LogPrint(LOG_ERR, "write %2d %2d %*s", 
	   brl->x, brl->y, curr_cols, brl->disp);
#endif

  
  for(i=0; i < curr_cols; i++)
    brl->disp[i] = change_bits[brl->disp[i]];

  if (memcmp(prev,brl->disp,curr_cols) != 0)
    {
      memcpy(prev,brl->disp,curr_cols);
      write_to_braille(addr_display, curr_cols, prev);
    }
}

/* ------------------------------------------------------------ */

static unsigned char pressed_modifiers = 0;
static int beg_pressed = 0;
static int end_pressed = 0;

/* found command - some actions to be done within the driver */
static int
handle_command(int cmd, int ispressed)
{
  switch(cmd) {
  case CMD_RESTARTBRL:
    init_table();
    if (curr_stats > 0)
      write_to_braille(addr_status, curr_stats, prevline);
    write_to_braille(addr_display, curr_cols, prev);
    break;

  case CMD_CUT_BEG:
    beg_pressed = ispressed;
#ifdef MOD_DEBUG
      LogPrint(LOG_ERR, "Cut Begin: %02x",beg_pressed );
#endif      
    cmd=CMD_NOOP;
    break;

  case CMD_CUT_END:
    end_pressed = ispressed;
#ifdef MOD_DEBUG
      LogPrint(LOG_ERR, "Cut End: %02x",end_pressed );
#endif      
    cmd=CMD_NOOP;
    break;

  }
  if (ispressed)
    return cmd;
  else
    return CMD_NOOP;
}

/* one key is pressed or released */
static int
handle_key(int code, int ispressed)
{
  int i;
  /* look for modfier keys */
  for(i=0; i < MODMAX; i++) 
    if( the_terminal->modifiers[i] == code) {
      /* found modifier: update bitfield */
      /* pressed_modifiers ^= (1<<i);  
         could cause trouble if we miss one event */
      if (ispressed)
	pressed_modifiers |= (1<<i);
      else
	pressed_modifiers &= ~(1<<i);

#ifdef MOD_DEBUG
      LogPrint(LOG_ERR, "Modifiers: %02x", pressed_modifiers);
#endif      
      return CMD_NOOP;
    }

  /* must be a "normal key" - search for cmd on keypress */
  for(i=0; i < CMDMAX; i++)
    if ( the_terminal->cmds[i].keycode == code && 
	 the_terminal->cmds[i].modifiers == pressed_modifiers)
      {
	LogPrint(LOG_DEBUG, "cmd: %d->%d", code, the_terminal->cmds[i].code); 
	return handle_command( the_terminal->cmds[i].code, ispressed );
      }

  /* no command found */
  LogPrint(LOG_DEBUG, "cmd: %d mod = %02x ??", code, pressed_modifiers); 
  return CMD_ERR;
}

/* ------------------------------------------------------------ */

/* some makros */
/* read byte */
#define READ(OFFS) \
  if (safe_read(brl_fd,buf+OFFS,1) != 1) \
      return EOF;                   \

static int 
readbrl (DriverCommandContext cmds)
{
  unsigned char buf [20];
  int i, l, code, num, action;
  int error_handling;

  do {
    error_handling = 0;

    READ(0);
    if (buf[0] != cSTX)
        return CMD_ERR;
    LogPrint(LOG_DEBUG,  "read: STX");

    READ(1);
    if (buf[1] != cIdReceive) {
      if (3 <= buf[1] && buf[1] <= 6) /* valid error codes 3-6 */
	READ(2);		/* read ETX */
      /* else
	 return CMD_ERR;
      */
      LogPrint(LOG_ERR, "error handling - code %d", buf[1]);
      delay(100);
      write_to_braille(addr_status, curr_stats, prevline);
      delay(100);
      write_to_braille(addr_display, curr_cols, prev);
      error_handling = 1;
      tcflush (brl_fd, TCIFLUSH);	/* clean line */

    }
  } while (error_handling);

  READ(2);			/* code - 2 bytes */
  READ(3); 
  READ(4);			/* length - 2 bytes */
  READ(5);
  l = 0x100*buf[4] + buf[5];	/* Data count */
  if (l > sizeof(buf))
    return CMD_ERR;
  for(i = 6; i < l; i++)
    READ(i);			/* Data */
  
# ifdef RD_DEBUG
  {
    char dbg_buffer[256];
    sprintf(dbg_buffer, "read: ");
    for(i=0; i<l; i++)
      sprintf(dbg_buffer + 6 + 3*i, " %02x",buf[i]);
    LogPrint(LOG_ERR, dbg_buffer);
  }
# endif

  if (buf[l-1] != cETX)		/* ETX - End */
    return CMD_ERR;

  code = 0x100*buf[2]+buf[3];
  action = (buf[6] == PRESSED);

  /* which key -> translate to OFFS_* + number */
  /* attn: number starts with 1 */
  if (code_front_first <= code && 
      code <= code_front_last) { /* front key */
    num = 1 + (code - code_front_first) / 3;
    return handle_key(OFFS_FRONT + num, action);
  } 
  else if (code_status_first <= code && 
	   code <= code_status_last) { /* status key */
    num = 1+ (code - code_status_first) / 3;
    return handle_key(OFFS_STAT + num, action);
  }
  else if (code_easy_first <= code && 
      code <= code_easy_last) { /* easy bar */
    num = 1 + (code - code_easy_first) / 3;
    return handle_key(OFFS_EASY + num, action);
  } 
  else if (code_switch_first <= code && 
      code <= code_switch_last) { /* easy bar */
    num = 1 + (code - code_switch_first) / 3;
    return handle_key(OFFS_SWITCH + num, action);
  } 
  else if (code_route_first <= code && 
	   code <= code_route_last) { /* Routing Keys */ 
    num = (code - code_route_first) / 3;
    if (action) {
      if (beg_pressed) /* Cut Begin */
	return CR_BEGBLKOFFSET + num;
      else if (end_pressed) /* Cut End */
	return CR_ENDBLKOFFSET + num;
      else  /* CSR Jump */
	return CR_ROUTEOFFSET + num;
    } else
      return CMD_NOOP;
  }
  LogPrint(LOG_ERR, "readbrl: Command Error - CmdKod:%d", code);
  return CMD_ERR;
}


#ifdef READ_CONFIG

/* ------------------------------------------------------------ */
/* read config */

static void read_file(char* name)
{
  LogPrint(LOG_DEBUG, "open config file %s", name);
  configfile = fopen(name, "r");
  if (configfile == NULL) {
    perror(name);
    return;
  }
  LogPrint(LOG_DEBUG, "read config file %s", name);
  yyparse ();
  fclose(configfile);
}

static void read_config()
{
  char* env;

  /* 1. try environment BRLTTY_PM_CONF */
  env = getenv(CONFIG_ENV);
  if (env != NULL) {
    read_file(env);
    return;
  }

  /* 2. try CONFIG_FILE */
  read_file(CONFIG_FILE);
}

#endif

