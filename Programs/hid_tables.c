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

#include "hid_defs.h"
#include "hid_tables.h"

#define HID_COLLECTION_TYPE_ENTRY(name) HID_TABLE_ENTRY(HID_COL, name)
HID_BEGIN_TABLE(CollectionType)
  HID_COLLECTION_TYPE_ENTRY(Physical),
  HID_COLLECTION_TYPE_ENTRY(Application),
  HID_COLLECTION_TYPE_ENTRY(Logical),
HID_END_TABLE(CollectionType)

#define HID_USAGE_PAGE_ENTRY(name) HID_TABLE_ENTRY(HID_UPG, name)
HID_BEGIN_TABLE(UsagePage)
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
HID_END_TABLE(UsagePage)

