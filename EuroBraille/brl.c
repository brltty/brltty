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

/* EuroBraille/brl.c - Braille display library for the EuroBraille family.
 * Copyright (C) 1997-2001 by Yannick Plassiard <Yannick.Plassiard@free.fr>
 *                        and Nicolas Pitre <nico@cam.org>
 * See the GNU General Public License for details in the ../COPYING file
 * See the README file for details about copyrights and version informations
 */

#define BRL_C 1

#define __EXTENSIONS__		/* for termios.h */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <string.h>

#include "brlconf.h"
#include "../brl.h"
#include "../message.h"
#include "../scr.h"
#include "../misc.h"
#include "../inskey.h"
#include "../config.h"
#include "../brl_driver.h"

static char StartupNotice[] =
  "  EuroBraille driver, version 1.0 \n";

static char StartupInfo[] =
  "  Copyright (C) 1997-2001 by \n"
  "      - Yannick PLASSIARD <Yannick.Plassiard@free.fr>\n"
  "      - Nicolas PITRE <nico@cam.org>\n";

static int model_ID = 0;
/* model identification string. possible values :
 * 0 - Unknown model name
 * 1 - NoteBRAILLE
 * 2 - Clio-NoteBRAILLE
 * 3 - SCRIBA
 * 4 - AzerBRAILLE
 */

static char version_ID[] = "Unknown";
/* This is where the driver will store the internal ROM version of the
 * terminal. It is used when the user asks the version of BRLTTY and his
 * terminal.
 */

#define BRLROWS		1
#define MAX_STCELLS	5	/* hiest number of status cells, if ever... */

/* This is the brltty braille mapping standard to Eurobraille's mapping table.
 */
char TransTable[256] =
{
   0x00, 0x01, 0x08, 0x09, 0x02, 0x03, 0x0A, 0x0B,
     0x10, 0x11, 0x18, 0x19, 0x12, 0x13, 0x1A, 0x1B,
     0x04, 0x05, 0x0C, 0x0D, 0x06, 0x07, 0x0E, 0x0F,
     0x14, 0x15, 0x1C, 0x1D, 0x16, 0x17, 0x1E, 0x1F,
     0x20, 0x21, 0x28, 0x29, 0x22, 0x23, 0x2A, 0x2B,
     0x30, 0x31, 0x38, 0x39, 0x32, 0x33, 0x3A, 0x3B,
     0x24, 0x25, 0x2C, 0x2D, 0x26, 0x27, 0x2E, 0x2F,
     0x34, 0x35, 0x3C, 0x3D, 0x36, 0x37, 0x3E, 0x3F,
     0x40, 0x41, 0x48, 0x49, 0x42, 0x43, 0x4A, 0x4B,
     0x50, 0x51, 0x58, 0x59, 0x52, 0x53, 0x5A, 0x5B,
     0x44, 0x45, 0x4C, 0x4D, 0x46, 0x47, 0x4E, 0x4F,
     0x54, 0x55, 0x5C, 0x5D, 0x56, 0x57, 0x5E, 0x5F,
     0x60, 0x61, 0x68, 0x69, 0x62, 0x63, 0x6A, 0x6B,
     0x70, 0x71, 0x78, 0x79, 0x72, 0x73, 0x7A, 0x7B,
     0x64, 0x65, 0x6C, 0x6D, 0x66, 0x67, 0x6E, 0x6F,
     0x74, 0x75, 0x7C, 0x7D, 0x76, 0x77, 0x7E, 0x7F,
     0x80, 0x81, 0x88, 0x89, 0x82, 0x83, 0x8A, 0x8B,
     0x90, 0x91, 0x98, 0x99, 0x92, 0x93, 0x9A, 0x9B,
     0x84, 0x85, 0x8C, 0x8D, 0x86, 0x87, 0x8E, 0x8F,
     0x94, 0x95, 0x9C, 0x9D, 0x96, 0x97, 0x9E, 0x9F,
     0xA0, 0xA1, 0xA8, 0xA9, 0xA2, 0xA3, 0xAA, 0xAB,
     0xB0, 0xB1, 0xB8, 0xB9, 0xB2, 0xB3, 0xBA, 0xBB,
     0xA4, 0xA5, 0xAC, 0xAD, 0xA6, 0xA7, 0xAE, 0xAF,
     0xB4, 0xB5, 0xBC, 0xBD, 0xB6, 0xB7, 0xBE, 0xBF,
     0xC0, 0xC1, 0xC8, 0xC9, 0xC2, 0xC3, 0xCA, 0xCB,
     0xD0, 0xD1, 0xD8, 0xD9, 0xD2, 0xD3, 0xDA, 0xDB,
     0xC4, 0xC5, 0xCC, 0xCD, 0xC6, 0xC7, 0xCE, 0xCF,
     0xD4, 0xD5, 0xDC, 0xDD, 0xD6, 0xD7, 0xDE, 0xDF,
     0xE0, 0xE1, 0xE8, 0xE9, 0xE2, 0xE3, 0xEA, 0xEB,
     0xF0, 0xF1, 0xF8, 0xF9, 0xF2, 0xF3, 0xFA, 0xFB,
     0xE4, 0xE5, 0xEC, 0xED, 0xE6, 0xE7, 0xEE, 0xEF,
     0xF4, 0xF5, 0xFC, 0xFD, 0xF6, 0xF7, 0xFE, 0xFF
};

/* Global variables */

int InDate = 0;
int brl_fd;			/* file descriptor for Braille display */
struct termios oldtio;		/* old terminal settings */
unsigned char *rawdata;		/* translated data to send to Braille */
unsigned char *prevdata;	/* previously sent raw data */
int NbCols = 0;			/* number of cells available */
static short ReWrite = 0;		/* 1 if display need to be rewritten */
static int PktNbr = 127; /* 127 at first time */
static int InMenu = 0;   /* Internal Date test-value */

/* Communication codes */

#define SOH     0x01
#define EOT     0x04
#define ACK     0x06
#define DLE     0x10
#define NACK    0x15

