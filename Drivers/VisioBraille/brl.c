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

#define __EXTENSIONS__
#define VERSION "BRLTTY driver for VisioBraille, version 0.1, 2000"
#define BRL_C 1

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#include "brlconf.h"
#include "../brl.h"
#include "../misc.h"
#include "../scr.h"
#include "../brl_driver.h"

#define MAXPKTLEN 512

struct termios oldtio,newtio;

int brl_fd;

unsigned char ibuf[MAXPKTLEN];
unsigned char obuf[MAXPKTLEN];
unsigned char ptl[41];
unsigned char dispbuf[40],prevdata[40];

int lgthi,lgtho;

unsigned char dotstable[256] = {
 0x20,0x41,0x22,0x43,0x2C,0x42,0x49,0x46,0x5F,0x45,0x25,0x44,0x3A,0x48,0x4A,0x47,
 0x27,0x4B,0x2A,0x4D,0x3B,0x4C,0x53,0x50,0x29,0x4F,0x26,0x4E,0x21,0x52,0x54,0x51,
 0x28,0x31,0x23,0x33,0x3F,0x32,0x39,0x36,0x2B,0x35,0x24,0x34,0x2E,0x38,0x57,0x37,
 0x2D,0x55,0x5E,0x58,0x3C,0x56,0x5C,0x40,0x3E,0x5A,0x30,0x59,0x3D,0x5b,0x5D,0x2F,
 0xE0,0x01,0xE2,0x03,0xEC,0x02,0x09,0x06,0x1F,0x05,0xE5,0x04,0xFA,0x08,0x0A,0x07,
 0xE7,0x0B,0xEA,0x0D,0xFB,0x0C,0x13,0x10,0xE9,0x0F,0xE6,0x0E,0xE1,0x12,0x14,0x11,
 0xE8,0xF1,0xE3,0xF3,0xFF,0xF2,0xF9,0xF6,0xEB,0xF5,0xE4,0xF4,0xEE,0xF8,0x17,0xF7,
 0xED,0x15,0x1E,0x18,0xFC,0x16,0x1C,0x00,0xFE,0x1A,0xF0,0x19,0xfd,0x1B,0x1D,0xEF,
 0xA0,0xC1,0xA2,0xC3,0xAC,0xC2,0xC9,0xC6,0xDF,0xC5,0xA5,0xC4,0xBA,0xC8,0xCA,0xC7,
 0xA7,0xCB,0xAA,0xCD,0xBB,0xCC,0xD3,0xD0,0xA9,0xCF,0xA6,0xCE,0xA1,0xD2,0xD4,0xD1,
 0xA8,0xB1,0xA3,0xB3,0xBF,0xB2,0xB9,0xB6,0xAB,0xB5,0xA4,0xB4,0xAE,0xB8,0xD7,0xB7,
 0xAD,0xD5,0xDE,0xD8,0xBC,0xD6,0xDC,0xC0,0xBE,0xDA,0xB0,0xD9,0xBD,0xDB,0xDD,0xAF,
 0x60,0x81,0x62,0x83,0x6C,0x82,0x89,0x86,0x9F,0x85,0x65,0x84,0x7A,0x88,0x8A,0x87,
 0x67,0x8B,0x6A,0x8D,0x7B,0x8C,0x93,0x90,0x69,0x8F,0x66,0x8E,0x61,0x92,0x94,0x91,
 0x68,0x71,0x63,0x73,0x7F,0x72,0x79,0x76,0x6B,0x75,0x64,0x74,0x6E,0x78,0x97,0x77,
 0x6D,0x95,0x9E,0x98,0x7C,0x96,0x9C,0x80,0x7E,0x9A,0x70,0x99,0x7D,0x9B,0x9D,0x6F
};

int apacket = 0; // 1 if we are reading a packet
int routing=0;
unsigned char special = 0; // 0x40 for 01-prefixed characters
unsigned char checksum=0; // xor checksum for the current packet
int ctrlpressed=0,altpressed=0;
unsigned char functionkeys[10][6] = {
 "\33[[A",
 "\33[[B",
 "\33[[C",
 "\33[[D",
 "\33[[E",
 "\33[17~",
 "\33[18~",
 "\33[19~",
 "\33[20~",
 "\33[21~"
};

#define keyA1 0xc0
#define keyA2 0xC1
#define keyA3 0xC2
#define keyA4 0xc3
#define keyA5 0xc4
#define keyA6 0xc5
#define keyA7 0xc6
#define keyA8 0xc7
#define keyB1 0xc8
#define keyB2 0xc9
#define keyB3 0xca
#define keyB4 0xcb
#define keyB5 0xcc
#define keyB6 0xcd
#define keyB7 0xce
#define keyB8 0xcf
#define keyC1 0xd0
#define keyC2 0xd1
#define keyC3 0xd2
#define keyC4 0xd3
#define keyC5 0xd4
#define keyC6 0xd5
#define keyC7 0xd6
#define keyC8 0xd7
#define keyD1 0xd8
#define keyD2 0xd9
#define keyD3 0xda
#define keyD4 0xdb
#define keyD5 0xdc
#define keyD6 0xdd
#define keyD7 0xde
#define keyD8 0xdf

