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
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>

#include "dataarea.h"

struct DataAreaStruct {
  unsigned char *address;
  size_t size;
  size_t used;
};

void
clearDataArea (DataArea *area) {
  area->address = NULL;
  area->size = 0;
  area->used = 0;
}

DataArea *
newDataArea (void) {
  DataArea *area;
  if ((area = malloc(sizeof(*area)))) clearDataArea(area);
  return area;
}

void
destroyDataArea (DataArea *area) {
  if (area->address) free(area->address);
  free(area);
}

int
allocateDataItem (DataArea *area, DataOffset *offset, size_t size, int alignment) {
  size_t newUsed = (area->used = (area->used + (alignment - 1)) / alignment * alignment) + size;

  if (newUsed > area->size) {
    size_t newSize = newUsed | 0XFFF;
    unsigned char *newAddress = realloc(area->address, newSize);

    if (!newAddress) {
      return 0;
    }

    memset(newAddress+area->size, 0, newSize-area->size);
    area->address = newAddress;
    area->size = newSize;
  }

  *offset = area->used;
  area->used = newUsed;
  return 1;
}

void *
getDataItem (DataArea *area, DataOffset offset) {
  return area->address + offset;
}

int
saveDataItem (DataArea *area, DataOffset *offset, const void *item, size_t size, int alignment) {
  if (!allocateDataItem(area, offset, size, alignment)) return 0;
  memcpy(getDataItem(area, *offset), item, size);
  return 1;
}
