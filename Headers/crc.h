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

#ifndef BRLTTY_INCLUDED_CRC
#define BRLTTY_INCLUDED_CRC

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef uint32_t crc_t;
#define CRC_C UINT32_C
typedef struct CRCGeneratorStruct CRCGenerator;

extern CRCGenerator *crcNewGenerator (
  unsigned int width, crc_t polynomial, crc_t initial, crc_t xor
);

extern void crcResetGenerator (CRCGenerator *crc);
extern void crcDestroyGenerator (CRCGenerator *crc);

extern crc_t crcGetChecksum (const CRCGenerator *crc);
extern void crcAddByte (CRCGenerator *crc, uint8_t byte);
extern void crcAddData (CRCGenerator *crc, const void *data, size_t size);

typedef struct {
  unsigned int checksumWidth;
  crc_t polynomialDivisor;
  crc_t initialRemainder;
  crc_t finalXorMask;
} CRCParameters;

typedef struct {
  unsigned int byteWidth;
  unsigned int byteShift;
  crc_t mostSignificantBit;
  crc_t remainderMask;
  crc_t remainderCache[UINT8_MAX + 1];
} CRCProperties;

extern const CRCParameters *crcGetParameters (const CRCGenerator *crc);
extern const CRCProperties *crcGetProperties (const CRCGenerator *crc);
extern crc_t crcGetRemainder (const CRCGenerator *crc);

#define CRC_CCITT_ALGORITHM_NAME "CRC-CCITT"
#define CRC_CCITT_CHECKSUM_WIDTH 16
#define CRC_CCITT_POLYNOMIAL_DIVISOR UINT16_C(0X1021)
#define CRC_CCITT_INITIAL_REMAINDER UINT16_MAX
#define CRC_CCITT_FINAL_XOR 0

static inline CRCGenerator *
crcNewGenerator_CCITT (void) {
  return crcNewGenerator(
    CRC_CCITT_CHECKSUM_WIDTH,
    CRC_CCITT_POLYNOMIAL_DIVISOR,
    CRC_CCITT_INITIAL_REMAINDER,
    CRC_CCITT_FINAL_XOR
  );
}

#define CRC_CRC16_ALGORITHM_NAME "CRC-16"
#define CRC_CRC16_CHECKSUM_WIDTH 16
#define CRC_CRC16_POLYNOMIAL_DIVISOR UINT16_C(0X8005)
#define CRC_CRC16_INITIAL_REMAINDER 0
#define CRC_CRC16_FINAL_XOR 0

static inline CRCGenerator *
crcNewGenerator_CRC16 (void) {
  return crcNewGenerator(
    CRC_CRC16_CHECKSUM_WIDTH,
    CRC_CRC16_POLYNOMIAL_DIVISOR,
    CRC_CRC16_INITIAL_REMAINDER,
    CRC_CRC16_FINAL_XOR
  );
}

#define CRC_CRC32_ALGORITHM_NAME "CRC-32"
#define CRC_CRC32_CHECKSUM_WIDTH 32
#define CRC_CRC32_POLYNOMIAL_DIVISOR UINT32_C(0X04C11DB7)
#define CRC_CRC32_INITIAL_REMAINDER UINT32_MAX
#define CRC_CRC32_FINAL_XOR UINT32_MAX

static inline CRCGenerator *
crcNewGenerator_CRC32 (void) {
  return crcNewGenerator(
    CRC_CRC32_CHECKSUM_WIDTH,
    CRC_CRC32_POLYNOMIAL_DIVISOR,
    CRC_CRC32_INITIAL_REMAINDER,
    CRC_CRC32_FINAL_XOR
  );
}

extern void crcLogParameters (const CRCGenerator *crc, const char *label);
extern void crcLogProperties (const CRCGenerator *crc, const char *label);

extern int crcVerifyChecksum (CRCGenerator *crc, const char *label, crc_t expected);
extern void crcVerifyProvidedGenerators (void);

extern int crcVerifyGeneratorWithData (
  CRCGenerator *crc, const char *label,
  const void *data, size_t size, crc_t expected
);

extern int crcVerifyGeneratorWithString (
  CRCGenerator *crc, const char *label,
  const char *string, crc_t expected
);

extern int crcVerifyAlgorithm (
  const char *label, const void *data, size_t size,
  unsigned int width, crc_t polynomial, crc_t initial, crc_t xor,
  crc_t expected
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CRC */
