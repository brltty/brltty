/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifndef _BRLAPI_H
#define _BRLAPI_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Instructions and constants for BrlApi's protocol */

#include <inttypes.h>
#include <unistd.h> /* The type size_t is defined there! */

/* Type of the structure you have to pass to brlapi_initializeConnection */
/* Usually, you can just pass NULL */
typedef struct
{
  char *authKey;
  char *hostName; /* Host and port where the server is listening */
} brlapi_settings_t;

typedef struct
{
  const char *client;
} brlapi_keybinding_t;

/* Default port of socket that allows to establish connections with BrlApi */
#define BRLAPI_SOCKETPORT "35751"

/* Name of file containing BrlApi's authentication key */
/* This name is relative to HOME_DIR */
#define BRLAPI_AUTHFILE "brlapi-key"
#define BRLAPI_ETCDIR "/etc/brltty"
#define BRLAPI_HOMEKEYDIR ".brlkeys"
#define BRLAPI_HOMEKEYEXT ".kbd"
#define BRLAPI_ETCKEYFILE "brlkeys"
#define BRLAPI_AUTHNAME BRLAPI_ETCDIR "/" BRLAPI_AUTHFILE

/* Maximum packet size for packets exchanged on sockets and with braille terminal */
#define BRLAPI_MAXPACKETSIZE 512

/* Packet types */

/* type : only unsigned can cross networks, 32bits */
typedef uint32_t brl_type_t;

/* key codes, 32 bits */
typedef uint32_t brl_keycode_t;
/* Bigest value */
#define BRL_KEYCODE_MAX ((brl_keycode_t) (UINT32_MAX))

#define BRLPACKET_BYE               'B'    /* Bye                         */
#define BRLPACKET_GETTTY            't'    /* asks for a specified tty    */
#define BRLPACKET_LEAVETTY          'L'    /* release the tty             */
#define BRLPACKET_KEY               'k'    /* braille key                 */
#define BRLPACKET_COMMAND           'c'    /* braille command             */
#define BRLPACKET_MASKKEYS          'm'    /* Mask a key-range            */
#define BRLPACKET_UNMASKKEYS        'u'    /* Unmask key range            */
#define BRLPACKET_WRITE             'W'    /* Write On Braille Display    */
#define BRLPACKET_STATWRITE         'S'    /* Write Status Cells          */
#define BRLPACKET_GETRAW            '*'    /* Enter in raw mode           */
#define BRLPACKET_LEAVERAW          '#'    /* Leave raw mode              */
#define BRLPACKET_PACKET            'p'    /* Raw packets                 */
#define BRLPACKET_ACK               'A'    /* Acknowledgement             */
#define BRLPACKET_ERROR             'E'    /* Error in protocol           */
#define BRLPACKET_GETDRIVERID       'd'    /* Ask which driver is used    */
#define BRLPACKET_GETDRIVERNAME     'n'    /*                             */
#define BRLPACKET_GETDISPLAYSIZE    's'    /* Dimensions of brl display   */
#define BRLPACKET_AUTHKEY           'K'    /* Authentication key          */

/* Arguments to GETTTY */
/* When taking control of a tty, one must specify under which form */
/* We want braille keys are delivered to us */
/* Two forms are possible: KEYCODES and COMMANDS */
/* KEYCODES are specific to each braille driver and correspond to a key press */
/* Using them leads to build highly driver-dependent applications, what */
/* can be very useful */
/* COMMANDS means that applications will get exactly the same value than */
/* brltty. This allows to build driver-indepent clients, which should be */
/* pleasent to use with a lot of different terminals */
#define BRLKEYCODES ((uint32_t) 1)
#define BRLCOMMANDS ((uint32_t) 2)

/* Error codes */ 
#define BRLERR_NOMEM                    1  /* Not enough memory */
#define BRLERR_TTYBUSY                  2  /* Already a connection running in this tty */
#define BRLERR_UNKNOWN_INSTRUCTION      3  /* Not implemented in protocol */
#define BRLERR_ILLEGAL_INSTRUCTION      4  /* Forbiden in current mode */
#define BRLERR_INVALID_PARAMETER        5  /* Out of range or have no sense */
#define BRLERR_INVALID_PACKET           6  /* Invalid size */
#define BRLERR_RAWNOTSUPP               7  /* Raw mode not supported by loaded driver */
#define BRLERR_KEYSNOTSUPP              8  /* Reading of key codes not supported by loaded driver */
#define BRLERR_CONNREFUSED              9  /* Connection refused */
#define BRLERR_OPNOTSUPP                10  /* Operation not supported */

