/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

/* Alva/brlconf.h - Configurable definitions for the Alva driver
 * Copyright (C) 1995-1998 by Nicolas Pitre <nico@cam.org>
 *
 */

#define BRLNAME	"Alva"

/* Known Device Identification Numbers (not to be changed) */
#define ABT_AUTO	-1	/* for new firmware only */
#define ABT320          0
#define ABT340          1
#define ABT34D          2
#define ABT380          3	/* ABT340 Desktop */
#define ABT38D          4	/* ABT380 Twin Space */
#define DEL440		11	/* Alva Delphi 40 */
#define DEL480		13 	/* Delphi Multimedia */
#define SAT540		14	/* Alva Satellite 540 */
#define SAT570 		15 	/* Alva Satellite 570 */


/***** User Settings *****  Edit as necessary for your system. */


/* Define next line to 1 if you have a firmware older than version 010495 */
#define ABT3_OLD_FIRMWARE 0


/* Specify the terminal model that you'll be using.
 * Use one of the previously defined device ID.
 * WARNING: Do not use BRL_AUTO if you have ABT3_OLD_FIRMWARE defined to 1 !
 */
#define MODEL   ABT_AUTO


#define BAUDRATE B9600


/* typematic settings */
#define TYPEMATIC_DELAY 10	/* nbr of cycles before a key is repeated */
#define TYPEMATIC_REPEAT 2	/* nbr of cycles between each key repeat */


/* Delay in miliseconds between forced full refresh of the display.
 * This is to minimize garbage effects due to noise on the serial line.
 */
#define REFRESH_RATE 1000

/* Define the preferred/default status cells mode. */
#define PREFSTYLE ST_AlvaStyle