#define PRT_E_PAR 0x01		/* parity error */
#define PRT_E_NUM 0x02		/* frame numver error */
#define PRT_E_ING 0x03		/* length error */
#define PRT_E_COM 0x04		/* command error */
#define PRT_E_DON 0x05		/* data error */
#define PRT_E_SYN 0x06		/* syntax error */

#define DIM_INBUFSZ 256

static int readbrlkey(int type);

static int
  sendbyte (unsigned char c)
{
   return (write (brl_fd, &c, 1));
}

static int
  WriteToBrlDisplay (int len, char *data)
{
   unsigned char parity = 0;

   if (!len)
     return (1);

   sendbyte (SOH);
   while (len--)
     {
	switch (*data)
	  {
	   case SOH:
	   case EOT:
	   case ACK:
	   case DLE:
	   case NACK:
	     sendbyte (DLE);
	  /* no break */
	   default:
	     sendbyte (*data);
	     parity ^= *data++;
	  }
     }
   sendbyte (PktNbr);
   parity ^= PktNbr;
   if (++PktNbr >= 256)
     PktNbr = 128;
   switch (parity)
     {
      case SOH:
      case EOT:
      case ACK:
      case DLE:
      case NACK:
	sendbyte (DLE);
      /* no break */
      default:
	sendbyte (parity);
     }
   sendbyte (EOT);
   return (1);
}

static void identbrl (void)
{
   LogAndStderr(LOG_NOTICE, StartupNotice);
   LogAndStderr(LOG_INFO, StartupInfo);
}

static void initbrl (brldim *brl, const char *dev)
{
   brldim res;			/* return result */
   struct termios newtio;	/* new terminal settings */

   res.disp = rawdata = prevdata = NULL;		/* clear pointers */

  /* Open the Braille display device for random access */
   brl_fd = open (dev, O_RDWR | O_NOCTTY);
   if (brl_fd < 0)
     {
	LogPrint( LOG_ERR, "%s: %s\n", dev, strerror(errno) );
	goto failure;
     }
   tcgetattr (brl_fd, &oldtio);	/* save current settings */

  /* Set 8E1, enable reading, parity generation, etc. */
   newtio.c_cflag = CS8 | CLOCAL | CREAD | PARENB;
   newtio.c_iflag = INPCK;
   newtio.c_oflag = 0;		/* raw output */
   newtio.c_lflag = 0;		/* don't echo or generate signals */
   newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
   newtio.c_cc[VTIME] = 0;

  /* set speed */
   cfsetispeed (&newtio, BAUDRATE);
   cfsetospeed (&newtio, BAUDRATE);
   tcsetattr (brl_fd, TCSANOW, &newtio);		/* activate new settings */

  /* Set model params... */
  /* should be autodetected here... */
   while (!NbCols)
     {
	int i = 0;
	unsigned char AskIdent[] =
	  {2, 'S', 'I',3,'M','P','\x37'};
	WriteToBrlDisplay (7, AskIdent);
	while (!NbCols)
	  {
	     delay (100);
	     readbrl (0);		/* to get the answer */
	     if (++i >= 10)
	       break;
	  }
     }
   sethlpscr (0);
   res.x = NbCols;		/* initialise size of display */
   res.y = BRLROWS;

  /* Allocate space for buffers */
   res.disp = (char *) malloc (res.x * res.y);
   rawdata = (unsigned char *) malloc (res.x * res.y);
   prevdata = (unsigned char *) malloc (res.x * res.y);
   if (!res.disp || !rawdata || !prevdata)
     {
	LogPrint( LOG_ERR, "can't allocate braille buffers\n" );
	goto failure;
     }

   ReWrite = 1;			/* To write whole display at first time */

   *brl = res;
   return;

   failure:;
   if (res.disp)
     free (res.disp);
   if (rawdata)
     free (rawdata);
   if (prevdata)
     free (prevdata);
   brl->x = -1;
   return;
}

static void closebrl (brldim *brl)
{
   free (brl->disp);
   free (rawdata);
   free (prevdata);
   tcsetattr (brl_fd, TCSANOW, &oldtio);		/* restore terminal settings */
   close (brl_fd);
}

static void writebrl (brldim *brl)
{
   int i, j;
   if (InMenu != 0) return;
   if (!ReWrite)
     {
      /* We update the display only if it has changed */
	if( memcmp( brl->disp, prevdata, NbCols ) != 0 )
	  ReWrite = 1;
     }
   if (ReWrite)
     {
      /* right end cells don't have to be transmitted if all dots down */
	i = NbCols;
#if 0   /* *** the ClioBraille doesn't seem to like this part... */
	while (--i > 0)		/* at least the first cell must go through... */
	  if (brl->disp[i] != 0)
	    break;
	i++;
#endif

	  {
	     char OutBuf[2 * i + 6];
	     char *p = OutBuf;
#if 0   /* *** don't know how to use DX correctly... */
	/* This part should display on the LCD screen */
	     *p++ = i + 2;
	     *p++ = 'D';
	     *p++ = 'X';
	     for (j = 0; j < 1; *p++ = ' ', j++);	/* fill LCD with whitespaces */
#else
	/* This is just to make the terminal accept the DY command */
	     *p++ = 2;
	     *p++ = 'D';
	     *p++ = 'X';
#endif

	/* This part displays on the braille line */
	     *p++ = i + 2;
	     *p++ = 'D';
	     *p++ = 'Y';
	     for (j = 0;
		  j < i;
		  *p++ = TransTable[(prevdata[j++] = brl->disp[j])]);
	     WriteToBrlDisplay (p - OutBuf, OutBuf);
	  }
	ReWrite = 0;
     }
}

static void setbrlstat (const unsigned char *st)
{
  /* sorry but the ClioBraille I got here doesn't have any status cells...
   * Don't know either if any EuroBraille terminal have some.
   */
}

