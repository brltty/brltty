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

/* EcoBraille/braille.c - Braille display library for ECO Braille series
 * Copyright (C) 1999 by Oscar Fernandez <ofa@once.es>
 * See the GNU Public license for details in the ../COPYING file
 *
 * For debuging define DEBUG variable
 */

/* Changes:
 *      mar 1' 2000:
 *              - fix correct size of braille lines.
 */


#define BRL_C 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <string.h>

#include "brlconf.h"
#include "Programs/brl.h"
#include "Programs/scr.h"
#include "Programs/misc.h"
#include "Programs/brl_driver.h"

/* Braille display parameters */
typedef struct{
    char *Name;
    int Cols;
    int NbStCells;
} BRLPARAMS;

static BRLPARAMS Models[NB_MODEL] ={
  {
    /* ID == 0 */
    "ECO20",
    20,
    2
  }
  ,
  {
    /* ID == 1 */
    "ECO40",
    40,
    4
  }
  ,
  {
    /* ID == 2 */
    "ECO80",
    80,
    4
  }
};

#define BRLROWS		1
#define MAX_STCELLS	4	/* hiest number of status cells */

/* Eco dot translate table. This Braille Line use Spanish Braille file 
 * text.es.tbl for default
 */
static char TransTable[256] ={
	0x00, 0x10, 0x01, 0x11, 0x20, 0x30, 0x21, 0x31, 
	0x02, 0x12, 0x03, 0x13, 0x22, 0x32, 0x23, 0x33, 
	0x40, 0x50, 0x41, 0x51, 0x60, 0x70, 0x61, 0x71, 
	0x42, 0x52, 0x43, 0x53, 0x62, 0x72, 0x63, 0x73, 
	0x04, 0x14, 0x05, 0x15, 0x24, 0x34, 0x25, 0x35, 
	0x06, 0x16, 0x07, 0x17, 0x26, 0x36, 0x27, 0x37, 
	0x44, 0x54, 0x45, 0x55, 0x64, 0x74, 0x65, 0x75, 
	0x46, 0x56, 0x47, 0x57, 0x66, 0x76, 0x67, 0x77, 
	0x00, 0x90, 0x81, 0x91, 0xA0, 0xB0, 0xA1, 0xB1, 
	0x82, 0x92, 0x83, 0x93, 0xA2, 0xB2, 0xA3, 0xB3, 
	0xC0, 0xD0, 0xC1, 0xD1, 0xE0, 0xF0, 0xE1, 0xF1, 
	0xC2, 0xD2, 0xC3, 0xD3, 0xE2, 0xF2, 0xE3, 0xF3, 
	0x84, 0x94, 0x85, 0x95, 0xA4, 0xB4, 0xA5, 0xB5, 
	0x86, 0x96, 0x87, 0x97, 0xA6, 0xB6, 0xA7, 0xB7, 
	0xC4, 0xD4, 0xC5, 0xD5, 0xE4, 0xF4, 0xE5, 0xF5, 
	0xC6, 0xD6, 0xC7, 0xD7, 0xE6, 0xF6, 0xE7, 0xF7, 
	0x08, 0x18, 0x09, 0x19, 0x28, 0x38, 0x29, 0x39, 
	0x0A, 0x1A, 0x0B, 0x1B, 0x2A, 0x3A, 0x2B, 0x3B, 
	0x48, 0x58, 0x49, 0x59, 0x68, 0x78, 0x69, 0x79, 
	0x4A, 0x5A, 0x4B, 0x5B, 0x6A, 0x7A, 0x6B, 0x7B, 
	0x0C, 0x1C, 0x0D, 0x1D, 0x2C, 0x3C, 0x2D, 0x3D, 
	0x0E, 0x1E, 0x0F, 0x1F, 0x4E, 0x3E, 0x2F, 0x3F, 
	0x4C, 0x5C, 0x4D, 0x5D, 0x6C, 0x7C, 0x6D, 0x7D, 
	0x00, 0x5E, 0x4F, 0x5F, 0x6E, 0x7E, 0x6F, 0x7F, 
	0x88, 0x98, 0x89, 0x99, 0xA8, 0xB8, 0xA9, 0xB9, 
	0x8A, 0x9A, 0x8B, 0x9B, 0xAA, 0xBA, 0xAB, 0xBB, 
	0xC8, 0xD8, 0xC9, 0xD9, 0xE8, 0xF8, 0xE9, 0xF9, 
	0xCA, 0xDA, 0xCB, 0xDB, 0xEA, 0xFA, 0xEB, 0xFB, 
	0x8C, 0x9C, 0x8D, 0x9D, 0xAC, 0xBC, 0xAD, 0xBD, 
	0x8E, 0x9E, 0x8F, 0x9F, 0xAE, 0xBE, 0xAF, 0xBF, 
	0xCC, 0xDC, 0xCD, 0xDD, 0xEC, 0xFC, 0xED, 0xFD, 
	0xCE, 0xDE, 0xCF, 0xDF, 0xEE, 0xFE, 0xEF, 0xFF
};

