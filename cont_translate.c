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

#include "cont_headers.h"
#define UNKNOWN B4|B5 /*character not in table*/

static TransTable *table; /*translation table*/
static const BYTE *src;
static BYTE *dest;
static int maxlen; /*length of whole source string*/
static int srclen, destlen; /*loop counters*/;
static BYTE before, after; /*the characters before and after a string*/
static int curfindlen; /*length of current find string*/
static TableEntry *curentry; /*pointer to current entry in table*/

static int validate (const int length)
{ /*check for valid contractions*/
int bucket; /*integer offset of talle entry*/
 int found; /*indicates valid contraction*/

if (length > 1)
bucket = table->buckets[hash(src)];
else bucket = table->cups[*src];
if (!bucket)
return 0;
if (srclen==0)
before = ' ';
else before = src[-1];
    do { /*while curentry->next != 0*/
curentry = (TableEntry *) table->entries + bucket;
found = 0;
curfindlen = curentry->findlen;
if (srclen+curfindlen <= maxlen && (length==1 || 
   !strncasecmp (src, curentry->findrep, curfindlen)))
  { /*check this entry*/
if (srclen+curfindlen >= maxlen)
after = ' ';
else after = src[curfindlen];
switch ((opcode) curentry->rule)
  { /*check validity of this contraction*/
  default: case always: case largesign:
found = 1;
break;
  case wholeword: case contraction:
if ((isspace(before) || ispunct(before)) && (isspace(after) || ispunct(after)))
found = 1;
break;
case begword:
if ((isspace(before) || ispunct(before)) && isalpha(after))
found = 1;
break;
case middle:
if (isalpha(before) && isalpha(after))
found = 1;
break;
case midend:
if (isalpha(before) && (isspace(after) || ispunct(after)))
found = 1;
break;
case midnum:
if (isdigit(before) && isdigit(after))
found = 1;
break;
case endword:
if (isalpha(before) && (isspace(after) || ispunct(after)))
found = 1;
break;
case lowword:
if (isspace(before) && isspace(after))
found = 1;
break;
case toby:
if ((isspace(before) || ispunct(before))&& isspace(after))
found = 1;
break;
  }} /*Done with checking this entry*/
if (found)
if (found)
break; /*don't look at rest of chain*/
bucket = curentry->next;
} /*any more in this chain?*/
while (bucket);
return found;
} /*done with validation*/

int TranslateContracted (void *tbl, const char *source, int *SourceLen,
 char *target, int *TargetLen, int *offsets, int CursorPos)
{ /*begin translation*/
int found;
 int curreplen; /*length of replacement string*/
 BYTE fake[12]; /*for making fake table entry*/
 int k, k1; //loop counter
 opcode prevop = nop ; //largesign, etc.
TableEntry *SomeEntry;

  if (tbl == NULL)
return 0;
table = (TransTable *) tbl;
src = source;
dest = target;
maxlen = *SourceLen;
srclen = destlen = 0;
while (srclen < *SourceLen && destlen < *TargetLen)
  { /*the main translation loop*/
found = validate (*SourceLen - srclen);
if (!found)
found = validate (1); /*check the single character*/
if (!found) /*character is not in table*/
  { /*make a fake table entry*/
curentry = (TableEntry *) fake;
curentry->next = 0;
curentry->findlen = curentry->replen = 1;
curentry->rule = always;
*curentry->findrep = *src;
 curentry->findrep[1] = UNKNOWN; /*dot pattern for unknown character*/
curfindlen = 1;
after = src[1];
  } /*fake entry completed*/
curreplen = curentry->replen;
if (table->numsign && !isdigit(before) && isdigit(*src))
{
*dest++ = table->numsign;
destlen++;
}
else if ((table->letsign && curentry->rule == contraction)
 || (table->letsign && curentry->rule!=wholeword && isspace(before)
     && isalpha(*src) && isspace (src[1])))
{
*dest++ = table->letsign;
destlen++;
}
if (table->multicaps && !isupper(before) && isupper(*src) && isupper(src[1]))
{
memcpy (dest, table-multicaps+1, *table->multicaps);
dest += *table->multicaps;
destlen += *table->multicaps;
}
else if (table->capsign && !isupper(before) && isupper(*src))
{
*dest++ = table->capsign;
destlen++;
}
switch ((opcode)curentry->rule)
  { /*take action according to rules*/
default:
TheUsual:
src += curfindlen;
srclen += curfindlen;
InsertIt:
memcpy (dest, curentry->findrep+curfindlen, curreplen);
dest += curreplen;
destlen += curreplen;
break;
case largesign:
if (isspace(before) && prevop == largesign)
{ //get rid of blank in output.
dest--;
destlen--;
}
goto TheUsual;
case toby:
src++;
srclen++;
goto TheUsual;
 case repeated:
while (!strncasecmp(curentry->findrep, src, curfindlen))
{
src += curfindlen;
srclen += curfindlen;
}
goto InsertIt;
case notrans: case contraction:
for (k=0; k<curfindlen; k++)
{ //insert dot patterns from table
SomeEntry = (TableEntry *) table->entries + table->cups[curentry->findrep[k]];
 while (SomeEntry->next) //last is probably best
SomeEntry = (TableEntry *) table->entries + SomeEntry->next;
 for (k1=0; k1<SomeEntry->replen; k1++)
*dest++ = SomeEntry->findrep[k1];
destlen += SomeEntry->replen;
}
src += curfindlen;
srclen += curfindlen;
break;
case ignore:
src += curfindlen;
srclen += curfindlen;
break;
 } /*end of action*/
if (!isspace(*src))
prevop = curentry->rule;
offsets[destlen] = srclen;
  } /*end of translation loop*/
*SourceLen = srclen;
*TargetLen = destlen;
return 1;
} /*translation completed*/
