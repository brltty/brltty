/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 */

/* This Driver was written as a project in the
 *   HTL W1, Abteilung Elektrotechnik, Wien - Österreich
 *   (Technical High School, Department for electrical engineering,
 *     Vienna, Austria)
 *  by
 *   Tibor Becker
 *   Michael Burger
 *   Herbert Gruber
 *   Heimo Schön
 * Teacher:
 *   August Hörandl <hoerandl@elina.htlw1.ac.at>
 *
 * papenmeier/brlconf.h - Braille display library for Papenmeier Screen 2D
 *
 * Edit as necessary for your system.
 */

/* So far, there is only support for serial communications, and
 * only the builtin table is used for character translation
 */

#define BRLNAME	"Papenmeier"

#define BRLCOLSMAX   80 

#define BAUDRATE B19200

/* codes used in protocoll */
#define cSTX 02
#define cETX 03
#define cIdSend 'S'
#define cIdReceive 'K'
#define PRESSED 1

/* maximum number of status cells */
#define PMSC 22

/* offset within data structure */
/* additional offset - use internal table */
#define offsetTable      512

/* debug output */
#undef WR_DEBUG
#undef RD_DEBUG
#undef MOD_DEBUG

/*
  #define MOD_DEBUG
  #define WR_DEBUG
  #define RD_DEBUG
*/

/* Define the preferred/default status cells mode. */
#define PREFSTYLE ST_Papenmeier

#define CONFIG_ENV   "BRLTTY_CONF"
#define CONFIG_FILE  "/etc/brltty/brltty.pm.conf"

#define MAXPATH  128