/* Global variables */
static int brl_fd;			/* file descriptor for Braille display */
static struct termios oldtio;		/* old terminal settings */
static unsigned char *rawdata;		/* translated data to send to Braille */
static unsigned char Status[MAX_STCELLS]; /* to hold status */
static BRLPARAMS *model;		/* points to terminal model config struct */
static int BrailleSize=0;		/* Braille size of braille line */

#ifdef DEBUG
int brl_log;
#endif /* DEBUG */

/* Communication codes */
static char BRL_ID[] = "\x10\x02\xF1";
#define DIM_BRL_ID 3
static char SYS_READY[] = "\x10\x02\xF1\x57\x57\x57\x10\x03";
#define DIM_SYS_READY 8
static char BRL_READY[] __attribute__((unused)) = "\x10\x02\x2E";
#define DIM_BRL_READY 3
static char BRL_WRITE_PREFIX[] = "\x61\x10\x02\xBC";
#define DIM_BRL_WRITE_PREFIX 4
static char BRL_WRITE_SUFIX[] = "\x10\x03";
#define DIM_BRL_WRITE_SUFIX 2
static char BRL_KEY[] = "\x10\x02\x88";
#define DIM_BRL_KEY 2


/* Status Sensors */
#define KEY_ST_SENSOR1	0xD5  /* Byte A */
#define KEY_ST_SENSOR2	0xD6
#define KEY_ST_SENSOR3	0xD0
#define KEY_ST_SENSOR4	0xD1

/* Main Sensors */
#define KEY_MAIN_MIN	0x80
#define KEY_MAIN_MAX	0xCF

/* Front Keys */
#define KEY_DOWN	0x01  /* Byte B */
#define KEY_RIGHT	0x02
#define KEY_CLICK	0x04
#define KEY_LEFT	0x08
#define KEY_UP		0x10

/* Function Keys */
#define KEY_F9		0x01  /* byte C */
#define KEY_ALT		0x02
#define KEY_F0		0x04
#define KEY_SHIFT	0x40

#define KEY_F1		0x01  /* Byte D */
#define KEY_F2		0x02
#define KEY_F3		0x04
#define KEY_F4		0x08
#define KEY_F5		0x10
#define KEY_F6		0x20
#define KEY_F7		0x40
#define KEY_F8		0x80


static int WriteToBrlDisplay(char *Data)
{
  int size = DIM_BRL_WRITE_PREFIX + DIM_BRL_WRITE_SUFIX + BrailleSize;
  char *buffTmp;
  
  /* Make temporal buffer */
  buffTmp = (unsigned char *) malloc(size);

  /* Copy the prefix, Data and sufix */
  memcpy(buffTmp, BRL_WRITE_PREFIX, DIM_BRL_WRITE_PREFIX);
  memcpy(buffTmp + DIM_BRL_WRITE_PREFIX, Data, BrailleSize);
  memcpy(buffTmp + DIM_BRL_WRITE_PREFIX + BrailleSize, BRL_WRITE_SUFIX, DIM_BRL_WRITE_SUFIX); 
  
  /* write directly to Braille Line  */
  write(brl_fd, buffTmp, size);
  
  /* Destroy temporal buffer */
  free(buffTmp);
  
return(0);
}


static void brl_identify(void)
{
  LogPrint(LOG_NOTICE, "EcoBraille driver, version 1.00");
  LogPrint(LOG_INFO, "   Copyright (C) 1999 by Oscar Fernandez <ofa@once.es>.");
}


