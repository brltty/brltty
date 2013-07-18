/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "bluetooth_internal.h"

#define BLUETOOTH_NAME_DRIVERS(name, ...) static const char *const bluetoothNameDrivers_##name[] = {__VA_ARGS__, NULL}
BLUETOOTH_NAME_DRIVERS(ActiveBraille, "ht");
BLUETOOTH_NAME_DRIVERS(AlvaBC, "al");
BLUETOOTH_NAME_DRIVERS(BasicBraille, "ht");
BLUETOOTH_NAME_DRIVERS(BaumConny, "bm");
BLUETOOTH_NAME_DRIVERS(BaumPocketVario, "bm");
BLUETOOTH_NAME_DRIVERS(BaumSuperVario, "bm");
BLUETOOTH_NAME_DRIVERS(BaumSVario, "bm");
BLUETOOTH_NAME_DRIVERS(BrailleConnect, "bm");
BLUETOOTH_NAME_DRIVERS(BrailleEdge, "hm");
BLUETOOTH_NAME_DRIVERS(BrailleSense, "hm");
BLUETOOTH_NAME_DRIVERS(BrailleStar, "ht");
BLUETOOTH_NAME_DRIVERS(Brailliant, "bm");
BLUETOOTH_NAME_DRIVERS(BrailliantBI, "hw");
BLUETOOTH_NAME_DRIVERS(Conny, "bm");
BLUETOOTH_NAME_DRIVERS(Focus, "fs");
BLUETOOTH_NAME_DRIVERS(HWGBrailliant, "bm");
BLUETOOTH_NAME_DRIVERS(Pronto, "bm");
BLUETOOTH_NAME_DRIVERS(Refreshabraille, "bm");
BLUETOOTH_NAME_DRIVERS(SuperVario, "bm");
BLUETOOTH_NAME_DRIVERS(TSM, "sk");
BLUETOOTH_NAME_DRIVERS(VarioConnect, "bm");

const BluetoothNameEntry bluetoothNameTable[] = {
  { .namePrefix = "Active Braille",
    .driverCodes = bluetoothNameDrivers_ActiveBraille
  },

  { .namePrefix = "ALVA BC",
    .driverCodes = bluetoothNameDrivers_AlvaBC
  },

  { .namePrefix = "Basic Braille",
    .driverCodes = bluetoothNameDrivers_BasicBraille
  },

  { .namePrefix = "BAUM Conny",
    .driverCodes = bluetoothNameDrivers_BaumConny
  },

  { .namePrefix = "Baum PocketVario",
    .driverCodes = bluetoothNameDrivers_BaumPocketVario
  },

  { .namePrefix = "Baum SuperVario",
    .driverCodes = bluetoothNameDrivers_BaumSuperVario
  },

  { .namePrefix = "Baum SVario",
    .driverCodes = bluetoothNameDrivers_BaumSVario
  },

  { .namePrefix = "BrailleConnect",
    .driverCodes = bluetoothNameDrivers_BrailleConnect
  },

  { .namePrefix = "BrailleEDGE",
    .driverCodes = bluetoothNameDrivers_BrailleEdge
  },

  { .namePrefix = "BrailleSense",
    .driverCodes = bluetoothNameDrivers_BrailleSense
  },

  { .namePrefix = "Braille Star",
    .driverCodes = bluetoothNameDrivers_BrailleStar
  },

  { .namePrefix = "Brailliant BI",
    .driverCodes = bluetoothNameDrivers_BrailliantBI
  },

  { .namePrefix = "Brailliant",
    .driverCodes = bluetoothNameDrivers_Brailliant
  },

  { .namePrefix = "Conny",
    .driverCodes = bluetoothNameDrivers_Conny
  },

  { .namePrefix = "Focus",
    .driverCodes = bluetoothNameDrivers_Focus
  },

  { .namePrefix = "HWG Brailliant",
    .driverCodes = bluetoothNameDrivers_HWGBrailliant
  },

  { .namePrefix = "Pronto!",
    .driverCodes = bluetoothNameDrivers_Pronto
  },

  { .namePrefix = "Refreshabraille",
    .driverCodes = bluetoothNameDrivers_Refreshabraille
  },

  { .namePrefix = "SuperVario",
    .driverCodes = bluetoothNameDrivers_SuperVario
  },

  { .namePrefix = "TSM",
    .driverCodes = bluetoothNameDrivers_TSM
  },

  { .namePrefix = "VarioConnect",
    .driverCodes = bluetoothNameDrivers_VarioConnect
  },

  { .namePrefix = NULL }
};
