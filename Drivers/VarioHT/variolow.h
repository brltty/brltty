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

#ifndef VARIO_LOW_INCLUDE 
#define VARIO_LOW_INCLUDE 
	/*	Doesnt really know what this one does, seems to have no effect on the
	 *	display .. fishy! */ 
#define VARIO_RESET		0xff
	/*	Sent back from the display after a data display call */ 
#define VARIO_DISPLAY_DATA_ACK		0x7e
	/*	Header (and length of header) for sending the 40 display bytes */ 
#define VARIO_DISPLAY_DATA	"\001\000\000\000\000"
#define VARIO_DISPLAY_DATA_LEN	5
	/*	Cursor movement keys have codes in the range of 0x20-0x6f (left to right)
	 *	for press and 0xa0-0xef for release */
#define VARIO_CURSOR_BASE	0xa0
#define VARIO_CURSOR_COUNT	40
	/*	The six buttons press and release codes .. they are numbered from
	 *	top left to bottom right like:
	 * 	1	4
	 *	2	5
	 *	3	6 */
#define VARIO_PUSHBUTTON_PRESS_1	0x04
#define VARIO_PUSHBUTTON_PRESS_2	0x03
#define VARIO_PUSHBUTTON_PRESS_3	0x08
#define VARIO_PUSHBUTTON_PRESS_4	0x07
#define VARIO_PUSHBUTTON_PRESS_5	0x0b
#define VARIO_PUSHBUTTON_PRESS_6	0x0f

#define VARIO_PUSHBUTTON_RELEASE_1	0x84
#define VARIO_PUSHBUTTON_RELEASE_2	0x83
#define VARIO_PUSHBUTTON_RELEASE_3	0x88
#define VARIO_PUSHBUTTON_RELEASE_4	0x87
#define VARIO_PUSHBUTTON_RELEASE_5	0x8b
#define VARIO_PUSHBUTTON_RELEASE_6	0x8f

/*
		bit		orig dot - 1
		0		0
		1		3
		2		1
		3		4
		4		2
		5		5
		6		6
		7		7
		
*/
#define CHAR_TO_VARIO_CHAR(a) (((a)&0xe1)|(((a)&0x02)<<2)|(((a)&0x04)>>1)|(((a)&0x08)<<1)|(((a)&0x10)>>2))




	/*	Open and set the serial port right */ 
int varioinit(char *dev) ;
	/*	Close the serial comm and flush buffers */ 
int varioclose();
	/*	Send reset */ 
int varioreset();
	/*	Send the ready formatted display byuffer to the vario */ 
int variodisplay(char *buff);
	/*	Check if any data available in the input buffers */ 
int variocheckwaiting();
	/*	Get data from the varrio, block until available  */ 
int varioget();
	/*	Translates the given buffer into vario special chars */ 
int variotranslate(char *frombuff, char *tobuff,int maxcnt);

#endif /* VARIO_LOW_INCLUDE */