static int isinbounds(unsigned char x,unsigned char a,unsigned char b)
{
 if (a<=x && x<=b) return 1; else return 0;
}

static void brl_identify(void)
{
  LogPrint(LOG_NOTICE, "VisioBraille Driver");
}

static void brl_initialize(char **parameters, brldim *brl,const char *tty)
{
 brl_fd = open(tty, O_RDWR | O_NOCTTY );
 if (brl_fd <0) {LogPrint(LOG_ERR,"impossible d'ouvrir le port serie: %s: %s",tty,strerror(errno)); exit(-1); }
 tcgetattr(brl_fd,&oldtio); /* sauvegarde les parametres actuels du port serie */
 bzero(&newtio, sizeof(newtio)); /*supprime les parametre du port serie pour en mettre des nouveaux */
 newtio.c_cflag = B57600 |  CS8 | PARENB | PARODD | CLOCAL | CREAD;
 newtio.c_iflag = IGNPAR;
 newtio.c_oflag = 0;
 newtio.c_lflag = 0;
 tcsetattr(brl_fd,TCSANOW,&newtio); /* affecte les nouveaux attributs au port serie */
 brl->x=TAILLEPLAGETACTILE; brl->y=1; brl->disp=dispbuf;
 ptl[0]=0x3e;
 obuf[0]=0x02;
}

static void brl_close(brldim *brl)
{
 if (brl_fd>=0)
 {
  tcsetattr(brl_fd,TCSADRAIN,&oldtio);
  close(brl_fd);
 }
}

static int sendpacket(void *p,int size)
{
 unsigned char *buf= (unsigned char *)p;
 unsigned char ch,chksum=0;
 int i,res;
 lgtho=1;
 for (i=0;i<size;i++)
 {
  chksum^=*(buf+i);
  if ((*(buf+i))<=5)
  {
   obuf[lgtho++]=01; 
   ch=(*(buf+i)) + 0x40;
  } else ch=(*(buf+i));
  obuf[lgtho++]=ch; 
 }
 if (chksum<=5)
 {
  obuf[lgtho++]=01; 
  chksum+=0x40;
 }
 obuf[lgtho++]=chksum; obuf[lgtho++]=03; 
 for (i=1; i<=5; i++)
 {
  write(brl_fd,obuf,lgtho);
  tcdrain(brl_fd);
  newtio.c_cc[VTIME]=10;
  tcsetattr(brl_fd,TCSANOW,&newtio);
  res=read(brl_fd,&ch,1);
  newtio.c_cc[VTIME]=0;
  tcsetattr(brl_fd,TCSANOW,&newtio);
  if ((res==1) && (ch=0x04)) return 0;
 }
 return (-1);
}

