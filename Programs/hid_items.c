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
#include "strfmt.h"
#include "hid_items.h"
#include "hid_defs.h"

#define HID_ITEM_TYPE_NAME(name) HID_NAME(HID_ITM, name)

const char *
hidGetItemTypeName (uint8_t type) {
  static const char *names[] = {
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

  return hidGetName(names, ARRAY_COUNT(names), type);
}

unsigned char
hidGetValueSize (unsigned char item) {
  static const unsigned char sizes[4] = {0, 1, 2, 4};
  return sizes[HID_ITEM_LENGTH(item)];
}

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
  unsigned char valueSize = hidGetValueSize(*byte);

  const unsigned char *endValue = ++byte + valueSize;
  if (endValue > endBytes) return 0;

  item->type = type;
  item->valueSize = valueSize;
  item->value.u = 0;

  {
    unsigned char shift = 0;

    while (byte < endValue) {
      item->value.u |= *byte++ << shift;
      shift += 8;
    }

    if (hidHasSignedValue(item->type)) {
      shift = 0X20 - shift;
      item->value.u <<= shift;
      item->value.s >>= shift;
    }
  }

  *bytes = byte;
  *count = endBytes - byte;
  return 1;
}

int
hidGetReportSize (
  const HidItemsDescriptor *items,
  uint8_t identifier,
  HidReportSize *size
) {
  const unsigned char *nextByte = items->bytes;
  size_t bytesLeft = items->count;

  int noIdentifier = !identifier;
  int reportFound = noIdentifier;

  size_t inputSize = 0;
  size_t outputSize = 0;
  size_t featureSize = 0;

  uint64_t itemTypesEncountered = 0;
  uint32_t reportIdentifier = 0;
  uint32_t reportSize = 0;
  uint32_t reportCount = 0;

  while (bytesLeft) {
    size_t offset = nextByte - items->bytes;
    HidItemDescription item;

    if (!hidGetNextItem(&item, &nextByte, &bytesLeft)) {
      if (bytesLeft) return 0;
      break;
    }

    if (item.type == HID_ITM_ReportID) {
      if (noIdentifier) {
        reportFound = 0;
        break;
      }

      reportIdentifier = item.value.u;
      if (reportIdentifier == identifier) reportFound = 1;
    } else {
      switch (item.type) {
      {
        size_t *size;

        case HID_ITM_Input:
          size = &inputSize;
          goto doSize;

        case HID_ITM_Output:
          size = &outputSize;
          goto doSize;

        case HID_ITM_Feature:
          size = &featureSize;
          goto doSize;

        doSize:
          if (reportIdentifier == identifier) *size += reportSize * reportCount;
          break;
      }

        case HID_ITM_ReportCount:
          reportCount = item.value.u;
          break;

        case HID_ITM_ReportSize:
          reportSize = item.value.u;
          break;

        case HID_ITM_Collection:
        case HID_ITM_EndCollection:
        case HID_ITM_UsagePage:
        case HID_ITM_UsageMinimum:
        case HID_ITM_UsageMaximum:
        case HID_ITM_Usage:
        case HID_ITM_LogicalMinimum:
        case HID_ITM_LogicalMaximum:
        case HID_ITM_PhysicalMinimum:
        case HID_ITM_PhysicalMaximum:
          break;

        default: {
          if (!(itemTypesEncountered & HID_ITEM_BIT(item.type))) {
            char log[0X100];
            STR_BEGIN(log, sizeof(log));
            STR_PRINTF("unhandled item type at offset %"PRIsize ": ", offset);

            {
              const char *name = hidGetItemTypeName(item.type);

              if (name) {
                STR_PRINTF("%s", name);
              } else {
                STR_PRINTF("0X%02X", item.type);
              }
            }

            STR_END;
            logMessage(LOG_WARNING, "%s", log);
          }

          break;
        }
      }
    }

    itemTypesEncountered |= HID_ITEM_BIT(item.type);
  }

  if (reportFound) {
    char log[0X100];
    STR_BEGIN(log, sizeof(log));
    STR_PRINTF("report size: %02X", identifier);

    {
      typedef struct {
        const char *label;
        size_t *bytes;
        size_t bits;
      } SizeEntry;

      SizeEntry sizeTable[] = {
        { .label = "In",
          .bits = inputSize,
          .bytes = &size->input
        },

        { .label = "Out",
          .bits = outputSize,
          .bytes = &size->output
        },

        { .label = "Ftr",
          .bits = featureSize,
          .bytes = &size->feature
        },
      };

      SizeEntry *entry = sizeTable;
      const SizeEntry *end = entry + ARRAY_COUNT(sizeTable);

      while (entry < end) {
        size_t bytes = (entry->bits + 7) / 8;
        if (bytes && !noIdentifier) bytes += 1;
        *entry->bytes = bytes;

        STR_PRINTF(" %s:%" PRIsize, entry->label, bytes);
        entry += 1;
      }
    }

    STR_END;
    logMessage(LOG_CATEGORY(HUMAN_INTERFACE), "%s", log);
  }

  return reportFound;
}
