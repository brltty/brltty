/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

/** type for packet type. Only unsigned can cross networks, 32bits */
typedef uint32_t brl_type_t;

#define BRLAPI_PROTOCOL_VERSION ((uint32_t) 3) /** Communication protocol version */

#define BRLPACKET_AUTHKEY           'K'    /**< Authentication key          */
#define BRLPACKET_BYE               'B'    /**< Bye                         */
#define BRLPACKET_GETDRIVERID       'd'    /**< Ask which driver is used    */
#define BRLPACKET_GETDRIVERNAME     'n'    /**< Ask which driver is used    */
#define BRLPACKET_GETDISPLAYSIZE    's'    /**< Dimensions of brl display   */
#define BRLPACKET_GETTTY            't'    /**< Asks for a specified tty    */
#define BRLPACKET_LEAVETTY          'L'    /**< Release the tty             */
#define BRLPACKET_KEY               'k'    /**< Braille key                 */
#define BRLPACKET_COMMAND           'c'    /**< Braille command             */
#define BRLPACKET_IGNOREKEYRANGE    'm'    /**< Mask key-range              */
#define BRLPACKET_IGNOREKEYSET      'M'    /**< Mask key-set                */
#define BRLPACKET_UNIGNOREKEYRANGE  'u'    /**< Unmask key range            */
#define BRLPACKET_UNIGNOREKEYSET    'U'    /**< Unmask key set              */
#define BRLPACKET_WRITEDOTS         'D'    /**< Write Dots On Braille Display */
#define BRLPACKET_STATWRITE         'S'    /**< Write Status Cells          */
#define BRLPACKET_EXTWRITE          'e'    /**< Extended Write              */
#define BRLPACKET_GETRAW            '*'    /**< Enter in raw mode           */
#define BRLPACKET_LEAVERAW          '#'    /**< Leave raw mode              */
#define BRLPACKET_PACKET            'p'    /**< Raw packets                 */
#define BRLPACKET_ACK               'A'    /**< Acknowledgement             */
#define BRLPACKET_ERROR             'E'    /**< Error in protocol           */

/** Magic number to give when sending a BRLPACKET_GETRAW packet */
#define BRLRAW_MAGIC (0xdeadbeefL)

typedef struct {
  uint32_t protocolVersion;
  unsigned char key;
} authStruct;

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
