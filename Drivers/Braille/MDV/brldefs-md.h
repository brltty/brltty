/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_MD_BRLDEFS
#define BRLTTY_INCLUDED_MD_BRLDEFS

typedef union {
  unsigned char bytes[1];

  struct {
    unsigned char soh;
    unsigned char stx;
    unsigned char code;
    unsigned char length;
    unsigned char etx;

    union {
      unsigned char bytes[0XFF];

      struct {
        unsigned char key;
      } navigationKey;

      struct {
        unsigned char key;
      } routingPress;

      struct {
        unsigned char key;
      } routingRelease;

      struct {
        unsigned char isChord;
        unsigned char dots;
        unsigned char ascii;
      } brailleKey;

      struct {
        unsigned char textCellCount;
        unsigned char statusCellCount;
        unsigned char dotsPerCell;
        unsigned char haveRoutingKeys;
        unsigned char majorVersion;
        unsigned char MinorVersion;
      } identity;
    } data;
  } PACKED fields;
} MDV_Packet;

typedef enum {
  MDV_PT_WRITE_ALL       =   0,
  MDV_PT_WRITE_STATUS    =   1,
  MDV_PT_WRITE_TEXT      =   2,
  MDV_PT_WRITE_LCD       =   5,
  MDV_PT_NAVIGATION_KEY  =  16,
  MDV_PT_ROUTING_PRESS   =  17,
  MDV_PT_ROUTING_RELEASE =  18,
  MDV_PT_BRAILLE_KEY     =  21,
  MDV_PT_IDENTIFY        =  36,
  MDV_PT_IDENTITY        =  37,
  MDV_PT_ACKNOWLEDGE     = 127,
} MDV_PacketType;

typedef enum {
  MDV_NAV_F1            = 0X01,
  MDV_NAV_F2            = 0X02,
  MDV_NAV_F3            = 0X03,
  MDV_NAV_F4            = 0X04,
  MDV_NAV_F5            = 0X05,
  MDV_NAV_F6            = 0X06,
  MDV_NAV_F7            = 0X07,
  MDV_NAV_F8            = 0X08,
  MDV_NAV_F9            = 0X09,
  MDV_NAV_F10           = 0X0A,
  MDV_NAV_LEFT          = 0X0B,
  MDV_NAV_UP            = 0X0C,
  MDV_NAV_RIGHT         = 0X0D,
  MDV_NAV_DOWN          = 0X0E,
  MDV_NAV_MASK_KEY      = 0X0F,

  MDV_NAV_MOD_SHIFT     = 0X10,
  MDV_NAV_MOD_LONG      = 0X20,
  MDV_NAV_MASK_MOD      = 0X30,

  MDV_NAV_SHIFT_PRESS   = 0X3F,
  MDV_NAV_SHIFT_RELEASE = 0X40,
} MDV_NavigationKey;

typedef enum {
  MDV_ROUTING_FIRST = 0X01,
  MDV_ROUTING_MASK  = 0X7F,
  MDV_ROUTING_SHIFT = 0X80,
} MDV_RoutingKey;

typedef enum {
  MDV_BRL_DOT1 = 0X01,
  MDV_BRL_DOT2 = 0X02,
  MDV_BRL_DOT3 = 0X04,
  MDV_BRL_DOT4 = 0X08,
  MDV_BRL_DOT5 = 0X10,
  MDV_BRL_DOT6 = 0X20,
  MDV_BRL_DOT7 = 0X40,
  MDV_BRL_DOT8 = 0X80,
} MDV_BrailleKey;

#endif /* BRLTTY_INCLUDED_MD_BRLDEFS */ 
