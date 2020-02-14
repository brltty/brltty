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

// This source has been designed for and may be used within external code.
// It doesn't rely on our prologue header.
//#include "prologue.h"

#include <string.h>

#include "log.h"
#include "crc_internal.h"

const uint8_t crcCheckData[] = {
  0X31, 0X32, 0X33, 0X34, 0X35, 0X36, 0X37, 0X38, 0X39
};

const uint8_t crcCheckSize = sizeof(crcCheckData);

void
crcLogAlgorithm (const CRCAlgorithm *algorithm) {
  logMessage(LOG_DEBUG,
    "CRC Algorithm: %s: Width:%u Poly:%X Init:%X Xor:%X RefDat:%u RefRes:%u Chk:%X Res:%X",
    algorithm->primaryName,
    algorithm->checksumWidth, algorithm->generatorPolynomial,
    algorithm->initialValue, algorithm->xorMask,
    algorithm->reflectData, algorithm->reflectResult,
    algorithm->checkValue, algorithm->residue
  );
}

void
crcLogProperties (const CRCProperties *properties) {
  logMessage(LOG_DEBUG,
    "CRC Properteis: Shift:%u MSB:%X Mask:%X",
    properties->byteShift,
    properties->mostSignificantBit,
    properties->valueMask
  );
}

static void
crcLogMismatch (const CRCGenerator *crc,const char *what, crc_t actual, crc_t expected) {
  logMessage(LOG_WARNING,
    "CRC %s mismatch: %s: Actual:%X Expected:%X",
    what, crcGetAlgorithmName(crc), actual, expected
  );
}

int
crcVerifyChecksum (const CRCGenerator *crc, crc_t expected) {
  crc_t actual = crcGetChecksum(crc);
  int ok = actual == expected;
  if (!ok) crcLogMismatch(crc, "checksum", actual, expected);
  return ok;
}

int
crcVerifyResidue (CRCGenerator *crc) {
  crc_t actual = crcGetResidue(crc);
  crc_t expected = crc->algorithm.residue;
  int ok = actual == expected;
  if (!ok) crcLogMismatch(crc, "residue", actual, expected);
  return ok;
}

int
crcVerifyAlgorithmWithData (
  const CRCAlgorithm *algorithm,
  const void *data, size_t size, crc_t expected
) {
  CRCGenerator *crc = crcNewGenerator(algorithm);
  crcAddData(crc, data, size);

  int ok = crcVerifyChecksum(crc, expected);
  if (ok) ok = crcVerifyResidue(crc);

  crcDestroyGenerator(crc);
  return ok;
}

int
crcVerifyAlgorithmWithString (
  const CRCAlgorithm *algorithm,
  const char *string, crc_t expected
) {
  return crcVerifyAlgorithmWithData(algorithm, string, strlen(string), expected);
}

int
crcVerifyAlgorithm (const CRCAlgorithm *algorithm) {
  return crcVerifyAlgorithmWithData(
    algorithm, crcCheckData, crcCheckSize, algorithm->checkValue
  );
}

int
crcVerifyProvidedAlgorithms (void) {
  int ok = 1;
  const CRCAlgorithm **algorithm = crcProvidedAlgorithms;

  while (*algorithm) {
    if (!crcVerifyAlgorithm(*algorithm)) ok = 0;
    algorithm += 1;
  }

  return ok;
}
