/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
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
 *   August Hörandl <hoerandl@elina.htlw1.ac.at>
 *
 * papenmeier/brl.c - Braille display library for Papenmeier Screen 2D
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

static FILE* dbg = NULL;
static char dbg_buffer[256];

static unsigned char prevline[PMSC] = "";
static unsigned char prev[BRLCOLS+1]= "";

static void init_table();

/* special table for papenmeier */
#define B1 1
#define B2 2
#define B3 4
#define B4 8
#define B5 16
#define B6 32
#define B7 64
#define B8 128

static void brl_debug(char * print_buffer)
{
  if (! dbg)
    dbg =  fopen ("/tmp/brltty-pm.log", "w");
  fprintf(dbg, "%s\n", print_buffer);
  fflush(dbg);
}

static void initbrlerror(brldim *brl)
{
  printf("\nInitbrl: failure at open\n");
  if (brl->disp)
    free (brl->disp);
  brl->x = -1;
}


static void initbrl (brldim *brl, const char *dev)
{
  brldim res;			/* return result */
  struct termios newtio;	/* new terminal settings */

  res.x = BRLCOLS;		/* initialise size of display */
  res.y = BRLROWS;
  res.disp = NULL;		/* clear pointers */

  /* Now open the Braille display device for random access */
  brl_fd = open (dev, O_RDWR | O_NOCTTY);
  if (brl_fd < 0) {
    initbrlerror(brl);
    return;
  }

  tcgetattr (brl_fd, &oldtio);	/* save current settings */

  /* Set bps, flow control and 8n1, enable reading */
  newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;

  /* Ignore bytes with parity errors and make terminal raw and dumb */
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;		/* raw output */
  newtio.c_lflag = 0;		/* don't echo or generate signals */
  newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
  newtio.c_cc[VTIME] = 0;
  tcflush (brl_fd, TCIFLUSH);	/* clean line */
  tcsetattr (brl_fd, TCSANOW, &newtio);		/* activate new settings */

  /* Allocate space for buffers */
  res.disp = (unsigned char *) malloc (res.x * res.y);
  if (!res.disp)
    initbrlerror(&res);

  init_table();

  *brl = res;
  return;
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
  LogAndStderr(LOG_NOTICE, "Papenmeier driver");
  LogAndStderr(LOG_INFO, "   Copyright (C) 1998 HTL W1 <hoerandl@elina.htlw1.ac.at>.");
# ifdef RD_DEBUG
  LogAndStderr(LOG_INFO, "   Input debugging enabled.");
# endif
# ifdef WR_DEBUG
  LogAndStderr(LOG_INFO, "   Output debugging enabled.");
# endif
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
  char line[BRLCOLS];
  char spalte[PMSC];
  int i;
  // don´t use the internal table for the status column 
  for(i=0; i < PMSC; i++)
    spalte[i] = 1; /* 1 = no table */
  write_to_braille(offsetTable+offsetVertical, PMSC, spalte);

  // don´t use the internal table for the line
  for(i=0; i < BRLCOLS; i++)
    line[i] = 1; // 1 = no table
  write_to_braille(offsetTable+offsetHorizontal, BRLCOLS, line);
}

static void
setbrlstat(const unsigned char* s)
{
 unsigned char buffer[PMSC];
 int i;
 for (i=0; i<PMSC; ++i)
   buffer[i] = change_bits[s[i]];
 if (memcmp(buffer, prevline, PMSC) != 0)
   {
     memcpy(prevline, buffer, PMSC);
     write_to_braille(offsetVertical, PMSC, prevline);
   }
}

static void
writebrl (brldim *brl)
{
  int i;

#ifdef WR_DEBUG
  sprintf(dbg_buffer, "write %2d %2d %80s", brl->x, brl->y, brl->disp);
  brl_debug(dbg_buffer);
#endif

  for(i=0; i < BRLCOLS; i++)
    brl->disp[i] = change_bits[brl->disp[i]];

  if (memcmp(prev,brl->disp,BRLCOLS) != 0)
    {
      memcpy(prev,brl->disp,BRLCOLS);
      write_to_braille(offsetHorizontal, BRLCOLS, prev);
    }
}

/* ------------------------------------------------------------ */

/* some makros */
/* read byte */
#define READ(OFFS) \
  if (safe_read(brl_fd,buf+OFFS,1) != 1) \
      return EOF;                   \

/* read byte and check value */
#define READ_CHK(OFFS, VAL)  \
    { READ(OFFS);            \
      if (buf[OFFS] != VAL)  \
        return CMD_ERR;      \
    }


