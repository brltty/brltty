/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.9.0, 06 April 1998
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <s.doyon@videotron.ca>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* EuroBraille/brl.c - Braille display library for the EuroBraille family.
 * Written by Nicolas Pitre <nicolas@visuaide.qc.ca>
 * Copyright (C) 1997 by VisuAide, Inc. <visuinfo@visuaide.qc.ca>
 * (this code has been created with some VisuAide's ressources and routines)
 * See the GNU Public license for details in the ../COPYING file
 *
 * $Id: brl.c,v 1.4 1996/10/03 08:08:13 nn201 Exp $
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
#include "../scr.h"
#include "../misc.h"


static char StartupString[] =
"  EuroBraille driver, version 0.1 \n"
"  Copyright (C) 1997 by Nicolas Pitre and VisuAide, Inc. \n"
"  <nicolas@visuaide.qc.ca>, <visuinfo@visuaide.qc.ca> \n";



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
  0xF4, 0xF5, 0xFC, 0xFD, 0xF6, 0xF7, 0xFE, 0xFF};



/* Global variables */

char DefDev[] = BRLDEV;		/* default braille device */
int brl_fd;			/* file descriptor for Braille display */
struct termios oldtio;		/* old terminal settings */
unsigned char *rawdata;		/* translated data to send to Braille */
unsigned char *prevdata;	/* previously sent raw data */
int NbCols = 0;			/* number of cells available */
short ReWrite = 0;		/* 1 if display need to be rewritten */



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




int
sendbyte (unsigned char c)
{
  return (write (brl_fd, &c, 1));
}


int
WriteToBrlDisplay (int len, char *data)
{
  static int PktNbr = 128;
  int parity = 0;

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
    default:
      sendbyte (parity);
    }
  sendbyte (EOT);
  return (1);
}


void
identbrl (const char *dev)
{
  /* Hello display... */
  printf (StartupString);
  printf ("  - device = %s\n", (dev) ? dev : DefDev);
}


brldim
initbrl (const char *dev)
{
  brldim res;			/* return result */
  struct termios newtio;	/* new terminal settings */

  res.disp = rawdata = prevdata = NULL;		/* clear pointers */

  /* Open the Braille display device for random access */
  if (!dev)
    dev = DefDev;
  brl_fd = open (dev, O_RDWR | O_NOCTTY);
  if (brl_fd < 0)
    goto failure;
  tcgetattr (brl_fd, &oldtio);	/* save current settings */

  /* Set 8E1, enable reading, parity generation, etc. */
  newtio.c_cflag = CS8 | CLOCAL | CREAD | PARENB;
  newtio.c_iflag &= ~(IGNPAR | PARMRK);
  newtio.c_iflag |= INPCK;
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
      {2, 'S', 'I'};
      WriteToBrlDisplay (3, AskIdent);
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
    goto failure;

  ReWrite = 1;			/* To write whole display at first time */

  return res;

failure:;
  if (res.disp)
    free (res.disp);
  if (rawdata)
    free (rawdata);
  if (prevdata)
    free (prevdata);
  res.x = -1;
  return res;
}


void
closebrl (brldim brl)
{
  free (brl.disp);
  free (rawdata);
  free (prevdata);
  tcsetattr (brl_fd, TCSANOW, &oldtio);		/* restore terminal settings */
  close (brl_fd);
}


void
writebrl (brldim brl)
{
  int i, j;

  if (!ReWrite)
    {
      /* We update the display only if it has changed */
      i = NbCols;
      while (--i >= 0)
	if (brl.disp[i] != prevdata[i])
	  {
	    ReWrite = 1;
	    break;
	  }
    }
  if (ReWrite)
    {
      /* right end cells don't have to be transmitted if all dots down */
      i = NbCols;
      while (--i > 0)		/* at least the first cell must go through... */
	if (brl.disp[i] != 0)
	  break;
      i++;

      {
	char OutBuf[2 * i + 6];
	char *p = OutBuf;
	*p++ = i + 2;
	*p++ = 'D';
	*p++ = 'X';
	for (j = 0; j < i; *p++ = ' ', j++);	/* fill LCD with whitespaces */
	*p++ = i + 2;
	*p++ = 'D';
	*p++ = 'Y';
	for (j = 0;
	     j < i;
	     *p++ = TransTable[(prevdata[j++] = brl.disp[j])]);
	WriteToBrlDisplay (p - OutBuf, OutBuf);
      }
      ReWrite = 0;
    }
}


