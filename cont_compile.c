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
static  char ErrorMessage[100]; /*for composing error messages*/
static int lineno=0; /*line number in table*/

static const char *GetToken (const char *buf, int *length)
{ /*find the next string of contiguous nonblank characters*/
const char *inbuf = buf + *length;
int len = 0;
 while ((*inbuf == ' ' || *inbuf == '\t')
 && (*inbuf != '\n' || *inbuf != '\0'))
inbuf++;
if (*inbuf != '\n' || *inbuf != '\0')
while (inbuf[len] != ' ' && inbuf[len] != '\t' && inbuf[len] != '\n'
 && inbuf[len] != '\0')
len++;
*length = len;
return inbuf;
} /*token obtained*/

static BYTE *ParseFind (const char *buf, const int length)
{ /*interpret find string*/
static BYTE holder[100];
 int k, k1=0; /*loop counters*/
 int hh, hl; /*hexadecimal digits*/
for (k=0; k<length; k++)
  if (buf[k] == '\\')
{ //escape sequence
if (buf[k+1] == '0' && buf[k+2] == 'x')
k+=2;
    switch (buf[k+1]) /*escape sequences*/
{
case '\\':
holder[k1++] = '\\';
k++;
break;
case '0': case '1': case '2': case '3': case '4': case '5': case '6':
case '7': case '8': case '9':
hh = buf[k+1] - '0';
goto GetLow;
case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
hh = (buf[k+1] | 32) - 'a';
GetLow:
if (buf[k+2] >= '0' && buf[k+2] <= '9')
hl = buf[k+2] - '0';
 else if ((buf[k+2] >= 'a' && buf[k+2] <= 'f')
 || (buf[k+2] >= 'A' && buf[k+2] <= 'F'))
hl = (buf[k+2] | 32) - 'a';
 else goto badesc;
if (hh==0 && hl==0)
  hh = 2; /*replace null with blank*/
holder[k1++] = 16*hh+hl;
k += 2;
break;
default: badesc:
holder[k1++] = ' ';
break;
}} /*Done with escape sequences*/
else holder[k1++] = buf[k];
holder[k1] = 0;
return holder;
} /*find string interpreted*/

BYTE *ParseDots (const char *buf, const int length)
{ /*get dot patterns*/
static BYTE holder[100];
 /*length of retuned string is in first byte.*/
 BYTE cell=0; /*assembly place for dots*/
 int k, k1=1; /*loop counters*/

for (k=0; k<length; k++)
  switch (buf[k])
    { /*or dots to make up Braille cell*/
case '1':
  cell |= B1;
break;
case '2':
cell |= B2;
break;
case '3':
cell |= B3;
break;
case '4':
cell |= B4;
break;
case '5':
cell |= B5;
break;
case '6':
cell |= B6;
break;
case '7':
cell |= B7;
break;
case '8':
cell |= B8;
break;
 case '0': /*blank*/
cell = 0;
break;
    case '-': /*got all dots for this cell*/
holder[k1++] = cell;
cell = 0;
break;
default:
sprintf (ErrorMessage, "invalid dot pattern on line %d.", lineno);
LogPrint (LOG_WARNING, ErrorMessage);
break;
}
 holder[k1] = cell; /*last cell*/
*holder = k1-1;
return holder;
} /*end of function ParseDots*/

