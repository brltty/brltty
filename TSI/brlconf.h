/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.9.0, 06 April 1998
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <s.doyon@videotron.ca>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* TSI/brlconf.h - Braille display driver for TSI displays
 *
 * Written by Stephane Doyon (doyons@JSP.UMonteal.CA)
 *
 * This is version 1.1 (September 25ft 1996) of the TSI driver.
 * It attempts full support for Navigator 20/40/80 and Powerbraille 40.
 * It is designed to be compiled into version 1.0.2 of BRLTTY.
 */

/* Configuration file for the TSI driver.
 * Edit as needed...
 */

/* Delay before typematic key repetitions
   Time before a key you hold down will start being repeated */
#define BRL_TYPEMATIC_DELAY 9
/* Speed of typematic key repetitions
   Delay between repetitions of a key when you hold it down */
#define BRL_TYPEMATIC_REPEAT 9

/* Note that these two previous options correspond to gateway's "/panelrep"
   option, except the numbers here are the square of the values you'd use
   with gateway. 9 and 9 is my favorite setting, which is quicker than
   navigator and gateway's default.
*/

/* Cut-and-paste cursor
   Initial position (cell) of the special cut-and-paste cursor
   Set to 0 for leftmost, 1 for middle, 2 for rightmost */
#define CUT_CSR_INIT_POS 0
/* Cut-and-paste cursor shape
   Defines the dots that form the cut-and-paste cursor
   Possible values: 0xFF for all dots, 0xC0 for dots 7-8, ... */
#define CUT_CSR_CHAR 0xFF

/* Flicker: how to flicker the braille display to attract attention
   Character to set the display to when flickering
   Possible values: 0 for spaces, 0xFF for all dots */
#define FLICKER_CHAR 0
/* How long the flicker lasts in milliseconds
   There is a minimum of a few milliseconds, so 0 means minimum delay */
#define FLICKER_DELAY 0

/* Low battery warning
   Comment it out if you don't want it */
#define LOW_BATTERY_WARN
#ifdef LOW_BATTERY_WARN
  /* How long does the message stay on the display */
  #define BATTERY_DELAY (1750) /* milliseconds */
#endif

/* You should never have to change anything bellow this line */

/* Braille display parameters */

/* Number of braille cells
   This is autodetected. Only use this to override.
   Possible values: 20/40/80 */
/*#define BRLCOLS 40*/

/* routing keys
   Define to 1 if you have routing keys, to 0 if you don't.
   This too is autodetected. Only use this to override. */
/*#define HAS_SW 1*/

/* End of driver config */
