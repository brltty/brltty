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

#ifndef _CONT_HEADERS_H
#define _CONT_HEADERS_H

#define HASHNUM 1087
#define BYTE unsigned char
#define hash(x) (((*x<<8)+(*x+1))%HASHNUM)

typedef struct { /*entry in translation table*/
  int next; /*next entry*/
  BYTE findlen, /*length of string to be replaced*/
    replen, /*length of replacement string*/
    rule; /*rule for testing validity of replacement*/
  BYTE findrep[20]; /*find and replacement strings*/
} TableEntry;

typedef struct { /*translation table*/
  int VerbalOpcodes; /*location of synonyms in table*/
  BYTE capsign, /*capitalization sign*/
    multicaps[5], /*multiple capital sign*/
    letsign, /*letter sign*/
    numsign; /*number sign*/
  int cups[256]; /*locations ( characters in table*/
 int buckets[HASHNUM]; /*offsets into hash table*/
  BYTE entries[1]; /*entries in this table*/
} TransTable;

typedef enum { /*Op codes*/
  synonym=0, /*define numeric opcodes as words*/
    tblsize, /*For overriding size heuristic*/
  capsign, /*dot pattern for capital sign*/
  multicaps, /*dot pattern for multiple capital sign*/
  letsign, /*dot pattern for letter sign*/
  numsign, /*number sign*/
  always, /*always use this contraction*/
  repeated, /*take just the first, i.e. multiple blanks*/
  wholeword, /*whole word contraction*/
  begword, /*beginning of word only*/
  middle, /*middle of word only*/
  endword, /*end of word only*/
  midend, /*middle or end of word*/
  midnum, /*middle of number, e.g., decimal point*/
  lowword, /*enough, were, was, etc.*/
  largesign, /*and, for, the with a */
  toby, /*to, by, into*/
    notrans, /*don't translate this string*/
  contraction, //multiletter word sign that needs letsign
  ignore, /*ignore this string (replace with nothing)*/
nop /*just for convenience*/
} opcode;

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#include "contract.h"
#include "misc.h"
#include "brl.h"

#endif /* !defined(_CONT_HEADERS_H) */
