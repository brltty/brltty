/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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

#include "prologue.h"

#include <stdio.h>

#include "Programs/misc.h"

//#define BRL_HAVE_VISUAL_DISPLAY
//#define BRL_HAVE_PACKET_IO
//#define BRL_HAVE_KEY_CODES
//#define BRL_HAVE_FIRMNESS
#include "Programs/brl_driver.h"

static TranslationTable outputTable;

static void
brl_identify (void) {
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
  }
  
  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
}

#ifdef BRL_HAVE_PACKET_IO
static ssize_t
brl_readPacket (BrailleDisplay *brl, unsigned char *buffer, size_t size) {
  return -1;
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const unsigned char *packet, size_t length) {
  return -1;
}

static int
brl_reset (BrailleDisplay *brl) {
  return 0;
}
#endif /* BRL_HAVE_PACKET_IO */

#ifdef BRL_HAVE_KEY_CODES
static int
brl_readKey (BrailleDisplay *brl) {
  return EOF;
}

static int
brl_keyToCommand (BrailleDisplay *brl, BRL_DriverCommandContext context, int key) {
  return EOF;
}
#endif /* BRL_HAVE_KEY_CODES */

static void
brl_writeWindow (BrailleDisplay *brl) {
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
}

#ifdef BRL_HAVE_VISUAL_DISPLAY
static void
brl_writeVisual (BrailleDisplay *brl) {
}
#endif /* BRL_HAVE_VISUAL_DISPLAY */

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return EOF;
}

#ifdef BRL_HAVE_FIRMNESS
static void
brl_firmness (BrailleDisplay *brl, BrailleFirmness setting) {
}
#endif /* BRL_HAVE_FIRMNESS */
