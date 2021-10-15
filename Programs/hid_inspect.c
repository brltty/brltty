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
#include "bitmask.h"
#include "hid_defs.h"
#include "hid_items.h"
#include "hid_inspect.h"

HidReports *
hidGetReports (const HidItemsDescriptor *items) {
  unsigned char identifiers[UINT8_MAX];
  unsigned char count = 0;

  BITMASK(haveIdentifier, UINT8_MAX+1, char);
  BITMASK_ZERO(haveIdentifier);

  const unsigned char *nextByte = items->bytes;
  size_t bytesLeft = items->count;

  while (1) {
    HidItem item;
    if (!hidGetNextItem(&item, &nextByte, &bytesLeft)) break;

    switch (item.type) {
      case HID_ITM_ReportID: {
        uint32_t identifier = item.value.u;

        if (!identifier) continue;
        if (identifier > UINT8_MAX) continue;
        if (BITMASK_TEST(haveIdentifier, identifier)) continue;

        BITMASK_SET(haveIdentifier, identifier);
        identifiers[count++] = identifier;
        break;
      }

      case HID_ITM_Input:
      case HID_ITM_Output:
      case HID_ITM_Feature:
        if (!count) identifiers[count++] = 0;
        break;
    }
  }

  HidReports *reports;
  size_t size = sizeof(*reports);
  size += count;

  if ((reports = malloc(size))) {
    memset(reports, 0, sizeof(*reports));

    reports->count = count;
    memcpy(reports->identifiers, identifiers, count);

    return reports;
  } else {
    logMallocError();
  }

  return NULL;
}

#define HID_USAGE_PAGE_ENTRY(name) HID_TABLE_ENTRY(HID_UPG, name)
#define HID_COLLECTION_TYPE_ENTRY(name) HID_TABLE_ENTRY(HID_COL, name)

const HidUsagePageEntry *
hidGetUsagePageEntry (uint32_t page) {
  static const HidUsagePageEntry table[] = {
    HID_USAGE_PAGE_ENTRY(GenericDesktop),
    HID_USAGE_PAGE_ENTRY(Simulation),
    HID_USAGE_PAGE_ENTRY(VirtualReality),
    HID_USAGE_PAGE_ENTRY(Sport),
    HID_USAGE_PAGE_ENTRY(Game),
    HID_USAGE_PAGE_ENTRY(GenericDevice),
    HID_USAGE_PAGE_ENTRY(KeyboardKeypad),
    HID_USAGE_PAGE_ENTRY(LEDs),
    HID_USAGE_PAGE_ENTRY(Button),
    HID_USAGE_PAGE_ENTRY(Ordinal),
    HID_USAGE_PAGE_ENTRY(Telephony),
    HID_USAGE_PAGE_ENTRY(Consumer),
    HID_USAGE_PAGE_ENTRY(Digitizer),
    HID_USAGE_PAGE_ENTRY(PhysicalInterfaceDevice),
    HID_USAGE_PAGE_ENTRY(Unicode),
    HID_USAGE_PAGE_ENTRY(AlphanumericDisplay),
    HID_USAGE_PAGE_ENTRY(MedicalInstruments),
    HID_USAGE_PAGE_ENTRY(BarCodeScanner),
    HID_USAGE_PAGE_ENTRY(Braille),
    HID_USAGE_PAGE_ENTRY(Scale),
    HID_USAGE_PAGE_ENTRY(MagneticStripeReader),
    HID_USAGE_PAGE_ENTRY(Camera),
    HID_USAGE_PAGE_ENTRY(Arcade),
  };

  static void *sorted = NULL;
  return hidGetTableEntry(
    table, ARRAY_COUNT(table), sizeof(table[0]),
    &sorted, page
  );
}

const HidCollectionTypeEntry *
hidGetCollectionTypeEntry (uint32_t type) {
  static const HidCollectionTypeEntry table[] = {
    HID_COLLECTION_TYPE_ENTRY(Physical),
    HID_COLLECTION_TYPE_ENTRY(Application),
    HID_COLLECTION_TYPE_ENTRY(Logical),
  };

  static void *sorted = NULL;
  return hidGetTableEntry(
    table, ARRAY_COUNT(table), sizeof(table[0]),
    &sorted, type
  );
}

