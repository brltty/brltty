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
#define VS_VERSION "BRLTTY driver for VisioBraille, version 0.2, 2002"
#define VS_COPYRIGHT "Copyright Sebastien HINDERER <Sebastien.Hinderer@libertysurf.fr"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "Programs/misc.h"
#include "Programs/scr.h"
#include "Programs/message.h"

typedef enum {
  PARM_DISPSIZE=0,
  PARM_PROMVER=1
} DriverParameter;
#define BRLPARMS "displaysize", "promVersion"

#define BRL_HAVE_PACKET_IO
#define BRL_HAVE_KEY_CODES
#include "Programs/brl_driver.h"
#include "braille.h"
#include "brldefs-vs.h"
#include "Programs/serial.h"

#define MAXPKTLEN 512

static SerialDevice *serialDevice;
#ifdef SendIdReq
static struct TermInfo {
  unsigned char code; 
  unsigned char version[3];
  unsigned char f1;
  unsigned char size[2];
  unsigned char dongle;
  unsigned char clock;
  unsigned char routing;
  unsigned char flash;
  unsigned char prog;
  unsigned char lcd;
  unsigned char f2[11];
} terminfo;
#endif /* SendIdReq */

static int printcode = 0;

/* Function : brl_writePacket */ 
/* Sends a packet of size bytes, stored at address p to the braille terminal */
/* Returns 0 if everything is right, -1 if an error occured while sending */
static ssize_t brl_writePacket(BrailleDisplay *brl, const unsigned char *p, size_t size)
{
  int lgtho = 1;
  static unsigned char obuf[MAXPKTLEN] = { 02 };
  const unsigned char *x;
  unsigned char *y = obuf+1;
  unsigned char chksum=0;
  int i,res;
  for (x=p; (x-p) < size; x++) {
    chksum ^= *x;
    if ((*x) <= 5) {
      *y = 01;
      y++; lgtho++; 
      *y = ( *x ) | 0x40;
    } else *y = *x;
    y++; lgtho++; 
  }
  if (chksum<=5) {
    *y = 1; y++; lgtho++;  
    chksum |= 0x40;
  }
  *y = chksum; y++; lgtho++; 
  *y = 3; y++; lgtho++; 
  for (i=1; i<=5; i++) {
    if (serialWriteData(serialDevice,obuf,lgtho) != lgtho) continue; /* write failed, retry */
    serialDrainOutput(serialDevice);
    res = serialReadData(serialDevice,&chksum,1,1000,0);
    if ((res==1) && (chksum == 0x04)) return 0;
  }
  return (-1);
}

/* Function : brl_readPacket */
/* Reads a packet of at most size bytes from the braille terminal */
/* and puts it at the specified adress */
/* Packets are read into a local buffer until completed and valid */
/* and are then copied to the buffer pointed by p. In this case, */
/* The size of the packet is returned */
/* If a packet is too long, it is discarded and a message sent to the syslog */
/* "+" packets are silently discarded, since they are only disturbing us */
static ssize_t brl_readPacket(BrailleDisplay *brl, unsigned char *p, size_t size) 
{
  int tmp = 0;
  static unsigned char ack = 04;
  static unsigned char nack = 05;
  static int apacket = 0;
  static unsigned char prefix, checksum;
  unsigned char ch;
  static unsigned char buf[MAXPKTLEN]; 
  static unsigned char *q;
  if ((p==NULL) || (size<2) || (size>MAXPKTLEN)) return 0; 
  while (serialReadChunk(serialDevice,&ch,&tmp,1,0,1000)) {
    if (ch==0x02) {
      apacket = 1;
      prefix = 0xff; 
      checksum = 0;
      q = &buf[0];
    } else if (apacket) {
      if (ch==0x01) {
        prefix &= ~(0x40); 
      } else if (ch==0x03) {
        if (checksum==0) {
          serialWriteData(serialDevice,&ack,1); 
          apacket = 0; q--;
          if (buf[0]!='+') {
            memcpy(p,buf,(q-buf));
            return q-&buf[0]; 
          }
        } else {
          serialWriteData(serialDevice,&nack,1);
          apacket = 0;
          return 0;
        }
      } else {
        if ((q-&buf[0])>=size) {
          LogPrint(LOG_WARNING,"Packet too long: discarded");
          apacket = 0;
          return 0;
        }
        ch &= prefix; prefix |= 0x40;
        checksum ^= ch;
        (*q) = ch; q++;   
      }
    }
    tmp = 0;
  } 
  return 0;
}

