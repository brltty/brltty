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
#include <gdk/gdkwindow.h>
#include <gdk/gdkproperty.h>
#include <gdk/gdkx.h>

#include <brltty/brldefs.h>
#include <brltty/api.h>

#include <X11/Xatom.h>

#include "braille.h"
#include "srintl.h"

typedef struct
{
    gint   x;
    gint   y;
    guchar key_codes[512];
    guchar sensor_codes[32];
} BRLTTYDeviceData;

/* Globals */
static GIOChannel 		*gioch = NULL;
static BRLDevCallback		client_callback = NULL;
static BRLTTYDeviceData 	dd;

/* Functions */


/* Send array of dots of length count to brlapi.  Argument blocking is ignored. */
gint
brltty_brl_send_dots (guchar 	*dots, 
		      gshort 	count, 
		      gshort 	blocking)
{
    gint 		i, 
			len = dd.x * dd.y;
    guchar 	        sendbuff[256];

    brlapi_writeStruct ws = BRLAPI_WRITESTRUCT_INITIALIZER;
  
    if (count > len) 
	return 0;

    /* Gnopernicus's idea of how to arrange braille dots
    * is slightly different from BRLTTY's representation.
    */
    for (i = 0; i < count; i++) 
    {
	guchar val = 0;
	
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
	
	sendbuff[i] = val;
    }
    if (count < len) 
    {
	memset (&sendbuff[count], 0, len-count);
    }

    ws.attrOr = sendbuff;
    if (brlapi_write(&ws) == 0) 
	return 1;
    else 
    {
        brlapi_perror("brlapi_write");
	return 0;
    }
}

void
brltty_brl_close_device ()
{
    brlapi_leaveTty();
    brlapi_closeConnection();
}

static void
send_key (gint layer,
          gint key)
{
    BRLEventData bed;
    sprintf(&dd.key_codes[0], "L%02dK%02d", layer, key);
    bed.key_codes = dd.key_codes;						
    client_callback(BRL_EVCODE_KEY_CODES, bed);
}

static gboolean
brltty_brl_glib_cb (GIOChannel   *source, 
		    GIOCondition condition, 
		    gpointer     data)
{
    brl_keycode_t	keypress;

    BRLEventCode 	bec;
    BRLEventData 	bed;

    while (brlapi_readKey (0, &keypress) == 1) 
    {
	/* This is an attempt to match as many BRLTTY commands
	 * to Gnopernicus equivalents as possible.
	 * Not all drivers will send all commands.
	 * If the user undefines the Gnopernicus key, the BRLTTY
	 * command will stop working.
	 */
	switch (keypress & BRL_MSK_CMD) 
	{
	    /*goto title*/
	    case BRL_CMD_TOP:
		send_key(0, 7);
		break;

	    /*goto statusbar*/
	    case BRL_CMD_BOT:
		send_key(0, 3);
		break;

	    /*goto menu*/
	    case BRL_CMD_TOP_LEFT:
		send_key(0, 9);
		break;

	    /*goto toolbar*/
	    case BRL_CMD_BOT_LEFT:
		send_key(0, 1);
		break;

	    /*goto focus*/
	    case BRL_CMD_HOME:
	    case BRL_CMD_RETURN:
		send_key(0, 12);
		break;

	    /*goto parent*/
	    case BRL_CMD_PRPROMPT:
		send_key(0, 8);
		break;

	    /*goto child*/
	    case BRL_CMD_NXPROMPT:
		send_key(0, 2);
		break;

	    /*goto first*/
	    case BRL_CMD_PRPGRPH:
		send_key(0, 15);
		break;

	    /*goto last*/
	    case BRL_CMD_NXPGRPH:
		send_key(0, 14);
		break;

	    /*goto previous*/
	    case BRL_CMD_LNUP:
	    case BRL_CMD_PRDIFLN:
		send_key(0, 4);
		break;
	    
	    /*goto next*/
	    case BRL_CMD_LNDN:
	    case BRL_CMD_NXDIFLN:
		send_key(0, 6);
		break;

	    /*toggle FocusTracking/FlatReview mode*/
	    case BRL_CMD_FREEZE:
		send_key(0, 10);
		break;

	    /*display left*/
	    case BRL_CMD_FWINLT:
	    case BRL_CMD_FWINLTSKIP:
    		send_key(9, 4);
		break;

	    /*display right*/
	    case BRL_CMD_FWINRT:
	    case BRL_CMD_FWINRTSKIP:
		send_key(9, 6);
		break;

	    /*char left*/
	    case BRL_CMD_CHRLT:
		send_key(9, 1);
		break;

	    /*char right*/
	    case BRL_CMD_CHRRT:
		send_key(9, 3);
		break;

	    /*decrease speech volume*/
	    case BRL_CMD_SAY_SOFTER:
		send_key(8, 1);
		break;

	    /*increase speech volume*/
	    case BRL_CMD_SAY_LOUDER:
		send_key(8, 3);
		break;
		    
	    /*decrease speech rate*/
	    case BRL_CMD_SAY_SLOWER:
		send_key(8, 4);
		break;

	    /*increase speech rate*/
	    case BRL_CMD_SAY_FASTER:
		send_key(8, 6);
		break;
		    
	    default:
	    {
		gint key = keypress & BRL_MSK_BLK;
		gint arg = keypress & BRL_MSK_ARG;

		switch (key) 
		{
		    case BRL_BLK_ROUTE:
			sprintf(&dd.sensor_codes[0], "HMS%02d", arg);
			bec = BRL_EVCODE_SENSOR;
			bed.sensor.sensor_codes = dd.sensor_codes;
			client_callback (bec, &bed);
			break;
		    default:
			//to be translated
			fprintf(stderr, "BRLTTY command code not bound to Gnopernicus key code: 0X%04X\n", keypress);
			break;
		}
		break;
	    }
	}
    }
    return TRUE;
}

