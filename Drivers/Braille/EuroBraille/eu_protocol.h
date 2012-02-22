/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

#include "eu_braille.h"

typedef struct {
  const char *name;

  int (*init) (BrailleDisplay *brl);
  int (*resetDevice) (BrailleDisplay *brl);

  ssize_t (*readPacket) (BrailleDisplay *brl, void *packet, size_t size);
  ssize_t (*writePacket) (BrailleDisplay *brl, const void *packet, size_t size);

  unsigned int (*readKey) (BrailleDisplay *brl);
  int (*readCommand) (BrailleDisplay *brl, KeyTableCommandContext c);
  int (*keyToCommand) (BrailleDisplay *brl, unsigned int key, KeyTableCommandContext ctx);

  void (*writeWindow) (BrailleDisplay *brl);
  int (*hasLcdSupport) (BrailleDisplay *brl);
  void (*writeVisual) (BrailleDisplay *brl, const wchar_t *text);
} t_eubrl_protocol;

typedef struct {
  const t_eubrl_protocol *protocol;
  ssize_t (*read) (BrailleDisplay *brl, void *buffer, size_t bufsize, int wait);
  ssize_t (*write) (BrailleDisplay *brl, const void *buf, size_t size);
} t_eubrl_io;

extern const t_eubrl_io *io;
extern const t_eubrl_protocol clioProtocol;
extern const t_eubrl_protocol esysirisProtocol;

extern unsigned int eubrl_handleBrailleKey (unsigned int key, KeyTableCommandContext ctx);

#endif /* __EU_PROTOCOL_H__ */
