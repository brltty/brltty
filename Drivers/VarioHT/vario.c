/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#include "Programs/misc.h"

#include "Programs/brl_driver.h"
#include "braille.h"
#include "variolow.h"

static unsigned char lastWindow[40];

static int pressedKeys;
static int activeKeys;
#define KEY_1 0X01
#define KEY_2 0X02
#define KEY_3 0X04
#define KEY_4 0X08
#define KEY_5 0X10
#define KEY_6 0X20

static void
brl_identify (void)
{
	LogPrint(LOG_NOTICE, "Vario Driver for HandyTech Emulation Mode");
}

static int 
brl_open (BrailleDisplay *brl, char **parameters, const char *device)
{
		/*	Seems to signal en error */ 
	if(!varioinit(device)) {
		memset(lastWindow, 0, 40);
		pressedKeys = 0;
		activeKeys = 0;

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
	if(memcmp(lastWindow,brl->buffer,40)!=0) {
		unsigned char outbuff[40];
		memcpy(lastWindow,brl->buffer,40);
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
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context)
{
	int code;
		/*	Since we are nonblocking this will happen quite a lot */ 
	while((code=varioget())!=-1) {
                int release;
                int key;
		if (code == VARIO_DISPLAY_DATA_ACK) continue;
		if (code == VARIO_MODEL_MODULAR40) continue;
                if ((release = ((code & VARIO_RELEASE_FLAG) != 0)))
                	code &= ~VARIO_RELEASE_FLAG;

		if ((code >= VARIO_CURSOR_BASE) &&
		    (code < (VARIO_CURSOR_BASE + VARIO_CURSOR_COUNT))) {
			int keys = activeKeys;
			activeKeys = 0;
			key = code - VARIO_CURSOR_BASE;
			if (release) return EOF;
			switch (keys) {
				case 0:
					return BRL_BLK_ROUTE + key;
				case (KEY_1):
					return BRL_BLK_DESCCHAR + key;
				case (KEY_2):
					return BRL_BLK_CUTAPPEND + key;
				case (KEY_3):
					return BRL_BLK_CUTBEGIN + key;
				case (KEY_4):
					return BRL_BLK_SETLEFT + key;
				case (KEY_5):
					return BRL_BLK_CUTLINE + key;
				case (KEY_6):
					return BRL_BLK_CUTRECT + key;
				default:
					return BRL_CMD_NOOP;
			}
		}

		switch (code) {
			case VARIO_PUSHBUTTON_1:
				key = KEY_1;
				break;
			case VARIO_PUSHBUTTON_2:
				key = KEY_2;
				break;
			case VARIO_PUSHBUTTON_3:
				key = KEY_3;
				break;
			case VARIO_PUSHBUTTON_4:
				key = KEY_4;
				break;
			case VARIO_PUSHBUTTON_5:
				key = KEY_5;
				break;
			case VARIO_PUSHBUTTON_6:
				key = KEY_6;
				break;
			default:
				continue;
		}
		if (release) {
			int keys = activeKeys;
			pressedKeys &= ~key;
			activeKeys = 0;
			switch (keys) {
				case (KEY_1):
					return BRL_CMD_LNUP;
				case (KEY_2):
					return BRL_CMD_FWINLT;
				case (KEY_3):
					return BRL_CMD_HELP;
				case (KEY_4):
					return BRL_CMD_LNDN;
				case (KEY_5):
					return BRL_CMD_FWINRT;
				case (KEY_6):
					return BRL_CMD_HOME;
				case (KEY_3 | KEY_1):
					return BRL_CMD_TOP_LEFT;
				case (KEY_3 | KEY_2):
					return BRL_CMD_SKPIDLNS;
				case (KEY_3 | KEY_4):
					return BRL_CMD_BOT_LEFT;
				case (KEY_3 | KEY_5):
					return BRL_CMD_CSRTRK;
				case (KEY_3 | KEY_6):
					return BRL_CMD_PREFMENU;
				default:
					return EOF;
			}
		} else {
			pressedKeys |= key;
			activeKeys = pressedKeys;
		}
	}
	return EOF;
}