static int Program(void)
{
   int touche;
   static int exitprog = 0;
   static int res3 = 0;
   switch (model_ID)
     {
      case 3:
	message("Beta level ...",0);
	break;
      default:
	message("P PROGRAMMING      x",0);
	break;
     }
   exitprog=0;
   while (!exitprog)
     {
	touche=readbrlkey(0);
	switch(touche)
	  {
	   case 1:
	     res3 = CMD_CONFMENU;
	     exitprog=1;
	     break;
	   case 2:
	     switch(model_ID)
	       {
		case 3: /* For SCRIBA only */
		  res3 = CMD_CONFMENU;
		  exitprog=1;
		  break;
		default:
		  res3 = CMD_RESET;
		  exitprog=1;
		  break;
	       }
	     break;
	   case 8: 
	     if (model_ID == 3) {
		res3 = CMD_CSRTRK;
		exitprog=1;
	     }
	     break;
	   case 10:
	     exitprog=1;
	     ReWrite=1;
	     break;
	  }
     }
   return res3;
}

static int stopmode(void) __attribute__((unused));
static int stopmode(void)
{
   unsigned char parity = 0;
   sendbyte(SOH);
   sendbyte(2);
   parity ^= 2;
   sendbyte('K');
   parity ^= 'K';
   sendbyte('r');
   parity ^= 'r';
   sendbyte(PktNbr - 2);
   parity ^= (PktNbr - 2);
   sendbyte(parity);
   sendbyte(EOT);
   return (0);
}

static int DisplayDate(void)
{
   unsigned char parity = 0;
   sendbyte(SOH);
   sendbyte(2);
   parity ^= 2;
   sendbyte('R');
   parity ^= 'R';
   sendbyte('D');
   parity ^= 'D';
   sendbyte(PktNbr);
   parity ^= PktNbr;
   if (PktNbr == 256) PktNbr = 0x80;
   sendbyte(parity);
   sendbyte(EOT);
   return 0;
}

static int startmode(void) __attribute__((unused));
static int startmode(void)
{
   static unsigned char parity = 0;
   sendbyte(SOH);
   sendbyte(2);
   parity ^= 2;
   sendbyte('K');
   parity ^= 'K';
   sendbyte('R');
   parity ^= 'R';
   sendbyte(PktNbr);
   parity ^= PktNbr;
   if (++PktNbr == 256 ) PktNbr = 0x80;
   sendbyte(parity);
   sendbyte(EOT);
   return (0);
}

/* On-line help function */

static int OnlineHelp(void)
{
   int touche;
   static int exithelp = 0;
   static int exithelp1 = 0;
   static int exithelp2 = 0;
   message("! On-line Help",0);
   exithelp=0;
   exithelp1=0;
   exithelp2=0;
   while (!exithelp)
     {
	exithelp1=0;
	exithelp2=0;
	touche=readbrlkey(0);
	switch(touche)
	  {
	   case 1:
	     message("1 undefined key",0);
	     break;
	   case 2:
	     message("2 previous line",0);
	     break;
	   case 3:
	     message("3 previous diff line",0);
	     break;
	   case 4:
	     message("4 view on cursor",0);
	     break;
	   case 5:
	     message("5 undefined key",0);
	     break;
	   case 6:
	     message("6 undefined key",0);
	     break;
	   case 7:
	     message("7 undefined key",0);
	     break;
	   case 8:
	     message("8 next line ",0);
	     break;
	   case 9:
	     message("9 next diff line",0);
	     break;
	   case 10:
	     message("* programming/cancel",0);
	     while (!exithelp1)
	       {
		  touche=readbrlkey(0);
		  switch(touche)
		    {
		     case 1:
		       message("*1 Config menu",0);
		       exithelp1=1;
		       break;
		     case 2:
		       message("*2 reset configuration",0);
		       exithelp1=1;
		       break;
		     case 10:
		       exithelp1=1;
		       exithelp=1;
		       break;
		    }
	       }
	     break;
	   case 11:
	     message("# view on (validate)",0);
	     while (!exithelp2)
	       {
		  touche=readbrlkey(0);
		  switch(touche)
		    {
		     case 1:
		       message("#1 undefined key",0);
		       exithelp2=1;
		       break;
		     case 2:
		       message("#2 undefined key",0);
		       exithelp2=1;
		       break;
		     case 3:
		       message("#3 top-left corner",0);
		       exithelp2=1;
		       break;
		     case 4:
		       message("#4 undefined key",0);
		       exithelp2=1;
		       break;
		     case 5:
		       message("#5 undefined key",0);
		       exithelp2=1;
		       break;
		     case 6:
		       message("#6 undefined key",0);
		       exithelp2=1;
		       break;
		     case 7:
		       message("#7 undefined key",0);
		       exithelp2=1;
		       break;
		     case 8:
		       message("#8 undefined key",0);
		       exithelp2=1;
		       break;
		     case 9:
		       message("#9 BottomLeft corner",0);
		       exithelp2=1;
		       break;
		     case 30:
		       message("#0 undefined key",0);
		       exithelp2=1;
		       break;
		     case 10:
		       message("#* undefined key",0);
		       exithelp2=1;
		       break;
		     case 11:
		       exithelp2=1;
		       exithelp=1;
		       break;
		     case 12:
		       message("#A toggle attributes",0);
		       exithelp2=1;
		       break;
		    }
	       }
	     break;
	   case 12:
	     message("A undefined key",0);
	     break;
	   case 13:
	     message("B mode selection",0);
	     break;
	   case 14:
	     message("C undefined key",0);
	     break;
	   case 15:
	     message("D repeat braille comb",0);
	     break;
	   case 16:
	     message("E move display left",0);
	     break;
	   case 17:
	     message("F top-left corner",0);
	     break;
	   case 18:
	     message("G cursor invis/visib",0);
	     break;
	   case 19:
	     message("H undefined key",0);
	     break;
	   case 20:
	     message("I show coordinates",0);
	     break;
	   case 21:
	     message("J start of block",0);
	     break;
	   case 22:
	     message("K end of block",0);
	     break;
	   case 23:
	     message("L paste block",0);
	     break;
	   case 24:
	     message("M move display right",0);
	     break;
	   case 30:
	     message("0 TrackCursor on/off",0);
	     break;
	  }
     }

   return 0;
}

