/* ttybrl.c
 *
 * Copyright 2003, Mario Lang
 * Copyright 2003, BAUM Retec A.G.
 * Copyright 2003, Sun Microsystems Inc. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* This file implements an interface to BRLTTY's BrlAPI
 *
 * BrlAPI implements generic display access.  This means
 * that every display supported by BRLTTY should work via this
 * driver.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <brltty/brldefs.h>
#include <brltty/api.h>

#include "braille.h"

typedef struct
{
  int 		x;
  int 		y;

  unsigned char key_codes[512];
  unsigned char sensor_codes[32];
} BRLTTY_DEVICE_DATA;

/* Globals */
static GIOChannel 		*gioch = NULL;
static BRL_DEV_CALLBACK		ClientCallback = NULL;
static BRLTTY_DEVICE_DATA 	dd;

/* Functions */

/* function brltty_brl_send_dots
 *
 * Send array of dots of length count to brlapi.  Argument blocking is
 * ignored.
 */
int
brltty_brl_send_dots (unsigned char 	*dots, 
		      short 		count, 
		      short 		blocking)
{
    int 		i, 
			len = dd.x * dd.y;
    unsigned char 	dotsbuff[256];

    brlapi_writeStruct ws = BRLAPI_WRITESTRUCT_INITIALIZER;
  
			
    if (count > len) 
	return 0;

  /* Gnopernicus's idea of how to arrange braille dots
   * is slightly different from BRLTTY's representation.
   */
    for (i = 0; i < count; i++) 
    {
	unsigned char val = 0;
	
	if (dots[i] & 0x01 ) 
	    val = val|BRL_DOT1;
	if (dots[i] & 0x02 ) 
	    val = val|BRL_DOT2;
	if (dots[i] & 0x04 ) 
	    val = val|BRL_DOT3;
	if (dots[i] & 0x08 ) 
	    val = val|BRL_DOT4;
	if (dots[i] & 0x10 ) 
	    val = val|BRL_DOT5;
	if (dots[i] & 0x20 ) 
	    val = val|BRL_DOT6;
	if (dots[i] & 0x40 ) 
	    val = val|BRL_DOT7;
	if (dots[i] & 0x80 ) 
	    val = val|BRL_DOT8;
	
	dotsbuff[i] = val;
    }
    if (count < len) 
    {
	memset (&dotsbuff[count], 0, len-count);
    }

    ws.attrOr = dotsbuff;
    ws.cursor = cursorPosition;

    if (brlapi_write(&ws) == 0) 
	return 1;
    else 
	return 0;
}

void
brltty_brl_close_device ()
{
    brlapi_leaveTty();
    brlapi_closeConnection();
}

static gboolean
brltty_brl_glib_cb (GIOChannel *source, GIOCondition condition, gpointer data)
{
    brl_keycode_t	keypress;

    BRAILLE_EVENT_CODE 	bec;
    BRAILLE_EVENT_DATA 	bed;

    while (brlapi_readKey (0, &keypress) == 1) 
    {
	/* TODO: Find a better way to map brltty commands to gnopernicus keys. */
	switch (keypress & ~BRL_FLG_TOGGLE_MASK) 
	{
	    case BRL_CMD_LNUP:
    		sprintf(&dd.key_codes[0], "DK00");
    		bec = bec_key_codes;
    		bed.KeyCodes = dd.key_codes;						
    		ClientCallback (bec, &bed);
    		break;
      
	    case BRL_CMD_HOME:
    		sprintf(&dd.key_codes[0], "DK01");
    		bec = bec_key_codes;
    		bed.KeyCodes = dd.key_codes;						
    		ClientCallback (bec, &bed);
    		break;

	    case BRL_CMD_LNDN:
    		sprintf(&dd.key_codes[0], "DK02");
    		bec = bec_key_codes;
    		bed.KeyCodes = dd.key_codes;						
    		ClientCallback (bec, &bed);
    		break;
      
	    case BRL_CMD_FWINLT:
	    case BRL_CMD_FWINLTSKIP:
    		sprintf(&dd.key_codes[0], "DK03");
    		bec = bec_key_codes;
    		bed.KeyCodes = dd.key_codes;						
    		ClientCallback (bec, &bed);
    		break;
      
	    case BRL_CMD_FWINRT:
	    case BRL_CMD_FWINRTSKIP:
    		sprintf(&dd.key_codes[0], "DK05");
        	bec = bec_key_codes;
    		bed.KeyCodes = dd.key_codes;						
    		ClientCallback (bec, &bed);
    		break;
      
    	    /* TBD: Default action (DK01DK02) and repeat last */
	    default: 
	    {
    		int key = keypress & BRL_MSK_BLK;
    		int arg = keypress & BRL_MSK_ARG;

    		switch (key) 
    		{
    		    case BRL_BLK_ROUTE:
			sprintf(&dd.sensor_codes[0], "HMS%02d", arg);
			bec = bec_sensor;
			bed.Sensor.SensorCodes = dd.sensor_codes;
			ClientCallback (bec, &bed);
			break;
		    default:
			fprintf(stderr,"BRLTTY command code not bound to Gnopernicus key code: 0X%04X\n", keypress);
			break;
    		}
    		break;
	    }
	}
    }
    return TRUE;
}

