/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2004 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 * All rights reserved.
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file LGPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/** \file
 * \brief types and constants for \e BrlAPI's protocol
 */

#ifndef _BRLAPI_PROTOCOL_H
#define _BRLAPI_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* this is for UINT32_MAX */
#include <inttypes.h>
#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif /* UINT32_MAX */

/* The type size_t is defined there! */
#include <unistd.h>

/** \defgroup brlapi_protocol BrlAPI's protocol
 * \brief Instructions and constants for \e BrlAPI 's protocol
 *
 * These are defines for the protocol between \e BrlAPI 's server and clients.
 * Understanding is not needed to use the \e BrlAPI library, so reading this
 * is not needed unless really wanting to connect to \e BrlAPI without
 * \e BrlAPI 's library.
 *
 * @{ */

#define BRLAPI_PROTOCOL_VERSION ((uint32_t) 6) /** Communication protocol version */

#define BRLPACKET_AUTHKEY           'K'    /**< Authentication key          */
#define BRLPACKET_GETDRIVERID       'd'    /**< Ask which driver is used    */
#define BRLPACKET_GETDRIVERNAME     'n'    /**< Ask which driver is used    */
#define BRLPACKET_GETDISPLAYSIZE    's'    /**< Dimensions of brl display   */
#define BRLPACKET_GETTTY            't'    /**< Asks for a specified tty    */
#define BRLPACKET_SETFOCUS          'F'    /**< Set current tty focus       */
#define BRLPACKET_LEAVETTY          'L'    /**< Release the tty             */
#define BRLPACKET_KEY               'k'    /**< Braille key                 */
#define BRLPACKET_IGNOREKEYRANGE    'm'    /**< Mask key-range              */
#define BRLPACKET_IGNOREKEYSET      'M'    /**< Mask key-set                */
#define BRLPACKET_UNIGNOREKEYRANGE  'u'    /**< Unmask key range            */
#define BRLPACKET_UNIGNOREKEYSET    'U'    /**< Unmask key set              */
#define BRLPACKET_WRITE             'w'    /**< Write                       */
#define BRLPACKET_GETRAW            '*'    /**< Enter in raw mode           */
#define BRLPACKET_LEAVERAW          '#'    /**< Leave raw mode              */
#define BRLPACKET_PACKET            'p'    /**< Raw packets                 */
#define BRLPACKET_ACK               'A'    /**< Acknowledgement             */
#define BRLPACKET_ERROR             'e'    /**< non-fatal error             */
#define BRLPACKET_EXCEPTION         'E'    /**< Exception                   */

/** Magic number to give when sending a BRLPACKET_GETRAW packet */
#define BRLRAW_MAGIC (0xdeadbeefL)

/** Structure of authentication packets */
typedef struct {
  uint32_t protocolVersion;
  unsigned char key;
} authStruct;

/** Structure of error packets */
typedef struct {
  uint32_t code;
  brl_type_t type;
  unsigned char packet;
} errorPacket_t;

/** Flags for writing */
#define BRLAPI_WF_DISPLAYNUMBER 0X01    /**< Display number                 */
#define BRLAPI_WF_REGION        0X02    /**< Region parameter               */
#define BRLAPI_WF_TEXT          0X04    /**< Contains some text             */
#define BRLAPI_WF_ATTR_AND      0X08    /**< And attributes                 */
#define BRLAPI_WF_ATTR_OR       0X10    /**< Or attributes                  */
#define BRLAPI_WF_CURSOR        0X20    /**< Cursor position                */

/** Structure of extended write packets */
typedef struct {
  uint32_t flags; /** Flags to tell which fields are present */
  unsigned char data; /** Fields in the same order as flag weight */
} writeStruct;

/* brlapi_writePacket */
/** Send a packet to \e BrlAPI server
 *
 * This function is for internal use, but one might use it if one really knows
 * what one is doing...
 *
 * \e type should only be one of the above defined BRLPACKET_*.
 *
 * The syntax is the same as write()'s.
 *
 * \return 0 on success, -1 on failure
 *
 * \sa brlapi_readPacket()
 */
ssize_t brlapi_writePacket(int fd, brl_type_t type, const void *buf, size_t size);

/* brlapi_readPacket */
/** Read a packet from \e BrlAPI server
 *
 * This function is for internal use, but one might use it if one really knows
 * what one is doing...
 *
 * \e type is where the function will store the packet type; it should always
 * be one of the above defined BRLPACKET_* (or else something very nasty must
 * have happened :/).
 *
 * The syntax is the same as read()'s.
 *
 * \return packet's size, -2 if \c EOF occurred, -1 on error
 *
 * \sa brlapi_writePacket()
 */
ssize_t brlapi_readPacket(int fd, brl_type_t *type, void *buf, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BRLAPI_PROTOCOL_H */