static void
ignore_block (gint block)
{
    if (brlapi_ignoreKeyRange(block, block|BRL_MSK_ARG) == -1) {
	brlapi_perror("brlapi_ignoreKeyRange");
	fprintf(stderr,"(block 0x%x)", block);
    }
}

static void
ignore_input (gint block)
{
    static const gint flags[] = {
    		         0 |		      0 |          	  0 |           	 0,
	                 0 |         	      0 |        	  0 | BRL_FLG_CHAR_CONTROL,
    		         0 |         	      0 | BRL_FLG_CHAR_META |           	 0,
	                 0 |         	      0 | BRL_FLG_CHAR_META | BRL_FLG_CHAR_CONTROL,
            		 0 | BRL_FLG_CHAR_UPPER |        	  0 |           	 0,
    		         0 | BRL_FLG_CHAR_UPPER |        	  0 | BRL_FLG_CHAR_CONTROL,
	                 0 | BRL_FLG_CHAR_UPPER | BRL_FLG_CHAR_META |            	 0,
            		 0 | BRL_FLG_CHAR_UPPER | BRL_FLG_CHAR_META | BRL_FLG_CHAR_CONTROL,
        BRL_FLG_CHAR_SHIFT |         	      0 |        	  0 |           	 0,
        BRL_FLG_CHAR_SHIFT |         	      0 |        	  0 | BRL_FLG_CHAR_CONTROL,
        BRL_FLG_CHAR_SHIFT |         	      0 | BRL_FLG_CHAR_META |           	 0,
        BRL_FLG_CHAR_SHIFT |   		      0 | BRL_FLG_CHAR_META | BRL_FLG_CHAR_CONTROL,
        BRL_FLG_CHAR_SHIFT | BRL_FLG_CHAR_UPPER |        	  0 |           	 0,
        BRL_FLG_CHAR_SHIFT | BRL_FLG_CHAR_UPPER |        	  0 | BRL_FLG_CHAR_CONTROL,
        BRL_FLG_CHAR_SHIFT | BRL_FLG_CHAR_UPPER | BRL_FLG_CHAR_META |           	 0,
        BRL_FLG_CHAR_SHIFT | BRL_FLG_CHAR_UPPER | BRL_FLG_CHAR_META | BRL_FLG_CHAR_CONTROL
    };
    const gint *flag = flags + (sizeof(flags) / sizeof(*flag));
    do {
        ignore_block(block | *--flag);
    } while (*flag);
}