static int brl_read(DriverCommandContext cmds)
{
 unsigned char kbuf[20]; // Keyboard simulation
 int lgthk=0; // Length of keyboard buffer
 int process=0; // 1 if a packet was successfully read
 unsigned char ch; // current character
 int functionkey=0;
 int res,i;
 while ((process==0)&&(read(brl_fd,&ch,1)==1)) // While we have nothing to process and something to read
 { // We proccess the characters...
  if (ch==0x02) // Begin of a new packet
  {
   lgthi=0; // 0 bytes
   apacket=1;
   checksum=0;
  } else
  if (apacket)
  {
   if (ch==0x01) special=0x40; // Prefix
   else
   if (ch==0x03) // End of a packet, check it and process it if necessary
   {
    if (checksum==0) // The packet is valid
    {
     ch=0x04; write(brl_fd,&ch,1); // Send acknowledge
     lgthi--; // remov1 checksum from input buffer
     process=1; // There is a packet to process
    }
    else // Bad packet, we have to ask TVB to resend it...
    {
     ch=0x05; write(brl_fd,&ch,1);
    }
    apacket=0;
   }
   else // add byte to buffer and update checksum
   {
    ch-=special; special=0;
    checksum^=ch;
    ibuf[lgthi++]=ch; 
   }
  }
 }
 if (process) // process packet
 {
  if ((ibuf[0]==0x3c) | (ibuf[0]==0x3d) | (ibuf[0]==0x23)) // a key was pressed
  {
   if (routing)
   {
    routing=0; res=EOF;
    if (isinbounds(ibuf[1],0xc0,0xE7) && (int)(ibuf[1]-0xC0)<TAILLEPLAGETACTILE)
     res=(int)(ibuf[1]-0xc0)+CR_ROUTE;
    return res;
   }
   functionkey=isinbounds(ibuf[1],0xe1,0xea);
   if (isinbounds(ibuf[1],0x20,0x8F) | functionkey | (ibuf[1]==0x93) | (ibuf[1]==0x94) | (ibuf[1]==0x96) | (ibuf[1]==0x97)) {
    lgthk=0;
    if (altpressed) 
    {
     if (functionkey)
     {
      res=(int)(ibuf[1])-0xE1; // Numéro de la console à mettre en 1er plan
      functionkey=0; // Touche de fonction prise en compte: -> éliminée!
      altpressed=0; // Alt géré, on élimine aussi...
      return CR_SWITCHVT+res;
     } else { kbuf[lgthk]=0x1b; lgthk++; altpressed=0; } 
    }
    if (ctrlpressed)
    {
     if (isinbounds(ibuf[1],'a','z')) { ibuf[1]-=0x60; }
     ctrlpressed=0;
    }
    if (functionkey)
    {
     res=(int)(ibuf[1])-0xE1;
     for (i=0;functionkeys[res][i];i++) kbuf[lgthk+i]=functionkeys[res][i];
     lgthk+=i;
    } else { 
     if (ibuf[1]==0x82) ibuf[1]=0xe9;
     else if (ibuf[1]==0x85) ibuf[1]=0xe0;
     else if (ibuf[1]==0x83) ibuf[1]=0xe2;
     else if (ibuf[1]==0x84) ibuf[1]=0xe4;
     else if (ibuf[1]==0x8a) ibuf[1]=0xe8;
     else if (ibuf[1]==0x88) ibuf[1]=0xea;
     else if (ibuf[1]==0x89) ibuf[1]=0xeb;
     else if (ibuf[1]==0x8b) ibuf[1]=0xef;
     else if (ibuf[1]==0x8c) ibuf[1]=0xee;
     else if (ibuf[1]==0x93) ibuf[1]=0xf4;
     else if (ibuf[1]==0x94) ibuf[1]=0xf6;
     else if (ibuf[1]==0x97) ibuf[1]=0xf9;
     else if (ibuf[1]==0x96) ibuf[1]=0xfb;
     else if (ibuf[1]==0x81) ibuf[1]=0xfc;
     else if (ibuf[1]==0x87) ibuf[1]=0xe7;
     kbuf[lgthk]=ibuf[1]; lgthk++; 
    }
    kbuf[lgthk]=0;
    insertString(kbuf);
   } else switch (ibuf[1]) 
   {
    case 0x01: return CMD_SIXDOTS;
    case 0x08: return VAL_PASSKEY + VPK_BACKSPACE;
    case 0x09: return VAL_PASSKEY + VPK_TAB;
    case 0x0D: return VAL_PASSKEY + VPK_RETURN;
    case 0x91: routing=1; break;
    case 0xA1: return CMD_HELP;
    case 0xA2: return CMD_TUNES; // Toggle bips
    case 0xA3: return CMD_PREFMENU;
    case 0xA4: return VAL_PASSKEY + VPK_PAGE_DOWN;
    case 0xA5: return VAL_PASSKEY + VPK_END;
    case 0xA8: return VAL_PASSKEY + VPK_HOME;
    case 0xA9: return CMD_INFO;
    case 0xB2: return CMD_PREFLOAD;
    case 0xB3: return CMD_PREFSAVE;
    case 0xB5: return VAL_PASSKEY + VPK_PAGE_UP;
    case 0xBE: ctrlpressed=!ctrlpressed; break; // toggle ctrl-state
    case 0xBF: altpressed=!altpressed; break; // toggle alt-state
    case keyA1: return CR_SWITCHVT; // Active console 1
    case keyA2: return CR_SWITCHVT+1; // Active console 2
    case keyA3: return CR_SWITCHVT+2; // Active console 3
    case keyA6: return CR_SWITCHVT+3; // Active console 4
    case keyA7: return CR_SWITCHVT+4; // Active console 5
    case keyA8: return CR_SWITCHVT+5; // Active console 6
    case keyB6: return CMD_TOP_LEFT; ;
    case keyD6: return CMD_BOT_LEFT;
    case keyA4: return CMD_FWINLTSKIP;
    case keyA5: return CMD_FWINRTSKIP;
    case keyB7: return CMD_LNUP;
    case keyD7: return CMD_LNDN;
    case keyC8: return CMD_FWINRT;
    case keyC6: return CMD_FWINLT;
    case keyC7: return CMD_HOME;
    case keyB2: return VAL_PASSKEY + VPK_CURSOR_UP;
    case keyD2: return VAL_PASSKEY + VPK_CURSOR_DOWN;
    case keyC3: return VAL_PASSKEY + VPK_CURSOR_RIGHT;
    case keyC1: return VAL_PASSKEY + VPK_CURSOR_LEFT;
    case keyB3: return CMD_CSRVIS;
    case keyD1: return VAL_PASSKEY + VPK_DELETE;
    default: return EOF;
   } // End of switch statement
  }
  // A packet was received but it's not a key, we ignore it...
  return EOF;
 }
 return EOF; // No packet received
}

static void brl_writeStatus(const unsigned char *s)
{
}

static void brl_writeWindow(brldim *brl)
{
 int i;
 if (memcmp(&prevdata,&dispbuf,40)==0) return;
 memcpy(&prevdata,&dispbuf,TAILLEPLAGETACTILE);
 for (i=0;i<=brl->x-1;i++) ptl[i+1]=dotstable[dispbuf[i]];
 sendpacket(&ptl,brl->x+1);
}
