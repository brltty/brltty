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

#include "Programs/brl.h"
#include "Programs/misc.h"

#define BRLNAME	"Vario"
#define PREFSTYLE ST_None

#include "Programs/brl_driver.h"
#include "braille.h"
#include "variolow.h"

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
                brl->helpPage = 0;
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
	if(memcmp(lastbuff,brl->buffer,40)!=0) {
		char		outbuff[40];
		memcpy(lastbuff,brl->buffer,40);
			/*	Redefine the given dot-pattern to match ours */
		variotranslate(brl->buffer,outbuff,40);
		variodisplay(outbuff);
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
	int noop = 0;
	int c;
		/*	Since we are nonblocking this will happen quite a lot */ 
	while((c=varioget())!=-1) {
		switch(c) {
			case VARIO_PUSHBUTTON_PRESS_3:
				shift_button_down=1;
			case VARIO_PUSHBUTTON_PRESS_1:
			case VARIO_PUSHBUTTON_PRESS_2:
			case VARIO_PUSHBUTTON_PRESS_4:
			case VARIO_PUSHBUTTON_PRESS_5:
			case VARIO_PUSHBUTTON_PRESS_6:
				noop=1;	
			case VARIO_DISPLAY_DATA_ACK:
			case 0X89: /* identity code for modular 40 */
				continue;	
	
			case VARIO_PUSHBUTTON_RELEASE_1:
				return (shift_button_down?CMD_TOP_LEFT:CMD_LNUP);
			case VARIO_PUSHBUTTON_RELEASE_2:
				return (shift_button_down?CMD_SKPIDLNS:CMD_FWINLT);
			case VARIO_PUSHBUTTON_RELEASE_3:
				shift_button_down=0;
				noop=1;
				continue;
			case VARIO_PUSHBUTTON_RELEASE_4:
				return (shift_button_down?CMD_BOT_LEFT:CMD_LNDN);
			case VARIO_PUSHBUTTON_RELEASE_5:
				return (shift_button_down?CMD_CSRTRK:CMD_FWINRT);
			case VARIO_PUSHBUTTON_RELEASE_6:
				return (shift_button_down?CMD_PREFMENU:CMD_HOME);

			default:
				if(c>=VARIO_CURSOR_BASE&&c<=VARIO_CURSOR_BASE+VARIO_CURSOR_COUNT)
					return CR_ROUTE+c-VARIO_CURSOR_BASE;
				break;
		}
	}
	return noop? CMD_NOOP: EOF;
}
