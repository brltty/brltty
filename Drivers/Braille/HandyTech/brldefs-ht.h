/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2010 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
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

#ifndef BRLTTY_INCLUDED_HT_BRLDEFS
#define BRLTTY_INCLUDED_HT_BRLDEFS

#define HT_USB_VENDOR 0X1FE4

typedef enum {
  HT_MODEL_UsbHidAdapter       = 0X03,
  HT_MODEL_BrailleWave         = 0X05,
  HT_MODEL_ModularEvolution64  = 0X36,
  HT_MODEL_ModularEvolution88  = 0X38,
  HT_MODEL_EasyBraille         = 0X44,
  HT_MODEL_Braillino           = 0X72,
  HT_MODEL_BrailleStar40       = 0X74,
  HT_MODEL_BrailleStar80       = 0X78,
  HT_MODEL_Modular20           = 0X80,
  HT_MODEL_Modular80           = 0X88,
  HT_MODEL_Modular40           = 0X89,
  HT_MODEL_Bookworm            = 0X90
} HT_ModelIdentifier;

/* Packet definition */
typedef enum {
  HT_PKT_Extended = 0X79,
  HT_PKT_NAK      = 0X7D,
  HT_PKT_ACK      = 0X7E,
  HT_PKT_OK       = 0XFE,
  HT_PKT_Reset    = 0XFF
} HT_PacketType;

typedef enum {
  HT_EXTPKT_Braille           = 0X01,
  HT_EXTPKT_Key               = 0X04,
  HT_EXTPKT_Confirmation      = 0X07,
  HT_EXTPKT_Scancode          = 0X09,
  HT_EXTPKT_SetAtcMode        = 0X50,
  HT_EXTPKT_SetAtcSensitivity = 0X51,
  HT_EXTPKT_AtcInfo           = 0X52
} HT_ExtendedPacketType;

typedef union {
  unsigned char bytes[4 + 0XFF];

  struct {
    HT_PacketType type:8;

    union {
      struct {
        HT_ModelIdentifier model:8;
      } PACKED ok;

      struct {
        HT_ModelIdentifier model:8;
        unsigned char length;
        HT_ExtendedPacketType type:8;

        union {
          unsigned char bytes[0XFF];
        } data;
      } PACKED extended;
    } data;
  } PACKED fields;
} HT_Packet;

typedef enum {
  HT_KEY_None = 0,

  HT_KEY_B1 = 0X03,
  HT_KEY_B2 = 0X07,
  HT_KEY_B3 = 0X0B,
  HT_KEY_B4 = 0X0F,

  HT_KEY_B5 = 0X13,
  HT_KEY_B6 = 0X17,
  HT_KEY_B7 = 0X1B,
  HT_KEY_B8 = 0X1F,

  HT_KEY_Up = 0X04,
  HT_KEY_Down = 0X08,

  /* Keypad keys (star80 and modular) */
  HT_KEY_B12 = 0X01,
  HT_KEY_Zero = 0X05,
  HT_KEY_B13 = 0X09,
  HT_KEY_B14 = 0X0D,

  HT_KEY_B11 = 0X11,
  HT_KEY_One = 0X15,
  HT_KEY_Two = 0X19,
  HT_KEY_Three = 0X1D,

  HT_KEY_B10 = 0X02,
  HT_KEY_Four = 0X06,
  HT_KEY_Five = 0X0A,
  HT_KEY_Six = 0X0E,

  HT_KEY_B9 = 0X12,
  HT_KEY_Seven = 0X16,
  HT_KEY_Eight = 0X1A,
  HT_KEY_Nine = 0X1E,

  /* Braille wave/star keys */
  HT_KEY_Escape = 0X0C,
  HT_KEY_Space = 0X10,
  HT_KEY_Return = 0X14,

  /* Braille star keys */
  HT_KEY_SpaceRight = 0X18,

  /* ranges and flags */
  HT_KEY_ROUTING = 0X20,
  HT_KEY_STATUS = 0X70,
  HT_KEY_RELEASE = 0X80
} HT_NavigationKey;

typedef enum {
  HT_SET_NavigationKeys = 0,
  HT_SET_RoutingKeys
} HT_KeySet;

#endif /* BRLTTY_INCLUDED_HT_BRLDEFS */ 
