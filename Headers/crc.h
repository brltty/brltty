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

typedef struct {
  const char *algorithmName; // the name of the algorithm
  unsigned int checksumWidth; // the width of the checksum (in bits)
  crc_t generatorPolynomial;

  crc_t initialValue; // the starting value (before any processing)
  crc_t xorMask; // the xor (exclussive or) mask to apply to the final value

  unsigned reflectInput:1; // reflect each input byte before processing it
  unsigned reflectResult:1; // reflect the final value (before xor)

  crc_t checkValue; // the checksum for the official check data ("123456789")
  crc_t residue; // the final value (no reflection or xor) of the check data
                 // followed by its checksum (in network byte order)
} CRCAlgorithmParameters;

extern const CRCAlgorithmParameters *crcProvidedAlgorithms[];
extern const CRCAlgorithmParameters crcAlgorithmParameters_CCITT_FALSE;
extern const CRCAlgorithmParameters crcAlgorithmParameters_HDLC;
extern const CRCAlgorithmParameters crcAlgorithmParameters_UMTS;
extern const CRCAlgorithmParameters crcAlgorithmParameters_GSM;

typedef struct {
  unsigned int byteWidth; // the width of a byte (in bits)
  unsigned int byteShift; // the bit offset of the high-order byte of the value
  crc_t mostSignificantBit; // the most significant bit of the value
  crc_t valueMask; // the mask for removing overflow bits in the value
  crc_t remainderCache[UINT8_MAX + 1]; // for precalculating a common
                                       // calculation for each input byte
} CRCGeneratorProperties;

extern CRCGenerator *crcNewGenerator (const CRCAlgorithmParameters *parameters);
extern void crcResetGenerator (CRCGenerator *crc);
extern void crcDestroyGenerator (CRCGenerator *crc);

extern void crcAddByte (CRCGenerator *crc, uint8_t byte);
extern void crcAddData (CRCGenerator *crc, const void *data, size_t size);

extern crc_t crcGetChecksum (const CRCGenerator *crc);
extern crc_t crcGetResidue (CRCGenerator *crc);

extern const CRCAlgorithmParameters *crcGetAlgorithmParameters (const CRCGenerator *crc);
extern const CRCGeneratorProperties *crcGetGeneratorProperties (const CRCGenerator *crc);
extern crc_t crcGetValue (const CRCGenerator *crc);

extern const uint8_t crcCheckData[];
extern const uint8_t crcCheckSize;

extern void crcLogAlgorithmParameters (const CRCAlgorithmParameters *parameters);
extern void crcLogGeneratorProperties (const CRCGenerator *crc);

extern int crcVerifyChecksum (CRCGenerator *crc, const char *label, crc_t expected);
extern int crcVerifyResidue (CRCGenerator *crc, const char *label);

extern int crcVerifyAlgorithm (const CRCAlgorithmParameters *parameters);
extern int crcVerifyProvidedAlgorithms (void);

extern int crcVerifyGeneratorWithData (
  CRCGenerator *crc, const char *label,
  const void *data, size_t size, crc_t expected
);

extern int crcVerifyGeneratorWithString (
  CRCGenerator *crc, const char *label,
  const char *string, crc_t expected
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CRC */
