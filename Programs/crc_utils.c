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

const uint8_t crcCheckData[] = {
  0X31, 0X32, 0X33, 0X34, 0X35, 0X36, 0X37, 0X38, 0X39
}; const uint8_t crcCheckSize = sizeof(crcCheckData);

void
crcLogAlgorithmParameters (const CRCAlgorithmParameters *parameters) {
  logMessage(LOG_DEBUG,
    "CRC Parameters: %s: Width:%u Poly:%X Init:%X Xor:%X RefIn:%u RefOut:%u Chk:%X Res:%X",
    parameters->algorithmName,
    parameters->checksumWidth, parameters->generatorPolynomial,
    parameters->initialValue, parameters->xorMask,
    parameters->reflectInput, parameters->reflectResult,
    parameters->checkValue, parameters->residue
  );
}

void
crcLogGeneratorProperties (const CRCGenerator *crc) {
  const CRCGeneratorProperties *properties = crcGetGeneratorProperties(crc);

  logMessage(LOG_DEBUG,
    "CRC Properteis: %s: Byte:%u Shift:%u MSB:%X Mask:%X",
    crc->parameters.algorithmName,
    properties->byteWidth, properties->byteShift,
    properties->mostSignificantBit, properties->valueMask
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
crcVerifyResidue (CRCGenerator *crc, const char *label) {
  crc_t actual = crcGetResidue(crc);
  crc_t expected = crc->parameters.residue;
  int ok = actual == expected;

  if (!ok) {
    logMessage(LOG_WARNING,
      "CRC: residue mismatch: %s: Actual:%X Expected:%X",
      label, actual, expected
    );
  }

  return ok;
}

int
crcVerifyGeneratorWithData (
  CRCGenerator *crc, const char *label,
  const void *data, size_t size, crc_t expected
) {
  crcResetGenerator(crc);
  crcAddData(crc, data, size);

  if (!label) label = crc->parameters.algorithmName;
  int okChecksum = crcVerifyChecksum(crc, label, expected);
  int okResidue = crcVerifyResidue(crc, label);

  return okChecksum && okResidue;
}

int
crcVerifyGeneratorWithString (
  CRCGenerator *crc, const char *label,
  const char *string, crc_t expected
) {
  return crcVerifyGeneratorWithData(crc, label, string, strlen(string), expected);
}

int
crcVerifyAlgorithm (const CRCAlgorithmParameters *parameters) {
  CRCGenerator *crc = crcNewGenerator(parameters);

  int ok = crcVerifyGeneratorWithData(
    crc, NULL, crcCheckData, crcCheckSize, parameters->checkValue
  );

  crcDestroyGenerator(crc);
  return ok;
}

int
crcVerifyProvidedAlgorithms (void) {
  int ok = 1;
  const CRCAlgorithmParameters **parameters = crcProvidedAlgorithms;

  while (*parameters) {
    if (!crcVerifyAlgorithm(*parameters)) ok = 0;
    parameters += 1;
  }

  return ok;
}
