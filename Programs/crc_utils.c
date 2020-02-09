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

void
crcLogParameters (const CRCGenerator *crc, const char *label) {
  logMessage(LOG_INFO,
    "CRC parameters: %s: Width:%u Polynomial:%X Initial:%X Xor:%X", label,
    crc->parameters.checksumWidth, crc->parameters.polynomialDivisor,
    crc->parameters.initialRemainder, crc->parameters.finalXorMask
  );
}

void
crcLogProperties (const CRCGenerator *crc, const char *label) {
  logMessage(LOG_INFO,
    "CRC properteis: %s: Shift:%u MSB:%X Mask:%X", label,
    crc->properties.byteShift,
    crc->properties.mostSignificantBit,
    crc->properties.remainderMask
  );
}

int
crcVerifyChecksum (CRCGenerator *crc, const char *label, crc_t expected) {
  crc_t actual = crcGetChecksum(crc);
  int ok = actual == expected;

  if (!ok) {
    logMessage(LOG_WARNING,
      "CRC: checksum mismatch: %s: Actual:%X Expected:%X",
      label, actual, expected
    );
  }

  return ok;
}

int
crcVerifyGeneratorWithData (CRCGenerator *crc, const char *label, const void *data, size_t size, crc_t expected) {
  crcResetGenerator(crc);
  crcAddData(crc, data, size);
  return crcVerifyChecksum(crc, label, expected);
}

int
crcVerifyGeneratorWithString (CRCGenerator *crc, const char *label, const char *string, crc_t expected) {
  return crcVerifyGeneratorWithData(crc, label, string, strlen(string), expected);
}

static int
crcVerifyAndDestroyGenerator (CRCGenerator *crc, const char *label, const void *data, size_t size, crc_t expected) {
  int ok = crcVerifyGeneratorWithData(crc, label, data, size, expected);
  crcDestroyGenerator(crc);
  return ok;
}

void
crcVerifyProvidedGenerators (void) {
  static const char data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  static const size_t size = sizeof(data);

  crcVerifyAndDestroyGenerator(
    crcNewGenerator_CCITT(), CRC_CCITT_ALGORITHM_NAME,
    data, size, UINT16_C(0X29B1)
  );

  crcVerifyAndDestroyGenerator(
    crcNewGenerator_CRC16(), CRC_CRC16_ALGORITHM_NAME,
    data, size, UINT16_C(0XFEE8)
  );

  crcVerifyAndDestroyGenerator(
    crcNewGenerator_CRC32(), CRC_CRC32_ALGORITHM_NAME,
    data, size, UINT32_C(0XFC891918)
  );
}

int
crcVerifyAlgorithm (
  const char *label, const void *data, size_t size,
  unsigned int width, crc_t polynomial, crc_t initial, crc_t xor,
  crc_t expected
) {
  CRCGenerator *crc = crcNewGenerator(width, polynomial, initial, xor);
  return crcVerifyAndDestroyGenerator(crc, label, data, size, expected);
}
