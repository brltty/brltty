/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "scr.h"
#include "message.h"

typedef enum {
  PARM_DISPSIZE=0,
  PARM_PROMVER=1,
  PARM_BAUD=2
} DriverParameter;
#define BRLPARMS "displaysize", "promversion", "baud"

#define BRL_HAVE_PACKET_IO
#define BRL_HAVE_KEY_CODES
#include "brl_driver.h"
#include "braille.h"
#include "brldefs-vs.h"
#include "io_serial.h"

#define MAXPACKETSIZE 512

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
static ssize_t brl_writePacket(BrailleDisplay *brl, const void *packet, size_t size)
{
  const unsigned char *p = packet;
  int lgtho = 1;
  unsigned char obuf[MAXPACKETSIZE];
  const unsigned char *x;
  unsigned char *y = obuf;
  unsigned char chksum=0;
  int i,res;

  *y++ = 02;
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
    serialAwaitInput(serialDevice, 1000);
    res = serialReadData(serialDevice,&chksum,1,0,0);
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
static ssize_t brl_readPacket(BrailleDisplay *brl, void *p, size_t size) 
{
  size_t offset = 0;
  static unsigned char ack = 04;
  static unsigned char nack = 05;
  static int apacket = 0;
  static unsigned char prefix, checksum;
  unsigned char ch;
  static unsigned char buf[MAXPACKETSIZE]; 
  static unsigned char *q;
  if ((p==NULL) || (size<2) || (size>MAXPACKETSIZE)) return 0; 
  while (serialReadChunk(serialDevice,&ch,&offset,1,0,1000)) {
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
    offset = 0;
  } 
  return 0;
}

/* Function : brl_reset */
/* This routine is called by the brlnet server, when an application that */
/* requested a raw-mode communication with the braille terminal dies before */
/* restoring a normal communication mode */
static int brl_reset(BrailleDisplay *brl)
{
  static unsigned char RescuePacket[] = {'#'}; 
  brl_writePacket(brl,RescuePacket,sizeof(RescuePacket));
  return 1;
}

/* Function : brl_construct */
/* Opens and configures the serial port properly */
/* if brl->textColumns <= 0 when brl_construct is called, then brl->textColumns is initialized */
/* either with the BRAILLEDISPLAYSIZE constant, defined in braille.h */
/* or with the size got through identification request if it succeeds */
/* Else, brl->textColumns is left unmodified by brl_construct, so that */
/* the braille display can be resized without reloading the driver */ 
static int brl_construct(BrailleDisplay *brl, char **parameters, const char *device)
{
#ifdef SendIdReq
  unsigned char ch = '?';
  int i;
#endif /* SendIdReq */
  int ds = BRAILLEDISPLAYSIZE;
  int promVersion = 4;
  int ttyBaud = 57600;
  if (*parameters[PARM_DISPSIZE]) {
    int dsmin=20, dsmax=40;
    if (!validateInteger(&ds, parameters[PARM_DISPSIZE], &dsmin, &dsmax))
      LogPrint(LOG_WARNING, "%s: %s", "invalid braille display size", parameters[PARM_DISPSIZE]);
  }
  if (*parameters[PARM_PROMVER]) {
    int pvmin=3, pvmax=6;
    if (!validateInteger(&promVersion, parameters[PARM_PROMVER], &pvmin, &pvmax))
      LogPrint(LOG_WARNING, "%s: %s", "invalid PROM version", parameters[PARM_PROMVER]);
  }
  if (*parameters[PARM_BAUD]) {
    int baud;
    if (serialValidateBaud(&baud, "TTY baud", parameters[PARM_BAUD], NULL)) {
      ttyBaud = baud;
    }
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }
  if (!(serialDevice = serialOpenDevice(device))) return 0;
  serialSetParity(serialDevice, SERIAL_PARITY_ODD);
  if (promVersion<4) serialSetFlowControl(serialDevice, SERIAL_FLOW_INPUT_CTS);
  serialRestartDevice(serialDevice,ttyBaud); 
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
      if (!brl->textColumns) brl->textColumns = BRAILLEDISPLAYSIZE;  
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
      if (brl->textColumns<=0) brl->textColumns = (terminfo.size[0]-'0')*10 + (terminfo.size[1]-'0');
    }
#else /* SendIdReq */
    brl->textColumns = ds;
#endif /* SendIdReq */
  brl->textRows=1; 
  return 1;
}

