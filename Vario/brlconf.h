/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

/* Vario/brlconf.h - Configuration file for the BAUM Vario 40/80 braille
 *                   display driver (brl.c)
 * Written by Mario Lang (mlang@delysid.org)
 */

/* Configuration file for the Vario driver.
 * Edit as needed...
 */

#define BRLNAME	"Vario"
#define PREFSTYLE ST_TiemanStyle

/* A query is sent if we don't get any keys in a certain time, to detect
   if the display was turned off.
   This is problematic at current Firmware versions, because the
   Vario resets the internal sleep timeout value to 0 everytime
   serial communication takes place.
   This will probably change in future Firmware versions.
   So, if you often connect/disconnect Varios from the serial port,
   uncomment this, if you dont really do that and want the Vario
   to sleep (perhaps if you have a Vario 80 which can even be turn off),
   leave it unchanged */
// #define USE_PING 1
/* ASAIK that the programmers at Baum Germany really changed the
   firmware inconjunction with Infotype $84 (Device ID), the standard setting
   will get to use pinging. Pinging is a nice feature. Sad to have
   to disable it by default. */

/* How soon do we get nervous and send the ping? (in miliseconds) */
#define PING_INTRVL 2000
/* The maximum time needed to realize that the display was turned off is
   approx PING_INTRVL +500 ms. */

#define REPEAT_TIME 170  /* Theoretically, repeating 4 times per second */

#define BUTTON_STEP 10
