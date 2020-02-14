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

#include "crc.h"

#define CRC_ALGORITHM_NAME(name) crcAlgorithm_ ## name
#define CRC_ALGORITHM_DEFINITION(name) static const CRCAlgorithm CRC_ALGORITHM_NAME(name)
#define CRC_SECONDARY_NAMES(...) .secondaryNames = (const char *const []){__VA_ARGS__, NULL}

/*
 * These CRC algorithms have been copied from:
 * http://reveng.sourceforge.net/crc-catalogue/16.htm
 */

CRC_ALGORITHM_DEFINITION(CRC16_ARC) = {
  .primaryName = "CRC-16/ARC",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("ARC", "CRC-16", "CRC-16/LHA", "CRC-IBM"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .reflectData = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0XBB3D),
};

CRC_ALGORITHM_DEFINITION(CRC16_CDMA2000) = {
  .primaryName = "CRC-16/CDMA2000",
  .algorithmClass = CRC_ALGORITHM_CLASS_ACADEMIC,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0XC867),
  .initialValue = UINT16_MAX,

  .checkValue = UINT16_C(0X4C06),
};

CRC_ALGORITHM_DEFINITION(CRC16_CMS) = {
  .primaryName = "CRC-16/CMS",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .initialValue = UINT16_MAX,

  .checkValue = UINT16_C(0XAEE7),
};

CRC_ALGORITHM_DEFINITION(CRC16_DDS_110) = {
  .primaryName = "CRC-16/DDS-110",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .initialValue = UINT16_C(0X800D),

  .checkValue = UINT16_C(0X9ECF),
};

CRC_ALGORITHM_DEFINITION(CRC16_DECT_R) = {
  .primaryName = "CRC-16/DECT-R",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("R-CRC-16"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X0589),
  .xorMask = UINT16_C(0X0001),

  .checkValue = UINT16_C(0X007E),
  .residue = UINT16_C(0X0589),
};

CRC_ALGORITHM_DEFINITION(CRC16_DECT_X) = {
  .primaryName = "CRC-16/DECT-X",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("X-CRC-16"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X0589),

  .checkValue = UINT16_C(0X007F),
};

CRC_ALGORITHM_DEFINITION(CRC16_DNP) = {
  .primaryName = "CRC-16/DNP",
  .algorithmClass = CRC_ALGORITHM_CLASS_CONFIRMED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X3D65),
  .reflectData = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XEA82),
  .residue = UINT16_C(0X66C5),
};

CRC_ALGORITHM_DEFINITION(CRC16_EN_13757) = {
  .primaryName = "CRC-16/EN-13757",
  .algorithmClass = CRC_ALGORITHM_CLASS_CONFIRMED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X3D65),
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XC2B7),
  .residue = UINT16_C(0XA366),
};

CRC_ALGORITHM_DEFINITION(CRC16_GENIBUS) = {
  .primaryName = "CRC-16/GENIBUS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("CRC-16/DARC", "CRC-16/EPC", "CRC-16/EPC-C1G2", "CRC-16/I-CODE"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_MAX,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XD64E),
  .residue = UINT16_C(0X1D0F),
};

CRC_ALGORITHM_DEFINITION(CRC16_GSM) = {
  .primaryName = "CRC-16/GSM",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XCE3C),
  .residue = UINT16_C(0X1D0F),
};

CRC_ALGORITHM_DEFINITION(CRC16_IBM_3740) = {
  .primaryName = "CRC-16/IBM-3740",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("CRC-16/AUTOSAR", "CRC-16/CCITT-FALSE"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_MAX,

  .checkValue = UINT16_C(0X29B1),
};

CRC_ALGORITHM_DEFINITION(CRC16_IBM_SDLC) = {
  .primaryName = "CRC-16/IBM-SDLC",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("CRC-16/ISO-HDLC", "CRC-16/ISO-IEC-14443-3-B", "CRC-16/X-25", "CRC-B", "X-25"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_MAX,
  .reflectData = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0X906E),
  .residue = UINT16_C(0XF0B8),
};

CRC_ALGORITHM_DEFINITION(CRC16_ISO_IEC_14443_3_A) = {
  .primaryName = "CRC-16/ISO-IEC-14443-3-A",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("CRC-A"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_C(0XC6C6),
  .reflectData = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0XBF05),
};

CRC_ALGORITHM_DEFINITION(CRC16_KERMIT) = {
  .primaryName = "CRC-16/KERMIT",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("CRC-16/CCITT", "CRC-16/CCITT-TRUE", "CRC-16/V-41-LSB", "CRC-CCITT", "KERMIT"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .reflectData = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0X2189),
};

CRC_ALGORITHM_DEFINITION(CRC16_LJ1200) = {
  .primaryName = "CRC-16/LJ1200",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X6F63),

  .checkValue = UINT16_C(0XBDF4),
};

CRC_ALGORITHM_DEFINITION(CRC16_MAXIM_DOW) = {
  .primaryName = "CRC-16/MAXIM-DOW",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("CRC-16/MAXIM"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .reflectData = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0X44C2),
  .residue = UINT16_C(0XB001),
};

CRC_ALGORITHM_DEFINITION(CRC16_MCRF4XX) = {
  .primaryName = "CRC-16/MCRF4XX",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_MAX,
  .reflectData = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0X6F91),
};

CRC_ALGORITHM_DEFINITION(CRC16_MODBUS) = {
  .primaryName = "CRC-16/MODBUS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("MODBUS"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .initialValue = UINT16_MAX,
  .reflectData = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0X4B37),
};

CRC_ALGORITHM_DEFINITION(CRC16_NRSC_5) = {
  .primaryName = "CRC-16/NRSC-5",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X080B),
  .initialValue = UINT16_MAX,
  .reflectData = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0XA066),
};

CRC_ALGORITHM_DEFINITION(CRC16_OPENSAFETY_A) = {
  .primaryName = "CRC-16/OPENSAFETY-A",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X5935),

  .checkValue = UINT16_C(0X5D38),
};

CRC_ALGORITHM_DEFINITION(CRC16_OPENSAFETY_B) = {
  .primaryName = "CRC-16/OPENSAFETY-B",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X755B),

  .checkValue = UINT16_C(0X20FE),
};

CRC_ALGORITHM_DEFINITION(CRC16_PROFIBUS) = {
  .primaryName = "CRC-16/PROFIBUS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("CRC-16/IEC-61158-2"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1DCF),
  .initialValue = UINT16_MAX,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XA819),
  .residue = UINT16_C(0XE394),
};

CRC_ALGORITHM_DEFINITION(CRC16_RIELLO) = {
  .primaryName = "CRC-16/RIELLO",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_C(0XB2AA),
  .reflectData = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0X63D0),
};

CRC_ALGORITHM_DEFINITION(CRC16_SPI_FUJITSU) = {
  .primaryName = "CRC-16/SPI-FUJITSU",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("CRC-16/AUG-CCITT"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_C(0X1D0F),

  .checkValue = UINT16_C(0XE5CC),
};

CRC_ALGORITHM_DEFINITION(CRC16_T10_DIF) = {
  .primaryName = "CRC-16/T10-DIF",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8BB7),

  .checkValue = UINT16_C(0XD0DB),
};

CRC_ALGORITHM_DEFINITION(CRC16_TELEDISK) = {
  .primaryName = "CRC-16/TELEDISK",
  .algorithmClass = CRC_ALGORITHM_CLASS_CONFIRMED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0XA097),

  .checkValue = UINT16_C(0X0FB3),
};

CRC_ALGORITHM_DEFINITION(CRC16_TMS37157) = {
  .primaryName = "CRC-16/TMS37157",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),
  .initialValue = UINT16_C(0X89EC),
  .reflectData = 1,
  .reflectResult = 1,

  .checkValue = UINT16_C(0X26B1),
};

CRC_ALGORITHM_DEFINITION(CRC16_UMTS) = {
  .primaryName = "CRC-16/UMTS",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("CRC-16/BUYPASS", "CRC-16/VERIFONE"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),

  .checkValue = UINT16_C(0XFEE8),
};

