/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

#ifndef _CONT_DEFINITIONS_H
#define _CONT_DEFINITIONS_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
 
#include "contract.h"
#include "misc.h"
#include "brl.h"

#define BYTE unsigned char

#define CTA(table, offset) (((BYTE *)(table)) + (offset))
#define CTE(table, offset) ((ContractionTableEntry *) CTA((table), (offset)))

#define HASHNUM 1087
#define hash(x) (((x[0]<<8)+x[1])%HASHNUM)

typedef enum { /*Op codes*/
  CTO_Synonym=0, /*define numeric opcodes as words*/
  CTO_IncludeFile, /*include a file*/
  CTO_CapitalSign, /*dot pattern for capital sign*/
  CTO_AllCapitalSign, /*dot pattern for multiple capital sign*/
  CTO_LetterSign, /*dot pattern for letter sign*/
  CTO_NumberSign, /*number sign*/
  CTO_Always, /*always use this contraction*/
  CTO_Repeated, /*take just the first, i.e. multiple blanks*/
  CTO_WholeWord, /*whole word contraction*/
  CTO_BegWord, /*beginning of word only*/
  CTO_MidWord, /*middle of word only*/
  CTO_EndWord, /*end of word only*/
  CTO_MidEndWord, /*middle or end of word*/
  CTO_MidNum, /*middle of number, e.g., decimal point*/
  CTO_LowWord, /*enough, were, was, etc.*/
  CTO_LargeSign, /*and, for, the with a */
  CTO_ToBy, /*to, by, into*/
  CTO_Literal, /*don't translate this string*/
  CTO_Contraction, //multiletter word sign that needs letsign
  CTO_Ignore, /*ignore this string (replace with nothing)*/
  CTO_EndNum, /*end of number*/
  CTO_BegNum, /*beginning of number*/
  CTO_PrePunc, /*punctuation in string at beginning of word*/
  CTO_PostPunc, /*punctuation in string at end of word*/
  CTO_None /*For internal use only*/
} ContractionTableOpcode;

typedef unsigned short int ContractionTableOffset;

typedef struct { /*entry in translation table*/
  ContractionTableOffset next; /*next entry*/
  BYTE opcode; /*rule for testing validity of replacement*/
  BYTE findlen; /*length of string to be replaced*/
  BYTE replen; /*length of replacement string*/
  BYTE findrep[1]; /*find and replacement strings*/
} ContractionTableEntry;

typedef struct { /*translation table*/
  ContractionTableOffset capitalSign; /*capitalization sign*/
  ContractionTableOffset allCapitalSign; /*multiple capital sign*/
  ContractionTableOffset letterSign; /*letter sign*/
  ContractionTableOffset numberSign; /*number sign*/
  ContractionTableOffset characters[0X100]; /*locations of character representations in table*/
  ContractionTableOffset cups[0X100]; /*locations of character rules in table*/
  ContractionTableOffset buckets[HASHNUM]; /*locations of multi-character rules in table*/
} ContractionTableHeader;

#endif /* !defined(_CONT_DEFINITIONS_H) */