/* Other constants */
#define BRLRAW_MAGIC (0xdeadbeefL) 

/* brl_InitializeConnection */
/* Creates a socket to connect to BrlApi */
/* Returns the file descriptor, or -1 if an error occured */
/* Note: We return the file descriptor in case the client wants to */
/* communicate with the server without using our routines, but, if he uses us, */
/* He will not have to pass the fd later, since we keep a copy of it */
int brlapi_initializeConnection(const brlapi_settings_t *, brlapi_settings_t *);

/* brlapi_closeConnection */
/* Cleanly close the socket */
void brlapi_closeConnection();

/* brlapi_writePacket */
int brlapi_writePacket(int fd, size_t size, brl_type_t type, const unsigned char *buf);

/* brlapi_readPacket */
/* Returns packet's size, -2 if EOF, -1 on error */
int brlapi_readPacket(int fd, size_t size, brl_type_t *type, unsigned char *buf);

/* brlapi_getRaw */
/* Switch to Raw mode */
/* Returns 1 if successfull, 0 if unavailable for the moment, -1 on error */
int brlapi_getRaw();

/* brlapi_leaveRaw */
/* Leave Raw mode */
/* Return -1 on error, 0 else */
int brlapi_leaveRaw();

/* brlapi_sendRaw */
/* Send a Raw Packet */
/* Return -1 on error, 0 else */
int brlapi_sendRaw(const unsigned char *buf, size_t size);

/* brlapi_recvRaw */
/* Get a Raw packet */
/* Returns its size, -1 on error, or -2 on EOF */
int brlapi_recvRaw(unsigned char *buf, size_t size);

/* brlapi_getDriverId */
/* Identify the driver BrlApi loaded */
char *brlapi_getDriverId();

/* brlapi_getDriverName */
/* Name of the driver BrlApi loaded */
char *brlapi_getDriverName();

/* brlapi_getDisplaySize */
/* Returns the size of the braille display */
int brlapi_getDisplaySize(int *x, int *y);

/* brlapi_loadAuthKey */
/* Loads an authentication key from the given file. The key is stored in */
/* auth, and its size is returned via authlength */
int brlapi_loadAuthKey(const char *filename, int *authlength, void *auth);

/* brlapi_getTty */
/* Takes control of a tty, for direct braille display / read */
/* If needed, loads a user's key bindings which let use ReadKeyCmd */
int brlapi_getTty(uint32_t tty, uint32_t how, brlapi_keybinding_t *keybinding);

/* brlapi_leaveTty */
/* Stops to control the tty */
int brlapi_leaveTty();

/* brlapi_writeBrl */
/* Writes to the braille display */
/* Be carefull, you must call GetTty first ! */
int brlapi_writeBrl(uint32_t cursor, char *str);

/* brlapi_readKey */
/* Reads a key from the braille keyboard */
/* Call GetTty first */
int brlapi_readKey(brl_keycode_t *code);

/* brlapi_readCommand */
/* Reads a command from the braille keyboard */
/* If the read is successfull, the command is put in code, and 0 is returned */
/* If an error occurs, we return -1 and *code is undefined */
int brlapi_readCommand(brl_keycode_t *code);

/* brlapi_readKeyCmd */
/* Reads a key from the braille keyboard, but return a command name, as bound */
/* in user's config file */
/* Call GetTty first */
const char *brlapi_readBinding(void);

/* brlapi_ignoreKeys */
/* Asks the server to return keys between x and y to brltty, rather than */
/* passing them to us on the socket */
int brlapi_ignoreKeys(brl_keycode_t x, brl_keycode_t y);

/* brlapi_unignoreKeys */
/* Says the server we are interested by keys between x and y */
int brlapi_unignoreKeys(brl_keycode_t x, brl_keycode_t y);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BRLAPI_H */
