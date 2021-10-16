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

#ifndef BRLTTY_INCLUDED_HID_TABLES
#define BRLTTY_INCLUDED_HID_TABLES

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

typedef struct {
  const void *const entries;
  size_t const size;
  size_t const count;
  const HidTableEntryHeader **sorted;
} HidTableDescriptor;

#define HID_BEGIN_TABLE(type) static const Hid##type##Entry hid##type##Entries[] = {

#define HID_END_TABLE(type) }; \
static HidTableDescriptor hid##type##Descriptor = { \
  .entries = hid##type##Entries, \
  .size = sizeof(hid##type##Entries[0]), \
  .count = ARRAY_COUNT(hid##type##Entries), \
  .sorted = NULL \
}; \
const Hid##type##Entry *hidGet##type##Entry (uint32_t value) { \
  return hidGetTableEntry(&hid##type##Descriptor, value); \
}

#define HID_TABLE_METHODS(type) \
extern const Hid##type##Entry *hidGet##type##Entry (uint32_t value);

typedef struct {
  HID_TABLE_ENTRY_HEADER;
} HidItemTypeEntry;
HID_TABLE_METHODS(ItemType)

typedef struct {
  HID_TABLE_ENTRY_HEADER;
} HidCollectionTypeEntry;
HID_TABLE_METHODS(CollectionType)

typedef struct {
  HID_TABLE_ENTRY_HEADER;
} HidUsagePageEntry;
HID_TABLE_METHODS(UsagePage)

extern const void *hidGetTableEntry (HidTableDescriptor *table, uint32_t value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID_TABLES */
