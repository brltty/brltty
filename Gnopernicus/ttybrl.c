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
#include <brltty/brlapi.h>

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
    unsigned char 	sendbuff[256];
  
			
    if (count > len) 
	return 0;

  /* Gnopernicus's idea of how to arrange braille dots
   * is slightly different from BRLTTY's representation.
   */
    for (i = 0; i < count; i++) 
    {
	unsigned char val = 0;
	
	if (dots[i] & 0x01 ) 
	    val = val|B1;
	if (dots[i] & 0x02 ) 
	    val = val|B2;
	if (dots[i] & 0x04 ) 
	    val = val|B3;
	if (dots[i] & 0x08 ) 
	    val = val|B4;
	if (dots[i] & 0x10 ) 
	    val = val|B5;
	if (dots[i] & 0x20 ) 
	    val = val|B6;
	if (dots[i] & 0x40 ) 
	    val = val|B7;
	if (dots[i] & 0x80 ) 
	    val = val|B8;
	
	sendbuff[i] = val;
    }
    if (count < len) 
    {
	memset (&sendbuff[count], 0, len-count);
    }

    if (brlapi_writeBrlDots(sendbuff) == 0) 
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

    while (brlapi_readCommand (0, &keypress ) == 1) 
    {
	/* TODO: Find a better way to map brltty commands to gnopernicus keys. */
	switch (keypress & ~VAL_SWITCHMASK) 
	{
	    case CMD_LNUP:
    		sprintf(&dd.key_codes[0], "DK00");
    		bec = bec_key_codes;
    		bed.KeyCodes = dd.key_codes;						
    		ClientCallback (bec, &bed);
    		break;
      
	    case CMD_HOME:
    		sprintf(&dd.key_codes[0], "DK01");
    		bec = bec_key_codes;
    		bed.KeyCodes = dd.key_codes;						
    		ClientCallback (bec, &bed);
    		break;

	    case CMD_LNDN:
    		sprintf(&dd.key_codes[0], "DK02");
    		bec = bec_key_codes;
    		bed.KeyCodes = dd.key_codes;						
    		ClientCallback (bec, &bed);
    		break;
      
	    case CMD_FWINLT:
    		sprintf(&dd.key_codes[0], "DK03");
    		bec = bec_key_codes;
    		bed.KeyCodes = dd.key_codes;						
    		ClientCallback (bec, &bed);
    		break;
      
	    case CMD_FWINRT:
    		sprintf(&dd.key_codes[0], "DK05");
        	bec = bec_key_codes;
    		bed.KeyCodes = dd.key_codes;						
    		ClientCallback (bec, &bed);
    		break;
      
    	    /* TBD: Default action (DK01DK02) and repeat last */
	    default: 
	    {
    		int key = keypress & VAL_BLK_MASK;
    		int arg = keypress & VAL_ARG_MASK;

    		switch (key) 
    		{
    		    case CR_ROUTE:
			sprintf(&dd.sensor_codes[0], "HMS%02d", arg);
			bec = bec_sensor;
			bed.Sensor.SensorCodes = dd.sensor_codes;
			ClientCallback (bec, &bed);
			break;
		    default:
			sprintf(&dd.key_codes[0], "DK%02d", keypress);
			fprintf(stderr,"default: Sending %s\n", dd.key_codes);
			bec = bec_key_codes;
			bed.KeyCodes = dd.key_codes;						
			ClientCallback (bec, &bed);
			break;
    		}
    		break;
	    }
	}
    }
    return TRUE;
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

    fprintf(stderr,"Requesting TTY 7 (currently hardcoded)\n");
    /* TODO: Write a function which reliably determines the TTY
     where the X server is running.  No idea how to do this, we might
     need a config setting for this? */
    brlapi_getTty (7, BRLCOMMANDS, NULL);

    /* fill device functions for the upper level */
    Device->send_dots = brltty_brl_send_dots;
    Device->close_device = brltty_brl_close_device;
	
    /* Setup glib polling for the socket fd of brlapi */
    gioch = g_io_channel_unix_new (fd);
    g_io_add_watch (gioch, G_IO_IN | G_IO_PRI, brltty_brl_glib_cb, NULL);

    ClientCallback = DeviceCallback;

    return 1;
}