CRC_ALGORITHM_DEFINITION(CRC16_USB) = {
  .primaryName = "CRC-16/USB",
  .algorithmClass = CRC_ALGORITHM_CLASS_THIRD_PARTY,

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X8005),
  .initialValue = UINT16_MAX,
  .reflectData = 1,
  .reflectResult = 1,
  .xorMask = UINT16_MAX,

  .checkValue = UINT16_C(0XB4C8),
  .residue = UINT16_C(0XB001),
};

CRC_ALGORITHM_DEFINITION(CRC16_XMODEM) = {
  .primaryName = "CRC-16/XMODEM",
  .algorithmClass = CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_SECONDARY_NAMES("CRC-16/ACORN", "CRC-16/LTE", "CRC-16/V-41-MSB", "XMODEM", "ZMODEM"),

  .checksumWidth = 16,
  .generatorPolynomial = UINT16_C(0X1021),

  .checkValue = UINT16_C(0X31C3),
};

const CRCAlgorithm *crcProvidedAlgorithms[] = {
  &CRC_ALGORITHM_NAME(CRC16_ARC),
  &CRC_ALGORITHM_NAME(CRC16_CDMA2000),
  &CRC_ALGORITHM_NAME(CRC16_CMS),
  &CRC_ALGORITHM_NAME(CRC16_DDS_110),
  &CRC_ALGORITHM_NAME(CRC16_DECT_R),
  &CRC_ALGORITHM_NAME(CRC16_DECT_X),
  &CRC_ALGORITHM_NAME(CRC16_DNP),
  &CRC_ALGORITHM_NAME(CRC16_EN_13757),
  &CRC_ALGORITHM_NAME(CRC16_GENIBUS),
  &CRC_ALGORITHM_NAME(CRC16_GSM),
  &CRC_ALGORITHM_NAME(CRC16_IBM_3740),
  &CRC_ALGORITHM_NAME(CRC16_IBM_SDLC),
  &CRC_ALGORITHM_NAME(CRC16_ISO_IEC_14443_3_A),
  &CRC_ALGORITHM_NAME(CRC16_KERMIT),
  &CRC_ALGORITHM_NAME(CRC16_LJ1200),
  &CRC_ALGORITHM_NAME(CRC16_MAXIM_DOW),
  &CRC_ALGORITHM_NAME(CRC16_MCRF4XX),
  &CRC_ALGORITHM_NAME(CRC16_MODBUS),
  &CRC_ALGORITHM_NAME(CRC16_NRSC_5),
  &CRC_ALGORITHM_NAME(CRC16_OPENSAFETY_A),
  &CRC_ALGORITHM_NAME(CRC16_OPENSAFETY_B),
  &CRC_ALGORITHM_NAME(CRC16_PROFIBUS),
  &CRC_ALGORITHM_NAME(CRC16_RIELLO),
  &CRC_ALGORITHM_NAME(CRC16_SPI_FUJITSU),
  &CRC_ALGORITHM_NAME(CRC16_T10_DIF),
  &CRC_ALGORITHM_NAME(CRC16_TELEDISK),
  &CRC_ALGORITHM_NAME(CRC16_TMS37157),
  &CRC_ALGORITHM_NAME(CRC16_UMTS),
  &CRC_ALGORITHM_NAME(CRC16_USB),
  &CRC_ALGORITHM_NAME(CRC16_XMODEM),
  NULL
};

const CRCAlgorithm *
crcGetProvidedAlgorithm (const char *name) {
  const CRCAlgorithm **algorithm = crcProvidedAlgorithms;

  while (*algorithm) {
    if (strcmp(name, (*algorithm)->primaryName) == 0) return *algorithm;
    const char *const *alias = (*algorithm)->secondaryNames;

    if (alias) {
      while (*alias) {
        if (strcmp(name, *alias) == 0) return *algorithm;
        alias += 1;
      }
    }

    algorithm += 1;
  }

  return NULL;
}
