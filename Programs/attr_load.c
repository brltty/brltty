/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>

#include "misc.h"
#include "datafile.h"
#include "attr.h"

typedef struct {
  unsigned char attribute;
  unsigned char operation;
} DotData;

typedef struct {
  DotData dots[8];
} AttributesData;

static void
makeAttributesTable (AttributesTable table, const AttributesData *attributes) {
  unsigned char bits = 0;

  do {
    unsigned char *cell = &table[bits];
    unsigned char dotIndex;

    *cell = 0;
    for (dotIndex=0; dotIndex<BRL_DOT_COUNT; dotIndex+=1) {
      const DotData *dot = &attributes->dots[dotIndex];
      int isSet = bits & dot->attribute;
      if (!isSet == !dot->operation) *cell |= brlDotBits[dotIndex];
    }
  } while ((bits += 1));
}

static int
parseAttributeOperand (DataFile *file, unsigned char *bit, unsigned char *operation, const wchar_t *characters, int length) {
  if (length > 1) {
    static const wchar_t operators[] = {'~', '='};
    const wchar_t *operator = wmemchr(operators, characters[0], ARRAY_COUNT(operators));

    if (operator) {
      typedef struct {
        const wchar_t *name;
        unsigned char bit;
      } AttributeEntry;

      static const AttributeEntry attributeTable[] = {
        {WS_C("fg-blue")  , 0X01},
        {WS_C("fg-green") , 0X02},
        {WS_C("fg-red")   , 0X04},
        {WS_C("fg-bright"), 0X08},
        {WS_C("bg-blue")  , 0X10},
        {WS_C("bg-green") , 0X20},
        {WS_C("bg-red")   , 0X40},
        {WS_C("blink")    , 0X80},

        {WS_C("bit0"), 0X01},
        {WS_C("bit1"), 0X02},
        {WS_C("bit2"), 0X04},
        {WS_C("bit3"), 0X08},
        {WS_C("bit4"), 0X10},
        {WS_C("bit5"), 0X20},
        {WS_C("bit6"), 0X40},
        {WS_C("bit7"), 0X80},

        {WS_C("bit01"), 0X01},
        {WS_C("bit02"), 0X02},
        {WS_C("bit04"), 0X04},
        {WS_C("bit08"), 0X08},
        {WS_C("bit10"), 0X10},
        {WS_C("bit20"), 0X20},
        {WS_C("bit40"), 0X40},
        {WS_C("bit80"), 0X80},

        {NULL, 0X00}
      };

      const AttributeEntry *attribute = attributeTable;

      characters += 1, length -= 1;

      while (attribute->name) {
        if ((length == wcslen(attribute->name)) &&
            (wmemcmp(characters, attribute->name, length) == 0)) {
          *bit = attribute->bit;
          *operation = operator - operators;
          return 1;
        }

        attribute += 1;
      }

      reportDataError(file, "invalid attribute name: %.*" PRIws, length, characters);
    } else {
      reportDataError(file, "invalid attribute operator: %.1" PRIws, &characters[0]);
    }
  }

  return 0;
}

static int
getAttributeOperand (DataFile *file, unsigned char *bit, unsigned char *operation) {
  DataOperand attribute;

  if (getDataOperand(file, &attribute, "attribute"))
    if (parseAttributeOperand(file, bit, operation, attribute.characters, attribute.length))
      return 1;

  return 0;
}

static int
processDotOperands (DataFile *file, void *data) {
  AttributesData *attributes = data;
  int dotIndex;

  if (getDotOperand(file, &dotIndex)) {
    DotData *dot = &attributes->dots[dotIndex];

    if (getAttributeOperand(file, &dot->attribute, &dot->operation)) return 1;
  }

  return 1;
}

static int
processAttributesLine (DataFile *file, void *data) {
  static const DataProperty properties[] = {
    {.name=WS_C("dot"), .processor=processDotOperands},
    {.name=WS_C("include"), .processor=processIncludeOperands},
    {.name=NULL, .processor=NULL}
  };

  return processPropertyOperand(file, properties, "attributes table directive", data);
}

int
loadAttributesTable (const char *name, AttributesTable table) {
  int ok = 0;
  AttributesData attributes;

  memset(&attributes, 0, sizeof(attributes));

  if (processDataFile(name, processAttributesLine, &attributes)) {
    makeAttributesTable(table, &attributes);
    ok = 1;
  }

  return ok;
}

void
fixAttributesTablePath (char **path) {
  fixPath(path, TRANSLATION_TABLE_EXTENSION, ATTRIBUTES_TABLE_PREFIX);
}
