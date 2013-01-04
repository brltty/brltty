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

#include "prologue.h"

#include "log.h"
#include "file.h"
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

int
replaceAttributesTable (const char *directory, const char *name) {
  int ok = 0;
  char *path;

  if ((path = makeAttributesTablePath(directory, name))) {
    AttributesTable *newTable;

    logMessage(LOG_DEBUG, "compiling attributes table: %s", path);
    if ((newTable = compileAttributesTable(path))) {
      AttributesTable *oldTable = attributesTable;
      attributesTable = newTable;
      destroyAttributesTable(oldTable);
      ok = 1;
    } else {
      logMessage(LOG_ERR, "%s: %s", gettext("cannot compile attributes table"), path);
    }

    free(path);
  }

  if (!ok) logMessage(LOG_ERR, "%s: %s", gettext("cannot load attributes table"), name);
  return ok;
}
