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

/* MiniBraille/braille.c - Braille display library
 * the following Tieman B.V. braille terminals are supported
 *
 * - MiniBraille v 1.5 (20 braille cells + 2 status)
 *   (probably other versions too)
 *
 * Brailcom o.p.s. <technik@brailcom.cz>
 *
 * Thanks to Tieman B.V., which gives me protocol information. Author.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Programs/misc.h"
#include "Programs/message.h"

#define BRLSTAT ST_TiemanStyle
#include "Programs/brl_driver.h"
#include "braille.h"
#include "minibraille.h"
#include "Programs/serial.h"

/* types */
enum mode_t { NORMAL_MODE, F1_MODE, F2_MODE, MANAGE_MODE, CLOCK_MODE };

/* global variables */
static SerialDevice *serialDevice = NULL;
static TranslationTable outputTable;
unsigned char xlated[20]; /* translated content of display */
unsigned char status[2]; /* xlated content of status area */
static enum mode_t mode = NORMAL_MODE;

/* #defines */
/* delay this time after command - better: use CTS, DTR, CTR or find other signal which means "ready for next command" */
#define AFTER_CMD_DELAY_TIME 10000
#define AFTER_CMD_DELAY do { approximateDelay(AFTER_CMD_DELAY_TIME/1000); } while (0)

static void beep(void)
{
	AFTER_CMD_DELAY;
	serialWriteData(serialDevice, "\eB\r", 3);
	AFTER_CMD_DELAY;
};

/* interface */
static void brl_identify(void)
{
	LogPrint(LOG_NOTICE, "Tieman B.V. MiniBraille driver");
	LogPrint(LOG_INFO, "   Copyright (C) 2000 by Brailcom o.p.s. <technik@brailcom.cz>"); 
}

static int brl_open(BrailleDisplay *brl, char **parameters, const char *device)
{
	__label__ __errexit;
	
	{
		static const DotsTable dots = {0X01, 0X02, 0X04, 0X80, 0X40, 0X20, 0X08, 0X10};
		makeOutputTable(&dots, &outputTable);
	}

	if (!isSerialDevice(&device)) {
		unsupportedDevice(device);
		return 0;
	}

	/* init geometry */
	brl->x = 20;
	brl->y = 1;

	/* open device */
	if (!(serialDevice = serialOpenDevice(device))) goto __errexit;
	serialRestartDevice(serialDevice, 9600);
	/* hm, how to switch to 38400 ? 
	serialWriteData(serialDevice, "\eV\r", 3);
	serialDiscardInput(serialDevice);
	serialSetBaud(serialDevice, 38400);
	*/

	message("BRLTTY Ready", 0);
	beep();
	return 1;

__errexit:
	LogPrint(LOG_ERR, "Cannot initialize MiniBraille");
	return 0;
}

static void brl_close(BrailleDisplay *brl)
{
	serialCloseDevice(serialDevice);
}

static void refresh(void)
{
	unsigned char datab[] = { 27, 'Z', '1' };
	unsigned char datae[] = { 13 };
	serialWriteData(serialDevice, datab, sizeof datab);
	serialWriteData(serialDevice, status, sizeof status);
	serialWriteData(serialDevice, xlated, sizeof xlated);
	serialWriteData(serialDevice, datae, sizeof datae);
};

static void brl_writeWindow(BrailleDisplay *brl)
{
	int i;
	for (i = 0; i < 20; i++) xlated[i] = outputTable[brl->buffer[i]];
	refresh();
}


static int brl_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext context)
{
	unsigned char znak;
	int rv;
	rv = serialReadData(serialDevice, &znak, 1, 0, 0);
	switch (mode) {
	case NORMAL_MODE:
		if (rv == -1) return EOF;
		if (rv == 0) return EOF;
		switch (znak) {
		case KEY_F1:
			mode = F1_MODE;
			message("F1 mode active", 0);
			return BRL_CMD_NOOP;
		case KEY_F2:
			mode = F2_MODE;
			message("F2 mode active", 0);
			return BRL_CMD_NOOP;
		case KEY_UP:
			return BRL_CMD_LNUP;
		case KEY_DOWN:
			return BRL_CMD_LNDN;
		case KEY_C:
			return BRL_CMD_HOME;
		case KEY_LEFT:
			return BRL_CMD_FWINLT;
		case KEY_RIGHT:
			return BRL_CMD_FWINRT;
		};
		beep();
		return EOF; /* cannot do anythink */
	case F1_MODE:
		if (rv == 0) { usleep(20000); return BRL_CMD_NOOP; };
		mode = NORMAL_MODE;
		if (rv == -1) return BRL_CMD_NOOP;
		switch (znak) {
		case KEY_F1:
			mode = MANAGE_MODE; /* F1-F1 prechod na management */
			message("Management mode", 0);
			return BRL_CMD_NOOP;
		case KEY_F2:
			return BRL_CMD_CSRVIS;
		case KEY_LEFT:
			return BRL_CMD_LNBEG;
		case KEY_RIGHT:
			return BRL_CMD_LNEND;
		case KEY_UP:
			return BRL_CMD_TOP_LEFT;
		case KEY_DOWN:
			return BRL_CMD_BOT_LEFT;
		case KEY_C:
			return BRL_CMD_CAPBLINK;
		};
		beep();
		return EOF;
	case F2_MODE:
		if (rv == 0) { usleep(20000); return BRL_CMD_NOOP; };
		mode = NORMAL_MODE;
		if (rv == -1) return BRL_CMD_NOOP;
		switch (znak) {
		case KEY_F1:
			return BRL_CMD_CSRSIZE;
		case KEY_F2:
			return BRL_CMD_SKPIDLNS;
		case KEY_C:
			return BRL_CMD_CSRTRK;
		case KEY_RIGHT:
			return BRL_CMD_SIXDOTS;
		case KEY_DOWN:
			return BRL_CMD_DISPMD;
		case KEY_LEFT:
			return BRL_CMD_INFO;
		case KEY_UP: /* stepmode (by melo byt) */
			return BRL_CMD_FREEZE;
		};
		beep();
		return EOF;
	case CLOCK_MODE: {
			char t[200];
			struct tm* c;
			time_t cas;
			cas = time(NULL);
			c = localtime(&cas);
			strftime(t, sizeof t, "%e.%m %Y %H:%M:%S", c);
			message(t, 0);
		};
		if (rv == 0) { usleep(500000); return BRL_CMD_NOOP; };
		if (rv == -1) return BRL_CMD_NOOP;
		mode = NORMAL_MODE;
		break;
	case MANAGE_MODE:
		if (rv == 0) { usleep(50000); return BRL_CMD_NOOP; };
		mode =  NORMAL_MODE;
		if (rv == -1) return BRL_CMD_NOOP;
		switch (znak) {
		case KEY_F1:
			return BRL_CMD_HELP;
		case KEY_F2:
			return BRL_CMD_INFO;
		case KEY_C:
			return BRL_CMD_PREFLOAD;
		case KEY_LEFT:
			return BRL_CMD_RESTARTBRL;
		case KEY_RIGHT:
			mode = CLOCK_MODE;
			return BRL_CMD_NOOP;
		case KEY_UP:
			return BRL_CMD_PREFMENU;
		case KEY_DOWN:
			return BRL_CMD_PREFSAVE;
		};
	};
	return EOF; /* never should reach this */
}

static void brl_writeStatus(BrailleDisplay *brl, const unsigned char *s)
{
	status[0] = outputTable[s[0]];
	status[1] = outputTable[s[1]];
	/* refresh(); */
}

