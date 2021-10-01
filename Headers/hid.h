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

#ifndef BRLTTY_INCLUDED_HID
#define BRLTTY_INCLUDED_HID

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
  uint8_t length;
} HidItemDescription;

extern int hidGetNextItem (
  HidItemDescription *item,
  const unsigned char **bytes,
  size_t *count
);

extern const unsigned char hidItemLengths[];
extern const char *hidGetItemTypeName (unsigned char type);
extern void hidLogItems (int level, const unsigned char *bytes, size_t count);

typedef struct {
  uint64_t definedItemTypes;
  unsigned int reportSize;
  unsigned char reportIdentifier;
} HidReportDescription;

extern int hidFillReportDescription (
  const unsigned char *bytes, size_t count,
  unsigned char identifier,
  HidReportDescription *report
);

extern int hidGetReportSize (
  const unsigned char *bytes, size_t count,
  unsigned char identifier, size_t *size
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID */
