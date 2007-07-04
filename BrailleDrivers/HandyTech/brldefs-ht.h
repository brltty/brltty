/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2007 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file COPYING-API for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* brldefs-ht.h : Useful definitions to handle HandyTech dispalys */

#ifndef BRLTTY_INCLUDED_HT_BRLDEFS
#define BRLTTY_INCLUDED_HT_BRLDEFS

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
        unsigned char model;
      } PACKED ok;

      struct {
        unsigned char model;
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
