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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "variolow.h"
#include "Programs/misc.h"
#include "Programs/brl.h"
#include "Programs/serial.h"


	/*	The filedescriptor of the open port, sorry to say, wee need a global
	 *	one here */ 
static SerialDevice *serialDevice=NULL;
static TranslationTable outputTable;

int varioinit(const char *device) 
{
	{
		static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
		makeOutputTable(&dots, &outputTable);
	}

	if (!isSerialDevice(&device)) {
		unsupportedDevice(device);
		return -1;
	}

	if((serialDevice = serialOpenDevice(device))) {
		serialSetParity(serialDevice, SERIAL_PARITY_ODD);
		if (serialRestartDevice(serialDevice, 19200)) {
			if (varioreset() == 0) return 0;
		}

		serialCloseDevice(serialDevice);
		serialDevice=NULL;
	}

	return -1;
}

int varioclose(void)
{
	if(serialDevice) {
		serialCloseDevice(serialDevice);
		serialDevice=NULL;
		return 0;
	}

	return -1;
}

int varioreset(void) 
{
	if(serialDevice) {
		char c=VARIO_RESET;
		int n=serialWriteData(serialDevice,&c,1);
		if (n==1) return 0;
	}

	return -1;
}

int variodisplay(const unsigned char *buff)
{
	if (serialDevice) {
		int n;
		unsigned char outbuff[VARIO_DISPLAY_DATA_LEN + 40];
		memcpy(outbuff,VARIO_DISPLAY_DATA,VARIO_DISPLAY_DATA_LEN);
		memcpy(outbuff+VARIO_DISPLAY_DATA_LEN,buff,40);
		if((n=serialWriteData(serialDevice,outbuff,sizeof(outbuff)))==sizeof(outbuff)) return 0;
	}
	
	return -1;
}

int varioget(void)
{
	if(serialDevice) {
		unsigned char c;
		if(serialReadData(serialDevice,&c,1,0,0)==1) return c;
	}

	return -1;
}

int variotranslate(const unsigned char *frombuff, unsigned char *tobuff,int count) 
{
	for(;count>0;count--) {
		tobuff[count-1]=outputTable[frombuff[count-1]];
	}

	return 0;
}