/* Function : brl_destruct */
/* Closes the braille device and deallocates dynamic structures */
static void brl_destruct(BrailleDisplay *brl)
{
  if (serialDevice) {
    serialCloseDevice(serialDevice);
  }
}

/* function : brl_writeWindow */
/* Displays a text on the braille window, only if it's different from */
/* the one alreadz displayed */
static int brl_writeWindow(BrailleDisplay *brl, const wchar_t *text)
{
  /* The following table defines how internal brltty format is converted to */
  /* VisioBraille format. */
  /* The table is declared static so that it is in data segment and not */
  /* in the stack */ 
  static const TranslationTable outputTable = {
    [0] = 0X20,
    [BRL_DOT1] = 0X41,
    [BRL_DOT1 | BRL_DOT2] = 0X42,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3] = 0X4C,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4] = 0X50,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X51,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X2F,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XEF,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X6F,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XAF,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X11,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X91,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XD1,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X40,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X00,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X80,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XC0,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0X10,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X90,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0XD0,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5] = 0X52,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X5b,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X1B,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X9B,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XDB,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0X12,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X92,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0XD2,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6] = 0X56,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0X16,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X96,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0XD6,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT7] = 0X0C,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0X8C,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT8] = 0XCC,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4] = 0X46,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5] = 0X47,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X37,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XF7,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X77,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XB7,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X07,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X87,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XC7,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6] = 0X36,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0XF6,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X76,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XB6,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT7] = 0X06,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X86,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT8] = 0XC6,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT5] = 0X48,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6] = 0X38,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XF8,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X78,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XB8,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT7] = 0X08,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X88,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT8] = 0XC8,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT6] = 0X32,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT6 | BRL_DOT7] = 0XF2,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X72,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT6 | BRL_DOT8] = 0XB2,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT7] = 0X02,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT7 | BRL_DOT8] = 0X82,
    [BRL_DOT1 | BRL_DOT2 | BRL_DOT8] = 0XC2,
    [BRL_DOT1 | BRL_DOT3] = 0X4B,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4] = 0X4D,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X4E,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X59,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X19,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X99,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XD9,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X0E,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X8E,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XCE,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X58,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X18,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X98,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XD8,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0X0D,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X8D,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0XCD,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT5] = 0X4F,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X5A,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X1A,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X9A,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XDA,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0X0F,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X8F,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0XCF,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT6] = 0X55,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0X15,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X95,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0XD5,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT7] = 0X0B,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0X8B,
    [BRL_DOT1 | BRL_DOT3 | BRL_DOT8] = 0XCB,
    [BRL_DOT1 | BRL_DOT4] = 0X43,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT5] = 0X44,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X34,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XF4,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X74,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XB4,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X04,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X84,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XC4,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT6] = 0X33,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0XF3,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X73,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XB3,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT7] = 0X03,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X83,
    [BRL_DOT1 | BRL_DOT4 | BRL_DOT8] = 0XC3,
    [BRL_DOT1 | BRL_DOT5] = 0X45,
    [BRL_DOT1 | BRL_DOT5 | BRL_DOT6] = 0X35,
    [BRL_DOT1 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XF5,
    [BRL_DOT1 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X75,
    [BRL_DOT1 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XB5,
    [BRL_DOT1 | BRL_DOT5 | BRL_DOT7] = 0X05,
    [BRL_DOT1 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X85,
    [BRL_DOT1 | BRL_DOT5 | BRL_DOT8] = 0XC5,
    [BRL_DOT1 | BRL_DOT6] = 0X31,
    [BRL_DOT1 | BRL_DOT6 | BRL_DOT7] = 0XF1,
    [BRL_DOT1 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X71,
    [BRL_DOT1 | BRL_DOT6 | BRL_DOT8] = 0XB1,
    [BRL_DOT1 | BRL_DOT7] = 0X01,
    [BRL_DOT1 | BRL_DOT7 | BRL_DOT8] = 0X81,
    [BRL_DOT1 | BRL_DOT8] = 0XC1,
    [BRL_DOT2] = 0X2C,
    [BRL_DOT2 | BRL_DOT3] = 0X3B,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4] = 0X53,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X54,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X5D,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X1D,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X9D,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XDD,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X14,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X94,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XD4,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X5C,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X1C,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X9C,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XDC,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0X13,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X93,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0XD3,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT5] = 0X21,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X3D,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0Xfd,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X7D,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XBD,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0XE1,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X61,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0XA1,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT6] = 0X3C,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0XFC,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X7C,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0XBC,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT7] = 0XFB,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0X7B,
    [BRL_DOT2 | BRL_DOT3 | BRL_DOT8] = 0XBB,
    [BRL_DOT2 | BRL_DOT4] = 0X49,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT5] = 0X4A,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X57,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X17,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X97,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XD7,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X0A,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X8A,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XCA,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT6] = 0X39,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0XF9,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X79,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XB9,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT7] = 0X09,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X89,
    [BRL_DOT2 | BRL_DOT4 | BRL_DOT8] = 0XC9,
    [BRL_DOT2 | BRL_DOT5] = 0X3A,
    [BRL_DOT2 | BRL_DOT5 | BRL_DOT6] = 0X2E,
    [BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XEE,
    [BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X6E,
    [BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XAE,
    [BRL_DOT2 | BRL_DOT5 | BRL_DOT7] = 0XFA,
    [BRL_DOT2 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X7A,
    [BRL_DOT2 | BRL_DOT5 | BRL_DOT8] = 0XBA,
    [BRL_DOT2 | BRL_DOT6] = 0X3F,
    [BRL_DOT2 | BRL_DOT6 | BRL_DOT7] = 0XFF,
    [BRL_DOT2 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X7F,
    [BRL_DOT2 | BRL_DOT6 | BRL_DOT8] = 0XBF,
    [BRL_DOT2 | BRL_DOT7] = 0XEC,
    [BRL_DOT2 | BRL_DOT7 | BRL_DOT8] = 0X6C,
    [BRL_DOT2 | BRL_DOT8] = 0XAC,
    [BRL_DOT3] = 0X27,
    [BRL_DOT3 | BRL_DOT4] = 0X2A,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X26,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X30,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XF0,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X70,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XB0,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0XE6,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X66,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XA6,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X5E,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X1E,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X9E,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XDE,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0XEA,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X6A,
    [BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0XAA,
    [BRL_DOT3 | BRL_DOT5] = 0X29,
    [BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X3E,
    [BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XFE,
    [BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X7E,
    [BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XBE,
    [BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0XE9,
    [BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X69,
    [BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0XA9,
    [BRL_DOT3 | BRL_DOT6] = 0X2D,
    [BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0XED,
    [BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X6D,
    [BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0XAD,
    [BRL_DOT3 | BRL_DOT7] = 0XE7,
    [BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0X67,
    [BRL_DOT3 | BRL_DOT8] = 0XA7,
    [BRL_DOT4] = 0X22,
    [BRL_DOT4 | BRL_DOT5] = 0X25,
    [BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X24,
    [BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XE4,
    [BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X64,
    [BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XA4,
    [BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0XE5,
    [BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X65,
    [BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XA5,
    [BRL_DOT4 | BRL_DOT6] = 0X23,
    [BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0XE3,
    [BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X63,
    [BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XA3,
    [BRL_DOT4 | BRL_DOT7] = 0XE2,
    [BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X62,
    [BRL_DOT4 | BRL_DOT8] = 0XA2,
    [BRL_DOT5] = 0X5F,
    [BRL_DOT5 | BRL_DOT6] = 0X2B,
    [BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XEB,
    [BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X6B,
    [BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XAB,
    [BRL_DOT5 | BRL_DOT7] = 0X1F,
    [BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X9F,
    [BRL_DOT5 | BRL_DOT8] = 0XDF,
    [BRL_DOT6] = 0X28,
    [BRL_DOT6 | BRL_DOT7] = 0XE8,
    [BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X68,
    [BRL_DOT6 | BRL_DOT8] = 0XA8,
    [BRL_DOT7] = 0XE0,
    [BRL_DOT7 | BRL_DOT8] = 0X60,
    [BRL_DOT8] = 0XA0
  };
  static unsigned char brailleDisplay[81]= { 0x3e }; /* should be large enough for everyone */
  static unsigned char prevData[80];
  int i;
  if (memcmp(&prevData,brl->buffer,brl->textColumns)==0) return 1;
  for (i=0; i<brl->textColumns; i++) brailleDisplay[i+1] = outputTable[*(brl->buffer+i)];
  if (brl_writePacket(brl,(unsigned char *) &brailleDisplay,brl->textColumns+1)==0) {
    memcpy(&prevData,brl->buffer,brl->textColumns);
  }
  return 1;
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
  if (type==BRL_VSMSK_CHAR) {
    int command = ch | BRL_BLK_PASSCHAR | altpressed | ctrlpressed;
    altpressed = ctrlpressed = 0;
    return command;
  }
  if (type==BRL_VSMSK_ROUTING) {
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
      int flags = altpressed;
      ch-=0xe1;
      altpressed = 0;
      return flags | BRL_BLK_PASSKEY | ( BRL_KEY_FUNCTION + ch); 
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
      case BRL_VSKEY_PLOC_PLOC_L: return BRL_CMD_LEARN;
      case BRL_VSKEY_PLOC_PLOC_R: return BRL_CMD_PREFLOAD;
      case BRL_VSKEY_PLOC_PLOC_S: return BRL_CMD_PREFSAVE;
      case BRL_VSKEY_PLOC_PLOC_T: return BRL_CMD_CSRTRK;
      case BRL_VSKEY_PLOC_PLOC_U: return BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP;
      case BRL_VSKEY_CONTROL: ctrlpressed = BRL_FLG_CHAR_CONTROL; return BRL_CMD_NOOP;
      case BRL_VSKEY_ALT: altpressed = BRL_FLG_CHAR_META; return BRL_CMD_NOOP;   
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
  unsigned char ch, packet[MAXPACKETSIZE];
  static int routing = 0;
  ssize_t packetSize;
  packetSize = brl_readPacket(brl,packet,sizeof(packet));
  if (packetSize==0) return EOF;
  if ((packet[0]!=0x3c) && (packet[0]!=0x3d) && (packet[0]!=0x23)) {
    logUnexpectedPacket(packet, packetSize);
    return EOF;
  }
  ch = packet[1];
  if (printcode) {
    char buf [100];
    sprintf(buf,"Keycode: 0x%x",ch);
    printcode = 0; /* MUST BE DONE BEFORE THE CALL TO MESSAGE */
    message(NULL,buf,MSG_WAITKEY | MSG_NODELAY);
    return EOF;
  }
  if (routing) {
    routing=0;
    if (ch>=0xc0)  return (packet[1]-0xc0) | BRL_VSMSK_ROUTING;
    return EOF;
  }
  if ((ch>=0xc0) && (ch<=0xdf)) return (ch-0xc0) | BRL_VSMSK_FUNCTIONKEY;
  if (ch==0x91) {
    routing = 1;
    return BRL_CMD_NOOP;
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
