/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "variolow.h"
#include "Programs/brl.h"
#include "brlconf.h"
#include "Programs/brl_driver.h"
#include "Programs/misc.h"


static unsigned char lastbuff[40];

static void
brl_identify (void)
{
  LogPrint(LOG_NOTICE, "HT Protocol driver");
}

static int 
brl_open (BrailleDisplay *brl, char **parameters, const char *dev)
{
		/*	Seems to signal en error */ 
	if(!varioinit((char*)dev)) {
			/*	Theese are pretty static */ 
		brl->x=40;
		brl->y=1;
		return 1;
	}
	return 0;
}

static void 
brl_close (BrailleDisplay *brl)
{
	varioclose();
}

static void 
brl_writeWindow (BrailleDisplay *brl)
{
	char		outbuff[40];
	int			i;

		/*	Idiot check one */ 
	if(!brl) {
		return;
	}
		/*	Only display something if the data actually differs, this 
		 *	could most likely cause some problems in redraw situations etc
		 *	but since the darn thing wants to redraw quite frequently otherwise 
		 *	this still makes a better lookin result */ 
	for(i=0;i<40;i++) {
		if(lastbuff[i]!=brl->buffer[i]) {
			memcpy(lastbuff,brl->buffer,40*sizeof(char));
				/*	Redefine the given dot-pattern to match ours */
			variotranslate(brl->buffer, outbuff, 40);
			variodisplay(outbuff);
			break;
		}
	}
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *st)
{
	/*	Dumdidummm */ 
}

static int 
brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds)
{
	static int shift_button_down=0;
	int	decoded=EOF;
	int c;
		/*	Since we are nonblocking this will happen quite a lot */ 
	if((c=varioget())==-1) {
		return EOF;
	}

	switch(c) {
			/*	Should be handled in a better manner, this will do for now tho */ 
		case VARIO_DISPLAY_DATA_ACK:
			decoded = CMD_NOOP;
		break;
			/*	Always ignore press codes */ 
		case VARIO_PUSHBUTTON_PRESS_1:
		case VARIO_PUSHBUTTON_PRESS_2:
		case VARIO_PUSHBUTTON_PRESS_4:
		case VARIO_PUSHBUTTON_PRESS_5:
		case VARIO_PUSHBUTTON_PRESS_6:
			decoded = CMD_NOOP;
		break;
			/*	We define the button 3 as shift ... */ 
		case VARIO_PUSHBUTTON_PRESS_3:
			shift_button_down=1;
		break;	

		case VARIO_PUSHBUTTON_RELEASE_1:
			decoded = (shift_button_down?CMD_TOP_LEFT:CMD_LNUP);
		break;
		case VARIO_PUSHBUTTON_RELEASE_2:
			decoded = (shift_button_down?CMD_SKPIDLNS:CMD_FWINLT);
		break;
		case VARIO_PUSHBUTTON_RELEASE_3:
			shift_button_down=0;
		break;
		case VARIO_PUSHBUTTON_RELEASE_4:
			decoded = (shift_button_down?CMD_BOT_LEFT:CMD_LNDN);
		break;
		case VARIO_PUSHBUTTON_RELEASE_5:
			decoded = (shift_button_down?CMD_CSRTRK:CMD_FWINRT);
		break;
		case VARIO_PUSHBUTTON_RELEASE_6:
			decoded = (shift_button_down?CMD_PREFMENU:CMD_HOME);
		break;
		default:
			if(c>=VARIO_CURSOR_BASE&&c<=VARIO_CURSOR_BASE+VARIO_CURSOR_COUNT) {
				decoded = CR_ROUTE+c-VARIO_CURSOR_BASE;
			}
		break;
	}
	return decoded;
}
