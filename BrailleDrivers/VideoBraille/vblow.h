/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

// vblow.h - Definition file for vblow.c: don't touch it!
#define KEY_UP 0x1
#define KEY_LEFT 0x2
#define KEY_RIGHT 0x4
#define KEY_DOWN 0x8
#define KEY_ATTRIBUTES 0x10
#define KEY_CURSOR 0x20
#define KEY_HOME 0x40
#define KEY_MENU 0x80

typedef struct {
  unsigned char bigbuttons;
  char routingkey : 7;
  char keypressed : 1;
} vbButtons;

int vbinit();
void vbtranslate(const unsigned char *,unsigned char *,int);
void BrButtons(vbButtons *);
void vbdisplay(unsigned char *);

