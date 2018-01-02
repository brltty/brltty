/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "bluetooth_internal.h"

#define BLUETOOTH_NAME_DRIVERS(name, ...) static const char *const bluetoothNameDrivers_##name[] = {__VA_ARGS__, NULL}
BLUETOOTH_NAME_DRIVERS(Actilino, "ht");
BLUETOOTH_NAME_DRIVERS(ActiveBraille, "ht");
BLUETOOTH_NAME_DRIVERS(ActiveStar, "ht");
BLUETOOTH_NAME_DRIVERS(AlvaBC, "al");
BLUETOOTH_NAME_DRIVERS(Apex, "bn");
BLUETOOTH_NAME_DRIVERS(BasicBraille, "ht");
BLUETOOTH_NAME_DRIVERS(BaumConny, "bm");
BLUETOOTH_NAME_DRIVERS(BaumPocketVario, "bm");
BLUETOOTH_NAME_DRIVERS(BaumSuperVario, "bm");
BLUETOOTH_NAME_DRIVERS(BaumSVario, "bm");
BLUETOOTH_NAME_DRIVERS(BrailleConnect, "bm");
BLUETOOTH_NAME_DRIVERS(BrailleEdge, "hm");
BLUETOOTH_NAME_DRIVERS(BrailleMe, "ic");
BLUETOOTH_NAME_DRIVERS(BrailleMemoPocket, "mm");
BLUETOOTH_NAME_DRIVERS(BrailleMemoSmart, "mm");
BLUETOOTH_NAME_DRIVERS(BrailleMemo32, "mm");
BLUETOOTH_NAME_DRIVERS(BrailleNoteTouch, "hw");
BLUETOOTH_NAME_DRIVERS(BrailleSense, "hm");
BLUETOOTH_NAME_DRIVERS(BrailleStar, "ht");
BLUETOOTH_NAME_DRIVERS(Braillex, "pm");
BLUETOOTH_NAME_DRIVERS(BrailliantBI, "hw");
BLUETOOTH_NAME_DRIVERS(Brailliant80, "hw");
BLUETOOTH_NAME_DRIVERS(Braillino, "ht");
BLUETOOTH_NAME_DRIVERS(B2G, "bm");
BLUETOOTH_NAME_DRIVERS(Conny, "bm");
BLUETOOTH_NAME_DRIVERS(EL12, "al", "vo");
BLUETOOTH_NAME_DRIVERS(Eurobraille, "eu");
BLUETOOTH_NAME_DRIVERS(Focus, "fs");
BLUETOOTH_NAME_DRIVERS(HWGBrailliant, "bm");
BLUETOOTH_NAME_DRIVERS(MB248, "md");
BLUETOOTH_NAME_DRIVERS(OrbitReader, "bm");
BLUETOOTH_NAME_DRIVERS(Pronto, "bm");
BLUETOOTH_NAME_DRIVERS(Refreshabraille, "bm");
BLUETOOTH_NAME_DRIVERS(SmartBeetle, "hm");
BLUETOOTH_NAME_DRIVERS(SuperVario, "bm");
BLUETOOTH_NAME_DRIVERS(TSM, "sk");
BLUETOOTH_NAME_DRIVERS(VarioConnect, "bm");
BLUETOOTH_NAME_DRIVERS(VarioUltra, "bm");

const BluetoothNameEntry bluetoothNameTable[] = {
  { .namePrefix = "Actilino ALO",
    .driverCodes = bluetoothNameDrivers_Actilino
  },

  { .namePrefix = "Active Braille AB",
    .driverCodes = bluetoothNameDrivers_ActiveBraille
  },

  { .namePrefix = "Active Star AS",
    .driverCodes = bluetoothNameDrivers_ActiveStar
  },

  { .namePrefix = "ALVA BC",
    .driverCodes = bluetoothNameDrivers_AlvaBC
  },

  { .namePrefix = "Apex",
    .driverCodes = bluetoothNameDrivers_Apex
  },

  { .namePrefix = "Basic Braille BB",
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

  { .namePrefix = "BrailleMe",
    .driverCodes = bluetoothNameDrivers_BrailleMe
  },

  { .namePrefix = "BMpk",
    .driverCodes = bluetoothNameDrivers_BrailleMemoPocket
  },

  { .namePrefix = "BMsmart",
    .driverCodes = bluetoothNameDrivers_BrailleMemoSmart
  },

  { .namePrefix = "BM32",
    .driverCodes = bluetoothNameDrivers_BrailleMemo32
  },

  { .namePrefix = "BrailleNote Touch",
    .driverCodes = bluetoothNameDrivers_BrailleNoteTouch
  },

  { .namePrefix = "BrailleSense",
    .driverCodes = bluetoothNameDrivers_BrailleSense
  },

  { .namePrefix = "Braille Star",
    .driverCodes = bluetoothNameDrivers_BrailleStar
  },

  { .namePrefix = "Braillex",
    .driverCodes = bluetoothNameDrivers_Braillex
  },

  { .namePrefix = "Brailliant BI",
    .driverCodes = bluetoothNameDrivers_BrailliantBI
  },

  { .namePrefix = "Brailliant 80",
    .driverCodes = bluetoothNameDrivers_Brailliant80
  },

  { .namePrefix = "Braillino BL",
    .driverCodes = bluetoothNameDrivers_Braillino
  },

  { .namePrefix = "B2G",
    .driverCodes = bluetoothNameDrivers_B2G
  },

  { .namePrefix = "Conny",
    .driverCodes = bluetoothNameDrivers_Conny
  },

  { .namePrefix = "EL12-",
    .driverCodes = bluetoothNameDrivers_EL12
  },

  { .namePrefix = "Esys-",
    .driverCodes = bluetoothNameDrivers_Eurobraille
  },

  { .namePrefix = "Focus",
    .driverCodes = bluetoothNameDrivers_Focus
  },

  { .namePrefix = "HWG Brailliant",
    .driverCodes = bluetoothNameDrivers_HWGBrailliant
  },

  { .namePrefix = "MB248",
    .driverCodes = bluetoothNameDrivers_MB248
  },

  { .namePrefix = "Orbit Reader",
    .driverCodes = bluetoothNameDrivers_OrbitReader
  },

  { .namePrefix = "Pronto!",
    .driverCodes = bluetoothNameDrivers_Pronto
  },

  { .namePrefix = "Refreshabraille",
    .driverCodes = bluetoothNameDrivers_Refreshabraille
  },

  { .namePrefix = "SmartBeetle",
    .driverCodes = bluetoothNameDrivers_SmartBeetle
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

  { .namePrefix = "VarioUltra",
    .driverCodes = bluetoothNameDrivers_VarioUltra
  },

  { .namePrefix = NULL }
};