void
setbrlstat (const unsigned char *st)
{
  /* sorry but the ClioBraille I got here doesn't have any status cells... 
   * Don't know either if any EuroBraille terminal have some. 
   */
}



int
readbrl (int type)
{
  int res = EOF;
  unsigned char c;
  static unsigned char buf[DIM_INBUFSZ];
  static int DLEflag = 0;
  static int pos = 0, p = 0, pktready = 0, OffsetType = CR_ROUTEOFFSET;

  /* here we process incoming data */
  while (!pktready && read (brl_fd, &c, 1))
    {
      if (DLEflag)
	{
	  buf[pos++] = c;
	  DLEflag = 0;
	}
      else
	switch (c)
	  {
	  case NACK:
	    ReWrite = 1;
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
	      unsigned int parity = 0;
	      if (pos < 4)
		break;		/* packets can't be shorter */
	      for (i = 0; i < pos - 1; i++)
		parity ^= buf[i];
	      if (parity != buf[pos - 1])
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
	    case 'K':		/* Keyboard -- here are the key bindings */
	      switch (buf[p + 1])
		{
		case 'T':	/* Control Keyboard */
		  switch (buf[p + 2])
		    {
		    case 'E':
		      res = CMD_HELP;
		      break;
		    case 'F':
		      res = CMD_INFO;
		      break;
		    case 'G':
		      res = CMD_CONFMENU;
		      break;
		    case 'H':
		      break;
		    case 'I':
		      break;
		    case 'J':
		      break;
		    case 'K':
		      OffsetType = CR_BEGBLKOFFSET;
		      break;
		    case 'L':
		      OffsetType = CR_ENDBLKOFFSET;
		      break;
		    case 'M':
		      res = CMD_PASTE;
		      break;
		    case 'A':
		      break;
		    case 'B':
		      break;
		    case 'C':
		      break;
		    case 'D':
		      break;
		    case '1':
		      res = CMD_TOP_LEFT;
		      break;
		    case '2':
		      res = CMD_LNUP;
		      break;
		    case '3':
		      res = CMD_PRDIFLN;
		      break;
		    case '4':
		      res = CMD_FWINLT;
		      break;
		    case '5':
		      res = CMD_HOME;
		      break;
		    case '6':
		      res = CMD_FWINRT;
		      break;
		    case '7':
		      res = CMD_BOT_LEFT;
		      break;
		    case '8':
		      res = CMD_LNDN;
		      break;
		    case '9':
		      res = CMD_NXDIFLN;
		      break;
		    case '*':
		      res = CMD_DISPMD;
		      break;
		    case '0':
		      res = CMD_CSRTRK;
		      break;
		    case '#':
		      res = CMD_FREEZE;
		      break;
		    }
		  break;
		case 'I':	/* Routing Key */
		  res = OffsetType + buf[p + 2] - 1;
		  OffsetType = CR_ROUTEOFFSET;  /* return to default */
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
		      case 0x025:	/* braille 'u' */
			res = CMD_LNUP;
			break;
		      case 0x019:	/* braille 'd' */
			res = CMD_LNDN;
			break;
		      case 0x007:	/* braille 'l' */
			res = CMD_FWINLT;
			break;
		      case 0x017:	/* braille 'r' */
			res = CMD_FWINRT;
			break;
		      case 0x013:	/* braille 'h' */
			res = CMD_HOME;
			break;
		      }
		    break;
		  }
		}
	      break;
	    case 'S':		/* status information */
	      if (buf[p + 1] == 'I')
		/* we take the third letter from the model identification 
		 * string as the amount of cells available... should work?
		 */
		if (!NbCols)
		  NbCols = (buf[p + 4] - '0') * 10;
	      break;
	    }
	  p += lg;
	}
    }

  return (res);
}
