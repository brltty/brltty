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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
 
#include "ctb.h"
#include "ctb_definitions.h"
#include "brl.h"

static const ContractionTableHeader *table;	/*translation table */
static const BYTE *src, *srcmin, *srcmax;
static BYTE *dest, *destmin, *destmax;
static int *offsets;
static BYTE before, after;	/*the characters before and after a string */
static int currentFindLength;		/*length of current find string */
static ContractionTableOpcode currentOpcode;
static ContractionTableOpcode previousOpcode;
static const ContractionTableRule *currentRule;	/*pointer to current rule in table */

#define setOffset() offsets[src - srcmin] = dest - destmin
#define CTC(c,a) (table->characters[(c)].attributes & (a))
#define CTL(c) (CTC((c), CTC_UpperCase)? table->characters[(c)].lowercase: (c))

static int
compareBytes (const BYTE *address1, const BYTE *address2, int count) {
  while (count) {
    if (CTL(*address1) != CTL(*address2)) return 0;
    --count, ++address1, ++address2;
  }
  return 1;
}

static void
setBefore (void) {
  before = (src == srcmin)? ' ': src[-1];
}

static void
setAfter (int length) {
  after = (src + length < srcmax)? src[length]: ' ';
}

static int
selectRule (int length) { /*check for valid contractions */
  int ruleOffset;

  if (length < 1) return 0;
  if (length == 1) {
    ruleOffset = table->characters[CTL(*src)].rules;
  } else {
    BYTE bytes[2];
    bytes[0] = CTL(src[0]);
    bytes[1] = CTL(src[1]);
    ruleOffset = table->rules[hash(bytes)];
  }
  while (ruleOffset) {
    currentRule = CTR(table, ruleOffset);
    currentOpcode = currentRule->opcode;
    currentFindLength = currentRule->findlen;
    if ((length == 1) ||
        ((currentFindLength <= length) &&
         compareBytes(src, currentRule->findrep, currentFindLength))) {
      /* check this rule */
      setAfter(currentFindLength);
      if ((!currentRule->after || CTC(before, currentRule->after)) &&
        (!currentRule->before || CTC(after, currentRule->before))) {
        switch (currentOpcode) { /*check validity of this contraction */
          case CTO_Always:
          case CTO_Repeated:
          case CTO_Replace:
          case CTO_Literal:
            return 1;
          case CTO_LargeSign:
            if (!CTC(before, CTC_Space) || !CTC(after, CTC_Space))
              currentOpcode = CTO_Always;
            return 1;
          case CTO_WholeWord:
          case CTO_Contraction:
            if (CTC(before, CTC_Space|CTC_Punctuation) &&
                CTC(after, CTC_Space|CTC_Punctuation))
              return 1;
            break;
          case CTO_LowWord:
            if (CTC(before, CTC_Space) && CTC(after, CTC_Space) &&
                (previousOpcode != CTO_JoinableWord))
              return 1;
            break;
          case CTO_JoinableWord:
            if (CTC(before, CTC_Space|CTC_Punctuation) &&
                CTC(after, CTC_Space) &&
                (dest + currentRule->replen < destmax)) {
              const BYTE *ptr = src + currentFindLength + 1;
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
          case CTO_PrefixableWord:
            if (CTC(before, CTC_Space|CTC_Letter|CTC_Punctuation) &&
                CTC(after, CTC_Space|CTC_Punctuation))
              return 1;
            break;
          case CTO_BegWord:
            if (CTC(before, CTC_Space|CTC_Punctuation) &&
                CTC(after, CTC_Letter))
              return 1;
            break;
          case CTO_BegMidWord:
            if (CTC(before, CTC_Letter|CTC_Space|CTC_Punctuation) &&
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
              const BYTE *post = src + currentFindLength;
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
      }
    }				/*Done with checking this rule */
    ruleOffset = currentRule->next;
  }
  return 0;
}

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
putCharacter (BYTE character) {
  ContractionTableOffset offset = table->characters[character].always;
  if (offset) {
    const ContractionTableRule *rule = CTR(table, offset);
    if (rule->replen) return putBytes(&rule->findrep[1], rule->replen);
    return putBytes(&textTable[character], 1);
  }
  return putUnknown(character);
}

static int
putCharacters (const BYTE *characters, int count) {
  while (count--)
    if (!putCharacter(*characters++))
      return 0;
  return 1;
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
  const BYTE *computerBraille = NULL;

  if (!(table = (ContractionTableHeader *) contractionTable)) return 0;
  srcmax = (srcmin = src = inputBuffer) + *inputLength;
  destmax = (destmin = dest = outputBuffer) + *outputLength;
  offsets = offsetsMap;
  previousOpcode = CTO_None;

  while (src < srcmax) { /*the main translation loop */
    setOffset();
    setBefore();
    if (computerBraille)
      if (src >= computerBraille)
        if (CTC(*src, CTC_Space) || CTC(src[-1], CTC_Space))
          computerBraille = NULL;
    if (computerBraille) {
      setAfter(1);
      currentOpcode = CTO_Literal;
      if (!putBytes(&textTable[*src], 1)) break;
      src++;
    } else if (selectRule(srcmax-src) || selectRule(1)) {
      if (table->locale &&
          (cursorOffset >= (src - srcmin)) &&
          (cursorOffset < (src - srcmin + currentFindLength))) {
        currentOpcode = CTO_Literal;
      } else if (table->numberSign && previousOpcode != CTO_MidNum &&
                 !CTC(before, CTC_Digit) && CTC(*src, CTC_Digit)) {
        if (!putSequence(table->numberSign)) break;
      } else if (table->englishLetterSign && CTC(*src, CTC_Letter)) {
        if ((currentOpcode == CTO_Contraction) ||
            (currentOpcode != CTO_EndNum && CTC(before, CTC_Digit)) ||
            (currentOpcode == CTO_Always && currentFindLength == 1 && CTC(before, CTC_Space) &&
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
      switch (currentOpcode) {
        case CTO_LargeSign:
          if (previousOpcode == CTO_LargeSign) {
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
      switch (currentOpcode) {
        case CTO_Replace:
          src += currentFindLength;
          if (!putCharacters(&currentRule->findrep[currentFindLength], currentRule->replen)) goto done;
          break;
        case CTO_Literal: {
          const BYTE *srcorig = src;
          computerBraille = src + currentFindLength;
          if (CTC(*src, CTC_Space)) continue;
          if (destword) {
            src = srcword;
            dest = destword;
          } else {
            src = srcmin;
            dest = destmin;
          }
          while (srcorig > src) offsets[--srcorig - srcmin] = -1;
          continue;
        }
        default:
          if (currentRule->replen) {
            if (!putBytes(&currentRule->findrep[currentFindLength], currentRule->replen)) goto done;
            src += currentFindLength;
          } else {
            const BYTE *srclim = src + currentFindLength;
            while (1) {
              if (!putCharacter(*src)) goto done;
              if (++src == srclim) break;
              setOffset();
            }
          }
      }

      /* post processing */
      switch (currentOpcode) {
        case CTO_Repeated: {
          const BYTE *srclim = srcmax - currentFindLength;
          setOffset();
          while ((src <= srclim) && compareBytes(currentRule->findrep, src, currentFindLength)) {
            src += currentFindLength;
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

    if (((src > srcmin) && CTC(src[-1], CTC_Space) && (currentOpcode != CTO_JoinableWord))) {
      srcword = src;
      destword = dest;
    }
    if ((dest == destmin) || dest[-1])
      previousOpcode = currentOpcode;
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
