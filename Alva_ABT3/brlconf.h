/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.0, 26 July 1996
 *
 * Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
 */

/* Alva_ABT3/brlconf.h - Configurable definitions for the Alva ABT3xx series
 * Copyright (C) 1995-1996 by Nicolas Pitre <nico@cam.org>
 *
 * Edit as necessary for your system.
 */



/* Device Identification Numbers (not to be changed) */
#define ABT320          0
#define ABT340          1
#define ABT380          2
#define ABT34D          3	/* ABT340 Desktop */
#define ABT38D          4	/* ABT380 Twin Space */
#define NB_MODEL        5



/***** User Settings *****/

/* uncomment next line if you have a firmware older than version 010495 */
/* #define ABT3_OLD_FIRMWARE */

/* Specify the terminal model that you'll be using.
 * Use one of the previously defined device ID.
 */
#define MODEL   ABT340

/* serial line baudrate... 
 * Note that default braille device is defined it ../Makefile
 */
#define BAUDRATE B9600

/* typematic settings */
#define TYPEMATIC_DELAY 10	/* nbr of cycles before a key is repeated */
#define TYPEMATIC_REPEAT 2	/* nbr of cycles between each key repeat */
