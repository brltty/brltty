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
        remainder ^= crc->parameters.polynomialDivisor;
      } else {
        remainder <<= 1;
      }
    }

    // Store the result into the table.
    crc->properties.remainderCache[dividend] = remainder & crc->properties.remainderMask;
  }
}

void
crcAddByte (CRCGenerator *crc, uint8_t byte) {
  // Divide the byte by the polynomial.
  byte ^= crc->currentRemainder >> crc->properties.byteShift;
  crc->currentRemainder = crc->properties.remainderCache[byte] ^ (crc->currentRemainder << crc->properties.byteWidth);
  crc->currentRemainder &= crc->properties.remainderMask;
}

void
crcAddData (CRCGenerator *crc, const void *data, size_t size) {
  const uint8_t *byte = data;
  const uint8_t *end = byte + size;
  while (byte < end) crcAddByte(crc, *byte++);
}

crc_t
crcGetChecksum (const CRCGenerator *crc) {
  return crc->currentRemainder ^ crc->parameters.finalXorMask;
}

void
crcResetGenerator (CRCGenerator *crc) {
  crc->currentRemainder = crc->parameters.initialRemainder;
}

CRCGenerator *
crcNewGenerator (unsigned int width, crc_t polynomial, crc_t initial, crc_t xor) {
  CRCGenerator *crc;

  if ((crc = malloc(sizeof(*crc)))) {
    memset(crc, 0, sizeof(*crc));

    crc->parameters.checksumWidth = width;
    crc->parameters.polynomialDivisor = polynomial;
    crc->parameters.initialRemainder = initial;
    crc->parameters.finalXorMask = xor;

    crc->properties.byteWidth = 8;
    crc->properties.byteShift = crc->parameters.checksumWidth - crc->properties.byteWidth;
    crc->properties.mostSignificantBit = CRC_C(1) << (crc->parameters.checksumWidth - 1);
    crc->properties.remainderMask = (crc->properties.mostSignificantBit - 1) | crc->properties.mostSignificantBit;
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

const CRCParameters *
crcGetParameters (const CRCGenerator *crc) {
  return &crc->parameters;
}

const CRCProperties *
crcGetProperties (const CRCGenerator *crc) {
  return &crc->properties;
}

crc_t
crcGetRemainder (const CRCGenerator *crc) {
  return crc->currentRemainder;
}
