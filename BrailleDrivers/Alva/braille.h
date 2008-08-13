/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* Alva/braille.h - Configurable definitions for the Alva driver
 * Copyright (C) 1995-1998 by Nicolas Pitre <nico@cam.org>
 *
 */

/* used by speech.c */
extern int AL_writeData( unsigned char *data, int len );


/* Define next line to 1 if you have a firmware older than version 010495 */
#define ABT3_OLD_FIRMWARE 0


#define BAUDRATE 9600


/* Delay in miliseconds between forced full refresh of the display.
 * This is to minimize garbage effects due to noise on the serial line.
 */
#define REWRITE_INTERVAL 1000
