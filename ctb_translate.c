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

#include "ctb_definitions.h"

static const ContractionTableHeader *table;	/*translation table */
static const BYTE *src, *srcmin, *srcmax;
static BYTE *dest, *destmin, *destmax;
static int *offsets;
static BYTE before, after;	/*the characters before and after a string */
static int curfindlen;		/*length of current find string */
static ContractionTableOpcode curop; // So vaalidate can change opcode
static ContractionTableOpcode prevop;		//largesign, etc.
static const ContractionTableEntry *curentry;	/*pointer to current entry in table */

#define setOffset() offsets[src - srcmin] = dest - destmin

static void
setBefore (void) {
  before = (src == srcmin)? ' ': src[-1];
}

static void
setAfter (int length) {
  after = (src + length < srcmax)? src[length]: ' ';
}

static int
selectRule (const int length) { /*check for valid contractions */
  int bucket;			/*integer offset of talle entry */

  if (length < 1)
    return 0;
  if (length == 1) {
    bucket = table->cups[*src];
  } else {
    BYTE bytes[2];		//to hold lower-case letters
    bytes[0] = tolower(src[0]);
    bytes[1] = tolower(src[1]);
    bucket = table->buckets[hash(bytes)];
  }
  setBefore();
  while (bucket) {
    curentry = CTE(table, bucket);
    curop = curentry->opcode;
    curfindlen = curentry->findlen;
    if (src + curfindlen <= srcmax &&
        (length == 1 || (strncasecmp(src, curentry->findrep, curfindlen) == 0))) { /*check this entry */
      setAfter(curfindlen);
      switch (curop) { /*check validity of this contraction */
        case CTO_Always:
        case CTO_Repeated:
        case CTO_Ignore:
        case CTO_Literal:
          return 1;
        case CTO_LargeSign:
          if (isalpha(before) || isalpha(after))
            curop = CTO_Always;
          return 1;
        case CTO_WholeWord:
        case CTO_Contraction:
          if ((isspace(before) || ispunct(before)) &&
              (isspace(after) || ispunct(after)))
            return 1;
          break;
        case CTO_LowWord:
          if (isspace(before) && isspace(after) && prevop != CTO_ToBy)
            return 1;
          break;
        case CTO_ToBy:
          if ((isspace(before) || ispunct(before)) && isspace(after) &&
              dest + curentry->replen < destmax) {
            const BYTE *ptr = src + curfindlen + 1;
            while (ptr < srcmax) {
              if (!isspace(*ptr)) {
                if (isalpha(*ptr))
                  return 1;
                break;
              }
              ptr++;
            }
          }
          break;
        case CTO_BegWord:
          if ((isspace(before) || ispunct(before)) && isalpha(after))
            return 1;
          break;
        case CTO_MidWord:
          if (isalpha(before) && isalpha(after))
            return 1;
          break;
        case CTO_MidEndWord:
          if (isalpha(before) &&
              (isalpha(after) || isspace(after) || ispunct(after)))
            return 1;
          break;
        case CTO_EndWord:
          if (isalpha(before) && (isspace(after) || ispunct(after)))
            return 1;
          break;
        case CTO_BegNum:
          if ((isspace(before) || ispunct(before)) && isdigit(after))
            return 1;
          break;
        case CTO_MidNum:
          if (isdigit(before) && isdigit(after))
            return 1;
          break;
        case CTO_EndNum:
          if (isdigit(before) && (isspace(after) || ispunct(after)))
            return 1;
          break;
        {
          int isPre;
        case CTO_PrePunc:
          isPre = 1;
          goto doPunc;
        case CTO_PostPunc:
          isPre = 0;
        doPunc:
          if (ispunct(*src)) {
            const BYTE *pre = src;
            const BYTE *post = src + curfindlen;
            while (--pre >= srcmin)
              if (!ispunct(*pre))
                break;
            while (post < srcmax) {
              if (!ispunct(*post))
                break;
              post++;
            }
            if (isPre) {
              if (((pre < srcmin) || isspace(*pre)) &&
                  ((post < srcmax) && !isspace(*post)))
                return 1;
            } else {
              if (((pre >= srcmin) && !isspace(*pre)) &&
                  ((post == srcmax) || isspace(*post)))
                return 1;
            }
          }
          break;
        }
        default:
          break;
      }
    }				/*Done with checking this entry */
    bucket = curentry->next;
  }
  return 0;
}				/*done with validation */

static int
putUnknown (BYTE character) { // Convert unknown character to hexadecimal.
  if (dest + 3 <= destmax) {
    static const BYTE hexDigits[] = {
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };
    *dest++ = textTable['\\'];
    *dest++ = textTable[hexDigits[(character >> 4) & 0XF]];
    *dest++ = textTable[hexDigits[character & 0XF]];
    return 1;
  }
  return 0;
}				// Done with translating char to hex

static int
putBytes (const BYTE *bytes, int count) {
  if (dest + count > destmax) return 0;
  memcpy(dest, bytes, count);
  dest += count;
  return 1;
}

static int
putCharacter (const ContractionTableHeader *table, BYTE character) { // Convert unknown character to hexadecimal.
  ContractionTableOffset offset = table->characters[character];
  if (offset) {
    const ContractionTableEntry *entry = CTE(table, offset);
    return putBytes(&entry->findrep[1], entry->replen);
  }
  if (putUnknown(character)) return 1;
  return 0;
}

