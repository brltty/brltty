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

#define VERSION "BRLTTY driver for MDV MB408S, version 0.8 (August 2000)"
/* MDV/braille.c - Braille display driver for MDV displays.
 *
 * Written by Stéphane Doyon (s.doyon@videotron.ca) in collaboration with
 * Simone Dal Maso <sdalmaso@protec.it>.
 *
 * It is being tested on MB408S, should also support MB208 and MB408L.
 * It is designed to be compiled in BRLTTY version 2.91-3.0.
 *
 * History:
 * 0.8: Stupid mistake processing keycode for SHIFT_PRESS/RELEASE. Swapped
 *    bindings for ATTRVIS and DISPMD. Send ACK for packet_to_process packets.
 *    Safety that forgets held routing keys when getting bad combination with
 *    ordinary keys. Bugs reported about locking/crashing with paste with
 *    routing keys, getting out of help, and getting out of freeze, are
 *    probably not solved.
 * 0.7: They have changed the protocol so that the SHIFT key pressed alone
 *    sends a code. Added plenty of key bindings. Fixed help.
 * 0.6: Added help file. Added hide/show cursor toggle on status cell
 *    routing key 1.
 * Unnumbered version: Fixes for dynmically loading drivers (declare all
 *    non-exported functions and variables static, satized debugging vs print).
 * 0.5: When receiving response to identification query, read all that's
 *    available, because there is usually an ACK packet pending (perhaps it
 *    always sends ACK + the response). Fixed bug that caused combiknation
 *    of routing and movement keys to fail.
 * 0.4: Fixed bug that put garbage instead of logging packet contents.
 *    Added key binding for showing attributes, and also for preferences menu
 *    (might change).
 * 0.3: Fixed bug in interpreting query reply which caused nonsense number
 *    of content and status cells.
 * 0.2: Added a few function keys, such as cursor tracking toggle. Put the
 *    display's line and column in status cells, with the line on top and
 *    the column on the bottom (reverse of what it was), does it work?
 *    Display parameters now set according to query reply.
 * 0.1: First draft ve5rsion. Query reply interpretation is bypassed and
 *    parameters are hard-coded. Has basic movement keys, routing keys
 *    and commands involving combinations of routing keys.
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

#define BRLSTAT ST_MDVStyle
#include "Programs/brl_driver.h"
#include "braille.h"
#include "Programs/serial.h"

/* Braille display parameters that do not change */
#define BRLROWS 1		/* only one row on braille display */

/* mb408s is 40cells + 2status cells, but there might be other models... */
#define MAXNRCONTENTCELLS 80
#define EXPECTEDNRSTATCELLS 2

/* protocol packets */
#define SOH 0x01
#define STX 0x02
#define ETX 0x03
#define CKSUM_MASK 0xAA55

/* Packet is SOH STX <code> <len> ETX <data...> <CKSUM_LOW-CKSUM_HIGH> */
static unsigned char packet_hdr[] = { SOH, STX, 0, 0, ETX};
#define PACKET_HDR_LEN 5
#define NRCKSUMBYTES 2
#define MAXPACKETLEN 255
/* <len> is coded on one byte */
#define MAXTOTALPACKETLEN MAXPACKETLEN+PACKET_HDR_LEN+NRCKSUMBYTES
#define OFF_CODE 2
#define OFF_LEN 3
#define OFF_DATA 5

static unsigned short
calc_cksum(unsigned char *buf)
/* Calculate two bytes checksum: buf is a packet containing the header and
   the data (with valid length). */
{  
  int i;
  unsigned short cksum = 0;
  int len = buf[OFF_LEN];
  /* SOH not included in cksum */
  for(i=1; i<len+PACKET_HDR_LEN; i++)
    cksum += buf[i];
  cksum ^= CKSUM_MASK;
  return cksum;
}

static void
put_cksum(unsigned char *buf)
/* Just append the cksum at the end of the packet. */
{
  unsigned short cksum = calc_cksum(buf);
  int len = buf[OFF_LEN];
  int pos = len + PACKET_HDR_LEN;
  buf[pos++] = (unsigned char) (cksum & 0xFF);
  buf[pos] = (unsigned char) (cksum >> 8);
  /* buf now contains PACKET_HDR_LEN+len+2 bytes */
}

