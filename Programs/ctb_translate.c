/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
 
#include "contract.h"
#include "ctb_definitions.h"
#include "brl.h"

static const ContractionTableHeader *table;	/*translation table */
static const BYTE *src, *srcmin, *srcmax;
static BYTE *dest, *destmin, *destmax;
static int *offsets;
static BYTE before, after;	/*the characters before and after a string */
static int curfindlen;		/*length of current find string */
static ContractionTableOpcode curop;
static ContractionTableOpcode prevop;
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

#define CTC(c,a) (table->characters[(c)].attributes & (a))
static int
selectRule (int length) { /*check for valid contractions */
  int bucket;			/*integer offset of talle entry */

  if (length < 1) return 0;
  if (length == 1) {
    bucket = table->cups[*src];
  } else {
    BYTE bytes[2];
    bytes[0] = table->characters[src[0]].lowercase;
    bytes[1] = table->characters[src[1]].lowercase;
    bucket = table->buckets[hash(bytes)];
  }
  while (bucket) {
    curentry = CTE(table, bucket);
    curop = curentry->opcode;
    curfindlen = curentry->findlen;
    if (length == 1 ||
        (curfindlen <= length &&
         (strncasecmp(src, curentry->findrep, curfindlen) == 0))) {
      /* check this entry */
      setAfter(curfindlen);
      switch (curop) { /*check validity of this contraction */
        case CTO_Always:
        case CTO_Repeated:
        case CTO_Ignore:
        case CTO_Literal:
          return 1;
        case CTO_LargeSign:
          if (!CTC(before, CTC_Space) || !CTC(after, CTC_Space))
            curop = CTO_Always;
          return 1;
        case CTO_WholeWord:
        case CTO_Contraction:
          if (CTC(before, CTC_Space|CTC_Punctuation) &&
              CTC(after, CTC_Space|CTC_Punctuation))
            return 1;
          break;
        case CTO_LowWord:
          if (CTC(before, CTC_Space) && CTC(after, CTC_Space) &&
              (prevop != CTO_JoinableWord))
            return 1;
          break;
        case CTO_JoinableWord:
          if (CTC(before, CTC_Space|CTC_Punctuation) &&
              CTC(after, CTC_Space) &&
              (dest + curentry->replen < destmax)) {
            const BYTE *ptr = src + curfindlen + 1;
            while (ptr < srcmax) {
              if (!CTC(*ptr, CTC_Space)) {
                if (CTC(*ptr, CTC_Letter)) return 1;
                break;
              }
              ptr++;
            }
          }
          break;
        case CTO_SuffixableWord:
          if (CTC(before, CTC_Space|CTC_Punctuation) &&
              CTC(after, CTC_Space|CTC_Letter|CTC_Punctuation))
            return 1;
          break;
        case CTO_BegWord:
          if (CTC(before, CTC_Space|CTC_Punctuation) &&
              CTC(after, CTC_Letter))
            return 1;
          break;
        case CTO_MidWord:
          if (CTC(before, CTC_Letter) && CTC(after, CTC_Letter))
            return 1;
          break;
        case CTO_MidEndWord:
          if (CTC(before, CTC_Letter) &&
              CTC(after, CTC_Letter|CTC_Space|CTC_Punctuation))
            return 1;
          break;
        case CTO_EndWord:
          if (CTC(before, CTC_Letter) &&
              CTC(after, CTC_Space|CTC_Punctuation))
            return 1;
          break;
        case CTO_BegNum:
          if (CTC(before, CTC_Space|CTC_Punctuation) &&
              CTC(after, CTC_Digit))
            return 1;
          break;
        case CTO_MidNum:
          if (CTC(before, CTC_Digit) && CTC(after, CTC_Digit))
            return 1;
          break;
        case CTO_EndNum:
          if (CTC(before, CTC_Digit) &&
              CTC(after, CTC_Space|CTC_Punctuation))
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
          if (CTC(*src, CTC_Punctuation)) {
            const BYTE *pre = src;
            const BYTE *post = src + curfindlen;
            while (--pre >= srcmin)
              if (!CTC(*pre, CTC_Punctuation))
                break;
            while (post < srcmax) {
              if (!CTC(*post, CTC_Punctuation))
                break;
              post++;
            }
            if (isPre) {
              if (((pre < srcmin) || CTC(*pre, CTC_Space)) &&
                  ((post < srcmax) && !CTC(*post, CTC_Space)))
                return 1;
            } else {
              if (((pre >= srcmin) && !CTC(*pre, CTC_Space)) &&
                  ((post == srcmax) || CTC(*post, CTC_Space)))
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
putBytes (const BYTE *bytes, int count) {
  if (dest + count > destmax) return 0;
  memcpy(dest, bytes, count);
  dest += count;
  return 1;
}

static int
putUnknown (BYTE character) { /* Convert unknown character to hexadecimal. */
  static const BYTE hexDigits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };
  BYTE buffer[3];
  buffer[0] = textTable['\\'];
  buffer[1] = textTable[hexDigits[(character >> 4) & 0XF]];
  buffer[2] = textTable[hexDigits[character & 0XF]];
  return putBytes(buffer, sizeof(buffer));
}

static int
putCharacter (BYTE character) { /* Convert unknown character to hexadecimal. */
  ContractionTableOffset offset = table->characters[character].entry;
  if (offset) {
    const ContractionTableEntry *entry = CTE(table, offset);
    if (!entry->replen) return putBytes(&textTable[character], 1);
    return putBytes(&entry->findrep[1], entry->replen);
  }
  if (putUnknown(character)) return 1;
  return 0;
}

static int
putSequence (ContractionTableOffset offset) {
  const BYTE *sequence = CTA(table, offset);
  return putBytes(sequence+1, *sequence);
}

int
contractText (void *contractionTable,
              const BYTE *inputBuffer, int *inputLength,
              BYTE *outputBuffer, int *outputLength,
              int *offsetsMap, const int cursorOffset) { /*begin translation */
  const BYTE *srcword = NULL;
  BYTE *destword = NULL; /* last word transla1ted */
  int computerBraille = 0;
  const BYTE *activeBegin, *activeEnd;

  if (!(table = (ContractionTableHeader *) contractionTable)) return 0;
  srcmax = (srcmin = src = inputBuffer) + *inputLength;
  destmax = (destmin = dest = outputBuffer) + *outputLength;
  offsets = offsetsMap;
  prevop = CTO_None;

  activeBegin = activeEnd = NULL;
  if (cursorOffset >= 0 && cursorOffset < *inputLength &&
      !CTC(inputBuffer[cursorOffset], CTC_Space)) {
    activeBegin = activeEnd = srcmin + cursorOffset;
    while (--activeBegin >= srcmin && !CTC(*activeBegin, CTC_Space));
    while (++activeEnd < srcmax && !CTC(*activeEnd, CTC_Space));
  }

  while (src < srcmax) { /*the main translation loop */
    setOffset();
    setBefore();
    if (computerBraille)
      if (CTC(*src, CTC_Space))
        computerBraille = 0;
    if (computerBraille || (src > activeBegin && src < activeEnd)) {
      setAfter(1);
      curop = CTO_Literal;
      if (!putBytes(&textTable[*src], 1)) break;
      src++;
    } else if (selectRule(srcmax-src) || selectRule(1)) {
      if (table->numberSign && prevop != CTO_MidNum &&
          !CTC(before, CTC_Digit) && CTC(*src, CTC_Digit)) {
        if (!putSequence(table->numberSign)) break;
      } else if (table->englishLetterSign && CTC(*src, CTC_Letter)) {
        if ((curop == CTO_Contraction) ||
            (curop != CTO_EndNum && CTC(before, CTC_Digit)) ||
            (curop == CTO_Always && curfindlen == 1 && CTC(before, CTC_Space) &&
             (src + 1 == srcmax || CTC(src[1], CTC_Space) ||
              (CTC(src[1], CTC_Punctuation) &&
              !(src + 2 < srcmax && src[1] == '.' && CTC(src[2], CTC_Letter)))))) {
          if (!putSequence(table->englishLetterSign)) break;
        }
      }
      if (CTC(*src, CTC_UpperCase)) {
        if (!CTC(before, CTC_UpperCase)) {
          if (table->beginCapitalSign &&
              (src + 1 < srcmax) && CTC(src[1], CTC_UpperCase)) {
            if (!putSequence(table->beginCapitalSign)) break;
          } else if (table->capitalSign) {
            if (!putSequence(table->capitalSign)) break;
          }
        }
      } else if (CTC(*src, CTC_LowerCase)) {
        if (table->endCapitalSign && (src - 2 >= srcmin) &&
            CTC(src[-1], CTC_UpperCase) && CTC(src[-2], CTC_UpperCase)) {
          if (!putSequence(table->endCapitalSign)) break;
        }
      }

      /* pre processing */
      switch (curop) {
        case CTO_LargeSign:
          if (prevop == CTO_LargeSign) {
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

      /* main processing */
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
          if (curentry->replen) {
            if (!putBytes(&curentry->findrep[curfindlen], curentry->replen)) goto done;
            src += curfindlen;
          } else {
            const BYTE *srclim = src + curfindlen;
            while (1) {
              if (!putCharacter(*src)) goto done;
              if (++src == srclim) break;
              setOffset();
            }
          }
      }

      /* post processing */
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
        case CTO_JoinableWord:
          while ((src < srcmax) && CTC(*src, CTC_Space)) src++;
          break;
        default:
          break;
      }
    } else {
      if (!putUnknown(*src)) break;
      src++;
    }

    if (((src > srcmin) && CTC(src[-1], CTC_Space) && (curop != CTO_JoinableWord))) {
      srcword = src;
      destword = dest;
    }
    if ((dest == destmin) || dest[-1])
      prevop = curop;
  }				/*end of translation loop */
done:

  if (destword != NULL && src < srcmax && !CTC(*src, CTC_Space)) {
    src = srcword;
    dest = destword;
  }
  if (src < srcmax) {
    setOffset();
    while (CTC(*src, CTC_Space))
      if (++src == srcmax)
        break;
  }

  *inputLength = src - srcmin;
  *outputLength = dest - destmin;
  return 1;
}				/*translation completed */
