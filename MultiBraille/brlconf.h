/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Nicolas Pitre <nico@cam.org>
 * Stéphane Doyon <s.doyon@videotron.ca>
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 */

/* MultiBraille/brlconf.h - Configurable definitions for the
 * following Tieman B.V. braille terminals
 * (infos out of a techn. product description sent to me from tieman by fax):
 *
 * - Brailleline 125 (no explicit description in tech. docs)
 * - Brailleline PICO II or MB145CR (45 braille modules + 1 dummy)
 * - Brailleline MB185CR (85 braille modules + 1 dummy)
 *
 * Wolfgang Astleitner, March/April 2000
 * Email: wolfgang.astleitner@liwest.at
 * brlconf.h,v 1.0
 *
 * Based on CombiBraille/brlconf.h by Nikhil Nair
 *
 * Edit as necessary for your system.
 */

#define BRLNAME	"MultiBraille"

#define BRLROWS 1					/* number of rows on Braille display */
#define BAUDRATE B38400		/* baud rate for Braille display */

/* The following sequences are sent at initialisation time, at termination
 * and before and after Braille data.  The first byte is the length of the
 * sequence.
 *
 * Initialisation is treated specially, as there may not be a Braille
 * display connected.  This relies on a reply from the display.
 */
#define ACK_TIMEOUT 5000	/* acknowledgement timeout in milliseconds */
#define MAX_ATTEMPTS 100		/* total tiimeout = timeout * attempts */
				/* try forever if MAX_ATTEMPTS = 0 */

#define MB_CR_EXTRAKEYS 6  /* amount of extra cursor routing keys. */
                           /* should always be 6 (-> tech. docs) */ 


/* Define the preferred/default status cells mode. */
#define PREFSTYLE ST_TiemanStyle