/* packet codes */
#define ACK 127
/* len 0 */
#define FULLREFRESH 0
/* len 42, 2stat +40 content cells, see below for dots pattenr coding */
#define REFRESHSTATUS 1
/* len 2*/
#define REFRESHCONTENT 2
/* len 40 */
#define REFRESHLCD 5
/* len 40, in ASCII */
#define REFRESHSTATNUMCOMPACT 8
/* len 2, each byte ranges 0-99, first number on top of the other. */
#define REPORTKEYPRESS 16
/* len 1, see bellow. */
#define REPORTROUTINGKEYPRESS 17
/* len 1, range 1-42, 1-2 for status cells */
#define REPORTROUTINGKEYRELEASE 18
#define QUERY 36
/* len 0 */
#define QUERYREPLY 37
/* len 6, see bellow */

/* This defines the mapping between brltty and MDV's dots pattern coding. */
static TranslationTable outputTable;

/* Key codes */
#define NRFKEYS 10 /* values 1-10 */
/* Shift and Long modifiers for all F-keys, (except LONG-F10 and
   SHIFT-LONG-F10, also SHIFT-F9 is only available in protocol 4 while
   we need 5 for the SHIFT key to work alone I think) */
#define SHIFT_MOD 0x10
#define LONG_MOD 0x20
#define MODIFIER_MASK 0x70
  /* It should be 0x30, except that I want to avoid confusion with
     SHIFT_RELEASE = 64d */
#define KEY_MASK 0x0F
/* For the folliwing, plain shift and shift-long combinations are defined
   but apparently not long- alone. */
#define LF 11
#define UP 12
#define RG 13
#define DN 14
/* Two special codes have been added afterwards and don't fit in the
   scheme: */
#define SHIFT_PRESS 63
#define SHIFT_RELEASE 64

/* Query reply */
#define OFF_NRCONTENTCELLS PACKET_HDR_LEN /* + 0 */
#define OFF_NRSTATCELLS PACKET_HDR_LEN+1
#define OFF_NRDOTSPERCELL PACKET_HDR_LEN+2 /* 6/8 */
#define OFF_HASROUTINGKEYS PACKET_HDR_LEN+3 /* 0/1 */
#define OFF_PRIMARYVERSION PACKET_HDR_LEN+4
#define OFF_SECONDARYVERSION PACKET_HDR_LEN+5
static unsigned char query_reply_packet_hdr[] = {SOH, STX, QUERYREPLY, 6, ETX};

/* delay between auto-detect attemps at initialization */
#define DETECT_DELAY (2000)	/* msec */

/* Global variables */

static int brl_fd;                /* file descriptor for comm port */
static struct termios oldtio,	  /* old terminal settings for com port */
                      curtio;     /* current settings */
static unsigned char *sendpacket, /* packet scratch pad */
                     *recvpacket,
                     *ackpacket,  /* code ACK len 0 prepared packet */
                     *prevdata,   /* previous data sent */
                     *prevstatbuf,/* record status cells content */
                     *statbuf;    /* record status cells content */
#define ACKPACKETLEN PACKET_HDR_LEN+NRCKSUMBYTES
static int brl_cols, 	          /* Number of cells available for text */
           nrstatcells;           /* number of status cells */
static char packet_to_process = 1, /* flag: if a packet is received while
				      expecting ACK in writebrl */
            *routing_were_pressed, /* flags for all the routing keys that
				      have been pressed since the last time
				      they were all unpressed. */
            *which_routing_keys; /* ordered list of pressed routing keys */


static void 
brl_identify (void)
{
  LogPrint(LOG_NOTICE, VERSION);
  LogPrint(LOG_INFO, "   Copyright (C) 1996-2000 by Stéphane Doyon <s.doyon@videotron.ca>.");
}


static int
myread(int fd, void *buf, unsigned len)
/* This is a replacement for read for use in nonblocking mode: when
   c_cc[VTIME] = 1, c_cc[VMIN] = 0. We want to read len bytes into buf, but
   return early if the timeout expires. I would have thought setting
   c_cc[VMIN] to len would have done the trick, but apparently c_cc[VMIN]>0
   means to wait indefinitly for at least 1byte, which we don't want. */
{
  char *ptr = (char *)buf;
  int r, l = 0;
  while(l < len){
    r = read(fd,ptr+l,len-l);
    if(r == 0) return(l);
    else if(r<0) return(r);
    l += r;
  }
  return(l);
}