/* Function : brl_reset */
/* This routine is called by the brlnet server, when an application that */
/* requested a raw-mode communication with the braille terminal dies before */
/* restoring a normal communication mode */
static int brl_reset(BrailleDisplay *brl)
{
  static unsigned char *RescuePacket = "#"; 
  brl_writePacket(brl,RescuePacket,strlen(RescuePacket));
  return 1;
}

/* Function : brl_identify */
/* Prints information about the driver in the system log and on stderr */
static void brl_identify()
{
  LogPrint(LOG_NOTICE, VS_VERSION);
  LogPrint(LOG_INFO,   "   "VS_COPYRIGHT);
}

/* Function : brl_open */
/* Opens and configures the serial port properly */
/* if brl->x <= 0 when brl_open is called, then brl->x is initialized */
/* either with the BRAILLEDISPLAYSIZE constant, defined in braille.h */
/* or with the size got through identification request if it succeeds */
/* Else, brl->x is left unmodified by brl_open, so that */
/* the braille display can be resized without reloading the driver */ 
static int brl_open(BrailleDisplay *brl, char **parameters, const char *device)
{
#ifdef SendIdReq
  unsigned char ch = '?';
  int i;
#endif /* SendIdReq */
  int ds = BRAILLEDISPLAYSIZE;
  int promVersion = 4;
  if (*parameters[PARM_DISPSIZE]) {
    int dsmin=20, dsmax=40;
    validateInteger(&ds, "Size of braille display",parameters[PARM_DISPSIZE],&dsmin,&dsmax);
  }
  if (*parameters[PARM_PROMVER]) {
    int pvmin=3, pvmax=6;
    validateInteger(&promVersion, "PROM version",parameters[PARM_PROMVER],&pvmin,&pvmax);
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }
  if (!(serialDevice = serialOpenDevice(device))) return 0;
  serialSetParity(serialDevice, SERIAL_PARITY_ODD);
  if (promVersion<4) serialSetFlowControl(serialDevice, SERIAL_FLOW_INPUT_CTS);
  serialRestartDevice(serialDevice,57600); 
#ifdef SendIdReq
  {
    brl_writePacket(brl,(unsigned char *) &ch,1); 
    i=5; 
    while (i>0) {
      if (brl_readPacket(brl,(unsigned char *) &terminfo,sizeof(terminfo))!=0) {
        if (terminfo.code=='?') {
          terminfo.f2[10] = '\0';
          break;
        }
      }
      i--;
    }
    if (i==0) {
      LogPrint(LOG_WARNING,"Unable to identify terminal properly");  
      if (brl->x<=0) brl->x = BRAILLEDISPLAYSIZE;  
    } else {
      LogPrint(LOG_INFO,"Braille terminal description:");
      LogPrint(LOG_INFO,"   version=%c%c%c",terminfo.version[0],terminfo.version[1],terminfo.version[2]);
      LogPrint(LOG_INFO,"   f1=%c",terminfo.f1);
      LogPrint(LOG_INFO,"   size=%c%c",terminfo.size[0],terminfo.size[1]);
      LogPrint(LOG_INFO,"   dongle=%c",terminfo.dongle);
      LogPrint(LOG_INFO,"   clock=%c",terminfo.clock);
      LogPrint(LOG_INFO,"   routing=%c",terminfo.routing);
      LogPrint(LOG_INFO,"   flash=%c",terminfo.flash);
      LogPrint(LOG_INFO,"   prog=%c",terminfo.prog);
      LogPrint(LOG_INFO,"   lcd=%c",terminfo.lcd);
      LogPrint(LOG_INFO,"   f2=%s",terminfo.f2);  
      if (brl->x<=0) brl->x = (terminfo.size[0]-'0')*10 + (terminfo.size[1]-'0');
    }
#else /* SendIdReq */
    if (brl->x<=0) brl->x = ds;
#endif /* SendIdReq */
  brl->y=1; 
  return 1;
}

/* Function : brl_close */
/* Closes the braille device and deallocates dynamic structures */
static void brl_close(BrailleDisplay *brl)
{
  if (serialDevice) {
    serialCloseDevice(serialDevice);
  }
}

/* function : brl_writeWindow */
/* Displays a text on the braille window, only if it's different from */
/* the one alreadz displayed */
static void brl_writeWindow(BrailleDisplay *brl)
{
  /* The following table defines how internal brltty format is converted to */
  /* VisioBraille format. Do *NOT* modify this table. */
  /* The table is declared static so that it is in data segment and not */
  /* in the stack */ 
  static unsigned char dotsTable[256] = {
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
  static unsigned char brailleDisplay[81]= { 0x3e }; /* should be large enough for everyone */
  static unsigned char prevData[80];
  int i;
  if (memcmp(&prevData,brl->buffer,brl->x)==0) return;
  for (i=0; i<brl->x; i++) brailleDisplay[i+1] = dotsTable[*(brl->buffer+i)];
  if (brl_writePacket(brl,(unsigned char *) &brailleDisplay,brl->x+1)==0) {
    memcpy(&prevData,brl->buffer,brl->x);
  }
}

/* Function : brl_writeStatus */
/* normally, this function should write status cells */
/* Actually it does nothing, since VisioBraille terminals have no such cells */
static void brl_writeStatus(BrailleDisplay *brl, const unsigned char *s)
{
}

/* Function : brl_keyToCommand */
/* Converts a key code to a brltty command according to the context */
int brl_keyToCommand(BrailleDisplay *brl, BRL_DriverCommandContext context, int code)
{
  static int ctrlpressed = 0; 
  static int altpressed = 0;
  static int cut = 0;
  static int descchar = 0;
  unsigned char ch;
  int type;
  ch = code & 0xff;
  type = code & (~ 0xff);
  if (code==0) return 0;
  if (code==EOF) return EOF;
  if (type==BRL_VSMSK_CHAR) return ch | BRL_BLK_PASSCHAR | altpressed | ctrlpressed | (altpressed = ctrlpressed = 0);
  else if (type==BRL_VSMSK_ROUTING) {
    ctrlpressed = altpressed = 0;
    switch (cut) {
      case 0:
        if (descchar) { descchar = 0; return ((int) ch) | BRL_BLK_DESCCHAR; }
        else return ((int) ch) | BRL_BLK_ROUTE;
      case 1: cut++; return ((int) ch) | BRL_BLK_CUTBEGIN;
      case 2: cut = 0; return ((int) ch) | BRL_BLK_CUTLINE;
    }
    return EOF; /* Should not be reached */
  } else if (type==BRL_VSMSK_FUNCTIONKEY) {
    ctrlpressed = altpressed = 0;
    switch (code) {
      case BRL_VSKEY_A1: return BRL_BLK_SWITCHVT;
      case BRL_VSKEY_A2: return BRL_BLK_SWITCHVT+1;
      case BRL_VSKEY_A3: return BRL_BLK_SWITCHVT+2;
      case BRL_VSKEY_A6: return BRL_BLK_SWITCHVT+3;
      case BRL_VSKEY_A7: return BRL_BLK_SWITCHVT+4;
      case BRL_VSKEY_A8: return BRL_BLK_SWITCHVT+5;
      case BRL_VSKEY_B5: cut = 1; return EOF;
      case BRL_VSKEY_B6: return BRL_CMD_TOP_LEFT; 
      case BRL_VSKEY_D6: return BRL_CMD_BOT_LEFT;
      case BRL_VSKEY_A4: return BRL_CMD_FWINLTSKIP;
      case BRL_VSKEY_B8: return BRL_CMD_FWINLTSKIP;
      case BRL_VSKEY_A5: return BRL_CMD_FWINRTSKIP;
      case BRL_VSKEY_D8: return BRL_CMD_FWINRTSKIP;
      case BRL_VSKEY_B7: return BRL_CMD_LNUP;
      case BRL_VSKEY_D7: return BRL_CMD_LNDN;
      case BRL_VSKEY_C8: return BRL_CMD_FWINRT;
      case BRL_VSKEY_C6: return BRL_CMD_FWINLT;
      case BRL_VSKEY_C7: return BRL_CMD_HOME;
      case BRL_VSKEY_B2: return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP;
      case BRL_VSKEY_D2: return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN;
      case BRL_VSKEY_C3: return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT;
      case BRL_VSKEY_C1: return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT;
      case BRL_VSKEY_B3: return BRL_CMD_CSRVIS;
      case BRL_VSKEY_D1: return BRL_BLK_PASSKEY + BRL_KEY_DELETE;  
      case BRL_VSKEY_D3: return BRL_BLK_PASSKEY + BRL_KEY_INSERT;
      case BRL_VSKEY_C5: return BRL_CMD_PASTE;
      case BRL_VSKEY_D5: descchar = 1; return EOF;
      case BRL_VSKEY_B4: printcode = 1; return EOF;
      default: return EOF;
    }
  } else if (type==BRL_VSMSK_OTHER) {
    /* ctrlpressed = 0; */
    if ((ch>=0xe1) && (ch<=0xea)) {
      ch-=0xe1;
      if (altpressed) {
        altpressed = 0;
        return BRL_BLK_SWITCHVT + ch;
      } else return BRL_BLK_PASSKEY+BRL_KEY_FUNCTION+ch; 
    }
    /* altpressed = 0; */
    switch (code) {
      case BRL_VSKEY_PLOC_LT: return BRL_CMD_SIXDOTS;
      case BRL_VSKEY_BACKSPACE: return BRL_BLK_PASSKEY + BRL_KEY_BACKSPACE;
      case BRL_VSKEY_TAB: return BRL_BLK_PASSKEY + BRL_KEY_TAB;
      case BRL_VSKEY_RETURN: return BRL_BLK_PASSKEY + BRL_KEY_ENTER;
      case BRL_VSKEY_PLOC_PLOC_A: return BRL_CMD_HELP;
      case BRL_VSKEY_PLOC_PLOC_B: return BRL_CMD_TUNES; 
      case BRL_VSKEY_PLOC_PLOC_C: return BRL_CMD_PREFMENU;
      case BRL_VSKEY_PLOC_PLOC_D: return BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN;
      case BRL_VSKEY_PLOC_PLOC_E: return BRL_BLK_PASSKEY + BRL_KEY_END;
      case BRL_VSKEY_PLOC_PLOC_F: return BRL_CMD_FREEZE;
      case BRL_VSKEY_PLOC_PLOC_H: return BRL_BLK_PASSKEY + BRL_KEY_HOME;
      case BRL_VSKEY_PLOC_PLOC_I: return BRL_CMD_INFO;
      case BRL_VSKEY_PLOC_PLOC_R: return BRL_CMD_PREFLOAD;
      case BRL_VSKEY_PLOC_PLOC_S: return BRL_CMD_PREFSAVE;
      case BRL_VSKEY_PLOC_PLOC_T: return BRL_CMD_CSRTRK;
      case BRL_VSKEY_PLOC_PLOC_U: return BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP;
      case BRL_VSKEY_CONTROL: ctrlpressed = BRL_FLG_CHAR_CONTROL; break; 
      case BRL_VSKEY_ALT: altpressed = BRL_FLG_CHAR_META; break;   
      case BRL_VSKEY_ESCAPE: return BRL_BLK_PASSKEY + BRL_KEY_ESCAPE;
      default: return EOF;
    }
  }
 return EOF; 
}

/* Function : brl_readKey */
/* Reads a key. The result is context-independent */
/* The intermediate value contains a keycode, masked with the key type */
/* the keytype is one of BRL_NORMALCHAR, BRL_FUNCTIONKEY or BRL_ROUTING */
/* for a normal character, the keycode is the latin1-code of the character */
/* for function-keys, codes 0 to 31 are reserved for A1 to D8 keys */
/* codes after 32 are for ~~* combinations, the order has to be determined */
/* for BRL_ROUTING, the code is the ofset to route, starting from 0 */
static int brl_readKey(BrailleDisplay *brl)
{
  unsigned char ch, ibuf[MAXPKTLEN-1];
  static int routing = 0;
  ssize_t lgthi;
  readNextPacket:
  lgthi = brl_readPacket(brl,(unsigned char *) &ibuf,MAXPKTLEN-1);
  if (lgthi==0) return EOF;
  if (ibuf[0]=='%') {
    ibuf[lgthi] = '\0';
    insertString(&ibuf[1]);
    return EOF;
  }  
  if ((ibuf[0]!=0x3c) && (ibuf[0]!=0x3d) && (ibuf[0]!=0x23)) {
    char buf[100];
    if (ibuf[0]==0x2b) return EOF;
    sprintf(buf,"Unknown packet 0x%x",ibuf[0]);
    message(buf,MSG_WAITKEY | MSG_NODELAY);
    return EOF;
  }
  ch = ibuf[1];
  if (printcode) {
    char buf [100];
    sprintf(buf,"Keycode: 0x%x",ch);
    printcode = 0; /* MUST BE DONE BEFORE THE CALL TO MESSAGE */
    message(buf,MSG_WAITKEY | MSG_NODELAY);
    return EOF;
  }
  if (routing) {
    routing=0;
    if (ch>=0xc0)  return (ibuf[1]-0xc0) | BRL_VSMSK_ROUTING;
    return EOF;
  }
  if ((ch>=0xc0) && (ch<=0xdf)) return (ch-0xc0) | BRL_VSMSK_FUNCTIONKEY;
  if (ch==0x91) {
    routing = 1;
    goto readNextPacket;
  } 
  if ((ch>=0x20) && (ch<=0x9e)) {
    switch (ch) {
      case 0x80: ch = 0xc7; break;
      case 0x81: ch = 0xfc; break;
      case 0x82: ch = 0xe9; break;
      case 0x83: ch = 0xe2; break;
      case 0x84: ch = 0xe4; break;
      case 0x85: ch = 0xe0; break;
      case 0x87: ch = 0xe7; break;
      case 0x88: ch = 0xea; break;
      case 0x89: ch = 0xeb; break;
      case 0x8a: ch = 0xe8; break; 
      case 0x8b: ch = 0xef; break;
      case 0x8c: ch = 0xee; break;
      case 0x8f: ch = 0xc0; break;
      case 0x93: ch = 0xf4; break;
      case 0x94: ch = 0xf6; break;
      case 0x96: ch = 0xfb; break; 
      case 0x97: ch = 0xf9; break;
      case 0x9e: ch = 0x60; break;
    }
    return ch | BRL_VSMSK_CHAR;
  }
  return ch | BRL_VSMSK_OTHER;
}

/* Function : brl_readCommand */
/* Reads a command from the braille keyboard */
static int brl_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext context)
{
  return brl_keyToCommand(brl,context,brl_readKey(brl));
}
