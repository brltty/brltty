/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include "prologue.h"

#include "atb.h"
#include "atb_internal.h"

static const unsigned char internalAttributesTableBytes[] = {
#include "attr.auto.h"
};

static AttributesTable internalAttributesTable = {
  .header.bytes = internalAttributesTableBytes,
  .size = 0
};

AttributesTable *attributesTable = &internalAttributesTable;

unsigned char
convertAttributesToDots (AttributesTable *table, unsigned char attributes) {
  return table->header.fields->attributesToDots[attributes];
}