gint
brltty_brl_open_device (gchar* 			device_name, 
			gshort 			port,
			BRLDevCallback 	        device_callback, 
			BRLDevice 		*device)
{
    gint fd;
    brlapi_settings_t settings;
    GdkWindow *root;
    GdkAtom VTAtom, type;
    guint VT = 0;
    gint format, length;
    unsigned char *buf;


    if ((fd = brlapi_initializeConnection (NULL, &settings)) < 0) 
    {
	brlapi_perror("Error opening brlapi connection"); //to be translated
	fprintf(stderr,"Please check that\n\
 - %s exists and contains some data\n\
 - you have read permission on %s\n\
 - BRLTTY is running\n", settings.authKey, settings.authKey); //to be translated
	return 0;
    }

    if (brlapi_getDisplaySize (&dd.x, &dd.y) != 0) 
    {
	brlapi_perror("Unable to get display size"); //to be translated
	return 0;
    }

    fprintf(stderr, "BrlAPI detected a %dx%d display\n", dd.x, dd.y); //to be translated

    device->cell_count = dd.x * dd.y;
    device->display_count = 1; /* No status cells implemented yet */

    device->displays[0].start_cell 	= 0;
    device->displays[0].width 		= dd.x;
    device->displays[0].type 		= BRL_DISP_MAIN;

    /* fill device functions for the upper level */
    device->send_dots = brltty_brl_send_dots;
    device->close_device = brltty_brl_close_device;
	
    /* Setup glib polling for the socket fd of brlapi */
    gioch = g_io_channel_unix_new (fd);
    g_io_add_watch (gioch, G_IO_IN | G_IO_PRI, brltty_brl_glib_cb, NULL);

    client_callback = device_callback;

    /* Determine the TTY where the X server is running.  

     This only works with XFree86 starting from 4.3.99.903 and X.Org
     starting from 6.8.0

     If you have an older version, the tty will be kept to 0 so that the
     brlapi_getTty() function will look at the CONTROLVT environment variable.
     So people having old versions should have something like this in their
     .xsession, before launching gnopernicus:

     CONTROLVT="$(grep "using VT number" "/var/log/XFree86.$(echo "$DISPLAY" | sed -e "s/^.*::*\([0-9]*\).*$/\1/").log" | sed -e "s/^.*using VT number \([0-9]*\).*$/\1/")"

     for XFree86

     CONTROLVT="$(grep "using VT number" "/var/log/Xorg.$(echo "$DISPLAY" | sed -e "s/^.*::*\([0-9]*\).*$/\1/").log" | sed -e "s/^.*using VT number \([0-9]*\).*$/\1/")"

     for X.Org

    */

    root = gdk_get_default_root_window();
    if (!root)
	goto gettty;

    VTAtom = gdk_atom_intern("XFree86_VT", TRUE);
    if (VTAtom == None)
	goto gettty;

    if (!(gdk_property_get(root, VTAtom, AnyPropertyType, 0, 1, FALSE,
	    &type, &format, &length, &buf)))
    {
	fprintf(stderr, "no XFree86_VT property\n"); //to be translated
	goto gettty;
    }

    if (length<1)
    {
	fprintf(stderr, "no item in XFree86_VT property\n"); //to be translated
    	goto gettty;
    }

    switch ((guint)type)
    {
	case XA_CARDINAL:
	case XA_INTEGER:
	case XA_WINDOW:
	    switch (format)
	    {
		case 8:  VT = (*(guint8  *)buf); break;
		case 16: VT = (*(guint16 *)buf); break;
		case 32: VT = (*(guint32 *)buf); break;
		default: fprintf(stderr, "Bad format for VT number\n"); break; //to be translated
	    }
	    break;
	default:
	    fprintf(stderr, "Bad type for VT number\n"); //to be translated
    }

gettty:
    if (brlapi_getTty (VT, BRLCOMMANDS) == -1)
    {
	brlapi_perror("Unable to get Tty"); //to be translated
	return 0;
    }

    ignore_input(BRL_BLK_PASSCHAR);
    ignore_input(BRL_BLK_PASSDOTS);
    ignore_block(BRL_BLK_PASSKEY);

    return 1;
}
