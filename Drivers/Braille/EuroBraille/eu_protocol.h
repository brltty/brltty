/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/** EuroBraille/eu_protocol.h -- Protocol defines, structures and unions
 ** This file contains all definitions about the two protocols.
 ** It is also used to store protocol method headers and the main structure
 ** to access the terminal generically by the High-level part.
 **
 ** See braille.c for the high-level access, and io.h for the low-level access.
**/


#ifndef __EU_PROTOCOL_H__
#define __EU_PROTOCOL_H__

#include	"eu_io.h"
#include	"string.h"

#define SPEED 9600
#define CHARSPERSEC ( SPEED / 11 ) /* 1 start + 8 data + 1 parity + 1 stop */
static inline void updateWriteDelay(BrailleDisplay *brl, unsigned int chars)
{
  brl->writeDelay += 1000 * ( chars / CHARSPERSEC ) + 1;
}

/** Enum which define what protocol is used. */

typedef enum	u_eubrl_protocolType
{
  UNITIALIZED_PROTOCOL = 0,
    CLIO_PROTOCOL,	/* Old protocolunsil Iris version 1.71. */
    ESYSIRIS_PROTOCOL,	/* New Iris (>= 1.71) and ESYS protocol */
}		t_eubrl_protocolType;


/** Structure which will store generic methods for protocol handling */

typedef struct	s_eubrl_protocol
{
  int		(*init)(BrailleDisplay *brl, t_eubrl_io *io);
  int		(*reset)(BrailleDisplay *brl);
  unsigned int	(*readKey)(BrailleDisplay *brl);
  int		(*readCommand)(BrailleDisplay *brl, BRL_DriverCommandContext c);
  int		(*keyToCommand)(BrailleDisplay *brl, unsigned int key, BRL_DriverCommandContext ctx);
  void		(*writeWindow)(BrailleDisplay *brl);
  int		(*hasLcdSupport)(BrailleDisplay *brl);
  void		(*writeVisual)(BrailleDisplay *brl, const wchar_t *text);
  ssize_t	(*readPacket)(BrailleDisplay *brl, void *packet, size_t size);
  ssize_t	(*writePacket)(BrailleDisplay *brl, const void *packet, size_t size);
  
  t_eubrl_protocolType	protocolType;
}		t_eubrl_protocol;



/** Here are the corresponding headers for each protocol. */

/** Notebraille/Clio/Scriba/Iris <1.71 protocol driver */

int		clio_init(BrailleDisplay *brl, t_eubrl_io *io);
int		clio_reset(BrailleDisplay *brl);
unsigned int	clio_readKey(BrailleDisplay *brl);
int		clio_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext c);
int		clio_keyToCommand(BrailleDisplay *brl, unsigned int key, BRL_DriverCommandContext c);
void		clio_writeWindow(BrailleDisplay *brl);
int		clio_hasLcdSupport(BrailleDisplay *brl);
void		clio_writeVisual(BrailleDisplay *brl, const wchar_t *text);
ssize_t		clio_readPacket(BrailleDisplay *brl, void *packet, size_t size);
ssize_t		clio_writePacket(BrailleDisplay *brl, const void *packet, size_t size);

/** Esys/Iris >= 1.71  protocol headers */


int		esysiris_init(BrailleDisplay *brl, t_eubrl_io *io);
int		esysiris_reset(BrailleDisplay *brl);
unsigned int	esysiris_readKey(BrailleDisplay *brl);
int		esysiris_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext c);
int		esysiris_keyToCommand(BrailleDisplay *brl, unsigned int key, BRL_DriverCommandContext c);
void		esysiris_writeWindow(BrailleDisplay *brl);
int		esysiris_hasLcdSupport(BrailleDisplay *brl);
void		esysiris_writeVisual(BrailleDisplay *brl, const wchar_t *text);
ssize_t		esysiris_readPacket(BrailleDisplay *brl, void *packet, size_t size);
ssize_t		esysiris_writePacket(BrailleDisplay *brl, const void *packet, size_t size);

unsigned int		protocol_handleBrailleKey(unsigned int key);
#endif /* __EU_PROTOCOL_H__ */