void *CompileContractionTable (const char *name)
{ /*compile source table into a table in memory*/
  BYTE inbuf[100]; /*for lines from table file*/
const BYTE *token;
 int length; /*length of token*/
 BYTE *holder; /*points to stuff to put in table in memory*/
 BYTE OpcodeTable[1500]; /*words redefining numeric opcodes*/
BYTE *OTptr = OpcodeTable; /*pointer to end of OpcodeTable*/
BYTE *OTlookup; /*pointer for looking things up in OpcodeTable*/
opcode op;
FILE *TableSource;
 struct stat  tblstat; /*status of table file*/
 int TableSize = sizeof(ErrorMessage);
int BytesInEntries = 0; /*bytes in table entries*/
TransTable *table = (TransTable *) ErrorMessage;
  TableEntry *curentry = (TableEntry *) ErrorMessage; /*current table entry*/
  TableEntry *SomeEntry; /*used in sorting*/
  TableEntry *preventry; /*also used in sorting*/
  int bucket; /*offset into table*/
if (!(TableSource = fopen (name, "r")))
  { /*fatal error*/
LogPrint (LOG_ERR,
 "Table File does not exist. Output will be in computer Braille.");
 return NULL; /*translator will simply copy input to output*/
}
 while (fgets (inbuf, sizeof(inbuf), TableSource))
 { /*Process lines in table*/
lineno++;
if (inbuf[strlen(inbuf)-1] != '\n')
{
sprintf (ErrorMessage, "In file %s line %d is too long. Ignored.",
 name, lineno);
LogPrint (LOG_WARNING, ErrorMessage);
continue;
}
length = 0;
token = GetToken (inbuf, &length);
if (length == 0)
  continue; /*blank line*/
 if (*token == '#') /*comment*/
continue;
if (*token < '0' || *token > '9')
  { /*look up word in opcode table*/
 OTlookup = OpcodeTable;
while (OTlookup < OTptr)
{
int k = strlen (OTlookup+1);
if (k == length && !strncmp (OTlookup+1, token, length))
  { /*valid verbal opcode*/
op = *OTlookup;
break;
}
OTlookup += k+2;
}
/*error*/
continue;
} /* Done with lookup*/
else op = atoi (token);
if (op != synonym)
{ //Have to do something to table.
if (BytesInEntries == 0)
{ //allocate memory for table.
if (op == tblsize)
{
token = GetToken (token, &length);
TableSize = atoi (token);
}
else {
fstat (fileno (TableSource), &tblstat);
 TableSize = tblstat.st_size + sizeof(TransTable)
+ sizeof(OpcodeTable) + 100 * sizeof(TableEntry);
}
table = (TransTable *) calloc (TableSize,1);
if (op == tblsize)
continue;
} //table allocated
/*start making table entry*/
if (sizeof(TransTable) + BytesInEntries + sizeof (TableEntry) + sizeof(inbuf)
 > TableSize)
{
sprintf (ErrorMessage, "At line %d table size of %d bytes is too small.",
 lineno, TableSize);
LogPrint (LOG_WARNING, ErrorMessage);
LogPrint (LOG_WARNING, "Braille will be only partially contracted.");
break;
}
  curentry = (TableEntry *) table->entries + BytesInEntries;
curentry->next = 0;
} //done doing things to table.
token = GetToken (token, &length);
 switch (op)
{ /*Carry out operations*/
case synonym:
*OTptr++ = atoi(token);
token = GetToken (token, &length);
strncpy (OTptr+1, token, length);
OTptr += length+1;
*OTptr++ = 0;
break;
case tblsize:
LogPrint (LOG_WARNING, "Table size must precede all table entries. Ignored.");
break;
 case always: case wholeword: case begword: case middle: case endword:
 case midend: case lowword: case toby: case largesign: case repeated: 
case midnum:
holder = ParseFind (token, length);
curentry->findlen = strlen (holder);
strcpy (curentry->findrep, holder);
token = GetToken (token, &length);
holder = ParseDots (token, length);
curentry->replen = *holder;
memcpy (curentry->findrep + curentry->findlen, holder+1, *holder);
curentry->rule = op;
break;
case notrans: case contraction: case ignore:
holder = ParseFind (token, length);
curentry->findlen = strlen (holder);
strcpy (curentry->findrep, holder);
curentry->replen = 0;
if (op == ignore)
curentry->rule = always;
break;
case capsign:
holder = ParseDots (token, length);
table->capsign = holder[1];
break;
case multicaps:
holder = ParseDots (token, length);
if (*holder > sizeof (table->multicaps)-1)
*holder = sizeof (table->multicaps)-1;
memcpy (table->multicaps, holder, *holder+1);
break;
case letsign:
holder = ParseDots (token, length);
table->letsign = holder[1];
break;
case numsign:
holder = ParseDots (token, length);
table->numsign = holder[1];
break;
default:
sprintf (ErrorMessage, "On line %d, invalid opcode.", lineno);
LogPrint (LOG_WARNING, ErrorMessage);
continue;
} /*Done with all operations*/
/*link new entry into table.*/
if (curentry->findlen == 1)
{ /*it's a cup, not a bucket*/
bucket = table->cups[*curentry->findrep];
if (bucket == 0)
table->cups[*curentry->findrep] = BytesInEntries;
 else {/*find end of chain*/
SomeEntry = (TableEntry *) table->entries + bucket;
while (SomeEntry->next)
SomeEntry = (TableEntry *) table->entries + SomeEntry->next;
SomeEntry->next = BytesInEntries;
}
goto UpdateSize;
}
bucket = hash(curentry->findrep);
if (table->buckets[bucket] == 0)
table->buckets[bucket] = BytesInEntries;
     else { /*insert table entry in chain, longest find string first*/
SomeEntry = (TableEntry *) table->entries + table->buckets[bucket];
if (curentry->findlen >= SomeEntry->findlen)
{
curentry->next = table->buckets[bucket];
table->buckets[bucket] = BytesInEntries;
}
else { /*look through the chain to find the right place*/
while (SomeEntry->next)
{ /*loop through chain*/
int k;
preventry = SomeEntry;
SomeEntry = (TableEntry *) table->entries + (k=SomeEntry->next);
if (curentry->findlen > SomeEntry->findlen)
{
preventry->next = BytesInEntries;
curentry->next = k;
goto UpdateSize;
}} /*new entry goes at end of chain*/
SomeEntry->next = BytesInEntries;
} /*done looking and inserted*/
} /*done with inserting table entry in chain*/
UpdateSize:
BytesInEntries += sizeof(TableEntry)
 + curentry->findlen + curentry->replen;
     } /*end of loop for processing TableSource*/
/*put verbal opcodes in table for printing utility*/
//curentry = (TableEntry *) table->entries + BytesInEntries;
//curentry->next = OTptr - OpcodeTable; /*It ain't next.*/
//memcpy (curentry->findrep, OpcodeTable, curentry->next);
//table->VerbalOpcodes = BytesInEntries;
fclose (TableSource);
return (void *) table;
} /*compilation completed*/


