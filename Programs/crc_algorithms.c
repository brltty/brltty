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

#include "crc.h"

const CRCAlgorithmParameters crcAlgorithmParameters_CCITT_FALSE = {
  .algorithmName = "CRC-16/CCITT-FALSE",
  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_MAX,
  .checkValue = UINT16_C(0X29B1),
};

const CRCAlgorithmParameters crcAlgorithmParameters_HDLC = {
  .algorithmName = "CRC-16/ISO-HDLC",
  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,
  .checkValue = UINT16_C(0X906E),
  .residue = UINT16_C(0XF0B8),
};

const CRCAlgorithmParameters crcAlgorithmParameters_UMTS = {
  .algorithmName = "CRC-16/UMTS",
  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .checkValue = UINT16_C(0XFEE8),
};

const CRCAlgorithmParameters crcAlgorithmParameters_GSM = {
  .algorithmName = "CRC-16/GSM",
  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .xorMask = UINT16_MAX,
  .checkValue = UINT16_C(0XCE3C),
  .residue = UINT16_C(0X1D0F),
};

const CRCAlgorithmParameters *crcProvidedAlgorithms[] = {
  &crcAlgorithmParameters_CCITT_FALSE,
  &crcAlgorithmParameters_HDLC,
  &crcAlgorithmParameters_UMTS,
  &crcAlgorithmParameters_GSM,
  NULL
};