#ifdef RD_DEBUG
#define KEY(CODE, VAL) \
     case CODE: brl_debug("readbrl: " #VAL); \
       return VAL;
#else
#define KEY(CODE, VAL) \
     case CODE: return VAL;
#endif


static int readbrl (int xx)
{
  unsigned char buf [20];
  int i, l;

  static int beg_pressed = 0;
  static int end_pressed = 0;

  READ_CHK(0, cSTX);		/* STX - Start */
# ifdef RD_DEBUG
  sprintf(dbg_buffer, "read: STX");
  brl_debug(dbg_buffer);
# endif

  READ_CHK(1, cIdReceive);	/* 'K' */
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
    sprintf(dbg_buffer, "read: ");
    for(i=0; i<l; i++)
      sprintf(dbg_buffer + 6 + 3*i, " %02x",buf[i]);
    brl_debug(dbg_buffer);
# endif

  if (buf[l-1] != cETX)		/* ETX - End */
    return CMD_ERR;

  if( buf[6] == PRESSED) 
    {
      int Kod = 0x100*buf[2] + buf[3];
      switch(Kod) 
	{
	  /* braille window movement */
	  /* Taste Unten - keys at the front of papenmeier terminal */
	  /* Taste Seite - keys at the status column on left hand side */
	  /* layout 7321 UP H S E DWN 4568 */

	  KEY(0x0006, CMD_ATTRUP);   /* Taste Unten "3"  */ 
	  KEY(0x0009, CMD_WINUP);    /* Taste Unten "2"  */    
	  KEY(0x000c, CMD_PRDIFLN);  /* Taste Unten "1"  */
	  KEY(0x000f, CMD_LNUP   );  /* Taste Unten /\   */

	  KEY(0x0012, CMD_TOP);	     /* Taste Unten "H"  */
	  KEY(0x0015, CMD_HOME);     /* Taste Unten "S"  */ 
	  KEY(0x0018, CMD_BOT);	     /* Taste Unten "E"  */     

	  KEY(0x001b, CMD_LNDN);     /* Taste Unten \/   */
	  KEY(0x001e, CMD_NXDIFLN);  /* Taste Unten "4"  */
	  KEY(0x0021, CMD_WINDN);    /* Taste Unten "5"  */
	  KEY(0x0024, CMD_ATTRDN);   /* Taste Unten "6"  */ 

	  /* misc */
	  KEY(0x0300, CMD_HELP);    /* Taste Seite "1"  */
	  KEY(0x306, CMD_CSRJMP_VERT); /* Taste Seite "3" */
	  KEY(0x309, CMD_MUTE); /* Taste Seite "4" */
	  KEY(0x30C, CMD_SAY); /* Taste Seite "5" */

	  KEY(0x030F, CMD_CSRTRK);  /* Taste Seite "6"  */
	  KEY(0x0312, CMD_DISPMD);  /* Taste Seite "7"  */
	  KEY(0x0315, CMD_INFO);    /* Taste Seite "8"  */
	  KEY(0x0318, CMD_FREEZE);  /* Taste Seite "9"  */

	  /* Configuration commands */
	  KEY(0x031B, CMD_CONFMENU);   /* Taste Seite "10" */
	  KEY(0x031E, CMD_SAVECONF);   /* Taste Seite "11" */
	  KEY(0x0321, CMD_RESET);      /* Taste Seite "12" */
							  
	  /* Configuration options */			  
	  KEY(0x0324, CMD_CSRVIS);    /* Taste Seite"13" */
	  KEY(0x0327, CMD_CSRSIZE);   /* Taste Seite"14" */
	  KEY(0x032A, CMD_CSRBLINK);  /* Taste Seite"15" */
	  KEY(0x032D, CMD_CAPBLINK);  /* Taste Seite"16" */
	  KEY(0x0330, CMD_SIXDOTS);   /* Taste Seite"17" */
	  KEY(0x0333, CMD_SND);       /* Taste Seite"18" */
	  KEY(0x0336, CMD_SKPIDLNS);  /* Taste Seite"19" */
	  KEY(0x0339, CMD_ATTRVIS);   /* Taste Seite"20" */
	  KEY(0x033c, CMD_ATTRBLINK); /* Taste Seite"21" */
	  /* Cut & Paste */
	  KEY(0x033f, CMD_PASTE);     /*  Taste Seite"22" */

	  /* reset status col */
	case 0x303: 
	  init_table();
	  write_to_braille(offsetVertical, PMSC, prevline);
	  write_to_braille(offsetHorizontal, BRLCOLS, prev);
	  return CMD_RESTARTBRL;

	  /* cut: begin, end - set flag */
	case 0x0003: /* Taste Unten "7"  */
	  beg_pressed = 1;
	  return EOF;
	case 0x0027: /* Taste Unten "8"  */
	  end_pressed = 1;	  
	  return EOF;

	default:
	  /* Routing Keys */
	  if (0x342 <=  Kod && Kod <= 0x42f)  
	    if (beg_pressed) /* Cut Begin */
	      return (Kod - 0x342)/3 + CR_BEGBLKOFFSET;
	    else if (end_pressed) /* Cut End */
	      return (Kod - 0x342)/3 + CR_ENDBLKOFFSET;
	    else  /* CSR Jump */
	      return (Kod - 0x342)/3 + CR_ROUTEOFFSET;
	  else
	    {
	      sprintf(dbg_buffer, "readbrl: Command Error - CmdKod:%d", Kod);
	      brl_debug(dbg_buffer);
	      return CMD_ERR;
	    }
	}
    }
  else {
    beg_pressed = end_pressed = 0;
    return EOF;	/* return key press from braille display */
  }
}








