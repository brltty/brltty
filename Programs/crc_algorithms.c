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

#define SECONDARY_NAMES(...) .secondaryNames = (const char *const []){__VA_ARGS__, NULL}

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

const CRCAlgorithmParameters crcAlgorithmParameters_x00 = {
  .algorithmName = "CRC-16/ARC",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("ARC", "CRC-16", "CRC-16/LHA", "CRC-IBM"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x8005,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = 0xbb3d,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x01 = {
  .algorithmName = "CRC-16/CDMA2000",
  .algorithmClass = CRC_ALGORITHM_CLASS_ACADEMIC,

  .checksumWidth = 16,
  .generatorPolynomial = 0xc867,
  .initialValue = UINT16_MAX,

  .checkValue = 0x4c06,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x02 = {
  .algorithmName = "CRC-16/CMS",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = 0x8005,
  .initialValue = UINT16_MAX,

  .checkValue = 0xaee7,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x03 = {
  .algorithmName = "CRC-16/DDS-110",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = 0x8005,
  .initialValue = 0x800d,

  .checkValue = 0x9ecf,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x04 = {
  .algorithmName = "CRC-16/DECT-R",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("R-CRC-16"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x0589,
  .xorMask = 0x0001,

  .checkValue = 0x007e,
  .residue = 0x0589,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x05 = {
  .algorithmName = "CRC-16/DECT-X",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("X-CRC-16"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x0589,

  .checkValue = 0x007f,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x06 = {
  .algorithmName = "CRC-16/DNP",
  .algorithmClass = CRC_ALGORITHM_CLASS_CONFIRMED,

  .checksumWidth = 16,
  .generatorPolynomial = 0x3d65,
  .reflectInput = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = 0xea82,
  .residue = 0x66c5,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x07 = {
  .algorithmName = "CRC-16/EN-13757",
  .algorithmClass = CRC_ALGORITHM_CLASS_CONFIRMED,

  .checksumWidth = 16,
  .generatorPolynomial = 0x3d65,
  .xorMask = UINT16_MAX,

  .checkValue = 0xc2b7,
  .residue = 0xa366,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x08 = {
  .algorithmName = "CRC-16/GENIBUS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/DARC", "CRC-16/EPC", "CRC-16/EPC-C1G2", "CRC-16/I-CODE"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,
  .initialValue = UINT16_MAX,
  .xorMask = UINT16_MAX,

  .checkValue = 0xd64e,
  .residue = 0x1d0f,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x09 = {
  .algorithmName = "CRC-16/GSM",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,
  .xorMask = UINT16_MAX,

  .checkValue = 0xce3c,
  .residue = 0x1d0f,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x10 = {
  .algorithmName = "CRC-16/IBM-3740",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/AUTOSAR", "CRC-16/CCITT-FALSE"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,
  .initialValue = UINT16_MAX,

  .checkValue = 0x29b1,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x11 = {
  .algorithmName = "CRC-16/IBM-SDLC",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/ISO-HDLC", "CRC-16/ISO-IEC-14443-3-B", "CRC-16/X-25", "CRC-B", "X-25"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = 0x906e,
  .residue = 0xf0b8,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x12 = {
  .algorithmName = "CRC-16/ISO-IEC-14443-3-A",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-A"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,
  .initialValue = 0xc6c6,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = 0xbf05,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x13 = {
  .algorithmName = "CRC-16/KERMIT",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/CCITT", "CRC-16/CCITT-TRUE", "CRC-16/V-41-LSB", "CRC-CCITT", "KERMIT"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = 0x2189,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x14 = {
  .algorithmName = "CRC-16/LJ1200",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = 0x6f63,

  .checkValue = 0xbdf4,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x15 = {
  .algorithmName = "CRC-16/MAXIM-DOW",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/MAXIM"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x8005,
  .reflectInput = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = 0x44c2,
  .residue = 0xb001,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x16 = {
  .algorithmName = "CRC-16/MCRF4XX",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = 0x6f91,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x17 = {
  .algorithmName = "CRC-16/MODBUS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("MODBUS"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x8005,
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = 0x4b37,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x18 = {
  .algorithmName = "CRC-16/NRSC-5",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = 0x080b,
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = 0xa066,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x19 = {
  .algorithmName = "CRC-16/OPENSAFETY-A",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = 0x5935,

  .checkValue = 0x5d38,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x20 = {
  .algorithmName = "CRC-16/OPENSAFETY-B",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = 0x755b,

  .checkValue = 0x20fe,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x21 = {
  .algorithmName = "CRC-16/PROFIBUS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/IEC-61158-2"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x1dcf,
  .initialValue = UINT16_MAX,
  .xorMask = UINT16_MAX,

  .checkValue = 0xa819,
  .residue = 0xe394,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x22 = {
  .algorithmName = "CRC-16/RIELLO",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,
  .initialValue = 0xb2aa,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = 0x63d0,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x23 = {
  .algorithmName = "CRC-16/SPI-FUJITSU",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/AUG-CCITT"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,
  .initialValue = 0x1d0f,

  .checkValue = 0xe5cc,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x24 = {
  .algorithmName = "CRC-16/T10-DIF",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = 0x8bb7,

  .checkValue = 0xd0db,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x25 = {
  .algorithmName = "CRC-16/TELEDISK",
  .algorithmClass = CRC_ALGORITHM_CLASS_CONFIRMED,

  .checksumWidth = 16,
  .generatorPolynomial = 0xa097,

  .checkValue = 0x0fb3,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x26 = {
  .algorithmName = "CRC-16/TMS37157",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,
  .initialValue = 0x89ec,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = 0x26b1,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x27 = {
  .algorithmName = "CRC-16/UMTS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/BUYPASS", "CRC-16/VERIFONE"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x8005,

  .checkValue = 0xfee8,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x28 = {
  .algorithmName = "CRC-16/USB",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = 0x8005,
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = 0xb4c8,
  .residue = 0xb001,
};

const CRCAlgorithmParameters crcAlgorithmParameters_x29 = {
  .algorithmName = "CRC-16/XMODEM",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/ACORN", "CRC-16/LTE", "CRC-16/V-41-MSB", "XMODEM", "ZMODEM"),

  .checksumWidth = 16,
  .generatorPolynomial = 0x1021,

  .checkValue = 0x31c3,
};

const CRCAlgorithmParameters *crcProvidedAlgorithms[] = {
  &crcAlgorithmParameters_CCITT_FALSE,
  &crcAlgorithmParameters_HDLC,
  &crcAlgorithmParameters_UMTS,
  &crcAlgorithmParameters_GSM,
  NULL
};