static int ViewOn(void)
{
   int touche;
   static int res2 = 0;
   static int exitviewon = 0;
   if (InDate == 1)
     {
	InDate = 0;
	ReWrite = 1;
	return (0);
     }
   switch (model_ID)
     {
      case 3:
	message("Alpha level ...",0);
	break;
      default:
	message("V VIEW ON          x",0);
	break;
     }
   exitviewon=0;
   while (!exitviewon)
     {
	touche=readbrlkey(0);
	switch(touche)
	  {
	   case 1:
	     exitviewon=1;
	     break;
	   case 3:
	     res2 = CMD_TOP_LEFT;
	     exitviewon=1;
	     break;
	   case 9:
	     res2 = CMD_BOT_LEFT;
	     exitviewon=1;
	     break;
	   case 11:
	     exitviewon=1;
	     break;
	   case 12:
	     res2 = CMD_DISPMD;
	     exitviewon=1;
	     break;
	   case 19:
	     OnlineHelp();
	     exitviewon=1;
	     break;
	   case 24:
	     res2 = DisplayDate();
	     break;
	  }
     }
   return res2;
}

static int readbrl(int type)
{
   int result;
   result=readbrlkey(0);
   switch (result)
     {
      case 30: /* '0' key was pressed */
	result = CMD_CSRTRK;
	break;
      case 1: /* '1' key was pressed */
	result = EOF;
	break;
      case 2: /* '2' key was pressed */
	result = CMD_LNUP;
	break;
      case 3: /* '3' key was pressed */
	result = CMD_PRDIFLN;
	break;
      case 4: /* '4' key was pressed */
	result = CMD_HOME;
	break;
      case 5: /* '5' key was pressed */
	result = CMD_NOOP;
	break;
      case 6: /* '6' key was pressed */
	result = CMD_NOOP;
	break;
      case 7: /* '7' key was pressed */
	break;
      case 8: /* '8 key was pressed */
	result = CMD_LNDN;
	break;
      case 9: /* '9' key was pressed */
	result = CMD_NXDIFLN;
	break;
      case 10: /* '*' key was pressed */
	result = Program();
	break;
      case 11: /* '#' key was pressed */
	result = ViewOn(); /* Lunch the View On  procedure */
	break;
      case 12: /* 'a' key was pressed */
	break;
      case 13: /* 'b' key was pressed */
	break;
      case 14: /* 'c' key was pressed */
	break;
      case 15: /* 'd' key was pressed */
	break;
      case 16: /* 'e' key pressed */
	result = CMD_FWINLT;
	break;
      case 17: /* 'f' key was pressed */
	switch(model_ID)
	  {
	   case 4: /* AzerBRAILLE model */
	     result = CMD_LNUP;
	     break;
	   default:
	     result = CMD_TOP_LEFT;
	     break;
	  }
	break;
      case 18: /* 'g' key was pressed */
	result = CMD_CSRVIS;
	break;
      case 19: /* 'h' key was pressed */
	break;
      case 20: /* 'i' kew was pressed */
	result = CMD_INFO;
	break;
      case 21: /* 'j' key was pressed */
	break;
      case 22: /* 'k' key was pressed */
	break;
      case 23: /* 'l' key was pressed */
	switch(model_ID)
	  {
	   case 1: /* NoteBRAILLE model */
	     result = CMD_LNDN;
	     break;
	   case 4: /* AzerBRAILLE model */
	     result = CMD_LNDN;
	     break;
	   default:
	     result = CMD_PASTE;
	     break;
	  }
	break;
      case 24: /* 'm' key was pressed */
	result = CMD_FWINRT;
	break;
      case 31:
	message(version_ID, MSG_WAITKEY);
	break;
     }
   return result;
}

static int routing(int routekey)
{
   int OffsetType = CR_ROUTEOFFSET;
   int res = EOF;
   switch (InMenu)
     {
      case 1:
	switch (routekey)
	  {
	   case 0x02: /* exit menu */     
	     ReWrite = 1;
	     InMenu = 0;
	     res = CMD_NOOP;
	     break;
	   case 0x06: /* Console Switching */
	     InMenu = 0;
	     message("switch:1 2 3 4 5 6 t", 0);
	     InMenu = 2;
	     ReWrite = 0;
	     res = CMD_NOOP;
	     break;
	   case 0x0B: /* Help */
	     InMenu = 0;
	     OnlineHelp();
	     
	     ReWrite = 1;
	     res = CMD_NOOP;
	     break;
	  }
	break;
      case 2:
	switch(routekey)
	  {
	   case 0x07: /* exit */
	     InMenu = 0;
	     
	     ReWrite = 1;
	     res = CMD_NOOP;
	     break;
	   case 0x09: /* switch to console 1 */
	     res = CR_SWITCHVT;   /* CR_WITCHVT + 0 */
	     InMenu = 0;
	     
	     ReWrite = 1;
	     break;
	   case 0x0B: /* switch to console 2 */
	     res = CR_SWITCHVT + 1;
	     ReWrite = 1;
	     InMenu = 0;
	     
	     break;
	   case 0x0D: /* switch to console 3 */
	     res = CR_SWITCHVT + 2;
	     InMenu = 0;
	     ReWrite = 1;
	     
	     break;
	   case 0x0F: /* switch to console 4 */
	     res = CR_SWITCHVT + 3;
	     ReWrite = 1;
	     InMenu = 0;
	     
	     break;
	   case 0x11: /* switch to console 5 */
	     res = CR_SWITCHVT + 4;
	     InMenu = 0;
	     ReWrite = 1;
	     
	     break;
	   case 0x13: /* switch to console 6 */
	     res = CR_SWITCHVT + 5;
	     InMenu = 0;
	     ReWrite = 1;
	     break;
	  }
	break;
      case 0:
	switch (routekey)
	  {
	   case 0x83: /* Entering in menu-mode */
	     message("-:tty Help t", 0);
	     InMenu = 1;
	     res = CMD_NOOP;
	     break;
	   default:
	     res = OffsetType + routekey - 1;
	     OffsetType = CR_ROUTEOFFSET;
	     break;
	  }
	break;
     }
   return res;
}

