/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* Thanks to the authors of the Vario-HT driver: the implementation of this
   driver is similar to the Vario-HT one. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vblow.h"
#include "brl.h"
#include "brlconf.h"
#include "../brl_driver.h"
#include "../misc.h"

static unsigned char lastbuff[40];

static void identbrl(void) {
  LogAndStderr(LOG_NOTICE, "VideoBraille Driver");
}

static void initbrl(brldim *brl, const char *dev) {
  /*	Seems to signal en error */ 
  brl->x=-1;
  if (!vbinit()) {
    /* Theese are pretty static */ 
    brl->x=40;
    brl->y=1;
    if (!(brl->disp=malloc(brl->x*brl->y))) {
      perror(dev);
      brl->x=-1;
    }
  }
}

static void closebrl(brldim *brl) {
  free(brl->disp);
}

static void writebrl(brldim *brl) {
  char outbuff[40];
  int i;

  if (!brl || !brl->disp) {
    return;
  }
  /* Only display something if the data actually differs, this 
  *  could most likely cause some problems in redraw situations etc
  *  but since the darn thing wants to redraw quite frequently otherwise 
  *  this still makes a better lookin result */ 
  for (i = 0; i<40; i++) {
    if (lastbuff[i]!=brl->disp[i]) {
      memcpy(lastbuff,brl->disp,40*sizeof(char));
      //  Redefine the given dot-pattern to match ours
      vbtranslate(brl->disp, outbuff, 40);
      vbdisplay(outbuff);
      vbdisplay(outbuff);
      shortdelay(VBREFRESHDELAY);
      break;
    }
  }
}

static void setbrlstat (const unsigned char *st) {
// The VideoBraille display has no status cells
}

static int readbrl(int type) {
  vbButtons buttons;
  BrButtons(&buttons);
  if (!buttons.keypressed) {
    return EOF;
  } else {
    vbButtons b;
    do {
      BrButtons(&b);
      buttons.bigbuttons |= b.bigbuttons;
      usleep(1);
    } while (b.keypressed);
    // Test which buttons has been pressed
    if (buttons.bigbuttons==KEY_UP) return CMD_LNUP;
    else if (buttons.bigbuttons==KEY_LEFT) return CMD_FWINLT;
    else if (buttons.bigbuttons==KEY_RIGHT) return CMD_FWINRT;
    else if (buttons.bigbuttons==KEY_DOWN) return CMD_LNDN;
    else if (buttons.bigbuttons==KEY_ATTRIBUTES) return CMD_ATTRVIS;
    else if (buttons.bigbuttons==KEY_CURSOR) return CMD_CSRVIS;
    else if (buttons.bigbuttons==KEY_HOME) {
      /* If a routing key has been pressed, then mark the beginning of a block;
         go to cursor position otherwise */
      return (buttons.routingkey>0) ? CR_BEGBLKOFFSET+buttons.routingkey-1 : CMD_HOME;
    }
    else if (buttons.bigbuttons==KEY_MENU) {
      /* If a routing key has been pressed, then mark the end of a block;
         go to configuration menu otherwise */
      return (buttons.routingkey>0) ? CR_ENDBLKOFFSET+buttons.routingkey-1 : CMD_CONFMENU;
    }
    else if (buttons.bigbuttons==(KEY_ATTRIBUTES | KEY_MENU)) return CMD_PASTE;
    else if (buttons.bigbuttons==(KEY_CURSOR | KEY_LEFT)) return CMD_CHRLT;
    else if (buttons.bigbuttons==(KEY_HOME | KEY_RIGHT)) return CMD_CHRRT;
    else if (buttons.bigbuttons==(KEY_UP | KEY_LEFT)) return CMD_TOP_LEFT;
    else if (buttons.bigbuttons==(KEY_RIGHT | KEY_DOWN)) return CMD_BOT_LEFT;
    else if (buttons.bigbuttons==(KEY_ATTRIBUTES | KEY_DOWN)) return CMD_HELP;
    else if (buttons.bigbuttons==(KEY_MENU | KEY_CURSOR)) return CMD_INFO;
    else if (buttons.bigbuttons==0) {
      // A cursor routing key has been pressed
      if (buttons.routingkey>0) {
        usleep(5);
        return CR_ROUTEOFFSET+buttons.routingkey-1;
      }
      else return EOF;
    } else
      return EOF;
  }
}
