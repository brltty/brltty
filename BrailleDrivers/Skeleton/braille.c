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

#include "prologue.h"

#include <stdio.h>

#include "misc.h"

//#define BRL_STATUS_FIELDS sf...
//#define BRL_HAVE_STATUS_CELLS
//#define BRL_HAVE_PACKET_IO
//#define BRL_HAVE_KEY_CODES
//#define BRL_HAVE_FIRMNESS
//#define BRL_HAVE_SENSITIVITY
#include "brl_driver.h"

static TranslationTable outputTable;

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
  }
  
  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
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

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  return 1;
}

#ifdef BRL_HAVE_STATUS_CELLS
static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  return 1;
}
#endif /* BRL_HAVE_STATUS_CELLS */

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return EOF;
}

#ifdef BRL_HAVE_FIRMNESS
static void
brl_firmness (BrailleDisplay *brl, BrailleFirmness setting) {
}
#endif /* BRL_HAVE_FIRMNESS */

#ifdef BRL_HAVE_SENSITIVITY
static void
brl_sensitivity (BrailleDisplay *brl, BrailleSensitivity setting) {
}
#endif /* BRL_HAVE_SENSITIVITY */