static int
   readbrlkey(int type)
{
   int res = EOF;
   unsigned char c;
   static unsigned char buf[DIM_INBUFSZ];
   static int DLEflag = 0, ErrFlag = 0;
   static int pos = 0, p = 0, pktready = 0, OffsetType = CR_ROUTEOFFSET;
   static int position = 2; /* to retrieve internal version */
  /* here we process incoming data */
   while (!pktready && read (brl_fd, &c, 1))
     {
	if (DLEflag)
	  {
	     DLEflag = 0;
	     if( pos < DIM_INBUFSZ ) buf[pos++] = c;
	  }
	else if( ErrFlag )
	  {
	     ErrFlag = 0;
	  /* Maybe should we check error code in c here? */
	     ReWrite = 1;
	  }
	else
	  switch (c)
	    {
	     case NACK:
	       ErrFlag = 1;
	    /* no break */
	     case ACK:
	     case SOH:
	       pos = 0;
	       break;
	     case DLE:
	       DLEflag = 1;
	       break;
	     case EOT:
		 {
	      /* end of packet, let's read it */
		    int i;
		    unsigned char parity = 0;
		    if (pos < 4)
		      break;		/* packets can't be shorter */
		    for( i = 0; i < pos-1; i++ )
		      parity ^= buf[i];
		    if ( parity != buf[pos - 1])
		      {
			 sendbyte (NACK);
			 sendbyte (PRT_E_PAR);
		      }
		    else if (buf[pos - 2] < 0x80)
		      {
			 sendbyte (NACK);
			 sendbyte (PRT_E_NUM);
		      }
		    else
		      {
		  /* packet is OK */
			 sendbyte (ACK);
			 pos -= 2;	/* now forget about packet number and parity */
			 p = 0;
			 pktready = 1;
		      }
		    break;
		 }
	     default:
	       if (pos < DIM_INBUFSZ)
		 buf[pos++] = c;
	       break;
	    }
     }

  /* Packet is OK, we go inside */
   if (pktready)
     {
	int lg;
	while (res == EOF)
	  {
	  /* let's look at the next message */
	     lg = buf[p++];
	     if (lg >= 0x80 || p + lg > pos)
	       {
		  pktready = 0;	/* we are done with this packet */
		  break;
	       }
	     switch (buf[p])
	       {
		case 'R': /* Mode checking */
		  switch(buf[p + 1])
		    {
		     case 'B': /* PC-BRAILLE mode */
		       InDate = 0;
		       ReWrite = 1; /* to refresh display */
		       InMenu = 0;
		       res = CMD_NOOP;
		       break;
		     case 'V': /* Braille and Speech mode */
		       message("! Speech unavailable", MSG_WAITKEY);
		       break;
		    }
		  break;
		case 'K':		/* Keyboard -- here are the key bindings */
		  switch (buf[p + 1])
		    {
		     case 'T':	/* Control Keyboard */
		       switch (buf[p + 2])
			 {
			  case 'E':
			    res = 16;
			    break;
			  case 'F':
			    res = 17;
			    break;
			  case 'G':
			    res = 18;
			    break;
			  case 'H':
			    res = 19;
			    break;
			  case 'I':
			    res = 20;
			    break;
			  case 'J':
			    res = 21;
			    OffsetType = CR_BEGBLKOFFSET;
			    break;
			  case 'K':
			    res = 22;
			    OffsetType = CR_ENDBLKOFFSET;
			    break;
			  case 'L':
			    res = 23;
			    break;
			  case 'M':
			    res = 24;
			    break;

			  case 'A':
			    res = 12;
			    break;
			  case 'B':
			    res = 13;
			    break;
			  case 'C':
			    res = 14;
			    break;
			  case 'D':
			    res = 15;
			    break;
			  case 'Z': /* ABCD Keys */
			    res = 31;
			    break;
			  case '1':
			    res = 1;
			    break;
			  case '2':
			    res = 2;
			    break;
			  case '3':
			    res = 3;
			    break;
			  case '4':
			    res = 4;
			    break;
			  case '5':
			    res = 5;
			    break;
			  case '6':
			    res = 6;
			    break;
			  case '7':
			    res = 7;
			    break;
			  case '8':
			    res = 8;
			    break;
			  case '9':
			    res = 9;
			    break;
			  case '*':
			    res = 10;
			    break;
			  case '0':
			    res = 30;
			    break;
			  case '#':
			    res = 11;
			    break;
			 }
		       break;
		     case 'I':	/* Routing Key */
		       res = routing(buf[p + 2]);
		       break;
		     case 'B':	/* Braille keyboard */
			 {
		    /* here the braille keys are bitmapped into an int with
		     * dots 1 through 8, left thumb and right thumb
		     * respectively.  It makes up to 1023 possible
		     * combinations! Here's some of them.
		     */
			    unsigned int keys = (buf[p + 2] & 0x3F) |
			      ((buf[p + 3] & 0x03) << 6) |
			      ((int) (buf[p + 2] & 0xC0) << 2);
			    switch (keys)
			      {
			       case 0x001: /* braille 'a' */
				 inskey("a");
				 break;
			       case 0x003: /* braille 'b' */
				 inskey("b");
				 break;
			       case 0x009: /* braille 'c' */
				 inskey("c");
				 break;
			       case 0x019: /* braille 'd' */
				 inskey("d");
				 break;
			       case 0x011: /* braille 'e' */
				 inskey("e");
				 break;
			       case 0x00b: /* braille 'f' */
				 inskey("f");
				 break;
			       case 0x01b: /* braille 'g' */
				 inskey("g");
				 break;
			       case 0x013: /* braille 'h' */
				 inskey("h");
				 break;
			       case 0x00a: /* braille 'i' */
				 inskey("i");
				 break;
			       case 0x01a: /* braille 'j' */
				 inskey("j");
				 break;
			       case 0x005: /* braille 'k' */
				 inskey("k");
				 break;
			       case 0x007: /* braille 'l' */
				 inskey("l");
				 break;
			       case 0x00d: /* braille 'm' */
				 inskey("m");
				 break;
			       case 0x01d: /* braille 'n' */
				 inskey("n");
				 break;
			       case 0x015: /* braille 'o' */
				 inskey("o");
				 break;
			       case 0x00f: /* braille 'p' */
				 inskey("p");
				 break;
			       case 0x01f: /* braille 'q' */
				 inskey("q");
				 break;
			       case 0x017: /* braille 'r' */
				 inskey("r");
				 break;
			       case 0x00e: /* braille 's' */
				 inskey("s");
				 break;
			       case 0x01e: /* braille 't' */
				 inskey("t");
				 break;
			       case 0x025:	/* braille 'u' */
				 inskey("u");
				 break;
			       case 0x027: /* braille 'v' */
				 inskey("v");
				 break;
			       case 0x03a: /* braille 'w' */
				 inskey("w");
				 break;
			       case 0x02d: /* braille 'x' */
				 inskey("x");
				 break;
			       case 0x03d: /* braille 'y' */
				 inskey("y");
				 break;
			       case 0x035: /* braille 'z' */
				 inskey("z");
				 break;
            /* Upperfase letters mapping */
			       case 0x041: /* braille 'A' */
				 inskey("A");
				 break;
			       case 0x043: /* braille 'B' */
				 inskey("B");
				 break;
			       case 0x049: /* braille 'C' */
				 inskey("C");
				 break;
			       case 0x059: /* braille 'D' */
				 inskey("D");
				 break;
			       case 0x051: /* braille 'E' */
				 inskey("E");
				 break;
			       case 0x04b: /* braille 'F' */
				 inskey("F");
				 break;
			       case 0x05b: /* braille 'G' */
				 inskey("G");
				 break;
			       case 0x053: /* braille 'H' */
				 inskey("H");
				 break;
			       case 0x04a: /* braille 'I' */
				 inskey("I");
				 break;
			       case 0x05a: /* braille 'J' */
				 inskey("J");
				 break;
			       case 0x045: /* braille 'K' */
				 inskey("K");
				 break;
			       case 0x047: /* braille 'L' */
				 inskey("L");
				 break;
			       case 0x04d: /* braille 'M' */
				 inskey("M");
				 break;
			       case 0x05d: /* braille 'N' */
				 inskey("N");
				 break;
			       case 0x055: /* braille 'O' */
				 inskey("O");
				 break;
			       case 0x04f: /* braille 'P' */
				 inskey("P");
				 break;
			       case 0x05f: /* braille 'Q' */
				 inskey("Q");
				 break;
			       case 0x057: /* braille 'R' */
				 inskey("R");
				 break;
			       case 0x04e: /* braille 'S' */
				 inskey("S");
				 break;
			       case 0x05e: /* braille 'T' */
				 inskey("T");
				 break;
			       case 0x065: /* braille 'U' */
				 inskey("U");
				 break;
			       case 0x067: /* braille 'V' */
				 inskey("V");
				 break;
			       case 0x07a: /* braille 'W' */
				 inskey("W");
				 break;
			       case 0x06d: /* braille 'X' */
				 inskey("X");
				 break;
			       case 0x07d: /* braille 'Y' */
				 inskey("Y");
				 break;
			       case 0x075: /* braille 'Z' */
				 inskey("Z");
				 break;
            /* The following codes are depending of the table you use.
             * These are digits mapping for CBISF table.
            */
			       case 0x03c: /* braille '0' */
				 inskey("0");
				 break;
			       case 0x021: /* braille '1' */
				 inskey("1");
				 break;
			       case 0x023: /* braille '2' */
				 inskey("2");
				 break;
			       case 0x029: /* braille '3' */
				 inskey("3");
				 break;
			       case 0x039: /* braille '4' */
				 inskey("4");
				 break;
			       case 0x031: /* braille '5' */
				 inskey("5");
				 break;
			       case 0x02b: /* braille '6' */
				 inskey("6");
				 break;
			       case 0x03b: /* braille '7' */
				 inskey("7");
				 break;
			       case 0x033: /* braille '8' */
				 inskey("8");
				 break;
			       case 0x02a: /* braille '9' */
				 inskey("9");
				 break;
            /* other chars */
			       case 0x016: /* braille '!' */
				 inskey("!");
				 break;
			       case 0x022: /* braille '?' */
				 inskey("?");
				 break;
			       case 0x02c: /* braille '#' */
				 inskey("#");
				 break;
			       case 0x002: /* braille ',' */
				 inskey(",");
				 break;
			       case 0x006: /* braille ';' */
				 inskey(";");
				 break;
			       case 0x095: /* braille 'ø' */
				 inskey("\xf8");
				 break;
			       case 0x01c: /* braille '@' */
				 inskey("@");
				 break;
			       case 0x014: /* braille '*' */
				 inskey("*");
				 break;
			       case 0x032: /* braille '/' */
				 inskey("/");
				 break;
			       case 0x0d6: /* braille '+' */
				 inskey("+");
				 break;
			       case 0x024: /* braille '-' */
				 inskey("-");
				 break;
			       case 0x012: /* braille ':' */
				 inskey(":");
				 break;
			       case 0x004: /* braille '.' */
				 inskey(".");
				 break;
			       case 0x026: /* braille '(' */
				 inskey("(");
				 break;
			       case 0x034: /* braille ')' */
				 inskey(")");
				 break;
			       case 0x036: /* braille '"' */
				 inskey("\x22");
				 break;
			       case 0x037: /* braille '[' */
				 inskey("[");
				 break;
			       case 0x03e: /* braille ']' */
				 inskey("]");
				 break;
			       case 0x077: /* braille '{' */
				 inskey("{");
				 break;
			       case 0x07e: /* braille '}' */
				 inskey("}");
				 break;
			       case 0x00c: /* braille '^' */
				 inskey("^");
				 break;
			       case 0x038: /* braille '$' */
				 inskey("$");
				 break;
			       case 0x020: /* braille ['] */
				 inskey("'");
				 break;
			       case 0x02e: /* braille '\' */
				 inskey("\\");
				 break;
			       case 0x02f: /* braille '%' */
				 inskey("%");
				 break;
			       case 0x018: /* braille '>' */
				 inskey(">");
				 break;
			       case 0x030: /* braille '<' */
				 inskey("<");
				 break;
			       case 0x0f6: /* braille '=' */
				 inskey("=");
				 break;
			       case 0x03f: /* braille '&' */
				 inskey("&");
				 break;
			       case 0x05c: /* ascii code 96 */
				 inskey("\x60");
				 break;
			       case 0x078: /* braille '_' */
				 inskey("_");
				 break;
			       case 0x04c: /* braille '~' */
				 inskey("~");
				 break;
            /* char accents */
			       case 0x0b7: /* braille '…' */
				 inskey("…");
				 break;
			       case 0x0e1: /* braille 'ƒ' */
				 inskey("ƒ");
				 break;
			       case 0x0ae: /* braille 'Š' */
				 inskey("Š");
				 break;
			       case 0x0e3: /* braille 'ˆ' */
				 inskey("ˆ");
				 break;
			       case 0x0ab: /* braille '‰' */
				 inskey("‰");
				 break;
			       case 0x0bf: /* braille '‚' */
				 inskey("\x82");
				 break;
			       case 0x0e9: /* braille 'Œ' */
				 inskey("Œ");
				 break;
			       case 0x0f9: /* braille '“' */
				 inskey("“");
				 break;
			       case 0x0f1: /* braille '–' */
				 inskey("–");
				 break;
			       case 0x0be: /* braille '—' */
				 inskey("—");
				 break;
			       case 0x0b3: /* braille '' */
				 inskey("");
				 break;
			       case 0x0af: /* braille '‡' */
				 inskey("‡");
				 break;
			       case 0x06e: /* braille '|' */
				 inskey("|");
				 break;
			       case 0x200: /* braille ' ' */
				 inskey(" ");
				 break;
			       case 0x100: /* braille backspace */
				 inskey("\x7f");
				 break;
			       case 0x300: /* braille enter */
				 inskey("\x0D");
				 break;
            /* the following codes define extended keys (ie. control, alt...)
             * The table used here is the eurobraille "!config.f" file.
            */
            /* braille mapping for functions keys */
			       case 0x208: /* Braille up key */
				 inskey("\33[A");
				 break;
			       case 0x220: /* braille down key */
				 inskey("\33[B");
				 break;
			       case 0x202: /* braille left key */
				 inskey("\33[D");
				 break;
			       case 0x210: /* braille right key */
				 inskey("\33[C");
				 break;
			       case 0x101: /* braille f1 */
				 inskey("\33[[A");
				 break;
			       case 0x103: /* braille f2 */
				 inskey("\33[[B");
				 break;
			       case 0x109: /* braille f3 */
				 inskey("\33[[C");
				 break;
			       case 0x119: /* braille f4 */
				 inskey("\33[[D");
				 break;
			       case 0x111: /* braille f5 */
				 inskey("\33[[E");
				 break;
			       case 0x10b: /* braille f6 */
				 inskey("\33[17~");
				 break;
			       case 0x11b: /* braille f7 */
				 inskey("\33[18~");
				 break;
			       case 0x112: /* braille f8 */
				 inskey("\33[19~");
				 break;
			       case 0x10a: /* braille f9 */
				 inskey("\33[20~");
				 break;
			       case 0x11a: /* braille f10 */
				 inskey("\33[21~");
				 break;
			       case 0x105: /* braille f11 */
				 inskey("\33[22~");
				 break;
			       case 0x107: /* braille f12 */
				 inskey("\33[24~");
				 break;
			       case 0x3ff: /* braille ctrl+alt+del */
				 inskey("reboot\x0d");
				 break;
			       case 0x207: /* home key */
				 inskey("\33[1~");
				 break;
			       case 0x238: /* end key */
				 inskey("\33[4~");
				 break;
			       case 0x205: /* braille page-up */
				 inskey("\33[5~");
				 break;
			       case 0x232: /* braille tab */
				 inskey("\x09");
				 break;
			       case 0x216: /* shift+tab key */
				 inskey("\x09");
				 break;
			       case 0x21b: /* braille esc key */
				 inskey("\x1b");
				 break;
			       case 0x228: /* braille page-down */
				 inskey("\33[6~");
				 break;
	    /* Control + char */
			       case 0x0c1: /* ctrl+a */
				 inskey("\x01");
				 break;
			       case 0x0c3: /* ctrl+b */
				 inskey("\x02");
				 break;
			       case 0x0c9: /* ctlr+c */
				 inskey("\x03");
				 break;
			       case 0x0d9: /* ctrl+d */
				 inskey("\x04");
				 break;
			       case 0x0d1: /* ctrl+e */
				 inskey("\x05");
				 break;
			       case 0x0cb: /* ctrl+f */
				 inskey("\x06");
				 break;
			       case 0x0db: /* ctrl+g */
				 inskey("\x07");
				 break;
			       case 0x0d3: /* ctrl+h */
				 inskey("\x08");
				 break;
			       case 0x0ca: /* ctrl+i */
				 inskey("\x09");
				 break;
			       case 0x0da: /* ctrl+j */
				 inskey("\x0a");
				 break;
			       case 0x0c5: /* ctrl+k */
				 inskey("\x0b");
				 break;
			       case 0x0c7: /* ctrl+l */
				 inskey("\x0c");
				 break;
			       case 0x0cd: /* ctrl+m */
				 inskey("\x0d");
				 break;
			       case 0x0dd: /* ctrl+n */
				 inskey("\x0e");
				 break;
			       case 0x0d5: /* ctrl+o */
				 inskey("\x0f");
				 break;
			       case 0x0cf: /* ctrl+p */
				 inskey("\x10");
				 break;
			       case 0x0df: /* ctrl+q */
				 inskey("\x11");
				 break;
			       case 0x0d7: /* ctrl+r */
				 inskey("\x12");
				 break;
			       case 0x0ce: /* ctrl+s */
				 inskey("\x13");
				 break;
			       case 0x0de: /* ctrl+t */
				 inskey("\x14");
				 break;
			       case 0x0e5: /* ctrl+u */
				 inskey("\x15");
				 break;
			       case 0x0e7: /* ctrl+v */
				 inskey("\x16");
				 break;
			       case 0x0fa: /* ctrl+w */
				 inskey("\x17");
				 break;
			       case 0x0ed: /* ctrl+x */
				 inskey("\x18");
				 break;
			       case 0x0fd: /* ctrl+y */
				 inskey("\x19");
				 break;
			       case 0x0f5: /* ctrl+z */
				 inskey("\x1a");
				 break;
	    /* alt + char */
			       case 0x301: /* alt+a */
				 inskey("\x1b\x61");
				 break;
			       case 0x303: /* alt+b */
				 inskey("\x1b\x62");
				 break;
			       case 0x309: /* alt+c */
				 inskey("\x1b\x63");
				 break;
			       case 0x319: /* alt+d */
				 inskey("\x1b\x64");
				 break;
			       case 0x311: /* alt+e */
				 inskey("\x1b\x65");
				 break;
			       case 0x30b: /* alt+f */
				 inskey("\x1b\x66");
				 break;
			       case 0x31b: /* alt+g */
				 inskey("\x1b\x67");
				 break;
			       case 0x313: /* alt+h */
				 inskey("\x1b\x68");
				 break;
			       case 0x30a: /* alt+i */
				 inskey("\x1b\x69");
				 break;
			       case 0x31a: /* alt+j */
				 inskey("\x1b\x6a");
				 break;
			       case 0x305: /* alt+k */
				 inskey("\x1b\x6b");
				 break;
			       case 0x307: /* alt+l */
				 inskey("\x1b\x6c");
				 break;
			       case 0x30d: /* alt+m */
				 inskey("\x1b\x6d");
				 break;
			       case 0x31d: /* alt+n */
				 inskey("\x1b\x6e");
				 break;
			       case 0x315: /* alt+o */
				 inskey("\x1b\x6f");
				 break;
			       case 0x30f: /* alt+p */
				 inskey("\x1b\x70");
				 break;
			       case 0x31f: /* alt+q */
				 inskey("\x1b\x71");
				 break;
			       case 0x317: /* alt+r */
				 inskey("\x1b\x72");
				 break;
			       case 0x30e: /* alt+s */
				 inskey("\x1b\x73");
				 break;
			       case 0x31e: /* alt+t */
				 inskey("\x1b\x74");
				 break;
			       case 0x325: /* alt+u */
				 inskey("\x1b\x75");
				 break;
			       case 0x327: /* alt+v */
				 inskey("\x1b\x76");
				 break;
			       case 0x33a: /* alt+w */
				 inskey("\x1b\x77");
				 break;
			       case 0x32d: /* alt+x */
				 inskey("\x1b\x78");
				 break;
			       case 0x33d: /* alt+y */
				 inskey("\x1b\x79");
				 break;
			       case 0x335: /* alt+z */
				 inskey("\x1b\x7a");
				 break;
			       case 0x321: /* alt+1 */
				 inskey("\x1b\x31");
				 break;
			       case 0x323: /* alt+2 */
				 inskey("\x1b\x32");
				 break;
			       case 0x329: /* alt+3 */
				 inskey("\x1b\x33");
				 break;
			       case 0x339: /* alt+4 */
				 inskey("\x1b\x34");
				 break;
			       case 0x331: /* alt+5 */
				 inskey("\x1b\x35");
				 break;
			       case 0x32b: /* alt+6 */
				 inskey("\x1b\x36");
				 break;
			       case 0x33b: /* alt+7 */
				 inskey("\x1b\x37");
				 break;
			       case 0x333: /* alt+8 */
				 inskey("\x1b\x38");
				 break;
			       case 0x32a: /* alt+9 */
				 inskey("\x1b\x39");
				 break;
			       case 0x33c: /* alt+0 */
				 inskey("\x1b\x30");
				 break;
			       case 0x224: /* del key */
				 inskey("\33[3~");
				 break;
			       default:
				 message("! Undef Braille key ", 0);
				 break;
			      }
			    break;
			 }
		    }
		  break;
		case 'S':		/* status information */
		  if (buf[p + 1] == 'I')
		/* we take the third letter from the model identification
		 * string as the amount of cells available and the two first letters
		 * for the model name in order to optimize the key configuration.
		 */
		    if (!NbCols)
		      {
			 if (buf[p + 2] == 'N')
			   if (buf[p + 3] == 'B')
			     model_ID = 1;
			 if (buf[p + 2] == 'C')
			   if (buf[p + 3] == 'N')
			     model_ID = 2;
			 if (buf[p + 2] == 'S')
			   if (buf[p + 3] == 'C')
			     model_ID = 3;
			 if (buf[p + 2] == 'C')
			   if (buf[p + 3] == 'Z')
			     model_ID = 4;
			 NbCols = (buf[p + 4] - '0') * 10;
		  /* Looking now at the internal version of the terminal... */
			 for (position=24; position<44; position++)
			   version_ID[position - 24] = buf[position];
		      }
		  ReWrite = 1;
		  break;
	       }
	     p += lg;
	  }
     }

   return (res);
}
