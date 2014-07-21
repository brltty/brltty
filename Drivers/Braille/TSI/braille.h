/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

/* TSI/braille.h - Configuration file for the TSI braille
 *                 display driver (brl.c)
 * Written by Stéphane Doyon (s.doyon@videotron.ca)
 *
 * This file is intended for version 2.2beta3 of the driver.
 */

/* Configuration file for the TSI driver.
 * Edit as needed...
 */

/* A query is sent if we don't get any keys in a certain time, to detect
   if the display was turned off. How soon do we get nervous and send
   the ping? (in miliseconds) */
#define PING_INTRVL 2000
/* The maximum time needed to realize that the display was turned off is
   approx PING_INTRVL +500 ms. */

/* Timing options: If you get a garbled display, you might try to uncomment
   one of the following defines. This attempts to avoid flow control problems
   by inserting various small delays. This is normally not needed and it will
   make the display slightly sluggish. However these options may be helpful
   in getting some TSI emulators to work correctly.

   For the TSI emulation mode of the mdv mb408s display, the following
   options are recommended:
     uncomment FORCE_FULL_SEND_DELAY
     leave FORCE_DRAIN_AFTER_SEND commented out
     leave SEND_DELAY set to 30
     NO_MULTIPLE_UPDATES will be set implicitly.
*/
/* This option forces BRLTTY to wait for the OS's buffer for the serial port
   to be flushed after each update to the braille display. (Note that we do
   not wait for the buffer on the UART chip to be flushed, which might take
   something like 30-60ms.) */
/*#define FORCE_DRAIN_AFTER_SEND 1*/
/* This option, in addition to waiting like the previous option, also imposes
   a wait of SEND_DELAY ms after sending a display update. It also imposes a
   wait of 2*SEND_DELAY ms before and SEND_DELAY after sending a ping. */
/*#define FORCE_FULL_SEND_DELAY 1*/
/* Note that the previous options are automatically triggered when some
   display models are recognized: Nav40 triggers FORCE_DRAIN_AFTER_SEND (is it
   necessary?), PB65/80 and Nav80 trigger FORCE_FULL_SEND_DELAY (and should be
   fine with SEND_DELAY set to 30). Only PB40 (and presumably Nav20?) are fine
   without timing adjustment. Also remember that there is a 40ms delay for
   every cycle of the main BRLTTY loop. */
#define SEND_DELAY 30
/* When a change of the display content is detected and an update is
   performed, it may be more efficient (in terms of total number of bytes
   sent) to update the display in several steps by updating only the portions
   that have changed. This option forces BRLTTY to always send only one
   "packet" per display update. It is implicitly activated whenever
   FORCE_FULL_SEND_DELAY is used. */
/*#define NO_MULTIPLE_UPDATES 1*/

/* TODO: an option that deactivates partial updates, so that the whole display
   is always updated... */

/* End of driver config */