STR_BEGIN_FORMATTER(hidFormatUsageFlags, uint32_t flags)
  typedef struct {
    const char *on;
    const char *off;
    uint16_t bit;
  } FlagEntry;

  static const FlagEntry flagTable[] = {
    { .bit = HID_USG_FLG_CONSTANT,
      .on = "const",
      .off = "data"
    },

    { .bit = HID_USG_FLG_VARIABLE,
      .on = "var",
      .off = "array"
    },

    { .bit = HID_USG_FLG_RELATIVE,
      .on = "rel",
      .off = "abs"
    },

    { .bit = HID_USG_FLG_WRAP,
      .on = "wrap",
    },

    { .bit = HID_USG_FLG_NON_LINEAR,
      .on = "nonlin",
    },

    { .bit = HID_USG_FLG_NO_PREFERRED,
      .on = "nopref",
    },

    { .bit = HID_USG_FLG_NULL_STATE,
      .on = "null",
    },

    { .bit = HID_USG_FLG_VOLATILE,
      .on = "volatile",
    },

    { .bit = HID_USG_FLG_BUFFERED_BYTE,
      .on = "buffbyte",
    },
  };

  const FlagEntry *flag = flagTable;
  const FlagEntry *end = flag + ARRAY_COUNT(flagTable);

  while (flag < end) {
    const char *name = (flags & flag->bit)? flag->on: flag->off;

    if (name) {
      if (STR_LENGTH > 0) STR_PRINTF(" ");
      STR_PRINTF("%s", name);
    }

    flag += 1;
  }
STR_END_FORMATTER

static int
hidListItem (const char *line, void *data) {
  return logMessage((LOG_CATEGORY(HUMAN_INTERFACE) | LOG_DEBUG), "%s", line);
}

int
hidListItems (const HidItemsDescriptor *items, HidItemLister *listItem, void *data) {
  if (!listItem) listItem = hidListItem;
  const char *label = "Items List";

  {
    char line[0X40];
    STR_BEGIN(line, sizeof(line));
    STR_PRINTF("Begin %s: Bytes:%"PRIsize, label, items->count);
    STR_END;
    if (!listItem(line, data)) return 0;
  }

  unsigned int itemCount = 0;
  const unsigned char *nextByte = items->bytes;
  size_t bytesLeft = items->count;

  int decOffsetWidth;
  int hexOffsetWidth;
  {
    unsigned int maximumOffset = bytesLeft;
    char buffer[0X20];

    decOffsetWidth = snprintf(buffer, sizeof(buffer), "%u", maximumOffset);
    hexOffsetWidth = snprintf(buffer, sizeof(buffer), "%x", maximumOffset);
  }

  while (1) {
    unsigned int offset = nextByte - items->bytes;
    HidItem item;
    int ok = hidGetNextItem(&item, &nextByte, &bytesLeft);

    char line[0X100];
    STR_BEGIN(line, sizeof(line));

    STR_PRINTF(
      "Item: %*" PRIu32 " (0X%.*" PRIX32 "):",
      decOffsetWidth, offset, hexOffsetWidth, offset
    );

    if (ok) {
      itemCount += 1;

      {
        const char *name = hidGetItemTypeName(item.type);

        if (name) {
          STR_PRINTF(" %s", name);
        } else {
          STR_PRINTF(" unknown item type: 0X%02X", item.type);
        }
      }

      if (item.valueSize > 0) {
        uint32_t hexValue = item.value.u & ((UINT64_C(1) << (item.valueSize * 8)) - 1);
        int hexPrecision = item.valueSize * 2;

        STR_PRINTF(
          " = %" PRId32 " (0X%.*" PRIX32 ")",
          item.value.s, hexPrecision, hexValue
        );
      }

      {
        uint32_t value = item.value.u;
        const char *text = NULL;
        char buffer[0X40];

        switch (item.type) {
          case HID_ITM_UsagePage: {
            const HidUsagePageEntry *upg = hidGetUsagePageEntry(value);
            if (upg) text = upg->header.name;
            break;
          }

          case HID_ITM_Collection: {
            const HidCollectionTypeEntry *col = hidGetCollectionTypeEntry(value);
            if (col) text = col->header.name;
            break;
          }

          case HID_ITM_Input:
          case HID_ITM_Output:
          case HID_ITM_Feature:
            STR_BEGIN(buffer, sizeof(buffer));
            STR_FORMAT(hidFormatUsageFlags, value);
            STR_END;
            text = buffer;
            break;
        }

        if (text) STR_PRINTF(": %s", text);
      }
    } else if (bytesLeft) {
      STR_PRINTF(" incomplete:");
      const unsigned char *end = nextByte + bytesLeft;

      while (nextByte < end) {
        STR_PRINTF(" %02X", *nextByte++);
      }
    } else {
      STR_PRINTF(" end");
    }

    STR_END;
    if (!listItem(line, data)) return 0;
    if (!ok) break;
  }

  {
    char line[0X40];
    STR_BEGIN(line, sizeof(line));
    STR_PRINTF("End %s: Items:%u", label, itemCount);
    STR_END;
    return listItem(line, data);
  }
}
