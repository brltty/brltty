/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_BRLDEFS
#define BRLTTY_INCLUDED_BRLDEFS

#include "ktbdefs.h"
#include "giodefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  BRL_FIRMNESS_MINIMUM,
  BRL_FIRMNESS_LOW,
  BRL_FIRMNESS_MEDIUM,
  BRL_FIRMNESS_HIGH,
  BRL_FIRMNESS_MAXIMUM
} BrailleFirmness;

typedef enum {
  BRL_SENSITIVITY_MINIMUM,
  BRL_SENSITIVITY_LOW,
  BRL_SENSITIVITY_MEDIUM,
  BRL_SENSITIVITY_HIGH,
  BRL_SENSITIVITY_MAXIMUM
} BrailleSensitivity;

typedef enum {
  BRL_ORIENTATION_NORMAL,
  BRL_ORIENTATION_ROTATED
} BrailleOrientation;

typedef struct BrailleDisplayStruct BrailleDisplay;
typedef struct BrailleDataStruct BrailleData;

typedef int BrailleFirmnessSetter (BrailleDisplay *brl, BrailleFirmness setting);
typedef int BrailleSensitivitySetter (BrailleDisplay *brl, BrailleSensitivity setting);
typedef void BrailleKeyRotator (BrailleDisplay *brl, unsigned char *set, unsigned char *key);

struct BrailleDisplayStruct {
  unsigned int textColumns, textRows;
  unsigned int statusColumns, statusRows;

  const char *keyBindings;
  KEY_NAME_TABLES_REFERENCE keyNameTables;
  KeyTable *keyTable;

  GioEndpoint *gioEndpoint;
  unsigned char *buffer;
  unsigned int writeDelay;

  int cursor;
  unsigned isCoreBuffer:1;
  unsigned resizeRequired:1;
  unsigned noDisplay:1;
  void (*bufferResized) (unsigned int rows, unsigned int columns);
  unsigned touchEnabled:1;
  unsigned highlightWindow:1;
  BrailleData *data;

  BrailleFirmnessSetter *setFirmness;
  BrailleSensitivitySetter *setSensitivity;
  BrailleKeyRotator *rotateKey;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRLDEFS */
