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

#define CRC_BYTE_WIDTH 8
#define CRC_BYTE_INDEXED_TABLE_SIZE (UINT8_MAX + 1)

static inline crc_t
crcMostSignificantBit (unsigned int width) {
  return CRC_C(1) << (width - 1);
}

extern crc_t crcReflectBits (crc_t fromValue, unsigned int width);
extern void crcReflectByte (uint8_t *byte);

typedef enum {
  CRC_ALGORITHM_CLASS_UNKNOWN,
  CRC_ALGORITHM_CLASS_CONFIRMED,
  CRC_ALGORITHM_CLASS_ATTESTED,
  CRC_ALGORITHM_CLASS_ACADEMIC,
  CRC_ALGORITHM_CLASS_THIRD_PARTY,
} CRCAlgorithmClass;

typedef struct {
  const char *primaryName; // the official name of the algorithm
  const char *const *secondaryNames; // other names that the algorithm is known by (NULL terminated)
  CRCAlgorithmClass algorithmClass;

  unsigned int checksumWidth; // the width of the checksum (in bits)
  crc_t generatorPolynomial; // the polynomial that generates the checksum

  crc_t initialValue; // the starting value (before any processing)
  crc_t xorMask; // the xor (exclussive or) mask to apply to the final value

  unsigned reflectInput:1; // reflect each input byte before processing it
  unsigned reflectResult:1; // reflect the final value (before the xor)

  crc_t checkValue; // the checksum for the official check data ("123456789")
  crc_t residue; // the final value (no reflection or xor) of the check data
                 // followed by its checksum (in network byte order)
} CRCAlgorithm;

extern const CRCAlgorithm *crcProvidedAlgorithms[];
extern const CRCAlgorithm *crcGetProvidedAlgorithm (const char *name);
extern void crcReflectValue (crc_t *value, const CRCAlgorithm *algorithm);

typedef struct {
  unsigned int byteShift; // the bit offset of the high-order byte of the value
  crc_t mostSignificantBit; // the most significant bit of the value
  crc_t valueMask; // the mask for removing overflow bits in the value
  const uint8_t *inputTranslationTable; // for optimizing input reflection

  // for preevaluating a common calculation on each input byte
  crc_t remainderCache[CRC_BYTE_INDEXED_TABLE_SIZE];
} CRCProperties;

typedef struct CRCGeneratorStruct CRCGenerator;
extern CRCGenerator *crcNewGenerator (const CRCAlgorithm *algorithm);
extern void crcResetGenerator (CRCGenerator *crc);
extern void crcDestroyGenerator (CRCGenerator *crc);

extern void crcAddByte (CRCGenerator *crc, uint8_t byte);
extern void crcAddData (CRCGenerator *crc, const void *data, size_t size);

extern crc_t crcGetChecksum (const CRCGenerator *crc);
extern crc_t crcGetResidue (CRCGenerator *crc);

extern const CRCAlgorithm *crcGetAlgorithm (const CRCGenerator *crc);
extern const CRCProperties *crcGetProperties (const CRCGenerator *crc);
extern crc_t crcGetValue (const CRCGenerator *crc);

extern void crcLogAlgorithm (const CRCAlgorithm *algorithm);
extern void crcLogProperties (const CRCProperties *properties);

extern const uint8_t crcCheckData[];
extern const uint8_t crcCheckSize;

extern int crcVerifyChecksum (const CRCGenerator *crc, crc_t expected);
extern int crcVerifyResidue (CRCGenerator *crc);

extern int crcVerifyAlgorithm (const CRCAlgorithm *algorithm);
extern int crcVerifyProvidedAlgorithms (void);

extern int crcVerifyAlgorithmWithData (
  const CRCAlgorithm *algorithm,
  const void *data, size_t size, crc_t expected
);

extern int crcVerifyAlgorithmWithString (
  const CRCAlgorithm *algorithm,
  const char *string, crc_t expected
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CRC */
