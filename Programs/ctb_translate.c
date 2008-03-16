/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "prologue.h"

#include <string.h>

#ifdef HAVE_LIBICUUC
#include <unicode/uchar.h>
#endif /* HAVE_LIBICUUC */

#include "tbl.h"
#include "ctb.h"
#include "ctb_internal.h"
#include "brl.h"

static ContractionTable *table;
static const wchar_t *src, *srcmin, *srcmax;
static BYTE *dest, *destmin, *destmax;
static int *offsets;
static wchar_t before, after;	/*the characters before and after a string */
static int currentFindLength;		/*length of current find string */
static ContractionTableOpcode currentOpcode;
static ContractionTableOpcode previousOpcode;
static const ContractionTableRule *currentRule;	/*pointer to current rule in table */

#define setOffset() offsets[src - srcmin] = dest - destmin

static const ContractionTableCharacter *
getContractionTableCharacter (wchar_t character) {
  const ContractionTableCharacter *characters = getContractionTableItem(table->header, table->header->characters);
  int first = 0;
  int last = table->header->characterCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    const ContractionTableCharacter *ctc = &characters[current];

    if (ctc->value < character) {
      first = current + 1;
    } else if (ctc->value > character) {
      last = current - 1;
    } else {
      return ctc;
    }
  }

  return NULL;
}

static CharacterEntry *
getCharacterEntry (wchar_t character) {
  int first = 0;
  int last = table->characterCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    CharacterEntry *entry = &table->characters[current];

    if (entry->value < character) {
      first = current + 1;
    } else if (entry->value > character) {
      last = current - 1;
    } else {
      return entry;
    }
  }

  if (table->characterCount == table->charactersSize) {
    int newSize = table->charactersSize;
    newSize = newSize? newSize<<1: 0X80;

    {
      CharacterEntry *newCharacters = realloc(table->characters, (newSize * sizeof(*newCharacters)));
      if (!newCharacters) return NULL;

      table->characters = newCharacters;
      table->charactersSize = newSize;
    }
  }

  memmove(&table->characters[first+1],
          &table->characters[first],
          (table->characterCount - first) * sizeof(*table->characters));
  table->characterCount += 1;

  {
    CharacterEntry *entry = &table->characters[first];
    memset(entry, 0, sizeof(*entry));
    entry->value = entry->uppercase = entry->lowercase = character;

    if (iswspace(character)) {
      entry->attributes |= CTC_Space;
    } else if (iswalpha(character)) {
      entry->attributes |= CTC_Letter;

      if (iswupper(character)) {
        entry->attributes |= CTC_UpperCase;
        entry->lowercase = towlower(character);
      }

      if (iswlower(character)) {
        entry->attributes |= CTC_LowerCase;
        entry->uppercase = towupper(character);
      }
    } else if (iswdigit(character)) {
      entry->attributes |= CTC_Digit;
    } else if (iswpunct(character)) {
      entry->attributes |= CTC_Punctuation;
    }

    {
      const ContractionTableCharacter *ctc = getContractionTableCharacter(character);
      if (ctc) entry->attributes |= ctc->attributes;
    }

    return entry;
  }
}

static int
testCharacter (wchar_t character, ContractionTableCharacterAttributes attributes) {
  const CharacterEntry *entry = getCharacterEntry(character);
  return entry && (attributes & entry->attributes);
}

static wchar_t
toLowerCase (wchar_t character) {
  const CharacterEntry *entry = getCharacterEntry(character);
  return entry? entry->lowercase: character;
}

static int
checkCurrentRule (const wchar_t *source) {
  const wchar_t *character = currentRule->findrep;
  int count = currentFindLength;

  while (count) {
    if (toLowerCase(*source) != toLowerCase(*character)) return 0;
    --count, ++source, ++character;
  }
  return 1;
}

static void
setBefore (void) {
  before = (src == srcmin)? WC_C(' '): src[-1];
}

static void
setAfter (int length) {
  after = (src + length < srcmax)? src[length]: WC_C(' ');
}

