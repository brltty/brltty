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

/* codes used in protocol */
#define cSTX 0X02
#define cETX 0X03
#define cIdSend 'S'
#define cIdIdentify 'I'
#define cIdReceive 'K'
#define PRESSED 1
#define IDENTITY_LENGTH 10

/* maximum number of status cells */
#define PMSC 22

/* offsets within input data structure */
#define RCV_KEYFUNC  0X0000 /* physical and logical function keys */
#define RCV_KEYROUTE 0X0300 /* routing keys */
#define RCV_SENSOR   0X0600 /* sensors or secondary routing keys */

/* offsets within output data structure */
#define XMT_BRLDATA  0X0000 /* data for braille display */
#define XMT_LCDDATA  0X0100 /* data for LCD */
#define XMT_BRLWRITE 0X0200 /* how to write each braille cell:
                             * 0 = convert data according to braille table (default)
                             * 1 = write directly
                             * 2 = mark end of braille display
                             */
#define XMT_BRLCELL  0X0300 /* description of eadch braille cell:
                             * 0 = has cursor routing key
                             * 1 = has cursor routing key and sensor
                             */
#define XMT_ASC2BRL  0X0400 /* ASCII to braille translation table */
#define XMT_LCDUSAGE 0X0500 /* source of LCD data:
                             * 0 = same as braille display
                             * 1 = not same as braille display
                             */
#define XMT_CSRPOSN  0X0501 /* cursor position (0 for no cursor) */
#define XMT_CSRDOTS  0X0502 /* cursor represenation in braille dots */
#define XMT_BRL2ASC  0X0503 /* braille to ASCII translation table */
#define XMT_LENFBSEQ 0X0603 /* length of feedback sequence for speech synthesizer */
#define XMT_LENKPSEQ 0X0604 /* length of keypad sequence */
#define XMT_TIMEK1K2 0X0605 /* key code suppression time for moving from K1 to K2 (left) */
#define XMT_TIMEK3K4 0X0606 /* key code suppression time for moving from K3 to K4 (up) */
#define XMT_TIMEK5K6 0X0607 /* key code suppression time for moving from K5 to K6 (right) */
#define XMT_TIMEK7K8 0X0608 /* key code suppression time for moving from K7 to K8 (down) */
#define XMT_TIMEROUT 0X0609 /* routing time interval */
#define XMT_TIMEOPPO 0X060A /* key code suppression time for opposite movements */

/* Define the preferred/default status cells mode. */
#define PREFSTYLE ST_Generic

#define PM_CONFIG_ENV   "BRLTTY_PM_CONF"
#define PM_CONFIG_FILE  HOME_DIRECTORY "/brltty-pm.conf"

#define MAXPATH  128