static void
ignoreBlock (int block)
{
    brlapi_ignoreKeyRange(block, block|BRL_MSK_ARG);
}

static void
ignoreInput (int block)
{
    static const int flags[] = {
                         0 |                  0 |                 0 |                    0,
                         0 |                  0 |                 0 | BRL_FLG_CHAR_CONTROL,
                         0 |                  0 | BRL_FLG_CHAR_META |                    0,
                         0 |                  0 | BRL_FLG_CHAR_META | BRL_FLG_CHAR_CONTROL,
                         0 | BRL_FLG_CHAR_UPPER |                 0 |                    0,
                         0 | BRL_FLG_CHAR_UPPER |                 0 | BRL_FLG_CHAR_CONTROL,
                         0 | BRL_FLG_CHAR_UPPER | BRL_FLG_CHAR_META |                    0,
                         0 | BRL_FLG_CHAR_UPPER | BRL_FLG_CHAR_META | BRL_FLG_CHAR_CONTROL,
        BRL_FLG_CHAR_SHIFT |                  0 |                 0 |                    0,
        BRL_FLG_CHAR_SHIFT |                  0 |                 0 | BRL_FLG_CHAR_CONTROL,
        BRL_FLG_CHAR_SHIFT |                  0 | BRL_FLG_CHAR_META |                    0,
        BRL_FLG_CHAR_SHIFT |                  0 | BRL_FLG_CHAR_META | BRL_FLG_CHAR_CONTROL,
        BRL_FLG_CHAR_SHIFT | BRL_FLG_CHAR_UPPER |                 0 |                    0,
        BRL_FLG_CHAR_SHIFT | BRL_FLG_CHAR_UPPER |                 0 | BRL_FLG_CHAR_CONTROL,
        BRL_FLG_CHAR_SHIFT | BRL_FLG_CHAR_UPPER | BRL_FLG_CHAR_META |                    0,
        BRL_FLG_CHAR_SHIFT | BRL_FLG_CHAR_UPPER | BRL_FLG_CHAR_META | BRL_FLG_CHAR_CONTROL
    };
    const int *flag = flags + (sizeof(flags) / sizeof(*flag));
    do {
        ignoreBlock(block | *--flag);
    } while (*flag);
}

int
brltty_brl_open_device (char* 			DeviceName, 
			short 			Port,
			BRL_DEV_CALLBACK 	DeviceCallback, 
			BRL_DEVICE 		*Device)
{
    int fd;

    if ( (fd = brlapi_initializeConnection (NULL, NULL) ) < 0) 
    {
	fprintf(stderr, "Error opening brlapi connection");
	return 0;
    }

    if (brlapi_getDisplaySize (&dd.x, &dd.y) != 0) 
    {
	fprintf(stderr, "Unable to get display size");
	return 0;
    }

    fprintf(stderr, "BrlAPI detected a %dx%d display\n", dd.x, dd.y);

    Device->CellCount = dd.x * dd.y;
    Device->DisplayCount = 1; /* No status cells implemented yet */

    Device->Displays[0].StartCell 	= 0;
    Device->Displays[0].Width 		= dd.x;
    Device->Displays[0].Type 		= bdt_main;

    /* fill device functions for the upper level */
    Device->send_dots = brltty_brl_send_dots;
    Device->close_device = brltty_brl_close_device;
	
    /* Setup glib polling for the socket fd of brlapi */
    gioch = g_io_channel_unix_new (fd);
    g_io_add_watch (gioch, G_IO_IN | G_IO_PRI, brltty_brl_glib_cb, NULL);

    ClientCallback = DeviceCallback;

    /* TODO: Write a function which reliably determines the TTY
     where the X server is running.  
     Currently if the tty is set to 0 the function brlapi_getTty will look at the CONTROLVT environment
     variable
     CONTROLVT="$(grep "using VT number" "/var/log/XFree86.$(echo "$DISPLAY" | sed -e "s/^.*::*\([0-9]*\).*$/\1/").log" | sed -e "s/^.*using VT number \([0-9]*\).*$/\1/")"
    */
    brlapi_getTty (0, BRLCOMMANDS);

    ignoreInput(BRL_BLK_PASSCHAR);
    ignoreInput(BRL_BLK_PASSDOTS);
    ignoreBlock(BRL_BLK_PASSKEY);

    return 1;
}

