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
 * This Driver was written as a project in the
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

char DefDev[] = BRLDEV;		/* default braille device */
int brl_fd = 0;			/* file descriptor for Braille display */
struct termios oldtio;		/* old terminal settings */

static FILE* dbg = NULL;
static char dbg_buffer[256];

static char prevline[PMSC] = "";
static char prev[BRLCOLS+1]= "";

void init_table();

void brl_debug(char * print_buffer)
{
  if (! dbg)
    dbg =  fopen ("/tmp/brltty.log", "w");
  fprintf(dbg, "%s\n", print_buffer);
  fflush(dbg);
}

brldim initbrlerror(brldim res)
{
  printf("\nInitbrl: failure at open\n");
  if (res.disp)
    free (res.disp);
  res.x = -1;
  return res;
}


brldim initbrl (const char *dev)
{
  brldim res;			/* return result */
  struct termios newtio;	/* new terminal settings */

  res.x = BRLCOLS;		/* initialise size of display */
  res.y = BRLROWS;
  res.disp = NULL;		/* clear pointers */

  /* Now open the Braille display device for random access */
  if (!dev)
    dev = DefDev;

  brl_fd = open (dev, O_RDWR | O_NOCTTY);
  if (brl_fd < 0) {
    res = initbrlerror(res);
    return res;
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
    res = initbrlerror(res);

  init_table();

  return res;
}

void
closebrl (brldim brl)
{
  free (brl.disp);
  tcsetattr (brl_fd, TCSANOW, &oldtio);	/* restore terminal settings */
  close (brl_fd);
}

void
identbrl (const char *dev)
{
  printf(BRLNAME " driver\n"
	 "Copyright (C) 1998 HTL W1 <hoerandl@elina.htlw1.ac.at>\n"
	 "Device = %s\n", (dev) ? dev : DefDev);
}

void 
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
  
  write(brl_fd,BrlHead, sizeof(BrlHead));
  write(brl_fd, data, size);
  write(brl_fd,BrlTrail, sizeof(BrlTrail));
}

void 
init_table()
{
  // don´t use the internal table for the status column 
  char spalte[PMSC];
  int i;
  for(i=0; i < PMSC; i++)
    spalte[i] = 1; // 1 = no table
  write_to_braille(offsetTable+offsetVertical, PMSC, spalte);
}

void 
pm_notable()
{
  /* only Papenmeier: don´t use internal table */
  char line[BRLCOLS];
  int i;
  for(i=0; i < BRLCOLS; i++)
    line[i] = 1; // 1 = no table
  write_to_braille(offsetTable+offsetHorizontal, BRLCOLS, line);
}

void
setbrlstat(const unsigned char* s)
{
 if (memcmp(s, prevline, PMSC) != 0)
   {
     memcpy(prevline, s, PMSC);
     write_to_braille(offsetVertical, PMSC, prevline);
   }
}

void
writebrl (brldim brl)
{
  if (strncmp(prev,brl.disp,BRLCOLS) != 0)
    {
      strncpy(prev,brl.disp,BRLCOLS);
  
#ifdef WR_DEBUG
	sprintf(dbg_buffer, "write %2d %2d %80s", brl.x, brl.y, brl.disp);
	brl_debug(dbg_buffer);
#endif

      write_to_braille(offsetHorizontal, BRLCOLS, prev);
    }
}

/* ------------------------------------------------------------ */

/* some makros */
/* read byte */
#define READ(OFFS) \
  if (read(brl_fd,buf+OFFS,1) != 1) \
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


int readbrl (int xx)
{
  unsigned char buf [20];
  int i, l;

  static int beg_pressed = 0;
  static int end_pressed = 0;

  READ_CHK(0, cSTX);		/* STX - Start */
  sprintf(dbg_buffer, "read: STX");
  brl_debug(dbg_buffer);

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
	  KEY(0x001b, CMD_LNDN );       /* Taste Unten \/   */
	  KEY(0x000c, CMD_PRDIFLN   );  /* Taste Unten "1"  */
	  KEY(0x000f, CMD_LNUP   );     /* Taste Unten /\   */
	  KEY(0x001e, CMD_NXDIFLN   );  /* Taste Unten "4"  */
	  KEY(0x0009, CMD_WINUP   );    /* Taste Unten "2"  */    
	  KEY(0x0021, CMD_WINDN   );    /* Taste Unten "5"  */
	  KEY(0x0012, CMD_TOP    );     /* Taste Unten "H"  */
	  KEY(0x0018, CMD_BOT    );     /* Taste Unten "E"  */     

	  /* misc */
	  KEY(0x0015, CMD_HOME  );  /* Taste Unten "S"  */ 
	  					       
	  KEY(0x0300, CMD_HELP  );  /* Taste Seite "1"  */

	  KEY(0x030F, CMD_CSRTRK  );/* Taste Seite "6"  */
	  KEY(0x0312, CMD_DISPMD  );/* Taste Seite "7"  */
	  KEY(0x0315, CMD_INFO  );  /* Taste Seite "8"  */
	  KEY(0x0318, CMD_FREEZE  );/* Taste Seite "9"  */

	  /* Configuration commands */
	  KEY(0x031B, CMD_CONFMENU );  /* Taste Seite "10" */
	  KEY(0x031E, CMD_SAVECONF );  /* Taste Seite "11" */
	  KEY(0x0321, CMD_RESET     ); /* Taste Seite "12" */
							  
	  /* Configuration options */			  
	  KEY(0x0324, CMD_CSRVIS );    /* Taste Seite"13" */
	  KEY(0x0327, CMD_CSRSIZE  );  /* Taste Seite"14" */
	  KEY(0x032A, CMD_CSRBLINK );  /* Taste Seite"15" */
	  KEY(0x032D, CMD_CAPBLINK  ); /* Taste Seite"16" */
	  KEY(0x0330, CMD_SIXDOTS   ); /* Taste Seite"17" */
	  KEY(0x0333, CMD_SLIDEWIN  ); /* Taste Seite"18" */
	  KEY(0x0336, CMD_SND   );     /* Taste Seite"19" */
	  KEY(0x0339, CMD_SKPIDLNS  ); /* Taste Seite"20" */

	  /* Cut & Paste */
	  KEY(0x0027, CMD_PASTE);  /* Taste Unten "8"  */

	  /* reset status col */
	case 0x303: 
	  init_table();
	  write_to_braille(offsetVertical, PMSC, prevline);
	  write_to_braille(offsetHorizontal, BRLCOLS, prev);
	  return EOF;

	  /* cut: begin, end - set flag */
	case 0x0003: /* Taste Unten "7"  */
	  beg_pressed = 1;
	  return EOF;

	case 0x0006: /* Taste Unten "3"  */
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








