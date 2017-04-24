/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2017 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "ctb_translate.h"

#include <liblouis.h>

static int
contractText_louis (BrailleContractionData *bcd) {
  return 0;
}

static void
finishCharacterEntry_louis (BrailleContractionData *bcd, CharacterEntry *entry) {
}

static const ContractionTableTranslationMethods louisTranslationMethods = {
  .contractText = contractText_louis,
  .finishCharacterEntry = finishCharacterEntry_louis
};

const ContractionTableTranslationMethods *
getContractionTableTranslationMethods_louis (void) {
  return &louisTranslationMethods;
}
