/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_HID_ITEMS
#define BRLTTY_INCLUDED_HID_ITEMS

#include "hid_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef union {
  uint32_t u;
  int32_t s;
} HidItemValue;

typedef struct {
  HidItemValue value;
  uint8_t type;
  uint8_t valueSize;
} HidItem;

extern int hidGetNextItem (
  HidItem *item,
  const unsigned char **bytes,
  size_t *count
);

typedef struct {
  const char *name;
  uint32_t value;
} HidTableEntryHeader;

#define HID_TABLE_ENTRY_HEADER HidTableEntryHeader header

#define HID_TABLE_ENTRY(prefix, suffix, ...) { \
  .header = { \
    .name = #suffix, \
    .value = prefix ## _ ## suffix, \
  }, \
  __VA_ARGS__ \
}

extern const void *hidGetTableEntry (
  const void *table, size_t count, size_t size,
  void *sorted, uint32_t value
);

typedef struct {
  HID_TABLE_ENTRY_HEADER;
} HidItemTypeEntry;

extern const HidItemTypeEntry *hidGetItemTypeEntry (uint8_t type);
extern const char *hidGetItemTypeName (uint8_t type);
extern unsigned char hidGetValueSize (unsigned char item);

extern int hidGetReportSize (
  const HidItemsDescriptor *items,
  uint8_t identifier,
  HidReportSize *size
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID_ITEMS */
