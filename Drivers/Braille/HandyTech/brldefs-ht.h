/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2008 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
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

/* brldefs-ht.h : Useful definitions to handle HandyTech displays */

#ifndef BRLTTY_INCLUDED_HT_BRLDEFS
#define BRLTTY_INCLUDED_HT_BRLDEFS

typedef enum {
  HT_Model_BrailleWave        = 0X05,
  HT_Model_ModularEvolution64 = 0X36,
  HT_Model_ModularEvolution88 = 0X38,
  HT_Model_EasyBraille        = 0X44,
  HT_Model_Braillino          = 0X72,
  HT_Model_BrailleStar40      = 0X74,
  HT_Model_BrailleStar80      = 0X78,
  HT_Model_Modular20          = 0X80,
  HT_Model_Modular80          = 0X88,
  HT_Model_Modular40          = 0X89,
  HT_Model_Bookworm           = 0X90
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

#endif /* BRLTTY_INCLUDED_HT_BRLDEFS */ 
