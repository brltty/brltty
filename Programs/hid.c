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

const unsigned char hidItemLengths[] = {0, 1, 2, 4};

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
      case HidItemType_ReportID: {
        if (!found && (value == identifier)) {
          memset(description, 0, sizeof(*description));
          description->reportIdentifier = identifier;

          found = 1;
          continue;
        }

        break;
      }

      case HidItemType_ReportCount: {
        if (found) {
          description->reportCount = value;
          goto defined;
        }

        break;
      }

      case HidItemType_ReportSize: {
        if (found) {
          description->reportSize = value;
          goto defined;
        }

        break;
      }

      case HidItemType_LogicalMinimum: {
        if (found) {
          description->logicalMinimum = value;
          goto defined;
        }

        break;
      }

      case HidItemType_LogicalMaximum: {
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
    if (description.definedItemTypes & HID_ITEM_BIT(HidItemType_ReportCount)) {
      if (description.definedItemTypes & HID_ITEM_BIT(HidItemType_ReportSize)) {
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