static int
receive_rest(unsigned char *packet)
/* Assuming an SOH has previously been read, receive the rest of a packet.
   If the packet seems wrong (missing STX ETX, too large len, wrong
   cksum) then it is discarded.
   Returns 1 if a packet was successfully read, 0 otherwise. */
{
  int len, cksum;
  /* assuming we have already caught an SOH */
  if(myread(brl_fd, packet+1, PACKET_HDR_LEN-1) != PACKET_HDR_LEN-1) return 0;
  /* Check for STX and ETX */
  if(packet[1] != packet_hdr[1] || packet[4] != packet_hdr[4]){
    LogPrint(LOG_DEBUG,"Invalid packet: STX %02x, ETX %02x",
	     packet[1],packet[4]);
    return 0;
  }
  len = packet[OFF_LEN];
  if(len > MAXPACKETLEN) return 0;
  if(myread(brl_fd, packet+PACKET_HDR_LEN, len+NRCKSUMBYTES)
     != len+NRCKSUMBYTES){
    LogPrint(LOG_DEBUG,"receive_rest(): short read count");
    return 0;
  }
#if 0
{
  int i;
  char msgbuf[(MAXPACKETLEN+NRCKSUMBYTES)*3+1], hexbuf[4];
  msgbuf[0] = 0;
  for(i=0; i<len+NRCKSUMBYTES; i++){
    sprintf(hexbuf, "%02x ", packet[i+PACKET_HDR_LEN]);
    strcat(msgbuf, hexbuf);
  }
  LogPrint(LOG_DEBUG,"Received packet: code %u, body %s",
	   packet[OFF_CODE],msgbuf);
}
#endif /* 0 */
  /* Verify checksum */
  cksum = calc_cksum(packet);
  if(packet[PACKET_HDR_LEN+len] != (unsigned char)(cksum & 0xFF)
     || packet[PACKET_HDR_LEN+len+1] != (unsigned char)(cksum >> 8)){
    LogPrint(LOG_DEBUG,"Checksum invalid");
    return 0;
  }
  return 1;
}
  
static int
expect_receive_packet(unsigned char *packet)
/* Assuming a response packet is expected, wait 0.2sec for a SOH and
   receive the packet. If we get something that doesn't seem to be a
   packet, we skip it and try again. Returns 1 if a packet was successfully
   received, 0 otherwise. */
{
  /* timeout of 0.2sec */
  curtio.c_cc[VTIME] = 2;
  curtio.c_cc[VMIN] = 0;
  tcsetattr (brl_fd, TCSANOW, &curtio);
  /* get the first byte*/
  if(read(brl_fd, packet, 1) <= 0) return 0;
  /* now timeout of only 0.1sec, so that we get all bytes that were sent
     together but we don't wait for more. */
  curtio.c_cc[VTIME] = 1;
  curtio.c_cc[VMIN] = 0;
  tcsetattr (brl_fd, TCSANOW, &curtio);
  while(1) {
    /* Read until we get an SOH */
    while(packet[0] != SOH)
      if(read(brl_fd, packet, 1) <= 0) return 0;
    /* Now read and check the rest of the packet */
    if(receive_rest(packet)) break;
    /* Try to read another packet. We hunt for an SOH again. For now we don't
       bother to handle the case where the SOH of a valid packet was read
       and discarded in receive_rest(). */
    if(read(brl_fd, packet, 1) <= 0) return 0;
  }
  /* success */
  return 1;
}

static int
peek_receive_packet(unsigned char *packet)
/* read packet if there's one already waiting, but return immediately
   if not. */
{
  do {
    /* reset to nonblocking, timeout 0 */
    curtio.c_cc[VTIME] = 0;
    curtio.c_cc[VMIN] = 0;
    tcsetattr (brl_fd, TCSANOW, &curtio);
    /* Check for first byte */
    do {
      if(read(brl_fd, packet, 1) <= 0) return 0;
    } while(packet[0] != SOH);
    /* further reads will wait a bit to get a complete sequence */
    /* timeout of 0.1sec */
    curtio.c_cc[VTIME] = 1;
    curtio.c_cc[VMIN] = 0;
    tcsetattr (brl_fd, TCSANOW, &curtio);
  } while(!receive_rest(packet));
  return 1;
}