static void brl_initialize(char **parameters, brldim *brl, const char *dev)
{
  brldim res;			/* return result */
  struct termios newtio;	/* new terminal settings */
  short ModelID = MODEL;
  unsigned char buffer[DIM_BRL_ID + 6];

  res.disp = rawdata = NULL;	/* clear pointers */

  /* Open the Braille display device */
  brl_fd = open(dev, O_RDWR | O_NOCTTY);
  if(brl_fd < 0){
    goto failure;
  }
  
#ifdef DEBUG
  brl_log = open("/tmp/brllog", O_CREAT | O_WRONLY);
  if(brl_log < 0){
    goto failure;
  }
#endif /* DEBUG */
      
  tcgetattr (brl_fd, &oldtio);	/* save current settings */

  /* Set 8n1, enable reading */
  newtio.c_cflag = CS8 | CLOCAL | CREAD;

  /* Set xon/xoff control, Ignore bytes with parity errors and make terminal raw and dumb */
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;		/* raw output */
  newtio.c_lflag = 0;		/* don't echo or generate signals */
  newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
  newtio.c_cc[VTIME] = 0;

  /* autodetecting ECO model */
  do{
      /* DTR back on */
      cfsetispeed(&newtio, BAUDRATE);
      cfsetospeed(&newtio, BAUDRATE);
      tcsetattr(brl_fd, TCSANOW, &newtio);	/* activate new settings */
      delay(600);				/* give time to send ID string */
      
      /* The 2 next lines can be commented out to try autodetect once anyway */
      if(ModelID != ECO_AUTO){
	break;
      }
      	
      if(read(brl_fd, &buffer, DIM_BRL_ID + 6) == DIM_BRL_ID + 6){
	  if(!strncmp (buffer, BRL_ID, DIM_BRL_ID)){
	  
	    /* Possible values; 0x20, 0x40, 0x80 */
	    int tmpModel=buffer[DIM_BRL_ID] / 0x20;

	    switch(tmpModel){
	     case 1: ModelID=0;
	       break;
	     case 2: ModelID=1;
	       break;
	     case 4: ModelID=2;
	       break;
	    default: ModelID=1;
	    }
	  }
      }
  }while(ModelID == ECO_AUTO);
  
  if(ModelID >= NB_MODEL || ModelID < 0){
    goto failure;		/* unknown model */
  }
    
  /* Need answer to BR */
  /*do{*/
      strcpy(buffer, SYS_READY);
      
      if(write(brl_fd, &buffer, DIM_SYS_READY) == DIM_SYS_READY){
         delay(100);
      }
      
      read(brl_fd, &buffer, DIM_BRL_READY + 6);
      /*}while(strncmp (buffer, BRL_READY, DIM_BRL_READY));*/
      
      LogPrint(LOG_DEBUG, "buffer is: %s",buffer);
  
  /* Set model params */
  model = &Models[ModelID];
  setHelpPageNumber (ModelID);
  res.x = model->Cols;		/* initialise size of main display */
  res.y = BRLROWS;		/* ever is 1 in this type of braille lines */
  
  /* Need to calculate the size; Cols + Status + 1 (space between) */
  BrailleSize = res.x + model->NbStCells + 1;

  /* Allocate space for buffers */
  res.disp = (char *) malloc (res.x * res.y);  	    /* for compatibility */
  rawdata = (unsigned char *) malloc (BrailleSize); /* Phisical size */
  if(!res.disp || !rawdata){
     goto failure;
  }    

  /* Empty buffers */
  memset(res.disp, 0, res.x*res.y);
  memset(rawdata, 0, BrailleSize);
  memset(Status, 0, MAX_STCELLS);

  *brl = res;
return;

failure:;
  if(res.disp){
     free(res.disp);
  }
  
  if(rawdata){
     free(rawdata);
  }
       
  brl->x = -1;
  
return;
}


static void brl_close(brldim *brl)
{
  free(brl->disp);
  free(rawdata);
  tcsetattr(brl_fd, TCSADRAIN, &oldtio);	/* restore terminal settings */
  close(brl_fd);

#ifdef DEBUG  
  close(brl_log);
#endif /* DEBUG */
}


