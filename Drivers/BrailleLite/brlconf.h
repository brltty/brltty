/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

/* BrailleLite/brlconf.h - Configurable definitions for the Braille Lite driver
 * N. Nair, 6 September 1998
 *
 * Edit as necessary for your system.
 */


#define BRLNAME "BrailleLite"

/* We always expect 8 data bits, no parity, 1 stop bit. */
/* Select baudrate to use */
#define BAUDRATE B9600
//#define BAUDRATE B38400

/* Define the preferred/default status cells mode. */
#define PREFSTYLE ST_None

/* Define this if you want to keep trying to detect a display forever,
   instead of exiting when no display appears to be connected. */
#define DETECT_FOREVER

/* Define the following for dots to character mapping for input to use 
   the same (user-defined) table as is used for output, instead of the
   hard-coded US table. */
#define USE_TEXTTRANS