static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device)
{
  int hasrouting, dotspercell, version1, version2;

  {
    static const DotsTable dots = {0X08, 0X04, 0X02, 0X80, 0X40, 0X20, 0X01, 0X10};
    makeOutputTable(&dots, &outputTable);
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  sendpacket = recvpacket = ackpacket
    = prevdata = statbuf = prevstatbuf
      = routing_were_pressed = which_routing_keys = NULL;

  /* Open the Braille display device for random access */
  if (!openSerialDevice(device, &brl_fd, &oldtio)) goto failure;

  /* Construct new settings */
  initializeSerialAttributes(&curtio);

  /* nonblocking input will be set in ..._receive_packet() functions */

  if(!restartSerialDevice(brl_fd, &curtio, 19200)) goto failure;
 
/* Allocate and init static packet buffers */
  if((sendpacket = (unsigned char *)malloc(MAXTOTALPACKETLEN)) == NULL
      || (recvpacket = (unsigned char *)malloc(MAXTOTALPACKETLEN)) == NULL
      || (ackpacket = (unsigned char *)malloc(ACKPACKETLEN)) == NULL)
    goto failure;
  memcpy(sendpacket, packet_hdr, PACKET_HDR_LEN);
  memcpy(ackpacket, packet_hdr, PACKET_HDR_LEN);
  ackpacket[OFF_CODE] = ACK;
  ackpacket[OFF_LEN] = 0;
  put_cksum(ackpacket);

  /* Query the display */
  sendpacket[OFF_CODE] = QUERY;
  sendpacket[OFF_LEN] = 0;
  put_cksum(sendpacket);
  while(1){
    if(write(brl_fd, sendpacket, PACKET_HDR_LEN+NRCKSUMBYTES)
       != PACKET_HDR_LEN+NRCKSUMBYTES)
      goto failure;
    tcdrain(brl_fd);
    while(expect_receive_packet(recvpacket)){
      if(memcmp(recvpacket, query_reply_packet_hdr, PACKET_HDR_LEN) == 0)
	goto detected;
      else if(recvpacket[OFF_CODE] == ACK)
	LogPrint(LOG_DEBUG,"Skipping probable ACK packet");
      else LogPrint(LOG_DEBUG,"Skipping invalid response to query");
    }
    LogPrint(LOG_DEBUG,"No response to query");
    delay(DETECT_DELAY);
  }

detected:
  brl_cols = recvpacket[OFF_NRCONTENTCELLS];
  nrstatcells = recvpacket[OFF_NRSTATCELLS];
  dotspercell = recvpacket[OFF_NRDOTSPERCELL];
  hasrouting = recvpacket[OFF_HASROUTINGKEYS];
  version1 = recvpacket[OFF_PRIMARYVERSION];
  version2 = recvpacket[OFF_SECONDARYVERSION];

  LogPrint(LOG_INFO,"Display replyed: %d cells, %d status cells, "
	   "%d dots per cell, has routing keys flag %d, version %d.%d",
	   brl_cols, nrstatcells, dotspercell, hasrouting, version1,version2);

  if(brl_cols > MAXNRCONTENTCELLS || brl_cols < 1){
    LogPrint(LOG_ERR, "Invalid number of cells: %d", brl_cols);
    goto failure;
  }
  if(nrstatcells != EXPECTEDNRSTATCELLS)
    LogPrint(LOG_NOTICE, "Unexpected number of status cells: %d", nrstatcells);
  if(nrstatcells < 0){
    LogPrint(LOG_ERR, "Invalid number of status cells: %d", nrstatcells);
    goto failure;
  }
  if(brl_cols + nrstatcells > MAXPACKETLEN){
    /* This is to make sure we don't overflow the sendpacket when sending
       braille to be displayed. */
    LogPrint(LOG_ERR, "Invalid total number of cells");
    goto failure;
  }

  brl->helpPage = 0;
  brl->x = brl_cols;		/* initialize size of display */
  brl->y = BRLROWS;		/* always 1 */

  /* Allocate space for buffers */
  /* For portability we want to avoid declarations such as char array[v]
     where v is not a compilation constant. BRLTTY must work as reliably as
     possible even in adverse conditions, so we don't want it to be affected
     by a failing malloc. Therefore we malloc all we need right now, and
     there will be no more mallocs later on.

     It might be best to just statically initialize all these arrays to
     the maximum possible size. */
  if((statbuf = (unsigned char *)malloc(nrstatcells)) == NULL
     || (prevdata = (unsigned char *) malloc (brl_cols)) == NULL
     || (prevstatbuf = (unsigned char *)malloc(nrstatcells)) == NULL
     || (routing_were_pressed = (char *)malloc(brl_cols+nrstatcells)) == NULL
     || (which_routing_keys = (char *)malloc(brl_cols+nrstatcells)) == NULL)
    goto failure;

  /* Force rewrite of display on first writebrl */
  memset(prevdata, 0xFF, brl_cols); /* all dots */
  memset(prevstatbuf, 0, nrstatcells); /* empty */

  memset(routing_were_pressed, 0, brl_cols+nrstatcells); /* none pressed, all
							    flags negative. */
  /* which_routing_keys does not require initialization */

  return 1;

failure:;
  LogPrint(LOG_WARNING,"MDV driver giving up");
  brl_close(brl);
  return 0;
}


static void 
brl_close (BrailleDisplay *brl)
{
  if (brl_fd >= 0)
    {
      putSerialAttributes (brl_fd, &oldtio);
      close (brl_fd);
    }
  if (prevdata) free (prevdata);
  if (statbuf) free (statbuf);
  if (prevstatbuf) free (prevstatbuf);
  if(sendpacket) free(sendpacket);
  if(recvpacket) free(recvpacket);
  if(ackpacket) free(ackpacket);
  if(routing_were_pressed) free(routing_were_pressed);
  if(which_routing_keys) free(which_routing_keys);
}


static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *s)
{
  if(nrstatcells >= EXPECTEDNRSTATCELLS)
    memcpy(statbuf, s, EXPECTEDNRSTATCELLS);
  /* BRLTTY expects this display to have only 2 status cells */
}