static int
selectRule (int length) {
  int ruleOffset;
  int maximumLength;

  if (length < 1) return 0;
  if (length == 1) {
    const ContractionTableCharacter *ctc = getContractionTableCharacter(toLowerCase(*src));
    if (!ctc) return 0;
    ruleOffset = ctc->rules;
    maximumLength = 1;
  } else {
    wchar_t characters[2];
    characters[0] = toLowerCase(src[0]);
    characters[1] = toLowerCase(src[1]);
    ruleOffset = table->header->rules[CTH(characters)];
    maximumLength = 0;
  }

  while (ruleOffset) {
    currentRule = getContractionTableItem(table->header, ruleOffset);
    currentOpcode = currentRule->opcode;
    currentFindLength = currentRule->findlen;

    if ((length == 1) ||
        ((currentFindLength <= length) &&
         checkCurrentRule(src))) {
      setAfter(currentFindLength);

      if (!maximumLength) {
        typedef enum {CS_Any, CS_Lower, CS_UpperSingle, CS_UpperMultiple} CapitalizationState;
#define STATE(c) (testCharacter((c), CTC_UpperCase)? CS_UpperSingle: testCharacter((c), CTC_LowerCase)? CS_Lower: CS_Any)
        CapitalizationState current = STATE(before);
        int i;
        maximumLength = currentFindLength;

        for (i=0; i<currentFindLength; ++i) {
          wchar_t character = src[i];
          CapitalizationState next = STATE(character);

          if ((i > 0) &&
              (((current == CS_Lower) && (next == CS_UpperSingle)) ||
               ((current == CS_UpperMultiple) && (next == CS_Lower)))) {
            maximumLength = i;
            break;
          }

          if ((current > CS_Lower) && (next == CS_UpperSingle)) {
            current = CS_UpperMultiple;
          } else if (next != CS_Any) {
            current = next;
          } else if (current == CS_Any) {
            current = CS_Lower;
          }
        }

#undef STATE
      }

      if ((currentFindLength <= maximumLength) &&
          (!currentRule->after || testCharacter(before, currentRule->after)) &&
          (!currentRule->before || testCharacter(after, currentRule->before))) {
        switch (currentOpcode) {
          case CTO_Always:
          case CTO_Repeated:
          case CTO_Literal:
            return 1;
          case CTO_LargeSign:
          case CTO_LastLargeSign:
            if (!testCharacter(before, CTC_Space) || !testCharacter(after, CTC_Space))
              currentOpcode = CTO_Always;
            return 1;
          case CTO_WholeWord:
          case CTO_Contraction:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Space|CTC_Punctuation))
              return 1;
            break;
          case CTO_LowWord:
            if (testCharacter(before, CTC_Space) && testCharacter(after, CTC_Space) &&
                (previousOpcode != CTO_JoinableWord) &&
                ((dest == destmin) || !dest[-1]))
              return 1;
            break;
          case CTO_JoinableWord:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                (before != '-') &&
                testCharacter(after, CTC_Space) &&
                (dest + currentRule->replen < destmax)) {
              const wchar_t *ptr = src + currentFindLength + 1;
              while (ptr < srcmax) {
                if (!testCharacter(*ptr, CTC_Space)) {
                  if (testCharacter(*ptr, CTC_Letter)) return 1;
                  break;
                }
                ptr++;
              }
            }
            break;
          case CTO_SuffixableWord:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Space|CTC_Letter|CTC_Punctuation))
              return 1;
            break;
          case CTO_PrefixableWord:
            if (testCharacter(before, CTC_Space|CTC_Letter|CTC_Punctuation) &&
                testCharacter(after, CTC_Space|CTC_Punctuation))
              return 1;
            break;
          case CTO_BegWord:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Letter))
              return 1;
            break;
          case CTO_BegMidWord:
            if (testCharacter(before, CTC_Letter|CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Letter))
              return 1;
            break;
          case CTO_MidWord:
            if (testCharacter(before, CTC_Letter) && testCharacter(after, CTC_Letter))
              return 1;
            break;
          case CTO_MidEndWord:
            if (testCharacter(before, CTC_Letter) &&
                testCharacter(after, CTC_Letter|CTC_Space|CTC_Punctuation))
              return 1;
            break;
          case CTO_EndWord:
            if (testCharacter(before, CTC_Letter) &&
                testCharacter(after, CTC_Space|CTC_Punctuation))
              return 1;
            break;
          case CTO_BegNum:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Digit))
              return 1;
            break;
          case CTO_MidNum:
            if (testCharacter(before, CTC_Digit) && testCharacter(after, CTC_Digit))
              return 1;
            break;
          case CTO_EndNum:
            if (testCharacter(before, CTC_Digit) &&
                testCharacter(after, CTC_Space|CTC_Punctuation))
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
            if (testCharacter(*src, CTC_Punctuation)) {
              const wchar_t *pre = src;
              const wchar_t *post = src + currentFindLength;
              while (--pre >= srcmin)
                if (!testCharacter(*pre, CTC_Punctuation))
                  break;
              while (post < srcmax) {
                if (!testCharacter(*post, CTC_Punctuation)) break;
                post++;
              }
              if (isPre) {
                if (((pre < srcmin) || testCharacter(*pre, CTC_Space)) &&
                    ((post < srcmax) && !testCharacter(*post, CTC_Space)))
                  return 1;
              } else {
                if (((pre >= srcmin) && !testCharacter(*pre, CTC_Space)) &&
                    ((post == srcmax) || testCharacter(*post, CTC_Space)))
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
putReplace (const ContractionTableRule *rule) {
  return putBytes((BYTE *)&rule->findrep[rule->findlen], rule->replen);
}

static int
putComputerBraille (wchar_t character) {
  BYTE cell = convertWcharToDots(textTable, character);
  return putBytes(&cell, 1);
}

static int
putCharacter (wchar_t character) {
  const ContractionTableCharacter *ctc = getContractionTableCharacter(character);
  if (ctc) {
    ContractionTableOffset offset = ctc->always;
    if (offset) {
      const ContractionTableRule *rule = getContractionTableItem(table->header, offset);
      if (rule->replen) return putReplace(rule);
    }
  }
  return putComputerBraille(character);
}

static int
putSequence (ContractionTableOffset offset) {
  const BYTE *sequence = getContractionTableItem(table->header, offset);
  return putBytes(sequence+1, *sequence);
}

static void
findLineBreakOpportunities (unsigned char *opportunities, const wchar_t *characters, size_t length) {
#ifdef HAVE_LIBICUUC
  /* UAX #14: Line Breaking Properties
   * http://unicode.org/reports/tr14/
   * Section 6: Line Breaking Algorithm
   *
   * !  Mandatory break at the indicated position
   * ^  No break allowed at the indicated position
   * _  Break allowed at the indicated position
   *
   * H  ideographs
   * h  small kana
   * 9  digits
   */

  if (length > 0) {
    ULineBreak lineBreakTypes[length];

    /* LB1: Assign a line breaking class to each code point of the input.
     */
    {
      int i;
      for (i=0; i<length; i+=1) {
        lineBreakTypes[i] = u_getIntPropertyValue(characters[i], UCHAR_LINE_BREAK);
      }
    }

    /* LB10: Treat any remaining combining mark as AL.
     */
    if (lineBreakTypes[0] == U_LB_COMBINING_MARK) lineBreakTypes[0] = U_LB_ALPHABETIC;

    /* LB2: Never break at the start of text.
     * sot ×
     */
    opportunities[0] = 0;

    {
      ULineBreak indirect = U_LB_SPACE;
      int limit = length - 1;
      int index = 0;

      while (index < limit) {
        ULineBreak before = lineBreakTypes[index];
        ULineBreak after = lineBreakTypes[++index];
        unsigned char *opportunity = &opportunities[index];

        if (before != U_LB_SPACE) indirect = before;

        /* LB4: Always break after hard line breaks
         * BK !
         */
        if (before == U_LB_MANDATORY_BREAK) {
          *opportunity = 1;
          continue;
        }

        /* LB5: Treat CR followed by LF, as well as CR, LF, and NL as hard line breaks.
         * CR ^ LF
         * CR !
         * LF !
         * NL !
         */
        if ((before == U_LB_CARRIAGE_RETURN) && (after == U_LB_LINE_FEED)) {
          *opportunity = 0;
          continue;
        }
        if ((before == U_LB_CARRIAGE_RETURN) ||
            (before == U_LB_LINE_FEED) ||
            (before == U_LB_NEXT_LINE)) {
          *opportunity = 1;
          continue;
        }

        /* LB6: Do not break before hard line breaks.
         * ^ ( BK | CR | LF | NL )
         */
        if ((after == U_LB_MANDATORY_BREAK) ||
            (after == U_LB_CARRIAGE_RETURN) ||
            (after == U_LB_LINE_FEED) ||
            (after == U_LB_NEXT_LINE)) {
          *opportunity = 0;
          continue;
        }

        /* LB7: Do not break before spaces or zero width space.
         * ^ SP
         * ^ ZW
         */
        if ((after == U_LB_SPACE) || (after == U_LB_ZWSPACE)) {
          *opportunity = 0;
          continue;
        }

        /* LB8: Break after zero width space.
         * ZW _
         */
        if (before == U_LB_ZWSPACE) {
          *opportunity = 1;
          continue;
        }

        /* LB9  Do not break a combining character sequence.
         */
        if (after == U_LB_COMBINING_MARK) {
          /* LB10: Treat any remaining combining mark as AL.
           */
          if ((before == U_LB_MANDATORY_BREAK) ||
              (before == U_LB_CARRIAGE_RETURN) ||
              (before == U_LB_LINE_FEED) ||
              (before == U_LB_NEXT_LINE) ||
              (before == U_LB_SPACE) ||
              (before == U_LB_ZWSPACE)) {
            before = U_LB_ALPHABETIC;
          }

          /* treat it as if it has the line breaking class of the base character
           */
          lineBreakTypes[index] = before;
          *opportunity = 0;
          continue;
        }

        /* LB11: Do not break before or after Word joiner and related characters.
         * ^ WJ
         * WJ ^
         */
        if ((before == U_LB_WORD_JOINER) || (after == U_LB_WORD_JOINER)) {
          *opportunity = 0;
          continue;
        }

        /* LB12: Do not break before or after NBSP and related characters.
         * [^SP] ^ GL
         * GL ^
         */
        if ((before != U_LB_SPACE) && (after == U_LB_GLUE)) {
          *opportunity = 0;
          continue;
        }
        if (before == U_LB_GLUE) {
          *opportunity = 0;
          continue;
        }

        /* LB13: Do not break before ‘]' or ‘!' or ‘;' or ‘/', even after spaces.
         * ^ CL
         * ^ EX
         * ^ IS
         * ^ SY
         */
        if ((after == U_LB_CLOSE_PUNCTUATION) ||
            (after == U_LB_EXCLAMATION) ||
            (after == U_LB_INFIX_NUMERIC) ||
            (after == U_LB_BREAK_SYMBOLS)) {
          *opportunity = 0;
          continue;
        }

        /* LB14: Do not break after ‘[', even after spaces.
         * OP SP* ^
         */
        if (indirect == U_LB_OPEN_PUNCTUATION) {
          *opportunity = 0;
          continue;
        }

        /* LB15: Do not break within ‘"[', even with intervening spaces.
         * QU SP* ^ OP
         */
        if ((indirect == U_LB_QUOTATION) && (after == U_LB_OPEN_PUNCTUATION)) {
          *opportunity = 0;
          continue;
        }

        /* LB16: Do not break within ‘]h', even with intervening spaces.
         * CL SP* ^ NS
         */
        if ((indirect == U_LB_CLOSE_PUNCTUATION) && (after == U_LB_NONSTARTER)) {
          *opportunity = 0;
          continue;
        }

        /* LB17: Do not break within ‘ــ', even with intervening spaces.
         * B2 SP* ^ B2
         */
        if ((indirect == U_LB_BREAK_BOTH) && (after == U_LB_BREAK_BOTH)) {
          *opportunity = 0;
          continue;
        }

        /* LB18: Break after spaces.
         * SP _
         */
        if (before == U_LB_SPACE) {
          *opportunity = 1;
          continue;
        }

        /* LB19: Do not break before or after  quotation marks.
         * ^ QU
         * QU ^
         */
        if ((before == U_LB_QUOTATION) || (after == U_LB_QUOTATION)) {
          *opportunity = 0;
          continue;
        }

        /* LB20: Break before and after unresolved.
         * _ CB
         * CB _
         */
        if ((after == U_LB_CONTINGENT_BREAK) || (before == U_LB_CONTINGENT_BREAK)) {
          *opportunity = 1;
          continue;
        }

        /* LB21: Do not break before hyphen-minus, other hyphens,
         *       fixed-width spaces, small kana, and other non-starters,
         *       or after acute accents.
         * ^ BA
         * ^ HY
         * ^ NS
         * BB ^
         */
        if ((after == U_LB_BREAK_AFTER) ||
            (after == U_LB_HYPHEN) ||
            (after == U_LB_NONSTARTER) ||
            (before == U_LB_BREAK_BEFORE)) {
          *opportunity = 0;
          continue;
        }

        /* LB22: Do not break between two ellipses,
         *       or between letters or numbers and ellipsis.
         * AL ^ IN
         * ID ^ IN
         * IN ^ IN
         * NU ^ IN
         */
        if ((after == U_LB_INSEPARABLE) &&
            ((before == U_LB_ALPHABETIC) ||
             (before == U_LB_IDEOGRAPHIC) ||
             (before == U_LB_INSEPARABLE) ||
             (before == U_LB_NUMERIC))) {
          *opportunity = 0;
          continue;
        }

        /* LB23: Do not break within ‘a9', ‘3a', or ‘H%'.
         * ID ^ PO
         * AL ^ NU
         * NU ^ AL
         */
        if (((before == U_LB_IDEOGRAPHIC) && (after == U_LB_POSTFIX_NUMERIC)) ||
            ((before == U_LB_ALPHABETIC) && (after == U_LB_NUMERIC)) ||
            ((before == U_LB_NUMERIC) && (after == U_LB_ALPHABETIC))) {
          *opportunity = 0;
          continue;
        }

        /* LB24: Do not break between prefix and letters or ideographs.
         * PR ^ ID
         * PR ^ AL
         * PO ^ AL
         */
        if (((before == U_LB_PREFIX_NUMERIC) && (after == U_LB_IDEOGRAPHIC)) ||
            ((before == U_LB_PREFIX_NUMERIC) && (after == U_LB_ALPHABETIC)) ||
            ((before == U_LB_POSTFIX_NUMERIC) && (after == U_LB_ALPHABETIC))) {
          *opportunity = 0;
          continue;
        }

        /* LB25:  Do not break between the following pairs of classes relevant to numbers:
         * CL ^ PO
         * CL ^ PR
         * NU ^ PO
         * NU ^ PR
         * PO ^ OP
         * PO ^ NU
         * PR ^ OP
         * PR ^ NU
         * HY ^ NU
         * IS ^ NU
         * NU ^ NU
         * SY ^ NU
         */
        if (((before == U_LB_CLOSE_PUNCTUATION) && (after == U_LB_POSTFIX_NUMERIC)) ||
            ((before == U_LB_CLOSE_PUNCTUATION) && (after == U_LB_PREFIX_NUMERIC)) ||
            ((before == U_LB_NUMERIC) && (after == U_LB_POSTFIX_NUMERIC)) ||
            ((before == U_LB_NUMERIC) && (after == U_LB_PREFIX_NUMERIC)) ||
            ((before == U_LB_POSTFIX_NUMERIC) && (after == U_LB_OPEN_PUNCTUATION)) ||
            ((before == U_LB_POSTFIX_NUMERIC) && (after == U_LB_NUMERIC)) ||
            ((before == U_LB_PREFIX_NUMERIC) && (after == U_LB_OPEN_PUNCTUATION)) ||
            ((before == U_LB_PREFIX_NUMERIC) && (after == U_LB_NUMERIC)) ||
            ((before == U_LB_HYPHEN) && (after == U_LB_NUMERIC)) ||
            ((before == U_LB_INFIX_NUMERIC) && (after == U_LB_NUMERIC)) ||
            ((before == U_LB_NUMERIC) && (after == U_LB_NUMERIC)) ||
            ((before == U_LB_BREAK_SYMBOLS) && (after == U_LB_NUMERIC))) {
          *opportunity = 0;
          continue;
        }

        /* LB26: Do not break a Korean syllable.
         * JL ^ (JL | JV | H2 | H3)
         * (JV | H2) ^ (JV | JT)
         * (JT | H3) ^ JT
         */
        if ((before == U_LB_JL) &&
            ((after == U_LB_JL) ||
             (after == U_LB_JV) ||
             (after == U_LB_H2) ||
             (after == U_LB_H3))) {
          *opportunity = 0;
          continue;
        }
        if (((before == U_LB_JV) || (before == U_LB_H2)) &&
            ((after == U_LB_JV) || (after == U_LB_JT))) {
          *opportunity = 0;
          continue;
        }
        if (((before == U_LB_JT) || (before == U_LB_H3)) &&
            (after == U_LB_JT)) {
          *opportunity = 0;
          continue;
        }

        /* LB27: Treat a Korean Syllable Block the same as ID.
         * (JL | JV | JT | H2 | H3) ^ IN
         * (JL | JV | JT | H2 | H3) ^ PO
         * PR ^ (JL | JV | JT | H2 | H3)
         */
        if (((before == U_LB_JL) || (before == U_LB_JV) || (before == U_LB_JT) ||
             (before == U_LB_H2) || (before == U_LB_H3)) &&
            (after == U_LB_INSEPARABLE)) {
          *opportunity = 0;
          continue;
        }
        if (((before == U_LB_JL) || (before == U_LB_JV) || (before == U_LB_JT) ||
             (before == U_LB_H2) || (before == U_LB_H3)) &&
            (after == U_LB_POSTFIX_NUMERIC)) {
          *opportunity = 0;
          continue;
        }
        if ((before == U_LB_PREFIX_NUMERIC) &&
            ((after == U_LB_JL) || (after == U_LB_JV) || (after == U_LB_JT) ||
             (after == U_LB_H2) || (after == U_LB_H3))) {
          *opportunity = 0;
          continue;
        }

        /* LB28: Do not break between alphabetics.
         * AL ^ AL
         */
        if ((before == U_LB_ALPHABETIC) && (after == U_LB_ALPHABETIC)) {
          *opportunity = 0;
          continue;
        }

        /* LB29: Do not break between numeric punctuation and alphabetics.
         * IS ^ AL
         */
        if ((before == U_LB_INFIX_NUMERIC) && (after == U_LB_ALPHABETIC)) {
          *opportunity = 0;
          continue;
        }

        /* LB30: Do not break between letters, numbers, or ordinary symbols
         *       and opening or closing punctuation.
         * (AL | NU) ^ OP
         * CL ^ (AL | NU)
         */
        if (((before == U_LB_ALPHABETIC) || (before == U_LB_NUMERIC)) &&
            (after == U_LB_OPEN_PUNCTUATION)) {
          *opportunity = 0;
          continue;
        }
        if ((before == U_LB_CLOSE_PUNCTUATION) &&
            ((after == U_LB_ALPHABETIC) || (after == U_LB_NUMERIC))) {
          *opportunity = 0;
          continue;
        }

        /* Unix options begin with a minus sign. */
        if ((before == U_LB_HYPHEN) && (after != U_LB_SPACE)) {
          if ((index >= 2) && (lineBreakTypes[index-2] == U_LB_SPACE)) {
            *opportunity = 0;
            continue;
          }
        }

        /* LB31: Break everywhere else.
         * ALL _
         * _ ALL
         */
        *opportunity = 1;
      }
    }
  }
#else /* HAVE_LIBICUUC */
  int wasSpace = 0;
  int index;

  for (index=0; index<length; index+=1) {
    int isSpace = testCharacter(characters[index], CTC_Space);
    opportunities[index] = wasSpace && !isSpace;
    wasSpace = isSpace;
  }
#endif /* HAVE_LIBICUUC */
}

int
contractText (
  ContractionTable *contractionTable,
  const wchar_t *inputBuffer, int *inputLength,
  BYTE *outputBuffer, int *outputLength,
  int *offsetsMap, const int cursorOffset
) {
  const wchar_t *srcword = NULL;
  BYTE *destword = NULL; /* last word transla1ted */
  BYTE *destlast = NULL;
  const wchar_t *literal = NULL;
  unsigned char lineBreakOpportunities[*inputLength];

  table = contractionTable;
  srcmax = (srcmin = src = inputBuffer) + *inputLength;
  destmax = (destmin = dest = outputBuffer) + *outputLength;
  offsets = offsetsMap;
  previousOpcode = CTO_None;

  findLineBreakOpportunities(lineBreakOpportunities, inputBuffer, *inputLength);

  while (src < srcmax) {
    destlast = dest;
    setOffset();
    setBefore();

    if (literal)
      if (src >= literal)
        if (testCharacter(*src, CTC_Space) || testCharacter(src[-1], CTC_Space))
          literal = NULL;

    if ((!literal && selectRule(srcmax-src)) || selectRule(1)) {
      if ((cursorOffset >= (src - srcmin)) &&
          (cursorOffset < (src - srcmin + currentFindLength))) {
        currentOpcode = CTO_Literal;
      }

      if (table->header->numberSign && previousOpcode != CTO_MidNum &&
                 !testCharacter(before, CTC_Digit) && testCharacter(*src, CTC_Digit)) {
        if (!putSequence(table->header->numberSign)) break;
      } else if (table->header->englishLetterSign && testCharacter(*src, CTC_Letter)) {
        if ((currentOpcode == CTO_Contraction) ||
            (currentOpcode != CTO_EndNum && testCharacter(before, CTC_Digit)) ||
            (currentOpcode == CTO_Always && currentFindLength == 1 && testCharacter(before, CTC_Space) &&
             (src + 1 == srcmax || testCharacter(src[1], CTC_Space) ||
              (testCharacter(src[1], CTC_Punctuation) && (src[1] != '.') && (src[1] != '\''))))) {
          if (!putSequence(table->header->englishLetterSign)) break;
        }
      }

      if (testCharacter(*src, CTC_UpperCase)) {
        if (!testCharacter(before, CTC_UpperCase)) {
          if (table->header->beginCapitalSign &&
              (src + 1 < srcmax) && testCharacter(src[1], CTC_UpperCase)) {
            if (!putSequence(table->header->beginCapitalSign)) break;
          } else if (table->header->capitalSign) {
            if (!putSequence(table->header->capitalSign)) break;
          }
        }
      } else if (testCharacter(*src, CTC_LowerCase)) {
        if (table->header->endCapitalSign && (src - 2 >= srcmin) &&
            testCharacter(src[-1], CTC_UpperCase) && testCharacter(src[-2], CTC_UpperCase)) {
          if (!putSequence(table->header->endCapitalSign)) break;
        }
      }

      /* preprocessing */
      switch (currentOpcode) {
        case CTO_LargeSign:
        case CTO_LastLargeSign:
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
      if (!literal && (currentOpcode == CTO_Literal)) {
        const wchar_t *srcorig = src;

        literal = src + currentFindLength;
        if (testCharacter(*src, CTC_Space)) continue;

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

      if (currentRule->replen &&
          !((currentOpcode == CTO_Always) && (currentFindLength == 1))) {
        if (!putReplace(currentRule)) goto done;
        src += currentFindLength;
      } else {
        const wchar_t *srclim = src + currentFindLength;
        while (1) {
          if (!putCharacter(*src)) goto done;
          if (++src == srclim) break;
          setOffset();
        }
      }

      /* postprocessing */
      switch (currentOpcode) {
        case CTO_Repeated: {
          const wchar_t *srclim = srcmax - currentFindLength;
          setOffset();
          while ((src <= srclim) && checkCurrentRule(src)) {
            src += currentFindLength;
          }
          break;
        }

        case CTO_JoinableWord:
          while ((src < srcmax) && testCharacter(*src, CTC_Space)) src++;
          break;

        default:
          break;
      }
    } else {
      if (!putComputerBraille(*src)) break;
      src++;
    }

    if (lineBreakOpportunities[src-srcmin] && (currentOpcode != CTO_JoinableWord)) {
      srcword = src;
      destword = dest;
    }

    if ((dest == destmin) || dest[-1]) {
      previousOpcode = currentOpcode;
    }
  }				/*end of translation loop */

done:
  if (destlast) dest = destlast;

  if ((src < srcmax) && destword &&
      ((destmax - destword) < ((destmax - destmin) / 2))) {
    src = srcword;
    dest = destword;
  }

  if (src < srcmax) {
    const wchar_t *srcorig = src;
    setOffset();
    do {
      if (!testCharacter(*src, CTC_Space)) {
        src = srcorig;
        break;
      }
    } while (++src < srcmax);
  }

  *inputLength = src - srcmin;
  *outputLength = dest - destmin;
  return 1;
}				/*translation completed */
