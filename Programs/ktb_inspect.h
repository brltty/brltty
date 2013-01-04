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

#ifndef BRLTTY_INCLUDED_KTB_INSPECT
#define BRLTTY_INCLUDED_KTB_INSPECT

#include "brldefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline const KeyContext *
getKeyContext (KeyTable *table, unsigned char context) {
  if (context < table->keyContextCount) return &table->keyContextTable[context];
  return NULL;
}

static inline int
isTemporaryKeyContext (const KeyTable *table, const KeyContext *ctx) {
  return ((ctx - table->keyContextTable) > KTB_CTX_DEFAULT) && !ctx->title;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KTB_INSPECT */
