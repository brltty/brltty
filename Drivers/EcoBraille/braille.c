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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "Programs/brl.h"
#include "Programs/misc.h"

#include "Programs/brl_driver.h"
#include "braille.h"
#include "Programs/serial.h"

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
static TranslationTable outputTable;

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
static char BRL_READY[]
#ifdef HAVE_ATTRIBUTE_UNUSED
       __attribute__((unused))
#endif /* HAVE_ATTRIBUTE_UNUSED */
       = "\x10\x02\x2E";
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


static int brl_open(BrailleDisplay *brl, char **parameters, const char *device)
{
  struct termios newtio;	/* new terminal settings */
  short ModelID = MODEL;
  unsigned char buffer[DIM_BRL_ID + 6];

  {
    static const DotsTable dots = {0X10, 0X20, 0X40, 0X01, 0X02, 0X04, 0X80, 0X08};
    makeOutputTable(&dots, &outputTable);
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  rawdata = NULL;	/* clear pointers */

  /* Open the Braille display device */
  if (!openSerialDevice(device, &brl_fd, &oldtio)) goto failure;
  
#ifdef DEBUG
  brl_log = open("/tmp/brllog", O_CREAT | O_WRONLY);
  if(brl_log < 0){
    goto failure;
  }
#endif /* DEBUG */

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
      restartSerialDevice(brl_fd, &newtio, BAUDRATE);	/* activate new settings */
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
  brl->helpPage = ModelID;
  brl->x = model->Cols;		/* initialise size of main display */
  brl->y = BRLROWS;		/* ever is 1 in this type of braille lines */
  
  /* Need to calculate the size; Cols + Status + 1 (space between) */
  BrailleSize = brl->x + model->NbStCells + 1;

  /* Allocate space for buffers */
  rawdata = (unsigned char *) malloc (BrailleSize); /* Phisical size */
  if(!rawdata){
     goto failure;
  }    

  /* Empty buffers */
  memset(rawdata, 0, BrailleSize);
  memset(Status, 0, MAX_STCELLS);

return 1;

failure:;
  if(rawdata){
     free(rawdata);
  }
       
return 0;
}


static void brl_close(BrailleDisplay *brl)
{
  free(rawdata);
  putSerialAttributes(brl_fd, &oldtio);	/* restore terminal settings */
  close(brl_fd);

#ifdef DEBUG  
  close(brl_log);
#endif /* DEBUG */
}


static void brl_writeWindow(BrailleDisplay *brl)
{
  int i, j;

  /* This Braille Line need to display all information, include status */
  
  /* Make status info to rawdata */
  for(i=0; i < model->NbStCells; i++)
      rawdata[i] = outputTable[Status[i]];

  i++;  /* step a phisical space with main cells */
  
  /* Make main info to rawdata */
  for(j=0; j < brl->x; j++)
      rawdata[i++] = outputTable[brl->buffer[j]];
     
  /* Write to Braille Display */
  WriteToBrlDisplay(rawdata);
}


static void brl_writeStatus(BrailleDisplay *brl, const unsigned char *st)
{
  /* Update status cells */
  memcpy(Status, st, model->NbStCells);
}


static int brl_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext context)
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
	        res = BRL_CMD_HELP;
	        break;

	   case KEY_ST_SENSOR2:
	        res = BRL_CMD_PREFMENU;
	        break;

	   case KEY_ST_SENSOR3:
	        res = BRL_CMD_DISPMD;
	        break;

	   case KEY_ST_SENSOR4:
	        res = BRL_CMD_INFO;
	        break;
        }

	/* Check Main Sensors */
	if(*(pBuff+3) >= KEY_MAIN_MIN && *(pBuff+3) <= KEY_MAIN_MAX){
	
	   /* Nothing */
	}
	
	/* Byte B. Check Front Keys */
	switch(*(pBuff+4)){
	   case KEY_DOWN: /* Down */
	        res = BRL_CMD_LNDN;
	        break;

	   case KEY_RIGHT: /* Right */
	        res = BRL_CMD_FWINRT;
	        break;

	   case KEY_CLICK: /* Eco20 Go to cursor */
	   
	        /* Only for ECO20, haven't function keys */
		if(model->Cols==20){
	           res = BRL_CMD_HOME;
		}
	        break;

	   case KEY_LEFT: /* Left */
	        res = BRL_CMD_FWINLT;
	        break;

	   case KEY_UP: /* Up  */
	        res = BRL_CMD_LNUP;
	        break;

	   case KEY_UP|KEY_CLICK: /* Top of screen  */
	        return(BRL_CMD_TOP);
	        break;

	   case KEY_DOWN|KEY_CLICK: /* Bottom of screen */
	        return(BRL_CMD_BOT);
	        break;

	   case KEY_LEFT|KEY_CLICK: /* Left one half window */
	        return(BRL_CMD_HWINLT);
	        break;

	   case KEY_RIGHT|KEY_CLICK: /* Right one half window */
	        return(BRL_CMD_HWINRT);
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
		     return(BRL_CMD_CSRTRK);
		}
	        break;
        }
	

	/* Byte D. Rest of Function Keys */
	switch(*(pBuff+6)){
	   case KEY_F1:
	        /* Nothing */
	        break;

	   case KEY_F2:  /* go to cursor */
                res = BRL_CMD_HOME;
	        break;

	   case KEY_F3:
	        /* Nothing */
	        break;

	   case KEY_F4:
	        /* Nothing */
	        break;

	   case KEY_F5: /* togle cursor visibility */
	        res = BRL_CMD_CSRVIS;
	        break;

	   case KEY_F6:
	        /* Nothing */
	        break;

	   case KEY_F7:
	        /* Nothing */
	        break;

	   case KEY_F8: /* Six dot mode */
	        res = BRL_CMD_SIXDOTS;
	        break;
        }
     }
  }
  
return(res);
}
