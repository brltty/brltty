/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_CTB_INTERNAL
#define BRLTTY_INCLUDED_CTB_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BYTE unsigned char

#define HASHNUM 1087
#define CTH(x) (((x[0]<<8)+x[1])%HASHNUM)

typedef uint32_t ContractionTableOffset;

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
  wchar_t value;
  ContractionTableOffset rules;
  ContractionTableOffset always;
  ContractionTableCharacterAttributes attributes;
} ContractionTableCharacter;

typedef enum { /*Op codes*/
  CTO_CapitalSign, /*dot pattern for capital sign*/
  CTO_BeginCapitalSign, /*dot pattern for beginning capital block*/
  CTO_EndCapitalSign, /*dot pattern for ending capital block*/

  CTO_EnglishLetterSign, /*dot pattern for english letter sign*/
  CTO_NumberSign, /*number sign*/

  CTO_Literal, /*don't translate this string*/
  CTO_Always, /*always use this contraction*/
  CTO_Repeatable, /*take just the first, i.e. multiple blanks*/

  CTO_LargeSign, /*and, for, of, the, with*/
  CTO_LastLargeSign, /*a*/
  CTO_WholeWord, /*whole word contraction*/
  CTO_JoinedWord, /*to, by, into*/
  CTO_LowWord, /*enough, were, was, etc.*/
  CTO_Contraction, /*multiletter word sign that needs letsign*/

  CTO_SuffixableWord, /*whole word or beginning of word*/
  CTO_PrefixableWord, /*whole word or end of word*/
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

  CTO_IncludeFile, /*include a file*/
  CTO_None /*For internal use only*/
} ContractionTableOpcode;

typedef struct {
  ContractionTableOffset next; /*next entry*/
  ContractionTableOpcode opcode; /*rule for testing validity of replacement*/
  ContractionTableCharacterAttributes after; /*character types which must foollow*/
  ContractionTableCharacterAttributes before; /*character types which must precede*/
  BYTE findlen; /*length of string to be replaced*/
  BYTE replen; /*length of replacement string*/
  wchar_t findrep[1]; /*find and replacement strings*/
} ContractionTableRule;

typedef struct { /*translation table*/
  ContractionTableOffset capitalSign; /*capitalization sign*/
  ContractionTableOffset beginCapitalSign; /*begin capitals sign*/
  ContractionTableOffset endCapitalSign; /*end capitals sign*/
  ContractionTableOffset englishLetterSign; /*english letter sign*/
  ContractionTableOffset numberSign; /*number sign*/
  ContractionTableOffset characters;
  uint32_t characterCount;
  ContractionTableOffset rules[HASHNUM]; /*locations of multi-character rules in table*/
} ContractionTableHeader;

typedef struct {
  wchar_t value;
  wchar_t uppercase;
  wchar_t lowercase;
  ContractionTableCharacterAttributes attributes;
} CharacterEntry;

struct ContractionTableStruct {
  union {
    ContractionTableHeader *fields;
    const unsigned char *bytes;
  } header;

  size_t size;

  CharacterEntry *characters;
  int charactersSize;
  int characterCount;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CTB_INTERNAL */
