/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
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
#include "crc_internal.h"

crc_t
crcReflectBits (crc_t fromValue, unsigned int width) {
  crc_t fromBit = crcMostSignificantBit(width);
  crc_t toBit = 1;
  crc_t toValue = 0;

  while (fromBit) {
    if (fromValue & fromBit) toValue |= toBit;
    fromBit >>= 1;
    toBit <<= 1;
  }

  return toValue;
}

static inline void
crcReflectByte (const CRCGenerator *crc, uint8_t *byte) {
  *byte = crcReflectBits(*byte, crc->properties.byteWidth);
}

static inline void
crcReflectValue (const CRCGenerator *crc, crc_t *value) {
  *value = crcReflectBits(*value, crc->parameters.checksumWidth);
}

static void
crcCacheRemainders (CRCGenerator *crc) {
  // Compute the remainder for each possible dividend.
  for (unsigned int dividend=0; dividend<=UINT8_MAX; dividend+=1) {
    // Start with the dividend followed by zeros.
    crc_t remainder = dividend << crc->properties.byteShift;

    // Perform modulo-2 division, a bit at a time.
    for (unsigned int bit=crc->properties.byteWidth; bit>0; bit-=1) {
      // Try to divide the current data bit.
      if (remainder & crc->properties.mostSignificantBit) {
        remainder <<= 1;
        remainder ^= crc->parameters.generatorPolynomial;
      } else {
        remainder <<= 1;
      }
    }

    // Store the result into the table.
    crc->properties.remainderCache[dividend] = remainder & crc->properties.valueMask;
  }
}

void
crcAddByte (CRCGenerator *crc, uint8_t byte) {
  if (crc->parameters.reflectInput) crcReflectByte(crc, &byte);
  byte ^= crc->currentValue >> crc->properties.byteShift;
  crc->currentValue = crc->properties.remainderCache[byte] ^ (crc->currentValue << crc->properties.byteWidth);
  crc->currentValue &= crc->properties.valueMask;
}

void
crcAddData (CRCGenerator *crc, const void *data, size_t size) {
  const uint8_t *byte = data;
  const uint8_t *end = byte + size;
  while (byte < end) crcAddByte(crc, *byte++);
}

crc_t
crcGetValue (const CRCGenerator *crc) {
  return crc->currentValue;
}

crc_t
crcGetChecksum (const CRCGenerator *crc) {
  crc_t checksum = crc->currentValue;
  if (crc->parameters.reflectResult) crcReflectValue(crc, &checksum);
  checksum ^= crc->parameters.xorMask;
  return checksum;
}

crc_t
crcGetResidue (CRCGenerator *crc) {
  crc_t originalValue = crc->currentValue;
  crc_t checksum = crcGetChecksum(crc);

  unsigned int size = crc->parameters.checksumWidth / crc->properties.byteWidth;
  uint8_t data[size];

  if (crc->parameters.reflectResult) {
    uint8_t *byte = data;
    const uint8_t *end = byte + size;

    while (byte < end) {
      *byte++ = checksum;
      checksum >>= crc->properties.byteWidth;
    }
  } else {
    uint8_t *byte = data + size;

    while (byte-- > data) {
      *byte = checksum;
      checksum >>= crc->properties.byteWidth;
    }
  }

  crcAddData(crc, data, size);
  crc_t residue = crc->currentValue;
  if (crc->parameters.reflectResult) crcReflectValue(crc, &residue);

  crc->currentValue = originalValue;
  return residue;
}

void
crcResetGenerator (CRCGenerator *crc) {
  crc->currentValue = crc->parameters.initialValue;
}

CRCGenerator *
crcNewGenerator (const CRCAlgorithmParameters *parameters) {
  CRCGenerator *crc;
  const char *name = parameters->primaryName;
  size_t size = sizeof(*crc) + strlen(name) + 1;

  if ((crc = malloc(size))) {
    memset(crc, 0, size);

    crc->parameters = *parameters;
    strcpy(crc->algorithmName, name);
    crc->parameters.primaryName = crc->algorithmName;

    crc->properties.byteWidth = 8;
    crc->properties.byteShift = crc->parameters.checksumWidth - crc->properties.byteWidth;

    crc->properties.mostSignificantBit = crcMostSignificantBit(crc->parameters.checksumWidth);
    crc->properties.valueMask = (crc->properties.mostSignificantBit - 1) | crc->properties.mostSignificantBit;

    crcCacheRemainders(crc);
    crcResetGenerator(crc);
    return crc;
  } else {
    logMallocError();
  }

  return NULL;
}

void
crcDestroyGenerator (CRCGenerator *crc) {
  free(crc);
}

const CRCAlgorithmParameters *
crcGetParameters (const CRCGenerator *crc) {
  return &crc->parameters;
}

const CRCGeneratorProperties *
crcGetProperties (const CRCGenerator *crc) {
  return &crc->properties;
}
