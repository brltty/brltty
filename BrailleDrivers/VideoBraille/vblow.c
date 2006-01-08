/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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

#include "prologue.h"

#include <string.h>

#include "Programs/misc.h"
#include "Programs/system.h"
#include "Programs/brl.h"
#include "braille.h"
#include "vblow.h"

#define LPTSTATUSPORT LPTPORT+1
#define LPTCONTROLPORT LPTPORT+2
  
static TranslationTable outputTable;

int vbinit() {
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
  }

  if (enablePorts(LOG_ERR, LPTPORT, 3)) {
    if (enablePorts(LOG_ERR, 0X80, 1)) {
      unsigned char alldots[40];
      memset(alldots, 0XFF, 40);
      vbdisplay(alldots);
      return 0;
    }
    disablePorts(LPTPORT, 3);
  }

  LogPrint(LOG_ERR, "Error: must be superuser");
  return -1;
}

void vbsleep(long x) {
  int i;
  for (i = 0; i<x; i++) writePort1(0x80, 1);
}

static void vbclockpause() {
  int i;
  for (i = 0; i<=VBCLOCK*100; i++) ;
}

void vbdisplay(unsigned char *vbBuf) {
  int i,j;
  char b;
  for (j = 0; j<VBSIZE; j++) {
    for (i = 7; i>=0; i--) {
      b = (vbBuf[j] << i) & VBLPTDATA;
      writePort1(LPTPORT, b);
      vbclockpause();
      writePort1(LPTPORT, b | VBLPTCLOCK);
      vbclockpause();
    }
  }
  writePort1(LPTPORT, b | VBLPTCLOCK);
  for (i = 0; i<=7; i++) vbclockpause();
  writePort1(LPTPORT, 0);
  for (i = 0; i<=7; i++) vbclockpause();
  writePort1(LPTPORT, VBLPTSTROBE);
  for (i = 0; i<=7; i++) vbclockpause();
  writePort1(LPTPORT, 0);
  vbclockpause();
}

void vbtranslate(const unsigned char *vbBuf,unsigned char *vbDest,int size) {
  int i;
  for (i=0; i<size; i++) vbDest[i] = outputTable[vbBuf[i]];
}

void BrButtons(vbButtons *dest) {
  char i;
  dest->bigbuttons = 0;
  dest->keypressed = 0;
  for (i = 47; i>=40; i--) {
    writePort1(LPTPORT, i);
    vbsleep(VBDELAY);
    if ((readPort1(LPTSTATUSPORT) & 0x08)==0) {
      dest->bigbuttons |= (1 << (i-40));
      dest->keypressed = 1;
    }
  }
  dest->routingkey = 0;
  for (i = 40; i>0; i--) {
    writePort1(LPTPORT, i-1);
    vbsleep(VBDELAY);
    if ((readPort1(LPTSTATUSPORT) & 0x08)==0) {
      dest->routingkey = i;
      dest->keypressed = 1;
      break;
    }
  }
}
