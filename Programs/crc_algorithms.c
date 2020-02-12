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

#include "crc.h"

/* These CRC algorithm definitions have been copied from:
 * These CRC algorithm parameters have been copied from:
 * http://reveng.sourceforge.net/crc-catalogue/16.htm
 */

#define SECONDARY_NAMES(...) .secondaryNames = (const char *const []){__VA_ARGS__, NULL}

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_ARC = {
  .primaryName = "CRC-16/ARC",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("ARC", "CRC-16", "CRC-16/LHA", "CRC-IBM"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0XBB3D),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_CDMA2000 = {
  .primaryName = "CRC-16/CDMA2000",
  .algorithmClass = CRC_ALGORITHM_CLASS_ACADEMIC,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0XC867),
  .initialValue = UINT16_MAX,

  .checkValue = UINT16_C(0X4C06),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_CMS = {
  .primaryName = "CRC-16/CMS",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .initialValue = UINT16_MAX,

  .checkValue = UINT16_C(0XAEE7),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_DDS_110 = {
  .primaryName = "CRC-16/DDS-110",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .initialValue = UINT16_C(0X800D),

  .checkValue = UINT16_C(0X9ECF),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_DECT_R = {
  .primaryName = "CRC-16/DECT-R",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("R-CRC-16"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X0589),
  .xorMask = UINT16_C(0X0001),

  .checkValue = UINT16_C(0X007E),
  .residue = UINT16_C(0X0589),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_DECT_X = {
  .primaryName = "CRC-16/DECT-X",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("X-CRC-16"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X0589),

  .checkValue = UINT16_C(0X007F),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_DNP = {
  .primaryName = "CRC-16/DNP",
  .algorithmClass = CRC_ALGORITHM_CLASS_CONFIRMED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X3D65),
  .reflectInput = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XEA82),
  .residue = UINT16_C(0X66C5),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_EN_13757 = {
  .primaryName = "CRC-16/EN-13757",
  .algorithmClass = CRC_ALGORITHM_CLASS_CONFIRMED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X3D65),
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XC2B7),
  .residue = UINT16_C(0XA366),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_GENIBUS = {
  .primaryName = "CRC-16/GENIBUS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/DARC", "CRC-16/EPC", "CRC-16/EPC-C1G2", "CRC-16/I-CODE"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_MAX,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XD64E),
  .residue = UINT16_C(0X1D0F),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_GSM = {
  .primaryName = "CRC-16/GSM",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XCE3C),
  .residue = UINT16_C(0X1D0F),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_IBM_3740 = {
  .primaryName = "CRC-16/IBM-3740",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/AUTOSAR", "CRC-16/CCITT-FALSE"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_MAX,

  .checkValue = UINT16_C(0X29B1),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_IBM_SDLC = {
  .primaryName = "CRC-16/IBM-SDLC",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/ISO-HDLC", "CRC-16/ISO-IEC-14443-3-B", "CRC-16/X-25", "CRC-B", "X-25"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0X906E),
  .residue = UINT16_C(0XF0B8),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_ISO_IEC_14443_3_A = {
  .primaryName = "CRC-16/ISO-IEC-14443-3-A",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-A"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_C(0XC6C6),
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0XBF05),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_KERMIT = {
  .primaryName = "CRC-16/KERMIT",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/CCITT", "CRC-16/CCITT-TRUE", "CRC-16/V-41-LSB", "CRC-CCITT", "KERMIT"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0X2189),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_LJ1200 = {
  .primaryName = "CRC-16/LJ1200",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X6F63),

  .checkValue = UINT16_C(0XBDF4),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_MAXIM_DOW = {
  .primaryName = "CRC-16/MAXIM-DOW",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/MAXIM"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .reflectInput = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0X44C2),
  .residue = UINT16_C(0XB001),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_MCRF4XX = {
  .primaryName = "CRC-16/MCRF4XX",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0X6F91),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_MODBUS = {
  .primaryName = "CRC-16/MODBUS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("MODBUS"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0X4B37),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_NRSC_5 = {
  .primaryName = "CRC-16/NRSC-5",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X080B),
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0XA066),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_OPENSAFETY_A = {
  .primaryName = "CRC-16/OPENSAFETY-A",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X5935),

  .checkValue = UINT16_C(0X5D38),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_OPENSAFETY_B = {
  .primaryName = "CRC-16/OPENSAFETY-B",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X755B),

  .checkValue = UINT16_C(0X20FE),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_PROFIBUS = {
  .primaryName = "CRC-16/PROFIBUS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/IEC-61158-2"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1DCF),
  .initialValue = UINT16_MAX,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XA819),
  .residue = UINT16_C(0XE394),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_RIELLO = {
  .primaryName = "CRC-16/RIELLO",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_C(0XB2AA),
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0X63D0),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_SPI_FUJITSU = {
  .primaryName = "CRC-16/SPI-FUJITSU",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/AUG-CCITT"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_C(0X1D0F),

  .checkValue = UINT16_C(0XE5CC),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_T10_DIF = {
  .primaryName = "CRC-16/T10-DIF",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8BB7),

  .checkValue = UINT16_C(0XD0DB),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_TELEDISK = {
  .primaryName = "CRC-16/TELEDISK",
  .algorithmClass = CRC_ALGORITHM_CLASS_CONFIRMED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0XA097),

  .checkValue = UINT16_C(0X0FB3),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_TMS37157 = {
  .primaryName = "CRC-16/TMS37157",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_C(0X89EC),
  .reflectInput = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0X26B1),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_UMTS = {
  .primaryName = "CRC-16/UMTS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/BUYPASS", "CRC-16/VERIFONE"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),

  .checkValue = UINT16_C(0XFEE8),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_USB = {
  .primaryName = "CRC-16/USB",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .initialValue = UINT16_MAX,
  .reflectInput = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XB4C8),
  .residue = UINT16_C(0XB001),
};

static const CRCAlgorithmParameters crcAlgorithmParameters_CRC16_XMODEM = {
  .primaryName = "CRC-16/XMODEM",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  SECONDARY_NAMES("CRC-16/ACORN", "CRC-16/LTE", "CRC-16/V-41-MSB", "XMODEM", "ZMODEM"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),

  .checkValue = UINT16_C(0X31C3),
};

const CRCAlgorithmParameters *crcProvidedAlgorithms[] = {
  &crcAlgorithmParameters_CRC16_ARC,
  &crcAlgorithmParameters_CRC16_CDMA2000,
  &crcAlgorithmParameters_CRC16_CMS,
  &crcAlgorithmParameters_CRC16_DDS_110,
  &crcAlgorithmParameters_CRC16_DECT_R,
  &crcAlgorithmParameters_CRC16_DECT_X,
  &crcAlgorithmParameters_CRC16_DNP,
  &crcAlgorithmParameters_CRC16_EN_13757,
  &crcAlgorithmParameters_CRC16_GENIBUS,
  &crcAlgorithmParameters_CRC16_GSM,
  &crcAlgorithmParameters_CRC16_IBM_3740,
  &crcAlgorithmParameters_CRC16_IBM_SDLC,
  &crcAlgorithmParameters_CRC16_ISO_IEC_14443_3_A,
  &crcAlgorithmParameters_CRC16_KERMIT,
  &crcAlgorithmParameters_CRC16_LJ1200,
  &crcAlgorithmParameters_CRC16_MAXIM_DOW,
  &crcAlgorithmParameters_CRC16_MCRF4XX,
  &crcAlgorithmParameters_CRC16_MODBUS,
  &crcAlgorithmParameters_CRC16_NRSC_5,
  &crcAlgorithmParameters_CRC16_OPENSAFETY_A,
  &crcAlgorithmParameters_CRC16_OPENSAFETY_B,
  &crcAlgorithmParameters_CRC16_PROFIBUS,
  &crcAlgorithmParameters_CRC16_RIELLO,
  &crcAlgorithmParameters_CRC16_SPI_FUJITSU,
  &crcAlgorithmParameters_CRC16_T10_DIF,
  &crcAlgorithmParameters_CRC16_TELEDISK,
  &crcAlgorithmParameters_CRC16_TMS37157,
  &crcAlgorithmParameters_CRC16_UMTS,
  &crcAlgorithmParameters_CRC16_USB,
  &crcAlgorithmParameters_CRC16_XMODEM,
  NULL
};

const CRCAlgorithmParameters *
crcGetAlgorithm (const char *name) {
  const CRCAlgorithmParameters **parameters = crcProvidedAlgorithms;

  while (*parameters) {
    if (strcmp(name, (*parameters)->primaryName) == 0) return *parameters;
    const char *const *alias = (*parameters)->secondaryNames;

    if (alias) {
      while (*alias) {
        if (strcmp(name, *alias) == 0) return *parameters;
        alias += 1;
      }
    }

    parameters += 1;
  }

  return NULL;
}
