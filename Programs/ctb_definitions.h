/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef _CTB_DEFINITIONS_H
#define _CTB_DEFINITIONS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>
#define BYTE unsigned char

#define CTA(table, offset) (((BYTE *)(table)) + (offset))
#define CTR(table, offset) ((ContractionTableRule *)CTA((table), (offset)))

#define HASHNUM 1087
#define hash(x) (((x[0]<<8)+x[1])%HASHNUM)

typedef unsigned long int ContractionTableOffset;

typedef enum {
  CTC_Space       = 0X01,
  CTC_Letter      = 0X02,
  CTC_Digit       = 0X04,
  CTC_Punctuation = 0X08,
  CTC_UpperCase   = 0X10,
  CTC_LowerCase   = 0X20
} ContractionTableCharacterAttribute;
typedef uint32_t ContractionTableCharacterAttributes;

typedef struct {
  ContractionTableOffset rules;
  ContractionTableOffset always;
  ContractionTableCharacterAttributes attributes;
  BYTE uppercase;
  BYTE lowercase;
} ContractionTableCharacter;

typedef enum { /*Op codes*/
  CTO_IncludeFile, /*include a file*/
  CTO_Locale, /*locale specification for character interpretation*/

  CTO_CapitalSign, /*dot pattern for capital sign*/
  CTO_BeginCapitalSign, /*dot pattern for beginning capital block*/
  CTO_EndCapitalSign, /*dot pattern for ending capital block*/

  CTO_EnglishLetterSign, /*dot pattern for english letter sign*/
  CTO_NumberSign, /*number sign*/

  CTO_Literal, /*don't translate this string*/
  CTO_Replace, /*replace this string with another*/
  CTO_Always, /*always use this contraction*/
  CTO_Repeated, /*take just the first, i.e. multiple blanks*/

  CTO_LargeSign, /*and, for, the with a */
  CTO_WholeWord, /*whole word contraction*/
  CTO_JoinableWord, /*to, by, into*/
  CTO_LowWord, /*enough, were, was, etc.*/
  CTO_Contraction, /*multiletter word sign that needs letsign*/

  CTO_SuffixableWord, /*whole word or beginning of word*/
  CTO_PrefixableWord, /*whole word or beginning of word*/
  CTO_BegWord, /*beginning of word only*/
  CTO_BegMidWord, /*beginning or middle of word*/
  CTO_MidWord, /*middle of word only*/
  CTO_MidEndWord, /*middle or end of word*/
  CTO_EndWord, /*end of word only*/

  CTO_PrePunc, /*punctuation in string at beginning of word*/
  CTO_PostPunc, /*punctuation in string at end of word*/

  CTO_BegNum, /*beginning of number*/
  CTO_MidNum, /*middle of number, e.g., decimal point*/
  CTO_EndNum, /*end of number*/

  CTO_Class, /*define a character class*/
  CTO_After, /*only match if after character in class*/
  CTO_Before, /*only match if before character in class*/

  CTO_None /*For internal use only*/
} ContractionTableOpcode;

typedef struct {
  ContractionTableOffset next; /*next entry*/
  ContractionTableOpcode opcode; /*rule for testing validity of replacement*/
  ContractionTableCharacterAttributes after; /*character types which must foollow*/
  ContractionTableCharacterAttributes before; /*character types which must precede*/
  BYTE findlen; /*length of string to be replaced*/
  BYTE replen; /*length of replacement string*/
  BYTE findrep[1]; /*find and replacement strings*/
} ContractionTableRule;

typedef struct { /*translation table*/
  ContractionTableOffset locale; /*locale specification*/
  ContractionTableOffset capitalSign; /*capitalization sign*/
  ContractionTableOffset beginCapitalSign; /*begin capitals sign*/
  ContractionTableOffset endCapitalSign; /*end capitals sign*/
  ContractionTableOffset englishLetterSign; /*english letter sign*/
  ContractionTableOffset numberSign; /*number sign*/
  ContractionTableCharacter characters[0X100]; /*descriptions of characters*/
  ContractionTableOffset rules[HASHNUM]; /*locations of multi-character rules in table*/
} ContractionTableHeader;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CTB_DEFINITIONS_H */