static void brl_writeWindow(brldim *brl)
{
  int i, j;

  /* This Braille Line need to display all information, include status */
  
  /* Make status info to rawdata */
  for(i=0; i < model->NbStCells; i++)
      rawdata[i] = TransTable[Status[i]];

  i++;  /* step a phisical space with main cells */
  
  /* Make main info to rawdata */
  for(j=0; j < brl->x; j++)
      rawdata[i++] = TransTable[brl->disp[j]];
     
  /* Write to Braille Display */
  WriteToBrlDisplay(rawdata);
}


static void brl_writeStatus(const unsigned char *st)
{
  /* Update status cells */
  memcpy(Status, st, model->NbStCells);
}


static int brl_read(DriverCommandContext cmds)
{
  int res = EOF;
  long bytes = 0;
  unsigned char *pBuff;
  unsigned char buff[18 + 1];
  
#ifdef DEBUG
  char tmp[80];
#endif /* DEBUG */

  /* Read info from Braille Line */
  if((bytes = read(brl_fd, buff, 18)) >= 9){

#ifdef DEBUG
     sprintf(tmp, "Type %d, Bytes read: %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
        type, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7], buff[8], buff[9]);
     write(brl_log, tmp, strlen(tmp)); 
#endif /* DEBUG */
  
     /* Is a Key? */
     pBuff=strstr(buff, BRL_KEY);
     if(!strncmp(pBuff, BRL_KEY, DIM_BRL_KEY)){  
    
        /* Byte A. Check Status sensors */
	switch(*(pBuff+3)){
	   case KEY_ST_SENSOR1:
	        res = CMD_HELP;
	        break;

	   case KEY_ST_SENSOR2:
	        res = CMD_PREFMENU;
	        break;

	   case KEY_ST_SENSOR3:
	        res = CMD_DISPMD;
	        break;

	   case KEY_ST_SENSOR4:
	        res = CMD_INFO;
	        break;
        }

	/* Check Main Sensors */
	if(*(pBuff+3) >= KEY_MAIN_MIN && *(pBuff+3) <= KEY_MAIN_MAX){
	
	   /* Nothing */
	}
	
	/* Byte B. Check Front Keys */
	switch(*(pBuff+4)){
	   case KEY_DOWN: /* Down */
	        res = CMD_LNDN;
	        break;

	   case KEY_RIGHT: /* Right */
	        res = CMD_FWINRT;
	        break;

	   case KEY_CLICK: /* Eco20 Go to cursor */
	   
	        /* Only for ECO20, haven't function keys */
		if(model->Cols==20){
	           res = CMD_HOME;
		}
	        break;

	   case KEY_LEFT: /* Left */
	        res = CMD_FWINLT;
	        break;

	   case KEY_UP: /* Up  */
	        res = CMD_LNUP;
	        break;

	   case KEY_UP|KEY_CLICK: /* Top of screen  */
	        return(CMD_TOP);
	        break;

	   case KEY_DOWN|KEY_CLICK: /* Bottom of screen */
	        return(CMD_BOT);
	        break;

	   case KEY_LEFT|KEY_CLICK: /* Left one half window */
	        return(CMD_HWINLT);
	        break;

	   case KEY_RIGHT|KEY_CLICK: /* Right one half window */
	        return(CMD_HWINRT);
	        break;
        }

	/* Byte C. Some Function Keys */
	switch(*(pBuff+5)){
	   case KEY_F9:
	        /* Nothing */
	        break;

           case KEY_ALT:
	        /* Nothing */
	        break;

	   case KEY_F0:
	        /* Nothing */
	        break;

	   case KEY_SHIFT: /* Cursor traking */
		if(*(pBuff+6)==KEY_F8){
		     return(CMD_CSRTRK);
		}
	        break;
        }
	

	/* Byte D. Rest of Function Keys */
	switch(*(pBuff+6)){
	   case KEY_F1:
	        /* Nothing */
	        break;

	   case KEY_F2:  /* go to cursor */
                res = CMD_HOME;
	        break;

	   case KEY_F3:
	        /* Nothing */
	        break;

	   case KEY_F4:
	        /* Nothing */
	        break;

	   case KEY_F5: /* togle cursor visibility */
	        res = CMD_CSRVIS;
	        break;

	   case KEY_F6:
	        /* Nothing */
	        break;

	   case KEY_F7:
	        /* Nothing */
	        break;

	   case KEY_F8: /* Six dot mode */
	        res = CMD_SIXDOTS;
	        break;
        }
     }
  }
  
return(res);
}
