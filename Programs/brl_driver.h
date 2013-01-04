/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_BRL_DRIVER
#define BRLTTY_INCLUDED_BRL_DRIVER

#include <stdio.h>

#include "brl.h"
#include "statdefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef BRLPARMS
static const char *const brl_parameters[] = {BRLPARMS, NULL};
#else /* BRLPARMS */
#define brl_parameters NULL
#endif /* BRLPARMS */

#ifdef BRL_STATUS_FIELDS
static const unsigned char brl_statusFields[] = {BRL_STATUS_FIELDS, sfEnd};
#else /* BRL_STATUS_FIELDS */
#define brl_statusFields NULL
#endif /* BRL_STATUS_FIELDS */

static int brl_construct (BrailleDisplay *brl, char **parameters, const char *device);
static void brl_destruct (BrailleDisplay *brl);

static int brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context);
static int brl_writeWindow (BrailleDisplay *brl, const wchar_t *characters);

#ifdef BRL_HAVE_STATUS_CELLS
static int brl_writeStatus (BrailleDisplay *brl, const unsigned char *cells);
#else /* BRL_HAVE_STATUS_CELLS */
#define brl_writeStatus NULL
#endif /* BRL_HAVE_STATUS_CELLS */

#ifdef BRL_HAVE_PACKET_IO
static ssize_t brl_readPacket (BrailleDisplay *brl, void *buffer, size_t size);
static ssize_t brl_writePacket (BrailleDisplay *brl, const void *buffer, size_t size);
static int brl_reset (BrailleDisplay *brl);
#else /* BRL_HAVE_PACKET_IO */
#define brl_readPacket NULL
#define brl_writePacket NULL
#define brl_reset NULL
#endif /* BRL_HAVE_PACKET_IO */

#ifdef BRL_HAVE_KEY_CODES
static int brl_readKey (BrailleDisplay *brl);
static int brl_keyToCommand (BrailleDisplay *brl, KeyTableCommandContext context, int key);
#else /* BRL_HAVE_KEY_CODES */
#define brl_readKey NULL
#define brl_keyToCommand NULL
#endif /* BRL_HAVE_KEY_CODES */

#ifndef BRLSYMBOL
#define BRLSYMBOL CONCATENATE(brl_driver_,DRIVER_CODE)
#endif /* BRLSYMBOL */

extern const BrailleDriver BRLSYMBOL;
const BrailleDriver BRLSYMBOL = {
  DRIVER_DEFINITION_INITIALIZER,

  brl_parameters,
  brl_statusFields,

  brl_construct,
  brl_destruct,

  brl_readCommand,
  brl_writeWindow,
  brl_writeStatus,

  brl_readPacket,
  brl_writePacket,
  brl_reset,

  brl_readKey,
  brl_keyToCommand
};

DRIVER_VERSION_DECLARATION(brl);

#define KEY_TABLE_LIST_REFERENCE const KeyTableDefinition *const *
#define KEY_TABLE_LIST_SYMBOL CONCATENATE(brl_ktb_,DRIVER_CODE)
#define KEY_TABLE_LIST_DECLARATION const KeyTableDefinition *const KEY_TABLE_LIST_SYMBOL[]
#define LAST_KEY_TABLE_DEFINITION NULL
#define BEGIN_KEY_TABLE_LIST \
  extern KEY_TABLE_LIST_DECLARATION; \
  KEY_TABLE_LIST_DECLARATION = {
#define END_KEY_TABLE_LIST LAST_KEY_TABLE_DEFINITION};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRL_DRIVER */
