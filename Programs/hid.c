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

const unsigned char hidItemLengths[] = {0, 1, 2, 4};

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

void
hidLogItems (int level, const unsigned char *items, size_t size) {
  const char *label = "HID items log";
  logMessage(level, "begin %s", label);

  const unsigned char *item = items;
  const unsigned char *endItems = item + size;

  while (item < endItems) {
    unsigned char type = HID_ITEM_TYPE(*item);
    uint32_t length = hidItemLengths[HID_ITEM_LENGTH(*item)];
    unsigned int offset = item++ - items;
    char log[0X100];

    {
      STR_BEGIN(log, sizeof(log));

      STR_PRINTF(
        "HID item at %" PRIu32 " (0X%" PRIX32 "):",
        offset, offset
      );

      {
        const char *name = hidGetItemTypeName(type);
        if (!name) name = "<unknown>";
        STR_PRINTF(" %s", name);
      }

      if (length > 0) {
        STR_PRINTF(" = ");
        const unsigned char *endValue = item + length;

        if (endValue > endItems) {
          STR_PRINTF("<truncated>");
        } else {
          uint32_t value = 0;
          unsigned int shift = 0;

          do {
            value |= *item++ << shift;
            shift += 8;
          } while (item < endValue);

          STR_PRINTF("%" PRIu32 " (0X%" PRIX32 ")", value, value);
        }
      }

      STR_END;
    }

    logMessage(LOG_NOTICE, "%s", log);
  }

  logMessage(level, "end %s", label);
}

int
hidFillReportDescription (
  const unsigned char *items, size_t size,
  unsigned char identifier,
  HidReportDescription *description
) {
  int found = 0;
  int index = 0;

  while (index < size) {
    unsigned char item = items[index++];
    HidItemType type = HID_ITEM_TYPE(item);
    unsigned char length = hidItemLengths[HID_ITEM_LENGTH(item)];
    uint32_t value = 0;

    if (length) {
      unsigned char shift = 0;

      do {
        if (index == size) return 0;
        value |= items[index++] << shift;
        shift += 8;
      } while (--length);
    }

    switch (type) {
      case HID_ITM_ReportID: {
        if (!found && (value == identifier)) {
          memset(description, 0, sizeof(*description));
          description->reportIdentifier = identifier;

          found = 1;
          continue;
        }

        break;
      }

      case HID_ITM_ReportCount: {
        if (found) {
          description->reportCount = value;
          goto defined;
        }

        break;
      }

      case HID_ITM_ReportSize: {
        if (found) {
          description->reportSize = value;
          goto defined;
        }

        break;
      }

      case HID_ITM_LogicalMinimum: {
        if (found) {
          description->logicalMinimum = value;
          goto defined;
        }

        break;
      }

      case HID_ITM_LogicalMaximum: {
        if (found) {
          description->logicalMaximum = value;
          goto defined;
        }

        break;
      }

      defined: {
        description->definedItemTypes |= HID_ITEM_BIT(type);
        continue;
      }

      default:
        break;
    }

    if (found) break;
  }

  return found;
}

int
hidGetReportSize (
  const unsigned char *items,
  size_t length,
  unsigned char identifier,
  size_t *size
) {
  HidReportDescription description;
  *size = 0;

  if (hidFillReportDescription(items, length, identifier, &description)) {
    if (description.definedItemTypes & HID_ITEM_BIT(HID_ITM_ReportCount)) {
      if (description.definedItemTypes & HID_ITEM_BIT(HID_ITM_ReportSize)) {
        uint32_t bytes = ((description.reportCount * description.reportSize) + 7) / 8;

        logMessage(LOG_CATEGORY(USB_IO), "HID report size: %02X = %"PRIu32, identifier, bytes);
        *size = 1 + bytes;
        return 1;
      } else {
        logMessage(LOG_WARNING, "HID report size not defined: %02X", identifier);
      }
    } else {
      logMessage(LOG_WARNING, "HID report count not defined: %02X", identifier);
    }
  } else {
    logMessage(LOG_WARNING, "HID report not found: %02X", identifier);
  }

  return 0;
}
