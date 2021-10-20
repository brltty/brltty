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
#include "hid_tables.h"

static int
hidCompareTableEntriesByValue (const void *element1, const void *element2) {
  const HidTableEntryHeader *const *header1 = element1;
  const HidTableEntryHeader *const *header2 = element2;

  HidUnsignedValue value1 = (*header1)->value;
  HidUnsignedValue value2 = (*header2)->value;

  if (value1 < value2) return -1;
  if (value1 > value2) return 1;
  return 0;
}

const void *
hidGetTableEntry (HidTable *table, HidUnsignedValue value) {
  if (!table->sorted) {
    if (!(table->sorted = malloc(ARRAY_SIZE(table->sorted, table->count)))) {
      logMallocError();
      return NULL;
    }

    {
      const void *entry = table->entries;
      const HidTableEntryHeader **header = table->sorted;

      for (unsigned int index=0; index<table->count; index+=1) {
        *header++ = entry;
        entry += table->size;
      }
    }

    qsort(
      table->sorted, table->count, sizeof(*table->sorted),
      hidCompareTableEntriesByValue
    );
  }

  unsigned int from = 0;
  unsigned int to = table->count;

  while (from < to) {
    unsigned int current = (from + to) / 2;
    const HidTableEntryHeader *header = table->sorted[current];
    if (value == header->value) return header;

    if (value < header->value) {
      to = current;
    } else {
      from = current + 1;
    }
  }

  return NULL;
}

#define HID_ITEM_TYPE_ENTRY(name) HID_TABLE_ENTRY(HID_ITM, name)
HID_BEGIN_TABLE(ItemType)
  HID_ITEM_TYPE_ENTRY(UsagePage),
  HID_ITEM_TYPE_ENTRY(Usage),
  HID_ITEM_TYPE_ENTRY(LogicalMinimum),
  HID_ITEM_TYPE_ENTRY(UsageMinimum),
  HID_ITEM_TYPE_ENTRY(LogicalMaximum),
  HID_ITEM_TYPE_ENTRY(UsageMaximum),
  HID_ITEM_TYPE_ENTRY(PhysicalMinimum),
  HID_ITEM_TYPE_ENTRY(DesignatorIndex),
  HID_ITEM_TYPE_ENTRY(PhysicalMaximum),
  HID_ITEM_TYPE_ENTRY(DesignatorMinimum),
  HID_ITEM_TYPE_ENTRY(UnitExponent),
  HID_ITEM_TYPE_ENTRY(DesignatorMaximum),
  HID_ITEM_TYPE_ENTRY(Unit),
  HID_ITEM_TYPE_ENTRY(ReportSize),
  HID_ITEM_TYPE_ENTRY(StringIndex),
  HID_ITEM_TYPE_ENTRY(Input),
  HID_ITEM_TYPE_ENTRY(ReportID),
  HID_ITEM_TYPE_ENTRY(StringMinimum),
  HID_ITEM_TYPE_ENTRY(Output),
  HID_ITEM_TYPE_ENTRY(ReportCount),
  HID_ITEM_TYPE_ENTRY(StringMaximum),
  HID_ITEM_TYPE_ENTRY(Collection),
  HID_ITEM_TYPE_ENTRY(Push),
  HID_ITEM_TYPE_ENTRY(Delimiter),
  HID_ITEM_TYPE_ENTRY(Feature),
  HID_ITEM_TYPE_ENTRY(Pop),
  HID_ITEM_TYPE_ENTRY(EndCollection),
HID_END_TABLE(ItemType)

const char *
hidItemTypeName (uint8_t type) {
  const HidItemTypeEntry *entry = hidGetItemTypeEntry(type);
  if (!entry) return NULL;
  return entry->header.name;
}

unsigned char
hidItemValueSize (unsigned char item) {
  static const unsigned char sizes[4] = {0, 1, 2, 4};
  return sizes[HID_ITEM_LENGTH(item)];
}

int
hidNextItem (
  HidItem *item,
  const unsigned char **bytes,
  size_t *count
) {
  if (!*count) return 0;

  const unsigned char *byte = *bytes;
  const unsigned char *endBytes = byte + *count;

  unsigned char type = HID_ITEM_TYPE(*byte);
  unsigned char valueSize = hidItemValueSize(*byte);

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
hidReportSize (
  const HidItemsDescriptor *items,
  HidReportIdentifier identifier,
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
  HidUnsignedValue reportIdentifier = 0;
  HidUnsignedValue reportSize = 0;
  HidUnsignedValue reportCount = 0;

  while (bytesLeft) {
    size_t offset = nextByte - items->bytes;
    HidItem item;

    if (!hidNextItem(&item, &nextByte, &bytesLeft)) {
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
              const char *name = hidItemTypeName(item.type);

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