static void 
brl_writeWindow (BrailleDisplay *brl)
{
  int i;
  unsigned char *p;

  if (brl->x != brl_cols || brl->y != BRLROWS)
    return;
    
  if(memcmp(prevdata, brl->buffer, brl_cols) == 0
     && memcmp(prevstatbuf, statbuf, nrstatcells) == 0)
    return;
  memcpy(prevdata, brl->buffer, brl_cols);
  memcpy(prevstatbuf, statbuf, nrstatcells);

  sendpacket[OFF_CODE] = FULLREFRESH;
  sendpacket[OFF_LEN] = nrstatcells+brl_cols;
  p = sendpacket+PACKET_HDR_LEN;
  for(i=0; i<nrstatcells; i++)
    *(p++) = outputTable[statbuf[i]];
  for(i=0; i < brl_cols; i++)
     *(p++) = outputTable[brl->buffer[i]];
  put_cksum(sendpacket);

  write(brl_fd, sendpacket, PACKET_HDR_LEN+nrstatcells+brl_cols+NRCKSUMBYTES);
  tcdrain(brl_fd);

  if(expect_receive_packet(recvpacket)){
    if(memcmp(recvpacket, ackpacket, ACKPACKETLEN) == 0)
      return;
    else{
      packet_to_process = 1;
      LogPrint(LOG_DEBUG, "After sending update, received code %d packet",
	       recvpacket[OFF_CODE]);
    }
  }else LogPrint(LOG_DEBUG, "No ACK after update");
  /* We'll do resends like this for now */
  memset(prevdata, 0xFF, brl_cols);
  memset(prevstatbuf, 0, nrstatcells);
  /* and then wait for writebrl to be called again. */
}


