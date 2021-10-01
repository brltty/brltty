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

#define HID_NAME(prefix, name) [prefix ## _ ## name] = #name
#define HID_ITEM_TYPE_NAME(name) HID_NAME(HID_ITM, name)
#define HID_COLLECTION_TYPE_NAME(name) HID_NAME(HID_COL, name)
#define HID_USAGE_PAGE_NAME(name) HID_NAME(HID_UPG, name)

static inline const char *
hidGetName (const char *const *names, size_t count, uint32_t index) {
  if (index >= count) return NULL;
  return names[index];
}

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

const char *
hidGetCollectionTypeName (uint32_t type) {
  static const char *names[] = {
    HID_COLLECTION_TYPE_NAME(Physical),
    HID_COLLECTION_TYPE_NAME(Application),
    HID_COLLECTION_TYPE_NAME(Logical),
  };

  return hidGetName(names, ARRAY_COUNT(names), type);
}

const char *
hidGetUsagePageName (uint16_t page) {
  static const char *names[] = {
    HID_USAGE_PAGE_NAME(GenericDesktop),
    HID_USAGE_PAGE_NAME(Simulation),
    HID_USAGE_PAGE_NAME(VirtualReality),
    HID_USAGE_PAGE_NAME(Sport),
    HID_USAGE_PAGE_NAME(Game),
    HID_USAGE_PAGE_NAME(GenericDevice),
    HID_USAGE_PAGE_NAME(KeyboardKeypad),
    HID_USAGE_PAGE_NAME(LEDs),
    HID_USAGE_PAGE_NAME(Button),
    HID_USAGE_PAGE_NAME(Ordinal),
    HID_USAGE_PAGE_NAME(Telephony),
    HID_USAGE_PAGE_NAME(Consumer),
    HID_USAGE_PAGE_NAME(Digitizer),
    HID_USAGE_PAGE_NAME(PhysicalInterfaceDevice),
    HID_USAGE_PAGE_NAME(Unicode),
    HID_USAGE_PAGE_NAME(AlphanumericDisplay),
    HID_USAGE_PAGE_NAME(MedicalInstruments),
    HID_USAGE_PAGE_NAME(BarCodeScanner),
    HID_USAGE_PAGE_NAME(Braille),
    HID_USAGE_PAGE_NAME(Scale),
    HID_USAGE_PAGE_NAME(MagneticStripeReader),
    HID_USAGE_PAGE_NAME(Camera),
    HID_USAGE_PAGE_NAME(Arcade),
  };

  return hidGetName(names, ARRAY_COUNT(names), page);
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
  const unsigned char *bytes, size_t count,
  unsigned char identifier, size_t *size
) {
  int noIdentifier = !identifier;
  int found = noIdentifier;

  size_t totalBits = 0;
  if (!noIdentifier) totalBits += 8;

  uint64_t definedItemTypes = 0;
  uint32_t reportSize = 0;
  uint32_t reportCount = 0;

  while (count) {
    HidItemDescription item;

    if (!hidGetNextItem(&item, &bytes, &count)) {
      if (count) return 0;
      break;
    }

    if (item.type == HID_ITM_ReportID) {
      if (noIdentifier) {
        found = 0;
        break;
      }

      if (found) break;
      if (item.value.u == identifier) found = 1;
      continue;
    }

    if (found) {
      switch (item.type) {
        case HID_ITM_Input:
        case HID_ITM_Output:
        case HID_ITM_Feature:
          totalBits += reportSize * reportCount;
          break;

        case HID_ITM_ReportCount:
          reportCount = item.value.u;
          break;

        case HID_ITM_ReportSize:
          reportSize = item.value.u;
          break;

        default:
          continue;
      }

      definedItemTypes |= HID_ITEM_BIT(item.type);
    }
  }

  if (found) {
    *size = (totalBits + 7) / 8;

    logMessage(LOG_CATEGORY(USB_IO),
      "HID report size: %02X = %"PRIsize,
      identifier, *size
    );
  } else {
    logMessage(LOG_WARNING, "HID report not found: %02X", identifier);
  }

  return found;
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

    if (item.valueSize > 0) {
      uint32_t hexValue = item.value.u & ((UINT64_C(1) << (item.valueSize * 8)) - 1);
      int hexPrecision = item.valueSize * 2;

      STR_PRINTF(
        " = %" PRId32 " (0X%.*" PRIX32 ")",
        item.value.s, hexPrecision, hexValue
      );
    }

    {
      const char *name = NULL;
      uint32_t value = item.value.u;

      switch (item.type) {
        case HID_ITM_Collection:
          name = hidGetCollectionTypeName(value);
          break;

        case HID_ITM_UsagePage:
          name = hidGetUsagePageName(value);
          break;
      }

      if (name) STR_PRINTF(": %s", name);
    }

    STR_END;
    logMessage(level, "%s", log);
  }

  logMessage(level, "end %s", label);
}
