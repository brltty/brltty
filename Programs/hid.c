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

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "hid.h"
#include "strfmt.h"

#define HID_ITEM_TYPE_NAME(name) [HID_ITM_ ## name] = #name

static const char *hidItemTypeNames[] = {
  HID_ITEM_TYPE_NAME(UsagePage),
  HID_ITEM_TYPE_NAME(Usage),
  HID_ITEM_TYPE_NAME(LogicalMinimum),
  HID_ITEM_TYPE_NAME(UsageMinimum),
  HID_ITEM_TYPE_NAME(LogicalMaximum),
  HID_ITEM_TYPE_NAME(UsageMaximum),
  HID_ITEM_TYPE_NAME(PhysicalMinimum),
  HID_ITEM_TYPE_NAME(DesignatorIndex),
  HID_ITEM_TYPE_NAME(PhysicalMaximum),
  HID_ITEM_TYPE_NAME(DesignatorMinimum),
  HID_ITEM_TYPE_NAME(UnitExponent),
  HID_ITEM_TYPE_NAME(DesignatorMaximum),
  HID_ITEM_TYPE_NAME(Unit),
  HID_ITEM_TYPE_NAME(ReportSize),
  HID_ITEM_TYPE_NAME(StringIndex),
  HID_ITEM_TYPE_NAME(Input),
  HID_ITEM_TYPE_NAME(ReportID),
  HID_ITEM_TYPE_NAME(StringMinimum),
  HID_ITEM_TYPE_NAME(Output),
  HID_ITEM_TYPE_NAME(ReportCount),
  HID_ITEM_TYPE_NAME(StringMaximum),
  HID_ITEM_TYPE_NAME(Collection),
  HID_ITEM_TYPE_NAME(Push),
  HID_ITEM_TYPE_NAME(Delimiter),
  HID_ITEM_TYPE_NAME(Feature),
  HID_ITEM_TYPE_NAME(Pop),
  HID_ITEM_TYPE_NAME(EndCollection),
};

const char *
hidGetItemTypeName (unsigned char type) {
  if (type >= ARRAY_COUNT(hidItemTypeNames)) return NULL;
  return hidItemTypeNames[type];
}

const unsigned char hidItemLengths[] = {0, 1, 2, 4};

int
hidGetNextItem (
  HidItemDescription *item,
  const unsigned char **bytes,
  size_t *count
) {
  if (!*count) return 0;

  const unsigned char *byte = *bytes;
  const unsigned char *endBytes = byte + *count;

  unsigned char type = HID_ITEM_TYPE(*byte);
  unsigned char length = hidItemLengths[HID_ITEM_LENGTH(*byte)];

  const unsigned char *endValue = ++byte + length;
  if (endValue > endBytes) return 0;

  item->type = type;
  item->length = length;
  item->value = 0;

  {
    unsigned char shift = 0;

    while (byte < endValue) {
      item->value |= *byte++ << shift;
      shift += 8;
    }

    if (hidHasSignedValue(item->type)) {
      shift = 0X20 - shift;
      item->value <<= shift;
      item->value >>= shift;
    }
  }

  *bytes = byte;
  *count = endBytes - byte;
  return 1;
}

int
hidFillReportDescription (
  const unsigned char *bytes, size_t count,
  unsigned char identifier,
  HidReportDescription *report
) {
  int found = 0;
  int32_t reportCount = 0;
  int32_t reportSize = 0;

  while (count) {
    HidItemDescription item;
    if (!hidGetNextItem(&item, &bytes, &count)) break;

    if (item.type ==  HID_ITM_ReportID) {
      if (found) return 1;

      if (item.value == identifier) {
        found = 1;
        memset(report, 0, sizeof(*report));

        report->reportIdentifier = identifier;
        report->reportSize = 0;
      }

      continue;
    }

    if (!found) continue;

    switch (item.type) {
      case HID_ITM_Input:
      case HID_ITM_Output:
      case HID_ITM_Feature:
      {
        report->reportSize += reportSize * reportCount;
        continue;
      }

      case HID_ITM_ReportCount: {
        reportCount = item.value;
        goto itemTypeEncountered;
      }

      case HID_ITM_ReportSize: {
        reportSize = item.value;
        goto itemTypeEncountered;
      }

      itemTypeEncountered: {
        report->definedItemTypes |= HID_ITEM_BIT(item.type);
        continue;
      }

      default:
        continue;
    }
  }

  return found;
}

int
hidGetReportSize (
  const unsigned char *bytes, size_t count,
  unsigned char identifier, size_t *size
) {
  HidReportDescription report;
  *size = 0;

  if (hidFillReportDescription(bytes, count, identifier, &report)) {
    *size = (report.reportSize + 7) / 8;
    *size += 1;

    logMessage(LOG_CATEGORY(USB_IO),
      "HID report size: %02X = %"PRIsize, identifier, *size
    );

    return 1;
  } else {
    logMessage(LOG_WARNING, "HID report not found: %02X", identifier);
  }

  return 0;
}

void
hidLogItems (int level, const unsigned char *bytes, size_t count) {
  const char *label = "HID items log";
  logMessage(level, "begin %s", label);

  const unsigned char *byte = bytes;

  while (1) {
    unsigned int offset = byte - bytes;

    HidItemDescription item;
    if (!hidGetNextItem(&item, &byte, &count)) break;

    char log[0X100];
    STR_BEGIN(log, sizeof(log));

    STR_PRINTF(
      "HID item at %" PRIu32 " (0X%" PRIX32 "):",
      offset, offset
    );

    {
      const char *name = hidGetItemTypeName(item.type);
      if (!name) name = "<unknown>";
      STR_PRINTF(" %s", name);
    }

    if (item.length > 0) {
      STR_PRINTF(
        " = %" PRId32 " (0X%" PRIX32 ")",
        item.value, item.value
      );
    }

    STR_END;
    logMessage(level, "%s", log);
  }

  logMessage(level, "end %s", label);
}
