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
 * This Driver was written as a project in the
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
 * only the buildin table is used for character translation
 */

#define BRLCOLS	80
#define BRLROWS	1

#define BRLNAME	"Papenmeier Screen 2D"

#define BAUDRATE B19200

/* codes used in protocoll */
#define cSTX 02
#define cETX 03
#define cIdSend 'S'
#define cIdReceive 'K'
#define PRESSED 1

/* number of status cells */
#define PMSC 22

/* offset within data structure */
#define offsetHorizontal 22
#define offsetVertical   0
/* additional offset - use internal table */
#define offsetTable      512

/* debug output to /tmp/brltty.log */
/*
  #define WR_DEBUG
  #define RD_DEBUG
*/
