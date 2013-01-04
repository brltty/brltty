/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2013 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_HW_BRLDEFS
#define BRLTTY_INCLUDED_HW_BRLDEFS

typedef enum {
  HW_MSG_INIT                 = 0X00,
  HW_MSG_INIT_RESP            = 0X01,
  HW_MSG_DISPLAY              = 0X02,
  HW_MSG_GET_KEYS             = 0X03,
  HW_MSG_KEYS                 = 0X04,
  HW_MSG_KEY_DOWN             = 0X05,
  HW_MSG_KEY_UP               = 0X06,
  HW_MSG_FIRMWARE_UPDATE      = 0X07,
  HW_MSG_FIRMWARE_RESP        = 0X08,
  HW_MSG_CONFIGURATION_UPDATE = 0X09,
  HW_MSG_CONFIGURATION_RESP   = 0X0A,
  HW_MSG_GET_CONFIGURATION    = 0X0B
} HW_MessageType;

typedef union {
  unsigned char bytes[3 + 0XFF];

  struct {
    unsigned char header;
    unsigned char type;
    unsigned char length;

    union {
      unsigned char bytes[0XFF];

      struct {
        unsigned char communicationDisabled;
        unsigned char modelIdentifier;
        unsigned char cellCount;
      } PACKED init;

      struct {
        unsigned char id;
      } PACKED key;
    } data;
  } PACKED fields;
} HW_Packet;

typedef enum {
  HW_KEY_Power   =  1,

  HW_KEY_Dot1    =  2,
  HW_KEY_Dot2    =  3,
  HW_KEY_Dot3    =  4,
  HW_KEY_Dot4    =  5,
  HW_KEY_Dot5    =  6,
  HW_KEY_Dot6    =  7,
  HW_KEY_Dot7    =  8,
  HW_KEY_Dot8    =  9,
  HW_KEY_Space   = 10,

  HW_KEY_Nav1    = 11,
  HW_KEY_Nav2    = 12,
  HW_KEY_Nav3    = 13,
  HW_KEY_Nav4    = 14,
  HW_KEY_Nav5    = 15,
  HW_KEY_Nav6    = 16,

  HW_KEY_Thumb1  = 17,
  HW_KEY_Thumb2  = 18,
  HW_KEY_Thumb3  = 19,
  HW_KEY_Thumb4  = 20,

  HW_KEY_ROUTING = 80
} HW_NavigationKey;

typedef enum {
  HW_SET_NavigationKeys = 0,
  HW_SET_RoutingKeys
} HW_KeySet;

#endif /* BRLTTY_INCLUDED_HW_BRLDEFS */ 
