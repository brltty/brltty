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

/* CombiBraille/brlconf.h - Configurable definitions for the
 * Tieman B.V. CombiBraille driver
 * N. Nair, 25 January 1996
 * $Id: brlconf.h,v 1.2 1996/09/21 23:34:52 nn201 Exp $
 *
 * Edit as necessary for your system.
 */

#define BRLNAME	"CombiBraille"

#define BRLCOLS(id) 	\
	((id) == 0 ? 20 : \
	((id) == 1 ? 40 : \
	((id) == 2 ? 80 : \
	((id) == 7 ? 20 : \
	((id) == 8 ? 40 : \
	((id) == 9 ? 80 : \
	-1))))))
#define BRLROWS 1		/* number of rows on Braille display */
#define BAUDRATE B38400		/* baud rate for Braille display */

/* The following sequences are sent at initialisation time, at termination
 * and before and after Braille data.  The first byte is the length of the
 * sequence.
 *
 * Initialisation is treated specially, as there may not be a Braille
 * display connected.  This relies on a reply from the display.
 */
#define INIT_SEQ "\002\033?"	/* string to send to Braille to initialise */
#define INIT_ACK "\002\033?"	/* string to expect as acknowledgement */
#define ACK_TIMEOUT 5000	/* acknowledgement timeout in milliseconds */
#define MAX_ATTEMPTS 0		/* total tiimeout = timeout * attempts */
				/* try forever if MAX_ATTEMPTS = 0 */
#define CLOSE_SEQ ""		/* string to send to Braille to close */
#define PRE_DATA "\002\033B"	/* string to send to */
				/*  Braille before dat */
#define POST_DATA ""		/* string to send to Braille after data */

/* Define the preferred/default status cells mode. */
#define PREFSTYLE ST_TiemanStyle
