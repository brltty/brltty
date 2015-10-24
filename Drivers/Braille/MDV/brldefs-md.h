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
} MD_Packet;

typedef enum {
  MD_PT_WRITE_ALL       =   0,
  MD_PT_WRITE_STATUS    =   1,
  MD_PT_WRITE_TEXT      =   2,
  MD_PT_WRITE_LCD       =   5,
  MD_PT_NAVIGATION_KEY  =  16,
  MD_PT_ROUTING_PRESS   =  17,
  MD_PT_ROUTING_RELEASE =  18,
  MD_PT_BRAILLE_KEY     =  21,
  MD_PT_IDENTIFY        =  36,
  MD_PT_IDENTITY        =  37,
  MD_PT_ACKNOWLEDGE     = 127,
} MD_PacketType;

typedef enum {
  MD_NAV_F1            = 0X01,
  MD_NAV_F2            = 0X02,
  MD_NAV_F3            = 0X03,
  MD_NAV_F4            = 0X04,
  MD_NAV_F5            = 0X05,
  MD_NAV_F6            = 0X06,
  MD_NAV_F7            = 0X07,
  MD_NAV_F8            = 0X08,
  MD_NAV_F9            = 0X09,
  MD_NAV_F10           = 0X0A,
  MD_NAV_LEFT          = 0X0B,
  MD_NAV_UP            = 0X0C,
  MD_NAV_RIGHT         = 0X0D,
  MD_NAV_DOWN          = 0X0E,
  MD_NAV_MASK_KEY      = 0X0F,

  MD_NAV_MOD_SHIFT     = 0X10,
  MD_NAV_MOD_LONG      = 0X20,
  MD_NAV_MASK_MOD      = 0X30,

  MD_NAV_SHIFT_PRESS   = 0X3F,
  MD_NAV_SHIFT_RELEASE = 0X40,
} MD_NavigationKey;

typedef enum {
  MD_ROUTING_FIRST = 0X01,
  MD_ROUTING_MASK  = 0X7F,
  MD_ROUTING_SHIFT = 0X80,
} MD_RoutingKey;

typedef enum {
  MD_BRL_DOT1 = 0X01,
  MD_BRL_DOT2 = 0X02,
  MD_BRL_DOT3 = 0X04,
  MD_BRL_DOT4 = 0X08,
  MD_BRL_DOT5 = 0X10,
  MD_BRL_DOT6 = 0X20,
  MD_BRL_DOT7 = 0X40,
  MD_BRL_DOT8 = 0X80,
} MD_BrailleKey;

typedef enum {
  MD_KEY_F1,
  MD_KEY_F2,
  MD_KEY_F3,
  MD_KEY_F4,
  MD_KEY_F5,
  MD_KEY_F6,
  MD_KEY_F7,
  MD_KEY_F8,
  MD_KEY_F9,
  MD_KEY_F10,

  MD_KEY_LEFT,
  MD_KEY_UP,
  MD_KEY_RIGHT,
  MD_KEY_DOWN,

  MD_KEY_SHIFT,
  MD_KEY_LONG,

  MD_KEY_DOT1,
  MD_KEY_DOT2,
  MD_KEY_DOT3,
  MD_KEY_DOT4,
  MD_KEY_DOT5,
  MD_KEY_DOT6,
  MD_KEY_DOT7,
  MD_KEY_DOT8,

  MD_KEY_SPACE,
} MD_KeyNumber;

typedef enum {
  MD_GRP_NavigationKeys,
  MD_GRP_RoutingKeys,
  MD_GRP_StatusKeys,
} MD_KeyGroup;

#endif /* BRLTTY_INCLUDED_MD_BRLDEFS */ 
