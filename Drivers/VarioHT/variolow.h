/*
 * BRLTTY - A background process providing access to the console screen (when in
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

#ifndef VARIO_LOW_INCLUDE 
#define VARIO_LOW_INCLUDE 
	/*	Doesnt really know what this one does, seems to have no effect on the
	 *	display .. fishy! */ 
#define VARIO_RESET		0XFF
#define VARIO_MODEL_MODULAR40	0X89
	/*	Sent back from the display after a data display call */ 
#define VARIO_DISPLAY_DATA_ACK		0X7E
	/*	Header (and length of header) for sending the 40 display bytes */ 
#define VARIO_DISPLAY_DATA	"\001\000\000\000\000"
#define VARIO_DISPLAY_DATA_LEN	5

#define VARIO_RELEASE_FLAG	0X80
	/*	Cursor movement keys have codes in the range of 0x20-0x6f (left to right)
	 *	for press and 0xa0-0xef for release */
#define VARIO_CURSOR_BASE	0X20
#define VARIO_CURSOR_COUNT	40
	/*	The press codes of the six buttons .. they are numbered from
	 *	top left to bottom right like:
	 * 	1	4
	 *	2	5
	 *	3	6 */
#define VARIO_PUSHBUTTON_1	0X04
#define VARIO_PUSHBUTTON_2	0X03
#define VARIO_PUSHBUTTON_3	0X08
#define VARIO_PUSHBUTTON_4	0X07
#define VARIO_PUSHBUTTON_5	0X0b
#define VARIO_PUSHBUTTON_6	0X0f

	/*	Open and set the serial port right */ 
int varioinit(const char *device);
	/*	Close the serial comm and flush buffers */ 
int varioclose();
	/*	Send reset */ 
int varioreset(void);
	/*	Send the ready formatted display byuffer to the vario */ 
int variodisplay(const unsigned char *buff);
	/*	Get data from the varrio, block until available  */ 
int varioget(void);
	/*	Translates the given buffer into vario special chars */ 
int variotranslate(const unsigned char *frombuff, unsigned char *tobuff,int maxcnt);

#endif /* VARIO_LOW_INCLUDE */