static int
putSequence (const ContractionTableHeader *table, ContractionTableOffset offset) {
  const BYTE *sequence = CTA(table, offset);
  return putBytes(sequence+1, *sequence);
}

int
contractText (void *contractionTable,
              const BYTE *inputBuffer, int *inputLength,
              BYTE *outputBuffer, int *outputLength,
              int *offsetsMap, const int cursorOffset) { /*begin translation */
  const BYTE *srcword = NULL; // it's location in source
  BYTE *destword = NULL; // last word transla1ted
  int computerBraille = 0;
  const BYTE *activeBegin, *activeEnd;

  if (!(table = (ContractionTableHeader *) contractionTable))
    return 0;
  srcmax = (srcmin = src = inputBuffer) + *inputLength;
  destmax = (destmin = dest = outputBuffer) + *outputLength;
  offsets = offsetsMap;
  prevop = CTO_None;		//largesign, etc.

  activeBegin = activeEnd = NULL;
  if (cursorOffset >= 0 && cursorOffset < *inputLength &&
      !isspace(inputBuffer[cursorOffset])) {
    activeBegin = activeEnd = srcmin + cursorOffset;
    while (--activeBegin >= srcmin && !isspace(*activeBegin));
    while (++activeEnd < srcmax && !isspace(*activeEnd));
  }

  while (src < srcmax && dest < destmax) { /*the main translation loop */
    if (computerBraille)
      if (isspace(*src))
        computerBraille = 0;
    if (computerBraille || (src > activeBegin && src < activeEnd)) {
      curop = CTO_Literal;
      setBefore();
      setAfter(1);
      setOffset();
      *dest++ = textTable[*src++];
    } else if (selectRule(srcmax-src) || selectRule(1)) {
      int curreplen = curentry->replen;

      if (src + curfindlen > srcmax) break;			//don't overflow buffer!
      setOffset();
      if (table->numberSign && prevop != CTO_MidNum &&
          !isdigit(before) && isdigit(*src)) {
        if (!putSequence(table, table->numberSign)) break;
      } else if (table->letterSign) {
        if ((curop == CTO_Contraction) ||
            (curop != CTO_EndNum && isdigit(before) && isalpha(*src)) ||
            (curop != CTO_WholeWord && curop != CTO_LowWord && curop != CTO_LargeSign &&
             isspace(before) && isalpha(*src) &&
             (src + curfindlen == srcmax || isspace(src[1]) || ispunct(src[1])))) {
          if (!putSequence(table, table->letterSign)) break;
        }
      }
      if (table->allCapitalSign &&
          !isupper(before) && isupper(*src) &&
          src + 1 < srcmax && isupper(src[1])) {
        if (!putSequence(table, table->allCapitalSign)) break;
      } else if (table->capitalSign && !isupper(before) && isupper(*src)) {
        if (!putSequence(table, table->capitalSign)) break;
      }
      if (dest + curreplen > destmax) break;			//don't overflow buffer!

      // pre processing
      switch (curop) {
        case CTO_LargeSign:
          if (prevop == CTO_LargeSign) { //get rid of blank in output.
            int srcoff = src - srcmin;
            int destlen;
            while (dest > destmin && !dest[-1]) dest--;
            setOffset();
            destlen = dest - destmin;
            while (srcoff > 0) {
              int destoff = offsets[--srcoff];
              if (destoff != -1) {
                if (destoff < destlen) break;
                offsets[srcoff] = -1;
              }
            }
          }
          break;
        default:
          break;
      }				/*end of action */

      // main processing
      switch (curop) {
        case CTO_Ignore:
          src += curfindlen;
          break;
        case CTO_Literal: {
          const BYTE *srcorig = src;
          if (destword) {
            src = srcword;
            dest = destword;
          } else {
            src = srcmin;
            dest = destmin;
          }
          while (srcorig > src) offsets[--srcorig - srcmin] = -1;
          computerBraille = 1;
          continue;
        }
        default:
          if (curreplen) {
            src += curfindlen;
            memcpy(dest, &curentry->findrep[curfindlen], curreplen);
            dest += curreplen;
          } else {
            int index;
            for (index = 0; index < curfindlen; index++) { //insert dot patterns from table
              if (index) setOffset();
              if (!putCharacter(table, *src)) break;
              src++;
            }
          }
      }

      // post processing
      switch (curop) {
        case CTO_Repeated: {
          const BYTE *srclim = srcmax - curfindlen;
          setOffset();
          while ((src <= srclim) &&
                 (strncasecmp(curentry->findrep, src, curfindlen) == 0)) {
            src += curfindlen;
          }
          break;
        }
        case CTO_ToBy:
          while ((src < srcmax) && isspace(*src)) {
            src++;
          }
          break;
        default:
          break;
      }
    } else {
      setOffset();
      if (!putUnknown(*src)) break;
      src++;
    }

    {
      int space = dest > destmin && dest[-1] == 0; //blank Braille cell
      if (space || (isspace(after) && curop != CTO_ToBy)) {
        srcword = src;
        destword = dest;
      }
      if (!space)
        prevop = curop;
    }
  }				/*end of translation loop */

  if (destword != NULL && src < srcmax && !isspace(*src)) {
    src = srcword;
    dest = destword;
  }
  if (src < srcmax) {
    setOffset();
    while (isspace(*src))
      if (++src == srcmax)
        break;
  }

  *inputLength = src - srcmin;
  *outputLength = dest - destmin;
  return 1;
}				/*translation completed */
