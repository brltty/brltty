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

#include <unistd.h>
#include <sys/io.h>
#include "brlconf.h"
#include "vblow.h"
#include "../misc.h"

#define LPTSTATUSPORT LPTPORT+1
#define LPTCONTROLPORT LPTPORT+2
  
int vbinit() {
  char alldots[40];
  int i;
  if (ioperm(LPTPORT,3,1)!=0 || ioperm(0x80,1,1)!=0) {
    LogPrint(LOG_ERR,"Error: must be superuser");
    return -1;
  }
  for (i = 0; i<40; i++) {
    alldots[i] = 255;
  }
  vbdisplay(alldots);
  return 0;
}

void vbsleep(long x) {
  int i;
  for (i = 0; i<x; i++) outb(1,0x80);
}

void vbclockpause() {
  int i;
  for (i = 0; i<=VBCLOCK*100; i++) ;
}

void vbdisplay(char *vbBuf) {
  int i,j;
  char b;
  for (j = 0; j<VBSIZE; j++) {
    for (i = 7; i>=0; i--) {
      b = (vbBuf[j] << i) & VBLPTDATA;
      outb(b,LPTPORT);
      vbclockpause();
      outb(b | VBLPTCLOCK,LPTPORT);
      vbclockpause();
    }
  }
  outb(b | VBLPTCLOCK,LPTPORT);
  for (i = 0; i<=7; i++) vbclockpause();
  outb(0,LPTPORT);
  for (i = 0; i<=7; i++) vbclockpause();
  outb(VBLPTSTROBE,LPTPORT);
  for (i = 0; i<=7; i++) vbclockpause();
  outb(0,LPTPORT);
  vbclockpause();
}

void vbtranslate(char *vbBuf,char *vbDest,int size) {
  int i;
  for (i = 0; i<size; i++) {
    char c = vbBuf[i] & 0xe1;
    c = c | ((vbBuf[i] << 2) & 0x8);
    c = c | ((vbBuf[i] >> 1) & 0x2);
    c = c | ((vbBuf[i] << 1) & 0x10);
    c = c | ((vbBuf[i] >> 2) & 0x4);
    vbDest[i] = c;
  }
}

void BrButtons(vbButtons *dest) {
  char i;
  dest->bigbuttons = 0;
  dest->keypressed = 0;
  for (i = 47; i>=40; i--) {
    outb(i,LPTPORT);
    vbsleep(VBDELAY);
    if ((inb(LPTSTATUSPORT) & 0x08)==0) {
      dest->bigbuttons |= (1 << (i-40));
      dest->keypressed = 1;
    }
  }
  dest->routingkey = 0;
  for (i = 39; i>=0; i--) {
    outb(i,LPTPORT);
    vbsleep(VBDELAY);
    if ((inb(LPTSTATUSPORT) & 0x08)==0) {
      dest->routingkey = i+1;
      dest->keypressed = 1;
      break;
    }
  }
}