static int 
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context)
{
  static char
    ignore_next_release = 0; /* when a command is triggered by combining
				routing keys with ordinary keys, it is
				triggered by the press of the ordinary key,
				and then the release of the routing key has to 
				be ignored. This is the flag for it. */
  static int
    nr_routing_cur_pressed = 0, /* number of routing keys currently pressed */
    howmanykeys = 0, /* number of entries in which_routing_keys */
    pending_cmd = EOF; /* For a key binding that triggers two commands,
			  store the second command here. */
  int code;
  if(pending_cmd != EOF){
    int cmd = pending_cmd;
    pending_cmd = EOF;
    return cmd;
  }
  do {
    if(!packet_to_process){
      if(!peek_receive_packet(recvpacket)) return EOF;
    }else packet_to_process = 0;
    if(memcmp(recvpacket, ackpacket, ACKPACKETLEN) != 0)
      /* ACK it, unless it is itself an ACK */
      write(brl_fd, ackpacket, ACKPACKETLEN);
    code = recvpacket[OFF_CODE];
  } while(code != REPORTKEYPRESS && code != REPORTROUTINGKEYPRESS
	  && code != REPORTROUTINGKEYRELEASE);
  /* Note that we discard ACK packets */
  /* All these packets should have a length of 1 */
  if(recvpacket[OFF_LEN] != 1){
    LogPrint(LOG_NOTICE,"Received key code 0x%x with length %d",
	       code, recvpacket[OFF_LEN]);
    return EOF;
  }

  switch(code) {
    case REPORTKEYPRESS: {
      int cmd = EOF;
      int keycode = recvpacket[PACKET_HDR_LEN];
      /* SHIFT_PRESS and SHIFT_RELEASE are exceptions to the following */
      int key = keycode & KEY_MASK;
      int modifier = keycode & MODIFIER_MASK;
      LogPrint(LOG_DEBUG,"Received key code 0x%x with modifier 0x%x",
	       key, modifier);
      if(nr_routing_cur_pressed > 0){
	ignore_next_release = 1;
	if(howmanykeys == 1 && modifier == 0){
	  switch(key){
	    case RG: cmd = BRL_BLK_CUTBEGIN + which_routing_keys[0]; break;
	    case LF: cmd = BRL_BLK_CUTRECT + which_routing_keys[0]; break;
	  };
	}
	if(cmd == EOF){
	  /* If the key is not recognized as a valid combination with the
	     routing keys: as a safety we clear/forget all routing keys
	     (pretend none are pressed). If we were fooled by repeat presses
	     or missed a release, this ensures we don't stay locked thinking
	     that some routing keys are held, and we return other keys to
	     their normal functions. Following routing key releases will be
	     ignored anyway. */
	  nr_routing_cur_pressed = 0;
	  memset(routing_were_pressed, 0, brl_cols);
	  howmanykeys = 0;
	}
      }else if(keycode == SHIFT_PRESS) {
	cmd = BRL_CMD_CSRHIDE | BRL_FLG_TOGGLE_ON;
      }else if(keycode == SHIFT_RELEASE) {
	cmd = BRL_CMD_CSRHIDE | BRL_FLG_TOGGLE_OFF;
      }else if(modifier == 0){
	switch(key) {
          case LF: cmd = BRL_CMD_FWINLT; break;
          case RG: cmd = BRL_CMD_FWINRT; break;
          case UP: cmd = BRL_CMD_LNUP; break;
          case DN: cmd = BRL_CMD_LNDN; break;
	  case 1: cmd = BRL_CMD_TOP_LEFT; break;
	  case 2: cmd = BRL_CMD_BOT_LEFT; break;
	  case 3: cmd = BRL_CMD_CHRLT; break;
	  case 4: cmd = BRL_CMD_HOME; break;
	  case 5: cmd = BRL_CMD_CSRTRK; break;
	  case 6: cmd = BRL_CMD_SKPIDLNS; break;
	  case 7: cmd = BRL_CMD_SKPBLNKWINS; break;
	  case 8: cmd = BRL_CMD_CHRRT; break;
	  case 10: cmd = BRL_CMD_PREFMENU; break;
	};
      }else if(modifier == SHIFT_MOD){
	switch(key) {
	case UP: cmd = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP; break;
	case DN: cmd = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN; break;
	case 1: cmd = BRL_CMD_FREEZE; break;
	case 2: cmd = BRL_CMD_INFO; break;
	case 3: cmd = BRL_CMD_HWINLT; break;
	case 4: cmd = BRL_CMD_CSRSIZE; break;
	case 5: cmd = BRL_CMD_CSRVIS; break;
	case 6: cmd = BRL_CMD_DISPMD; break;
	case 8: cmd = BRL_CMD_HWINRT; break;
	case 10: cmd = BRL_CMD_PASTE; break;
	};
      }else if(modifier == LONG_MOD){
	switch(key) {
	case 4: cmd = BRL_CMD_CSRBLINK; break;
	case 5: cmd = BRL_CMD_CAPBLINK; break;
	case 6: cmd = BRL_CMD_ATTRBLINK; break;
	};
      }else if(modifier == (SHIFT_MOD|LONG_MOD)){
	switch(key) {
	case 6: cmd = BRL_CMD_ATTRVIS; break;
	};
      }
      return cmd;
    }
    break;
    case REPORTROUTINGKEYPRESS:
    case REPORTROUTINGKEYRELEASE: {
      int whichkey = recvpacket[PACKET_HDR_LEN];
      LogPrint(LOG_DEBUG,"Received routing key %s for key %d",
	       ((code == REPORTROUTINGKEYPRESS) ? "press" : "release"),
	       whichkey);
      if(whichkey < 1 || whichkey > brl_cols+nrstatcells) return EOF;
      if(whichkey <= nrstatcells){
	/* special handling for routing keys over status cells: currently
	   only key 1 is mapped. */
	if(whichkey == 1)
	  return BRL_CMD_CSRHIDE 
	    | ((code == REPORTROUTINGKEYPRESS) ? BRL_FLG_TOGGLE_OFF : BRL_FLG_TOGGLE_ON);
	else return EOF;
      }
      whichkey -= nrstatcells;
      whichkey--;
      /* whichkey now ranges 0 - bro_cols-1 instead of 1 - brl_cols. */
      if(code == REPORTROUTINGKEYPRESS){
	int i;
	routing_were_pressed[whichkey] = 1;
	nr_routing_cur_pressed++;
	/* remake which_routing_keys */
	for (howmanykeys = 0, i = 0; i < brl_cols; i++)
	  if (routing_were_pressed[i])
	    which_routing_keys[howmanykeys++] = i;
	/* routing_were_pressed[i] tells if routing key i is pressed.
	   which_routing_keys[0] to which_routing_keys[howmanykeys-1] lists
	   the numbers of the keys that are pressed. */
	return EOF;
      }else{
	int cmd = EOF;
	if(nr_routing_cur_pressed == 0){
	  ignore_next_release = 0;
	  return EOF;
	}
	nr_routing_cur_pressed--;
	if(nr_routing_cur_pressed > 0) return EOF;
	/* We interpret the command when all keys have been released. */
	if(ignore_next_release);
	else if (howmanykeys == 1)
	  cmd = BRL_BLK_ROUTE + which_routing_keys[0];
	else if (howmanykeys == 3 && which_routing_keys[1] == brl_cols-2
		 && which_routing_keys[2] == brl_cols-1)
	  cmd = BRL_BLK_CUTBEGIN + which_routing_keys[0];
	else if (howmanykeys == 3 && which_routing_keys[0] == 0
		 && which_routing_keys[1] == 1)
	  cmd = BRL_BLK_CUTRECT + which_routing_keys[2];
	else if ((howmanykeys == 4 && which_routing_keys[0] == 0
		  && which_routing_keys[1] == 1
		  && which_routing_keys[2] == brl_cols-2
		  && which_routing_keys[3] == brl_cols-1)
	       || (howmanykeys == 2 && which_routing_keys[0] == 1
		   && which_routing_keys[1] == 2))
	  cmd = BRL_CMD_PASTE;
	else if (howmanykeys == 2 && which_routing_keys[0] == 0
		 && which_routing_keys[1] == brl_cols-1)
	  cmd = BRL_CMD_HELP;
	else if(howmanykeys == 3
		&& which_routing_keys[0]+2 == which_routing_keys[1]){
	  cmd = BRL_BLK_CUTBEGIN + which_routing_keys[0];
	  pending_cmd = BRL_BLK_CUTRECT + which_routing_keys[2];
	}
	/* Reset to no keys pressed */
	memset(routing_were_pressed, 0, brl_cols);
	howmanykeys = 0;
	ignore_next_release = 0;
	return cmd;
      }
    }
    break;
  };
  return EOF;
}
