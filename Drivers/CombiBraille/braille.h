/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

/* CombiBraille/braille.h - Configurable definitions for the
 * Tieman B.V. CombiBraille driver
 * N. Nair, 25 January 1996
 * $Id: brlconf.h,v 1.2 1996/09/21 23:34:52 nn201 Exp $
 *
 * Edit as necessary for your system.
 */

/* used by speech.c */
#include "Programs/serial.h"
extern SerialDevice *CB_serialDevice;
extern int CB_charactersPerSecond;			/* file descriptor for Braille display */

#define BRLCOLS(id) 	\
	((id) == 0 ? 20 : \
	((id) == 1 ? 40 : \
	((id) == 2 ? 80 : \
	((id) == 7 ? 20 : \
	((id) == 8 ? 40 : \
	((id) == 9 ? 80 : \
	-1))))))
#define BRLROWS 1		/* number of rows on Braille display */
#define BAUDRATE 38400		/* baud rate for Braille display */

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
